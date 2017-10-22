// Snap Websites Server -- handle Snap! files cassandra settings
// Copyright (C) 2016-2017  Made to Order Software Corp.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

// cassandra
//
#include "cassandra.h"

// our lib
//
#include "snapmanager/form.h"

// snapwebsites lib
//
#include <snapwebsites/chownnm.h>
#include <snapwebsites/file_content.h>
#include <snapwebsites/log.h>
#include <snapwebsites/not_reached.h>
#include <snapwebsites/not_used.h>
#include <snapwebsites/process.h>

// Qt lib
//
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QTextStream>

// casswrapper
//
#include <casswrapper/query.h>

// C++ lib
//
#include <fstream>
#include <vector>
#include <sys/stat.h>

// YAML-CPP
//
#include <yaml-cpp/yaml.h>

// last entry
//
#include <snapwebsites/poison.h>


SNAP_PLUGIN_START(cassandra, 1, 0)


namespace
{

const QString g_ssl_keys_dir        = "/etc/cassandra/ssl/";
const QString g_cassandra_yaml      = "/etc/cassandra/cassandra.yaml";
const QString g_keystore_password   = "qZ0LK74eiPecWcTQJCX2";
const QString g_truststore_password = g_keystore_password; //"fu87kxWq4ktrkuZqVLQX";

QString get_local_listen_address()
{
    std::string listen_address;
    try
    {
        YAML::Node node = YAML::LoadFile( g_cassandra_yaml.toUtf8().data() );
        listen_address = node["listen_address"].Scalar();
    }
    catch( std::exception const& x )
    {
        SNAP_LOG_ERROR("'listen_address' is not defined in your cassandra.yaml! Cannot generate keys! Reason=[")(x.what())("]");
    }
    catch( ... )
    {
        SNAP_LOG_ERROR("'listen_address' is not defined in your cassandra.yaml! Cannot generate keys!");
    }

    return listen_address.c_str();
}

YAML::Node read_node_from_yaml()
{
    SNAP_LOG_TRACE("read_node_from_yaml()");
    return YAML::LoadFile( g_cassandra_yaml.toUtf8().data() );
}

void write_node_to_yaml( const YAML::Node& node )
{
    SNAP_LOG_TRACE("write_node_to_yaml()");
    QString const header_comment(
            QString("Automatically generated file on '%1'. Do not modify!")
            .arg( QDateTime::currentDateTime().toString( Qt::LocalDate ) ) );

    YAML::Emitter emitter;
    emitter.SetIndent( 4 );
    emitter.SetMapFormat( YAML::Flow );
    emitter
        << YAML::Comment(header_comment.toUtf8().data())
        << node
        // SNAP-497: apparently, you have to specifically tell the emitter
        // that you want a newline in the output.
        << YAML::Newline
        << YAML::Comment("vim: ts=4 sw=4 et");

    QFile file( g_cassandra_yaml );
    if( !file.open( QIODevice::WriteOnly | QIODevice::Truncate ) )
    {
        QString const errmsg = QString("Cannot open '%1' for writing!").arg(file.fileName());
        SNAP_LOG_ERROR(qPrintable(errmsg));
        return;
    }
    //
    QTextStream out( &file );
    out << emitter.c_str();
}


void create_seed_field( snap_manager::server_status & server_status, YAML::Node node, QString const & plugin_name )
{
    try
    {
        QString previous_value;
        //
        for( auto seq : node["seed_provider"] )
        {
            for( auto subseq : seq["parameters"] )
            {
                previous_value = QString::fromUtf8(subseq["seeds"].Scalar().c_str());
                break;
            }
        }

        snap_manager::status_t const conf_field
                ( snap_manager::status_t::state_t::STATUS_STATE_INFO
                  , plugin_name
                  , "seeds"
                  , previous_value
                  );
        server_status.set_field(conf_field);
        return;
    }
    catch( std::exception const& x )
    {
        SNAP_LOG_ERROR("create_seed_field() exception caught! what=[")(x.what())("]");
    }
    catch( ... )
    {
        SNAP_LOG_ERROR("create_seed_field() unknown exception caught!");
    }

    // if we get here, that means that there was an error trying to read the yaml file
    //
    snap_manager::status_t const conf_field(
                snap_manager::status_t::state_t::STATUS_STATE_WARNING
                , plugin_name
                , "seeds"
                , QString("There was an error reading \"seeds\" from \"%1\"!").arg(g_cassandra_yaml));
    server_status.set_field(conf_field);
}


void create_field( snap_manager::server_status & server_status, YAML::Node node, QString const & plugin_name, std::string const & parameter_name )
{
    try
    {
        snap_manager::status_t const conf_field
                ( snap_manager::status_t::state_t::STATUS_STATE_INFO
                  , plugin_name
                  , QString::fromUtf8(parameter_name.c_str())
                  , node[parameter_name].IsDefined()
                  ? QString::fromUtf8(node[parameter_name].Scalar().c_str())
                  : QString()
                    );
        server_status.set_field(conf_field);
        return;
    }
    catch( std::exception const& x )
    {
        SNAP_LOG_ERROR("create_field() exception caught! what=[")(x.what())("]");
    }
    catch( ... )
    {
        SNAP_LOG_ERROR("create_field() unknown exception caught!");
    }

    // if we get here, that means that there was an error trying to read the yaml file
    //
    snap_manager::status_t const conf_field(
                snap_manager::status_t::state_t::STATUS_STATE_WARNING
                , plugin_name
                , QString::fromUtf8(parameter_name.c_str())
                , QString("There was an error reading \"%1\" from \"%2\"!").arg(parameter_name.c_str()).arg(g_cassandra_yaml));
    server_status.set_field(conf_field);
}




} // no name namespace



/** \brief Get a fixed cassandra plugin name.
 *
 * The cassandra plugin makes use of different fixed names. This function
 * ensures that you always get the right spelling for a given name.
 *
 * \param[in] name  The name to retrieve.
 *
 * \return A pointer to the name.
 */
char const * get_name(name_t name)
{
    switch(name)
    {
    case name_t::SNAP_NAME_SNAPMANAGERCGI_CASSANDRA_NAME:
        return "name";

    default:
        // invalid index
        throw snap_logic_exception("Invalid SNAP_NAME_SNAPMANAGERCGI_CASSANDRA_...");

    }
    NOTREACHED();
}




/** \brief Initialize the cassandra plugin.
 *
 * This function is used to initialize the cassandra plugin object.
 */
cassandra::cassandra()
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Clean up the cassandra plugin.
 *
 * Ensure the cassandra object is clean before it is gone.
 */
cassandra::~cassandra()
{
}


/** \brief Get a pointer to the cassandra plugin.
 *
 * This function returns an instance pointer to the cassandra plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the cassandra plugin.
 */
cassandra * cassandra::instance()
{
    return g_plugin_cassandra_factory.instance();
}


/** \brief Return the description of this plugin.
 *
 * This function returns the English description of this plugin.
 * The system presents that description when the user is offered to
 * install or uninstall a plugin on his website. Translation may be
 * available in the database.
 *
 * \return The description in a QString.
 */
QString cassandra::description() const
{
    return "Handle the settings in the cassandra.yaml file.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString cassandra::dependencies() const
{
    return "|server|";
}


/** \brief Check whether updates are necessary.
 *
 * This function is ignored in snapmanager.cgi and snapmanagerdaemon plugins.
 *
 * \param[in] last_updated  The UTC Unix date when the website was last updated (in micro seconds).
 *
 * \return The UTC Unix date of the last update of this plugin.
 */
int64_t cassandra::do_update(int64_t last_updated)
{
    NOTUSED(last_updated);

    SNAP_PLUGIN_UPDATE_INIT();
    // no updating in snapmanager*
    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Initialize cassandra.
 *
 * This function terminates the initialization of the cassandra plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void cassandra::bootstrap(snap_child * snap)
{
    f_snap = dynamic_cast<snap_manager::manager *>(snap);
    if(f_snap == nullptr)
    {
        throw snap_logic_exception("snap pointer does not represent a valid manager object.");
    }

    SNAP_LISTEN  ( cassandra, "server", snap_manager::manager, retrieve_status,          _1     );
    SNAP_LISTEN  ( cassandra, "server", snap_manager::manager, handle_affected_services, _1     );
    SNAP_LISTEN  ( cassandra, "server", snap_manager::manager, add_plugin_commands,      _1     );
    SNAP_LISTEN  ( cassandra, "server", snap_manager::manager, process_plugin_message,   _1, _2 );
    SNAP_LISTEN0 ( cassandra, "server", snap_manager::manager, communication_ready              );
}

#if 0
void output_node( YAML::Node node, int const level = 0 )
{
    std::string prefix( level+1, ' ' );
    if( !node.IsDefined() )
    {
        std::cout << prefix << "node is not defined!" << std::endl;
        return;
    }

    if( node.IsNull() )
    {
        std::cout << prefix << "node is null!" << std::endl;
        return;
    }

    if( node.IsScalar() )
    {
        if( node.Tag() == "!" )
        {
            node.SetTag( "?" );
        }
        //
        std::cout << prefix << "node is a scalar: [" << node << "]" << std::endl;
        return;
    }

    if( node.IsSequence() )
    {
        std::cout << prefix << "node is a sequence:" << std::endl;
        for( auto seq : node )
        {
            output_node( seq, level + 1 );
        }

        return;
    }

    if( node.IsMap() )
    {
        std::cout << prefix << "node is a map:" << std::endl;
        for( auto map : node )
        {
            std::cout << prefix << "name=[" << map.first << "]" << std::endl;
            output_node( map.second, level + 1 );
        }
        return;
    }
}
#endif

/** \brief Determine this plugin status data.
 *
 * This function builds a tree of statuses.
 *
 * \param[in] server_status  The map of statuses.
 */
void cassandra::on_retrieve_status(snap_manager::server_status & server_status)
{
    if(f_snap->stop_now_prima())
    {
        return;
    }

    if(!is_installed())
    {
        return;
    }

    // get the data
    //
    QFile config_file(g_cassandra_yaml);
    if( !config_file.exists() )
    {
        // create an error field which is not editable
        //
        snap_manager::status_t const conf_field(
                    snap_manager::status_t::state_t::STATUS_STATE_WARNING
                    , get_plugin_name()
                    , "cassandra_yaml"
                    , QString("\"%1\" does not exist or cannot be read!").arg(g_cassandra_yaml));
        server_status.set_field(conf_field);
        return;
    }

    {
        snap_manager::status_t const conf_field(
                    snap_manager::status_t::state_t::STATUS_STATE_INFO
                    , get_plugin_name()
                    , "restart_cassandra"
                    , "");
        server_status.set_field(conf_field);
    }

    {
        snap_manager::status_t const conf_field(
                    snap_manager::status_t::state_t::STATUS_STATE_INFO
                    , get_plugin_name()
                    , "purge_ssl_keys"
                    , "");
        server_status.set_field(conf_field);
    }

    YAML::Node node = YAML::LoadFile( g_cassandra_yaml.toUtf8().data() );
    //
    create_seed_field ( server_status, node, get_plugin_name()  );
    create_field      ( server_status, node, get_plugin_name(), "cluster_name"          );
    create_field      ( server_status, node, get_plugin_name(), "listen_address"        );
    create_field      ( server_status, node, get_plugin_name(), "rpc_address"           );
    create_field      ( server_status, node, get_plugin_name(), "broadcast_rpc_address" );
    create_field      ( server_status, node, get_plugin_name(), "auto_snapshot"         );

    // also add a "join a cluster" field
    //
    // TODO: add the field ONLY if the node does not include a
    //       snap_websites context!
    {
        snap_manager::status_t const conf_field(
                    snap_manager::status_t::state_t::STATUS_STATE_INFO
                    , get_plugin_name()
                    , "join_a_cluster"
                    , "");
        server_status.set_field(conf_field);
    }

    // if joined, we want the user to be able to change the replication factor
    //
    {
        QString const replication_factor(get_replication_factor());
        snap_manager::status_t const conf_field(
                    snap_manager::status_t::state_t::STATUS_STATE_INFO
                    , get_plugin_name()
                    , "replication_factor"
                    , replication_factor);
        server_status.set_field(conf_field);
    }

    // Present the server SSL option (to allow node-to-node encryption).
    //
    {
        snap_manager::status_t const conf_field
                ( snap_manager::status_t::state_t::STATUS_STATE_INFO
                  , get_plugin_name()
                  , "use_server_ssl"
                  , node["server_encryption_options"]["internode_encryption"].Scalar().c_str()
                );
        server_status.set_field(conf_field);
    }

    // Present the client SSL option (to allow client-to-server encryption).
    //
    {
        snap_manager::status_t const conf_field
                ( snap_manager::status_t::state_t::STATUS_STATE_INFO
                  , get_plugin_name()
                  , "use_client_ssl"
                  , node["client_encryption_options"]["enabled"].Scalar().c_str()
                );
        server_status.set_field(conf_field);
    }
}


bool cassandra::is_installed()
{
    // for now we just check whether the executable is here, this is
    // faster than checking whether the package is installed.
    //
    return access("/usr/sbin/cassandra", R_OK | X_OK) == 0;
}



/** \brief Transform a value to HTML for display.
 *
 * This function expects the name of a field and its value. It then adds
 * the necessary HTML to the specified element to display that value.
 *
 * If the value is editable, then the function creates a form with the
 * necessary information (hidden fields) to save the data as required
 * by that field (i.e. update a .conf/.xml file, create a new file,
 * remove a file, etc.)
 *
 * \param[in,out] parent  The parent element where we add the fields.
 * \param[in] s  The field being worked on.
 * \param[in] uri  The URI to send the POST.
 *
 * \return true if we handled this field.
 */
bool cassandra::display_value(QDomElement parent, snap_manager::status_t const & s, snap::snap_uri const & uri)
{
    QDomDocument doc(parent.ownerDocument());

    if(s.get_field_name() == "restart_cassandra")
    {
        snap_manager::form f(
                  get_plugin_name()
                , s.get_field_name()
                ,   snap_manager::form::FORM_BUTTON_RESTART
                  | snap_manager::form::FORM_BUTTON_RESTART_EVERYWHERE
                );

        f.generate(parent, uri);
        return true;
    }

    if(s.get_field_name() == "purge_ssl_keys")
    {
        snap_manager::form f(
                  get_plugin_name()
                , s.get_field_name()
                ,   snap_manager::form::FORM_BUTTON_SAVE
                  | snap_manager::form::FORM_BUTTON_SAVE_EVERYWHERE
                );

        snap_manager::widget_input::pointer_t field(std::make_shared<snap_manager::widget_input>(
                          "Purge all SSL keys! Type in 'purge_ssl_keys' to engage, then click 'Save' or 'Save Everywhere'."
                        , s.get_field_name()
                        , ""
                        , "Be careful with this option--this will delete the entire /etc/cassandra/ssl directory"
                          " and blow away the public keys as well. It will regenerate new key pairs, and instruct snapdbproxy to accept"
                          " new versions of the key for this IP. It will not restart the Cassandra server, however"
                          " so you need to do that by hand (using the 'Restart Cassandra' option)."
                        ));
        f.add_widget(field);

        f.generate(parent, uri);
        return true;
    }

    if(s.get_field_name() == "cluster_name")
    {
        // in case it is not marked as INFO, it is "not editable" (we are
        // unsure of the current file format)
        //
        if(s.get_state() == snap_manager::status_t::state_t::STATUS_STATE_WARNING)
        {
            return false;
        }

        // the cluster name
        //
        snap_manager::form f(
                  get_plugin_name()
                , s.get_field_name()
                ,   snap_manager::form::FORM_BUTTON_RESET
                  | snap_manager::form::FORM_BUTTON_SAVE
                  | snap_manager::form::FORM_BUTTON_SAVE_EVERYWHERE
                );

        snap_manager::widget_input::pointer_t field(std::make_shared<snap_manager::widget_input>(
                          "Cassandra 'ClusterName'"
                        , s.get_field_name()
                        , s.get_value()
                        , "The name of the Cassandra cluster. All your Cassandra Nodes must be using the exact same name or they won't be able to join the cluster."
                        ));
        f.add_widget(field);

        f.generate(parent, uri);

        return true;
    }

    if(s.get_field_name() == "seeds")
    {
        // in case it is not marked as INFO, it is "not editable" (we are
        // unsure of the current file format)
        //
        if(s.get_state() == snap_manager::status_t::state_t::STATUS_STATE_WARNING)
        {
            return false;
        }

        // the list of seeds
        //
        snap_manager::form f(
                  get_plugin_name()
                , s.get_field_name()
                ,   snap_manager::form::FORM_BUTTON_RESET
                  //| snap_manager::form::FORM_BUTTON_SAVE
                  | snap_manager::form::FORM_BUTTON_SAVE_EVERYWHERE
                );

        snap_manager::widget_input::pointer_t field(std::make_shared<snap_manager::widget_input>(
                          "Cassandra Seeds"
                        , s.get_field_name()
                        , s.get_value()
                        , "This is a list of comma separated IP addresses representing Cassandra seeds."
                        ));
        f.add_widget(field);

        f.generate(parent, uri);

        return true;
    }

    if(s.get_field_name() == "listen_address")
    {
        // in case it is not marked as INFO, it is "not editable" (we are
        // unsure of the current file format)
        //
        if(s.get_state() == snap_manager::status_t::state_t::STATUS_STATE_WARNING)
        {
            return false;
        }

        // the address used to listen on client connections
        //
        snap_manager::form f(
                  get_plugin_name()
                , s.get_field_name()
                ,   snap_manager::form::FORM_BUTTON_RESET
                  | snap_manager::form::FORM_BUTTON_RESTORE_DEFAULT
                  | snap_manager::form::FORM_BUTTON_SAVE
                );

        snap_manager::widget_input::pointer_t field(std::make_shared<snap_manager::widget_input>(
                          "Cassandra Listen Address"
                        , s.get_field_name()
                        , s.get_value()
                        , "This is the Private IP Address of this computer, which Cassandra listens on for of Cassandra node connections."
                        ));
        f.add_widget(field);

        f.generate(parent, uri);

        return true;
    }

    if(s.get_field_name() == "rpc_address")
    {
        // in case it is not marked as INFO, it is "not editable" (we are
        // unsure of the current file format)
        //
        if(s.get_state() == snap_manager::status_t::state_t::STATUS_STATE_WARNING)
        {
            return false;
        }

        // the address used to listen for RPC calls (CQL)
        //
        snap_manager::form f(
                  get_plugin_name()
                , s.get_field_name()
                ,   snap_manager::form::FORM_BUTTON_RESET
                  | snap_manager::form::FORM_BUTTON_RESTORE_DEFAULT
                  | snap_manager::form::FORM_BUTTON_SAVE
                );

        snap_manager::widget_input::pointer_t field(std::make_shared<snap_manager::widget_input>(
                          "Cassandra RPC Address"
                        , s.get_field_name()
                        , s.get_value()
                        , "Most often, this is the Private IP Address of this computer, which Cassandra listens on for client connections. It is possible to set this address to 0.0.0.0 to listen for connections from anywhere. However, that is not considered safe and by default the firewall blocks the Cassandra port."
                        ));
        f.add_widget(field);

        f.generate(parent, uri);

        return true;
    }

    if(s.get_field_name() == "broadcast_rpc_address")
    {
        // in case it is not marked as INFO, it is "not editable" (we are
        // unsure of the current file format)
        //
        if(s.get_state() == snap_manager::status_t::state_t::STATUS_STATE_WARNING)
        {
            return false;
        }

        // the broadcast RCP address
        //
        snap_manager::form f(
                  get_plugin_name()
                , s.get_field_name()
                ,   snap_manager::form::FORM_BUTTON_RESET
                  | snap_manager::form::FORM_BUTTON_RESTORE_DEFAULT
                  | snap_manager::form::FORM_BUTTON_SAVE
                );

        snap_manager::widget_input::pointer_t field(std::make_shared<snap_manager::widget_input>(
                          "Cassandra Broadcast RPC Address"
                        , s.get_field_name()
                        , s.get_value()
                        , "This is the Private IP Address of this computer, which Cassandra uses to for broadcast information between Cassandra nodes and client connections."
                        ));
        f.add_widget(field);

        f.generate(parent, uri);

        return true;
    }

    if(s.get_field_name() == "auto_snapshot")
    {
        // in case it is not marked as INFO, it is "not editable" (we are
        // unsure of the current file format)
        //
        if(s.get_state() == snap_manager::status_t::state_t::STATUS_STATE_WARNING)
        {
            return false;
        }

        // the broadcast RCP address
        //
        snap_manager::form f(
                  get_plugin_name()
                , s.get_field_name()
                ,   snap_manager::form::FORM_BUTTON_RESET
                  | snap_manager::form::FORM_BUTTON_RESTORE_DEFAULT
                  | snap_manager::form::FORM_BUTTON_SAVE
                  | snap_manager::form::FORM_BUTTON_SAVE_EVERYWHERE
                );

        QStringList options;
        options << "false";
        options << "true";
        snap_manager::widget_select::pointer_t field(std::make_shared<snap_manager::widget_select>(
                          "Cassandra Auto-Snapshot"
                        , s.get_field_name()
                        , options
                        , s.get_value()
                        , "Cassandra says that you should set this parameter to \"true\"."
                         " However, when set to true, the DROP TABLE and TRUNCATE commands"
                         " become extremely slow because the database creates a snapshot"
                         " of the table before dropping or truncating it. We change this"
                         " parameter to \"false\" by default because if you DROP TABLE or"
                         " TRUNCATE by mistake, you probably have a bigger problem."
                         " Also, we offer a \"snapbackup\" tool which should be more than"
                         " enough to save all the data from all the tables. And somehow,"
                         " \"snapbackup\" goes a huge whole lot faster. (although if you"
                         " start having a really large database, you could end up not"
                         " being able to use \"snapbackup\" at all... once you reach"
                         " that limit, you may want to turn the auto_snapshot feature"
                         " back on."
                        ));
        f.add_widget(field);

        f.generate(parent, uri);

        return true;
    }

    if(s.get_field_name() == "join_a_cluster")
    {
        // the name of the computer to connect to
        //
        snap_manager::form f(
                  get_plugin_name()
                , s.get_field_name()
                ,   snap_manager::form::FORM_BUTTON_RESET
                  | snap_manager::form::FORM_BUTTON_SAVE
                );

        // TODO: get the list of names and show as a dropdown
        //
        snap_manager::widget_input::pointer_t field(std::make_shared<snap_manager::widget_input>(
                          "Enter the server_name of the computer to join:"
                        , s.get_field_name()
                        , s.get_value()
                        , "<p>The <code>server_name</code> parameter from snapcommunicator.conf"
                         " is used to contact that specific server, get the Cassandra"
                         " node information from that server, and then add the Cassandra"
                         " node running on this computer to the one on that other computer.</p>"
                         "<p>Note that to finish up a join quickly, it is important to run"
                         " <code>nodetool cleanup</code> on all the other nodes once the"
                         " new node is marked active (joined). Otherwise the data won't be"
                         " shared properly.</p>"
                         "<p><strong>WARNING:</strong> There is currently no safeguard for this"
                         " feature. The computer will proceed and possibly destroy some of your"
                         " data in the process if this current computer node is not a new node."
                         " If you have a replication factor larger than 1, then it should be okay.<p>"
                        ));
        f.add_widget(field);

        f.generate(parent, uri);

        return true;
    }

    if(s.get_field_name() == "replication_factor")
    {
        // the replication factor for Cassandra
        //
        snap_manager::form f(
                  get_plugin_name()
                , s.get_field_name()
                ,   snap_manager::form::FORM_BUTTON_RESET
                  | snap_manager::form::FORM_BUTTON_SAVE
                );

        snap_manager::widget_input::pointer_t field(std::make_shared<snap_manager::widget_input>(
                          "Enter the replication factor (RF):"
                        , s.get_field_name()
                        , s.get_value()
                        , "<p>By default we create the Snap! cluster with a replication factor of 1"
                         " (since you need 2 or more nodes to have a higher replication factor...)"
                         " This option let you change the factor. It must be run on a computer with"
                         " a Cassandra node. Make sure you do not enter a number larger than the"
                         " total number of nodes or your cluster will be stuck.<p>"
                        ));
        f.add_widget(field);

        f.generate(parent, uri);

        return true;
    }

    if(s.get_field_name() == "use_server_ssl")
    {
        snap_manager::form f(
                  get_plugin_name()
                , s.get_field_name()
                ,   snap_manager::form::FORM_BUTTON_RESET
                  | snap_manager::form::FORM_BUTTON_SAVE
                  | snap_manager::form::FORM_BUTTON_SAVE_EVERYWHERE
                );

        snap_manager::widget_input::pointer_t field(std::make_shared<snap_manager::widget_input>(
                          "Turn on server-to-server encryption (none, all, dc:&lt;<i>name</i>&gt;, rack:&lt;<i>name</i>&gt;):"
                        , s.get_field_name()
                        , s.get_value()
                        , "<p>By default, Cassandra communicates in the clear on the listening address."
                          " When you change this option to anything except 'none', 'server to server'' encryption will be turned on between"
                          " nodes. Also, if it is not already created, a server key pair will be created also,"
                          " and the trusted keys will be exchanged with each node on the network.<p>"
                        ));
        f.add_widget(field);

        f.generate(parent, uri);

        return true;
    }

    if(s.get_field_name() == "use_client_ssl")
    {
        snap_manager::form f(
                  get_plugin_name()
                , s.get_field_name()
                ,   snap_manager::form::FORM_BUTTON_RESET
                  | snap_manager::form::FORM_BUTTON_SAVE
                  | snap_manager::form::FORM_BUTTON_SAVE_EVERYWHERE
                );

        QStringList options;
        options << "false";
        options << "true";
        snap_manager::widget_select::pointer_t field(std::make_shared<snap_manager::widget_select>(
                          "Turn on client-to-server encryption (true or false):"
                        , s.get_field_name()
                        , options
                        , s.get_value()
                        , "<p>By default, Cassandra communicates in the clear on the listening address."
                          " When you turn on this flag, client to server encryption will be turned on between"
                          " clients and nodes. If it is not already present, a trusted client key will be generated."
                          " <i>snapdbproxy</i> will then query the nodes it's connected to and request the keys.<p>"
                        ));
        f.add_widget(field);

        f.generate(parent, uri);

        return true;
    }

    return false;
}


/** \brief Save 'new_value' in field 'field_name'.
 *
 * This function saves 'new_value' in 'field_name'.
 *
 * \param[in] button_name  The name of the button the user clicked.
 * \param[in] field_name  The name of the field to update.
 * \param[in] new_value  The new value to save in that field.
 * \param[in] old_or_installation_value  The old value, just in case
 *            (usually ignored,) or the installation values (only
 *            for the self plugin that manages bundles.)
 *
 * \return true if the new_value was applied successfully.
 */
bool cassandra::apply_setting(QString const & button_name, QString const & field_name, QString const & new_value, QString const & old_or_installation_value, std::set<QString> & affected_services)
{
    NOTUSED(old_or_installation_value);

    if( field_name == "restart_cassandra" )
    {
        affected_services.insert("cassandra-restart");
        return true;
    }

    if( field_name == "purge_ssl_keys" )
    {
        if( new_value == "purge_ssl_keys" )
        {
            auto session( casswrapper::Session::create() );
            QString const rm_cmd(
                        QString("rm -rf /etc/cassandra/ssl /etc/cassandra/public %1")
                            .arg(session->get_keys_path())
                        );
            if( system( rm_cmd.toUtf8().data() ) )
            {
                SNAP_LOG_ERROR("Cannot remove keys directories!");
            }
            //
            generate_keys();
            send_client_key( true );
            send_server_key();
        }
        else
        {
            SNAP_LOG_WARNING("Not purging keys, since user did not type in 'purge_ssl_keys'.");
        }
        return true;
    }

    bool const use_default(button_name == "restore_default");

    if(field_name == "join_a_cluster")
    {
        if(new_value == f_snap->get_server_name())
        {
            SNAP_LOG_ERROR("trying to join yourself (\"")(new_value)("\") is not going to work.");
        }
        else if(f_joining)
        {
            SNAP_LOG_ERROR("trying to join when you already ran that process. If it failed, restart snapmanagerdaemon and try again.");
        }
        else
        {
            f_joining = true;

            snap::snap_communicator_message cassandra_query;
            cassandra_query.set_server(new_value);
            cassandra_query.set_service("snapmanagerdaemon");
            cassandra_query.set_command("CASSANDRAQUERY");
            get_cassandra_info(cassandra_query);
            f_snap->forward_message(cassandra_query);
        }

        return true;
    }

    if(field_name == "replication_factor")
    {
        set_replication_factor(new_value);
        return true;
    }

    std::string const name( field_name.toUtf8().data() );
    std::string const val ( new_value.toUtf8().data()  );

    if( field_name == "cluster_name" )
    {
        //affected_services.insert("cassandra-restart");

        YAML::Node full_nodes = read_node_from_yaml();
        full_nodes[name] = val;
        write_node_to_yaml( full_nodes );
        return true;
    }

    if( field_name == "seeds" )
    {
        //affected_services.insert("cassandra-restart");

        YAML::Node full_nodes = read_node_from_yaml();

        for( auto seq : full_nodes["seed_provider"] )
        {
            for( auto subseq : seq["parameters"] )
            {
                SNAP_LOG_TRACE("writings 'seeds' with val=[")(val)("], new_value=[")(new_value)("]");
                subseq["seeds"].SetTag("?");
                subseq["seeds"] = val;
                break;
            }
        }

        write_node_to_yaml( full_nodes );
        return true;
    }

    if  ( field_name == "listen_address"
       || field_name == "rpc_address"
       || field_name == "broadcast_rpc_address"
        )
    {
        //affected_services.insert("cassandra-restart");

        YAML::Node full_nodes = read_node_from_yaml();
        full_nodes[name] = use_default ? "localhost" : val;
        write_node_to_yaml( full_nodes );
        return true;
    }

    if(field_name == "auto_snapshot")
    {
        //affected_services.insert("cassandra-restart");

        YAML::Node full_nodes = read_node_from_yaml();
        full_nodes[name] = use_default ? "false" : val;
        write_node_to_yaml( full_nodes );
        return true;
    }

    if( field_name == "use_server_ssl" )
    {
        //affected_services.insert("cassandra-restart");

        YAML::Node full_nodes = read_node_from_yaml();

        YAML::Node seo( full_nodes["server_encryption_options"] );
        seo["internode_encryption"] = val;
        seo["keystore"]             = "/etc/cassandra/ssl/keystore.jks";
        seo["keystore_password"]    = g_keystore_password.toUtf8().data();
        seo["truststore"]           = "/etc/cassandra/ssl/keystore.jks";
        seo["truststore_password"]  = g_truststore_password.toUtf8().data();

        write_node_to_yaml( full_nodes );
        return true;
    }

    if( field_name == "use_client_ssl" )
    {
        //affected_services.insert("cassandra-restart");

        YAML::Node full_nodes = read_node_from_yaml();

        auto ceo( full_nodes["client_encryption_options"] );
        ceo["enabled"]             = val;
        ceo["optional"]            = "false";
        ceo["keystore"]            = "/etc/cassandra/ssl/keystore.jks";
        ceo["keystore_password"]   = g_keystore_password.toUtf8().data();

        write_node_to_yaml( full_nodes );
        return true;
    }

    return false;
}



void cassandra::on_handle_affected_services(std::set<QString> & affected_services)
{
    bool restarted(false);

    auto const & it_restart(std::find(affected_services.begin(), affected_services.end(), QString("cassandra-restart")));
    if(it_restart != affected_services.end())
    {
        // remove since we are handling that one here
        //
        affected_services.erase(it_restart);

        // restart cassandra
        //
        // the stop can be extremely long and because of that, a
        // system restart does not always work correctly so we have
        // our own tool to restart cassandra
        //
        snap::process p("restart cassandra");
        p.set_mode(snap::process::mode_t::PROCESS_MODE_COMMAND);
        p.set_command("snaprestartcassandra");
        NOTUSED(p.run());           // errors are automatically logged by snap::process

        restarted = true;
    }

    auto const & it_reload(std::find(affected_services.begin(), affected_services.end(), QString("cassandra-reload")));
    if(it_reload != affected_services.end())
    {
        // remove since we are handling that one here
        //
        affected_services.erase(it_reload);

        // do the reload only if we did not already do a restart (otherwise
        // it is going to be useless)
        //
        if(!restarted)
        {
            // reload cassandra
            //
            snap::process p("reload cassandra");
            p.set_mode(snap::process::mode_t::PROCESS_MODE_COMMAND);
            p.set_command("systemctl");
            p.add_argument("reload");
            p.add_argument("cassandra");
            NOTUSED(p.run());           // errors are automatically logged by snap::process
        }
    }
}


void cassandra::send_client_key( bool const force, snap::snap_communicator_message const* message )
{
    // A client requested the public key for authentication.
    //
    QFile file( g_ssl_keys_dir + "client.pem" );
    if( file.open( QIODevice::ReadOnly | QIODevice::Text ) )
    {
        SNAP_LOG_TRACE("client.pem file found, sending to requesing app");

        QTextStream in(&file);
        //
        snap::snap_communicator_message cmd;
        cmd.set_command("CASSANDRAKEY");
        //
        if( message == nullptr )
        {
            //cmd.set_server("*");
            cmd.set_service("*");
        }
        else
        {
            cmd.reply_to(*message);
        }
        //
        cmd.add_parameter( "key"   , in.readAll() );
        cmd.add_parameter( "cache" , "ttl=60"     );
        if( force )
        {
            cmd.add_parameter( "force", "true" );
        }
        get_cassandra_info(cmd);
        f_snap->forward_message(cmd);

        SNAP_LOG_TRACE("CASSANDRAKEY message sent!");
    }
    else
    {
        QString const errmsg(QString("Cannot open '%1' for reading!").arg(file.fileName()));
        SNAP_LOG_INFO(errmsg);
    }
}


void cassandra::send_server_key()
{
    // Send the node key for the requesting peer.
    //
    QFile file( g_ssl_keys_dir + "node.cer" );
    if( file.open( QIODevice::ReadOnly | QIODevice::Text ) )
    {
        SNAP_LOG_TRACE("node.cer file found, broadcasting to all...");

        QTextStream in(&file);
        //
        snap::snap_communicator_message cmd;
        cmd.set_command("CASSANDRASERVERKEY");
        cmd.set_service("*");
        //cmd.set_service("snapmanagerdaemon");
        cmd.add_parameter( "key"   , in.readAll()    );
        cmd.add_parameter( "cache" , "ttl=60"        );
        get_cassandra_info(cmd);
        f_snap->forward_message(cmd);

        SNAP_LOG_TRACE("CASSANDRASERVERKEY sent!");
    }
    else
    {
        QString const errmsg(QString("Cannot open '%1' for reading!").arg(file.fileName()));
        SNAP_LOG_ERROR(errmsg);
    }
}


void cassandra::generate_keys()
{
    //
    // check whether the configuration file exists, if not then do not
    // bother, Cassandra is not even installed
    //
    QFile config_file(g_cassandra_yaml);
    if( !config_file.exists() )
    {
        SNAP_LOG_ERROR("Cannot read Cassandra configuration! Not generating keys!");
        return;
    }

    QString const listen_address( get_local_listen_address() );
    if( listen_address.isEmpty() )
    {
        SNAP_LOG_ERROR( "/etc/cassandra/cassandra.yaml does not contain a 'listen_address' entry! Cannot generate keys!" );
    }

    QDir ssl_dir(g_ssl_keys_dir);
    if( ssl_dir.exists() )
    {
        SNAP_LOG_TRACE("/etc/cassandra/ssl already exists, so we do nothing.");
        return;
    }

    SNAP_LOG_TRACE("The directory '")(g_ssl_keys_dir)("' does not exist, so generating Cassandra SSL keys...");

    // Create the directory, make sure it's in the snapwebsites group,
    // and make it so we have full access to it, but nothing for the rest
    // of the world.
    //
    ssl_dir.mkdir( g_ssl_keys_dir );
    snap::chownnm( g_ssl_keys_dir, "root", "cassandra" );
    if(::chmod( g_ssl_keys_dir.toUtf8().data(), 0750 ) != 0)
    {
        // we continue, this is just a log file, as long as we can write to it
        // we should be good, it's just a tad bit less secure
        //
        int const e(errno);
        SNAP_LOG_WARNING("Could not set mode of \"")(g_ssl_keys_dir)("\" to 0750. (errno: ")(e)(", ")(strerror(e));
    }

    char const * pd = "/etc/cassandra/public";
    QDir public_dir(pd);
    if( !public_dir.exists() )
    {
        public_dir.mkdir( pd );
        if(::chmod( pd, 0755 ) != 0)
        {
            // we continue, this is just a log file, as long as we can write to it
            // we should be good, it's just a tad bit less secure
            //
            int const e(errno);
            SNAP_LOG_WARNING("Could not set mode of \"")(pd)("\" to 0640. (errno: ")(e)(", ")(strerror(e));
        }
    }

    // Replace the periods with underscores, so that way it's a
    // little nicer as part of a file name.
    //
    QString listen_address_us( listen_address );
    listen_address_us.replace( '.', '_' );

    // Now generate the keys...
    //
    QStringList command_list;

    // Generate the keypair keystore for SSL
    //
    command_list << QString
       (
       "keytool -noprompt -genkeypair -keyalg RSA "
       " -alias      node%5"
       " -validity   36500"
       " -keystore   %1/keystore.jks"
       " -storepass  %2"
       " -keypass    %3"
       " -dname \"CN=%4, OU=Cassandra Backend, O=Made To Order Software Corp, L=Orangevale, ST=California, C=US\""
       )
          .arg(ssl_dir.path())
          .arg(g_truststore_password)
          .arg(g_keystore_password)
          .arg(listen_address)
          .arg(listen_address_us)
          ;

     // Export the node's public key. This will be distributed to the
     // other Cassandra nodes on the network.
     //
     command_list << QString
       (
       "keytool -export -rfc -alias node%3"
       " -file %1/node.cer"
       " -keystore %1/keystore.jks"
       " -storepass %2"
       )
          .arg(ssl_dir.path())
          .arg(g_truststore_password)
          .arg(listen_address_us)
       ;

    // Import our own public node key just incase
    //
    //command_list << QString
    //   (
    //   "keytool -import -v -trustcacerts "
    //   " -alias node%1"
    //   " -file %2/node.cer"
    //   " -keystore %2/keystore.jks"
    //   " -storepass %3"
    //   )
    //      .arg(listen_address_us)
    //      .arg(ssl_dir.path())
    //      .arg(g_truststore_password)
    //   ;


    // Export the client certificate. This will be shared
    // with snapdbproxy instances.
    //
    command_list << QString
       (
       "keytool -exportcert -rfc -noprompt"
       " -alias node%1"
       " -keystore %2/keystore.jks"
       " -file %2/client.pem"
       " -storepass %3"
       )
          .arg(listen_address_us)
          .arg(ssl_dir.path())
          .arg(g_truststore_password)
       ;

    // Export CQLSH-friendly keys.
    //
    // keytool -importkeystore -srckeystore keystore.node0 -destkeystore node0.p12 -deststoretype PKCS12 -srcstorepass cassandra -deststorepass cassandra
    // openssl pkcs12 -in node0.p12 -nokeys -out node0.cer.pem -passin pass:cassandra
    // openssl pkcs12 -in node0.p12 -nodes -nocerts -out node0.key.pem -passin pass:cassandra
    command_list << QString
       (
       "keytool -importkeystore"
       " -srckeystore %1/keystore.jks"
       " -destkeystore %1/node.p12"
       " -deststoretype PKCS12"
       " -srcstorepass %2"
       " -deststorepass %2"
       )
          .arg(ssl_dir.path())
          .arg(g_truststore_password)
       ;

    command_list << QString
       (
       "openssl pkcs12"
       " -in %1/node.p12"
       " -nokeys"
       " -out %2/cqlsh.cert.%3.pem"
       " -passin pass:%4"
       )
          .arg(ssl_dir.path())
          .arg(public_dir.path())
          .arg(listen_address_us)
          .arg(g_truststore_password)
       ;

    for( auto const& cmd : command_list )
    {
        if( system( cmd.toUtf8().data() ) != 0 )
        {
            SNAP_LOG_ERROR("Cannot execute command '")(cmd)("'!");
        }
    }

    // Copy the public key to a public-accessable folder
    //
    // (Do this after we run the above commands, so that the client.pem file
    // exists. It works better that way, I'm told.)
    //
    auto const source_client_pem( QString("%1client.pem").arg(g_ssl_keys_dir) );
    auto const dest_client_pem  ( QString("%1/client_%2.pem").arg(pd).arg(listen_address_us) );
    bool const copied = QFile::copy( source_client_pem, dest_client_pem );
    if( !copied )
    {
        SNAP_LOG_ERROR("Cannot copy [")(source_client_pem)("] to [")(dest_client_pem)("]");
    }
}


void cassandra::on_communication_ready()
{
    // now we can broadcast our CASSANDRAQUERY so we have
    // information about all our accomplices
    //
    // IMPORTANT: this won't work properly if all the other nodes are
    //            not yet fired up; for that reason the CASSANDRAQUERY
    //            includes the information that the CASSANDRAFIELDS
    //            reply includes because that way we avoid re-sending
    //            the message when we later receive a CASSANDRAQUERY
    //            message from a node that just woke up
    //
    // TODO:
    // At this point, I am thinking we should not send this message
    // until later enough so we know whether Cassandra started and
    // whether the context is defined or not... but I'm not implementing
    // that now.
    //
    //snap::snap_communicator_message cassandra_query;
    //cassandra_query.set_service("*");
    //cassandra_query.set_command("CASSANDRAQUERY");
    //get_cassandra_info(cassandra_query);
    //f_snap->forward_message(cassandra_query);

    // If this is a system with Cassandra installed, generate the keys
    // and distribute them.
    //
    QFile config_file(g_cassandra_yaml);
    if( config_file.exists() )
    {
        // Make sure server keys are generated.
        //
        generate_keys();

        // Next, send keys to everyone
        //
        send_client_key();
        send_server_key();
    }
}


void cassandra::on_add_plugin_commands(snap::snap_string_list & understood_commands)
{
    understood_commands << "CASSANDRAQUERY";
    understood_commands << "CASSANDRAFIELDS";
    understood_commands << "CASSANDRAKEYS";         // Send our public key to the requesting server...
    understood_commands << "CASSANDRASERVERKEY";    // Request public keys from other nodes...
}


namespace
{

void import_server_key( const QString& msg_listen_address, const QString& key )
{
    // Open the file...
    //
    if( msg_listen_address == get_local_listen_address() )
    {
        SNAP_LOG_TRACE("We received our own listen address [")(msg_listen_address)("], so no need to add the cert.");
        return;
    }
    //
    QString listen_address_us( msg_listen_address );
    listen_address_us.replace( '.', '_' );
    QString const full_path( QString("%1%2.cer")
                             .arg(g_ssl_keys_dir)
                             .arg(listen_address_us)
                             );
    try
    {
        QFile file( full_path );
        if( file.exists() )
        {
            // We already have the file, so ignore this.
            //
            SNAP_LOG_TRACE("We already have server cert file [")(full_path)("], so ignoring.");
            return;
        }

        if( !file.open( QIODevice::WriteOnly | QIODevice::Truncate ) )
        {
            QString const errmsg = QString("Cannot open '%1' for writing!").arg(file.fileName());
            SNAP_LOG_ERROR(errmsg);
            return;
        }
        //
        // ...and stream the file out to disk so we have the node key for
        // node-to-node SSL connections.
        //
        QTextStream out( &file );
        out << key;
    }
    catch( std::exception const& ex )
    {
        SNAP_LOG_ERROR("Cannot write SSL CERT file! what=[")(ex.what())("]");
        return;
    }
    catch( ... )
    {
        SNAP_LOG_ERROR("Cannot write SSL CERT file! Unknown error!");
        return;
    }

    SNAP_LOG_TRACE("Received cert file [")(full_path)("], adding importing into server session.");
    //
    QString const cmd(
                QString
                (
                    "keytool -import -noprompt -trustcacerts "
                    " -alias node%1"
                    " -file %2"
                    " -storepass %3"
                    " -keystore %4/keystore.jks"
                    )
                .arg(listen_address_us)
                .arg(full_path)
                .arg(g_truststore_password)
                .arg(g_ssl_keys_dir)
                );
    if( system( cmd.toUtf8().data() ) != 0 )
    {
        SNAP_LOG_ERROR("Cannot execute command '")(cmd)("'! Key is likely already in the truststore.");
        return;
    }

    // restart cassandra
    //
    // the stop can be extremely long and because of that, a
    // system restart does not always work correctly so we have
    // our own tool to restart cassandra
    //
    SNAP_LOG_TRACE("Restarting cassandra because we imported a new public cert.");
    //
    snap::process p("restart cassandra");
    p.set_mode(snap::process::mode_t::PROCESS_MODE_COMMAND);
    p.set_command("snaprestartcassandra");
    NOTUSED(p.run());           // errors are automatically logged by snap::process
}

}
// namespace


void cassandra::on_process_plugin_message(snap::snap_communicator_message const & message, bool & processed)
{
    QString const command(message.get_command());
    SNAP_LOG_TRACE("cassandra::on_process_plugin_message(), command=[")(command)("]");

    if(command == "CASSANDRAFIELDS")
    {
        //QString const server(message.get_sent_from_server());

        //
        // WARNING: Right now we assume that this reply is directly
        //          a reply to a CASSANDRAQUERY we sent to a specific
        //          computer and as a result we JOIN that other computer
        //          Cassandra cluster... We still have a flag, to make
        //          sure we are in the correct state, but as we want
        //          to implement a CASSANDRAQUERY that gets broadcast
        //          we may need to fix up the algorithm quite a bit
        //          (and actually the join won't require sending the
        //          CASSANDRAQUERY because we should already have the
        //          information anyway...)
        //

        if(f_joining)
        {
            join_cassandra_node(message);
            f_joining = false;
        }

        processed = true;
    }
    else if(command == "CASSANDRAQUERY")
    {
        // reply with a CASSANDRAINFO directly to the computer that
        // asked for it
        //
        snap::snap_communicator_message cassandra_status;
        cassandra_status.reply_to(message);
        cassandra_status.set_command("CASSANDRAFIELDS");
        get_cassandra_info(cassandra_status);
        f_snap->forward_message(cassandra_status);

        processed = true;
    }
    else if( command == "CASSANDRAKEYS" )
    {
        SNAP_LOG_TRACE("Processing command CASSANDRAKEYS");
        send_client_key( false, &message );
        processed = true;
    }
    else if( command == "CASSANDRASERVERKEY" )
    {
        SNAP_LOG_TRACE("Processing command CASSANDRASERVERKEY");
        import_server_key( message.get_parameter("listen_address"), message.get_parameter("key") );
        processed = true;
    }
}


void cassandra::get_cassandra_info(snap::snap_communicator_message & status)
{
    // check whether the configuration file exists, if not then do not
    // bother, Cassandra is not even installed
    //
    QFile config_file( g_cassandra_yaml );
    struct stat st;
    if
       ( stat("/usr/sbin/cassandra", &st) != 0
      || !config_file.exists()
       )
    {
        status.add_parameter("status", "not-installed");
        return;
    }

    status.add_parameter("status", "installed");

    try
    {
        YAML::Node node = YAML::LoadFile( g_cassandra_yaml.toUtf8().data() );

        // if installed we want to include the "cluster_name" and "seeds"
        // parameters
        //
        {
            status.add_parameter( "cluster_name",
                QString::fromUtf8(node["cluster_name"].Scalar().c_str()) );
        }

        {
            for( auto seq : node["seed_provider"] )
            {
                for( auto subseq : seq["parameters"] )
                {
                    status.add_parameter( "seeds",
                        QString::fromUtf8(subseq["seeds"].Scalar().c_str()) );
                    break;
                }
            }
        }

        {
            status.add_parameter( "listen_address",
                QString::fromUtf8(node["listen_address"].Scalar().c_str()) );
        }
    }
    catch( std::exception const& x )
    {
        SNAP_LOG_ERROR("cassandra::get_cassandra_info() exception caught, what=[")(x.what())("]!");
    }
    catch( ... )
    {
        SNAP_LOG_ERROR("cassandra::get_cassandra_info() unknown exception caught!");
    }

#if 0
    // the following actually requires polling for the port...
    //
    // i.e. we cannot assume that cassandra is not running just because
    // we cannot yet connect to the port...
    //
    // polling can be done by reading a netfilter socket which can be
    // optimized to only match the TCP port we're interested in so we
    // really receive either 0 or 1 response (i.e. socket not present
    // or socket present)
    //
    // polling the old way can be done without connecting by reading the
    // /proc/net/tcp file and analyzing the data (the field names are
    // defined on the first line)
    //
    // There is the code from netstat.c which parses one of those lines:
    //
    //   num = sscanf(line,
    //   "%d: %64[0-9A-Fa-f]:%X %64[0-9A-Fa-f]:%X %X %lX:%lX %X:%lX %lX %d %d %ld %512s\n",
    //       &d, local_addr, &local_port, rem_addr, &rem_port, &state,
    //       &txq, &rxq, &timer_run, &time_len, &retr, &uid, &timeout, &inode, more);
    //
    try
    {
        // if the connection fails, we cannot have either of the following
        // fields so the catch() makes sure to avoid them
        //
        int port(9042);
        QString const port_str(snap_dbproxy_conf["cassandra_port"]);
        if(!port_str.isEmpty())
        {
            bool ok(false);
            port = port_str.toInt(&ok, 10);
            if(!ok
            || port <= 0
            || port > 65535)
            {
                SNAP_LOG_ERROR( "Invalid cassandra_port specification in snapdbproxy.conf (invalid number, smaller than 1 or larger than 65535)" );
                port = 0;
            }
        }
        if(port > 0)
        {
            auto session( casswrapper::Session::create() );
            session->connect(snap_dbproxy_conf["cassandra_host_list"], port);
            if( !session->isConnected() )
            {
                SNAP_LOG_WARNING( "Cannot connect to cassandra host! Check cassandra_host_list and cassandra_port in snapdbproxy.conf!" );
            }
            else
            {
                auto meta( casswrapper::schema::SessionMeta::create( session ) );
                meta->loadSchema();
                auto const & keyspaces( meta->getKeyspaces() );
                QString const context_name(snap::get_name(snap::name_t::SNAP_NAME_CONTEXT));
                if( keyspaces.find(context_name) == std::end(keyspaces) )
                {
                    // no context yet, offer to create the context
                    //
                    snap_manager::status_t const create_context(
                                  snap_manager::status_t::state_t::STATUS_STATE_INFO
                                , get_plugin_name()
                                , "cassandra_create_context"
                                , context_name
                                );
                    server_status.set_field(create_context);
                }
                else
                {
                    // database is available and context is available,
                    // offer to create the tables (it should be automatic
                    // though, but this way we can click on this one last
                    // time before installing a website)
                    //
                    snap_manager::status_t const create_tables(
                                  snap_manager::status_t::state_t::STATUS_STATE_INFO
                                , get_plugin_name()
                                , "cassandra_create_tables"
                                , context_name
                                );
                    server_status.set_field(create_tables);
                }
            }
        }
    }
    catch( std::exception const & e )
    {
        SNAP_LOG_ERROR("Caught exception: ")(e.what());
    }
    catch( ... )
    {
        SNAP_LOG_ERROR("Caught unknown exception!");
    }
#endif
}


void cassandra::join_cassandra_node(snap::snap_communicator_message const & message)
{
    QString const cluster_name(message.get_parameter("cluster_name"));
    QString const seeds(message.get_parameter("seeds"));

    QString preamble("#!/bin/sh\n");

    preamble += "BUNDLE_UPDATE_CLUSTER_NAME=";
    preamble += cluster_name;
    preamble += "\n";

    preamble += "BUNDLE_UPDATE_SEEDS=";
    preamble += seeds;
    preamble += "\n";

    QFile script_original(":/manager-plugins/cassandra/join_cassandra_node.sh");
    if(!script_original.open(QIODevice::ReadOnly))
    {
        SNAP_LOG_ERROR("failed to open the join_cassandra_node.sh resouce file.");
        return;
    }
    QByteArray const original(script_original.readAll());

    QByteArray const script(preamble.toUtf8() + original);

    // Put the script in the cache and run it
    //
    // TODO: add a /scripts/ sub-directory so all scripts can be found
    //       there instead of the top directory?
    //
    QString const cache_path(f_snap->get_cache_path());
    std::string const script_filename(std::string(cache_path.toUtf8().data()) + "/join_cassandra_node.sh");
    snap::file_content output_file(script_filename);
    output_file.set_content(script.data());
    output_file.write_all();
    if(chmod(script_filename.c_str(), 0700) != 0)
    {
        // TODO:
        // we should change the creation of the file to make use of
        // open() so we can specify the permissions at the time the
        // file is created so it is immediately protected
        //
        int const e(errno);
        SNAP_LOG_WARNING("join cassandra script file mode could not be changed to 700, (errno: ")(e)(", ")(strerror(e))(")");
    }

    snap::process p("join cassandra node");
    p.set_mode(snap::process::mode_t::PROCESS_MODE_COMMAND);
    p.set_command(QString::fromUtf8(script_filename.c_str()));
    NOTUSED(p.run());           // errors are automatically logged by snap::process
}


QString cassandra::get_replication_factor()
{
    QString const context_name(snap::get_name(snap::name_t::SNAP_NAME_CONTEXT));

    // initialize the reading of the configuration file
    //
    snap::snap_config config("snapdbproxy");

    // get the list of Cassandra hosts, "127.0.0.1" by default
    //
    QString cassandra_host_list("127.0.0.1");
    if(config.has_parameter("cassandra_host_list"))
    {
        cassandra_host_list = config[ "cassandra_host_list" ];
        if(cassandra_host_list.isEmpty())
        {
            SNAP_LOG_ERROR("cassandra_host_list cannot be empty.");
            return QString();
        }
    }

    // get the Cassandra port, 9042 by default
    //
    int cassandra_port(9042);
    if(config.has_parameter("cassandra_port"))
    {
        std::size_t pos(0);
        std::string const port(config["cassandra_port"]);
        cassandra_port = std::stoi(port, &pos, 10);
        if(pos != port.length()
        || cassandra_port < 0
        || cassandra_port > 65535)
        {
            SNAP_LOG_ERROR("cassandra_port to connect to Cassandra must be defined between 0 and 65535.");
            return QString();
        }
    }

    // create a new Cassandra session
    //
    auto session( casswrapper::Session::create() );

    // increase the request timeout "dramatically" because creating a
    // context is very slow
    //
    // note: we do not make use of the QCassandraRequestTimeout class
    //       because we will just create the context and be done with it
    //       so there is no real need for us to restore the timeout
    //       at a later time
    //
    session->setTimeout(5 * 60 * 1000); // timeout = 5 min.

    // connect to the Cassandra cluster
    //
    try
    {
        bool const use_ssl(config["cassandra_use_ssl"] == "true");
        SNAP_LOG_DEBUG("connection attempt to Cassandra cluster ")(use_ssl ? " with SSL." : " in plain mode.");
        session->connect( cassandra_host_list, cassandra_port, use_ssl ); // throws on failure!
        if(!session->isConnected())
        {
            // this error should not ever appear since the connect()
            // function throws on errors, but for completeness...
            //
            SNAP_LOG_ERROR("could not connect to Cassandra cluster.");
            return QString();
        }
    }
    catch(std::exception const & e)
    {
        SNAP_LOG_ERROR("could not connect to Cassandra cluster. Exception: ")(e.what());
        return QString();
    }

    auto meta( casswrapper::schema::SessionMeta::create( session ) );
    meta->loadSchema();
    auto const & keyspaces( meta->getKeyspaces() );
    auto const & context( keyspaces.find(context_name) );
    if( context == std::end(keyspaces) )
    {
        SNAP_LOG_ERROR("could not find \"")(context_name)("\" context in Cassandra.");
        return QString();
    }

//    auto fields(context->second->getFields());
//    for(auto f = fields.begin(); f != fields.end(); ++f)
//    {
//        SNAP_LOG_ERROR("field: ")(f->first);
//    }

    auto const & fields(context->second->getFields());
    auto const replication(fields.find("replication"));
    if(replication == fields.end())
    {
        SNAP_LOG_ERROR("could not find \"replication\" as one of the context fields.");
        return QString();
    }
    //casswrapper::schema::Value const & value(replication->second);
//SNAP_LOG_ERROR("value type: ")(static_cast<int>(value.type()));
    //QVariant const v(value.variant());
    //QString const json(v.toString());

    casswrapper::schema::Value::map_t const map(replication->second.map());
//for(auto m : map)
//{
//    SNAP_LOG_ERROR("map: [")(m.first)("] value type: ")(static_cast<int>(m.second.type()));
//}
    auto const item(map.find("dc1"));
    if(item == map.end())
    {
        SNAP_LOG_ERROR("could not find \"dc1\" in the context replication definition.");
        return QString();
    }

//SNAP_LOG_ERROR("got item with type: ")(static_cast<int>(item->second.type()));
//SNAP_LOG_ERROR("Value as string: ")(item->second.variant().toString());

    return item->second.variant().toString();
}


void cassandra::set_replication_factor(QString const & replication_factor)
{
    QString const context_name(snap::get_name(snap::name_t::SNAP_NAME_CONTEXT));

    // initialize the reading of the configuration file
    //
    snap::snap_config config("snapdbproxy");

    // get the list of Cassandra hosts, "127.0.0.1" by default
    //
    QString cassandra_host_list("127.0.0.1");
    if(config.has_parameter("cassandra_host_list"))
    {
        cassandra_host_list = config[ "cassandra_host_list" ];
        if(cassandra_host_list.isEmpty())
        {
            SNAP_LOG_ERROR("cassandra_host_list cannot be empty.");
            return;
        }
    }

    // get the Cassandra port, 9042 by default
    //
    int cassandra_port(9042);
    if(config.has_parameter("cassandra_port"))
    {
        std::size_t pos(0);
        std::string const port(config["cassandra_port"]);
        cassandra_port = std::stoi(port, &pos, 10);
        if(pos != port.length()
        || cassandra_port < 0
        || cassandra_port > 65535)
        {
            SNAP_LOG_ERROR("cassandra_port to connect to Cassandra must be defined between 0 and 65535.");
            return;
        }
    }

    // create a new Cassandra session
    //
    auto session( casswrapper::Session::create() );

    // increase the request timeout "dramatically" because creating a
    // context is very slow
    //
    // note: we do not make use of the QCassandraRequestTimeout class
    //       because we will just create the context and be done with it
    //       so there is no real need for us to restore the timeout
    //       at a later time
    //
    session->setTimeout(5 * 60 * 1000); // timeout = 5 min.

    // connect to the Cassandra cluster
    //
    try
    {
        bool const use_ssl(config["cassandra_use_ssl"] == "true");
        SNAP_LOG_DEBUG("connection attempt to Cassandra cluster ")(use_ssl ? " with SSL." : " in plain mode.");
        session->connect( cassandra_host_list, cassandra_port, use_ssl ); // throws on failure!
        if(!session->isConnected())
        {
            // this error should not ever appear since the connect()
            // function throws on errors, but for completeness...
            //
            SNAP_LOG_ERROR("error: could not connect to Cassandra cluster.");
            return;
        }
    }
    catch(std::exception const & e)
    {
        SNAP_LOG_ERROR("error: could not connect to Cassandra cluster. Exception: ")(e.what());
        return;
    }

    // when called here we have f_session defined but no context yet
    //
    QString query_str( QString("ALTER KEYSPACE %1").arg(context_name) );

    query_str += QString( " WITH replication = { 'class': 'NetworkTopologyStrategy', 'dc1': '%1' }" ).arg(replication_factor);

    auto query( casswrapper::Query::create( session ) );
    query->query( query_str, 0 );
    //query->setConsistencyLevel( ... );
    //query->setTimestamp(...);
    //query->setPagingSize(...);
    query->start();
}


SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
