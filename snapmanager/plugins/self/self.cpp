// Snap Websites Server -- manage the snapmanager.cgi and snapmanagerdaemon
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
#include "self.h"


// snapmanager lib
//
#include "snapmanager/form.h"


// snapwebsites lib
//
#include <snapwebsites/file_content.h>
#include <snapwebsites/log.h>
#include <snapwebsites/process.h>
#include <snapwebsites/qdomhelpers.h>


// snapdev lib
//
#include <snapdev/join_strings.h>
#include <snapdev/not_reached.h>
#include <snapdev/not_used.h>
#include <snapdev/string_pathinfo.h>
#include <snapdev/tokenize_string.h>


// libaddr lib
//
#include <libaddr/addr_parser.h>
#include <libaddr/iface.h>


// Qt lib
//
#include <QFile>


// C lib
//
#include <sys/file.h>
#include <sys/sysinfo.h>


// last include
//
#include <snapdev/poison.h>



SNAP_PLUGIN_START(self, 1, 0)


namespace
{


char const * g_configuration_filename = "snapmanager";
char const * g_configuration_communicator_filename = "snapcommunicator";


void file_descriptor_deleter(int * fd)
{
    if(close(*fd) != 0)
    {
        int const e(errno);
        SNAP_LOG_WARNING("closing file descriptor failed (errno: ")(e)(", ")(strerror(e))(")");
    }
}

} // no name namespace



/** \brief Get a fixed self plugin name.
 *
 * The self plugin makes use of different fixed names. This function
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
    case name_t::SNAP_NAME_SNAPMANAGERCGI_SELF_NAME:
        return "name";

    default:
        // invalid index
        throw snap_logic_exception("Invalid SNAP_NAME_SNAPMANAGERCGI_SELF_...");

    }
    NOTREACHED();
}




/** \brief Initialize the self plugin.
 *
 * This function is used to initialize the self plugin object.
 */
self::self()
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Clean up the self plugin.
 *
 * Ensure the self object is clean before it is gone.
 */
self::~self()
{
}


/** \brief Get a pointer to the self plugin.
 *
 * This function returns an instance pointer to the self plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the self plugin.
 */
self * self::instance()
{
    return g_plugin_self_factory.instance();
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
QString self::description() const
{
    return "Manage the snapmanager.cgi and snapmanagerdaemon settings.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString self::dependencies() const
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
int64_t self::do_update(int64_t last_updated)
{
    NOTUSED(last_updated);

    SNAP_PLUGIN_UPDATE_INIT();
    // no updating in snapmanager*
    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Initialize self.
 *
 * This function terminates the initialization of the self plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void self::bootstrap(snap_child * snap)
{
    f_snap = dynamic_cast<snap_manager::manager *>(snap);
    if(f_snap == nullptr)
    {
        throw snap_logic_exception("snap pointer does not represent a valid manager object.");
    }

    SNAP_LISTEN  ( self, "server", snap_manager::manager, retrieve_status,          _1     );
    SNAP_LISTEN  ( self, "server", snap_manager::manager, add_plugin_commands,      _1     );
    SNAP_LISTEN  ( self, "server", snap_manager::manager, process_plugin_message,   _1, _2 );
}


/** \brief Determine this plugin status data.
 *
 * This function builds a tree of statuses.
 *
 * \param[in] server_status  The map of statuses.
 */
void self::on_retrieve_status(snap_manager::server_status & server_status)
{
    if(f_snap->stop_now_prima())
    {
        return;
    }

    {
        snap_manager::status_t const up(snap_manager::status_t::state_t::STATUS_STATE_INFO, get_plugin_name(), "status", "up");
        server_status.set_field(up);
    }

    {
        snap_manager::status_t const ip(snap_manager::status_t::state_t::STATUS_STATE_INFO, get_plugin_name(), "ip", f_snap->get_public_ip());
        server_status.set_field(ip);
    }

    {
        addr::iface::vector_t interfaces( addr::iface::get_local_addresses() );
        for( auto const & i : interfaces )
        {
            addr::addr const & a(i.get_address());
            if( !a.is_ipv4() )
            //||  addr.get_network_type() != snap_addr::addr::network_type_t::NETWORK_TYPE_PRIVATE )
            {
                continue;
            }

            SNAP_LOG_TRACE("get interface ")(i.get_name())(", ip addr=")(a.to_ipv4_string(addr::addr::string_ip_t::STRING_IP_ALL));
            snap_manager::status_t const iface_status ( snap_manager::status_t::state_t::STATUS_STATE_INFO
                                               , get_plugin_name()
                                               , QString("if::%1 (%2)")
                                                 .arg(i.get_name().c_str())
                                                 .arg(a.get_network_type_string().c_str())
                                               , a.to_ipv4_string(addr::addr::string_ip_t::STRING_IP_ALL).c_str()
                                               );
            server_status.set_field(iface_status);
        }
    }

    {
        struct sysinfo info;
        if(sysinfo(&info) == 0)
        {
            // TODO: also add uptime, loads and how/much used memory
            //       also change the Kb to Mb, Gb, Tb... as required
            //
            QString meminfo(
                    QString("RAM: %1Kb - Swap: %2Kb")
                        .arg((static_cast<long long>(info.totalram) + (static_cast<long long>(info.totalhigh) << 32)) * static_cast<long long>(info.mem_unit) / 1024LL)
                        .arg(static_cast<long long>(info.totalswap) * static_cast<long long>(info.mem_unit) / 1024LL));
            snap_manager::status_t::state_t status(snap_manager::status_t::state_t::STATUS_STATE_INFO);
            struct stat st;
            bool const has_cassandra(stat("/usr/sbin/cassandra", &st) == 0);
            if(info.totalswap > 0)
            {
                // there should not be a swap file along Cassandra
                //
                if(has_cassandra)
                {
                    status = snap_manager::status_t::state_t::STATUS_STATE_HIGHLIGHT;
                    meminfo += " (WARNING: You have a swap file on a system running Cassandra. This is not recommended.)";
                }
            }
            else
            {
                // there should probably be a swap file when Cassandra is
                // not installed on a machine
                //
                if(!has_cassandra)
                {
                    status = snap_manager::status_t::state_t::STATUS_STATE_HIGHLIGHT;
                    meminfo += " (WARNING: You do not have a swap file on this system. This is recommended on most computers except those running Cassandra.)";
                }
            }
            snap_manager::status_t const memory(
                                  status
                                , get_plugin_name()
                                , "memory"
                                , meminfo);
            server_status.set_field(memory);
        }
    }

    {
        // right now we have ONE level for ALL .properties, later we should
        // probably duplicate this code and allow each .properties file to
        // be edited as required
        //
        snap::snap_config logger_properties("/etc/snapwebsites/logger/snapmanagerdaemon.properties");
        std::string const level_appenders(logger_properties["log4cplus.logger.snap"]);
        std::string level("INFO");
        std::string::size_type const pos(level_appenders.find_first_of(','));
        if(pos > 0)
        {
            level = level_appenders.substr(0, pos);
        }
        snap_manager::status_t const up(
                            snap_manager::status_t::state_t::STATUS_STATE_INFO,
                            get_plugin_name(),
                            "log_level",
                            QString::fromUtf8(level.c_str()));
        server_status.set_field(up);
    }

    bool no_installs(false);
    {
        QString const updates(f_snap->count_packages_that_can_be_updated(true));
        if(!updates.isEmpty())
        {
            QStringList msg;
            if( f_system_active )
            {
                msg << "<b>CLUSTER IS NOT IN MAINTENANCE MODE!</b>"
                      "<br/><i>It is highly recommended that your cluster be put in maintenance mode "
                      "before upgrading to avoid data loss.</i><br/><br/>";
            }
            //
            if( f_backends_active > 0 )
            {
                msg << QString("<b>%1 BACKENDS ARE RUNNING!</b>"
                      "<br/><i>It is highly recommended that you disable all of the backends on your cluster "
                      "before upgrading to avoid data loss.</i><br/><br/>")
                    .arg(f_backends_active);
            }
            SNAP_LOG_DEBUG("f_system_active=")(f_system_active)(", f_backends_active=")(f_backends_active)(", msg=")(msg.join("\n"));
            //
            snap_manager::status_t const upgrade_required(
                              snap_manager::status_t::state_t::STATUS_STATE_WARNING
                            , get_plugin_name()
                            , "upgrade_required"
                            , QString("%1;%2").arg(msg.join(" ")).arg(updates)
                            );
            server_status.set_field(upgrade_required);
            no_installs = true;
        }
    }

    {
        // TODO: offer a way to define "/run/reboot-required" in
        //       the snapmanager.conf file
        //
        struct stat st;
        if(stat("/run/reboot-required", &st) == 0)
        {
            QString msg;
            if( f_system_active )
            {
                msg = "<b>CLUSTER IS NOT IN MAINTENANCE MODE!</b>"
                      "<br/><i>It is highly recommended that your cluster be put in maintenance mode "
                      "before rebooting to avoid data loss.</i><br/><br/>";
            }

            // TBD: should we put the content of that file as the message?
            //      (it could be tainted though...)
            //
            snap_manager::status_t const reboot_required(
                              snap_manager::status_t::state_t::STATUS_STATE_WARNING
                            , get_plugin_name()
                            , "reboot_required"
                            , QString("%1Server \"%2\" requires a reboot.")
                                .arg(msg)
                                .arg(f_snap->get_server_name()));
            server_status.set_field(reboot_required);
        }
    }

#if 0
    // this does not currently work properly
    //
    // (list of front end computers -- a.k.a. snap1,snap2,snap3)
    {
        snap::snap_string_list const & frontend_servers(f_snap->get_snapmanager_frontend());
        snap_manager::status_t const frontend(
                      frontend_servers.empty()
                            ? snap_manager::status_t::state_t::STATUS_STATE_WARNING
                            : snap_manager::status_t::state_t::STATUS_STATE_INFO
                    , get_plugin_name()
                    , "snapmanager_frontend"
                    , frontend_servers.join(","));
        server_status.set_field(frontend);
    }
#endif

    {
        std::string const redirect_unwanted(f_snap->get_parameter("redirect_unwanted"));
        snap_manager::status_t const redirect(
                      snap_manager::status_t::state_t::STATUS_STATE_INFO
                    , get_plugin_name()
                    , "redirect_unwanted"
                    , QString::fromUtf8(redirect_unwanted.c_str()));
        server_status.set_field(redirect);
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
bool self::display_value(QDomElement parent, snap_manager::status_t const & s, snap::snap_uri const & uri)
{
    QDomDocument doc(parent.ownerDocument());

    if(s.get_field_name() == "refresh")
    {
        // create a form with one Refresh button
        //
        snap_manager::form f(
                  get_plugin_name()
                , s.get_field_name()
                , snap_manager::form::FORM_BUTTON_REFRESH
                );

        snap_manager::widget_description::pointer_t field(std::make_shared<snap_manager::widget_description>(
                          "Click Refresh to request a new status from all the snapcommunicators, including this one."
                        , s.get_field_name()
                        , "This button makes sure that all snapcommunicators resend their status data so that way you get the latest."
                          " Note that the resending is not immediate. The thread handling the status wakes up once every minute or so,"
                          " therefore you will get new data for snapmanager.cgi within 1 or 2 minutes."
                        ));
        f.add_widget(field);

        f.generate(parent, uri);

        return true;
    }

#if 0
    if(s.get_field_name() == "snapmanager_frontend")
    {
        // the list if frontend snapmanagers that are to receive statuses
        // of the cluster computers; may be just one computer; should not
        // be empty; shows a text input field
        //
        snap_manager::form f(
                  get_plugin_name()
                , s.get_field_name()
                , snap_manager::form::FORM_BUTTON_RESET | snap_manager::form::FORM_BUTTON_SAVE_EVERYWHERE
                );

        snap_manager::widget_input::pointer_t field(std::make_shared<snap_manager::widget_input>(
                          "List of Front End Servers"
                        , s.get_field_name()
                        , s.get_value()
                        , QString("This is a list of Front End servers that accept requests to snapmanager.cgi."
                                 " Only the few computers that accept such request need to be named here."
                                 " Names are expected to be comma separated.%1")
                                .arg(s.get_state() == snap_manager::status_t::state_t::STATUS_STATE_WARNING
                                        ? " <span style=\"color: red;\">The Warning Status is due to the fact that"
                                          " the list on this computer is currently empty. If it was not defined yet,"
                                          " add the value. If it is defined on other servers, you may want to go on"
                                          " one of those servers page and click Save Everywhere from there.</span>"
                                        : "")
                        ));
        f.add_widget(field);

        f.generate(parent, uri);

        return true;
    }
#endif

    if(s.get_field_name() == "redirect_unwanted")
    {
        // the list of URIs from which we can download software bundles;
        // this should not be empty; shows a text input field
        //
        snap_manager::form f(
                  get_plugin_name()
                , s.get_field_name()
                , snap_manager::form::FORM_BUTTON_RESET | snap_manager::form::FORM_BUTTON_SAVE | snap_manager::form::FORM_BUTTON_SAVE_EVERYWHERE
                );

        snap_manager::widget_input::pointer_t field(std::make_shared<snap_manager::widget_input>(
                          "A URI to redirect unwanted hits"
                        , s.get_field_name()
                        , s.get_value()
                        , "Whenever a user who is not in the list of clients=... hits the snapmanager.cgi script, he will be redirected to this URI. Absolutely any URI can be used."
                        ));
        f.add_widget(field);

        f.generate(parent, uri);

        return true;
    }

    if(s.get_field_name() == "reboot_required")
    {
        // the OS declared that a reboot was required, offer the option
        //
        snap_manager::form f(
                  get_plugin_name()
                , s.get_field_name()
                , snap_manager::form::FORM_BUTTON_REBOOT
                );

        snap_manager::widget_description::pointer_t field(std::make_shared<snap_manager::widget_description>(
                          "Reboot Required"
                        , s.get_field_name()
                        , s.get_value() // the value is the description!
                        ));
        f.add_widget(field);

        f.generate(parent, uri);

        return true;
    }

    if(s.get_field_name() == "log_level")
    {
        // the current log level (at least in snapmanagerdaemon.properties)
        //
        snap_manager::form f(
                  get_plugin_name()
                , s.get_field_name()
                , snap_manager::form::FORM_BUTTON_RESET | snap_manager::form::FORM_BUTTON_SAVE | snap_manager::form::FORM_BUTTON_SAVE_EVERYWHERE
                );

        QStringList log_level_list;
        log_level_list << "TRACE";
        log_level_list << "DEBUG";
        log_level_list << "INFO";
        log_level_list << "WARNING";
        log_level_list << "ERROR";
        log_level_list << "FATAL";

        snap_manager::widget_select::pointer_t field(std::make_shared<snap_manager::widget_select>(
                          "Enter Log Level"
                        , s.get_field_name()
                        , log_level_list
                        , s.get_value()
                        , "<p>The log level can be any one of the following:</p>"
                          "<ul>"
                            "<li>TRACE -- trace level, you get everything!</li>"
                            "<li>DEBUG -- debug level, you get additional logs about things that may be problems.</li>"
                            "<li>INFO -- normal informational level, this is the default.</li>"
                            "<li>WARNING -- only display warnings, errors and fatal errors, no additional information.</li>"
                            "<li>ERROR -- only display errors and fatal errors.</li>"
                            "<li>FATAL -- only display messages about fatal errors (why a service quits abnormally when it has a chance to log such.)</li>"
                          "</ul>"
                        ));
        f.add_widget(field);

        f.generate(parent, uri);

        return true;
    }

    if(s.get_field_name() == "upgrade_required")
    {
        // the OS declared that a reboot was required, offer the option
        //
        snap_manager::form f(
                  get_plugin_name()
                , s.get_field_name()
                ,   snap_manager::form::FORM_BUTTON_UPGRADE
                  | snap_manager::form::FORM_BUTTON_UPGRADE_EVERYWHERE
                );

        snap::snap_string_list counts(s.get_value().split(";"));
        if(counts.size() < 3)
        {
            // put some defaults to avoid crashes
            counts << "0";
            counts << "0";
            counts << "0";
        }

        QString const description(QString("%1%2 packages can be updated.<br/>%3 updates are security updates.")
                            .arg(counts[0])
                            .arg(counts[1])
                            .arg(counts[2])
                            );

        snap_manager::widget_description::pointer_t field(std::make_shared<snap_manager::widget_description>(
                          "Upgrade Required"
                        , s.get_field_name()
                        , description
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
 *            for the bundles plugin that manages bundles.)
 *
 * \return true if the new_value was applied and the affected_services
 *         should be sent their RELOADCONFIG message.
 */
bool self::apply_setting(QString const & button_name
                       , QString const & field_name
                       , QString const & new_value
                       , QString const & old_or_installation_value
                       , std::set<QString> & affected_services)
{
    NOTUSED(old_or_installation_value);

    // refresh is a special case in the "self" plugin only
    //
    if(button_name == "refresh")
    {
        snap_config snap_communicator_conf(g_configuration_communicator_filename);
        QString const signal_secret(snap_communicator_conf["signal_secret"]);

        {
            // setup the message to send to other snapmanagerdaemons
            //
            snap::snap_communicator_message resend;
            resend.set_service("*");
            resend.set_command("MANAGERRESEND");
            resend.add_parameter("kick", "now");

            // we just send a UDP message in this case, no acknowledgement
            //
            snap::snap_communicator::snap_udp_server_message_connection::send_message(f_snap->get_signal_address()
                                                                                    , f_snap->get_signal_port()
                                                                                    , resend
                                                                                    , signal_secret.toUtf8().data());
        }
        {
            snap::snap_communicator_message cgistatus;
            cgistatus.set_service("*");
            cgistatus.set_command("CGISTATUS_REQUEST");

            // we just send a UDP message in this case, no acknowledgement
            //
            snap::snap_communicator::snap_udp_server_message_connection::send_message(f_snap->get_signal_address()
                                                                                    , f_snap->get_signal_port()
                                                                                    , cgistatus
                                                                                    , signal_secret.toUtf8().data());
        }
        {
            snap::snap_communicator_message backendstatus;
            backendstatus.set_service("*");
            backendstatus.set_command("BACKENDSTATUS_REQUEST");

            // we just send a UDP message in this case, no acknowledgement
            //
            snap::snap_communicator::snap_udp_server_message_connection::send_message(f_snap->get_signal_address()
                                                                                    , f_snap->get_signal_port()
                                                                                    , backendstatus
                                                                                    , signal_secret.toUtf8().data());
        }

        // messages sent...
        //
        // we also ask for the snapmanagerdaemon to restart otherwise
        // the bundles would not get reloaded from the remote server
        //
        affected_services.insert("snapmanagerdaemon");
        return true;
    }

    // after installations and upgrades, a reboot may be required
    //
    if(button_name == "reboot")
    {
        f_snap->reboot();
        return true;
    }

    // once in a while packages get an update, the upgrade button appears
    // and when clicked this funtion gets called
    //
    if(button_name == "upgrade"
    || button_name == "upgrade_everywhere")
    {
        NOTUSED(f_snap->upgrader());
        //f_snap->reset_aptcheck(); -- this is too soon, the upgrader() call
        //                             now creates a child process with fork()
        //                             to make sure we can go on even when
        //                             snapmanagerdaemon gets upgraded

        // TBD: we need to add something to the affected_services?
        //      (the snapupgrader tool should restart the whole stack
        //      anyway so we should be fine...)
        //
        return true;
    }

    // restore defaults?
    //
    bool const use_default_value(button_name == "restore_default");

    // WARNING: since we commented out the snapmanager_frontend for now
    //          we should never get here with such a field name so we
    //          do not have to do that here
    //
    if(field_name == "snapmanager_frontend")
    {
        affected_services.insert("snapmanagerdaemon");

        QString value(new_value);
        if(use_default_value)
        {
            value = "";
        }

        // TODO: the path to the snapmanager.conf is hard coded, it needs to
        //       use the path of the file used to load the .conf in the
        //       first place (I'm just not too sure how to get that right
        //       now, probably from the "--config" parameter, but how do
        //       we do that for each service?)
        //
        NOTUSED(f_snap->replace_configuration_value("/etc/snapwebsites/snapwebsites.d/snapmanager.conf", field_name, value));
        return true;
    }

    if(field_name == "redirect_unwanted")
    {
        // replace the value in that field so it shows updated in the
        // interface as well
        //
        snap::snap_config snapmanager(g_configuration_filename);
        snapmanager["redirect_unwanted"] = new_value;

        // we do not need to restart anything because the value is used
        // by the snapmanager.cgi which is started on each user access
        //
        NOTUSED(f_snap->replace_configuration_value("/etc/snapwebsites/snapwebsites.d/snapmanager.conf", field_name, new_value));
        return true;
    }

    // user wants a new log level
    //
    if(field_name == "log_level")
    {
        // we have to restart all the services, by restarting snapcommunicator
        // though, it restarts everything.
        //
        affected_services.insert("snapcommunicator");

        SNAP_LOG_DEBUG("Running command: snapchangeloglevel ")(new_value);
        int const r(system(("snapchangeloglevel " + new_value).toUtf8().data()));
        if(r != 0)
        {
            int const e(errno);
            SNAP_LOG_ERROR("snapchangeloglevel failed with error: ")(e)(", ")(strerror(e));
        }
        return true;
    }

    return false;
}


void self::on_add_plugin_commands(snap::snap_string_list & understood_commands)
{
    understood_commands << "BACKENDSTATUS";
    understood_commands << "CGISTATUS";
}


void self::on_process_plugin_message(snap::snap_communicator_message const & message, bool & processed)
{
    QString const command(message.get_command());
    SNAP_LOG_TRACE("self::on_process_plugin_message(), command=[")(command)("]");

    if(command == "BACKENDSTATUS")
    {
        f_backends_active = 0;

        auto const & params( message.get_all_parameters() );
        for( auto const & key : params.keys() )
        {
            if( !key.startsWith("backend_") )
            {
                continue;
            }
            //
            snap_manager::service_status_t status( snap_manager::manager::string_to_service_status(params[key].toUtf8().data()) );
            switch( status )
            {
                case snap_manager::service_status_t::SERVICE_STATUS_UNKNOWN       :
                case snap_manager::service_status_t::SERVICE_STATUS_NOT_INSTALLED :
                case snap_manager::service_status_t::SERVICE_STATUS_DISABLED      :
                    break;

                case snap_manager::service_status_t::SERVICE_STATUS_ENABLED       :
                case snap_manager::service_status_t::SERVICE_STATUS_ACTIVE        :
                case snap_manager::service_status_t::SERVICE_STATUS_FAILED        :
                    ++f_backends_active;
                    break;
            }
        }

        SNAP_LOG_DEBUG("BACKENDSTATUS received! f_backends_active=")(f_backends_active);
        processed = true;
    }
    else if( command == "CGISTATUS" )
    {
        f_system_active = (message.get_integer_parameter("status") == 0);
        SNAP_LOG_DEBUG("CGISTATUS received! f_system_active=")(f_system_active);
        processed = true;
    }
}



SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
