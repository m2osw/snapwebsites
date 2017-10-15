// Snap Websites Server -- manage the snapcommunicator settings
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

// communicator
//
#include "communicator.h"

// other plugins
//
#include <snapmanager/plugins/vpn/vpn.h>

// our lib
//
#include <snapmanager/form.h>

// snapwebsites lib
//
#include <snapwebsites/join_strings.h>
#include <snapwebsites/log.h>
#include <snapwebsites/not_reached.h>
#include <snapwebsites/not_used.h>
#include <snapwebsites/process.h>
#include <snapwebsites/qdomhelpers.h>
#include <snapwebsites/qdomxpath.h>
#include <snapwebsites/string_pathinfo.h>
#include <snapwebsites/tokenize_string.h>

// Qt lib
//
#include <QFile>

// C lib
//
#include <sys/file.h>

// last entry
//
#include <snapwebsites/poison.h>


SNAP_PLUGIN_START(communicator, 1, 0)


namespace
{

char const * g_configuration_filename = "snapcommunicator";

char const * g_configuration_d_filename = "/etc/snapwebsites/snapwebsites.d/snapcommunicator.conf";

char const * g_service_filename = "/lib/systemd/system/snapcommunicator.service";
char const * g_service_override_filename = "/etc/systemd/system/snapcommunicator.service.d/override.conf";


void file_descriptor_deleter(int * fd)
{
    if(close(*fd) != 0)
    {
        int const e(errno);
        SNAP_LOG_WARNING("closing file descriptor failed (errno: ")(e)(", ")(strerror(e))(")");
    }
}


} // no name namespace



/** \brief Get a fixed communicator plugin name.
 *
 * The communicator plugin makes use of different fixed names. This function
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
    case name_t::SNAP_NAME_SNAPMANAGERCGI_SNAPCOMMUNICATOR_AFTER:
        return "after";

    case name_t::SNAP_NAME_SNAPMANAGERCGI_SNAPCOMMUNICATOR_AFTER_FIELD:
        return "Unit::After";

    case name_t::SNAP_NAME_SNAPMANAGERCGI_SNAPCOMMUNICATOR_FORGET_NEIGHBOR:
        return "forget_neighbors";

    case name_t::SNAP_NAME_SNAPMANAGERCGI_SNAPCOMMUNICATOR_MY_ADDRESS:
        return "my_address";

    case name_t::SNAP_NAME_SNAPMANAGERCGI_SNAPCOMMUNICATOR_NEIGHBORS:
        return "neighbors";

    case name_t::SNAP_NAME_SNAPMANAGERCGI_SNAPCOMMUNICATOR_REQUIRE:
        return "require";

    case name_t::SNAP_NAME_SNAPMANAGERCGI_SNAPCOMMUNICATOR_REQUIRE_FIELD:
        return "Unit::Require";

    default:
        // invalid index
        throw snap_logic_exception("Invalid SNAP_NAME_SNAPMANAGERCGI_COMMUNICATOR_...");

    }
    NOTREACHED();
}




/** \brief Initialize the communicator plugin.
 *
 * This function is used to initialize the communicator plugin object.
 */
communicator::communicator()
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Clean up the communicator plugin.
 *
 * Ensure the communicator object is clean before it is gone.
 */
communicator::~communicator()
{
}


/** \brief Get a pointer to the communicator plugin.
 *
 * This function returns an instance pointer to the communicator plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the communicator plugin.
 */
communicator * communicator::instance()
{
    return g_plugin_communicator_factory.instance();
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
QString communicator::description() const
{
    return "Manage the snapcommunicator settings.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString communicator::dependencies() const
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
int64_t communicator::do_update(int64_t last_updated)
{
    NOTUSED(last_updated);

    SNAP_PLUGIN_UPDATE_INIT();
    // no updating in snapmanager*
    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Initialize communicator.
 *
 * This function terminates the initialization of the communicator plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void communicator::bootstrap(snap_child * snap)
{
    f_snap = dynamic_cast<snap_manager::manager *>(snap);
    if(f_snap == nullptr)
    {
        throw snap_logic_exception("snap pointer does not represent a valid manager object.");
    }

    SNAP_LISTEN(communicator, "server", snap_manager::manager, retrieve_status, _1);
}


/** \brief Determine this plugin status data.
 *
 * This function builds a tree of statuses.
 *
 * \param[in] server_status  The map of statuses.
 */
void communicator::on_retrieve_status(snap_manager::server_status & server_status)
{
    if(f_snap->stop_now_prima())
    {
        return;
    }

    // TODO: find a way to get the configuration filename for snapcommunicator
    //       (i.e. take it from the XML?)
    {
        snap_config snap_communicator_conf(g_configuration_filename);

        snap_manager::status_t const my_address(
                      snap_manager::status_t::state_t::STATUS_STATE_INFO
                    , get_plugin_name()
                    , get_name(name_t::SNAP_NAME_SNAPMANAGERCGI_SNAPCOMMUNICATOR_MY_ADDRESS)
                    , snap_communicator_conf[get_name(name_t::SNAP_NAME_SNAPMANAGERCGI_SNAPCOMMUNICATOR_MY_ADDRESS)]);
        server_status.set_field(my_address);

        snap_manager::status_t const neighbors(
                      snap_manager::status_t::state_t::STATUS_STATE_INFO
                    , get_plugin_name()
                    , get_name(name_t::SNAP_NAME_SNAPMANAGERCGI_SNAPCOMMUNICATOR_NEIGHBORS)
                    , get_known_neighbors() + "|"
                      + snap_communicator_conf[get_name(name_t::SNAP_NAME_SNAPMANAGERCGI_SNAPCOMMUNICATOR_NEIGHBORS)]);
        server_status.set_field(neighbors);

        snap_manager::status_t const forget_neighbor(
                      snap_manager::status_t::state_t::STATUS_STATE_INFO
                    , get_plugin_name()
                    , get_name(name_t::SNAP_NAME_SNAPMANAGERCGI_SNAPCOMMUNICATOR_FORGET_NEIGHBOR)
                    , "");
        server_status.set_field(forget_neighbor);
    }

    // when the user installed VPN (client or server) then we want to
    // check whether we have the following in snapcommunicator.service:
    //
    //    After=sys-devices-virtual-net-tun0.device
    //    Require=sys-devices-virtual-net-tun0.device
    //
    if(vpn::vpn::is_installed())
    {
        snap_config config(g_service_filename, g_service_override_filename);

        {
            std::string const after(config[get_name(name_t::SNAP_NAME_SNAPMANAGERCGI_SNAPCOMMUNICATOR_AFTER_FIELD)]);
            snap_manager::status_t const field(
                      snap_manager::status_t::state_t::STATUS_STATE_INFO
                    , get_plugin_name()
                    , get_name(name_t::SNAP_NAME_SNAPMANAGERCGI_SNAPCOMMUNICATOR_AFTER)
                    , QString::fromUtf8(after.c_str())
                    );
            server_status.set_field(field);
        }

        {
            std::string const require(config[get_name(name_t::SNAP_NAME_SNAPMANAGERCGI_SNAPCOMMUNICATOR_REQUIRE_FIELD)]);
            snap_manager::status_t const field(
                      snap_manager::status_t::state_t::STATUS_STATE_INFO
                    , get_plugin_name()
                    , get_name(name_t::SNAP_NAME_SNAPMANAGERCGI_SNAPCOMMUNICATOR_REQUIRE)
                    , QString::fromUtf8(require.c_str())
                    );
            server_status.set_field(field);
        }
    }
}


// TODO: put that in the library so the snapcommunicator and this plugin can both use the same function?
QString communicator::get_known_neighbors()
{
    // get the path to the cache, create if necessary
    //
    QString neighbors_cache_filename(f_snap->get_cache_path());
    if(neighbors_cache_filename.isEmpty())
    {
        neighbors_cache_filename = "/var/cache/snapwebsites";
    }
    neighbors_cache_filename += "/neighbors.txt";

    QFile cache(neighbors_cache_filename);
    if(!cache.open(QIODevice::ReadOnly))
    {
        return QString();
    }

    QString neighbors;

    char buf[1024];
    for(;;)
    {
        qint64 const r(cache.readLine(buf, sizeof(buf)));
        if(r < 0)
        {
            break;
        }
        if(r > 0
        && buf[0] != '#')
        {
            if(!neighbors.isEmpty())
            {
                neighbors += ", ";
            }

            QString const line(QString::fromUtf8(buf, r).trimmed());
            neighbors += line;

            // TODO: verify that each entry is a valid IP address
        }
    }

    return neighbors;
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
bool communicator::display_value(QDomElement parent, snap_manager::status_t const & s, snap::snap_uri const & uri)
{
    QDomDocument doc(parent.ownerDocument());

    if(s.get_field_name() == get_name(name_t::SNAP_NAME_SNAPMANAGERCGI_SNAPCOMMUNICATOR_MY_ADDRESS))
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
                          "The Private Network IP Address of this computer:"
                        , s.get_field_name()
                        , s.get_value()
                        , "Here you want to enter the Private Network IP Address."
                         " If you have your own private network, this is likely"
                         " the eth1 or equivalent IP address. If you have OpenVPN,"
                         " then it is the IP address shown in the tun0 interface"
                         " (with ifconfig, we also show those IPs on this page"
                         " under self.)"
                        ));
        f.add_widget(field);

        f.generate(parent, uri);

        return true;
    }

    if(s.get_field_name() == get_name(name_t::SNAP_NAME_SNAPMANAGERCGI_SNAPCOMMUNICATOR_NEIGHBORS))
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

        // extract the list of known neighbors and the value from
        // the field value; they are separated by a pipe character
        //
        QString known_neighbors;
        QString value;

        QString known_neighbors_value(s.get_value());
        int const pos(known_neighbors_value.indexOf('|'));
        if(pos >= 0)
        {
            known_neighbors = known_neighbors_value.mid(0, pos);
            value = known_neighbors_value.mid(pos + 1);
        }
        else
        {
            value = known_neighbors_value;
        }

        snap_manager::widget_input::pointer_t field(std::make_shared<snap_manager::widget_input>(
                          "The comma separated IP addresses of one or more neighbors:"
                        , s.get_field_name()
                        , value
                        , QString("<p>This field accepts the IP address of one or more neighbors"
                         " in the same private network.</p>"
                         "<p><strong>NOTE:</strong> By default we install"
                         " snapcommunicator with SSL encryption between computers."
                         " However, if you removed that encryption mechanism, you"
                         " must either turn it back on or use a form of tunneling"
                         " such as OpenVPN.</p>%1").arg(
                             known_neighbors.isEmpty()
                                ? QString("<p>No neighbors are known at this time.</p>")
                                : QString("<p>The already known neighbors are: %1</p>").arg(known_neighbors))
                        ));
        f.add_widget(field);

        f.generate(parent, uri);

        return true;
    }

    if(s.get_field_name() == get_name(name_t::SNAP_NAME_SNAPMANAGERCGI_SNAPCOMMUNICATOR_FORGET_NEIGHBOR))
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
                          "One neighbor to remove (IP:Port):"
                        , s.get_field_name()
                        , s.get_value()
                        , "This object is here to allow you to actually really"
                         " remove a neighbor. Once neighbors were shared on the"
                         " cluster, there are copies everywhere. So the easest"
                         " way is to use this field and enter the IP address"
                         " and the port. For example: \"10.8.0.1:4040\" (the"
                         " default port is 4040)."
                        ));
        f.add_widget(field);

        f.generate(parent, uri);

        return true;
    }

    if(s.get_field_name() == get_name(name_t::SNAP_NAME_SNAPMANAGERCGI_SNAPCOMMUNICATOR_AFTER))
    {
        snap_manager::form f(
                  get_plugin_name()
                , s.get_field_name()
                , snap_manager::form::FORM_BUTTON_SAVE
                );
        snap_manager::widget_input::pointer_t field(std::make_shared<snap_manager::widget_input>(
                    "\"After=\" of snapcommunicator.service"
                    , s.get_field_name()
                    , s.get_value()
                    , "<p>You are using a VPN so the snapcommunicator.service must start after the OpenVPN is started."
                     " This means the After= parameter is expected to include:</p>"
                     "<pre>sys-devices-virtual-net-tun0.device</pre>"
                     "<p>If you have other parameters in the After= variable, make sure to add a space between"
                     " each one of them.</p>"
                     "<p>At time of writing, the default After= variable is:</p>"
                     "<pre>After=network.target</pre>"
                     "<p>So with the VPN it would become:</p>"
                     "<pre>After=network.target sys-devices-virtual-net-tun0.device</pre>"
                    ));
        f.add_widget(field);
        f.generate(parent, uri);
        return true;
    }

    if(s.get_field_name() == get_name(name_t::SNAP_NAME_SNAPMANAGERCGI_SNAPCOMMUNICATOR_REQUIRE))
    {
        snap_manager::form f(
                  get_plugin_name()
                , s.get_field_name()
                , snap_manager::form::FORM_BUTTON_SAVE
                );
        snap_manager::widget_input::pointer_t field(std::make_shared<snap_manager::widget_input>(
                    "\"Require=\" of snapcommunicator.service"
                    , s.get_field_name()
                    , s.get_value()
                    , "<p>You are using a VPN so the snapcommunicator.service must start after the OpenVPN is started."
                     " This means the Require= parameter is expected to include:</p>"
                     "<pre>sys-devices-virtual-net-tun0.device</pre>"
                     "<p>If you have other parameters in the Require= variable, make sure to add a space between"
                     " each one of them.</p>"
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
bool communicator::apply_setting(QString const & button_name, QString const & field_name, QString const & new_value, QString const & old_or_installation_value, std::set<QString> & affected_services)
{
    NOTUSED(old_or_installation_value);
    NOTUSED(button_name);

    // restore defaults?
    //
    //bool const use_default_value(button_name == "restore_default");

    if(field_name == get_name(name_t::SNAP_NAME_SNAPMANAGERCGI_SNAPCOMMUNICATOR_MY_ADDRESS))
    {
        // this address is to connect this snapcommunicator to
        // other snapcommunicators
        //
        affected_services.insert("snapcommunicator");
        affected_services.insert("snapmanagerdaemon");

        // Here we change the "my_address" and "listen" parameters because
        // the two fields are expected to have the exact same IP address in
        // nearly 100% of all cases... note that we force the port to 4040
        // because at this point we do not want to offer an end user
        // interface to deal with all the ports.
        //
        NOTUSED(f_snap->replace_configuration_value(g_configuration_d_filename, field_name, new_value));
        NOTUSED(f_snap->replace_configuration_value(g_configuration_d_filename, "listen", new_value + ":4040"));
        return true;
    }

    if(field_name == get_name(name_t::SNAP_NAME_SNAPMANAGERCGI_SNAPCOMMUNICATOR_NEIGHBORS))
    {
        // for potential new neighbors indicated in snapcommunicator
        // we have to restart it
        //
        affected_services.insert("snapcommunicator");
        affected_services.insert("snapmanagerdaemon");

        NOTUSED(f_snap->replace_configuration_value(g_configuration_d_filename, field_name, new_value));
        return true;
    }

    if(field_name == get_name(name_t::SNAP_NAME_SNAPMANAGERCGI_SNAPCOMMUNICATOR_FORGET_NEIGHBOR))
    {
        // remove neighbors by sending a FORGET message
        //
        snap::snap_communicator_message forget;
        forget.set_command("FORGET");
        forget.add_parameter("ip", new_value);
        f_snap->forward_message(forget);

        return true;
    }

    if(field_name == get_name(name_t::SNAP_NAME_SNAPMANAGERCGI_SNAPCOMMUNICATOR_AFTER))
    {
        // we are changing the snapcommunicator but only the manager daemon
        // needs to be restarted so it gets the correct status; the After
        // parameter should not affect the currently running snapcommunicator
        //
        affected_services.insert("snapmanagerdaemon");

        f_snap->replace_configuration_value(
                          g_service_override_filename
                        , get_name(name_t::SNAP_NAME_SNAPMANAGERCGI_SNAPCOMMUNICATOR_AFTER_FIELD)
                        , new_value
                        , snap_manager::REPLACE_CONFIGURATION_VALUE_SECTION);
        snap::process p("reload daemon");
        p.set_mode(snap::process::mode_t::PROCESS_MODE_COMMAND);
        p.set_command("systemctl");
        p.add_argument("daemon-reload"); // python script sends output to STDERR
        NOTUSED(p.run());
        return true;
    }

    if(field_name == get_name(name_t::SNAP_NAME_SNAPMANAGERCGI_SNAPCOMMUNICATOR_REQUIRE))
    {
        // we are changing the snapcommunicator but only the manager daemon
        // needs to be restarted so it gets the correct status; the After
        // parameter should not affect the currently running snapcommunicator
        //
        affected_services.insert("snapmanagerdaemon");

        f_snap->replace_configuration_value(
                          g_service_override_filename
                        , get_name(name_t::SNAP_NAME_SNAPMANAGERCGI_SNAPCOMMUNICATOR_REQUIRE_FIELD)
                        , new_value
                        , snap_manager::REPLACE_CONFIGURATION_VALUE_SECTION);
        snap::process p("reload daemon");
        p.set_mode(snap::process::mode_t::PROCESS_MODE_COMMAND);
        p.set_command("systemctl");
        p.add_argument("daemon-reload"); // python script sends output to STDERR
        NOTUSED(p.run());
        return true;
    }

    return false;
}





SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
