// Snap Websites Server -- manage the snapwatchdog settings
// Copyright (c) 2016-2018  Made to Order Software Corp.  All Rights Reserved
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

// watchdog
//
#include "watchdog.h"

// our lib
//
#include <snapmanager/form.h>

// snapwebsites lib
//
#include <snapwebsites/glob_dir.h>
#include <snapwebsites/join_strings.h>
#include <snapwebsites/log.h>
#include <snapwebsites/not_reached.h>
#include <snapwebsites/not_used.h>
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


SNAP_PLUGIN_START(watchdog, 1, 0)


namespace
{


char const * g_configuration_filename = "snapwatchdog";

char const * g_configuration_d_filename = "/etc/snapwebsites/snapwebsites.d/snapwatchdog.conf";




} // no name namespace



/** \brief Get a fixed watchdog plugin name.
 *
 * The watchdog plugin makes use of different fixed names. This function
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
    case name_t::SNAP_NAME_SNAPMANAGERCGI_WATCHDOG_NAME:
        return "name";

    default:
        // invalid index
        throw snap_logic_exception("Invalid SNAP_NAME_SNAPMANAGERCGI_WATCHDOG_...");

    }
    NOTREACHED();
}




/** \brief Initialize the watchdog plugin.
 *
 * This function is used to initialize the watchdog plugin object.
 */
watchdog::watchdog()
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Clean up the watchdog plugin.
 *
 * Ensure the watchdog object is clean before it is gone.
 */
watchdog::~watchdog()
{
}


/** \brief Get a pointer to the watchdog plugin.
 *
 * This function returns an instance pointer to the watchdog plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the watchdog plugin.
 */
watchdog * watchdog::instance()
{
    return g_plugin_watchdog_factory.instance();
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
QString watchdog::description() const
{
    return "Manage the snapwatchdog settings.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString watchdog::dependencies() const
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
int64_t watchdog::do_update(int64_t last_updated)
{
    NOTUSED(last_updated);

    SNAP_PLUGIN_UPDATE_INIT();
    // no updating in snapmanager*
    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Initialize watchdog.
 *
 * This function terminates the initialization of the watchdog plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void watchdog::bootstrap(snap_child * snap)
{
    f_snap = dynamic_cast<snap_manager::manager *>(snap);
    if(f_snap == nullptr)
    {
        throw snap_logic_exception("snap pointer does not represent a valid manager object.");
    }

    SNAP_LISTEN(watchdog, "server", snap_manager::manager, retrieve_status, _1);
}


/** \brief Determine this plugin status data.
 *
 * This function builds a tree of statuses.
 *
 * \param[in] server_status  The map of statuses.
 */
void watchdog::on_retrieve_status(snap_manager::server_status & server_status)
{
    if(f_snap->stop_now_prima())
    {
        return;
    }

    {
        // get the snapwatchdog status
        //
        snap_manager::service_status_t status(f_snap->service_status("/usr/bin/snapwatchdogserver", "snapwatchdog"));

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

    snap_config snap_watchdog_conf(g_configuration_filename);

    {
        snap_manager::status_t const from_email(
                      snap_manager::status_t::state_t::STATUS_STATE_INFO
                    // We need access to the value here?!
                    //  from_email_value.isEmpty()
                    //        ? snap_manager::status_t::state_t::STATUS_STATE_ERROR
                    //        : snap_manager::status_t::state_t::STATUS_STATE_INFO
                    , get_plugin_name()
                    , "from_email"
                    , snap_watchdog_conf["from_email"]);
        server_status.set_field(from_email);
    }

    {
        snap_manager::status_t const administrator_email(
                      snap_manager::status_t::state_t::STATUS_STATE_INFO
                    // We need access to the value here?!
                    //  administrator_email_value.isEmpty()
                    //        ? snap_manager::status_t::state_t::STATUS_STATE_ERROR
                    //        : snap_manager::status_t::state_t::STATUS_STATE_INFO
                    , get_plugin_name()
                    , "administrator_email"
                    , snap_watchdog_conf["administrator_email"]);
        server_status.set_field(administrator_email);
    }

    {
        snap_manager::status_t const plugins(
                      snap_manager::status_t::state_t::STATUS_STATE_INFO
                    , get_plugin_name()
                    , "plugins"
                    , snap_watchdog_conf["plugins"]);
        server_status.set_field(plugins);
    }

    // check that we have an MTA, which means having a "sendmail" tool
    //
    struct stat st;
    if(stat("/usr/sbin/sendmail", &st) != 0)
    {
        snap_manager::status_t const plugins(
                      snap_manager::status_t::state_t::STATUS_STATE_ERROR
                    , get_plugin_name()
                    , "no-mta"
                    , "not available");
        server_status.set_field(plugins);
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
bool watchdog::display_value(QDomElement parent, snap_manager::status_t const & s, snap::snap_uri const & uri)
{
    QDomDocument doc(parent.ownerDocument());

    if(s.get_field_name() == "service_status")
    {
        // The current status of the snapwatchdog service
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
                    ,   snap_manager::form::FORM_BUTTON_RESET
                      | snap_manager::form::FORM_BUTTON_SAVE
                    );

            QStringList service_list;
            service_list << "disabled";
            service_list << "enabled";
            service_list << "active";
            service_list << "failed";

            snap_manager::widget_select::pointer_t field(std::make_shared<snap_manager::widget_select>(
                              "Enabled/Disabled/Activate watchdog"
                            , s.get_field_name()
                            , service_list
                            , s.get_value()
                            , "<p>Enter the new state of the snapwatchdog"
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

    if(s.get_field_name() == "from_email")
    {
        // the list if frontend snapmanagers that are to receive statuses
        // of the cluster computers; may be just one computer; should not
        // be empty; shows a text input field
        //
        snap_manager::form f(
                  get_plugin_name()
                , s.get_field_name()
                ,   snap_manager::form::FORM_BUTTON_RESET
                  | snap_manager::form::FORM_BUTTON_SAVE
                  | snap_manager::form::FORM_BUTTON_SAVE_EVERYWHERE
                );

        snap_manager::widget_input::pointer_t field(std::make_shared<snap_manager::widget_input>(
                          "From Email:"
                        , s.get_field_name()
                        , s.get_value()
                        , "<p>The email address to use in the From: ... field. It has to be a valid email"
                         " because your Postfix installation is otherwise going to fail forwarding"
                         " emails anywhere.</p>"
                        ));
        f.add_widget(field);

        f.generate(parent, uri);

        return true;
    }

    if(s.get_field_name() == "administrator_email")
    {
        // the list if frontend snapmanagers that are to receive statuses
        // of the cluster computers; may be just one computer; should not
        // be empty; shows a text input field
        //
        snap_manager::form f(
                  get_plugin_name()
                , s.get_field_name()
                ,   snap_manager::form::FORM_BUTTON_RESET
                  | snap_manager::form::FORM_BUTTON_SAVE
                  | snap_manager::form::FORM_BUTTON_SAVE_EVERYWHERE
                );

        snap_manager::widget_input::pointer_t field(std::make_shared<snap_manager::widget_input>(
                          "Administrator Email:"
                        , s.get_field_name()
                        , s.get_value()
                        , "<p>The email address where the Watchdog emails get send."
                         " Obviously, this has to be a valid email that you are"
                         " going to receive. The idea is that the Watchdog sends"
                         " you emails about problems and you come to your servers"
                         " and fix. An invalid email would not make sense, right?</p>"
                        ));
        f.add_widget(field);

        f.generate(parent, uri);

        return true;
    }

    if(s.get_field_name() == "plugins")
    {
        // the list of enabled plugins
        //
        snap_manager::form f(
                  get_plugin_name()
                , s.get_field_name()
                ,   snap_manager::form::FORM_BUTTON_RESET
                  | snap_manager::form::FORM_BUTTON_SAVE
                  | snap_manager::form::FORM_BUTTON_RESTORE_DEFAULT
                );

        // get the list of watchdog plugins that are available on this
        // computer
        //
        QString const available_plugins(get_list_of_available_plugins());

        QString const plugin_names(s.get_value());
        snap_string_list plugin_names_list(plugin_names.split(','));
        plugin_names_list.sort();
        QString const plugin_names_lined(plugin_names_list.join('\n'));

        snap_manager::widget_text::pointer_t field(std::make_shared<snap_manager::widget_text>(
                          "List of Watchdog plugins to run on this system"
                        , s.get_field_name()
                        , plugin_names_lined
                        , "<p>Enter the name of each of the plugin you want to run on this system,"
                            " one per line. Spaces and tabs will be ignored.</p>"
                          "<p>The current default is:</p>"
                          "<ul>"
                            "<li>cpu</li>"
                            "<li>disk</li>"
                            "<li>memory</li>"
                            "<li>network</li>"
                            "<li>processes</li>"
                            "<li>watchscripts</li>"
                          "</ul>"
                          "<p>The plugins currently available on this system are:</p>"
                          "<ul>"
                        + available_plugins
                        + "</ul>"
                        ));
        f.add_widget(field);
        f.generate(parent, uri);
        return true;
    }

    if(s.get_field_name() == "no-mta")
    {
        // the list of enabled plugins
        //
        snap_manager::form f(
                  get_plugin_name()
                , s.get_field_name()
                , 0
                );

        snap_manager::widget_input::pointer_t field(std::make_shared<snap_manager::widget_input>(
                          "Missing MTA"
                        , s.get_field_name()
                        , "ignored"
                        , "<p>For the full functioning of the snapwatchdog daemon, you must"
                            " install an MTA. It is also useful to get any kind of email to"
                            " you. For example, a CRON script may be failing and it will"
                            " attempt to send you an email. However, without the MTA you"
                            " won't get any of those emails.</p>"
                          "<p>On at least one computer, generally a backend, you want to"
                          " install Postfix (the snapmailserver bundle.) On the other"
                          " computers, you want to install the Snap! MTA which is very"
                          " small and very fast and uses a very small amount of memory"
                          " only when an email is being sent. This is done by installing"
                          " the snapmta bundle.</p>"
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
bool watchdog::apply_setting(QString const & button_name, QString const & field_name, QString const & new_value, QString const & old_or_installation_value, std::set<QString> & affected_services)
{
    NOTUSED(old_or_installation_value);
    NOTUSED(button_name);

    bool const use_default_value(button_name == "restore_default");

    if(field_name == "service_status")
    {
        snap_manager::service_status_t const status(snap_manager::manager::string_to_service_status(new_value.toUtf8().data()));
        f_snap->service_apply_status("snapwatchdog", status);
        return true;
    }

    if(field_name == "from_email")
    {
        // to make use of the new list, make sure to restart
        //
        affected_services.insert("snapwatchdog");

        // fix the value in memory
        //
        snap_config snap_watchdog_conf(g_configuration_filename);
        snap_watchdog_conf["from_email"] = new_value;

        f_snap->replace_configuration_value(g_configuration_d_filename, "from_email", new_value);
        return true;
    }

    if(field_name == "administrator_email")
    {
        // to make use of the new list, make sure to restart
        //
        affected_services.insert("snapwatchdog");

        // fix the value in memory
        //
        snap_config snap_watchdog_conf(g_configuration_filename);
        snap_watchdog_conf["administrator_email"] = new_value;

        f_snap->replace_configuration_value(g_configuration_d_filename, "administrator_email", new_value);
        return true;
    }

    if(field_name == "plugins")
    {
        // fix the value in memory
        //
        QString const available_plugins(get_list_of_available_plugins());
        snap_config snap_watchdog_conf(g_configuration_filename);
        snap_string_list names;
        if(use_default_value)
        {
            // restore to the default list of plugins
            //
            names << "cpu";
            names << "disk";
            names << "memory";
            names << "network";
            names << "processes";
            names << "watchscripts";
        }
        else
        {
            names = new_value.split('\n');
        }
        snap_string_list clean_names;
        int const max(names.size());
        for(int idx(0); idx < max; ++idx)
        {
            QString const t(names[idx].trimmed());
            if(!t.isEmpty())
            {
                if(available_plugins.indexOf("<li>" + t + "</li>") == -1)
                {
                    // probably mispelled, it would break the load so don't
                    // allow it in the .conf file
                    //
                    SNAP_LOG_ERROR("Could not find plugin named \"")
                                  (t)
                                  ("\" in the list of available plugins. Please try again.");
                    return true;
                }

                clean_names << t;
            }
        }
        QString const new_list_of_plugins(clean_names.join(','));
        snap_watchdog_conf["plugins"] = new_list_of_plugins;

        // to make use of the new list, make sure to restart
        //
        affected_services.insert("snapwatchdog");

        f_snap->replace_configuration_value(g_configuration_d_filename, "plugins", new_list_of_plugins);
        return true;
    }

    return false;
}


/** \brief Generate a list of plugins.
 *
 * This function generates an HTML list of watchdog plugin names. The list
 * comes from the plugins we can find in the "plugins_path" folder.
 *
 * \return An HTML string with the list of plugin names.
 */
QString watchdog::get_list_of_available_plugins()
{
    QString available_plugins;

    char const * plugins_path_variable_name(snap::get_name(snap::name_t::SNAP_NAME_CORE_PARAM_PLUGINS_PATH));
    snap_config snap_watchdog_conf(g_configuration_filename);
    std::string const & plugins_path(snap_watchdog_conf[plugins_path_variable_name]);
    try
    {
        glob_dir const plugin_filenames(plugins_path + "/*watchdog_*.so", GLOB_ERR | GLOB_NOESCAPE);
        plugin_filenames.enumerate_glob(std::bind(&watchdog::get_plugin_names, this, std::placeholders::_1, &available_plugins));
    }
    catch(glob_dir_exception const &)
    {
        // on a developer system files are going to be in a subfolder
        // so try again just in case
        //
        try
        {
            glob_dir const plugin_filenames(plugins_path + "/watchdog_*/*watchdog_*.so", GLOB_ERR | GLOB_NOESCAPE);
            plugin_filenames.enumerate_glob(std::bind(&watchdog::get_plugin_names, this, std::placeholders::_1, &available_plugins));
        }
        catch(glob_dir_exception const &)
        {
            // reading that list failed?!
            // (we presentthe error as a list item since we are
            // going to be added inside a list)
            //
            available_plugins += "<li><strong style=\"color: red\">An error occurred while reading the list of available plugins.</li>";
        }
    }

    return available_plugins;
}


/** \brief Append the name of each plugin in the \p available_plugins string.
 *
 * This function is called once per watchdog plugin. It saves the filename
 * in the plugin_name string. Each name is saved between an \<li> tag so
 * it looks like a list element.
 *
 * \param[in] plugin_filename  The name of one plugin.
 */
void watchdog::get_plugin_names(QString plugin_filename, QString * available_plugins)
{
    int const basename_pos(plugin_filename.lastIndexOf('/') + 1);
    int name_pos(plugin_filename.indexOf('_', basename_pos));
    if(name_pos == -1)
    {
        name_pos = basename_pos;
    }
    else
    {
        // skip the '_'
        //
        ++name_pos;
    }
    int extension_pos(plugin_filename.indexOf(".so", name_pos));
    if(extension_pos == -1)
    {
        extension_pos = plugin_filename.length();
    }
    *available_plugins += "<li>" + plugin_filename.mid(name_pos, extension_pos - name_pos) + "</li>";
}





SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
