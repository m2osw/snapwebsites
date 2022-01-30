// Copyright (c) 2014-2019  Made to Order Software Corp.  All Rights Reserved
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
#include "listener.h"


// other plugins
//
#include "../messages/messages.h"
#include "../permissions/permissions.h"
#include "../server_access/server_access.h"
#include "../users/users.h"


// snapdev lib
//
#include <snapdev/not_reached.h>
#include <snapdev/not_used.h>


// C++ lib
//
#include <iostream>


// last include
//
#include <snapdev/poison.h>



SNAP_PLUGIN_START(listener, 1, 0)



/** \brief Initialize the listener plugin.
 *
 * This function is used to initialize the listener plugin object.
 */
listener::listener()
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Clean up the listener plugin.
 *
 * Ensure the listener object is clean before it is gone.
 */
listener::~listener()
{
}


/** \brief Get a pointer to the listener plugin.
 *
 * This function returns an instance pointer to the listener plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the listener plugin.
 */
listener * listener::instance()
{
    return g_plugin_listener_factory.instance();
}


/** \brief A path or URI to a logo for this plugin.
 *
 * This function returns a 64x64 icons representing this plugin.
 *
 * \return A path to the logo.
 */
QString listener::icon() const
{
    return "/images/listener/listener-logo-64x64.png";
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
QString listener::description() const
{
    return "Check whether a page or document (when the page represents an"
          " attachment) is ready for consumption. For example, the listener"
          " is used by the editor to listen for attachment upload completion.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString listener::dependencies() const
{
    return "|messages|path|permissions|server_access|users|";
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
int64_t listener::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2017, 5, 6, 23, 30, 30, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Update the database with our content references.
 *
 * Send our content to the database so the system can find us when a
 * user references our pages.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added
 *                                 to the database by this update (in
 *                                 micro-seconds).
 */
void listener::content_update(int64_t variables_timestamp)
{
    snapdev::NOT_USED(variables_timestamp);

    content::content::instance()->add_xml(get_plugin_name());
}


/** \brief Initialize the listener.
 *
 * This function terminates the initialization of the listener plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void listener::bootstrap(snap_child * snap)
{
    f_snap = snap;

    SNAP_LISTEN(listener, "server", server, process_post, boost::placeholders::_1);
}


/** \brief Accept a POST to request information about the server.
 *
 * This function manages the data sent to the server by a client script.
 * In many cases, it is used to know whether something is true or false,
 * although the answer may be any valid text.
 *
 * The function verifies that the "editor_session" variable is set, if
 * not it ignores the POST sine another plugin may be the owner.
 *
 * \note
 * This function is a server signal generated by the snap_child
 * execute() function.
 *
 * \param[in] uri_path  The path received from the HTTP server.
 */
void listener::on_process_post(QString const & uri_path)
{
    // This would impose a huge burden on the server (one session # per
    // page and include that in the resulting HTML) and it does not add
    // to the security. Whether the user is logged in is enough
    // information and that is automatically checked if the current
    // page (uri_path) requires the user to be logged in. However, we
    // do a lot of similar tests on each path being checked (i.e. a path
    // that requires more rights than what the user currently has will
    // get the response: permission denied, so the client code can just
    // drop it.)
    //QString const listener_full_session(f_snap->postenv("_server_access_listener_session"));
    //if(listener_full_session.isEmpty())
    //{
    //    // if the _server_access_listener_session variable does not exist,
    //    // do not consider this POST as a Server Access Listener POST
    //    return;
    //}

    // if no listener size, then it is not a POST for us
    if(!f_snap->postenv_exists("_listener_size"))
    {
        return;
    }

    content::path_info_t ipath;
    ipath.set_path(uri_path);
    ipath.set_main_page(true);
    ipath.force_locale("xx");  // ??

    //messages::messages * messages(messages::messages::instance());
    users::users * users_plugin(users::users::instance());
    permissions::permissions * permissions_plugin(permissions::permissions::instance());
    server_access::server_access * server_access_plugin(server_access::server_access::instance());
    path::path * path_plugin(path::path::instance());

    QString const size_str(f_snap->postenv("_listener_size"));
    bool ok(false);
    int max_uri(size_str.toInt(&ok, 10));
    if(!ok)
    {
        // this number must be a valid decimal, integer; it cannot be empty
        // either which is caught by the toInt() function
        f_snap->die(snap_child::http_code_t::HTTP_CODE_NOT_ACCEPTABLE, "Not Acceptable",
                "The number of URI you are listening to is not a valid integer.",
                QString("Somehow _listener_size is not an integer (%1).").arg(size_str));
        snapdev::NOT_REACHED();
    }
    if(max_uri < 0)
    {
        // this number must be a positive decimal; this is not caught by
        // toInt() [if I may say]
        f_snap->die(snap_child::http_code_t::HTTP_CODE_NOT_ACCEPTABLE, "Not Acceptable",
                "The number of URI you are listening to is negative...",
                QString("Somehow _listener_size is negative (%1 -> %2).").arg(size_str).arg(max_uri));
        snapdev::NOT_REACHED();
    }

    QString const & user_path(users_plugin->get_user_info().get_user_path(false));
    QString const & login_status(permissions_plugin->get_login_status());
    for(int i(0); i < max_uri; ++i)
    {
        QString const name(QString("uri%1").arg(i));

        snap_uri uri(f_snap->postenv(name));
        QString action(uri.query_option(server::server::instance()->get_parameter("qs_action")));
        if(action.isEmpty())
        {
            action = "view";
        }
        content::path_info_t page_ipath;
        page_ipath.set_path(uri.path());

        // can this user access this URI?
        content::permission_flag allowed;
        path_plugin->access_allowed
            ( user_path         // current user
            , page_ipath        // this page
            , action            // can the current user act that way on this page
            , login_status      // the log in status of the currently user
            , allowed           // give me the result here
            );

        QDomDocument doc;
        QDomElement result(doc.createElement("result"));
        doc.appendChild(result);
        result.setAttribute("href", uri.get_original_uri());
        if(allowed.allowed())
        {
            // the user can access this path, check whatever the user is
            // trying to check
            listener_check(uri, page_ipath, doc, result);
            if(result.attribute("status").isEmpty())
            {
                throw listener_exception_status_missing("none of the listeners of the listener_check() signal set a status in the result element");
            }
        }
        else
        {
            // the user is not allowed, reply with a permission error
            QDomElement message(doc.createElement("message"));
            result.appendChild(message);
            QDomText permission_denied(doc.createTextNode("permission denied"));
            message.appendChild(permission_denied);
            result.setAttribute("status", "failed");
        }
        server_access_plugin->ajax_append_data("listener", doc.toString(-1).toUtf8());
    }

    server_access_plugin->create_ajax_result(ipath, true);
    server_access_plugin->ajax_output();
}


bool listener::listener_check_impl(snap_uri const & uri, content::path_info_t & page_ipath, QDomDocument doc, QDomElement result)
{
    snapdev::NOT_USED(uri, page_ipath, doc, result);

    // TODO: add handling of all the plugins that cannot include the listener
    //       if any requires it (users? to test when we get logged out?)

    return true;
}


SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
