// Snap Websites Server -- manage the snapdbproxy settings
// Copyright (c) 2016-2019  Made to Order Software Corp.  All Rights Reserved
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
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA


// self
//
#include "dbproxy.h"


// our lib
//
#include <snapmanager/form.h>


// snapwebsites lib
//
#include <snapwebsites/process.h>
#include <snapwebsites/qdomhelpers.h>
#include <snapwebsites/qdomxpath.h>
#include <snapwebsites/snap_cassandra.h>


// snaplogger lib
//
#include <snaplogger/message.h>


// snapdev lib
//
#include <snapdev/join_strings.h>
#include <snapdev/not_reached.h>
#include <snapdev/not_used.h>
#include <snapdev/pathinfo.h>
#include <snapdev/tokenize_string.h>


// CassWrapper
//
#include <casswrapper/schema.h>
#include <casswrapper/session.h>


// Qt lib
//
#include <QFile>


// C lib
//
#include <sys/file.h>


// last include
//
#include <snapdev/poison.h>



SNAP_PLUGIN_START(dbproxy, 1, 0)




namespace
{

//char const * g_service_filename = "/lib/systemd/system/snapdbproxy.service";

char const * g_configuration_filename = "snapdbproxy";

char const * g_configuration_d_filename = "/etc/snapwebsites/snapwebsites.d/snapdbproxy.conf";


void file_descriptor_deleter(int * fd)
{
    if(close(*fd) != 0)
    {
        int const e(errno);
        SNAP_LOG_WARNING("closing file descriptor failed (errno: ")(e)(", ")(strerror(e))(")");
    }
}


} // no name namespace



/** \brief Get a fixed dbproxy plugin name.
 *
 * The dbproxy plugin makes use of different fixed names. This function
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
    case name_t::SNAP_NAME_SNAPMANAGERCGI_DBPROXY_NAME:
        return "name";

    default:
        // invalid index
        throw snap_logic_exception("Invalid SNAP_NAME_SNAPMANAGERCGI_DBPROXY_...");

    }
    snapdev::NOT_REACHED();
}




/** \brief Initialize the dbproxy plugin.
 *
 * This function is used to initialize the dbproxy plugin object.
 */
dbproxy::dbproxy()
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Clean up the dbproxy plugin.
 *
 * Ensure the dbproxy object is clean before it is gone.
 */
dbproxy::~dbproxy()
{
}


/** \brief Get a pointer to the dbproxy plugin.
 *
 * This function returns an instance pointer to the dbproxy plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the dbproxy plugin.
 */
dbproxy * dbproxy::instance()
{
    return g_plugin_dbproxy_factory.instance();
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
QString dbproxy::description() const
{
    return "Manage the snapdbproxy settings.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString dbproxy::dependencies() const
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
int64_t dbproxy::do_update(int64_t last_updated)
{
    snapdev::NOT_USED(last_updated);

    SNAP_PLUGIN_UPDATE_INIT();
    // no updating in snapmanager*
    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Initialize dbproxy.
 *
 * This function terminates the initialization of the dbproxy plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void dbproxy::bootstrap(snap_child * snap)
{
    f_snap = dynamic_cast<snap_manager::manager *>(snap);
    if(f_snap == nullptr)
    {
        throw snap_logic_exception("snap pointer does not represent a valid manager object.");
    }

    SNAP_LISTEN(dbproxy, "server", snap_manager::manager, retrieve_status, boost::placeholders::_1);
}


/** \brief Determine this plugin status data.
 *
 * This function builds a tree of statuses.
 *
 * \param[in] server_status  The map of statuses.
 */
void dbproxy::on_retrieve_status(snap_manager::server_status & server_status)
{
    if(f_snap->stop_now_prima())
    {
        return;
    }

    snap_config snap_dbproxy_conf(g_configuration_filename);

    {
        snap_manager::status_t const host_list(
                      snap_manager::status_t::state_t::STATUS_STATE_INFO
                    , get_plugin_name()
                    , "cassandra_host_list"
                    , snap_dbproxy_conf["cassandra_host_list"]);
        server_status.set_field(host_list);
    }

    {
        bool const use_ssl = snap_dbproxy_conf["cassandra_use_ssl"] == "true";
        snap_manager::status_t const use_ssl_switch(
                      snap_manager::status_t::state_t::STATUS_STATE_INFO
                    , get_plugin_name()
                    , "cassandra_use_ssl"
                    , use_ssl ? "true" : "false"
                    );
        server_status.set_field(use_ssl_switch);
    }

    // run a quick test to see whether the setup is correct and if so whether
    // it's possible to connect to the database
    //
    int port(9042);
    bool can_connect(false);
    QString message("database is ready");
    try
    {
        // if the connection fails, we cannot have either of the following
        // fields so the catch() makes sure to avoid them
        //
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
                message = QString("cassandra_port is invalid (%1).").arg(port);
                port = 0;
            }
        }
        if(port > 0)
        {
            bool const use_ssl(snap_dbproxy_conf["cassandra_use_ssl"] == "true");
            SNAP_LOG_DEBUG("connection attempt to Cassandra cluster ")(use_ssl ? " with SSL." : " in plain mode.");
            auto session(casswrapper::Session::create());
            session->connect(snap_dbproxy_conf["cassandra_host_list"], port, use_ssl);
            can_connect = session->isConnected();
        }

    }
    catch(std::exception const & e)
    {
        SNAP_LOG_WARNING("Caught exception: ")(e.what());
        message = QString("attempt at connecting to Cassandra resulted in an exception (%1).").arg(e.what());
    }
    catch(...)
    {
        SNAP_LOG_ERROR("Caught unknown exception!");
        message = "attempt at connecting to Cassandra resulted in an exception (unknown exception).";
    }

    {
        snap_manager::status_t const cassandra_status(
                      port < 0
                        ? snap_manager::status_t::state_t::STATUS_STATE_ERROR
                        : (can_connect
                            ? snap_manager::status_t::state_t::STATUS_STATE_INFO
                            : snap_manager::status_t::state_t::STATUS_STATE_WARNING)
                    , get_plugin_name()
                    , "cassandra_status"
                    , snap::get_name(snap::name_t::SNAP_NAME_CONTEXT)
                );
        server_status.set_field(cassandra_status);
    }
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
 * \param[in] server_status  The map of statuses.
 * \param[in] s  The field being worked on.
 *
 * \return true if we handled this field.
 */
bool dbproxy::display_value(QDomElement parent, snap_manager::status_t const & s, snap::snap_uri const & uri)
{
    QDomDocument doc(parent.ownerDocument());

    if(s.get_field_name() == "cassandra_host_list")
    {
        // the list if frontend snapmanagers that are to receive statuses
        // of the cluster computers; may be just one computer; should not
        // be empty; shows a text input field
        //
        snap_manager::form f(
                  get_plugin_name()
                , s.get_field_name()
                , snap_manager::form::FORM_BUTTON_RESET | snap_manager::form::FORM_BUTTON_SAVE
                );

        snap_manager::widget_input::pointer_t field(std::make_shared<snap_manager::widget_input>(
                          "Cassandra Node IP Addresses:"
                        , s.get_field_name()
                        , s.get_value()
                        , "The list of <strong>comma separated</strong> IP addresses used to connect to Cassandra."
                         " In general these are seed nodes, although it does not need to be."
                         " The C++ Cassandra driver will adjust the information as"
                         " required and connect to additional nodes automatically."
                        ));
        f.add_widget(field);

        f.generate(parent, uri);

        return true;
    }

    if(s.get_field_name() == "cassandra_use_ssl")
    {
        // the list if frontend snapmanagers that are to receive statuses
        // of the cluster computers; may be just one computer; should not
        // be empty; shows a text input field
        //
        snap_manager::form f(
                  get_plugin_name()
                , s.get_field_name()
                , snap_manager::form::FORM_BUTTON_SAVE
                );


        QStringList switch_list;
        switch_list << "true";
        switch_list << "false";
        //
        snap_manager::widget_select::pointer_t field(std::make_shared<snap_manager::widget_select>(
                          "Cassandra connections should use SSL:"
                        , s.get_field_name()
                        , switch_list
                        , s.get_value()
                        , "Specify 'true' or 'false."
                         " If 'true' is specified, then snapdbproxy will expect SSL certs to be delivered via"
                         " the cassandra snapmanager plugin."
                        ));
        f.add_widget(field);

        f.generate(parent, uri);

        return true;
    }

    if(s.get_field_name() == "cassandra_status")
    {
        // display the current status of the database
        // (note: it could be scylla or cassandra)
        //
        snap_manager::form f(
                  get_plugin_name()
                , s.get_field_name()
                , snap_manager::form::FORM_BUTTON_NONE
            );

        snap_manager::widget_input::pointer_t field(std::make_shared<snap_manager::widget_input>(
                          "Database Status"
                        , s.get_field_name()
                        , s.get_value()
                        , "<p>This entry shows the current status of the Cassandra cluster.</p>"
                         "<ul>"
                             "<li>If it shows as an error, the setup is currently not acceptable and no connection can be obtained.</li>"
                             "<li>If it shows as a warning, we are not able to connect to the database.</li>"
                             "<li>If it does not show an error or a warning, we can connect to the database.</li>"
                         "</ul>"
                         "<p><b>Note:</b> this test does not check anything within the database, only that it is accessible.</p>"
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
 * \param[in] affected_services  The list of services that were affected
 *            by this call.
 *
 * \return true if the new_value was applied successfully.
 */
bool dbproxy::apply_setting(QString const & button_name, QString const & field_name, QString const & new_value, QString const & old_or_installation_value, std::set<QString> & affected_services)
{
    snapdev::NOT_USED(old_or_installation_value, button_name);

    // restore defaults?
    //
    //bool const use_default_value(button_name == "restore_default");

    if(field_name == "cassandra_host_list")
    {
        // to make use of the new list, make sure to restart
        //
        affected_services.insert("snapdbproxy");

        // fix the value in memory
        //
        snap_config snap_dbproxy_conf(g_configuration_filename);
        snap_dbproxy_conf["cassandra_host_list"] = new_value;

        return f_snap->replace_configuration_value(g_configuration_d_filename, "cassandra_host_list", new_value);
    }

    if(field_name == "cassandra_use_ssl")
    {
        // to make use of the new list, make sure to restart
        //
        affected_services.insert("snapdbproxy");

        // fix the value in memory
        //
        snap_config snap_dbproxy_conf(g_configuration_filename);
        snap_dbproxy_conf["cassandra_use_ssl"] = new_value;

        snapdev::NOT_USED(f_snap->replace_configuration_value(g_configuration_d_filename, "cassandra_use_ssl", new_value));

        // the affected_services.insert() already sends a RELOADCONFIG message
        //bool const config_set = f_snap->replace_configuration_value(g_configuration_d_filename, "cassandra_use_ssl", new_value);
        //if( config_set )
        //{
        //    // Send restart message to snapdbproxy, because we changed the configuration.
        //    //
        //    snap::snap_communicator_message cmd;
        //    cmd.set_service("snapdbproxy");
        //    cmd.set_command("RELOADCONFIG");
        //    f_snap->forward_message(cmd);
        //}

        return true;
    }

    return false;
}





SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
