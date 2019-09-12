// Snap Websites Server -- manage the snapbackend settings
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


// backend
//
#include "backend.h"


// our lib
//
#include "snapmanager/form.h"


// snapwebsites lib
//
#include <snapwebsites/log.h>
#include <snapwebsites/process.h>
#include <snapwebsites/qdomhelpers.h>
#include <snapwebsites/qdomxpath.h>


// snapdev lib
//
#include <snapdev/join_strings.h>
#include <snapdev/not_reached.h>
#include <snapdev/not_used.h>
#include <snapdev/string_pathinfo.h>
#include <snapdev/tokenize_string.h>


// Qt lib
//
#include <QFile>
#include <QTextStream>


// C lib
//
#include <sys/file.h>


// last include
//
#include <snapdev/poison.h>



SNAP_PLUGIN_START(backend, 1, 0)


namespace
{

char const * g_configuration_filename = "snapserver";

char const * g_configuration_d_filename = "/etc/snapwebsites/snapwebsites.d/snapserver.conf";


// TODO: offer the user a way to change the path to the executable?
struct backend_services
{
    char const *        f_service_name       = nullptr;
    char const *        f_service_executable = nullptr;
    char const *        f_recovery           = nullptr;     // default Service::RestartSec value
    char const *        f_wanted_by          = nullptr;     // static services require this with `systemctl add-wants <wanted-by> <service-name>`
    int                 f_nice               = 0;           // default nice value
};

backend_services const g_not_found;

backend_services const g_services[5] =
{
    {
        /* f_service_name       */ "snapbackend",
        /* f_service_executable */ "/usr/sbin/snapbackend",
        /* f_recovery           */ nullptr,
        /* f_wanted_by          */ nullptr,
        /* f_nice               */ 5
    },
    {
        /* f_service_name       */ "snapimages",
        /* f_service_executable */ "/usr/sbin/snapbackend",
        /* f_recovery           */ "1h",
        /* f_wanted_by          */ "multi-user.target",
        /* f_nice               */ 10
    },
    {
        /* f_service_name       */ "snaplistjournal",
        /* f_service_executable */ "/usr/sbin/snapbackend",
        /* f_recovery           */ "5min",
        /* f_wanted_by          */ nullptr,
        /* f_nice               */ 3
    },
    {
        /* f_service_name       */ "snappagelist",
        /* f_service_executable */ "/usr/sbin/snapbackend",
        /* f_recovery           */ "5min",
        /* f_wanted_by          */ "multi-user.target",
        /* f_nice               */ 3
    },
    {
        /* f_service_name       */ "snapsendmail",
        /* f_service_executable */ "/usr/sbin/snapbackend",
        /* f_recovery           */ "1h",
        /* f_wanted_by          */ "multi-user.target",
        /* f_nice               */ 7
    }
};


backend_services const & get_service_by_name( QString const & service_name )
{
    for(auto const & service_info : g_services)
    {
        if( service_info.f_service_name == service_name )
        {
            return service_info;
        }
    }

    return g_not_found;
}




}
// unnamed namespace



/** \brief Get a fixed backend plugin name.
 *
 * The backend plugin makes use of different fixed names. This function
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
    case name_t::SNAP_NAME_SNAPMANAGERCGI_BACKEND_NAME:
        return "name";

    default:
        // invalid index
        throw snap_logic_exception("Invalid SNAP_NAME_SNAPMANAGERCGI_BACKEND_...");

    }
    NOTREACHED();
}




/** \brief Initialize the backend plugin.
 *
 * This function is used to initialize the backend plugin object.
 */
backend::backend()
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Clean up the backend plugin.
 *
 * Ensure the backend object is clean before it is gone.
 */
backend::~backend()
{
}


/** \brief Get a pointer to the backend plugin.
 *
 * This function returns an instance pointer to the backend plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the backend plugin.
 */
backend * backend::instance()
{
    return g_plugin_backend_factory.instance();
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
QString backend::description() const
{
    return "Manage the snapbackend settings.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString backend::dependencies() const
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
int64_t backend::do_update(int64_t last_updated)
{
    NOTUSED(last_updated);

    SNAP_PLUGIN_UPDATE_INIT();
    // no updating in snapmanager*
    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Initialize backend.
 *
 * This function terminates the initialization of the backend plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void backend::bootstrap(snap_child * snap)
{
    f_snap = dynamic_cast<snap_manager::manager *>(snap);
    if(f_snap == nullptr)
    {
        throw snap_logic_exception("snap pointer does not represent a valid manager object.");
    }

    SNAP_LISTEN  ( backend, "server", snap_manager::manager, retrieve_status,        _1     );
    SNAP_LISTEN  ( backend, "server", snap_manager::manager, add_plugin_commands,    _1     );
    SNAP_LISTEN  ( backend, "server", snap_manager::manager, process_plugin_message, _1, _2 );
    SNAP_LISTEN0 ( backend, "server", snap_manager::manager, communication_ready            );
}


/** \brief Determine this plugin status data.
 *
 * This function builds a tree of statuses.
 *
 * \param[in] server_status  The map of statuses.
 */
void backend::on_retrieve_status(snap_manager::server_status & server_status)
{
    if(f_snap->stop_now_prima())
    {
        return;
    }

    snap_config snap_server_conf(g_configuration_filename);

    // Add the enable/disable ALL backends pulldown:
    //

    {
        QString const status(snap_server_conf["backend_status"]);
        SNAP_LOG_DEBUG("all_services -- backend_status=")(status);
        snap_manager::status_t const all_status_widget(
                    status == "enabled"
                        ? snap_manager::status_t::state_t::STATUS_STATE_INFO
                        : snap_manager::status_t::state_t::STATUS_STATE_WARNING,
                    get_plugin_name(),
                    "all_services",
                    status);
        server_status.set_field(all_status_widget);
    }

    QString const backends(snap_server_conf["backends"]);

    {
        snap_manager::status_t const backends_widget(
                    snap_manager::status_t::state_t::STATUS_STATE_INFO,
                    get_plugin_name(),
                    "backends",
                    backends);
        server_status.set_field(backends_widget);
    }

    snap_string_list const enabled_backends(backends.split(','));

    for(auto const & service_info : g_services)
    {
        // get the backend service status
        //
        snap_manager::service_status_t status(f_snap->service_status(
                        service_info.f_service_executable,
                        std::string(service_info.f_service_name) + (service_info.f_recovery != nullptr ? "" : ".timer")));

        // transform to a string
        //
        QString const status_string(QString::fromUtf8(snap_manager::manager::service_status_to_string(status)));

        // create status widget
        //
        // if the state is DISABLED and the service is NOT part of
        // enabled_backends, then it is normal (INFO)
        //
        // if the state is DISABLED and the service IS part of
        // enabled_backends, then it is not working right (WARNING)
        //
        // if the state is ACTIVE and the service is part of
        // enabled_backends, then it is normal (INFO)
        //
        // anything else is an error
        //
        snap_manager::status_t::state_t state(snap_manager::status_t::state_t::STATUS_STATE_HIGHLIGHT);
        if(status == snap_manager::service_status_t::SERVICE_STATUS_DISABLED)
        {
            if(enabled_backends.contains(service_info.f_service_name))
            {
                // it is supposed to be running, this is a warning already
                //
                state = snap_manager::status_t::state_t::STATUS_STATE_WARNING;
            }
            else
            {
                // this is normal
                //
                state = snap_manager::status_t::state_t::STATUS_STATE_INFO;
            }
        }
        else if(status == snap_manager::service_status_t::SERVICE_STATUS_ACTIVE
             && enabled_backends.contains(service_info.f_service_name))
        {
            state = snap_manager::status_t::state_t::STATUS_STATE_INFO;
        }

        snap_manager::status_t const status_widget(
                        state,
                        // the status can be caculated depending on whether
                        // the service is expected to be running or not;
                        // keeping the old code for a little while until
                        // the new code works 100%
                        //
                        //status == snap_manager::service_status_t::SERVICE_STATUS_NOT_INSTALLED
                        //        ? snap_manager::status_t::state_t::STATUS_STATE_ERROR
                        //        : status == snap_manager::service_status_t::SERVICE_STATUS_DISABLED
                        //                ? snap_manager::status_t::state_t::STATUS_STATE_HIGHLIGHT
                        //                : snap_manager::status_t::state_t::STATUS_STATE_INFO,
                        get_plugin_name(),
                        QString("%1::service_status").arg(service_info.f_service_name),
                        status_string);
        server_status.set_field(status_widget);

        if(status != snap_manager::service_status_t::SERVICE_STATUS_NOT_INSTALLED)
        {
            snap_config service_config(std::string("/lib/systemd/system/") + service_info.f_service_name + ".service"
                                     , std::string("/etc/systemd/system/") + service_info.f_service_name + ".service.d/override.conf");
            snap_manager::status_t const nice(snap_manager::status_t::state_t::STATUS_STATE_INFO,
                                      get_plugin_name(),
                                      QString("%1::nice").arg(service_info.f_service_name),
                                      service_config["Service::Nice"]);
            server_status.set_field(nice);

            if(service_info.f_recovery != nullptr)
            {
                snap_manager::status_t const recovery(snap_manager::status_t::state_t::STATUS_STATE_INFO,
                                          get_plugin_name(),
                                          QString("%1::recovery").arg(service_info.f_service_name),
                                          service_config["Service::RestartSec"]);
                server_status.set_field(recovery);
            }
            else
            {
                // for the delay between runs of the snapbackend as a CRON
                // service the delay is in the .timer file instead
                //
                snap_config timer_config(std::string("/lib/systemd/system/") + service_info.f_service_name + ".timer"
                                       , std::string("/etc/systemd/system/") + service_info.f_service_name + ".timer.d/override.conf");
                snap_manager::status_t const cron(snap_manager::status_t::state_t::STATUS_STATE_INFO,
                                          get_plugin_name(),
                                          QString("%1::cron").arg(service_info.f_service_name),
                                          timer_config["Timer::OnUnitActiveSec"]);
                server_status.set_field(cron);
            }
        }
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
bool backend::display_value(QDomElement parent, snap_manager::status_t const & s, snap::snap_uri const & uri)
{
    QDomDocument doc(parent.ownerDocument());

    if(s.get_field_name() == "all_services")
    {
        SNAP_LOG_DEBUG("backend::display_value(): all_services! s.get_value()=")(s.get_value());
        snap_manager::form f(
                  get_plugin_name()
                , s.get_field_name()
                ,   snap_manager::form::FORM_BUTTON_RESET
                  | snap_manager::form::FORM_BUTTON_SAVE_EVERYWHERE
                );

        snap::snap_string_list service_list;
        service_list << "disabled";
        service_list << "enabled";

        snap_manager::widget_select::pointer_t field(std::make_shared<snap_manager::widget_select>(
                      "Enable or disable ALL backend services"
                    , s.get_field_name()
                    , service_list
                    , s.get_value()
                    , "<p>Enable or disable all backend services either on this system or cluster-wide.</p>"
                      "<p>Hit <strong>Save Everywhere</strong> to change or re-iterate the state."
                      " There is no legitimate way to only enable or disable the backend services"
                      " on just one computer. This feature is always run cluster wide.</p>"
                      "<p>Note that you can click the <strong>Save Everywhere</strong> button without"
                      " changing the status. The system will force the state again and make sure it is"
                      " enabled or disabled on all computers. This feature can be useful after an upgrade"
                      " since all the backends may not get restarted properly after such.</p>"
                    ));
        f.add_widget(field);
        f.generate(parent, uri);
        return true;
    }

    if(s.get_field_name() == "backends")
    {
        // list of enabled backends
        //
        snap_manager::form f(
                  get_plugin_name()
                , s.get_field_name()
                ,   snap_manager::form::FORM_BUTTON_RESET
                  | snap_manager::form::FORM_BUTTON_SAVE
                  | snap_manager::form::FORM_BUTTON_RESTORE_DEFAULT
                );

        QString available_services;
        for(auto const & service_info : g_services)
        {
            available_services += "<li>" + QString::fromUtf8(service_info.f_service_name) + "</li>";
        }

        QString const backend_names(s.get_value());
        snap_string_list backend_names_list(backend_names.split(","));
        backend_names_list.sort();
        QString const backend_names_lined(backend_names_list.join('\n'));

        snap_manager::widget_text::pointer_t field(std::make_shared<snap_manager::widget_text>(
                    "Select backend services to run on this system"
                    , s.get_field_name()
                    , backend_names_lined
                    , "<p>Select the exact list of backends to run on this system."
                      " This information is saved in the snapserver.conf file."
                      " It is used by the main On/Off switch (the all_services flag.)</p>"
                      "<p>The following is a list of available backends:</p>"
                      "<ul>"
                    + available_services
                    + "</ul>"
                      "<p>If a service is not active and the \"all_services\" flag is"
                      " \"enabled\", then you can try to click \"Save\" here to give"
                      " that service a nudge. The system will again go through the loop"
                      " trying to start/stop each service as required. You do not have"
                      " to make changes to the list in this situation.</p>"
                    ));
        f.add_widget(field);
        f.generate(parent, uri);
        return true;
    }

    // **************** WARNING ******************
    // the second part of this display_value() function only handles
    // service fields where the name has to be "<service>::<field>"
    // anything else has to be placed before this comment
    // **************** WARNING ******************

    int const pos(s.get_field_name().indexOf("::"));
    if(pos <= 0)
    {
        return false;
    }

    QString const service_name(s.get_field_name().mid(0, pos));
    QString const field_name(s.get_field_name().mid(pos + 2));

    if(service_name.isEmpty()
    || field_name.isEmpty())
    {
        return false;
    }

    if(field_name == "service_status")
    {
        auto const & service_info( get_service_by_name(service_name) );
        if( service_info.f_service_name == nullptr )
        {
            SNAP_LOG_WARNING("Service [")(service_name)("] not found!");
            return false;
        }

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
                            , "<p>This plugin should not be able to detect that the service in question is"
                             " uninstalled since the plugin is part of that service and thus it should"
                             " disappear along the main binary... Please report this bug.</p>"
                            ));
            f.add_widget(field);
            f.generate(parent, uri);
        }
        else
        {
            snap_manager::form f(
                      get_plugin_name()
                    , s.get_field_name()
                    ,   snap_manager::form::FORM_BUTTON_NONE
                    );

            // the old code would allow us to change the status of each
            // individual backend, which would often create a mess in the
            // sense that some backends would then get lost and the system
            // (specificall, snapwatchdog) would not know whether a backend
            // is expected to be running or not

            //QStringList service_list;
            //service_list << "disabled";
            //service_list << "enabled";
            //service_list << "active";
            //service_list << "failed";   // Not sure about this...

            //snap_manager::widget_select::pointer_t field(std::make_shared<snap_manager::widget_select>(
            //                  QString("Enabled/Disabled/Activate %1").arg(service_info.f_service_name)
            //                , s.get_field_name()
            //                , service_list
            //                , s.get_value()
            //                , QString("<p>Enter the new state of the %1"
            //                  " service as one of:</p>"
            //                  "<ul>"
            //                  "  <li>disabled -- deactivate and disable the service</li>"
            //                  "  <li>enabled -- enable the service, deactivate if it was activated</li>"
            //                  "  <li>active -- enable and activate the service</li>"
            //                  "</ul>"
            //                  "<p>You cannot request to go to the \"failed\" status. It appears in the list for status purposes only."
            //                  " To uninstall search for the corresponding bundle and"
            //                  " click the <strong>Uninstall</strong> button.</p>")
            //                        .arg(service_info.f_service_name)
            //                ));

            // in the new version we still want to display the current status
            // of the backend by not allow the user to change it here, instead
            // they want to add/remove them from the "backends" field
            //
            snap_manager::widget_description::pointer_t field(std::make_shared<snap_manager::widget_description>(
                              "Service Current Status"
                            , s.get_field_name()
                            , "<p>This service is currently <strong>" + s.get_value() + "</strong></p>"
                              "<p>A service is \"disabled\" when either one of the following is true:</p>"
                              "<ol>"
                                "<li>\"all_services\" is currently disabled</li>"
                                "<li>the service does not appear in the \"backends\" list of enabled services</li>"
                              "</ol>"
                              "<p>When the \"all_services\" is currently disabled, this probably means you are"
                              " working on upgrading your system. Changing that setting to \"Enabled\" will"
                              " restore all the backend services to their normal \"active\" status as expected.</p>"
                              "<p>When the \"backends\" list does not include the name of this backend,"
                              " it is consided disabled and won't run whether \"all_services\" is enabled or not.</p>"
                              "<p>By editing either one of those two other fields, the service status will"
                              " change accordingly as you save the new value.</p>"
                              "<p>Notice that the \"all_services\" can only be \"Saved Everywhere\" since"
                              " it really only makes sense to turn off backends on all computers, not just"
                              " one of them.</p>"
                            ));
            f.add_widget(field);
            f.generate(parent, uri);
        }

        return true;
    }

    if(field_name == "recovery")
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
                  | snap_manager::form::FORM_BUTTON_RESTORE_DEFAULT
                );

        snap_manager::widget_input::pointer_t field(std::make_shared<snap_manager::widget_input>(
                          QString("Recovery Delay for %1 Backend").arg(service_name)
                        , s.get_field_name()
                        , s.get_value()
                        , QString("Delay before restarting %1 if it fails to restart immediately after a crash. This number is in seconds.").arg(service_name)
                        ));
        f.add_widget(field);
        f.generate(parent, uri);
        return true;
    }

    if(field_name == "cron")
    {
        // the list if frontend snapmanagers that are to receive statuses
        // of the cluster computers; may be just one computer; should not
        // be empty; shows a text input field
        //
        snap_manager::form f(
                  get_plugin_name()
                , s.get_field_name()
                ,   snap_manager::form::FORM_BUTTON_RESET
                  | snap_manager::form::FORM_BUTTON_SAVE_EVERYWHERE
                  | snap_manager::form::FORM_BUTTON_SAVE
                  | snap_manager::form::FORM_BUTTON_RESTORE_DEFAULT
                );

        snap_manager::widget_input::pointer_t field(std::make_shared<snap_manager::widget_input>(
                          QString("CRON Delay between runs of %1").arg(service_name)
                        , s.get_field_name()
                        , s.get_value()
                        , QString("The delay, in seconds, between each run of the %1 backend process."
                                 " At this time, this is the amount of time between runs."
                                 " If a run takes 10min and this delay is 5min, then the snapbackend will run once every 15min. or so."
                                 " The value can be followed by 'ms' for milliseconds,"
                                 " 's' for seconds, 'min' for minutes,"
                                 " combos work too: 5min 30s. For more, see"
                                 " <a href=\"https://www.freedesktop.org/software/systemd/man/systemd.time.html\">systemd.time</a>")
                                        .arg(service_name)
                        ));
        f.add_widget(field);
        f.generate(parent, uri);
        return true;
    }

    if(field_name == "nice")
    {
        // the list if frontend snapmanagers that are to receive statuses
        // of the cluster computers; may be just one computer; should not
        // be empty; shows a text input field
        //
        snap_manager::form f(
                  get_plugin_name()
                , s.get_field_name()
                ,   snap_manager::form::FORM_BUTTON_RESET
                  | snap_manager::form::FORM_BUTTON_SAVE_EVERYWHERE
                  | snap_manager::form::FORM_BUTTON_SAVE
                  | snap_manager::form::FORM_BUTTON_RESTORE_DEFAULT
                );

        snap_manager::widget_input::pointer_t field(std::make_shared<snap_manager::widget_input>(
                          QString("Nice value for %1").arg(service_name)
                        , s.get_field_name()
                        , s.get_value()
                        , "The nice value is the same as the nice command line"
                         " Unix utility, here we accept a value from 0 to 19."
                         " It changes the priority of the process."
                         " The larger the value, the weaker the priority of that"
                         " process (it will yield to processes with a smaller"
                         " nice value.)"
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
bool backend::apply_setting( QString const     & button_name
                           , QString const     & field_name
                           , QString const     & new_value
                           , QString const     & old_or_installation_value
                           , std::set<QString> & affected_services
                           )
{
    NOTUSED(old_or_installation_value);
    NOTUSED(affected_services);

    // restore defaults?
    //
    bool const use_default_value(button_name == "restore_default");

    if(field_name == "all_services")
    {
        // We don't support the "Restore Default" here because we need
        // the "Save Everywhere" to work right
        //QString const value(use_default_value ? "enabled" : new_value);
        SNAP_LOG_DEBUG("all_services = ")(new_value);

        snap_config snap_server_conf(g_configuration_filename);
        snap_server_conf["backend_status"] = new_value;
        NOTUSED(f_snap->replace_configuration_value(g_configuration_d_filename, "backend_status", new_value));

        update_all_services();
        return true;
    }

    if(field_name == "backends")
    {
        snap_string_list names;
        if(use_default_value)
        {
            names << "snaplistjournal";
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
                auto const & service(get_service_by_name(t));
                if(service.f_service_name == nullptr)
                {
                    // probably mispelled, it would break the load so don't
                    // allow it in the .conf file
                    //
                    SNAP_LOG_ERROR("Could not find backend named \"")
                                  (t)
                                  ("\" in the list of available backends. Please try again.");
                    return true;
                }

                clean_names << t;
            }
        }
        QString const new_list_of_plugins(clean_names.join(','));
        snap_config snap_server_conf(g_configuration_filename);
        snap_server_conf["backends"] = new_list_of_plugins;

        f_snap->replace_configuration_value(g_configuration_d_filename, "backends", new_list_of_plugins);

        update_all_services();
        return true;
    }

    int const pos(field_name.indexOf("::"));
    if(pos <= 0)
    {
        return false;
    }

    QString const service_name(field_name.mid(0, pos));
    QString const field(field_name.mid(pos + 2));

    if(service_name.isEmpty()
    || field.isEmpty())
    {
        return false;
    }

    // determine executable using the list of supported backend services
    //
    auto const & service_info(get_service_by_name( service_name ));
    if( service_info.f_service_name == nullptr )
    {
        return false;
    }

    SNAP_LOG_WARNING("Got field \"")(field)("\" to change for \"")(service_name)("\" executable = [")(service_info.f_service_executable)("].");

    if(field == "service_status")
    {
        // this is now ignored, use the "all_services" and "backends"
        // fields instead (there is a "service_status" entry, but it's
        // now just a description with the status of the service

        //QString unit_name(service_name);
        //if(service_info.f_recovery == nullptr)
        //{
        //    unit_name += ".timer";
        //}
        //auto const status(snap_manager::manager::string_to_service_status(new_value.toUtf8().data()));
        //if( change_service_status( service_info.f_service_executable, unit_name, status ) )
        //{
        //    send_status();
        //}

        // Save new configuration in file
        //
        //status_file sf;
        //QString const status_string( QString::fromUtf8(snap_manager::manager::service_status_to_string(status)) );
        //sf[unit_name] = status_string;
        //SNAP_LOG_DEBUG("++++ Writing status_file from apply_setting()!");
        //sf.save();

        return true;
    }

    // TODO: the following works just fine at this time, but it is not very
    //       safe:
    //         1. we should use a snap_process to get errors logged automatically
    //         2. we should have a way to change a variable within a [section]
    //
    if(field == "recovery")
    {
        QString const filename(QString("/etc/systemd/system/%1.service.d/override.conf").arg(service_name));
        QString const value(use_default_value ? service_info.f_recovery : new_value);
        if(f_snap->replace_configuration_value(filename, "Service::RestartSec", value, snap_manager::REPLACE_CONFIGURATION_VALUE_SECTION))
        {
            // make sure the cache gets updated
            //
            snap_config service_config(std::string("/lib/systemd/system/") + service_name + ".service"
                                     , std::string("/etc/systemd/system/") + service_name + ".service.d/override.conf");
            service_config["Service::RestartSec"] = value;
        }
        snap::process p("reload daemon");
        p.set_mode(snap::process::mode_t::PROCESS_MODE_COMMAND);
        p.set_command("systemctl");
        p.add_argument("daemon-reload");
        NOTUSED(p.run());
        return true;
    }

    if(field == "cron")
    {
        QString const filename(QString("/etc/systemd/system/%1.timer.d/override.conf").arg(service_name));
        QString const value(use_default_value ? "5min" : new_value);
        if(f_snap->replace_configuration_value(filename, "Timer::OnUnitActiveSec", value, snap_manager::REPLACE_CONFIGURATION_VALUE_SECTION | snap_manager::REPLACE_CONFIGURATION_VALUE_RESET_TIMER))
        {
            // make sure the cache gets updated
            //
            snap_config service_config(std::string("/lib/systemd/system/") + service_name + ".timer"
                                     , std::string("/etc/systemd/system/") + service_name + ".timer.d/override.conf");
            service_config["Timer::OnUnitActiveSec"] = value;
        }
        snap::process p("reload daemon");
        p.set_mode(snap::process::mode_t::PROCESS_MODE_COMMAND);
        p.set_command("systemctl");
        p.add_argument("daemon-reload");
        NOTUSED(p.run());
        return true;
    }

    if(field == "nice")
    {
        // verify that the value is sensible as a nice value
        //
        bool ok(false);
        int const nice(new_value.toInt(&ok, 10));
        if(!ok
        || nice < 0
        || nice > 19)
        {
            // TODO: report error to admin. in snapmanager.cgi
            //
            SNAP_LOG_ERROR("The nicevalue is limited to a number between 0 and 19. \"")
                          (new_value)
                          ("\" is not acceptable. Please try again.");
            return true;
        }

        QString const filename(QString("/etc/systemd/system/%1.service.d/override.conf").arg(service_name));
        QString value(use_default_value
                    ? QString("%1").arg(service_info.f_nice)
                    : new_value);
        if(f_snap->replace_configuration_value(filename, "Service::Nice", value, snap_manager::REPLACE_CONFIGURATION_VALUE_SECTION))
        {
            // make sure the cache gets updated
            //
            snap_config service_config(std::string("/lib/systemd/system/") + service_name + ".service"
                                     , std::string("/etc/systemd/system/") + service_name + ".service.d/override.conf");
            service_config["Service::Nice"] = value;
        }
        snap::process p("reload daemon");
        p.set_mode(snap::process::mode_t::PROCESS_MODE_COMMAND);
        p.set_command("systemctl");
        p.add_argument("daemon-reload");
        NOTUSED(p.run());
        return true;
    }

    return false;
}



bool backend::change_service_status( QString const & exe_path
                                   , QString const & unit_name
                                   , snap_manager::service_status_t status
                                   , char const * wanted_by )
{
    bool changed(false);
    if( f_snap->service_status( exe_path.toUtf8().data(), unit_name.toUtf8().data() ) != status )
    {
        f_snap->service_apply_status( unit_name.toUtf8().data()
                                    , status
                                    , std::string(wanted_by == nullptr ? "" : wanted_by) );
        changed = true;
    }

    return changed;
}


void backend::update_all_services()
{
    snap_config snap_server_conf(g_configuration_filename);
    bool const disable_all(snap_server_conf["backend_status"] == "disabled");
    QString const backends(snap_server_conf["backends"]);
    snap_string_list const enabled_backends(backends.split(','));
    int const max(enabled_backends.size());
    std::map<QString, bool> enabled_backends_map;
    for(int idx(0); idx < max; ++idx)
    {
        enabled_backends_map[enabled_backends[idx].trimmed()] = true;
    }

    SNAP_LOG_DEBUG("generate status: ")(snap_server_conf["backend_status"]);

    for(auto const & service_info : g_services)
    {
        // get the backend service status
        //
        // we currently limit the status to 2 values:
        //   . disabled -- backend is not used
        //   . active -- the backend is used as expected
        //
        // another possible status is "enabled" which we don't need here
        // (and "not installed" which doesn't apply here at all.)
        //
        snap_manager::service_status_t to_status(snap_manager::service_status_t::SERVICE_STATUS_DISABLED);

        // is the main flag enabled, if not keep DISABLED as the to_status
        //
        if(!disable_all)
        {
            // is that backend enabled? (a.k.a. "active" in terms of systemd)
            //
            if(enabled_backends_map.find(service_info.f_service_name) != enabled_backends_map.end())
            {
                to_status = snap_manager::service_status_t::SERVICE_STATUS_ACTIVE;
            }
        }

        // now update the status at the system level
        //
        QString const unit_name(QString("%1%2")
                            .arg(service_info.f_service_name)
                            .arg(service_info.f_recovery != nullptr ? "" : ".timer"));
        change_service_status(service_info.f_service_executable, unit_name, to_status, service_info.f_wanted_by);
    }

    send_status();
}


void backend::send_status( snap::snap_communicator_message const* message )
{
    snap::snap_communicator_message cmd;
    cmd.set_command("BACKENDSTATUS");
    //
    if( message == nullptr )
    {
        cmd.set_service("*");
    }
    else
    {
        cmd.reply_to(*message);
    }

    for(auto const & service_info : g_services)
    {
        // get the backend service status
        //
        snap_manager::service_status_t status(f_snap->service_status(
                        service_info.f_service_executable,
                        std::string(service_info.f_service_name) + (service_info.f_recovery != nullptr ? "" : ".timer")));
        QString const status_string(QString::fromUtf8(snap_manager::manager::service_status_to_string(status)));
        cmd.add_parameter( QString("backend_%1").arg(service_info.f_service_name), status_string );
    }

    //cmd.add_parameter( "cache" , "ttl=60"     ); TODO?
    f_snap->forward_message(cmd);

    SNAP_LOG_DEBUG("BACKENDSTATUS message sent!");
}


void backend::on_communication_ready()
{
    send_status();
}


void backend::on_add_plugin_commands(snap::snap_string_list & understood_commands)
{
    understood_commands << "BACKENDSTATUS_REQUEST";
}


void backend::on_process_plugin_message(snap::snap_communicator_message const & message, bool & processed)
{
    QString const command(message.get_command());
    SNAP_LOG_TRACE("backend::on_process_plugin_message(), command=[")(command)("]");

    if(command == "BACKENDSTATUS_REQUEST")
    {
        send_status( &message );
        processed = true;
    }
}





SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
