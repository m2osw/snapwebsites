// Snap Websites Server -- manage the snapserver settings
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


// snapserver_manager
//
#include "snapserver_manager.h"


// our lib
//
#include <snapmanager/form.h>


// snapwebsites
//
#include    <snapwebsites/qdomhelpers.h>
#include    <snapwebsites/qdomxpath.h>
#include    <snapwebsites/snap_exception.h>


// serverplugins
//
#include    <serverplugins/factory.h>


// snaplogger
//
#include    <snaplogger/message.h>


// snapdev
//
#include <snapdev/join_strings.h>
#include <snapdev/not_reached.h>
#include <snapdev/not_used.h>
#include <snapdev/pathinfo.h>
#include <snapdev/tokenize_string.h>


// Qt lib
//
#include <QFile>


// C lib
//
#include <sys/file.h>


// last include
//
#include <snapdev/poison.h>



SERVERPLUGINS_START(snapserver_manager)
    , ::serverplugins::description(
            "Manage the snapsnapserver_manager settings.")
    , ::serverplugins::dependency("server")
SERVERPLUGINS_END(snapserver_manager)


namespace
{


char const * g_configuration_filename = "/etc/snapwebsites/snapserver.conf";

char const * g_configuration_d_filename = "/etc/snapwebsites/snapwebsites.d/snapserver.conf";

//void file_descriptor_deleter(int * fd)
//{
//    if(close(*fd) != 0)
//    {
//        int const e(errno);
//        SNAP_LOG_WARNING("closing file descriptor failed (errno: ")(e)(", ")(strerror(e))(")");
//    }
//}


} // no name namespace



/** \brief Get a fixed snapserver_manager plugin name.
 *
 * The snapserver_manager plugin makes use of different fixed names. This function
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
    case name_t::SNAP_NAME_SNAPMANAGERCGI_SNAPSERVER_MANAGER_NAME:
        return "name";

    default:
        // invalid index
        throw snap_logic_error("Invalid SNAP_NAME_SNAPMANAGERCGI_SNAPSERVER_MANAGER_...");

    }
    snapdev::NOT_REACHED();
}




/** \brief Initialize snapserver_manager.
 *
 * This function terminates the initialization of the snapserver_manager plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void snapserver_manager::bootstrap()
{
    SNAP_LISTEN(snapserver_manager, "server", snap_manager::manager, retrieve_status, boost::placeholders::_1);
}


/** \brief Determine this plugin status data.
 *
 * This function builds a tree of statuses.
 *
 * \param[in] server_status  The map of statuses.
 */
void snapserver_manager::on_retrieve_status(snap_manager::server_status & server_status)
{
    if(f_snap->stop_now_prima())
    {
        return;
    }

    // allow for editing the IP:port info
    //
    advgetopt::conf_file_setup config_setup(g_configuration_filename);
    advgetopt::conf_file::pointer_t snap_server_conf(advgetopt::conf_file::get_conf_file(config_setup));

    {
        snap_manager::status_t const host_list(
                      snap_manager::status_t::state_t::STATUS_STATE_INFO
                    , get_plugin_name()
                    , "listen"
                    , toQString(snap_server_conf->get_parameter("listen")));
        server_status.set_field(host_list);
    }

    // get the snapserver status
    //
    snap_manager::service_status_t status(f_snap->service_status("/usr/sbin/snapserver", "snapserver"));

    // transform to a string
    //
    QString const status_string(QString::fromUtf8(snap_manager::manager::service_status_to_string(status)));

    // create status widget
    //
    snap_manager::status_t const status_widget(
                    status == snap_manager::service_status_t::SERVICE_STATUS_NOT_INSTALLED
                            ? snap_manager::status_t::state_t::STATUS_STATE_ERROR
                            : status == snap_manager::service_status_t::SERVICE_STATUS_DISABLED
                                    ? snap_manager::status_t::state_t::STATUS_STATE_HIGHLIGHT
                                    : snap_manager::status_t::state_t::STATUS_STATE_INFO,
                    get_plugin_name(),
                    "service_status",
                    status_string);
    server_status.set_field(status_widget);
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
bool snapserver_manager::display_value(QDomElement parent, snap_manager::status_t const & s, snap::snap_uri const & uri)
{
    QDomDocument doc(parent.ownerDocument());

    if(s.get_field_name() == "listen")
    {
        // IP:port, usually remains 127.0.0.1 unless the server is moved
        // on a middle-end server.
        //
        // Also, unless we move snapmanager support to snapmanager.cgi/daemon
        // we need to connect and thus can use 0.0.0.0 here for that period
        // of time
        //
        snap_manager::form f(
                  get_plugin_name()
                , s.get_field_name()
                , snap_manager::form::FORM_BUTTON_RESET | snap_manager::form::FORM_BUTTON_SAVE
                );

        snap_manager::widget_input::pointer_t field(std::make_shared<snap_manager::widget_input>(
                          "Snap Server IP Addresses:"
                        , s.get_field_name()
                        , s.get_value()
                        , "By default we setup the Snap! Servers with IP address 127.0.0.1 and port 4004."
                         " If you move the Snap! Servers on a separate computer (not on the computer"
                         " with Apache2 and snap.cgi--i.e. the front end bundle,) then you will need to"
                         " change the IP address to your computer Private Network IP Address (if you use"
                         " OpenVPN, it is likely the tun0 IP address. If you do not use OpenVPN, it is the"
                         " address of something like eth1 or enp0s8.)"
                        ));
        f.add_widget(field);

        f.generate(parent, uri);

        return true;
    }

    if(s.get_field_name() == "service_status")
    {
        // The current status of the snapserver service
        //
        snap_manager::service_status_t const status(snap_manager::manager::string_to_service_status(s.get_value().toUtf8().data()));

        if(status == snap_manager::service_status_t::SERVICE_STATUS_NOT_INSTALLED)
        {
            // there is nothing we can do if it is not considered installed
            //
            snap_manager::form f(
                      get_plugin_name()
                    , s.get_field_name()
                    , snap_manager::form::FORM_BUTTON_NONE
                    );

            snap_manager::widget_description::pointer_t field(std::make_shared<snap_manager::widget_description>(
                              "Somehow the service plugin is still in place when the service was uninstalled"
                            , s.get_field_name()
                            , "This plugin should not be able to detect that the service in question is"
                             " uninstalled since the plugin is part of that service and thus it should"
                             " disappear along the main binary... Please report this bug."
                            ));
            f.add_widget(field);

            f.generate(parent, uri);
        }
        else
        {
            snap_manager::form f(
                      get_plugin_name()
                    , s.get_field_name()
                    , snap_manager::form::FORM_BUTTON_RESET | snap_manager::form::FORM_BUTTON_SAVE
                    );

            QStringList service_list;
            service_list << "disabled";
            service_list << "enabled";
            service_list << "active";
            service_list << "failed";

            snap_manager::widget_select::pointer_t field(std::make_shared<snap_manager::widget_select>(
                              "Enabled/Disabled/Activate Snap! Server"
                            , s.get_field_name()
                            , service_list
                            , s.get_value()
                            , "<p>Enter the new state of the snapserver"
                              " service as one of:</p>"
                              "<ul>"
                              "  <li>disabled -- deactivate and disable the service</li>"
                              "  <li>enabled -- enable the service, deactivate if it was activated</li>"
                              "  <li>active -- enable and activate the service</li>"
                              "</ul>"
                              "<p>You cannot request to go to the \"failed\" status."
                              " To uninstall search for the corresponding bundle and"
                              " click the <strong>Uninstall</strong> button.</p>"
                            ));
            f.add_widget(field);

            f.generate(parent, uri);
        }

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
bool snapserver_manager::apply_setting(QString const & button_name, QString const & field_name, QString const & new_value, QString const & old_or_installation_value, std::set<QString> & affected_services)
{
    snapdev::NOT_USED(old_or_installation_value, button_name, affected_services);

    if(field_name == "listen")
    {
        // to make use of the new list, make sure to restart
        //
        affected_services.insert("snapserver");

        // fix the value in memory
        //
        advgetopt::conf_file_setup setup(g_configuration_filename);
        advgetopt::conf_file::pointer_t snap_server_conf(advgetopt::conf_file::get_conf_file(setup));
        snap_server_conf.set_parameter("listen", new_value);

        return f_snap->replace_configuration_value(g_configuration_d_filename, "listen", new_value);
    }

    if(field_name == "service_status")
    {
        snap_manager::service_status_t const status(snap_manager::manager::string_to_service_status(new_value.toUtf8().data()));
        f_snap->service_apply_status("snapserver", status);
        return true;
    }

    return false;
}





SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
