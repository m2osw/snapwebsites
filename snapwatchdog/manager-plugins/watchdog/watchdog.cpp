// Snap Websites Server -- manage the snapwatchdog settings
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
#include "watchdog.h"


// snapmanager lib
//
#include <snapmanager/form.h>
#include <snapmanager/version.h>


// it's not under snapmanager from within snapwatchdog
#include <snapmanagercgi.h>


// snapwebsites lib
//
#include <snapwebsites/email.h>
#include <snapwebsites/glob_dir.h>
#include <snapwebsites/log.h>
#include <snapwebsites/qdomhelpers.h>
#include <snapwebsites/qdomxpath.h>


// snapdev lib
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



SNAP_PLUGIN_START(watchdog, 1, 0)


namespace
{


char const * g_default_watchdog_plugins = "apt"
                                         ",cpu"
                                         ",disk"
                                         ",flags"
                                         ",log"
                                         ",memory"
                                         ",network"
                                         ",packages"
                                         ",processes"
                                         ",watchscripts"
                                         ;


char const * g_configuration_filename = "snapwatchdog";

char const * g_configuration_d_filename = "/etc/snapwebsites/snapwebsites.d/snapwatchdog.conf";

char const * g_test_mta_results = "/var/lib/snapwebsites/snapwatchdog/test-mta.txt";

char const * g_watchdog_last_result_filename = "/var/cache/snapwebsites/snapwatchdog/last_results.txt";



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
    case name_t::SNAP_NAME_SNAPMANAGERCGI_WATCHDOG_FROM_EMAIL:
        return "from_email";

    default:
        // invalid index
        throw snap_logic_exception("Invalid SNAP_NAME_SNAPMANAGERCGI_WATCHDOG_...");

    }
    snapdev::NOT_REACHED();
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
    snapdev::NOT_USED(last_updated);

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

    SNAP_LISTEN(watchdog, "server", snap_manager::manager, retrieve_status, boost::placeholders::_1);

    // we cannot use dynamic_cast<>() because it accesses the typeinfo of
    // snap_manager::manager_cgi which creates a linkage problem (i.e. when
    // not loaded from snapmanagercgi the plugin fails linkage)
    //
    //     // the following requires a typeinfo variable linkage
    //     snap_manager::manager_cgi * cgi(dynamic_cast<snap_manager::manager_cgi *>(f_snap));
    //
    // so for this reason we instead use a simple virtual function for
    // the "same" information--that works perfectly
    //
    std::string const type(f_snap->server_type());
    if(type == "manager_cgi")
    {
        SNAP_LISTEN(watchdog, "server", snap_manager::manager_cgi, generate_content, boost::placeholders::_1, boost::placeholders::_2, boost::placeholders::_3, boost::placeholders::_4, boost::placeholders::_5);
    }
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
        snap_manager::service_status_t status(f_snap->service_status("/usr/sbin/snapwatchdogserver", "snapwatchdog"));

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
    {
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
        else
        {
            QString email;

            // avoid the try/catch in case the file does not exist yet
            //
            if(stat(g_test_mta_results, &st) == 0)
            {
                try
                {
                    snap_config test_mta(g_test_mta_results);
                    email = test_mta["email"];
                }
                catch(snap_configurations_exception const & e)
                {
                    SNAP_LOG_DEBUG("test_mta could not be read.");
                    // ignore the error otherwise
                }
            }

            snap_manager::status_t const plugins(
                          snap_manager::status_t::state_t::STATUS_STATE_INFO
                        , get_plugin_name()
                        , "test_mta"
                        , email);
            server_status.set_field(plugins);
        }
    }

    // Get the latest status first
    // (TBD: I think this will not change often enough...)
    //
    {
        std::string error_count("0");
        struct stat st;
        if(stat(g_watchdog_last_result_filename, &st) == 0)
        {
            snap_config const last_results(g_watchdog_last_result_filename);
            error_count = last_results["error_count"];
        }
        snap_manager::status_t const plugins(
                      std::stoi(error_count) == 0
                            ? snap_manager::status_t::state_t::STATUS_STATE_INFO
                            : snap_manager::status_t::state_t::STATUS_STATE_ERROR
                    , get_plugin_name()
                    , "last_results"
                    , error_count.c_str()); // no need for Utf8, a number is ASCII
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

        // we "dynamically" generate the list of default plugins so that
        // way we can have it defined once; "unfortunately" C/C++ does
        // not have a way to prepare a string at compile time without
        // writing complex templates...
        //
        QString const default_plugins_string(g_default_watchdog_plugins);
        snap::snap_string_list const default_plugins(default_plugins_string.split(','));
        QString const default_plugins_list(default_plugins.join("</li><li>"));

        snap_manager::widget_text::pointer_t field(std::make_shared<snap_manager::widget_text>(
                          "List of Watchdog plugins to run on this system"
                        , s.get_field_name()
                        , plugin_names_lined
                        , "<p>Enter the name of each of the plugin you want to run on this system,"
                            " one per line. Spaces and tabs will be ignored.</p>"
                          "<p>The current default is:</p>"
                          "<ul>"
                            "<li>" + default_plugins_list + "</li>"
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

    if(s.get_field_name() == "test_mta")
    {
        // the list of enabled plugins
        //
        snap_manager::form f(
                  get_plugin_name()
                , s.get_field_name()
                , snap_manager::form::FORM_BUTTON_SAVE
                );

        QString status;

        // avoid the try/catch in case the file does not exist yet
        //
        struct stat st;
        if(stat(g_test_mta_results, &st) == 0)
        {
            try
            {
                snap_config const test_mta(g_test_mta_results);
                status = QString::fromUtf8(test_mta["status"].c_str());
            }
            catch(snap_configurations_exception const & e)
            {
                SNAP_LOG_DEBUG("test_mta could not be read.");
                // ignore the error otherwise
            }
        }
        if(status.isEmpty())
        {
            status = "<p>The MTA was not yet tested through the Watchdog.</p>";
        }

        bool const has_root_emails(stat("/var/mail/root", &st) == 0);

        snap_manager::widget_input::pointer_t field(std::make_shared<snap_manager::widget_input>(
                          "Send a test email"
                        , s.get_field_name()
                        , s.get_value()
                        , "<p>Enter an email address and click the Save button to make sure that"
                          " sendmail works as expected.</p>"
                        + status
                        + (has_root_emails
                            ? "<p><strong>WARNING:</strong> root user has emails on this computer."
                              " This could mean that the MTA is not functional or incorrectly setup.</p>"
                            : "")
                        ));
        f.add_widget(field);
        f.generate(parent, uri);
        return true;
    }

    if(s.get_field_name() == "last_results")
    {
        // the list of enabled plugins
        //
        snap_manager::form f(
                  get_plugin_name()
                , s.get_field_name()
                , snap_manager::form::FORM_BUTTON_NONE
                );

        // try to be as close as possible to the latest results, only
        // the info/error status won't be tweaked in time here...
        //
        std::string error_count("0");
        struct stat st;
        if(stat(g_watchdog_last_result_filename, &st) == 0)
        {
            // read a couple of values from the last result file
            //
            snap_config const last_results(g_watchdog_last_result_filename);
            error_count = last_results["error_count"];
        }
        int const errcnt(std::stoi(error_count));

        // we should always have a snapmanager_cgi here
        //
        QString const host(uri.query_option("host"));
        snap_manager::widget_description::pointer_t field(std::make_shared<snap_manager::widget_description>(
                          "Snap! Watchdog Last Results"
                        , s.get_field_name()
                        , QString("<p>The snapwatchdogserver found %1 error%2.</p>"
                                  "<p><a href=\"/snapmanager?host=%3&amp;function=watchdog&amp;position=latest\">View the last results</a></p>")
                                    .arg(errcnt == 0 ? "no" : error_count.c_str())
                                    .arg(errcnt == 1 ? "" : "s")
                                    .arg(host)
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
    snapdev::NOT_USED(old_or_installation_value, button_name);

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
            QString const default_plugins_string(g_default_watchdog_plugins);
            names = default_plugins_string.split(',');
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

    if(field_name == "test_mta")
    {
        // to make use of the new test MTA parameters, make sure to restart
        //
        affected_services.insert("snapwatchdog");

        snap_config snap_watchdog_conf(g_configuration_filename);
        QString const from_email(snap_watchdog_conf["from_email"]);

        //QString const from_email(f_snap->get_server_parameter(get_name(name_t::SNAP_NAME_SNAPMANAGERCGI_WATCHDOG_FROM_EMAIL)));
        if(from_email.isEmpty())
        {
            f_snap->replace_configuration_value(
                          g_test_mta_results
                        , "status"
                        , "The `from_email` must be defined to test the MTA from the watchdog.");
        }
        else if(new_value.isEmpty())
        {
            f_snap->replace_configuration_value(
                          g_test_mta_results
                        , "status"
                        , "You must enter a test_mta email address so we can send the test email somewhere.");
        }
        else
        {
            // create the email and add a few headers
            //
            email e;
            e.set_from(from_email);
            e.set_to(new_value);
            e.set_priority(email::priority_t::EMAIL_PRIORITY_NORMAL);

            char hostname[HOST_NAME_MAX + 1];
            if(gethostname(hostname, sizeof(hostname)) != 0)
            {
                strncpy(hostname, "<unknown>", sizeof(hostname));
            }
            QString const subject(QString("snapwatchdog: test email from %1")
                            .arg(hostname));
            e.set_subject(subject);

            e.add_header("X-SnapWatchdog-Version", SNAPMANAGER_VERSION_STRING);

            // prevent blacklisting
            // (since we won't run the `sendmail` plugin validation, it's not necessary)
            //e.add_parameter(sendmail::get_name(sendmail::name_t::SNAP_NAME_SENDMAIL_BYPASS_BLACKLIST), "true");

            // create the body
            //
            email::attachment text;
            text.quoted_printable_encode_and_set_data("This is the test email from your Snap! Watchdog.", "text/plain");
            e.set_body_attachment(text);

            // finally send email
            //
            e.send();

            f_snap->replace_configuration_value(g_test_mta_results, "status", "Email was sent. Now verify that you receive it.");
            f_snap->replace_configuration_value(g_test_mta_results, "email", new_value);
        }
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
    snap_string_list available_plugins;

    char const * plugins_path_variable_name(snap::get_name(snap::name_t::SNAP_NAME_CORE_PARAM_PLUGINS_PATH));
    snap_config snap_watchdog_conf(g_configuration_filename);
    std::string const & plugins_paths(snap_watchdog_conf[plugins_path_variable_name]);

    std::vector<std::string> paths;
    snapdev::tokenize_string(paths, plugins_paths, ":", true, " ");

    for(auto p : paths)
    {
        try
        {
            // we do not need to have glob() sort for us since we may have multiple
            // lists and they would not be properly sorted in the end, instead
            // we sort our available_plugins list of strings before we return
            //
            glob_dir const plugin_filenames(p + "/*watchdog_*.so", GLOB_ERR | GLOB_NOESCAPE | GLOB_NOSORT, true);
            plugin_filenames.enumerate_glob(std::bind(&watchdog::get_plugin_names, this, std::placeholders::_1, &available_plugins));
        }
        catch(glob_dir_exception const &)
        {
            // on a developer system files are going to be in a subfolder
            // so try again just in case
            //
            try
            {
                glob_dir const plugin_filenames(p + "/watchdog_*/*watchdog_*.so", GLOB_ERR | GLOB_NOESCAPE | GLOB_NOSORT, true);
                plugin_filenames.enumerate_glob(std::bind(&watchdog::get_plugin_names, this, std::placeholders::_1, &available_plugins));
            }
            catch(glob_dir_exception const &)
            {
                // reading that list failed?!
                // (we presentthe error as a list item since we are
                // going to be added inside a list)
                //
                available_plugins << "<li><strong style=\"color: red\">An error occurred while reading the list of available plugins.</li>";
            }
        }
    }

    available_plugins.sort();

    return available_plugins.join("");
}


/** \brief Append the name of each plugin in the \p available_plugins string.
 *
 * This function is called once per watchdog plugin. It saves the filename
 * in the plugin_name string. Each name is saved between an \<li> tag so
 * it looks like a list element.
 *
 * \param[in] plugin_filename  The name of one plugin.
 */
void watchdog::get_plugin_names(QString plugin_filename, snap_string_list * available_plugins)
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
    *available_plugins << "<li>" + plugin_filename.mid(name_pos, extension_pos - name_pos) + "</li>";
}


/** \brief Generate content which is a menu entry and a page if the function.
 *
 * The generate_content() gets called to generate the content of the current
 * page. If the function=... query string is set to 'watchdog'.
 *
 * \param[in] doc  The document being worked on.
 * \param[in] output  The output where we put the data in case we generate it.
 * \param[in] menu  Menu entries.
 */
void watchdog::on_generate_content(QDomDocument doc, QDomElement root, QDomElement output, QDomElement menu, snap::snap_uri const & uri)
{
    snapdev::NOT_USED(doc, root, output);

    QString const host(uri.query_option("host"));

    if(!host.isEmpty())
    {
        // add an option to the menu so one can access that page from there
        //
        QDomElement item(doc.createElement("item"));
        item.setAttribute("href"
                        , QString("?host=%1&function=watchdog&position=latest")
                                .arg(host)
                         );
        menu.appendChild(item);
        QDomText text(doc.createTextNode("Host Watchdog"));
        item.appendChild(text);
    }

    {
        QString const function(uri.query_option("function"));
        if(function == "watchdog")
        {
            snap::snap_config snap_watchdog_conf(g_configuration_filename);
            QString const data_path(snap_watchdog_conf["data_path"]);
            QString const full_data_path(data_path + "/data");

            // the position indicates the file, if we can't load it get
            // the latest (also if position == "latest")
            //
            QString data_filename;
            QString const position(uri.query_option("position"));
            if(position != "latest")
            {
                // not the latest, check for a file with that name
                //
                data_filename = full_data_path + "/" + position;
                struct stat st;
                if(stat(data_filename.toUtf8().data(), &st) != 0)
                {
                    data_filename.clear();
                }
            }
            if(data_filename.isEmpty())
            {
                // file at 'position' not found, try the latest instead
                //
                QString const cache_path(snap_watchdog_conf["cache_path"]);
                QString const last_results_filename(cache_path + "/last_results.txt");
                struct stat st;
                if(stat(last_results_filename.toUtf8().data(), &st) == 0)
                {
                    snap::snap_config last_results_conf(last_results_filename.toUtf8().data());
                    data_filename = last_results_conf["data_path"];
                    if(stat(data_filename.toUtf8().data(), &st) != 0)
                    {
                        data_filename.clear();
                    }
                }
            }
            if(data_filename.isEmpty())
            {
                // lastest from last_results.txt failed, try again with
                // a glob(), this is much slower than getting the latest
                // from the last_results.txt file, but it still works
                //
                typedef struct stat stat_t;
                struct newest_t
                {
                    QString     f_filename = QString();
                    stat_t      f_stat = stat_t(); // not initialized if f_filename.isEmpty() is true
                };
                newest_t newest;
                auto newest_data = [&newest](QString const & filename)
                    {
                        struct stat st;
                        stat(filename.toUtf8().data(), &st);
                        if(newest.f_filename.isEmpty()
                        || st.st_mtime > newest.f_stat.st_mtime)
                        {
                            newest.f_filename = filename;
                            newest.f_stat = st;
                        }
                    };

                glob_dir d(full_data_path + "/[0-9]*.xml", GLOB_NOSORT, true);
                d.enumerate_glob(newest_data);

                // get the result of the glob()
                // it may still be empty if the pattern did not match any
                // filename
                //
                data_filename = newest.f_filename;
                struct stat st;
                if(stat(data_filename.toUtf8().data(), &st) != 0)
                {
                    data_filename.clear();
                }
            }
            QDomDocument data_doc;
            if(!data_filename.isEmpty())
            {
                QFile data_file(data_filename);
                if(!data_doc.setContent(&data_file))
                {
                    // could not read that file as XML
                    //
                    data_filename.clear();
                }
            }
            QDomElement body_tag;
            if(!data_filename.isEmpty())
            {
                // at this time we place the XSLT file in the www location
                //
                QString const snapwatchdog_data_filename(":/xsl/layout/snapwatchdog-data.xsl");
                QFile snapwatchdog_data_file(snapwatchdog_data_filename);
                if(snapwatchdog_data_file.open(QIODevice::ReadOnly))
                {
                    QByteArray data(snapwatchdog_data_file.readAll());
                    QString const snapwatchdog_data_xsl(QString::fromUtf8(data.data(), data.size()));
                    if(snapwatchdog_data_xsl.isEmpty())
                    {
                        // emit an error because to debug this otherwise is
                        // going to be complicated!
                        //
                        SNAP_LOG_ERROR("could not read the \"")
                                      (snapwatchdog_data_filename)
                                      ("\" XSLT file.");
                    }
                    else
                    {
                        xslt x;
                        x.set_xsl(snapwatchdog_data_xsl);
                        x.set_document(data_doc);
                        QDomDocument doc_page("html");
                        x.evaluate_to_document(doc_page);

                        QDomNodeList body_list(doc_page.elementsByTagName("body"));
                        if(body_list.size() == 1)
                        {
                            // we don't have to test whether it's an element,
                            // it will return a null node if so
                            //
                            body_tag = body_list.at(0).toElement();
                        }
                    }
                }
            }
            // if it all worked we have a body_tag now
            //
            if(body_tag.isNull())
            {
                QDomElement h1(doc.createElement("h1"));
                QDomText title(doc.createTextNode(QString("Snap! Watchdog (%1)").arg(host)));
                h1.appendChild(title);
                output.appendChild(h1);

                QDomElement div(doc.createElement("div"));
                div.setAttribute("id", "tabs"); // we don't actually have tabs in this case...
                div.setAttribute("class", "error");
                output.appendChild(div);

                QDomElement p(doc.createElement("p"));
                div.appendChild(p);
                QDomElement strong(doc.createElement("strong"));
                p.appendChild(strong);
                QDomText error(doc.createTextNode("ERROR:"));
                strong.appendChild(error);
                QDomText text(doc.createTextNode(" no data file could be loaded, is the snapwatchdorserver running at all?"));
                p.appendChild(text);
            }
            else
            {
                snap::snap_dom::insert_node_to_xml_doc(output, body_tag);
                //QDomText text(doc.createTextNode("Got watchdog function!"));
                //output.appendChild(text);
            }
        }
    }
}



SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
