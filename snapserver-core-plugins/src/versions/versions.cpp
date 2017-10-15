// Snap Websites Server -- generate versions of all the parts used by snap
// Copyright (C) 2011-2017  Made to Order Software Corp.
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

#include "./versions.h"

#include "../users/users.h"
#include "../permissions/permissions.h"

#include <snapwebsites/log.h>
#include <snapwebsites/not_reached.h>
#include <snapwebsites/not_used.h>
#include <snapwebsites/process.h>

#include <csspp/csspp.h>
#include <libtld/tld.h>
#include <QtSerialization/QSerialization.h>

#include <iostream>

#include <snapwebsites/poison.h>


SNAP_PLUGIN_START(versions, 1, 0)


/* \brief Get a fixed versions name.
 *
 * The versions plugin makes use of different names in the database. This
 * function ensures that you get the right spelling for a given name.
 *
 * \param[in] name  The name to retrieve.
 *
 * \return A pointer to the name.
 */
char const * get_name(name_t name)
{
    switch(name)
    {
    case name_t::SNAP_NAME_VERSIONS_VERSION:
        return "versions::version";

    default:
        // invalid index
        throw snap_logic_exception("invalid name_t::SNAP_NAME_VERSIONS_...");

    }
    NOTREACHED();
}









/** \brief Initialize the versions plugin.
 *
 * This function is used to initialize the versions plugin object.
 */
versions::versions()
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Clean up the versions plugin.
 *
 * Ensure the versions object is clean before it is gone.
 */
versions::~versions()
{
}


/** \brief Get a pointer to the versions plugin.
 *
 * This function returns an instance pointer to the versions plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the versions plugin.
 */
versions * versions::instance()
{
    return g_plugin_versions_factory.instance();
}


/** \brief Send users to the plugin settings.
 *
 * This path represents this plugin settings.
 */
QString versions::settings_path() const
{
    return "/admin/versions";
}


/** \brief A path or URI to a logo for this plugin.
 *
 * This function returns a 64x64 icons representing this plugin.
 *
 * \return A path to the logo.
 */
QString versions::icon() const
{
    return "/images/versions/versions-logo-64x64.png";
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
QString versions::description() const
{
    return "The versions plugin displays the version of all the parts used"
          " by Snap! The parts include the main snap library, the plugins,"
          " and all the tools that the server may use. It is a filter so it"
          " can be displayed on any page where the filter is allowed.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString versions::dependencies() const
{
    return "|content|filter|permissions|users|";
}


/** \brief Check whether updates are necessary.
 *
 * This function updates the database when a newer version is installed
 * and the corresponding updates where not run.
 *
 * This works for newly installed plugins and older plugins that were
 * updated.
 *
 * \param[in] last_updated  The UTC Unix date when the website was last updated (in micro seconds).
 *
 * \return The UTC Unix date of the last update of this plugin.
 */
int64_t versions::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2016, 1, 16, 22, 52, 51, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Update the database with our content references.
 *
 * Send our content to the database so the system can find us when a
 * user references our pages.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added
 *                        to the database by this update (in micro-seconds).
 */
void versions::content_update(int64_t variables_timestamp)
{
    NOTUSED(variables_timestamp);

    content::content::instance()->add_xml(get_plugin_name());
}


/** \brief Initialize the versions.
 *
 * This function terminates the initialization of the versions plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void versions::bootstrap(snap_child * snap)
{
    f_snap = snap;

    SNAP_LISTEN(versions, "filter", filter::filter, replace_token, _1, _2, _3);
    SNAP_LISTEN(versions, "filter", filter::filter, token_help, _1);
}



void versions::on_replace_token(content::path_info_t & ipath, QDomDocument & xml, filter::filter::token_info_t & token)
{
    NOTUSED(ipath);
    NOTUSED(xml);

    users::users * users_plugin(users::users::instance());
    users::users::user_info_t user_info( users_plugin->get_user_info() );
    QString const user_path(user_info.get_user_path(false));
    if(token.is_token("versions::versions")
    && user_path != users::get_name(users::name_t::SNAP_NAME_USERS_ANONYMOUS_PATH))
    {
        content::path_info_t page_ipath;
        page_ipath.set_path("admin/versions");
        permissions::permissions * permissions_plugin(permissions::permissions::instance());
        QString const & login_status(permissions_plugin->get_login_status());
        content::permission_flag allowed;
        path::path * path_plugin(path::path::instance());
        path_plugin->access_allowed
            ( user_path         // current user
            , page_ipath        // this page
            , "administer"      // can the current user act that way on this page
            , login_status      // the log in status of the currently user
            , allowed           // give me the result here
            );
        if(allowed.allowed())
        {
            // okay, this user is really allowed so generate the versions

            // first show this Core release version, this is the GIT branch
            // which cmake generates on the g++ command line
            //
            // TODO: This does not work yet. Not too sure how to properly
            //       get the GIT branch yet.
            //
            //token.f_replacement += "<h3>Release</h3><ul><li>The Core Release Version is <strong>"
            //                       BOOST_STRINGIZE(GIT_BRANCH)
            //                       "</strong></li></ul>";

            // libraries
            //
            versions_libraries(token);

            // Plugin Versions
            //
            token.f_replacement += "<h3>Plugins</h3><ul>";
            plugins::plugin_map_t const & list_of_plugins(plugins::get_plugin_list());
            for(plugins::plugin_map_t::const_iterator it(list_of_plugins.begin());
                                              it != list_of_plugins.end();
                                              ++it)
            {
                token.f_replacement += QString("<li>%1 v%2.%3</li>")
                        .arg((*it)->get_plugin_name())
                        .arg((*it)->get_major_version())
                        .arg((*it)->get_minor_version());
            }
            token.f_replacement += "</ul>";

            // Tools
            //
            versions_tools(token);
        }
    }
}


void versions::on_token_help(filter::filter::token_help_t & help)
{
    help.add_token("versions::versions",
        "Display the version of all the libraries, plugins, and tools used by this installation of Snap!");
}


bool versions::versions_libraries_impl(filter::filter::token_info_t & token)
{
    token.f_replacement += "<h3>Libraries</h3><ul>";

        // Snap! Server
    token.f_replacement += "<li>snapwebsite v";
    token.f_replacement += f_snap->get_running_server_version();
    token.f_replacement += " (compiled with " SNAPWEBSITES_VERSION_STRING ")</li>";
        // CGI
    token.f_replacement += "<li>Apache interface: ";
    token.f_replacement += f_snap->snapenv("GATEWAY_INTERFACE");
    token.f_replacement += "</li>";
        // Qt
    token.f_replacement += "<li>Qt v";
    token.f_replacement += qVersion();
    token.f_replacement += " (compiled with " QT_VERSION_STR ")</li>";
        // libQtCassandra
    token.f_replacement += "<li>libQtCassandra v";
    token.f_replacement += QtCassandra::QCassandra::version();
    token.f_replacement += " (compiled with ";
    token.f_replacement += QtCassandra::QT_CASSANDRA_LIBRARY_VERSION_STRING;
    token.f_replacement += ")</li>";
        // libQtSerialization
    token.f_replacement += "<li>libQtSerialization v";
    token.f_replacement += QtSerialization::QLibraryVersion();
    token.f_replacement += " (compiled with ";
    token.f_replacement += QtSerialization::QT_SERIALIZATION_LIBRARY_VERSION_STRING;
    token.f_replacement += ")</li>";
        // libtld
    token.f_replacement += "<li>libtld v";
    token.f_replacement += tld_version();
    token.f_replacement += " (compiled with " LIBTLD_VERSION ")</li>";
        // libcsspp (content is always included and cannot listen for on_versions_libraries())
    token.f_replacement += "<li>libcsspp v";
    token.f_replacement += csspp::csspp_library_version();
    token.f_replacement += " (compiled with " CSSPP_VERSION ")</li>";

    return true;
}


void versions::versions_libraries_done(filter::filter::token_info_t & token)
{
    token.f_replacement += "</ul>";
}


bool versions::versions_tools_impl(filter::filter::token_info_t & token)
{
    token.f_replacement += "<h3>Tools</h3><ul>";

        // iplock
    if(access("/usr/sbin/iplock", X_OK))
    {
        token.f_replacement += "<li>iplock ";
        {
            process p("check iplock version");
            p.set_mode(process::mode_t::PROCESS_MODE_OUTPUT);
            p.set_command("iplock");
            p.add_argument("--version");
            p.run();
            token.f_replacement += p.get_output();
        }
        token.f_replacement += "</li>";
    }

        // snapdb
    if(access("/usr/bin/snapdb", X_OK))
    {
        token.f_replacement += "<li>snapdb ";
        {
            process p("check snapdb version");
            p.set_mode(process::mode_t::PROCESS_MODE_OUTPUT);
            p.set_command("/usr/bin/snapdb");
            p.add_argument("--version");
            p.run();
            token.f_replacement += p.get_output();
        }
        token.f_replacement += "</li>";
    }

        // snapbounce
    if(access("/usr/bin/snapbounce", X_OK))
    {
        token.f_replacement += "<li>snapbounce ";
        {
            process p("check snapbounce version");
            p.set_mode(process::mode_t::PROCESS_MODE_OUTPUT);
            p.set_command("/usr/bin/snapbounce");
            p.add_argument("--version");
            p.run();
            token.f_replacement += p.get_output();
        }
        token.f_replacement += "</li>";
    }

        // snapdbproxy
    if(access("/usr/bin/snapdbproxy", X_OK))
    {
        token.f_replacement += "<li>snapdbproxy ";
        {
            process p("check snapdbproxy version");
            p.set_mode(process::mode_t::PROCESS_MODE_OUTPUT);
            p.set_command("/usr/bin/snapdbproxy");
            p.add_argument("--version");
            p.run();
            token.f_replacement += p.get_output();
        }
        token.f_replacement += "</li>";
    }

        // snapcommunicator
    if(access("/usr/bin/snapcommunicator", X_OK))
    {
        token.f_replacement += "<li>snapcommunicator ";
        {
            process p("check snapcommunicator version");
            p.set_mode(process::mode_t::PROCESS_MODE_OUTPUT);
            p.set_command("/usr/bin/snapcommunicator");
            p.add_argument("--version");
            p.run();
            token.f_replacement += p.get_output();
        }
        token.f_replacement += "</li>";
    }

        // snapfirewall
    if(access("/usr/bin/snapfirewall", X_OK))
    {
        token.f_replacement += "<li>snapfirewall ";
        {
            process p("check snapfirewall version");
            p.set_mode(process::mode_t::PROCESS_MODE_OUTPUT);
            p.set_command("/usr/bin/snapfirewall");
            p.add_argument("--version");
            p.run();
            token.f_replacement += p.get_output();
        }
        token.f_replacement += "</li>";
    }

        // snaplayout
    if(access("/usr/bin/snaplayout", X_OK))
    {
        token.f_replacement += "<li>snaplayout ";
        {
            process p("check snaplayout version");
            p.set_mode(process::mode_t::PROCESS_MODE_OUTPUT);
            p.set_command("/usr/bin/snaplayout");
            p.add_argument("--version");
            p.run();
            token.f_replacement += p.get_output();
        }
        token.f_replacement += "</li>";
    }

        // snaplock
    if(access("/usr/bin/snaplock", X_OK))
    {
        token.f_replacement += "<li>snaplock ";
        {
            process p("check snaplock version");
            p.set_mode(process::mode_t::PROCESS_MODE_OUTPUT);
            p.set_command("/usr/bin/snaplock");
            p.add_argument("--version");
            p.run();
            token.f_replacement += p.get_output();
        }
        token.f_replacement += "</li>";
    }

        // snapwatchdogserver
        //
        // TODO: until the snapwatchdogserver offers a snapserver-plugin
        //       which does that itself
        //
    if(access("/usr/bin/snapwatchdogserver", X_OK))
    {
        token.f_replacement += "<li>snapwatchdog ";
        {
            process p("check snapwatchdogserver version");
            p.set_mode(process::mode_t::PROCESS_MODE_OUTPUT);
            p.set_command("/usr/bin/snapwatchdogserver");
            p.add_argument("--version");
            p.run();
            token.f_replacement += p.get_output();
        }
        token.f_replacement += "</li>";
    }

// 1. these two binaries moved
// 2. they do not support --version anymore (See SNAP-509)
//
//        // snap.cgi
//    if(access("/usr/lib/cgi-bin/snap.cgi", X_OK))
//    {
//        token.f_replacement += "<li>snap.cgi ";
//        {
//            process p("check snap.cgi version");
//            p.set_mode(process::mode_t::PROCESS_MODE_OUTPUT);
//            p.set_command("/usr/lib/cgi-bin/snap.cgi");
//            p.add_argument("--version");
//            p.run();
//            token.f_replacement += p.get_output();
//        }
//        token.f_replacement += "</li>";
//    }
//
//        // snapmanager.cgi
//    if(access("/usr/lib/cgi-bin/snapmanager.cgi", X_OK))
//    {
//        token.f_replacement += "<li>snapmanager.cgi ";
//        {
//            process p("check snapmanager.cgi version");
//            p.set_mode(process::mode_t::PROCESS_MODE_OUTPUT);
//            p.set_command("/usr/lib/cgi-bin/snapmanager.cgi");
//            p.add_argument("--version");
//            p.run();
//            token.f_replacement += p.get_output();
//        }
//        token.f_replacement += "</li>";
//    }

    return true;
}


void versions::versions_tools_done(filter::filter::token_info_t & token)
{
    token.f_replacement += "</ul>";
}



SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
