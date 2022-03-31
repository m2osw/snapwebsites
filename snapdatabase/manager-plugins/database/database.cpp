// Snap Websites Server -- manage the snapdatabase settings
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
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA


// self
//
#include "database.h"


// our lib
//
#include <snapmanager/form.h>


// snapwebsites lib
//
#include <snapwebsites/file_content.h>
#include <snapwebsites/glob_dir.h>
#include <snapwebsites/process.h>
#include <snapwebsites/qdomhelpers.h>
#include <snapwebsites/qdomxpath.h>


// snaplogger lib
//
#include <snaplogger/message.h>


// snapdev lib
//
#include <snapdev/join_strings.h>
#include <snapdev/not_reached.h>
#include <snapdev/not_used.h>
#include <snapdev/pathinfo.h>


// Qt lib
//
#include <QFile>


// C lib
//
#include <sys/file.h>


// last include
//
#include <snapdev/poison.h>



SNAP_PLUGIN_START(snapdatabase, 1, 0)


namespace
{


char const *    g_conf_filename   = "/etc/network/firewall.conf";
char const *    g_firewall_script = "/etc/network/firewall";

char const *    g_conf_iplock_filename = "/etc/iplock/schemes/schemes.d/all.conf";
char const *    g_conf_iplock_glob     = "/etc/iplock/schemes/*.conf";


} // no name namespace



/** \brief Get a fixed snapdatabase plugin name.
 *
 * The snapdatabase plugin makes use of different fixed names. This function
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
    case name_t::SNAP_NAME_SNAPMANAGERCGI_SNAPDATABASE_ADMIN_IPS:
        return "admin_ips";

    case name_t::SNAP_NAME_SNAPMANAGERCGI_SNAPDATABASE_NAME:
        return "name";

    case name_t::SNAP_NAME_SNAPMANAGERCGI_SNAPDATABASE_PRIVATE_INTERFACE:
        return "private_interface";

    case name_t::SNAP_NAME_SNAPMANAGERCGI_SNAPDATABASE_PRIVATE_IP:
        return "private_ip";

    case name_t::SNAP_NAME_SNAPMANAGERCGI_SNAPDATABASE_PUBLIC_INTERFACE:
        return "public_interface";

    case name_t::SNAP_NAME_SNAPMANAGERCGI_SNAPDATABASE_PUBLIC_IP:
        return "public_ip";

    case name_t::SNAP_NAME_SNAPMANAGERCGI_SNAPDATABASE_SERVICE_STATUS:
        return "service_status";

    case name_t::SNAP_NAME_SNAPMANAGERCGI_SNAPDATABASE_WHITELIST:
        return "whitelist";

    default:
        // invalid index
        throw snap_logic_exception("Invalid SNAP_NAME_SNAPMANAGERCGI_SNAPDATABASE_...");

    }
    snapdev::NOT_REACHED();
}




/** \brief Initialize the snapdatabase plugin.
 *
 * This function is used to initialize the snapdatabase plugin object.
 */
snapdatabase::snapdatabase()
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Clean up the snapdatabase plugin.
 *
 * Ensure the snapdatabase object is clean before it is gone.
 */
snapdatabase::~snapdatabase()
{
}


/** \brief Get a pointer to the snapdatabase plugin.
 *
 * This function returns an instance pointer to the snapdatabase plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the snapdatabase plugin.
 */
snapdatabase * snapdatabase::instance()
{
    return g_plugin_snapdatabase_factory.instance();
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
QString snapdatabase::description() const
{
    return "Manage the snapdatabase settings.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString snapdatabase::dependencies() const
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
int64_t snapdatabase::do_update(int64_t last_updated)
{
    snapdev::NOT_USED(last_updated);

    SNAP_PLUGIN_UPDATE_INIT();
    // no updating in snapmanager*
    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Initialize snapdatabase.
 *
 * This function terminates the initialization of the snapdatabase plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void snapdatabase::bootstrap(snap_child * snap)
{
    f_snap = dynamic_cast<snap_manager::manager *>(snap);
    if(f_snap == nullptr)
    {
        throw snap_logic_exception("snap pointer does not represent a valid manager object.");
    }

    SNAP_LISTEN(snapdatabase, "server", snap_manager::manager, retrieve_status, boost::placeholders::_1);
    SNAP_LISTEN(snapdatabase, "server", snap_manager::manager, handle_affected_services, boost::placeholders::_1);
}


/** \brief Determine this plugin status data.
 *
 * This function builds a tree of statuses.
 *
 * \param[in] server_status  The map of statuses.
 */
void snapdatabase::on_retrieve_status(snap_manager::server_status & server_status)
{
    if(f_snap->stop_now_prima())
    {
        return;
    }

    {
        // get the snapdatabase status
        //
        snap_manager::service_status_t status(f_snap->service_status("/usr/sbin/snapdatabase", "snapdatabase"));

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
                        get_name(name_t::SNAP_NAME_SNAPMANAGERCGI_SNAPDATABASE_SERVICE_STATUS),
                        status_string);
        server_status.set_field(status_widget);
    }

    {
        // load the iplock configuration file named all.conf
        // we want the "whitelist" paramter
        //
        snap_config iplock_config(g_conf_iplock_filename);
        std::string whitelist;
        if(iplock_config.configuration_file_exists())
        {
            whitelist = iplock_config[get_name(name_t::SNAP_NAME_SNAPMANAGERCGI_SNAPDATABASE_WHITELIST)];
        }

        // create status widget
        //
        // TBD: shall we allow for the editing of each file separately?
        //
        snap_manager::status_t const status_widget(
                        snap_manager::status_t::state_t::STATUS_STATE_INFO,
                        get_plugin_name(),
                        get_name(name_t::SNAP_NAME_SNAPMANAGERCGI_SNAPDATABASE_WHITELIST),
                        QString::fromUtf8(whitelist.c_str()));
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


void snapdatabase::retrieve_settings_field(snap_manager::server_status & server_status,
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
                //
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
bool snapdatabase::display_value(QDomElement parent, snap_manager::status_t const & s, snap::snap_uri const & uri)
{
    QDomDocument doc(parent.ownerDocument());

    if(s.get_field_name() == get_name(name_t::SNAP_NAME_SNAPMANAGERCGI_SNAPDATABASE_SERVICE_STATUS))
    {
        // The current status of the snapdatabase service
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
                            , "<p>Enter the new state of the snapdatabase"
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

    if(s.get_field_name() == get_name(name_t::SNAP_NAME_SNAPMANAGERCGI_SNAPDATABASE_WHITELIST))
    {
        snap_manager::form f(
                  get_plugin_name()
                , s.get_field_name()
                , snap_manager::form::FORM_BUTTON_RESET | snap_manager::form::FORM_BUTTON_SAVE | snap_manager::form::FORM_BUTTON_SAVE_EVERYWHERE
                );

        snap_manager::widget_input::pointer_t field(std::make_shared<snap_manager::widget_input>(
                          "White listed comma separated IP addresses"
                        , s.get_field_name()
                        , s.get_value()
                        , "<p>Enter one or more IP addresses with an optional mask."
                         " For example, 10.4.32.0/24 will allow 256 IPs (10.4.32.0"
                         " to 10.4.32.255).</p>"
                         "<p>In general this feature is used to (1) whitelist your"
                         " own static IP address and (2) whitelist IP addresses of"
                         " computers performing PCI Compliance.</p>"
                         "<p><strong>WARNING:</strong> the field only shows the"
                         " IP addresses defined in the <code>all.conf</code> file."
                         " If you made manual changes to other files, do not use"
                         " this feature here. This save will replace the"
                         " \"whitelist\" parameter in all the .conf files found"
                         " under <code>/etc/iplock/schemes/schemes.d/</code>.</p>"
                        ));
        f.add_widget(field);

        f.generate(parent, uri);

        return true;
    }

    if(s.get_field_name() == get_name(name_t::SNAP_NAME_SNAPMANAGERCGI_SNAPDATABASE_PUBLIC_IP))
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

    if(s.get_field_name() == get_name(name_t::SNAP_NAME_SNAPMANAGERCGI_SNAPDATABASE_PUBLIC_INTERFACE))
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

    if(s.get_field_name() == get_name(name_t::SNAP_NAME_SNAPMANAGERCGI_SNAPDATABASE_PRIVATE_IP))
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

    if(s.get_field_name() == get_name(name_t::SNAP_NAME_SNAPMANAGERCGI_SNAPDATABASE_PRIVATE_INTERFACE))
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

    if(s.get_field_name() == get_name(name_t::SNAP_NAME_SNAPMANAGERCGI_SNAPDATABASE_ADMIN_IPS))
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
bool snapdatabase::apply_setting(QString const & button_name, QString const & field_name, QString const & new_value, QString const & old_or_installation_value, std::set<QString> & affected_services)
{
    snapdev::NOT_USED(old_or_installation_value, button_name);

    if(field_name == get_name(name_t::SNAP_NAME_SNAPMANAGERCGI_SNAPDATABASE_SERVICE_STATUS))
    {
        snap_manager::service_status_t const status(snap_manager::manager::string_to_service_status(new_value.toUtf8().data()));
        f_snap->service_apply_status("snapdatabase", status);
        return true;
    }

    if(field_name == get_name(name_t::SNAP_NAME_SNAPMANAGERCGI_SNAPDATABASE_WHITELIST))
    {
        // go through the list of .conf files in the schemes directory
        // and update the corresponding file in the schemes.d directory
        // (would the config do it automatically? since it's not under
        // /etc/snapwebsites, I'm not sure it does at the moment)
        //
        glob_dir iplock_conf(g_conf_iplock_glob, GLOB_NOSORT, true);
        iplock_conf.enumerate_glob([&](std::string filename)
            {
                // add the /schemes.d/ segment
                //
                std::string::size_type const pos(filename.rfind('/'));
                if(pos == std::string::npos)
                {
                    return;
                }
                std::string const path(filename.substr(0, pos));
                std::string const basename(filename.substr(pos + 1));

                std::string const conf_filename(path + "/schemes.d/" + basename);

                snapdev::NOT_USED(f_snap->replace_configuration_value(
                              QString::fromUtf8(conf_filename.c_str())
                            , field_name
                            , new_value
                            , snap_manager::REPLACE_CONFIGURATION_VALUE_CREATE_BACKUP));
            });

        snap_config iplock_config(g_conf_iplock_filename);
        iplock_config[field_name] = new_value;

        return true;
    }

    if(field_name == get_name(name_t::SNAP_NAME_SNAPMANAGERCGI_SNAPDATABASE_PUBLIC_IP))
    {
        affected_services.insert("firewall-reload");
        snapdev::NOT_USED(f_snap->replace_configuration_value(
                      g_conf_filename
                    , "PUBLIC_IP"
                    , new_value
                    , snap_manager::REPLACE_CONFIGURATION_VALUE_DOUBLE_QUOTE | snap_manager::REPLACE_CONFIGURATION_VALUE_MUST_EXIST));
        return true;
    }

    if(field_name == get_name(name_t::SNAP_NAME_SNAPMANAGERCGI_SNAPDATABASE_PUBLIC_INTERFACE))
    {
        affected_services.insert("firewall-reload");
        snapdev::NOT_USED(f_snap->replace_configuration_value(
                      g_conf_filename
                    , "PUBLIC_INTERFACE"
                    , new_value
                    , snap_manager::REPLACE_CONFIGURATION_VALUE_DOUBLE_QUOTE | snap_manager::REPLACE_CONFIGURATION_VALUE_MUST_EXIST));
        return true;
    }

    if(field_name == get_name(name_t::SNAP_NAME_SNAPMANAGERCGI_SNAPDATABASE_PRIVATE_IP))
    {
        affected_services.insert("firewall-reload");
        snapdev::NOT_USED(f_snap->replace_configuration_value(
                      g_conf_filename
                    , "PRIVATE_IP"
                    , new_value
                    , snap_manager::REPLACE_CONFIGURATION_VALUE_DOUBLE_QUOTE | snap_manager::REPLACE_CONFIGURATION_VALUE_MUST_EXIST));
        return true;
    }

    if(field_name == get_name(name_t::SNAP_NAME_SNAPMANAGERCGI_SNAPDATABASE_PRIVATE_INTERFACE))
    {
        affected_services.insert("firewall-reload");
        snapdev::NOT_USED(f_snap->replace_configuration_value(
                      g_conf_filename
                    , "PRIVATE_INTERFACE"
                    , new_value
                    , snap_manager::REPLACE_CONFIGURATION_VALUE_DOUBLE_QUOTE | snap_manager::REPLACE_CONFIGURATION_VALUE_MUST_EXIST));
        return true;
    }

    if(field_name == get_name(name_t::SNAP_NAME_SNAPMANAGERCGI_SNAPDATABASE_ADMIN_IPS))
    {
        affected_services.insert("firewall-reload");
        snapdev::NOT_USED(f_snap->replace_configuration_value(
                      g_conf_filename
                    , "ADMIN_IPS"
                    , new_value
                    , snap_manager::REPLACE_CONFIGURATION_VALUE_DOUBLE_QUOTE | snap_manager::REPLACE_CONFIGURATION_VALUE_MUST_EXIST));
        return true;
    }

    if(field_name == "private_network_ips")
    {
        affected_services.insert("firewall-reload");
        snapdev::NOT_USED(f_snap->replace_configuration_value(
                      g_conf_filename
                    , "PRIVATE_NETWORK_IPS"
                    , new_value
                    , snap_manager::REPLACE_CONFIGURATION_VALUE_DOUBLE_QUOTE | snap_manager::REPLACE_CONFIGURATION_VALUE_MUST_EXIST));
        return true;
    }

    if(field_name == "secure_ip")
    {
        affected_services.insert("firewall-reload");
        snapdev::NOT_USED(f_snap->replace_configuration_value(
                      g_conf_filename
                    , "SECURE_IP"
                    , new_value
                    , snap_manager::REPLACE_CONFIGURATION_VALUE_DOUBLE_QUOTE | snap_manager::REPLACE_CONFIGURATION_VALUE_MUST_EXIST));
        return true;
    }

    return false;
}


void snapdatabase::on_handle_affected_services(std::set<QString> & affected_services)
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
        snapdev::NOT_USED(p.run());           // errors are automatically logged by snap::process
    }
}





SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
