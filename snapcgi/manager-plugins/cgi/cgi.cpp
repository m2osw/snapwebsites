// Snap Websites Server -- manage the snapcgi settings
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

// cgi
//
#include "cgi.h"

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


SNAP_PLUGIN_START(cgi, 1, 0)


namespace
{

// TODO: offer the user a way to change this path?
//char const * g_service_filename = "/etc/snapwebsites/services.d/service-snapcgi.xml";

char const * g_configuration_filename = "snapcgi";

char const * g_configuration_d_filename = "/etc/snapwebsites/snapwebsites.d/snapcgi.conf";

char const * g_configuration_apache2_maintenance = "/etc/apache2/snap-conf/snap-apache2-maintenance.conf";


void file_descriptor_deleter(int * fd)
{
    if(close(*fd) != 0)
    {
        int const e(errno);
        SNAP_LOG_WARNING("closing file descriptor failed (errno: ")(e)(", ")(strerror(e))(")");
    }
}


} // no name namespace



/** \brief Get a fixed cgi plugin name.
 *
 * The cgi plugin makes use of different fixed names. This function
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
    case name_t::SNAP_NAME_SNAPMANAGERCGI_CGI_NAME:
        return "name";

    default:
        // invalid index
        throw snap_logic_exception("Invalid SNAP_NAME_SNAPMANAGERCGI_CGI_...");

    }
    NOTREACHED();
}




/** \brief Initialize the cgi plugin.
 *
 * This function is used to cgiialize the cgi plugin object.
 */
cgi::cgi()
    //: f_snap(nullptr) -- auto-cgi
{
}


/** \brief Clean up the cgi plugin.
 *
 * Ensure the cgi object is clean before it is gone.
 */
cgi::~cgi()
{
}


/** \brief Get a pointer to the cgi plugin.
 *
 * This function returns an instance pointer to the cgi plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the cgi plugin.
 */
cgi * cgi::instance()
{
    return g_plugin_cgi_factory.instance();
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
QString cgi::description() const
{
    return "Manage the snapcgi settings.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString cgi::dependencies() const
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
int64_t cgi::do_update(int64_t last_updated)
{
    NOTUSED(last_updated);

    SNAP_PLUGIN_UPDATE_INIT();
    // no updating in snapmanager*
    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Initialize cgi.
 *
 * This function terminates the cgiialization of the cgi plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void cgi::bootstrap(snap_child * snap)
{
    f_snap = dynamic_cast<snap_manager::manager *>(snap);
    if(f_snap == nullptr)
    {
        throw snap_logic_exception("snap pointer does not represent a valid manager object.");
    }

    SNAP_LISTEN  ( cgi, "server", snap_manager::manager, retrieve_status,          _1     );
    SNAP_LISTEN  ( cgi, "server", snap_manager::manager, add_plugin_commands,      _1     );
    SNAP_LISTEN  ( cgi, "server", snap_manager::manager, process_plugin_message,   _1, _2 );
    SNAP_LISTEN0 ( cgi, "server", snap_manager::manager, communication_ready              );
}


int get_retry_from_content( std::string const & content )
{
    int retry_after(0);
    std::string::size_type const pos(content.find("##MAINTENANCE-START##"));
    char const * s(content.c_str() + pos + 21);
    while(isspace(*s))
    {
        ++s;
    }
    if(*s != '#')
    {
        std::string::size_type const ra_pos(content.find("Retry-After"));
        char const * ra(content.c_str() + ra_pos + 11);
        for(; *ra == '"' || isspace(*ra); ++ra);
        for(; *ra >= '0' && *ra <= '9'; ++ra)
        {
            retry_after = retry_after * 10 + *ra - '0';
            if(retry_after > 365 * 24 * 60 * 60) // more than 1 year?!?
            {
                retry_after = 365 * 24 * 60 * 60;
                break;
            }
        }
        if(retry_after < 60) // less than a minute?!?
        {
            retry_after = 60;
        }
    }
    return retry_after;
}


/** \brief Determine this plugin status data.
 *
 * This function builds a tree of statuses.
 *
 * \param[in] server_status  The map of statuses.
 */
void cgi::on_retrieve_status(snap_manager::server_status & server_status)
{
    if(f_snap->stop_now_prima())
    {
        return;
    }

    // TODO: find a way to get the configuration filename for snapcgi
    //       (i.e. take it from the XML?)
    {
        snap_config snap_cgi(g_configuration_filename);

        snap_manager::status_t const snapserver(
                      snap_manager::status_t::state_t::STATUS_STATE_INFO
                    , get_plugin_name()
                    , "snapserver"
                    , snap_cgi["snapserver"]);
        server_status.set_field(snapserver);
    }

    // allow for turning maintenance ON or OFF
    {
        // TODO: determine the current status
        //
        snap::file_content conf(g_configuration_apache2_maintenance);
        if(conf.exists())
        {
            int const retry_after = conf.read_all()? get_retry_from_content(conf.get_content()) : 0;

            // TODO: display retry_after in minutes, hours, days...
            snap_manager::status_t const maintenance(
                          snap_manager::status_t::state_t::STATUS_STATE_INFO
                        , get_plugin_name()
                        , "maintenance"
                        , retry_after == 0 ? "in-service" : QString("%1").arg(retry_after));
            server_status.set_field(maintenance);
        }
        else
        {
            snap_manager::status_t const maintenance(
                          snap_manager::status_t::state_t::STATUS_STATE_ERROR
                        , get_plugin_name()
                        , "maintenance"
                        , QString::fromUtf8(g_configuration_apache2_maintenance) + " is missing");
            server_status.set_field(maintenance);
        }
    }

}


void cgi::send_status( snap::snap_communicator_message const* message )
{
    snap::snap_communicator_message cmd;
    cmd.set_command("CGISTATUS");
    //
    if( message == nullptr )
    {
        cmd.set_service("*");
    }
    else
    {
        cmd.reply_to(*message);
    }
    //
    snap::file_content conf(g_configuration_apache2_maintenance);
    if(conf.exists())
    {
        int const retry_after = conf.read_all()? get_retry_from_content(conf.get_content()) : 0;
        cmd.add_parameter( "status", retry_after );
    }
    else
    {
        cmd.add_parameter( "status", 0 );
    }

    //cmd.add_parameter( "cache" , "ttl=60"     );
    f_snap->forward_message(cmd);

    SNAP_LOG_DEBUG("CGISTATUS message sent!");
}


void cgi::on_communication_ready()
{
    send_status();
}


void cgi::on_add_plugin_commands(snap::snap_string_list & understood_commands)
{
    understood_commands << "CGISTATUS_REQUEST";
}


void cgi::on_process_plugin_message(snap::snap_communicator_message const & message, bool & processed)
{
    QString const command(message.get_command());
    SNAP_LOG_TRACE("cgi::on_process_plugin_message(), command=[")(command)("]");

    if(command == "CGISTATUS_REQUEST")
    {
        send_status( &message );
        processed = true;
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
bool cgi::display_value(QDomElement parent, snap_manager::status_t const & s, snap::snap_uri const & uri)
{
    QDomDocument doc(parent.ownerDocument());

    if(s.get_field_name() == "snapserver")
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
                          "IP Address and Port (IP:Port) to connect to the snapserver service:"
                        , s.get_field_name()
                        , s.get_value()
                        , "By default this is set to 127.0.0.1:4004 as we expect that the snapserver"
                         " will also be running on the server running Apache2. It is possible, though,"
                         " to put snapserver on other computers for safety and increased resources. In"
                         " that case, enter the Private Network IP address of a snapserver to contact."
                         " At some point, this will be a list of such IP:port, but we do not yet"
                         " support such."
                        ));
        f.add_widget(field);

        f.generate(parent, uri);

        return true;
    }

    if(s.get_field_name() == "maintenance")
    {
        // if there is an error, we do not offer the user to do anything
        // (i.e. field is in display only mode)
        //
        if(s.get_state() == snap_manager::status_t::state_t::STATUS_STATE_ERROR)
        {
            snap_manager::form f(
                      get_plugin_name()
                    , s.get_field_name()
                    , snap_manager::form::FORM_BUTTON_NONE
                    );

            snap_manager::widget_description::pointer_t field(std::make_shared<snap_manager::widget_description>(
                              "Maintenance Mode Not Available"
                            , s.get_field_name()
                            , s.get_value() // the value has additional information
                            ));
            f.add_widget(field);

            f.generate(parent, uri);
        }
        else
        {
            snap_manager::form f(
                      get_plugin_name()
                    , s.get_field_name()
                    , snap_manager::form::FORM_BUTTON_RESET | snap_manager::form::FORM_BUTTON_SAVE_EVERYWHERE
                    );

            snap_manager::widget_input::pointer_t field(std::make_shared<snap_manager::widget_input>(
                              "Service Mode:"
                            , s.get_field_name()
                            , s.get_value()
                            , "<p>The <b>Service Mode</b> defines whether the service is currently"
                             " \"in-service\", which means the website serves pages as expected"
                             " or in maintenance (number of seconds the maintenance will take),"
                             " which means we display a maintenance page only.</p>"
                             "<p>Note: You may enter a number followed by 's' for seconds,"
                             " 'm' for minutes, 'h' for hours, 'd' for days.</p>"
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
bool cgi::apply_setting(QString const & button_name, QString const & field_name, QString const & new_value, QString const & old_or_installation_value, std::set<QString> & affected_services)
{
    NOTUSED(old_or_installation_value);
    NOTUSED(button_name);

    if(field_name == "snapserver")
    {
        // fix the value in memory
        //
        snap_config snap_cgi(g_configuration_filename);
        snap_cgi["snapserver"] = new_value;

        NOTUSED(f_snap->replace_configuration_value(g_configuration_d_filename, "snapserver", new_value));
        return true;
    }

    if(field_name == "maintenance")
    {
        // TODO: actually call a script so someone at the console could
        //       turn on/off the maintenance mode on a computer
        //

        int retry_after(0);
        if(new_value != "in-service")
        {
            std::string const retry_after_str(new_value.toUtf8().data());
            char const * ra(retry_after_str.c_str());
            for(; *ra >= '0' && *ra <= '9'; ++ra)
            {
                retry_after = retry_after * 10 + *ra - '0';
            }
            for(; isspace(*ra); ++ra);
            switch(*ra)
            {
            case 's':
                // already in seconds
                //retry_after *= 1;
                break;

            case 'm':
                retry_after *= 60;
                break;

            case 'h':
                retry_after *= 60 * 60;
                break;

            case 'd':
                retry_after *= 24 * 60 * 60;
                break;

            //default: error?
            }
        }

        snap::process p("go to maintenance");
        p.set_mode(snap::process::mode_t::PROCESS_MODE_OUTPUT);
        p.set_command("sed");
        p.add_argument("-i.bak");
        p.add_argument("-e");

        if(retry_after != 0)
        {
            // go from in-service to maintenance
            //
            p.add_argument("'/##MAINTENANCE-START##/,/##MAINTENANCE-END##/ s/^#\\([^#]\\)/\\1/'");

            // also change the Retry-After in this case
            //
            p.add_argument("-e");
            p.add_argument(QString("'/##MAINTENANCE-START##/,/##MAINTENANCE-END##/ s/Retry-After \".*\"/Retry-After \"%1\"/'").arg(retry_after));
        }
        else
        {
            // go from maintenance to in-service
            //
            p.add_argument("'/##MAINTENANCE-START##/,/##MAINTENANCE-END##/ s/^\\([^#]\\)/#\\1/'");

            // leave the last Retry-After as it was
        }

        p.add_argument(QString::fromUtf8(g_configuration_apache2_maintenance));
        int const r(p.run());
        if(r != 0)
        {
            SNAP_LOG_ERROR("The sed command to switch between maintenance and in-service failed with ")(r)(", output: ")(p.get_output(true).trimmed());
        }

        // make sure apache2 gets reloaded too
        //
        affected_services.insert("apache2-reload");

        send_status();

        return true;
    }

    return false;
}





SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
