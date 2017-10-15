// Snap Websites Server -- manage the snapfirewall settings
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

// firewall
//
#include "firewall.h"

// our lib
//
#include <snapmanager/form.h>

// snapwebsites lib
//
#include <snapwebsites/file_content.h>
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


SNAP_PLUGIN_START(firewall, 1, 0)


namespace
{


char const *    g_conf_filename   = "/etc/network/firewall.conf";
char const *    g_firewall_script = "/etc/network/firewall";


//void file_descriptor_deleter(int * fd)
//{
//    if(close(*fd) != 0)
//    {
//        int const e(errno);
//        SNAP_LOG_WARNING("closing file descriptor failed (errno: ")(e)(", ")(strerror(e))(")");
//    }
//}


} // no name namespace



/** \brief Get a fixed firewall plugin name.
 *
 * The firewall plugin makes use of different fixed names. This function
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
    case name_t::SNAP_NAME_SNAPMANAGERCGI_FIREWALL_NAME:
        return "name";

    default:
        // invalid index
        throw snap_logic_exception("Invalid SNAP_NAME_SNAPMANAGERCGI_FIREWALL_...");

    }
    NOTREACHED();
}




/** \brief Initialize the firewall plugin.
 *
 * This function is used to initialize the firewall plugin object.
 */
firewall::firewall()
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Clean up the firewall plugin.
 *
 * Ensure the firewall object is clean before it is gone.
 */
firewall::~firewall()
{
}


/** \brief Get a pointer to the firewall plugin.
 *
 * This function returns an instance pointer to the firewall plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the firewall plugin.
 */
firewall * firewall::instance()
{
    return g_plugin_firewall_factory.instance();
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
QString firewall::description() const
{
    return "Manage the snapfirewall settings.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString firewall::dependencies() const
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
int64_t firewall::do_update(int64_t last_updated)
{
    NOTUSED(last_updated);

    SNAP_PLUGIN_UPDATE_INIT();
    // no updating in snapmanager*
    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Initialize firewall.
 *
 * This function terminates the initialization of the firewall plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void firewall::bootstrap(snap_child * snap)
{
    f_snap = dynamic_cast<snap_manager::manager *>(snap);
    if(f_snap == nullptr)
    {
        throw snap_logic_exception("snap pointer does not represent a valid manager object.");
    }

    SNAP_LISTEN(firewall, "server", snap_manager::manager, retrieve_status, _1);
    SNAP_LISTEN(firewall, "server", snap_manager::manager, handle_affected_services, _1);
}


/** \brief Determine this plugin status data.
 *
 * This function builds a tree of statuses.
 *
 * \param[in] server_status  The map of statuses.
 */
void firewall::on_retrieve_status(snap_manager::server_status & server_status)
{
    if(f_snap->stop_now_prima())
    {
        return;
    }

    {
        // get the snapfirewall status
        //
        snap_manager::service_status_t status(f_snap->service_status("/usr/bin/snapfirewall", "snapfirewall"));

        // transform to a string
        //
        QString const status_string(QString::fromUtf8(snap_manager::manager::service_status_to_string(status)));

        // create status widget
        //
        snap_manager::status_t const status_widget(
                        status == snap_manager::service_status_t::SERVICE_STATUS_NOT_INSTALLED
                                ? snap_manager::status_t::state_t::STATUS_STATE_ERROR
                                : status == snap_manager::service_status_t::SERVICE_STATUS_DISABLED
                                        ? snap_manager::status_t::state_t::STATUS_STATE_WARNING
                                        : snap_manager::status_t::state_t::STATUS_STATE_INFO,
                        get_plugin_name(),
                        "service_status",
                        status_string);
        server_status.set_field(status_widget);
    }

    {
        retrieve_settings_field(server_status, "PUBLIC_IP");
        retrieve_settings_field(server_status, "PUBLIC_INTERFACE");
        retrieve_settings_field(server_status, "PRIVATE_IP");
        retrieve_settings_field(server_status, "PRIVATE_INTERFACE");
        retrieve_settings_field(server_status, "ADMIN_IPS");
        retrieve_settings_field(server_status, "PRIVATE_NETWORK_IPS");
        retrieve_settings_field(server_status, "SECURE_IP");
    }
}


void firewall::retrieve_settings_field(snap_manager::server_status & server_status,
                                       std::string const & variable_name)
{
    // we want fields to be lowercase, when these variables are uppercase
    //
    std::string field_name(variable_name);
    std::transform(
              field_name.begin()
            , field_name.end()
            , field_name.begin()
            , [](char c)
            {
                // very quick and dirty since these are variable names
                // in a shell script...
                return c >= 'A' && c <= 'Z' ? c | 0x20 : c;
            });
    QString const qfield_name(QString::fromUtf8(field_name.c_str()));

    // get the data
    //
    file_content fc(g_conf_filename);
    if(fc.read_all())
    {
        // could read the file, go on
        //
        bool found(false);
        std::string const content(fc.get_content());
        std::string::size_type const pos(snap_manager::manager::search_parameter(content, variable_name + "=", 0, false));
        if(pos != std::string::size_type(-1))
        {
            // found one, get the content
            //
            std::string::size_type p1(pos + variable_name.length() + 1);
            if(p1 < content.length() && content[p1] == '"')
            {
                ++p1;
                std::string::size_type const p2(content.find_first_of("\"", p1));
                if(p2 != std::string::size_type(-1))
                {
                    std::string const value(content.substr(p1, p2 - p1));

                    snap_manager::status_t const conf_field(
                                      snap_manager::status_t::state_t::STATUS_STATE_INFO
                                    , get_plugin_name()
                                    , qfield_name
                                    , QString::fromUtf8(value.c_str()));
                    server_status.set_field(conf_field);

                    found = true;
                }
            }
        }
        if(!found)
        {
            // we got the file, but could not find the field as expected
            //
            snap_manager::status_t const conf_field(
                              snap_manager::status_t::state_t::STATUS_STATE_WARNING
                            , get_plugin_name()
                            , qfield_name
                            , QString("\"%1\" is not editable at the moment.").arg(g_conf_filename));
            server_status.set_field(conf_field);
        }
    }
    else if(fc.exists())
    {
        // create an error field which is not editable
        //
        snap_manager::status_t const conf_field(
                          snap_manager::status_t::state_t::STATUS_STATE_WARNING
                        , get_plugin_name()
                        , qfield_name
                        , QString("\"%1\" is not editable at the moment.").arg(g_conf_filename));
        server_status.set_field(conf_field);
    }
    // else -- file does not exist
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
bool firewall::display_value(QDomElement parent, snap_manager::status_t const & s, snap::snap_uri const & uri)
{
    QDomDocument doc(parent.ownerDocument());

    if(s.get_field_name() == "service_status")
    {
        // The current status of the snapfirewall service
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
                              "Enabled/Disabled/Activate Firewall"
                            , s.get_field_name()
                            , service_list
                            , s.get_value()
                            , "<p>Enter the new state of the snapfirewall"
                              " service as one of:</p>"
                              "<ul>"
                              "  <li>disabled -- deactivate and disable the service</li>"
                              "  <li>enabled -- enable the service, deactivate if it was activated</li>"
                              "  <li>active -- enable and activate the service</li>"
                              "</ul>"
                              "<p>You cannot request to go to the \"failed\" status."
                              " To uninstall search for the corresponding bundle and"
                              " click the <strong>Uninstall</strong> button.</p>"
                              "<p><strong>WARNING:</strong> The current snapmanagercgi"
                              " implementation does not clearly give you feedback if"
                              " you mispell the new status. We suggest you copy and"
                              " paste from this description to avoid mistakes.</p>"
                            ));
            f.add_widget(field);

            f.generate(parent, uri);
        }

        return true;
    }

    if(s.get_field_name() == "public_ip")
    {
        snap_manager::form f(
                  get_plugin_name()
                , s.get_field_name()
                , snap_manager::form::FORM_BUTTON_RESET | snap_manager::form::FORM_BUTTON_SAVE
                );

        snap_manager::widget_input::pointer_t field(std::make_shared<snap_manager::widget_input>(
                          "This Computer Public IP"
                        , s.get_field_name()
                        , s.get_value()
                        , "Enter the IP address of this computer, the one facing the Internet (often was eth0)."
                        ));
        f.add_widget(field);

        f.generate(parent, uri);

        return true;
    }

    if(s.get_field_name() == "public_interface")
    {
        snap_manager::form f(
                  get_plugin_name()
                , s.get_field_name()
                , snap_manager::form::FORM_BUTTON_RESET | snap_manager::form::FORM_BUTTON_SAVE
                );

        snap_manager::widget_input::pointer_t field(std::make_shared<snap_manager::widget_input>(
                          "The Interface This Computer uses for Public IP"
                        , s.get_field_name()
                        , s.get_value()
                        , "Enter the name of the interface (such as 'eth0') that this computer uses for his Public IP address."
                        ));
        f.add_widget(field);

        f.generate(parent, uri);

        return true;
    }

    if(s.get_field_name() == "private_ip")
    {
        snap_manager::form f(
                  get_plugin_name()
                , s.get_field_name()
                , snap_manager::form::FORM_BUTTON_RESET | snap_manager::form::FORM_BUTTON_SAVE
                );

        snap_manager::widget_input::pointer_t field(std::make_shared<snap_manager::widget_input>(
                          "This Computer Private IP"
                        , s.get_field_name()
                        , s.get_value()
                        , "Enter the private IP address of this computer, the one used to communicate with your other private computers (such as eth1)."
                        ));
        f.add_widget(field);

        f.generate(parent, uri);

        return true;
    }

    if(s.get_field_name() == "private_interface")
    {
        snap_manager::form f(
                  get_plugin_name()
                , s.get_field_name()
                , snap_manager::form::FORM_BUTTON_RESET | snap_manager::form::FORM_BUTTON_SAVE
                );

        snap_manager::widget_input::pointer_t field(std::make_shared<snap_manager::widget_input>(
                          "The Interface This Computer uses for Private IP"
                        , s.get_field_name()
                        , s.get_value()
                        , "Enter the name of the interface (such as 'eth1') that this computer uses for his Private IP address."
                        ));
        f.add_widget(field);

        f.generate(parent, uri);

        return true;
    }

    if(s.get_field_name() == "admin_ips")
    {
        snap_manager::form f(
                  get_plugin_name()
                , s.get_field_name()
                , snap_manager::form::FORM_BUTTON_RESET | snap_manager::form::FORM_BUTTON_SAVE | snap_manager::form::FORM_BUTTON_SAVE_EVERYWHERE
                );

        snap_manager::widget_input::pointer_t field(std::make_shared<snap_manager::widget_input>(
                          "List of Administrator IPs"
                        , s.get_field_name()
                        , s.get_value()
                        , "Enter the <strong>space separated</strong> list of IPs that your administrators use to access this computer."
                        ));
        f.add_widget(field);

        f.generate(parent, uri);

        return true;
    }

    if(s.get_field_name() == "private_network_ips")
    {
        snap_manager::form f(
                  get_plugin_name()
                , s.get_field_name()
                , snap_manager::form::FORM_BUTTON_RESET | snap_manager::form::FORM_BUTTON_SAVE | snap_manager::form::FORM_BUTTON_SAVE_EVERYWHERE
                );

        snap_manager::widget_input::pointer_t field(std::make_shared<snap_manager::widget_input>(
                          "List of Private Network IPs"
                        , s.get_field_name()
                        , s.get_value()
                        , "Enter the <strong>space separated</strong> list of IPs of all the computers present in your private network."
                        ));
        f.add_widget(field);

        f.generate(parent, uri);

        return true;
    }

    if(s.get_field_name() == "secure_ip")
    {
        snap_manager::form f(
                  get_plugin_name()
                , s.get_field_name()
                , snap_manager::form::FORM_BUTTON_RESET | snap_manager::form::FORM_BUTTON_SAVE
                );

        snap_manager::widget_input::pointer_t field(std::make_shared<snap_manager::widget_input>(
                          "Secure IP"
                        , s.get_field_name()
                        , s.get_value()
                        , "Enter the secure IP of this computer if you have one."
                         " This is most often the <code>tun0</code> IP address"
                         " created by OpenVPN. An address such as 10.8.0.34."
                         " This field can remain empty if you are not using"
                         " OpenVPN on your private network."
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
bool firewall::apply_setting(QString const & button_name, QString const & field_name, QString const & new_value, QString const & old_or_installation_value, std::set<QString> & affected_services)
{
    NOTUSED(old_or_installation_value);
    NOTUSED(button_name);

    if(field_name == "service_status")
    {
        snap_manager::service_status_t const status(snap_manager::manager::string_to_service_status(new_value.toUtf8().data()));
        f_snap->service_apply_status("snapfirewall", status);
        return true;
    }

    if(field_name == "public_ip")
    {
        affected_services.insert("firewall-reload");
        NOTUSED(f_snap->replace_configuration_value(
                      g_conf_filename
                    , "PUBLIC_IP"
                    , new_value
                    , snap_manager::REPLACE_CONFIGURATION_VALUE_DOUBLE_QUOTE | snap_manager::REPLACE_CONFIGURATION_VALUE_MUST_EXIST));
        return true;
    }

    if(field_name == "public_interface")
    {
        affected_services.insert("firewall-reload");
        NOTUSED(f_snap->replace_configuration_value(
                      g_conf_filename
                    , "PUBLIC_INTERFACE"
                    , new_value
                    , snap_manager::REPLACE_CONFIGURATION_VALUE_DOUBLE_QUOTE | snap_manager::REPLACE_CONFIGURATION_VALUE_MUST_EXIST));
        return true;
    }

    if(field_name == "private_ip")
    {
        affected_services.insert("firewall-reload");
        NOTUSED(f_snap->replace_configuration_value(
                      g_conf_filename
                    , "PRIVATE_IP"
                    , new_value
                    , snap_manager::REPLACE_CONFIGURATION_VALUE_DOUBLE_QUOTE | snap_manager::REPLACE_CONFIGURATION_VALUE_MUST_EXIST));
        return true;
    }

    if(field_name == "private_interface")
    {
        affected_services.insert("firewall-reload");
        NOTUSED(f_snap->replace_configuration_value(
                      g_conf_filename
                    , "PRIVATE_INTERFACE"
                    , new_value
                    , snap_manager::REPLACE_CONFIGURATION_VALUE_DOUBLE_QUOTE | snap_manager::REPLACE_CONFIGURATION_VALUE_MUST_EXIST));
        return true;
    }

    if(field_name == "admin_ips")
    {
        affected_services.insert("firewall-reload");
        NOTUSED(f_snap->replace_configuration_value(
                      g_conf_filename
                    , "ADMIN_IPS"
                    , new_value
                    , snap_manager::REPLACE_CONFIGURATION_VALUE_DOUBLE_QUOTE | snap_manager::REPLACE_CONFIGURATION_VALUE_MUST_EXIST));
        return true;
    }

    if(field_name == "private_network_ips")
    {
        affected_services.insert("firewall-reload");
        NOTUSED(f_snap->replace_configuration_value(
                      g_conf_filename
                    , "PRIVATE_NETWORK_IPS"
                    , new_value
                    , snap_manager::REPLACE_CONFIGURATION_VALUE_DOUBLE_QUOTE | snap_manager::REPLACE_CONFIGURATION_VALUE_MUST_EXIST));
        return true;
    }

    if(field_name == "secure_ip")
    {
        affected_services.insert("firewall-reload");
        NOTUSED(f_snap->replace_configuration_value(
                      g_conf_filename
                    , "SECURE_IP"
                    , new_value
                    , snap_manager::REPLACE_CONFIGURATION_VALUE_DOUBLE_QUOTE | snap_manager::REPLACE_CONFIGURATION_VALUE_MUST_EXIST));
        return true;
    }

    return false;
}


void firewall::on_handle_affected_services(std::set<QString> & affected_services)
{
    auto const & it_reload(std::find(affected_services.begin(), affected_services.end(), QString("firewall-reload")));
    if(it_reload != affected_services.end())
    {
        // remove since we are handling that one here
        //
        affected_services.erase(it_reload);

        // run the firewall script to apply the changes
        //
        snap::process p("reload firewall");
        p.set_mode(snap::process::mode_t::PROCESS_MODE_COMMAND);
        p.set_command(g_firewall_script);
        NOTUSED(p.run());           // errors are automatically logged by snap::process
    }
}





SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
