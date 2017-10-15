// Snap Websites Server -- path handling
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

#include "path.h"

#include "../links/links.h"
#include "../messages/messages.h"
#include "../server_access/server_access.h"

#include <snapwebsites/log.h>
#include <snapwebsites/not_reached.h>
#include <snapwebsites/not_used.h>
#include <snapwebsites/qdomhelpers.h>
#include <snapwebsites/qstring_stream.h>
#include <snapwebsites/snap_uri.h>

#include <iostream>

#include <snapwebsites/poison.h>


SNAP_PLUGIN_START(path, 1, 0)

/* \brief Get a fixed path name.
 *
 * The path plugin makes use of different names in the database. This
 * function ensures that you get the right spelling for a given name.
 *
 * \param[in] name  The name to retrieve.
 *
 * \return A pointer to the name.
 */
//char const * get_name(name_t name)
//{
//    // Note: <branch>.<revision> are actually replaced by a full version
//    //       when dealing with JavaScript and CSS files (Version: field)
//    switch(name)
//    {
//    case name_t::SNAP_NAME_CONTENT_PRIMARY_OWNER:
//        return "path::primary_owner";
//
//    default:
//        // invalid index
//        throw snap_logic_exception("invalid name_t::SNAP_NAME_PATH_...");
//
//    }
//    NOTREACHED();
//}




path_error_callback::path_error_callback(snap_child * snap, content::path_info_t & ipath)
    : f_snap(snap)
    , f_ipath(ipath)
    //, f_plugin(nullptr)
    //, f_autologout(false)
{
}


void path_error_callback::set_plugin(plugins::plugin * p)
{
    f_plugin = p;
}


void path_error_callback::set_autologout(bool autologout)
{
    f_autologout = autologout;
}


void path_error_callback::on_error(
              snap_child::http_code_t err_code
            , QString const & err_name
            , QString const & err_description
            , QString const & err_details
            , bool const err_by_mime_type)
{
    // first check whether we are handling an AJAX request
    //
    server_access::server_access * server_access_plugin(server_access::server_access::instance());
    if(server_access_plugin->is_ajax_request())
    {
//std::cerr << "***\n*** PATH Permission denied, but we can ask user for credentials with a redirect...\n***\n";
        messages::messages::instance()->set_error(err_name, err_description, err_details, false);
        QString const err_code_string(QString("%1").arg(static_cast<int>(err_code)));
        server_access_plugin->ajax_append_data("error-code", err_code_string.toUtf8());
        server_access_plugin->create_ajax_result(f_ipath, false);
        server_access_plugin->ajax_output();
        f_snap->output_result(snap_child::HEADER_MODE_ERROR, f_snap->get_output());
        f_snap->exit(0);
        NOTREACHED();
    }

    // give a chance to other plugins to handle the error
    // (Especially the attachment plugin when "weird" data was requested)
    //
    if(err_by_mime_type && f_plugin != nullptr)
    {
        // will this plugin handle that error?
        //
        permission_error_callback::error_by_mime_type * handle_error(dynamic_cast<permission_error_callback::error_by_mime_type *>(f_plugin));
        if(handle_error)
        {
            // attempt to inform the user using the proper type of data
            // so that way it is easier to debug than sending HTML
            //
            try
            {
                // define a default error name if undefined
                //
                QString http_name;
                f_snap->define_http_name(err_code, http_name);

                // log the error
                //
                SNAP_LOG_FATAL("path::on_error(): ")(err_details)(" (")(static_cast<int>(err_code))(" ")(err_name)(": ")(err_description)(")");

                // On error we do not return the HTTP protocol, only the Status field
                // it just needs to be first to make sure it works right
                //
                f_snap->set_header("Status", QString("%1 %2\n")
                        .arg(static_cast<int>(err_code))
                        .arg(http_name));

                // content type has to be defined by the handler, also
                // the output auto-generated
                //
                //f_snap->set_header(get_name(snap::name_t::SNAP_NAME_CORE_CONTENT_TYPE_HEADER), "text/html; charset=utf8", HEADER_MODE_EVERYWHERE);
                //f_snap->output_result(HEADER_MODE_ERROR, html.toUtf8());
                handle_error->on_handle_error_by_mime_type(err_code, err_name, err_description, f_ipath.get_key());
            }
            catch(...)
            {
                // ignore all errors because at this point we must die quickly.
                //
                SNAP_LOG_FATAL("path.cpp:on_error(): try/catch caught an exception");
            }

            // exit with an error
            //
            f_snap->exit(1);
            NOTREACHED();
        }
    }
    f_snap->die(err_code, err_name, err_description, err_details);
    NOTREACHED();
}


void path_error_callback::on_redirect(
        /* messages::set_error() */ QString const & err_name, QString const & err_description, QString const & err_details, bool err_security,
        /* snap_child::page_redirect() */ QString const & path, snap_child::http_code_t const http_code)
{
    // TODO: remove this message dependency
    server_access::server_access * server_access_plugin(server_access::server_access::instance());
    if(server_access_plugin->is_ajax_request())
    {
        // Since the user sent an AJAX request, we have to reply with
        // an AJAX answer; however, we CANNOT send an AJAX redirect
        // when sending an error back to the client... so we actually
        // use set_warning() instead of set_error().
        //
//std::cerr << "***\n*** PATH Permission denied, but we can ask user for credentials with a redirect...\n***\n";
        if(!err_security)
        {
            messages::messages::instance()->set_warning(err_name, err_description, err_details);
        }
        else
        {
            // we cannot generate a warning with a secure error message...
            // we just log it for now.
            SNAP_LOG_FATAL(logging::log_security_t::LOG_SECURITY_SECURE)
                    ("path::on_redirect(): ")(err_details)(" (")
                                (err_name)(": ")(err_description)(")");
            // we still generate a warning so the end users has a chance
            // to see something at some point
            messages::messages::instance()->set_warning("An Error Occurred", "An unspecified error occurred.", "Please check your secure log for more information.");
        }

        // we are about to die without calling the die() or page_redirect()
        // functions so we need to call the attach_to_session() function
        // explicitly
        //
        server::server::instance()->attach_to_session();

        server_access_plugin->create_ajax_result(f_ipath, true);
        server_access_plugin->ajax_redirect(QString("/%1").arg(path), "_top");
        server_access_plugin->ajax_output();
        f_snap->output_result(snap_child::HEADER_MODE_REDIRECT, f_snap->get_output());
        f_snap->exit(0);
    }
    else
    {
        if(f_autologout)
        {
            // an auto-logout is not an error
            //
            messages::messages::instance()->set_info(err_name, err_description);
        }
        else
        {
            messages::messages::instance()->set_error(err_name, err_description, err_details, err_security);
        }
        f_snap->page_redirect(path, http_code, err_description, err_details);
    }
    NOTREACHED();
}



/** \brief Called by plugins that can handle dynamic paths.
 *
 * Some plugins handle a very large number of paths in a fully
 * dynamic manner, which means that they can generate the data
 * for any one of those paths in a way that is extremely fast
 * without the need of creating millions of entries in the
 * database.
 *
 * These plugins are given a chance to handle a path whenever
 * the content plugin calls the can_handle_dynamic_path() signal.
 * At that point, a plugin can respond by calling this function
 * with itself.
 *
 * For example, a plugin that displays a date in different formats
 * could be programmed to understand the special path:
 *
 * \code
 * /formatted-date/YYYYMMDD/FMT
 * \endcode
 *
 * which could be a request to the system to format the date
 * YYYY-MM-DD using format FMT.
 *
 * \param[in] p  The plugin that can handle the path specified in the signal.
 */
void dynamic_plugin_t::set_plugin(plugins::plugin * p)
{
//std::cerr << "handle_dynamic_path(" << p->get_plugin_name() << ")\n";
    if(f_plugin != nullptr)
    {
        // two different plugins are fighting for the same path
        // we'll have to enhance our error to give the user a way to choose
        // the plugin one wants to use for this request...
        content::content::instance()->get_snap()->die(snap_child::http_code_t::HTTP_CODE_MULTIPLE_CHOICE,
                "Multiple Choices",
                "This page references multiple plugins and the server does not currently have means of choosing one over the other.",
                QString("User tried to access dynamic page but more than one plugin says it owns the resource, primary is \"%1\", second request by \"%2\"")
                        .arg(f_plugin->get_plugin_name()).arg(p->get_plugin_name()));
        NOTREACHED();
    }

    f_plugin = p;
}


/** \brief Tell the system that a fallback exists for this path.
 *
 * Some plugins may understand a path even if not an exact match as
 * otherwise expected by the system.
 *
 * For example, the attachment plugin understands all of the following
 * even though the only file that really exists in the database is
 * "jquery.js":
 *
 * \li jquery.js.gz
 * \li jquery.min.js
 * \li jquery.min.js.gz
 * \li jquery-1.2.3.js
 * \li jquery-1.2.3.js.gz
 * \li jquery-1.2.3.min.js
 * \li jquery-1.2.3.min.js.gz
 *
 * Type of filenames that we support in core:
 *
 * \li Compressions: .gz, .bz2, .xz, ...
 * \li Minified: .min.js, .min.css
 * \li Resized: -32x32.png, -64x64.jpg, ...
 * \li Cropped: -32x32+64+64.png
 * \li Black and White: -bw.png, -bw.jpg, ...
 * \li Converted: file is .pdf, user gets a .png ...
 * \li Book: .pdf on the root page of a book tree
 *
 * \param[in] p  The plugin that understands the path.
 * \param[in] cpath  The path replacing the one used by the user.
 */
void dynamic_plugin_t::set_plugin_if_renamed(plugins::plugin * p, QString const & cpath)
{
    if(f_plugin_if_renamed != nullptr)
    {
        // in this case we really cannot handle the path properly...
        // I'm not too sure how we can resolve the problem because we
        // cannot be sure in which order the plugins will be executing
        // the tests...
        content::content::instance()->get_snap()->die(snap_child::http_code_t::HTTP_CODE_MULTIPLE_CHOICE,
                    "Multiple Choices",
                    "This page references multiple plugins if the path is renamed and the server does not currently have means of choosing one over the other.",
                    QString("User tried to access dynamic page, but more than one plugin says it can handle it: primary \"%2\", second request \"%3\".")
                            .arg(f_plugin_if_renamed->get_plugin_name()).arg(p->get_plugin_name()));
        NOTREACHED();
    }

    f_plugin_if_renamed = p;
    f_cpath_renamed = cpath;
}


/** \brief Initialize the path plugin.
 *
 * This function initializes the path plugin.
 */
path::path()
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Destroy the path plugin.
 *
 * This function cleans up the path plugin.
 */
path::~path()
{
}


/** \brief Get a pointer to the path plugin.
 *
 * This function returns an instance pointer to the path plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the path plugin.
 */
path * path::instance()
{
    return g_plugin_path_factory.instance();
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
QString path::description() const
{
    return "This plugin manages the path to a page. This is used to determine"
        " the plugin that knows how to handle the data displayed to the user"
        " when given a specific path.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString path::dependencies() const
{
    return "|content|links|messages|server_access|";
}


/** \brief Bootstrap the path.
 *
 * This function adds the events the path plugin is listening for.
 *
 * \param[in] snap  The child handling this request.
 */
void path::bootstrap(::snap::snap_child * snap)
{
    f_snap = snap;

    SNAP_LISTEN(path, "server", server, execute, _1);
    SNAP_LISTEN(path, "server", server, improve_signature, _1, _2, _3);
}


/** \brief Retrieve the plugin corresponding to a path.
 *
 * This function searches for the plugin that is to be used to handle the
 * given path.
 *
 * \param[in,out] ipath  The path to be probed.
 * \param[in,out] err_callback  Interface implementation to call on errors.
 *
 * \return A pointer to the plugin that owns this path.
 */
plugins::plugin * path::get_plugin(content::path_info_t & ipath, permission_error_callback & err_callback)
{
    QtCassandra::QCassandraTable::pointer_t content_table(content::content::instance()->get_content_table());

    // get the name of the plugin that owns this URL
    plugins::plugin * owner_plugin(nullptr);

    QString const primary_owner(content::get_name(content::name_t::SNAP_NAME_CONTENT_PRIMARY_OWNER));

    // define the primary owner
    if(content_table->exists(ipath.get_key())
    && content_table->row(ipath.get_key())->exists(primary_owner))
    //&& content_table->row(ipath.get_key())->exists(content::get_name(content::name_t::SNAP_NAME_CONTENT_STATUS)))
    {
//SNAP_LOG_TRACE("found path ")(ipath.get_key())(" in database... no dynamic stuff.");
        QString const action(define_action(ipath));

        // I do not think this is smart, instead I pass the action to the
        // on_path_execute() function (within the ipath, really) which
        // has to react accordingly...
        // (That way a plugin may completely forbid a delete, for example.)
        //
        // That being said, it probably should use the action to determine
        // the plugin that understands that action, but I think the
        // implementation shown below is incorrect because we probably don't
        // want that information to be saved in every single page... (i.e.
        // old pages would miss the information of a new action and also
        // that would many many more fields which in most cases would
        // probably not be useful)
        //
        // we may have a specific owner with a specific action
        // (TBD: we may want a signal on that one and have a dynamic owner)
        //
        //QString const owner_with_action(QString("%1::%2").arg(primary_owner).arg(action));
        //if(content_table->row(ipath.get_key())->exists(owner_with_action))
        //{
        //    owner = content_table->row(ipath.get_key())->cell(owner_with_action)->value().stringValue();
        //}
        //else if(content_table->row(ipath.get_key())->exists(primary_owner))
        //{
        //    owner = content_table->row(ipath.get_key())->cell(primary_owner)->value().stringValue();
        //}

        // verify that the status is good for displaying this page
        content::path_info_t::status_t status(ipath.get_status());
        switch(status.get_state())
        {
        case content::path_info_t::status_t::state_t::CREATE:
            err_callback.on_error(snap_child::http_code_t::HTTP_CODE_LOCKED,
                        "Page Locked",
                        "This page is currently locked. You may try again at a later time.",
                        QString("User tried to access page \"%1\" but its status state is CREATE.")
                                .arg(ipath.get_key()),
                        false);
            return nullptr;

        case content::path_info_t::status_t::state_t::UNKNOWN_STATE:
            // TBD: should we throw instead when unknown (because get_state()
            //      is not expected to ever return that value)
            err_callback.on_error(snap_child::http_code_t::HTTP_CODE_NOT_FOUND,
                        "Unknow Page Status",
                        "An internal error occured and this page cannot properly be displayed at this time.",
                        QString("User tried to access page \"%1\" but its status state is %2.")
                                .arg(ipath.get_key())
                                .arg(static_cast<int>(status.get_state())),
                        false);
            return nullptr;

        case content::path_info_t::status_t::state_t::NORMAL:
        case content::path_info_t::status_t::state_t::HIDDEN:   // TBD -- probably requires special handling to know whether we can show those pages
        case content::path_info_t::status_t::state_t::MOVED:    // MOVED pages will redirect a little later (if allowed)
        case content::path_info_t::status_t::state_t::DELETED:  // DELETED pages are handled below, after we determined the plugin
            break;

        }

        // get the modified date so we can setup the Last-Modified HTTP header field
        // it is also another way to determine that a path is valid
        QtCassandra::QCassandraValue const value(content_table->row(ipath.get_key())->cell(content::get_name(content::name_t::SNAP_NAME_CONTENT_CREATED))->value());
        QString const owner(content_table->row(ipath.get_key())->cell(primary_owner)->value().stringValue());
        if(value.nullValue() || owner.isEmpty())
        {
            err_callback.on_error(snap_child::http_code_t::HTTP_CODE_NOT_FOUND,
                        "Invalid Page",
                        "An internal error occured and this page cannot properly be displayed at this time.",
                        QString("User tried to access page \"%1\" but it does not look valid (null value? %2, empty owner? %3)")
                                .arg(ipath.get_key())
                                .arg(static_cast<int>(value.nullValue()))
                                .arg(static_cast<int>(owner.isEmpty())),
                        false);
            return nullptr;
        }
        // TODO: this is not correct anymore! (we're getting the creation
        //       date, not last mod.)
        //
        //       only we probably need to get the last modification date
        //       from the last revision...
        //
        f_last_modified = value.int64Value();

        // retrieve the plugin pointer
#ifdef DEBUG
        SNAP_LOG_TRACE("path::get_plugin() cpath=")(ipath.get_cpath());
        SNAP_LOG_TRACE("   action=")(action);
        SNAP_LOG_TRACE("   execute [")(ipath.get_key())("] with plugin [")(owner)("]");
#endif
//std::cerr << "Execute [" << ipath.get_key() << "] with plugin [" << owner << "]\n";
        owner_plugin = plugins::get_plugin(owner);
        if(owner_plugin == nullptr)
        {
            // if the plugin cannot be found then either it was mispelled
            // or the plugin is not currently installed...
            //
            err_callback.on_error(snap_child::http_code_t::HTTP_CODE_NOT_FOUND,
                        "Plugin Missing",
                        "This page is not currently available as its plugin is not currently installed.",
                        QString("User tried to access page \"%1\" but its plugin (%2) does not exist (not installed? mispelled?)")
                                .arg(ipath.get_cpath())
                                .arg(owner),
                        false);
            return nullptr;
        }

        if(status.get_state() == content::path_info_t::status_t::state_t::DELETED)
        {
            // TODO: these are rather complicated business rules, which
            //       may need to be somewhere else than the get_plugin()
            //       function (?)
            //
            // See: http://webmasters.stackexchange.com/questions/42252/whats-the-best-http-code-for-dynamically-deleted-pages
            // According to that question/answer, the best practice is:
            //
            //        404 -- may come back one day
            //        410 -- gone "forever"
            //        301 or 308 -- moved permanently (see MOVED below)
            //
            // TODO: for administrators who can undelete pages, the DELETED
            //       status will need special handling at some point
            //
            // if the action is "delete" then we reply positively that the
            // page was deleted;

            // got a valid plugin, check whether this user could restore
            // the page because if so we want to offer a button for that
            // purpose; otherwise we just return a 2XX answer
            //
            quiet_error_callback restore_error_callback(f_snap, true);
            // the quiet_error_callback does not offer a plugin pointer
            //path_error_callback * pec(dynamic_cast<path_error_callback *>(&restore_error_callback));
            //if(pec)
            //{
            //    pec->set_plugin(owner_plugin);
            //}
            if(action == "delete")
            {
                // user was trying to delete the page...
                verify_permissions(ipath, restore_error_callback);
                if(restore_error_callback.has_error())
                {
                    // user does not have permission to delete the page
                    // we return a 403
                    err_callback.on_error(snap_child::http_code_t::HTTP_CODE_FORBIDDEN,
                                "Action Forbidden",
                                "You are not permitted to delete this page.",
                                QString("User tried to delete page \"%1\" but has no such permission (even though the page is already deleted!).")
                                        .arg(ipath.get_key()),
                                false);
                    return nullptr;
                }

                // check whether the restore is valid for the link
                add_restore_link_to_signature_for(ipath.get_key());

                // TODO: this result is positive but will not be caught
                //       by the AJAX process which is a problem since
                //       we could end up sending HTML instead of a quick
                //       XML response.
                //
                //       Also, the error code should most certainly be
                //       a 404, even if we have a link saying "Restore Page"
                //
                err_callback.on_error(snap_child::http_code_t::HTTP_CODE_OK,
                            "Page Deleted",
                            "This page was deleted.",
                            QString("User accessed already deleted page \"%1\" with action \"%2\".")
                                    .arg(ipath.get_key())
                                    .arg(action),
                            false);
                return nullptr;
            }

            // force the action to "restore" to test permission and see
            // whether the user could restore this page
            ipath.set_parameter("action", "restore");
            verify_permissions(ipath, restore_error_callback);
            if(restore_error_callback.has_error())
            {
                // restore is not allowed for that user so the error is a
                // simple 404 (i.e. search engines would see this page)
                err_callback.on_error(snap_child::http_code_t::HTTP_CODE_NOT_FOUND,
                            "Page Not Found",
                            "This page does not exist on this website.",
                            QString("User tried to access deleted page \"%1\" but has no such permission.")
                                    .arg(ipath.get_key()),
                            false);
                return nullptr;
            }

            // only administrators come this far

            if(action != "restore")
            {
                // action is not restore and the page is deleted so the only
                // thing we can show the user is an error with a button
                // offering him/her to restore the page
                //
                // The restore will appear as a link in the signature
                f_add_restore_link_to_signature.push_back(ipath.get_cpath());

                err_callback.on_error(snap_child::http_code_t::HTTP_CODE_GONE,
                            "Page Was Deleted",
                            "This page was deleted. There is a link below you can click to restore it. Until then, it will appear as a \"Page Not Found\" to users who do not have permission to restore it.",
                            QString("User accessed deleted page \"%1\" with action \"%2\".")
                                    .arg(ipath.get_key())
                                    .arg(action),
                            false);
                return nullptr;
            }

            // user is trying to restore a page and he has such a
            // permission so let him do so
            //
            // just not implemented yet...
            err_callback.on_error(snap_child::http_code_t::HTTP_CODE_NOT_IMPLEMENTED,
                        "Restore Not Implemented",
                        "This page was deleted and could be restored once that functionality gets implemented.",
                        QString("User tried to restore deleted page \"%1\", which is a function to be implemented still.")
                                .arg(ipath.get_key()),
                        false);
            return nullptr;
        }
    }
    else
    {
        // this key does not exist as is in the database, but...
        // it may be a dynamically defined path, check for a
        // plugin that would have defined such a path
        dynamic_plugin_t dp;
        can_handle_dynamic_path(ipath, dp);

        owner_plugin = dp.get_plugin();
//SNAP_LOG_TRACE("Testing for page dynamically [")(ipath.get_cpath())("] -> ")(owner_plugin ? owner_plugin->get_plugin_name() : "no plugin found");
        if(owner_plugin == nullptr)
        {
            // a plugin (such as the attachment, images, or search plugins)
            // may take care of this path by renaming it
            owner_plugin = dp.get_plugin_if_renamed();
            if(owner_plugin != nullptr)
            {
                ipath.set_parameter("renamed_path", dp.get_renamed_path());
            }
        }
    }

    if(owner_plugin != nullptr)
    {
        // got a valid plugin, verify that the user has permission
        //
        path_error_callback * pec(dynamic_cast<path_error_callback *>(&err_callback));
        if(pec)
        {
            pec->set_plugin(owner_plugin);
        }
        verify_permissions(ipath, err_callback);
    }

    return owner_plugin;
}


void path::add_restore_link_to_signature_for(QString const page_path)
{
    content::path_info_t ipath;

    // verify that the user could restore that page
    ipath.set_path(page_path);
    ipath.set_parameter("action", "restore");
    quiet_error_callback restore_error_callback(f_snap, true);
    verify_permissions(ipath, restore_error_callback);
    if(!restore_error_callback.has_error())
    {
        f_add_restore_link_to_signature.push_back(ipath.get_cpath());
    }
}


/** \brief Verify for permissions.
 *
 * This function calculates the permissions of the user to access the
 * specified path with the specified action. If the result is that the
 * current user does not have permission to access the page, then the
 * function checks whether the user is logged in. If not, he gets
 * sent to the log in page after saving the current path as the place
 * to come back after logging in. If the user is already logged in,
 * then an Access Denied error is generated.
 *
 * \param[in,out] ipath  The path which permissions are being checked.
 * \param[in,out] err_callback  An object with on_error() and on_redirect()
 *                              functions.
 */
void path::verify_permissions(content::path_info_t & ipath, permission_error_callback & err_callback)
{
    QString const action(define_action(ipath));

    SNAP_LOG_TRACE("verify_permissions(): ipath=")(ipath.get_key())(", action=")(action);

    // only actions that are defined in the permission types are
    // allowed, anything else is funky action from a hacker or
    // whatnot and we just die with an error in that case
    validate_action(ipath, action, err_callback);
}


/** \brief Check whether a user has permission to access a page.
 *
 * This event is sent to all plugins that want to check for permissions.
 * In general, just the permissions plugin does that work, but other
 * plugins can also check. The result is true by default and if any
 * plugin decides that the page is not accessible, the result is set
 * to false. A plugin is not allowed to set the flag back to true.
 *
 * \param[in] user_path  The path to the user being checked.
 * \param[in,out] path  The path being checked.
 * \param[in] action  The action being checked.
 * \param[in] login_status  The status the user is in.
 * \param[in,out] result  The returned result.
 *
 * \return true if the signal should be propagated.
 */
bool path::access_allowed_impl(QString const & user_path, content::path_info_t & ipath, QString const & action, QString const & login_status, content::permission_flag & result)
{
    NOTUSED(user_path);
    NOTUSED(ipath);
    NOTUSED(action);
    NOTUSED(login_status);

    return result.allowed();
}


/** \fn void path::validate_action(content::path_info_t& ipath, QString const& action, permission_error_callback& err_callback)
 * \brief Validate the user action.
 *
 * This function validates the user action. If invalid or if that means
 * the user does not have enough rights to access the specified path,
 * then the event calls die() at some point and returns.
 *
 * \param[in,out] ipath  The path being validated.
 * \param[in] action  The action being performed against \p path.
 * \param[in,out] err_callback  Call functions on errors.
 */



/** \brief Dynamically compute the action for a path.
 *
 * Depending on the path and method (GET, POST, DELETE, PUT...) the system
 * reacts with a default action.
 *
 * Note that a path is automatically assigned the action as a parameter.
 * If the parameter named "action" is already defined, then that value
 * is returned and no other heuristic is used to determine the action.
 *
 * End users can force the action by using the qs_action string ("a"
 * by default) as in the following where the action is set to "edit":
 *
 * http://snapwebsites.com/terms-and-conditions?a=edit
 *
 * \todo
 * Really support all methods.
 *
 * \param[in] ipath  The path for which an action is to be determined.
 *                   (It is expected to be the main path though)
 *
 * \return The named action.
 */
QString path::define_action(content::path_info_t & ipath)
{
    QString action(ipath.get_parameter("action"));
    if(action.isEmpty())
    {
        QString const qs_action(f_snap->get_server_parameter("qs_action"));
        snap_uri const & uri(f_snap->get_uri());
        if(uri.has_query_option(qs_action))
        {
            // the user specified an action
            action = uri.query_option(qs_action);
        }

        if(action.isEmpty())
        {
            // use the default
            if(f_snap->has_post())
            {
                // this could also be "edit" or "create"...
                // but "administer" is more restrictive at this point
                action = "administer";
            }
            else if(ipath.get_cpath() == "admin" || ipath.get_cpath().startsWith("admin/"))
            {
                action = "administer";
            }
            else
            {
                action = "view";
            }
        }

        // save the action in the path
        ipath.set_parameter("action", action);
    }
    else if(action != "administer"
         && (ipath.get_cpath() == "admin" || ipath.get_cpath().startsWith("admin/")))
    {
        // TBD: anything under /admin is supposed to be administrative
        //      forms and navigation pages; however, we have our layout
        //      data which should probably be moved to another location
        //      because many of those pages are supposed to be public!
        //      (i.e. boxes) and certainly not administrative pages
        //      unless marked as such using some permission information
        //
        action = "administer";

        // save the action in the path
        ipath.set_parameter("action", action);
    }

    return action;
}


/** \brief Analyze the URL and execute the corresponding callback.
 *
 * This function looks for the page that needs to be displayed
 * from the URL information.
 *
 * \todo
 * Should we also test with case insensitive paths? (if all
 * else failed) Or should we make sure URL is all lowercase and
 * thus always make it case insensitive?
 *
 * \param[in] uri_path  The path received from the HTTP server.
 */
void path::on_execute(QString const & uri_path)
{
    content::path_info_t ipath;
    ipath.set_path(uri_path);

    // WARNING: the set_main_page() has the side effect of clearing out
    //          all the other parameters
    //
    ipath.set_main_page(true);

#ifdef DEBUG
    SNAP_LOG_TRACE("path::on_execute(\"")(uri_path)
            ("\") -> [")(ipath.get_cpath())
            ("] [branch=")(ipath.get_branch())
            ("] [revision=")(ipath.get_revision())
            ("]");
#endif

    // allow modules to redirect now, it has to be really early, note
    // that it will be BEFORE the path module verifies the permissions
    // AND before the POST data was managed
    {
        QString const original_cpath(ipath.get_cpath());
        check_for_redirect(ipath);
        if(original_cpath != ipath.get_cpath())
        {
            // change the path in main_ipath too
            f_snap->set_uri_path(QString("/%1").arg(ipath.get_cpath()));
        }
    }

    path_error_callback main_page_error_callback(f_snap, ipath);

    f_last_modified = 0;
    plugins::plugin * path_plugin(get_plugin(ipath, main_page_error_callback));

    // make a copy of the action in the snap child class URI so we can
    // easily access that information at any point, not just the
    // verify_rights() function
    //
    // WARNING: the get_plugin() defines the "action" parameter in ipath
    //          so we cannot check it before then
    //
    f_snap->set_action(ipath.get_parameter("action"));

    preprocess_path(ipath, path_plugin);

    // The last modification date is saved in the get_plugin()
    // It's a bit ugly but that way we test there that the page is valid and
    // we avoid having to search that information again to define the
    // corresponding header. However, it cannot be done in the get_plugin()
    // function since it may be called for other pages than the main page.
    //
    // ddd, dd MMM yyyy hh:mm:ss +0000
    if(0 != f_last_modified)
    {
        f_snap->set_header("Last-Modified", f_snap->date_to_string(f_last_modified, snap_child::date_format_t::DATE_FORMAT_HTTP));
    }

    // if a plugin pointer was defined we expect that the dynamic_cast<> will
    // always work, however path_plugin may be nullptr
    path_execute * pe(dynamic_cast<path_execute *>(path_plugin));
    if(pe == nullptr)
    {
        // not found, give a chance to some plugins to do something with the
        // current data (i.e. auto-search, internally redirect to a nice
        // Page Not Found page, etc.)
        page_not_found(ipath);
        if(f_snap->empty_output())
        {
            // no page_not_found() support and no error generated so far,
            // generate a default error now:
            if(path_plugin != nullptr)
            {
                // if the page exists then
                QString const owner(path_plugin->get_plugin_name());
                f_snap->die(snap_child::http_code_t::HTTP_CODE_NOT_FOUND,
                            "Plugin Missing",
                            "This page is not currently available as its plugin is not currently installed.",
                            "User tried to access page \"" + ipath.get_cpath() + "\" but its plugin (" + owner + ") does not yet implement the on_path_execute() function.");
            }
            else
            {
                f_snap->die(snap_child::http_code_t::HTTP_CODE_NOT_FOUND,
                            "Page Not Found",
                            "This page does not exist on this website.",
                            "User tried to access page \"" + ipath.get_cpath() + "\" and no dynamic path handling happened");
            }
            NOTREACHED();
        }
    }
    else
    {
        // execute the path for real

        // if the user POSTed something, manage that content first, the
        // effect is often to redirect the user in which case we want to
        // emit an HTTP Location and return; also, with AJAX we may end
        // up stopping early (i.e. not generate a full page but instead
        // return the "form results".)
        //
        // TBD: Could we not also allow a post in case we did not find
        //      a plugin to handle the page? (i.e. when pe is nullprt)
        f_snap->process_post();

        // if the buffer is still empty, the post process did not generate
        // an AJAX response, so go on by executing the page
        if(f_snap->empty_output())
        {
#ifdef DEBUG
            SNAP_LOG_TRACE("**** calling pe->on_path_execute(")(ipath.get_cpath())(")");
#endif
            if(!pe->on_path_execute(ipath))
            {
                // TODO (TBD):
                // page_not_found() not called here because the page exists
                // it is just not available right now and thus we
                // may not want to replace it with something else?
                f_snap->die(snap_child::http_code_t::HTTP_CODE_NOT_FOUND,
                        "Page Not Present",
                        "Somehow this page is not currently available.",
                        QString("User tried to access page \"%1\" but the page's plugin (%2) refused it.")
                                .arg(ipath.get_cpath()).arg(path_plugin->get_plugin_name()));
                NOTREACHED();
            }
        }
    }
}


/** \brief Allow modules to redirect before we do anything else.
 *
 * This signal is used to allow plugins to redirect before we hit anything
 * else. Note that this happens BEFORE we check for permissions.
 *
 * Note that the ipath parameter can be changed to a new path. This
 * means, internally, you may switch between one page and another.
 * In other words, you can send the user to a page such as /cute
 * and show the contents of page /ugly. This effect is done by
 * doing this:
 *
 * \code
 *      if(ipath.get_cpath() == "cute")
 *      {
 *          // "soft redirect"
 *          ipath.set_path("ugly");
 *          return;
 *      }
 * \endcode
 *
 * Note that means the f_snap->get_uri() will return the old ("cute")
 * path until the signal returns. Then the path plugin fixes it
 * accordingly. This is a way you have to check whether someone already
 * did a soft redirect when entering your on_check_for_redirect()
 * implementation:
 *
 * \code
 *      // path is returned without a starting "/" from a snap_uri object
 *      if(ipath.get_cpath() != f_snap->get_uri().path())
 *      {
 *          // someone already did a "soft redirect"
 *          return;
 *      }
 * \endcode
 *
 * \param[in,out] ipath  The path the client is trying to access.
 *
 * \return true if the message is to be propagated.
 */
bool path::check_for_redirect_impl(content::path_info_t & ipath)
{
    // check whether the page mode is currently MOVED
    content::path_info_t::status_t const status(ipath.get_status());
    if(status.get_state() == content::path_info_t::status_t::state_t::MOVED)
    {
        // the page was moved, get the new location and auto-redirect
        // user to the new page
        //
        // TODO: avoid auto-redirect if user is an administrator so that
        //       way the admin can reuse the page in some way
        //
        // TBD: what code is the most appropriate here? (we are using 301
        //      for now, but 303 or 307 could be better?)
        //
        links::link_info info(content::get_name(content::name_t::SNAP_NAME_CONTENT_CLONE), false, ipath.get_key(), ipath.get_branch());
        QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(info));
        links::link_info clone_info;
        if(link_ctxt->next_link(clone_info))
        {
            // WARNING: we could have been cloned multiple times,
            //          we just use the first link for now...
            //
            content::path_info_t moved_ipath;
            moved_ipath.set_path(clone_info.key());
            if(moved_ipath.get_status().get_state() == content::path_info_t::status_t::state_t::NORMAL)
            {
                // we have a valid destination, go there
                //
                // TODO: check that the user has enough permissions to view
                //       the destination; if so then do the redirect,
                //       otherwise there is no need to redirect
                //
                f_snap->page_redirect(moved_ipath.get_key(),
                            snap_child::http_code_t::HTTP_CODE_MOVED_PERMANENTLY,
                            "Redirect to the new version of this page.",
                            QString("This page (%1) was moved so we are redirecting this user to the new location (%2).")
                                    .arg(ipath.get_key()).arg(moved_ipath.get_key()));
                NOTREACHED();
            }
            // else -- TODO: if the destination status is MOVED, we can process it too!
        }

        // we cannot redirect to the copy, so just say not found
        f_snap->die(snap_child::http_code_t::HTTP_CODE_NOT_FOUND,
                    "Invalid Page",
                    "This page is not currently valid. It cannot be viewed.",
                    QString("User tried to access page \"%1\" but it is marked as MOVED and the destination is either unspecified or not NORMAL.")
                            .arg(ipath.get_key()));
        NOTREACHED();
    }

    return true;
}


/** \brief Improves the error signature.
 *
 * This function adds the search page to the brief signature of die()
 * errors.
 *
 * \param[in] path  The path to the page that generated the error.
 * \param[in] doc  The DOM document.
 * \param[in,out] signature_tag  The HTML signature to improve.
 */
void path::on_improve_signature(QString const & url_path, QDomDocument doc, QDomElement signature_tag)
{
    if(f_add_restore_link_to_signature.contains(url_path))
    {
        QString const qs_action(f_snap->get_server_parameter("qs_action"));

        // add a space between the previous link and this one
        snap_dom::append_plain_text_to_node(signature_tag, " ");

        // add a link to the user account
        QDomElement a_tag(doc.createElement("a"));
        a_tag.setAttribute("class", "restore");
        //a_tag.setAttribute("target", "_top"); -- I do not think _top will work here
        a_tag.setAttribute("href", QString("?%1=restore").arg(qs_action));
        // TODO: translate
        snap_dom::append_plain_text_to_node(a_tag, "Restore Deleted Page");

        signature_tag.appendChild(a_tag);
    }
}


/** \fn void path::preprocess_path(content::path_info_t & ipath, plugins::plugin * owner_plugin)
 * \brief Allow other modules to do some pre-processing.
 *
 * This signal is sent just before we run the actual execute() function
 * of the plugins. This can be useful to make some early changes to
 * the database so the page being displayed uses the correct data.
 *
 * \param[in,out] ipath  The path of the page being processed.
 * \param[in] owner_plugin  The plugin that owns this page (may be nullptr).
 */


/** \fn void path::can_handle_dynamic_path(content::path_info_t& ipath, dynamic_plugin_t& plugin_info)
 * \brief Default implementation of the dynamic path handler.
 *
 * This function doesn't do anything as the path plugin does not itself
 * offer another way to handle a path than checking the database (which
 * has priority and thus this function never gets called if that happens.)
 *
 * A good example of this signal can be found in the attachment plugin
 * which accepts paths such as an attachment with .gz to download the
 * compressed version of a file.
 *
 * The char_chart plugin shows you a way to handle a large number of
 * dynamic pages under a specific path.
 *
 * The favicon shows how a certain files can be selected dynamically.
 *
 * The qrcode plugin shows how one can create an image dynamically,
 * using a path very similar to the char_chart creating HTML pages.
 *
 * \param[in] ipath  The canonicalized path to be checked.
 * \param[in.out] plugin_info  Will hold information about the plugin that
 *                             can handle this dynamic path.
 */


/** \fn void path::page_not_found(content::path_info_t& ipath)
 * \brief Default implementation of the page not found signal.
 *
 * This function doesn't do anything as the path plugin does not itself
 * offer another way to handle a path than checking the database (which
 * has priority and thus this function never gets called if that happens.)
 *
 * If no other plugin transforms the result then a standard, plain text
 * 404 will be presented to the user.
 *
 * \param[in] ipath  The canonicalized path that generated a page not found.
 */



SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
