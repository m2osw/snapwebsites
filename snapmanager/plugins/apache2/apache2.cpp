// Snap Websites Server -- handle Snap! files apache2 settings
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

// apache2
//
#include "apache2.h"

// our lib
//
#include "snapmanager/form.h"

// snapwebsites lib
//
#include <snapwebsites/file_content.h>
#include <snapwebsites/log.h>
#include <snapwebsites/not_reached.h>
#include <snapwebsites/not_used.h>
#include <snapwebsites/process.h>

// C++ lib
//
#include <fstream>

// last entry
//
#include <snapwebsites/poison.h>


SNAP_PLUGIN_START(apache2, 1, 0)


namespace
{

    std::string const g_snapmanager_apache_conf = "/etc/apache2/snap-conf/snapmanager/snapmanager-apache2-common.conf";
    std::string const g_snapcgi_apache_conf     = "/etc/apache2/snap-conf/default/000-snap-apache2-default-common.conf";

} // no name namespace



/** \brief Get a fixed apache2 plugin name.
 *
 * The apache2 plugin makes use of different fixed names. This function
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
    case name_t::SNAP_NAME_SNAPMANAGERCGI_APACHE2_NAME:
        return "name";

    default:
        // invalid index
        throw snap_logic_exception("Invalid SNAP_NAME_SNAPMANAGERCGI_APACHE2_...");

    }
    NOTREACHED();
}




/** \brief Initialize the apache2 plugin.
 *
 * This function is used to initialize the apache2 plugin object.
 */
apache2::apache2()
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Clean up the apache2 plugin.
 *
 * Ensure the apache2 object is clean before it is gone.
 */
apache2::~apache2()
{
}


/** \brief Get a pointer to the apache2 plugin.
 *
 * This function returns an instance pointer to the apache2 plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the apache2 plugin.
 */
apache2 * apache2::instance()
{
    return g_plugin_apache2_factory.instance();
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
QString apache2::description() const
{
    return "Handle the settings in the apache2.conf files provided by Snap! Websites.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString apache2::dependencies() const
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
int64_t apache2::do_update(int64_t last_updated)
{
    NOTUSED(last_updated);

    SNAP_PLUGIN_UPDATE_INIT();
    // no updating in snapmanager*
    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Initialize apache2.
 *
 * This function terminates the initialization of the apache2 plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void apache2::bootstrap(snap_child * snap)
{
    f_snap = dynamic_cast<snap_manager::manager *>(snap);
    if(f_snap == nullptr)
    {
        throw snap_logic_exception("snap pointer does not represent a valid manager object.");
    }

    SNAP_LISTEN  ( apache2, "server", snap_manager::manager, retrieve_status,          _1     );
    SNAP_LISTEN  ( apache2, "server", snap_manager::manager, handle_affected_services, _1     );
}


/** \brief Determine this plugin status data.
 *
 * This function builds a tree of statuses.
 *
 * \param[in] server_status  The map of statuses.
 */
void apache2::on_retrieve_status(snap_manager::server_status & server_status)
{
    if(f_snap->stop_now_prima())
    {
        return;
    }

    // retrieve the two status
    //
    retrieve_status_of_conf(server_status, "snapmanager", g_snapmanager_apache_conf );
    retrieve_status_of_conf(server_status, "snapcgi",     g_snapcgi_apache_conf     );
}


void apache2::retrieve_status_of_conf(snap_manager::server_status & server_status,
                                      std::string const & conf_namespace,
                                      std::string const & conf_filename)
{
    // get the data
    //
    file_content fc(conf_filename);
    if(fc.read_all())
    {
        // could read the file, go on
        //
        bool found(false);
        std::string const content(fc.get_content());
        std::string::size_type const pos(snap_manager::manager::search_parameter(content, "servername", 0, true));
        if(pos != std::string::npos)
        {
            if(pos == 0 || content[pos - 1] != '#')
            {
                // found one, get the name
                //
                std::string::size_type const p1(content.find_first_of(" \t", pos));
                if(p1 != std::string::npos)
                {
                    std::string::size_type const p2(content.find_first_not_of(" \t", p1));
                    if(p2 != std::string::npos)
                    {
                        std::string::size_type const p3(content.find_first_of(" \t\r\n", p2));
                        if(p3 != std::string::npos)
                        {
                            std::string const name(content.substr(p2, p3 - p2));

                            snap_manager::status_t const conf_field(
                                              snap_manager::status_t::state_t::STATUS_STATE_INFO
                                            , get_plugin_name()
                                            , QString::fromUtf8((conf_namespace + "::server_name").c_str())
                                            , QString::fromUtf8(name.c_str()));
                            server_status.set_field(conf_field);

                            found = true;
                        }
                    }
                }
            }
            else
            {
                // we found a ServerName but it is "immediately" commented
                // out (immediately preceeded by a '#') so here we see
                // such as ""
                //
                snap_manager::status_t const conf_field(
                                  snap_manager::status_t::state_t::STATUS_STATE_HIGHLIGHT
                                , get_plugin_name()
                                , QString::fromUtf8((conf_namespace + "::server_name").c_str())
                                , QString());
                server_status.set_field(conf_field);

                found = true;
            }
        }
        if(!found)
        {
            // we got the file, but could not find the field as expected
            //
            snap_manager::status_t const conf_field(
                              snap_manager::status_t::state_t::STATUS_STATE_WARNING
                            , get_plugin_name()
                            , QString::fromUtf8((conf_namespace + "::server_name").c_str())
                            , ("\"" + conf_filename + "\" is not editable at the moment.").c_str());
            server_status.set_field(conf_field);
        }

        // try to see whether we have a RewriteRule setting up an
        // environment variable named STATUS (it should always be
        // there)
        //
        // at this point only the snapmanager has such
        //
        if(conf_namespace == "snapmanager")
        {
            std::string::size_type const website_status_pos(content.find("RewriteRule .* - [env=STATUS:"));
            if(website_status_pos != std::string::npos)
            {
                std::string::size_type const website_status_end(content.find("]", website_status_pos + 29));
                std::string const website_status(content.substr(website_status_pos + 29, website_status_end - website_status_pos - 29));

                snap_manager::status_t const conf_field(
                                  website_status == "new"
                                        ? snap_manager::status_t::state_t::STATUS_STATE_WARNING
                                        : snap_manager::status_t::state_t::STATUS_STATE_INFO
                                , get_plugin_name()
                                , QString::fromUtf8((conf_namespace + "::website_status").c_str())
                                , QString::fromUtf8(website_status.c_str()));
                server_status.set_field(conf_field);
            }
        }
    }
    else if(fc.exists())
    {
        // create an error field which is not editable
        //
        snap_manager::status_t const conf_field(
                          snap_manager::status_t::state_t::STATUS_STATE_WARNING
                        , get_plugin_name()
                        , QString::fromUtf8((conf_namespace + "::server_name").c_str())
                        , ("\"" + conf_filename + "\" is not editable at the moment.").c_str());
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
bool apache2::display_value(QDomElement parent, snap_manager::status_t const & s, snap::snap_uri const & uri)
{
    QDomDocument doc(parent.ownerDocument());

    if(s.get_field_name().endsWith("::server_name"))
    {
        // in case it is not marked as INFO, it is "not editable" (we are
        // unsure of the current file format)
        //
        if(s.get_state() == snap_manager::status_t::state_t::STATUS_STATE_WARNING)
        {
            return false;
        }

        // the server name
        //
        snap_manager::form f(
                  get_plugin_name()
                , s.get_field_name()
                ,   snap_manager::form::FORM_BUTTON_RESET
                  | snap_manager::form::FORM_BUTTON_RESTORE_DEFAULT
                  | snap_manager::form::FORM_BUTTON_SAVE
                );

        snap_manager::widget_input::pointer_t field(std::make_shared<snap_manager::widget_input>(
                          "Apache2 'ServerName'"
                        , s.get_field_name()
                        , s.get_value()
                        , "Enter the name of the server. This name becomes mandatory for snapmanager.cgi if you intend to install the snapfront bundle. For snap.cgi, it is a good idea to put your main website name so Apache2 gets a form of fallback."
                        ));
        f.add_widget(field);

        f.generate(parent, uri);

        return true;
    }

    if(s.get_field_name().endsWith("::website_status"))
    {
        // the website status
        //
        snap_manager::form f(
                  get_plugin_name()
                , s.get_field_name()
                ,   snap_manager::form::FORM_BUTTON_RESET
                  | snap_manager::form::FORM_BUTTON_RESTORE_DEFAULT
                  | snap_manager::form::FORM_BUTTON_SAVE
                  | snap_manager::form::FORM_BUTTON_SAVE_EVERYWHERE
                );

        QStringList statuses;
        statuses << "new";
        statuses << "installed";
        snap_manager::widget_select::pointer_t field(std::make_shared<snap_manager::widget_select>(
                          "Website Status"
                        , s.get_field_name()
                        , statuses
                        , s.get_value()
                        , "Enter the status of the website. Either \"new\" or \"installed\"."
                         " When set to \"new\", the end users can see the index.html help page"
                         " which can make it easy to determine the version of the Snap!"
                         " environment. We strongly suggest that you use \"installed\"."
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
 *
 * \return true if the new_value was handled by this function.
 */
bool apache2::apply_setting(QString const & button_name, QString const & field_name, QString const & new_value, QString const & old_or_installation_value, std::set<QString> & affected_services)
{
    NOTUSED(old_or_installation_value);

    // we support Save and Restore Default of the id_rsa.pub file
    //
    if(field_name.endsWith("::server_name"))
    {
        // if the user was asking to restore the default, the default is
        // to have the ServerName parameter commented out
        //
        bool const comment(button_name == "restore_default");

        // make sure that we received a valid button click
        //
        if(!comment
        && button_name != "save")
        {
            return true;
        }

        bool const update_snapmanager(field_name.startsWith("snapmanager::"));

        if(!update_snapmanager
        && comment)
        {
            // there is no "default" for the snap.gci configuration
            return true;
        }

        // generate the path to the id_rsa file
        //
        std::string const conf_filename(update_snapmanager
                                            ? g_snapmanager_apache_conf
                                            : g_snapcgi_apache_conf
                                       );

        // get the current data
        //
        file_content fc(conf_filename);
        bool success(fc.read_all());
        if(!success)
        {
            return true;
        }

        // make a backup
        //
        if(!fc.write_all(conf_filename + ".bak"))
        {
            return true;
        }

        // retrieve the current content
        //
        std::string content(fc.get_content());

        // compute the new ServerName line(s)
        //
        std::string const new_name(std::string(comment ? "#" : "")
                                 + "ServerName "
                                 + (comment ? "snap.example.com" : new_value.toUtf8().data())
                             );

        // search each ServerName line and replace it
        //
        for(std::string::size_type pos(0); pos < content.length(); pos += new_name.length())
        {
            // search for the ServerName parameter
            //
            pos = snap_manager::manager::search_parameter(content, "servername", pos, true);
            if(pos == std::string::size_type(-1))
            {
                // we are done, there are no more ServerName entries
                // (or there were none at all?!)
                //
                break;
            }

            // include the '#' if present immediately before the
            // parameter name
            //
            if(pos > 0 && content[pos - 1] == '#')
            {
                --pos;
            }

            // search the end of the line so that way we can cut the
            // parameter in three
            //
            std::string::size_type eol(content.find_first_of("\r\n", pos));
            if(eol == std::string::size_type(-1))
            {
                // something went wrong (i.e. the ServerName should not
                // be the very last thing in the file)
                //
                return true;
            }

            // we have the start and the end so we can now cut the
            // string and save the new parameters
            //
            content = content.substr(0, pos)
                    + new_name
                    + content.substr(eol);
        }

        // it worked, let's save the result
        //
        fc.set_content(content);
        if(fc.write_all())
        {
            // it worked, make sure apache2 gets restarted
            //
            affected_services.insert("apache2-restart");
            return true;
        }

        // the write back to disk failed
        //
        return true;
    }

    if(field_name.endsWith("::website_status"))
    {
        // if the user was asking to restore the default, the default is
        // to have the status set to "new"
        //
        QString new_website_status(
                    button_name == "restore_default"
                        ? "new"
                        : new_value
                );

        // get the current data
        //
        file_content fc(g_snapmanager_apache_conf);
        bool success(fc.read_all());
        if(!success)
        {
            return true;
        }

        // make a backup
        //
        if(!fc.write_all(g_snapmanager_apache_conf + ".bak"))
        {
            return true;
        }

        // retrieve the current content
        //
        std::string const content(fc.get_content());

        std::string::size_type const website_status_pos(content.find("RewriteRule .* - [env=STATUS:"));
        if(website_status_pos == std::string::npos)
        {
            return true;
        }

        std::string::size_type const website_status_end(content.find("]", website_status_pos + 29));
        if(website_status_end == std::string::npos)
        {
            return true;
        }

        std::string const new_content(
                      content.substr(0, website_status_pos + 29)
                    + new_website_status
                    + content.substr(website_status_end));

        // it worked, let's save the result
        //
        fc.set_content(new_content);
        if(!fc.write_all())
        {
            // the write back to disk failed
            //
            return true;
        }

        // it worked, make sure apache2 gets restarted
        //
        affected_services.insert("apache2-restart");
        return true;
    }

    return false;
}



void apache2::on_handle_affected_services(std::set<QString> & affected_services)
{
    bool restarted(false);

    auto const & it_restart(std::find(affected_services.begin(), affected_services.end(), QString("apache2-restart")));
    if(it_restart != affected_services.end())
    {
        // remove since we are handling that one here
        //
        affected_services.erase(it_restart);

        // super ugly hack! if this is the current system being updated,
        // then snapmanager.cgi needs a bit of time to finish up... with
        // a small sleep we at least do not get an immediate error (you
        // can if you try to load another right afterward, though...)
        //
        sleep(2);

        // restart apache2
        //
        snap::process p("restart apache2");
        p.set_mode(snap::process::mode_t::PROCESS_MODE_COMMAND);
        p.set_command("systemctl");
        p.add_argument("restart");
        p.add_argument("apache2");
        NOTUSED(p.run());           // errors are automatically logged by snap::process

        restarted = true;
    }

    auto const & it_reload(std::find(affected_services.begin(), affected_services.end(), QString("apache2-reload")));
    if(it_reload != affected_services.end())
    {
        // remove since we are handling that one here
        //
        affected_services.erase(it_reload);

        // do the reload only if we did not already do a restart (otherwise
        // it is going to be useless)
        //
        if(!restarted)
        {
            // reload apache2
            //
            snap::process p("reload apache2");
            p.set_mode(snap::process::mode_t::PROCESS_MODE_COMMAND);
            p.set_command("systemctl");
            p.add_argument("reload");
            p.add_argument("apache2");
            NOTUSED(p.run());           // errors are automatically logged by snap::process
        }
    }
}




SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
