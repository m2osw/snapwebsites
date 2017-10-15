// Snap Websites Server -- manage the snapbackend settings
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

// backend
//
#include "backend.h"

// our lib
//
#include "snapmanager/form.h"

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
#include <QTextStream>

// C lib
//
#include <sys/file.h>

// last entry
//
#include <snapwebsites/poison.h>


SNAP_PLUGIN_START(backend, 1, 0)


namespace
{

// TODO: offer the user a way to change this path?
struct backend_services
{
    char const *        f_service_name       = nullptr;
    char const *        f_service_executable = nullptr;
    bool                f_recovery           = true;
    int                 f_nice               = 0;
};

backend_services g_services[5] = {
        { "snapbackend",        "/usr/bin/snapbackend", false,  5 },
        { "snapimages",         "/usr/bin/snapbackend", true,  10 },
        { "snaplistjournal",    "/usr/bin/snapbackend", true,   3 },
        { "snappagelist",       "/usr/bin/snapbackend", true,   3 },
        { "snapsendmail",       "/usr/bin/snapbackend", true,   7 }
    };

backend_services get_service_by_name( QString const & service_name )
{
    for(auto const & service_info : g_services)
    {
        if( service_info.f_service_name == service_name )
        {
            return service_info;
        }
    }

    return backend_services();
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


class status_file
{
public:
    status_file()
    {
        load_file();
    }

    QString &       operator[]( QString const& name )       { return f_map[name];    }
    QString const & operator[]( QString const& name ) const { return f_map.at(name); }

    bool is_empty() const { return f_map.empty(); }
    void clear()          { f_map.clear();        }
    void save()           { save_file();          }

private:
    std::map<QString,QString>   f_map;
    QString const               f_filename = "/var/cache/snapwebsites/backend-status.txt";

    void load_file()
    {
        QFile the_file( f_filename );
        if( !the_file.exists() )
        {
            // Doesn't yet exist, so don't load anything
            return;
        }

        if( !the_file.open(QFile::ReadOnly) )
        {
            SNAP_LOG_ERROR("Cannot read backend status file!");
            return;
        }

        QTextStream in(&the_file);
        QString line;
        while( in.readLineInto(&line) )
        {
            line = line.trimmed();
            if( line[0] == '#' ) continue; // ignore comments
            QStringList values( line.split('=') );
            f_map[values[0]] = values[1];
        }
    }

    void save_file()
    {
        QFile the_file( f_filename );
        if( !the_file.open(QFile::WriteOnly | QFile::Truncate) )
        {
            SNAP_LOG_ERROR("Cannot open backend status file for write!");
            return;
        }

        QTextStream out(&the_file);
        out << "# Auto-generated file by `snapbackend`.\n";
        for( auto const& pair : f_map )
        {
            SNAP_LOG_DEBUG("outputting first=")(pair.first)(", second=")(pair.second);
            out << pair.first << "=" << pair.second << "\n";
        }

        the_file.flush();
    }
};


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

    // Add the enable/disable ALL backends pulldown:
    //
    {
        status_file sf;
        if( !sf.is_empty() )
        {
            SNAP_LOG_DEBUG("backend::on_retrieve_status(): snapbackend::all_services, sf[\"disabled\"]=")(sf["disabled"]);
            bool const disabled_mode(sf["disabled"]=="true");
            snap_manager::status_t const all_status_widget(
                        disabled_mode
                            ? snap_manager::status_t::state_t::STATUS_STATE_WARNING
                            : snap_manager::status_t::state_t::STATUS_STATE_INFO,
                        get_plugin_name(),
                        "snapbackend::all_services",
                        disabled_mode? "disabled" : "enabled");
            server_status.set_field(all_status_widget);
        }
    }

    // TODO: add support so the user can edit the recovery delay, the nice
    //       value and in case of CRON, the timer ticking
    //

    // TODO: replace with new snapfirewall implementation
    //
    for(auto const & service_info : g_services)
    {
        // get the backend service status
        //
        snap_manager::service_status_t status(f_snap->service_status(
                        service_info.f_service_executable,
                        std::string(service_info.f_service_name) + (service_info.f_recovery ? "" : ".timer")));

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

            if(service_info.f_recovery)
            {
                snap_manager::status_t const recovery(snap_manager::status_t::state_t::STATUS_STATE_INFO,
                                          get_plugin_name(),
                                          QString("%1::recovery").arg(service_info.f_service_name),
                                          service_config["Service::RestartSec"]);
                server_status.set_field(recovery);
            }
            else
            {
                // for the delay between runs of the snapbackend as a CRON service
                // the delay is in the .timer file instead
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


bool backend::change_service_status( QString const& exe_path, QString const& unit_name, snap_manager::service_status_t status )
{
    bool changed(false);
    if( f_snap->service_status( exe_path.toUtf8().data(), unit_name.toUtf8().data() ) != status )
    {
        f_snap->service_apply_status( unit_name.toUtf8().data(), status );
        changed = true;
    }

    return changed;
}


void backend::update_all_services( QString const & new_value )
{
    SNAP_LOG_DEBUG("backend::update_all_services(), new_value=")(new_value);
    status_file sf;

    for(auto const & service_info : g_services)
    {
        // get the backend service status
        //
        QString const unit_name(QString("%1%2").arg(service_info.f_service_name).arg(service_info.f_recovery ? "" : ".timer"));
        if( sf[unit_name].isEmpty() )
        {
            sf[unit_name] = "disabled";
        }

        auto const to_status(
            (new_value == "disabled")
              ? snap_manager::service_status_t::SERVICE_STATUS_DISABLED
              : snap_manager::manager::string_to_service_status(sf[unit_name].toUtf8().data())
            );
        change_service_status( service_info.f_service_executable, unit_name, to_status );
    }

    if( new_value == "disabled" )
    {
        sf["disabled"] = "true";
    }
    else
    {
        sf["disabled"] = "false";
    }

    sf.save();

    send_status();
}


void backend::send_update( QString const& new_value )
{
    snap::snap_communicator_message cmd;
    cmd.set_command("BACKENDALLSERVICES");
    cmd.set_service("*");
    //cmd.add_parameter( "cache"    , "ttl=60"  ); TODO?
    cmd.add_parameter( "disabled" , new_value );
    f_snap->forward_message(cmd);

    SNAP_LOG_DEBUG("BACKENDALLSERVICES message sent, new_value=")(new_value);
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
                        std::string(service_info.f_service_name) + (service_info.f_recovery ? "" : ".timer")));
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
    understood_commands << "BACKENDALLSERVICES";
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
    else if( command == "BACKENDALLSERVICES" )
    {
        SNAP_LOG_DEBUG("BACKENDALLSERVICES recieved!");
        update_all_services( message.get_parameter( "disabled" ) );
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

    int const pos(s.get_field_name().indexOf("::"));
    if(pos <= 0)
    {
        return false;
    }

    if(s.get_field_name() == "snapbackend::all_services")
    {
        SNAP_LOG_DEBUG("backend::display_value(): snapbackend::all_services! s.get_value()=")(s.get_value());
        snap_manager::form f(
                get_plugin_name()
                , s.get_field_name()
                , snap_manager::form::FORM_BUTTON_RESET | snap_manager::form::FORM_BUTTON_SAVE | snap_manager::form::FORM_BUTTON_SAVE_EVERYWHERE
                );

        QStringList service_list;
        service_list << "disabled";
        service_list << "enabled";

        snap_manager::widget_select::pointer_t field(std::make_shared<snap_manager::widget_select>(
                    "Enable or disable ALL backend services"
                    , s.get_field_name()
                    , service_list
                    , s.get_value()
                    , "<p>Enable or disable all backend services either on this system or cluster-wide.</p>"
                        "<p>Use the 'SAVE' button for the local system only, or hit 'SAVE EVERYWHERE' for cluster-wide effect."
                        " When you disable the current configuration, it will be remembered on enable-all.</p>"
                    ));
        f.add_widget(field);

        f.generate(parent, uri);

        return true;
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
            service_list << "failed";   // Not sure about this...

            snap_manager::widget_select::pointer_t field(std::make_shared<snap_manager::widget_select>(
                              QString("Enabled/Disabled/Activate %1").arg(service_info.f_service_name)
                            , s.get_field_name()
                            , service_list
                            , s.get_value()
                            , QString("<p>Enter the new state of the %1"
                              " service as one of:</p>"
                              "<ul>"
                              "  <li>disabled -- deactivate and disable the service</li>"
                              "  <li>enabled -- enable the service, deactivate if it was activated</li>"
                              "  <li>active -- enable and activate the service</li>"
                              "</ul>"
                              "<p>You cannot request to go to the \"failed\" status. It appears in the list for status purposes only."
                              " To uninstall search for the corresponding bundle and"
                              " click the <strong>Uninstall</strong> button.</p>")
                                    .arg(service_info.f_service_name)
                            ));
            f.add_widget(field);

            f.generate(parent, uri);
        }

        return true;
    }

    //if(field_name == "disabled")
    //{
    //    // the list if frontend snapmanagers that are to receive statuses
    //    // of the cluster computers; may be just one computer; should not
    //    // be empty; shows a text input field
    //    //
    //    snap_manager::form f(
    //              get_plugin_name()
    //            , s.get_field_name()
    //            , snap_manager::form::FORM_BUTTON_RESET | snap_manager::form::FORM_BUTTON_SAVE_EVERYWHERE | snap_manager::form::FORM_BUTTON_SAVE | snap_manager::form::FORM_BUTTON_RESTORE_DEFAULT
    //            );

    //    snap_manager::widget_input::pointer_t field(std::make_shared<snap_manager::widget_input>(
    //                      QString("Enable/Disable %1 Backend").arg(service_name)
    //                    , s.get_field_name()
    //                    , s.get_value()
    //                    , QString("Define whether the %1 backend is \"enabled\" or \"disabled\".").arg(service_name)
    //                    ));
    //    f.add_widget(field);

    //    f.generate(parent, uri);

    //    return true;
    //}

    if(field_name == "recovery")
    {
        // the list if frontend snapmanagers that are to receive statuses
        // of the cluster computers; may be just one computer; should not
        // be empty; shows a text input field
        //
        snap_manager::form f(
                  get_plugin_name()
                , s.get_field_name()
                , snap_manager::form::FORM_BUTTON_RESET | snap_manager::form::FORM_BUTTON_SAVE_EVERYWHERE | snap_manager::form::FORM_BUTTON_SAVE | snap_manager::form::FORM_BUTTON_RESTORE_DEFAULT
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
                , snap_manager::form::FORM_BUTTON_RESET | snap_manager::form::FORM_BUTTON_SAVE_EVERYWHERE | snap_manager::form::FORM_BUTTON_SAVE | snap_manager::form::FORM_BUTTON_RESTORE_DEFAULT
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
                                 " combos work too: 5min 30s. For more, see <a href=\"https://www.freedesktop.org/software/systemd/man/systemd.time.html\">sytemd.time</a>").arg(service_name)
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
                , snap_manager::form::FORM_BUTTON_RESET | snap_manager::form::FORM_BUTTON_SAVE_EVERYWHERE | snap_manager::form::FORM_BUTTON_SAVE | snap_manager::form::FORM_BUTTON_RESTORE_DEFAULT
                );

        snap_manager::widget_input::pointer_t field(std::make_shared<snap_manager::widget_input>(
                          QString("Nice value for %1").arg(service_name)
                        , s.get_field_name()
                        , s.get_value()
                        , "The nice value is the same as the nice command line Unix utility. It changes the priority of the process. The larger the value, the weak the priority of that process (it will yield to processes with a smaller nice value.)"
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
    NOTUSED(button_name);
    NOTUSED(old_or_installation_value);
    NOTUSED(affected_services);

    // restore defaults?
    //
    //bool const use_default_value(button_name == "restore_default");

    if( field_name == "snapbackend::all_services" )
    {
        SNAP_LOG_DEBUG("backend::apply_setting(): snapbackend::all_services!");
        update_all_services( new_value );
        send_update        ( new_value );
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
        QString unit_name(service_name);
        if(!service_info.f_recovery)
        {
            unit_name += ".timer";
        }
        auto const status(snap_manager::manager::string_to_service_status(new_value.toUtf8().data()));
        if( change_service_status( service_info.f_service_executable, unit_name, status ) )
        {
            send_status();
        }

        // Save new configuration in file
        //
        status_file sf;
        QString const status_string( QString::fromUtf8(snap_manager::manager::service_status_to_string(status)) );
        sf[unit_name] = status_string;
        SNAP_LOG_DEBUG("++++ Writing status_file from apply_setting()!");
        sf.save();

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
        if(f_snap->replace_configuration_value(filename, "Service::RestartSec", new_value, snap_manager::REPLACE_CONFIGURATION_VALUE_SECTION))
        {
            // make sure the cache gets updated
            //
            snap_config service_config(std::string("/lib/systemd/system/") + service_name + ".service"
                                     , std::string("/etc/systemd/system/") + service_name + ".service.d/override.conf");
            service_config["Service::RestartSec"] = new_value;
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
        if(f_snap->replace_configuration_value(filename, "Timer::OnUnitActiveSec", new_value, snap_manager::REPLACE_CONFIGURATION_VALUE_SECTION))
        {
            // make sure the cache gets updated
            //
            snap_config service_config(std::string("/lib/systemd/system/") + service_name + ".timer"
                                     , std::string("/etc/systemd/system/") + service_name + ".timer.d/override.conf");
            service_config["Timer::OnUnitActiveSec"] = new_value;
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
        QString const filename(QString("/etc/systemd/system/%1.service.d/override.conf").arg(service_name));
        if(f_snap->replace_configuration_value(filename, "Service::Nice", new_value, snap_manager::REPLACE_CONFIGURATION_VALUE_SECTION))
        {
            // make sure the cache gets updated
            //
            snap_config service_config(std::string("/lib/systemd/system/") + service_name + ".service"
                                     , std::string("/etc/systemd/system/") + service_name + ".service.d/override.conf");
            service_config["Service::Nice"] = new_value;
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





SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
