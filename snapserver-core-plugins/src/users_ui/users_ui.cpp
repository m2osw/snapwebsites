// Snap Websites Server -- users_ui handling
// Copyright (C) 2012-2017  Made to Order Software Corp.
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

/** \file
 * \brief Users User Interface handling.
 *
 * This plugin handles the user interface of the Users plugin.
 *
 * The forms supported are:
 *
 * \li The log in screen.
 * \li The log out feature and thank you page.
 * \li The registration.
 * \li The verification of an email to register.
 * \li The request for a new password.
 * \li The verification of an email to change a forgotten password.
 *
 * More basic features, such as actually creating a user are part
 * of the "users" plugin itself and not the "users_ui".
 */

#include "users_ui.h"

#include "../editor/editor.h"
#include "../output/output.h"
#include "../messages/messages.h"
#include "../sendmail/sendmail.h"
#include "../password/password.h"

#include <snapwebsites/log.h>
#include <snapwebsites/not_reached.h>
#include <snapwebsites/not_used.h>

#include <snapwebsites/poison.h>


SNAP_PLUGIN_START(users_ui, 1, 0)


namespace
{

const int SALT_SIZE = 32;
// the salt size must be even
BOOST_STATIC_ASSERT((SALT_SIZE & 1) == 0);

const int COOKIE_NAME_SIZE = 12; // the real size is (COOKIE_NAME_SIZE / 3) * 4
// we want 3 bytes to generate 4 characters
BOOST_STATIC_ASSERT((COOKIE_NAME_SIZE % 3) == 0);

} // no name namespace



/** \class users_ui
 * \brief The users_ui plugin to handle user interface of the "users" plugin.
 *
 * This class handles all the necessary user related end user pages:
 *
 * \li User log in
 * \li User registration
 * \li User registration token verification
 * \li User registration token re-generation
 * \li User forgotten password
 * \li User forgotten password token verification
 * \li User profile
 * \li User change of password
 * \li ...
 */


/** \brief Initialize the users_ui plugin.
 *
 * This function initializes the users_ui plugin.
 */
users_ui::users_ui()
    //: f_snap(nullptr) -- auto-init
    //, f_user_changing_password_key("") -- auto-init
{
}

/** \brief Destroy the users_ui plugin.
 *
 * This function cleans up the users_ui plugin.
 */
users_ui::~users_ui()
{
}


/** \brief Get a pointer to the users_ui plugin.
 *
 * This function returns an instance pointer to the users_ui plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the users_ui plugin.
 */
users_ui * users_ui::instance()
{
    return g_plugin_users_ui_factory.instance();
}


/** \brief Send users to the plugin settings.
 *
 * This path represents this plugin settings.
 */
QString users_ui::settings_path() const
{
    return "/admin/settings/users";
}


/** \brief A path or URI to a logo for this plugin.
 *
 * This function returns a 64x64 icons representing this plugin.
 *
 * \return A path to the logo.
 */
QString users_ui::icon() const
{
    return "/images/users/users-logo-64x64.png";
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
QString users_ui::description() const
{
    return "The users_ui plugin manages all the user interface (forms)"
           " on a website.";
}


/** \brief Change the help URI to the base plugin.
 *
 * This help_uri() function returns the URI to the base plugin URI
 * since this plugin is just an extension and does not need to have
 * a separate help page.
 *
 * \return The URI to the locale plugin help page.
 */
QString users_ui::help_uri() const
{
    // TBD: should we instead call the help_uri() of the users plugin?
    //
    //      users::users::instance()->help_uri();
    //
    //      I'm afraid that it would be a bad example because the pointer
    //      may not be a good pointer anymore at this time (once we
    //      properly remove plugins that we loaded just to get their info.)
    //
    return "http://snapwebsites.org/help/plugin/users";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString users_ui::dependencies() const
{
    return "|editor|form|layout|messages|password|output|path|sendmail|users|";
}


/** \brief Check whether updates are necessary.
 *
 * This function updates the database when a newer version is installed
 * and the corresponding updates where not run.
 *
 * This works for newly installed plugins and older plugins that were
 * updated.
 *
 * \param[in] last_updated  The UTC Unix date when the website was last
 *                          updated (in micro seconds).
 *
 * \return The UTC Unix date of the last update of this plugin.
 */
int64_t users_ui::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2015, 11,  4, 15, 46, 37, fix_owner_update);
    SNAP_PLUGIN_UPDATE(2017,  1, 17, 13, 57, 10, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Update the users_ui plugin content.
 *
 * This function updates the contents in the database using the
 * system update settings found in the resources.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added
 *                                 to the database by this update
 *                                 (in micro-seconds).
 */
void users_ui::content_update(int64_t variables_timestamp)
{
    NOTUSED(variables_timestamp);

    content::content::instance()->add_xml(get_plugin_name());
}


/** \brief This is an update for legacy websites.
 *
 * This function converts the specified pages so they are owned
 * by "users_ui" instead of "users".
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added
 *                                 to the database by this update
 *                                 (in micro-seconds).
 */
void users_ui::fix_owner_update(int64_t variables_timestamp)
{
    NOTUSED(variables_timestamp);

    // I leave this here as an example, only:
    //
    // 1. I fixed the XML files so it is not required
    // 2. on installation, it runs BEFORE pages get installed and never
    //    again, so if I were to still have errors in the list of pages
    //    presented below, it would double "fail"
    // 3. this should be called from do_dynamic_update() instead
    //
    char const * paths[] =
    {
        "login",
        //"verify-credentials",
        //"forgot-password",
        //"new-password",
        //"register",
        //"verify/resend",
        //"verify",
        //"layouts/default/left/login"   // this is a box
        //"logout",
        //"user/password",
        //"user/password/replace",
        //"images/users",                           users
        //"admin/settings/users",                   output
        //"admin/email/users",                      output -- this may be because it is a directory and not a "useful" page in itself?
        //"admin/email/users/verify",
        //"admin/email/users/forgot-password",
        //"admin/page/users",
        //"admin/page/users/profile",
    };

    content::content * content_plugin(content::content::instance());
    libdbproxy::table::pointer_t content_table(content_plugin->get_content_table());
    QString const plugin_name(get_plugin_name());

    for(auto const & s : paths)
    {
        content::path_info_t ipath;
        ipath.set_path(s);
        content_table->getRow(ipath.get_key())->getCell(content::get_name(content::name_t::SNAP_NAME_CONTENT_PRIMARY_OWNER))->setValue(plugin_name);
    }
}


/** \brief Bootstrap the users.
 *
 * This function adds the events the users plugin is listening for.
 *
 * \param[in] snap  The child handling this request.
 */
void users_ui::bootstrap(::snap::snap_child * snap)
{
    f_snap = snap;

    SNAP_LISTEN0(users_ui, "server", server, attach_to_session);
    SNAP_LISTEN0(users_ui, "server", server, detach_from_session);
    SNAP_LISTEN(users_ui, "path", path::path, can_handle_dynamic_path, _1, _2);
    SNAP_LISTEN(users_ui, "path", path::path, check_for_redirect, _1);
    SNAP_LISTEN(users_ui, "filter", filter::filter, replace_token, _1, _2, _3);
    SNAP_LISTEN(users_ui, "filter", filter::filter, token_help, _1);
    SNAP_LISTEN(users_ui, "editor", editor::editor, init_editor_widget, _1, _2, _3, _4, _5);
    SNAP_LISTEN(users_ui, "editor", editor::editor, finish_editor_form_processing, _1, _2);
    SNAP_LISTEN(users_ui, "editor", editor::editor, save_editor_fields, _1);
}


/** \brief Save the user session identifier on password change.
 *
 * To avoid loggin people before they are done changing their password,
 * so that way they cannot go visit all the private pages on the website,
 * we use a session variable to save the information about the user who
 * is changing his password.
 */
void users_ui::on_attach_to_session()
{
    if(!f_user_changing_password_key.isEmpty())
    {
        users::users * users_plugin(users::users::instance());
        users_plugin->attach_to_session(users::get_name(users::name_t::SNAP_NAME_USERS_CHANGING_PASSWORD_KEY), f_user_changing_password_key);
    }
    else if(!f_user_changing_password_key_clear)
    {
        // it was not empty when on_detach_from_session() was called
        // so we have to detach now (i.e. delete from the session)
        //
        users::users * users_plugin(users::users::instance());
        NOTUSED(users_plugin->detach_from_session(users::get_name(users::name_t::SNAP_NAME_USERS_CHANGING_PASSWORD_KEY)));
    }
}


/** \brief Retrieve data that was attached to a session.
 *
 * This function is the opposite of the on_attach_to_session(). It is
 * called before the execute() to reinitialize objects that previously
 * saved data in the user session.
 */
void users_ui::on_detach_from_session()
{
    // TODO:
    // here we probably should do a get_from_session() because we may need
    // the variable between several different forms before it really gets
    // deleted permanently (i.e. we are reattaching now, but if a crash
    // occurs between the detach and attach, we lose the information!)
    // So the concerned function(s) should clear() the variable when
    // officially done with it.
    users::users * users_plugin(users::users::instance());
    f_user_changing_password_key = users_plugin->get_from_session(users::get_name(users::name_t::SNAP_NAME_USERS_CHANGING_PASSWORD_KEY));
    f_user_changing_password_key_clear = f_user_changing_password_key.isEmpty();
}


/** \brief Replace a token with a corresponding value.
 *
 * This function replaces the users tokens with their value. In some cases
 * the values were already computed in the XML document, so all we have to do is query
 * the XML and return the corresponding value.
 *
 * The supported tokens are:
 *
 * \li users::email -- the user email as is
 * \li users::email_anchor -- the user email as an anchor (mailto:)
 * \li users::since -- the date and time when the user registered
 *
 * \param[in,out] ipath  The path to the page being worked on.
 * \param[in] plugin_owner  The plugin that owns this ipath content.
 * \param[in,out] xml  The XML document used with the layout.
 * \param[in,out] token  The token object, with the token name and optional parameters.
 */
void users_ui::on_replace_token(content::path_info_t & ipath, QDomDocument & xml, filter::filter::token_info_t & token)
{
    NOTUSED(ipath);
    NOTUSED(xml);

    if(!token.is_namespace("users::"))
    {
        // not a users plugin token
        return;
    }

    bool const users_picture(token.is_token("users::picture"));
    if(users_picture)
    {
        SNAP_LOG_TRACE() << "first is_token(\"users::picture\")";
        // setup as the default image by default
        token.f_replacement = "<img src=\"/images/users/default-user-image.png\" alt=\"Default user picture\" width=\"32\" height=\"32\"/>";
    }

    users::users * users_plugin(users::users::instance());
    auto const & user_info(users_plugin->get_user_info());
    if(!user_info.is_valid())
    {
        // user is not known
        return;
    }

    if(user_info.is_anonymous()
    || !user_info.exists())
    {
        // cannot find user...
        //
        // (TBD: we may want to have some info for the anonymous user?)
        //
        return;
    }

    // anything else requires the user to be verified
    libdbproxy::value const verified_on(user_info.get_value(users::name_t::SNAP_NAME_USERS_VERIFIED_ON));
    if(verified_on.nullValue())
    {
        // not verified yet
        return;
    }

    if(token.is_token("users::picture"))
    {
        // make sure that the user created and verified his account
        libdbproxy::value const value(user_info.get_value(users::name_t::SNAP_NAME_USERS_PICTURE));
        if(!value.nullValue())
        {
            SNAP_LOG_TRACE("second is_token(\"users::picture\")");

            // TBD: not sure right now how we will offer those
            //      probably with a special path that tells us
            //      to go look in the users' table
            //
            //      We may also want to only offer the Avatar for
            //      user picture(s)
            //
            token.f_replacement = QString("<img src=\"...\"/>");
        }
    }
}


void users_ui::on_token_help(filter::filter::token_help_t & help)
{
    help.add_token("users::picture",
        "Display a picture for the specified user. (not implemented yet, we still want to support an avatar like feature but we need a way to upload an image first.)");
}


/** \brief Check whether \p cpath matches our introducers.
 *
 * This function checks that cpath matches our introducer and if
 * so we tell the path plugin that we're taking control to
 * manage this path.
 *
 * We understand "user" as in list of users.
 *
 * We understand "user/<name>" as in display that user information
 * (this may be turned off on a per user or for the entire website.)
 * Websites that only use an email address for the user identification
 * do not present these pages publicly.
 *
 * We understand "profile" which displays the current user profile
 * information in detail and allow for editing of what can be changed.
 *
 * We understand "login" which displays a form for the user to log in.
 *
 * We understand "verify-credentials" which is very similar to "login"
 * albeit simpler and only appears if the user is currently logged in
 * but not recently logged in (i.e. administration rights.)
 *
 * We understand "logout" to allow users to log out of Snap! C++.
 *
 * We understand "register" to display a registration form to users.
 *
 * We understand "verify" to check a session that is being returned
 * as the user clicks on the link we sent on registration.
 *
 * We understand "forgot-password" to let users request a password reset
 * via a simple form.
 *
 * \todo
 * If we cannot find a global way to check the Origin HTTP header
 * sent by the user agent, we probably want to check it here in
 * pages where the referrer should not be a "weird" 3rd party
 * website.
 *
 * \param[in,out] ipath  The path being handled dynamically.
 * \param[in,out] plugin_info  If you understand that cpath, set yourself here.
 */
void users_ui::on_can_handle_dynamic_path(content::path_info_t & ipath, path::dynamic_plugin_t & plugin_info)
{
    // is that path already going to be handled by someone else?
    // (avoid wasting time if that is the case)
    //
    // this happens when the attachment plugin is to handle user
    // image previews
    if(plugin_info.get_plugin()
    || plugin_info.get_plugin_if_renamed())
    {
        return;
    }

    //
    // WARNING:
    //
    //    DO NOT PROCESS ANYTHING HERE!
    //
    //    At this point we do not know whether the user has the right
    //    permissions yet.
    //
    //    See users_ui::on_path_execute() instead.
    //
    QString cpath(ipath.get_cpath());
    if(cpath == "user"                      // list of (public) users
    || cpath == "profile"                   // the logged in user profile
    || cpath == "login"                     // form to log user in
    || cpath == "logout"                    // log user out
    || cpath == "register"                  // form to let new users register
    || cpath == "verify-credentials"        // re-log user in
    || cpath == "verify"                    // verification form so the user can enter his code
    || cpath.left(7) == "verify/"           // link to verify user's email; and verify/resend form
    || cpath == "forgot-password"           // form for users to reset their password
    || cpath == "new-password"              // form for users to enter their forgotten password verification code
    || cpath.left(13) == "new-password/")   // form for users to enter their forgotten password verification code
    {
        // tell the path plugin that this is ours
        //
        plugin_info.set_plugin(this);
    }
    else if(cpath.left(5) == "user/")       // show a user profile (user/ is followed by the user identifier or some edit page such as user/password)
    {
        snap_string_list const user_segments(cpath.split("/"));
        if(user_segments.size() == 2)
        {
            plugin_info.set_plugin(this);
        }
    }
}


/** \brief Check whether the user is accessing a "change password" page.
 *
 * The system wants to prevent the user from accessing the change
 * password pages if the user changed his password very recently
 * (see the delay between password changes as defined by
 * the "users" password policy.)
 *
 * \param[in,out] ipath  The ipath of the page being accessed.
 */
void users_ui::on_check_for_redirect(content::path_info_t & ipath)
{
    QString const cpath(ipath.get_cpath());
    if(cpath == "user/password")
    {
        users::users * users_plugin(users::users::instance());
        auto user_info(users_plugin->get_user_info());
        if(user_info.exists()
        && users_plugin->user_is_logged_in()) // only logged in users can change their password
        {
            password::policy_t pp("users");
            int64_t const delay(pp.get_delay_between_password_changes());
            if(delay > 0)
            {
                // get the logged in user
                users::get_name(users::name_t::SNAP_NAME_USERS_PASSWORD_MODIFIED);
                //libdbproxy::table::pointer_t users_table(users_plugin->get_users_table());
                int64_t const password_last_modification( user_info.get_value(users::name_t::SNAP_NAME_USERS_PASSWORD_MODIFIED).safeInt64Value(0, 0) );
                int64_t const start_date(f_snap->get_start_date());
                if(password_last_modification + delay * 60LL * 1000000LL > start_date)
                {
                    // trying to change the password again too soon
                    messages::messages::instance()->set_error(
                        "Permission Denied",
                        QString("You are not currently authorized to change your password. You will have to wait about %1 minutes before you can do so again.")
                                        .arg(1 + (password_last_modification + delay * 60LL * 1000000LL - start_date) / (60LL * 1000000LL)),
                        "attempt to change password again too soon",
                        false
                    );
                    f_snap->page_redirect("user/me", snap_child::http_code_t::HTTP_CODE_SEE_OTHER, "Permission Denied", "You changed your account password recently and this website does not allow you to change it again right away. You will have to wait some time and try again.");
                    NOTREACHED();
                }
            }
        }
    }
}


/** \brief Execute the specified path.
 *
 * This is a dynamic page which the users plugin knows how to handle.
 *
 * This function never returns if the "page" is just a verification
 * process which redirects the user (i.e. "verify/<id>", and
 * "new-password/<id>" at this time.)
 *
 * Other paths may also redirect the user in case the path is not
 * currently supported (mainly because the user does not have
 * permission.)
 *
 * \param[in,out] ipath  The canonicalized path.
 *
 * \return true if the processing worked as expected, false if the page
 *         cannot be created ("Page Not Present" results on false)
 */
bool users_ui::on_path_execute(content::path_info_t & ipath)
{
    // handle the few that do some work and redirect immediately
    // (although it could be in the on_generate_main_content()
    // it is a big waste of time to start building a page when
    // we know we will redirect the user anyway)
    if(ipath.get_cpath().left(7) == "verify/"
    && ipath.get_cpath() != "verify/resend")
    {
        users::users * users_plugin(users::users::instance());
        users_plugin->verify_user(ipath);
        NOTREACHED();
    }
    else if(ipath.get_cpath().left(13) == "new-password/")
    {
        verify_password(ipath);
        NOTREACHED();
    }

    f_snap->output(layout::layout::instance()->apply_layout(ipath, this));

    return true;
}


void users_ui::on_generate_main_content(content::path_info_t & ipath, QDomElement & page, QDomElement & body)
{
    QString const cpath(ipath.get_cpath());
    if(!cpath.isEmpty())
    {
        // the switch() optimization is worth it because all user pages
        // hit this test, so saving a few ms is always worth the trouble!
        // (i.e. at the moment, we already have 11 tests; any one cpath
        // would be checked 11 times for any page other than one of those
        // 11 pages... with the new scheme, we compare between 0 and 3 times
        // instead)
        switch(cpath[0].unicode())
        {
        case 'f':
            if(cpath == "forgot-password")
            {
                prepare_forgot_password_form();
            }
            break;

        case 'l':
            if(cpath == "login")
            {
                prepare_login_form();
            }
            else if(cpath == "logout")
            {
                // closing current session if any and show the logout page
                logout_user(ipath, page, body);
                return;
            }
            break;

        case 'n':
            if(cpath == "new-password")
            {
                prepare_new_password_form();
            }
            break;

        //case 'p':
        //  if(cpath == "profile")
        //  {
        //      // TODO: write user profile editor
        //      //       this is /user, /user/###, and /user/me at this point
        //      //user_profile(body);
        //      return;
        //  }
        //  break;

        case 'r':
            // "register" is the same form as "verify" and "verify/resend"
            if(cpath == "register")
            {
                prepare_basic_anonymous_form();
            }
            break;

        case 'u':
            if(cpath == "user")
            {
                // TODO: write user listing (similar to the /admin page
                //       in gathering the info)
                //list_users(body);
                output::output::instance()->on_generate_main_content(ipath, page, body);
                return;
            }
            else if(cpath == "user/password/replace")
            {
                // this is a very special form that is accessible by users who
                // requested to change the password with the "forgot password"
                // capability
                //
                prepare_replace_password_form(body);
            }
            else if(cpath.left(5) == "user/")
            {
                show_user(ipath, page, body);
                return;
            }
            break;

        case 'v':
            if(cpath == "verify-credentials")
            {
                prepare_verify_credentials_form();
            }
            else if(cpath == "verify"
                 || cpath == "verify/resend")
            {
                prepare_basic_anonymous_form();
            }
            break;

        }
    }

    // any other user_ui page is just like regular content
    output::output::instance()->on_generate_main_content(ipath, page, body);
}



void users_ui::on_generate_boxes_content(content::path_info_t & page_cpath, content::path_info_t & ipath, QDomElement & page, QDomElement & box)
{

// TODO: as an extension, only allow the login/register forms when
//       the user adds a query string with a secret key
//           example.com?login=key  (admin can choose the name, i.e. "login")

    users::users * users_plugin(users::users::instance());
    if(users_plugin->user_is_logged_in()) // logged in users never see the login/register boxes
    {
        if(ipath.get_cpath().endsWith("login")
        || ipath.get_cpath().endsWith("register"))
        {
            return;
        }
    }
    //else -- if the user is not anonymous, we could still hide those boxes
    //        but in that case we'd want a flag to know whether this website
    //        works one way or the other...

//std::cerr << "GOT TO USER BOXES!!! [" << ipath.get_key() << "]\n";
    if(ipath.get_cpath().endsWith("/login"))
    {
        // do not display the login box on the login page
        // or if the user is already logged in

// DEBUG -- at this point there are conflicts with more than 1 form on a page, so I only allow that form on the home page
//if(page_cpath.get_cpath() != "") return;

        if(page_cpath.get_cpath() == "login"
        || page_cpath.get_cpath() == "register")
        {
            return;
        }
    }

    output::output::instance()->on_generate_main_content(ipath, page, box);
}


/** \brief Let the user replace their password.
 *
 * This is a very special form that is only accessible when the user
 * requests a special link after forgetting their password.
 *
 * \param[in] body  The body where the form is saved.
 */
void users_ui::prepare_replace_password_form(QDomElement & body)
{
    NOTUSED(body);

    users::users * users_plugin(users::users::instance());

    // make sure the user is properly setup
    //
    if(users_plugin->user_has_administrative_rights())
    {
        // user is administratively (recently) logged in already,
        // send him to his normal password form
        //
        f_snap->page_redirect("user/password", snap_child::http_code_t::HTTP_CODE_SEE_OTHER, "Already Logged In", "You are already logged in so you cannot access this page at this time.");
        NOTREACHED();
    }
    if(users_plugin->user_is_logged_in())
    {
        // user logged in a while back, ask for credentials again
        // (we want the user to have  administrative permissions,
        // meaning we want the user to have logged in recently.)
        //
        f_snap->page_redirect("verify-credentials", snap_child::http_code_t::HTTP_CODE_SEE_OTHER, "Not Enough Permissions", "You are logged in with minimal permissions. To access this page we have to verify your credentials.");
        NOTREACHED();
    }
    if(f_user_changing_password_key.isEmpty())
    {
        // user is not even logged in and he did not follow a valid link
        // XXX the login page is probably the best choice?
        f_snap->page_redirect("login", snap_child::http_code_t::HTTP_CODE_SEE_OTHER, "Replace Password Not Possible", "You required to change your password in a way which is not currently valid. Please go to log in instead.");
        NOTREACHED();
    }
}


/** \brief Show the user profile.
 *
 * This function shows a user profile. By default one can use user/me to
 * see his profile. The administrators can see any profile. Otherwise
 * only public profiles and the user own profile are accessible.
 */
void users_ui::show_user(content::path_info_t & ipath, QDomElement & page, QDomElement & body)
{
    QString user_path(ipath.get_cpath());
    int64_t identifier(0);
    QString user_id(user_path.mid(5));
    if( user_id == "me" || user_id == "password" )
    {
        users::users * users_plugin(users::users::instance());

        // retrieve the logged in user identifier
        //
        // (TBD: could we allow the user to go to "/user/me" even when the
        // user is not fully logged in?)
        //
        auto const user_info(users_plugin->get_user_info());
        if(!users_plugin->user_is_logged_in())
        {
            // user was trying to change his password?
            //
            if(user_id == "password")
            {
                users_plugin->set_referrer("user/password", user_info);
            }

            messages::messages::instance()->set_error(
                "Permission Denied",
                "You are not currently logged in. You may check out your profile only when logged in.",
                "attempt to view the current user page when the user is not logged in",
                false
            );
            // redirect the user to the log in page
            //
            f_snap->page_redirect("login", snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
            NOTREACHED();
            return;
        }
        if(!user_info.exists())
        {
            // This should never happen... we checked that account when the
            // user logged in, although the anonymous user has no data in
            // the database in case we are dealing with such.
            //
            messages::messages::instance()->set_error(
                "Could Not Find Your Account",
                "Somehow we could not find your account on this system.",
                QString("user account for \"%1\" does not exist at this point").arg(user_info.get_user_key()),
                true
            );
            // redirect the user to the log in page
            //
            f_snap->page_redirect("login", snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
            NOTREACHED();
            return;
        }
        libdbproxy::value value(user_info.get_value(users::name_t::SNAP_NAME_USERS_IDENTIFIER));
        if(value.nullValue())
        {
            messages::messages::instance()->set_error(
                "Could Not Find Your Account",
                "Somehow we could not find your account on this system.",
                QString("user account for \"%1\" does not have an identifier").arg(user_info.get_user_key()),
                true
            );
            // redirect the user to the log in page
            //
            f_snap->page_redirect("login", snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
            NOTREACHED();
            return;
        }
        identifier = value.int64Value();

        if(user_id == "password")
        {
            // user is editing his password
            //
            prepare_password_form();
            output::output::instance()->on_generate_main_content(ipath, page, body);
            return;
        }

        // Probably not necessary to change user_id now
        //
        user_path = QString("user/%1").arg(identifier);
    }
    else
    {
        bool ok(false);
        identifier = user_id.toLongLong(&ok);
        if(!ok)
        {
            // invalid user identifier, generate a 404
            f_snap->die(snap_child::http_code_t::HTTP_CODE_NOT_FOUND,
                    "User Not Found",
                    "This user does not exist. Please check the URI and make corrections as required.",
                    "User attempt to access user \"" + user_id + "\" which does not look like a valid integer.");
            NOTREACHED();
        }

        // verify that the identifier indeed represents a user
        const QString site_key(f_snap->get_site_key_with_slash());

        // TODO: should be user identifier...?
        const QString user_key(QString("%1%2/%3")
                               .arg(site_key)
                               .arg(users::get_name(users::name_t::SNAP_NAME_USERS_PATH))
                               .arg(user_id)
                               );
        libdbproxy::table::pointer_t content_table(content::content::instance()->get_content_table());
        if(!content_table->exists(user_key))
        {
            f_snap->die(snap_child::http_code_t::HTTP_CODE_NOT_FOUND,
                "User Not Found",
                QString("We could not find an account for user \"%1\" on this system.").arg(user_id),
                QString("user account for \"%1\" does not exist at this point").arg(user_id)
            );
            NOTREACHED();
        }
    }
//printf("Got user [%s] / [%ld]\n", cpath.toUtf8().data(), identifier);
//std::cout << "Got user [" << identifier << "]" << std::endl << std::flush;

    // generate the user profile
        // TODO: write user profile viewer (i.e. we need to make use of the identifier here!)
    content::path_info_t user_ipath;
    user_ipath.set_path(user_path);
    output::output::instance()->on_generate_main_content(user_ipath, page, body);
}


/** \brief Generate the password form.
 *
 * This function adds a compiled password form to the body content.
 * (i.e. this is the main page body content.)
 *
 * This form includes the original password, and the new password with
 * a duplicate to make sure the user enters it twice properly.
 *
 * The password can also be changed by requiring the system to send
 * an email. In that case, and if the user then remembers his old
 * password, then this form is hit on the following log in.
 */
void users_ui::prepare_password_form()
{
    users::users * users_plugin(users::users::instance());
    if(!users_plugin->user_is_logged_in())
    {
        // user needs to be logged in to edit his password
        f_snap->die(snap_child::http_code_t::HTTP_CODE_FORBIDDEN,
                "Access Denied",
                "You need to be logged in and have enough permissions to access this page.",
                "user attempt to change a password without enough permissions.");
        NOTREACHED();
    }
}


/** \brief Prepare the login form.
 *
 * This function makes sure that the user is not already logged in because
 * if so the user can just be sent to his profile (/user/me).
 *
 * Otherwise it saves the current HTTP_REFERER information as the page to
 * redirect the user after a successfull login.
 */
void users_ui::prepare_login_form()
{
    users::users * users_plugin(users::users::instance());
    if(users_plugin->user_is_logged_in())
    {
        // user is logged in already, just send him to his profile
        f_snap->page_redirect("user/me", snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
        NOTREACHED();
    }

// TODO: as an extension, only allow the login/register forms when
//       the user adds a query string with a secret key
//           example.com?login=key  (admin can choose the name, i.e. "login")

    // pass the user info whether it is valid or not
    //
    auto const user_info(users_plugin->get_user_info());
    users_plugin->set_referrer( f_snap->snapenv("HTTP_REFERER"), user_info );
}


/** \brief Verify user credentials.
 *
 * The verify user credentials form can only appear to users who logged
 * in a while back and who need administrative rights to access a page.
 */
void users_ui::prepare_verify_credentials_form()
{
    // user is an anonymous user, send him to the login form instead
    users::users * users_plugin(users::users::instance());
    if(!users_plugin->user_is_logged_in())
    {
        f_snap->page_redirect("login", snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
        NOTREACHED();
    }

    if(users_plugin->user_has_administrative_rights())
    {
        // ?!? -- what should we do in this case?
        f_snap->page_redirect("user/me", snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
        NOTREACHED();
    }

    // Note that here users_plugin->user_is_logged_in() may return
    // true, only this is not enough to administer the website
    // so not enough to let a user change his password
}


/** \brief Log the current user out.
 *
 * Actually this function only generates the log out page. The log out itself
 * is processed at the same time as the cookie in the on_process_cookies()
 * function.
 *
 * This function calls the on_generate_main_content() of the content plugin.
 *
 * \param[in,out] ipath  The path being processed (logout[/...]).
 * \param[in,out] page  The page XML data.
 * \param[in,out] body  The body XML data.
 */
void users_ui::logout_user(content::path_info_t & ipath, QDomElement & page, QDomElement & body)
{
    // generate the body
    // we already logged the user out in the on_process_cookies() function
    if(ipath.get_cpath() != "logout")
    {
        // make sure the page exists if the user was sent to antoher plugin
        // path (i.e. logout/fantom from the fantom plugin could be used to
        // display a different greating because the user was kicked out by
        // spirits...); if it does not exist, force "logout" as the default
        libdbproxy::table::pointer_t content_table(content::content::instance()->get_content_table());
        if(!content_table->exists(ipath.get_key()))
        {
            // forcing to exact /logout page which we know will work
            ipath.set_path("logout");
        }
    }

    output::output::instance()->on_generate_main_content(ipath, page, body);
}


/** \brief Prepare a public user form.
 *
 * This function is used to prepare a basic user form which is only
 * intended for anonymous users. All it does is verify that the user
 * is not logged in. If logged in, then the user is simply send to
 * his profile (user/me).
 */
void users_ui::prepare_basic_anonymous_form()
{
    users::users * users_plugin(users::users::instance());
    if(users_plugin->user_is_logged_in())
    {
        // user is logged in already, just send him to his profile
        //
        f_snap->page_redirect("user/me", snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
        NOTREACHED();
    }
}


/** \brief Resend a verification email to the user.
 *
 * This function sends the verification email as if the user was just
 * registering. It is at items useful if the first email gets blocked
 * or lost in a junk mail folder.
 *
 * We should also show the "From" email on our forms so users can say
 * that these are okay.
 *
 * \todo
 * Add a question such as "what's your favority movie", "where were you
 * born", etc. so we can limit the number of people who use this form.
 */
void users_ui::prepare_forgot_password_form()
{
    users::users * users_plugin(users::users::instance());
    if(users_plugin->user_is_logged_in())
    {
        // send user to his change password form if he's logged in
        //
        // XXX look into changing this policy and allow logged in
        //     users to request a password change? (I don't think
        //     it matters actually)
        //
        messages::messages::instance()->set_error(
            "You Are Logged In",
            "If you want to change your password and forgot your old password, you'll have to log out and request for a new password while not logged in.",
            "user tried to get to the forgot_password_form() while logged in.",
            false
        );
        f_snap->page_redirect("user/password", snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
        NOTREACHED();
    }
}


/** \brief Allow the user to use his verification code to log in.
 *
 * This function verifies a verification code that was sent so the user
 * could change his password (i.e. an automatic log in mechanism.)
 */
void users_ui::prepare_new_password_form()
{
    users::users * users_plugin(users::users::instance());
    if(users_plugin->user_is_logged_in())
    {
        // send user to his change password form if he's logged in
        // XXX look into changing this policy and allow logged in
        //     users to request a password change? (I don't think
        //     it matters actually)
        messages::messages::instance()->set_error(
            "You Are Already Logged In",
            "If you want to change your password and forgot your old password, you'll have to log out and request for a new password while not logged in.",
            "user tried to get to the forgot_password_form() while logged in.",
            false
        );
        f_snap->page_redirect("user/password", snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
        NOTREACHED();
    }
}


/** \fn void users_ui::user_verified(content::path_info_t& ipath, int64_t identifier)
 * \brief Signal that a new user was verified.
 *
 * After a user registers, he receives an email with a magic number that
 * needs to be used for the user to register on the system.
 *
 * This signal is used in order to tell other plugins that the user did
 * following that link.
 *
 * \param[in,out] ipath  The user path.
 * \param[in] identifier  The user identifier.
 */


/** \brief Check that password verification code.
 *
 * This function verifies a password verification code that is sent to
 * the user whenever he says he forgot his password.
 *
 * \param[in] ipath  The path used to access this page.
 */
void users_ui::verify_password(content::path_info_t & ipath)
{
    users::users * users_plugin(users::users::instance());
    if(users_plugin->user_is_logged_in())
    {
        // TBD: delete the "password" tag if present?
        //      that would seem wrong; if we have a module that forces
        //      users to enter a new password on their next log in,
        //      then we should not delete the link! that way will work
        //      albeit the user could have their session renewed many
        //      times over before they are really forced to change their
        //      password (but that is another problem.)
        //
        // user is logged in already, just send him to his profile
        // (if logged in he was verified in some way!)
        //
        f_snap->page_redirect("user/me", snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
        NOTREACHED();
    }

    QString const session_id(ipath.get_cpath().mid(13));

    // TODO: add support for a forgotten password cookie as a second shield against
    //       hackers would could end up seeing the email in transit.
    //       see SNAP-259 for other details

    sessions::sessions::session_info info;
    sessions::sessions * session(sessions::sessions::instance());
    // TODO: remove the ending characters such as " ", "/", "\" and "|"?
    //       (it happens that people add those by mistake at the end of a URI...)
    session->load_session(session_id, info);
    libdbproxy::value verify_ignore_user_agent(f_snap->get_site_parameter(users::get_name(users::name_t::SNAP_NAME_USERS_VERIFY_IGNORE_USER_AGENT_FOR_PASSWORD)));
    QString const path(info.get_object_path());
    if( info.get_session_type() != sessions::sessions::session_info::session_info_type_t::SESSION_INFO_VALID
     || ((info.add_check_flags(0) & info.CHECK_HTTP_USER_AGENT) != 0
                && verify_ignore_user_agent.safeSignedCharValue(0, 0) == 0
                && info.get_user_agent() != f_snap->snapenv(snap::get_name(snap::name_t::SNAP_NAME_CORE_HTTP_USER_AGENT)))
     || path.mid(0, 6) != users::users::user_info_t::get_full_anonymous_path() )
    {
        // it failed, the session could not be loaded properly
        SNAP_LOG_WARNING("users::verify_password() could not load the user session ")
                            (session_id)(" properly. Session error: ")
                            (sessions::sessions::session_info::session_type_to_string(info.get_session_type()))(".");
        // TODO change message support to use strings from the database so they can get translated
        messages::messages::instance()->set_error(
            "Invalid Forgotten Password Verification Code",
            QString("The specified verification code (%1) is not correct."
                    " Please verify that you used the correct link or try to use the form below to enter your verification code."
                    " If you already followed the link once, then you already exhausted that verfication code and if you need another you have to click the Resend link below.")
                    .arg(session_id),
            QString("user trying his forgotten password verification with code \"%1\" got error: %2.")
                    .arg(session_id)
                    .arg(sessions::sessions::session_info::session_type_to_string(info.get_session_type())),
            true
        );
        // we are likely on the verification link for the new password
        // so we want to send people to the new-password page instead
        //
        // XXX should we avoid the redirect if we are already on that page?
        //
        f_snap->page_redirect("new-password", snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
        NOTREACHED();
    }

    // it looks like the session is valid, get the user identifier and verify
    // that the account exists in the database
    //QString const user_email(path.mid(6)); // this is the user_key from the session, it is a canonicalized email TODO! This should be an id!
    //QString const (path.mid(6)); // this is the user_key from the session, it is a canonicalized email TODO! This should be an id!
    //auto const user_info(users_plugin->get_user_info_by_email(user_email));
    QString const id_string(path.mid(6));
    bool ok(false);
    users::users::identifier_t const identifier(id_string.toLongLong(&ok, 10)); // this is the identifier from the session (SNAP-258)
    if(!ok)
    {
        messages::messages::instance()->set_error(
            "Could Not Find Your Account",
            "Somehow we could not find your account on this system.",
            QString("count not convert user ID from \"%1\" to a valid identifier").arg(path),
            true
        );
        // redirect the user to the log in page
        f_snap->page_redirect("login", snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
        NOTREACHED();
    }
    auto const user_info(users_plugin->get_user_info_by_id(identifier));
    if(!user_info.exists())
    {
        // This should never happen...
        messages::messages::instance()->set_error(
            "Could Not Find Your Account",
            "Somehow we could not find your account on this system.",
            QString("user account for \"%1\" does not exist at this point").arg(user_info.get_user_email()),
            true
        );
        // redirect the user to the log in page
        f_snap->page_redirect("login", snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
        NOTREACHED();
    }

#if 0
    libdbproxy::value const user_identifier(user_info.get_value(users::name_t::SNAP_NAME_USERS_IDENTIFIER));
    if(user_identifier.nullValue())
    {
        SNAP_LOG_FATAL("users::verify_password() could not load the user identifier, the row exists but the cell did not make it (")
                        (user_email)("/")
                        (users::get_name(users::name_t::SNAP_NAME_USERS_IDENTIFIER))(").");
        // TODO where to send that user?! have an error page for all of those
        //      "your account is dead, sorry dear..."
        f_snap->page_redirect("login", snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
        NOTREACHED();
    }
    int64_t const identifier(user_identifier.int64Value());
#endif
    content::path_info_t user_ipath;
    user_ipath.set_path(QString("%1/%2").arg(users::get_name(users::name_t::SNAP_NAME_USERS_PATH)).arg(identifier));

    // before we actually accept this verification code, we must make sure
    // the user is still marked as a new user (he should or the session
    // would be invalid, but for security it is better to check again)
    links::link_info user_status_info(users::get_name(users::name_t::SNAP_NAME_USERS_STATUS), true, user_ipath.get_key(), user_ipath.get_branch());
    QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(user_status_info));
    links::link_info status_info;
    if(!link_ctxt->next_link(status_info))
    {
        // This should never happen... because the session should logically
        // prevent it from happening (i.e. the status link should always be
        // there) although maybe the admin could delete this link somehow?
        messages::messages::instance()->set_error(
            "Forgotten Password?",
            "It does not look like you requested a new password for your account. The form is being canceled.",
            QString("user account for \"%1\", which requested a mew password, is not marked as expected a new password").arg(user_info.get_user_email()),
            true
        );
        // redirect the user to the log in page
        f_snap->page_redirect("login", snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
        NOTREACHED();
    }

    // a status link exists... is it the right one?
    QString const site_key(f_snap->get_site_key_with_slash());
    if(status_info.key() != site_key + users::get_name(users::name_t::SNAP_NAME_USERS_PASSWORD_PATH))
    {
        // This should never happen... because the session should logically
        // prevent it from happening (i.e. the status link should always be
        // there) although maybe the admin could delete this link somehow?
        messages::messages::instance()->set_error(
            "Forgotten Password?",
            "It does not look like you requested a new password for your account. If you did so multiple times, know that you can only follow one of the links once. Doing so voids the other links.",
            QString("user account for \"%1\", which requested a new password, is not marked as expecting a new password: %2.")
                    .arg(user_info.get_user_email())
                    .arg(status_info.key()),
            true
        );
        // redirect the user to the log in page? (XXX should this be the registration page instead?)
        f_snap->page_redirect("login", snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
        NOTREACHED();
    }
    // remove the "user/password" status link so the user can now log in
    // he was successfully logged in -- don't kill this one yet...
    //links::links::instance()->delete_link(user_status_info);

    // redirect the user to the "semi-public replace password page"
    send_to_replace_password_page(user_info.get_user_email(), false);
    NOTREACHED();
}


/** \brief This function sends the user to the replace password.
 *
 * WARNING: Use this function at your own risk! It allows the user to
 *          change (his) password and thus it should be done only if
 *          you know for sure (as sure as one can be in an HTTP context)
 *          that the user is allowed to do this.
 *
 * This function saves the email of the user to redirect to the
 * /user/password/replace page. That page is semi-public in that it can
 * be accessed by users who forgot their password after they followed
 * a link we generate from the "I forgot my password" account. It is
 * semi-public because, after all, it can be accessed by someone who is
 * not actually logged in.
 *
 * The function redirects you so it does not return.
 *
 * The function saves the date and time when it gets called, and the IP
 * address of the user who triggered the call.
 *
 * \todo
 * Look whether we could move this call (and the verify that calls this
 * function) in the users_ui plugin where it really belongs.
 *
 * \param[in] email  The email of the user to redirect.
 * \param[in] set_status  Whether to setup the user status too.
 */
void users_ui::send_to_replace_password_page(QString const & email, bool const set_status)
{
    users::users * users_plugin(users::users::instance());
    users::users::user_info_t user_info(users_plugin->get_user_info_by_email(email));

    // the only caller already does that but if this is a public function,
    // we want to make double sure!
    //
    if(!user_info.exists())
    {
        // This should never happen...
        messages::messages::instance()->set_error(
            "Could Not Find Your Account",
            "Somehow we could not find your account on this system.",
            QString("user account for \"%1\" does not exist at this point").arg(user_info.get_user_email()),
            true
        );
        // redirect the user to the log in page
        f_snap->page_redirect("login", snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
        NOTREACHED();
    }

    if(set_status)
    {
        // mark the user with the types/users/password tag
        // (i.e. user requested a new password)
        // source
        //
        QString const link_name(users::get_name(users::name_t::SNAP_NAME_USERS_STATUS));
        bool const source_unique(true);
        content::path_info_t user_ipath;
        user_ipath.set_path(user_info.get_user_path(false));
        links::link_info source(link_name, source_unique, user_ipath.get_key(), user_ipath.get_branch());

        // destination
        //
        QString const link_to(users::get_name(users::name_t::SNAP_NAME_USERS_STATUS));
        bool const destination_unique(false);
        content::path_info_t password_path;
        password_path.set_path(users::get_name(users::name_t::SNAP_NAME_USERS_PASSWORD_PATH));
        links::link_info destination(link_to, destination_unique, password_path.get_key(), password_path.get_branch());

        // create link
        //
        links::links::instance()->create_link(source, destination);
    }

    // Save the date when the user sent the request
    //
    libdbproxy::value value;
    value.setInt64Value(f_snap->get_start_date());
    user_info.set_value(users::name_t::SNAP_NAME_USERS_FORGOT_PASSWORD_ON, value);

    // Save the user IP address when the user sent the request
    //
    value.setStringValue(f_snap->snapenv(snap::get_name(snap::name_t::SNAP_NAME_CORE_REMOTE_ADDR)));
    user_info.set_value(users::name_t::SNAP_NAME_USERS_FORGOT_PASSWORD_IP, value);

    // make sure that this variable is set to a canonicalized user key
    //
    f_user_changing_password_key = user_info.get_user_key();

    // send the user to the "public" replace password page since he got verified
    //
    f_snap->page_redirect("user/password/replace", snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
    NOTREACHED();
}


/** \brief Process a post from one of the users forms.
 *
 * This function processes the post of a user form. The function uses the
 * \p ipath parameter in order to determine which form is being processed.
 *
 * \param[in,out] ipath  The path the user is accessing now.
 * \param[in] session_info  The user session being processed.
 */
void users_ui::on_process_form_post(content::path_info_t & ipath, sessions::sessions::session_info const & session_info)
{
    NOTUSED(session_info);

    QString const cpath(ipath.get_cpath());
    if(cpath == "login")
    {
        process_login_form(users::users::login_mode_t::LOGIN_MODE_FULL);
    }
    else if(cpath == "verify-credentials")
    {
        process_login_form(users::users::login_mode_t::LOGIN_MODE_VERIFICATION);
    }
    else if(cpath == "register")
    {
        process_register_form();
    }
    else if(cpath == "verify/resend")
    {
        process_verify_resend_form();
    }
    else if(cpath == "verify")
    {
        process_verify_form();
    }
    else if(cpath == "forgot-password")
    {
        process_forgot_password_form();
    }
    else if(cpath == "new-password")
    {
        process_new_password_form();
    }
    else if(cpath == "user/password/replace")
    {
        process_replace_password_form();
    }
    else if(cpath == "user/password")
    {
        process_password_form();
    }
    else
    {
        // this should not happen because invalid paths will not pass the
        // session validation process
        throw users_ui_exception_invalid_path("users_ui::on_process_form_post() was called with an unsupported path: \"" + ipath.get_key() + "\"");
    }
}


/** \brief Log the user in from the log in form.
 *
 * This function uses the credentials specified in the log in form.
 * The function searches for the user account and read its hashed
 * password and compare the password typed in the form. If it
 * matches, then the user receives a cookie and is logged in for
 * some time.
 *
 * This function takes a mode.
 *
 * \li LOGIN_MODE_FULL -- full mode (for the login form)
 * \li LOGIN_MODE_VERIFICATION -- verification mode (for the verify-credentials form)
 *
 * \param[in] login_mode  The mode used to log in: full, verification.
 */
void users_ui::process_login_form(users::users::login_mode_t login_mode)
{
    users::users * users_plugin(users::users::instance());
    messages::messages * messages_plugin(messages::messages::instance());

    // TODO: add support to log user using a username instead of
    //       just the email address (having a username or pseudonym makes
    //       it harder for hackers to find your account!)

    // retrieve the row for that user
    QString const email(f_snap->postenv("email"));
    users::users::user_info_t const user_info(users_plugin->get_user_info_by_email(email));
    if(login_mode == users::users::login_mode_t::LOGIN_MODE_VERIFICATION
            && users_plugin->get_user_info().get_user_key() != user_info.get_user_key())    // TODO: compare IDs instead?
    {
        // XXX we could also automatically log the user out and send him
        //     to the log in screen... (we certainly should do so on the
        //     third attempt!)
        messages_plugin->set_error(
            "Wrong Credentials",
            "These are wrong credentials. If you are not sure who you were logged as, please <a href=\"/logout\">log out</a> first and then log back in.",
            QString("users_ui::process_login_form() email mismatched when verifying credentials (got \"%1\", expected \"%2\").")
                    .arg(user_info.get_user_email())
                    .arg(users_plugin->get_user_info().get_user_email()),
            false
        );
        return;
    }

    QString const password(f_snap->postenv("password"));

    bool validation_required(false);
    QString const details(users_plugin->login_user(email, password, validation_required, login_mode));

    if(!details.isEmpty())
    {
        if(messages_plugin->get_error_count() == 0
        && messages_plugin->get_warning_count() == 0)
        {
            // print an end user message only if the number of
            // errors/warnings is still zero

            // IMPORTANT:
            //   We have ONE error message because whatever the error we do not
            //   want to tell the user exactly what went wrong (i.e. wrong email,
            //   or wrong password.)
            //
            //   This is important because if someone is registered with an email
            //   such as example@snapwebsites.info and a hacker tries that email
            //   and gets an error message saying "wrong password," now the hacker
            //   knows that the user is registered on that Snap! C++ system.

            // user not registered yet?
            // email misspelled?
            // incorrect password?
            // email still not validated?
            //
            // TODO: Put the messages in the database so they can be translated
            messages_plugin->set_error(
                "Could Not Log You In",
                validation_required
                  ? "Your account was not yet <a href=\"/verify\" title=\"Click here to enter a verification code\">validated</a>."
                        " Please make sure to first follow the link we sent in your email."
                        " If you did not yet receive that email, we can send you another <a href=\"/verify/resend\">confirmation email</a>."
                  : "Your email or password were incorrect."
                        " If you are not registered, you may want to consider <a href=\"/register\">registering</a> first?",
                details,
                false // should this one be true?
            );
        }
        else
        {
            // in this case we only want to log the details
            // the plugin that generated errors/warnings is
            // considered to otherwise be in charge
            SNAP_LOG_WARNING("Could not log user in (but another plugin generated an error): ")(details);
        }
    }
}


/** \brief Register a user.
 *
 * This function saves a user credential information as defined in the
 * registration form.
 *
 * This function creates a new entry in the users table and then links
 * that entry in the current website.
 *
 * \todo
 * We need to look into the best way to implement the connection with
 * the current website. We do not want all the websites to automatically
 * know about all the users (i.e. a website has a list of users, but
 * that's not all the users registered in Snap!)
 */
void users_ui::process_register_form()
{
    users::users * users_plugin(users::users::instance());
    messages::messages * messages(messages::messages::instance());
    sendmail::sendmail * sendmail_plugin(sendmail::sendmail::instance());

    // We validated the email already and we just don't need to do it
    // twice, if two users create an account "simultaneously (enough)"
    // with the same email, that's probably not a normal user (i.e. a
    // normal user would not be able to create two accounts at the
    // same time.) The email is the row key of the user table.
    //
    QString const email(f_snap->postenv("email"));

    // before we attempt a registration we check with sendmail whether
    // the email address is alright...
    //
    if(!sendmail_plugin->validate_email(email, nullptr))
    {
        messages::messages::instance()->set_error(
            // TODO: ameliorate the error message, here we use the message
            //       given to us by a throw and it includes some technical
            //       data and is not translated... at the same time, it
            //       should rarely happen
            "Invalid Email Address",
            QString("The specified email (%1) address was marked as invalid. Please check the email to make sure it is correct.").arg(email),
            QString("email address \"%1\" not considered valid by the system.").arg(email),
            true // the message includes an email which may be blacklisted (and thus a valid/legitimate email) so it should be hidden
        );
        return;
    }

    QString reason;
    users::users::status_t const status(users_plugin->register_user(email, f_snap->postenv("password"), reason));
    switch(status)
    {
    case users::users::status_t::STATUS_NEW:
        verify_email(email);
        messages->set_info(
            "We registered your account",
            QString("We sent you an email to \"%1\". In the email there is a link you need to follow to finish your registration.").arg(email)
        );
        // redirect the user to the verification form
        f_snap->page_redirect("verify", snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
        NOTREACHED();
        break;

    case users::users::status_t::STATUS_VALID:
        // already exists since we found a valid entry of this user
        messages->set_error(
            "User Already Exists",
            QString("A user with email \"%1\" already exists. If it is you, then try to request a new password if you need a reminder.").arg(email),
            QString("user \"%1\" trying to register a second time.").arg(email),
            true
        );
        break;

    case users::users::status_t::STATUS_BLOCKED:
        // already exists since we found a valid entry of this user
        f_snap->die(snap_child::http_code_t::HTTP_CODE_FORBIDDEN,
                "Access Denied",
                "You are not allowed to create an account on this website.",
                QString("User \"%1\" is blocked and does not have permission to create an account here.").arg(email));
        NOTREACHED();
        break;

    case users::users::status_t::STATUS_PASSWORD:
        if(!reason.isEmpty())
        {
            // password not viewed as secure enough
            messages->set_error(
                "Password Not Strong Enough",
                QString("The password you specified is not considered secure enough. Please, try again with a stronger password. Reason: %1").arg(reason),
                "password used is either not strong enough or was black listed.",
                true
            );
        }
        else
        {
            messages->set_error(
                "User Already Exists",
                QString("A user with email \"%1\" already exists. However, he needs to verify his email address. If it is you, try the Enter Verification Code link.").arg(email),
                QString("user \"%1\" trying to register a second time.").arg(email),
                true
            );
        }
        break;

    default:
        // ???
        f_snap->die(snap_child::http_code_t::HTTP_CODE_FORBIDDEN,
                "Access Denied",
                "You are not allowed to create an account on this website.",
                QString("register_user() returned an unexpected status (%1) for \"%2\".").arg(static_cast<int>(status)).arg(email));
        NOTREACHED();
        break;

    }
}


/** \brief Send an email so the user can log in without password.
 *
 * This process generates an email with a secure code. It is sent to the
 * user which will have to click on a link to auto-login in his account.
 * Once there, he will be forced to enter a new password (and duplicate
 * thereof).
 *
 * This only works for currently active users.
 */
void users_ui::process_forgot_password_form()
{
    users::users * users_plugin(users::users::instance());

    QString const email(f_snap->postenv("email"));
    QString details;

    auto const & user_info( users_plugin->get_user_info_by_email(email) );

    // check to make sure that a user with that email address exists
    if(user_info.exists())
    {
        // existing users have a unique identifier
        // necessary to create the user key below
        libdbproxy::value const user_identifier(user_info.get_value(users::name_t::SNAP_NAME_USERS_IDENTIFIER));
        if(!user_identifier.nullValue())
        {
            int64_t const identifier(user_identifier.int64Value());
            content::path_info_t user_ipath;
            user_ipath.set_path(QString("%1/%2").arg(users::get_name(users::name_t::SNAP_NAME_USERS_PATH)).arg(identifier));

            // verify the status of this user
            links::link_info user_status_info(users::get_name(users::name_t::SNAP_NAME_USERS_STATUS), true, user_ipath.get_key(), user_ipath.get_branch());
            QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(user_status_info));
            links::link_info status_info;
            QString status;
            if(link_ctxt->next_link(status_info))
            {
                // a status link exists...
                status = status_info.key();
            }
            // empty represents ACTIVE
            // or if user already requested for a new password
            QString const site_key(f_snap->get_site_key_with_slash());
            if(status == "" || status == site_key + users::get_name(users::name_t::SNAP_NAME_USERS_PASSWORD_PATH))
            {
                // Only users considered active can request a new password
                forgot_password_email(user_info);

                // mark the user with the types/users/password tag
                QString const link_name(users::get_name(users::name_t::SNAP_NAME_USERS_STATUS));
                bool const source_unique(true);
                links::link_info source(link_name, source_unique, user_ipath.get_key(), user_ipath.get_branch());
                QString const link_to(users::get_name(users::name_t::SNAP_NAME_USERS_STATUS));
                bool const destination_unique(false);
                content::path_info_t dpath;
                dpath.set_path(users::get_name(users::name_t::SNAP_NAME_USERS_PASSWORD_PATH));
                links::link_info destination(link_to, destination_unique, dpath.get_key(), dpath.get_branch());
                links::links::instance()->create_link(source, destination);

                // once we sent the new code, we can send the user back
                // to the verify form
                messages::messages::instance()->set_info(
                    "New Verification Email Sent",
                    "We just sent you a new verification email. Please check your account and follow the verification link or copy and paste your verification code below."
                );
                f_snap->page_redirect("new-password", snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
                NOTREACHED();
            }
            else
            {
                details = "user " + email + " is not active nor in \"new password\" mode, we do not send verification emails to such";
            }
        }
        else
        {
            details = "somehow we saw that a row existed for " + email + ", but we could not retrieve it";
        }
    }
    else
    {
        // XXX here we could test the email address and if invalid generate
        //     different details (we'd need to do that only if we get quite
        //     a few of those errors, we could then block IPs with repetitive
        //     invalid email addresses)
        //
        // probably a stupid spammer robot
        details = "user asking for forgot-password with an unknown email address: " + email;
    }

    // ONE error so whatever the reason the end user cannot really know
    // whether someone registered with that email address on our systems
    messages::messages::instance()->set_error(
        "Not an Active Account",
        "This email is not from an active account. No email was sent to you.",
        details,
        false
    );
    // no redirect, the same form will be shown again
}


/** \brief Processing the forgotten password verification code.
 *
 * This process verifies that the verification code entered is the one
 * expected for the user to correct a forgotten password.
 *
 * This works only if the user is active with a status of "password".
 * If not we assume that the user already changed his password because
 * (1) we force the user to do so if that status is on; and (2) the
 * link is removed when the new password gets saved successfully.
 */
void users_ui::process_new_password_form()
{
    const QString session_id(f_snap->postenv("verification_code"));
    content::path_info_t ipath;
    ipath.set_path("new-password/" + session_id);
    verify_password(ipath);
}



/** \brief Save the new password assuming everything checks out.
 *
 * This saves the new password in the database and logs the user in so
 * he can go on with his work.
 */
void users_ui::process_replace_password_form()
{
    // make sure the user is properly setup
    users::users * users_plugin(users::users::instance());
    if(users_plugin->user_is_logged_in())
    {
        // user is logged in already, send him to his normal password form
        f_user_changing_password_key.clear();
        f_snap->page_redirect("user/password", snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
        NOTREACHED();
    }
    if(f_user_changing_password_key.isEmpty())
    {
        // user is not logged in and he did not follow a valid link
        // XXX the login page is probably the best choice?
        f_snap->page_redirect("login", snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
        NOTREACHED();
    }

    // for errors if any
    QString details;

    // replace the password assuming we can find that user information
    //libdbproxy::table::pointer_t users_table(users_plugin->get_users_table());
    //if(users_table->exists(f_user_changing_password_key))
    auto user_info(users_plugin->get_user_info_by_email(f_user_changing_password_key));
    if(user_info.exists())
    {
        //libdbproxy::row::pointer_t row(users_table->getRow(f_user_changing_password_key));

        // existing users have a unique identifier
        // necessary to create the user key below
        libdbproxy::value const user_identifier(user_info.get_value(users::name_t::SNAP_NAME_USERS_IDENTIFIER));
        if(!user_identifier.nullValue())
        {
            int64_t const identifier(user_identifier.int64Value());
            content::path_info_t user_ipath;
            user_ipath.set_path(QString("%1/%2").arg(users::get_name(users::name_t::SNAP_NAME_USERS_PATH)).arg(identifier));

            // verify the status of this user
            links::link_info user_status_info(users::get_name(users::name_t::SNAP_NAME_USERS_STATUS), true, user_ipath.get_key(), user_ipath.get_branch());
            QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(user_status_info));
            links::link_info status_info;
            if(link_ctxt->next_link(status_info))
            {
                // a password status link exists...
                QString const site_key(f_snap->get_site_key_with_slash());
                if(status_info.key() == site_key + users::get_name(users::name_t::SNAP_NAME_USERS_PASSWORD_PATH))
                {
                    QString const password(f_snap->postenv("password"));

                    users::users::user_security_t security;
                    //security.set_user_key(f_user_changing_password_key);
                    //security.set_email("");
                    security.set_user_info(user_info);
                    security.set_password(password);
                    security.set_bypass_blacklist(true);
                    users_plugin->check_user_security(security);
                    if(security.get_secure().allowed())
                    {

                        // We are good, save the new password and remove that link

                        // Save encrypted password
                        users_plugin->save_password(user_info, password, "users");

                        // Unlink from the password tag too
                        links::links::instance()->delete_link(user_status_info);

                        // Now we auto-log in the user... the session should
                        // already be adequate from the on_process_cookies()
                        // call
                        //
                        // TODO to make this safer we really need the extra
                        //      3 questions and ask one of them when the user
                        //      request the new password or when he comes back
                        //      in the replace password form
                        //
                        users_plugin->create_logged_in_user_session(user_info);

                        f_user_changing_password_key.clear();

                        content::content::instance()->modified_content(user_ipath);

                        // once we sent the new code, we can send the user back
                        // to the verify form
                        messages::messages::instance()->set_info(
                            "Password Changed",
                            "Your new password was saved. Next time you want to log in, you can use your email with this new password."
                        );

                        // TBD: should we use the saved login redirect instead?
                        //      (if not then we probably want to clear it)
                        f_snap->page_redirect("user/me", snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
                        NOTREACHED();
                    }

                    // well... someone said "I do not like this password"!
                    details = security.get_secure().reason();
                }
                else
                {
                    // the link is not saying "PASSWORD"
                    details = QString("user \"%1\" did not request change their password").arg(f_user_changing_password_key);
                }
            }
            else
            {
                // This happens for all users already active, users who are
                // blocked, etc.
                details = QString("user \"%1\" is currently active, only user who forgot their password should be sent here").arg(f_user_changing_password_key);
            }
        }
        else
        {
            details = QString("somehow we saw that a row existed for \"%1\", but we could not retrieve the user identifier").arg(f_user_changing_password_key);
        }
    }
    else
    {
        details = QString("user \"%1\" does not exist in the users table").arg(f_user_changing_password_key);
    }

    // we're done with this variable
    // we have to explicitly clear it or it may stay around for a long time
    // (i.e. it gets saved in the session table)
    f_user_changing_password_key.clear();

    messages::messages::instance()->set_error(
        "Not a Valid Account",
        "Somehow an error occured while we were trying to update your account password.",
        details,
        false
    );

    // XXX the login page is probably the best choice?
    f_snap->page_redirect("login", snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
    NOTREACHED();
}


/** \brief Process the password form.
 *
 * This function processes the password form. It verifies that the
 * old_password is correct. If so, it saves the new password in the
 * user's account.
 *
 * The function then redirects the user to his profile (user/me).
 */
void users_ui::process_password_form()
{
    users::users * users_plugin(users::users::instance());
    auto user_info(users_plugin->get_user_info());

    // make sure the user is properly logged in first
    //
    if(!users_plugin->user_is_logged_in())
    {
        // user is not even logged in!?
        //
        f_snap->page_redirect("login", snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
        NOTREACHED();
    }

    // for errors if any
    QString details;

    // replace the password assuming we can find that user information
    if(user_info.exists())
    {
        // We're good, save the new password and remove that link.
        // Existing users have a unique identifier, necessary to create the user key below.
        //
        libdbproxy::value const user_identifier(user_info.get_value(users::name_t::SNAP_NAME_USERS_IDENTIFIER));
        if(!user_identifier.nullValue())
        {
            int64_t const identifier(user_identifier.int64Value());
            content::path_info_t user_ipath;
            user_ipath.set_path(QString("%1/%2").arg(users::get_name(users::name_t::SNAP_NAME_USERS_PATH)).arg(identifier));

            // verify the status of this user
            links::link_info user_status_info(users::get_name(users::name_t::SNAP_NAME_USERS_STATUS), true, user_ipath.get_key(), user_ipath.get_branch());
            QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(user_status_info));
            bool delete_password_status(false);
            links::link_info status_info;
            if(link_ctxt->next_link(status_info))
            {
                // a status link exists...
                QString const site_key(f_snap->get_site_key_with_slash());
                if(status_info.key() == site_key + users::get_name(users::name_t::SNAP_NAME_USERS_BLOCKED_PATH)
                || status_info.key() == site_key + users::get_name(users::name_t::SNAP_NAME_USERS_AUTO_PATH)
                || status_info.key() == site_key + users::get_name(users::name_t::SNAP_NAME_USERS_NEW_PATH))
                {
                    // somehow the user is not blocked or marked as auto...
                    f_snap->die(snap_child::http_code_t::HTTP_CODE_FORBIDDEN,
                            "Access Denied", "You need to be logged in and have enough permissions to access this page.",
                            "User attempt to change a password in his account which is currently blocked.");
                    NOTREACHED();
                }
                else if(status_info.key() == site_key + users::get_name(users::name_t::SNAP_NAME_USERS_PASSWORD_PATH))
                {
                    // we will be able to delete this one
                    delete_password_status = true;
                }
            }

            // TODO make sure that the new password is not the same as the
            //      last X passwords, including the old_password/new_password
            //      variables as defined here

            // compute the hash of the old password to make sure the user
            // knows his password
            //
            // (1) get the digest
            libdbproxy::value value(user_info.get_value(users::name_t::SNAP_NAME_USERS_PASSWORD_DIGEST));
            QString const old_digest(value.stringValue());

            // (2) we need the passord:
            QString const old_password(f_snap->postenv("old_password"));

            // (3) get the salt in a buffer
            value = user_info.get_value(users::name_t::SNAP_NAME_USERS_PASSWORD_SALT);
            QByteArray const old_salt(value.binaryValue());

            // (4) compute the expected hash
            QByteArray old_hash;
            users_plugin->encrypt_password(old_digest, old_password, old_salt, old_hash);

            // (5) retrieved the saved hashed password
            value = user_info.get_value(users::name_t::SNAP_NAME_USERS_PASSWORD);
            QByteArray const saved_hash(value.binaryValue());

            // (6) verify that it matches
            if(old_hash.size() == saved_hash.size()
            && memcmp(old_hash.data(), saved_hash.data(), old_hash.size()) == 0)
            {
                // XXX should we verify the new password validity before
                //     we verify the old password
                //
                QString const new_password(f_snap->postenv("new_password"));

                // make sure the new password is not actually equal to
                // the existing password
                //
                QByteArray new_hash;
                users_plugin->encrypt_password(old_digest, new_password, old_salt, new_hash);
                if(old_hash.size() == new_hash.size()
                && memcmp(old_hash.data(), new_hash.data(), new_hash.size()) == 0)
                {
                    messages::messages::instance()->set_error(
                        "Invalid Password",
                        "The password your entered is the same as your old password which is not allowed. Please try again.",
                        "user is trying to \"change\" his password with the same password!?",
                        false
                    );
                    return;
                }

                users::users::user_security_t security;
                security.set_user_info(users_plugin->get_user_info());
                security.set_password(new_password);
                security.set_bypass_blacklist(true);
                users_plugin->check_user_security(security);
                if(security.get_secure().allowed())
                {
                    // The user entered his old password properly
                    // save the new password
                    users_plugin->save_password(user_info, new_password, "users");

                    // Unlink from the password tag too
                    if(delete_password_status)
                    {
                        links::links::instance()->delete_link(user_status_info);
                    }

                    content::content::instance()->modified_content(user_ipath);

                    // once we sent the new code, we can send the user back
                    // to the verify form
                    //
                    messages::messages::instance()->set_info(
                        "Password Changed",
                        "Your new password was saved. Next time you want to log in, you must use your email with this new password."
                    );
                    QString referrer(users_plugin->detach_referrer(user_info));
                    if(referrer == "user/password")
                    {
                        // ignore the default redirect if it is to this page
                        //
                        referrer.clear();
                    }
                    if(referrer.isEmpty())
                    {
                        // Redirect user to his profile
                        //
                        f_snap->page_redirect("user/me", snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
                    }
                    else
                    {
                        // If the user logged in when he needed to still change
                        // his password and there still was a referrer path
                        //
                        f_snap->page_redirect(referrer, snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
                    }
                    NOTREACHED();
                }
                messages::messages::instance()->set_error(
                    "Invalid Password",
                    QString("The new password is not strong enough. Please try again. Reason: %1").arg(security.get_secure().reason()),
                    "user is trying to change his password but the new password is not strong enough for this website",
                    false
                );
                return;
            }
            else
            {
                messages::messages::instance()->set_error(
                    "Invalid Password",
                    "The password your entered as your old password is not correct. Please try again.",
                    "user is trying to change his password and he mistyped his existing password",
                    false
                );
                return;
            }
        }
        else
        {
            details = QString("somehow we saw that a row existed for \"%1\", but we could not retrieve the user identifier")
                        .arg(users_plugin->get_user_info().get_user_key());
        }
    }
    else
    {
        details = QString("user \"%1\" does not exist in the users table")
                .arg(users_plugin->get_user_info().get_user_key());
    }

    messages::messages::instance()->set_error(
        "Not a Valid Account",
        "Somehow an error occured while we were trying to update your account password.",
        details,
        false
    );

    // XXX the profile page is probably the best choice?
    f_snap->page_redirect("user/me", snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
    NOTREACHED();
}


/** \brief "Resend" the verification email.
 *
 * This function runs whenever a user requests the system to send an
 * additional verification code a given email address.
 *
 * Before we proceed, we verify that the user status is "new" (tag
 * as such.) If not, we generate an error and prevent the email from
 * being sent.
 */
void users_ui::process_verify_resend_form()
{
    users::users * users_plugin(users::users::instance());

    QString const email(f_snap->postenv("email"));
    QString details;

    // check to make sure that a user with that email address exists
    auto user_info(users_plugin->get_user_info_by_email(email));
    if(user_info.exists())
    {
        // existing users have a unique identifier
        // necessary to create the user key below
        libdbproxy::value const user_identifier(user_info.get_value(users::name_t::SNAP_NAME_USERS_IDENTIFIER));
        if(user_identifier.size() == sizeof(int64_t))
        {
            int64_t const identifier(user_identifier.int64Value());
            content::path_info_t user_ipath;
            user_ipath.set_path(QString("%1/%2").arg(users::get_name(users::name_t::SNAP_NAME_USERS_PATH)).arg(identifier));

            // verify the status of this user
            links::link_info user_status_info(users::get_name(users::name_t::SNAP_NAME_USERS_STATUS), true, user_ipath.get_key(), user_ipath.get_branch());
            QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(user_status_info));
            links::link_info status_info;
            if(link_ctxt->next_link(status_info))
            {
                // a status link exists...
                QString const site_key(f_snap->get_site_key_with_slash());
                if(status_info.key() == site_key + users::get_name(users::name_t::SNAP_NAME_USERS_NEW_PATH))
                {
                    // Only new users are allowed to get another verification email
                    verify_email(email);
                    // once we sent the new code, we can send the user back
                    // to the verify form
                    messages::messages::instance()->set_info(
                        "New Verification Email Sent",
                        "We just sent you a new verification email. Please check your account and follow the verification link or copy and paste your verification code below."
                    );
                    f_snap->page_redirect("verify", snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
                    NOTREACHED();
                }

                details = QString("user \"%1\" is not new (maybe it is active, blocked, auto...), we do not send verification emails to such").arg(email);
            }
            else
            {
                // This happens for all users already active, users who are
                // blocked, etc.
                details = QString("user \"%1\" is currently active, we do not send verification emails to such").arg(email);
            }
        }
        else
        {
            details = QString("somehow we saw that a row existed for \"%1\", but we could not retrieve it").arg(email);
        }
    }
    else
    {
        // XXX here we could test the email address and if invalid generate
        //     different details (we'd need to do that only if we get quite
        //     a few of those errors, we could then block IPs with repetitive
        //     invalid email addresses)
        //
        // probably a stupid spammer robot
        details = QString("user asking for verify-resend with an unknown email address: %1").arg(email);
    }

    // ONE error so whatever the reason the end user cannot really know
    // whether someone registered with that email address on our systems
    messages::messages::instance()->set_error(
        "Not a New Account",
        "This email is not from a new account. It may be from an already active account, or from someone who never registered with us, or someone who is currently blocked. <strong>No verification email was sent.</strong>",
        details,
        false
    );
    // no redirect, the same form will be shown again
}


/** \brief Process the verification code.
 *
 * This function runs the verify_user() function with the code that the
 * user entered in the form. This is similar to going to the
 * verify/\<verification_code> page to get an account confirmed.
 *
 * The verification code gets "simplified" as in all spaces get removed.
 * The code cannot include spaces anyway and when someone does a copy &
 * paste, at times, a space is added at the end. This way, such spaces
 * will be ignored.
 */
void users_ui::process_verify_form()
{
    // verify the code the user entered, the verify_user() function
    // will automatically redirect us if necessary; we should
    // get an error if redirect to ourselves
    //
    QString verification_code(f_snap->postenv("verification_code"));
    content::path_info_t ipath;
    ipath.set_path("verify/" + verification_code.simplified());
    users::users * users_plugin(users::users::instance());
    users_plugin->verify_user(ipath);
}




/** \brief Send an email to request email verification.
 *
 * This function generates an email and sends it. The email is used to request
 * the user to verify that he receives said emails.
 *
 * \param[in] email  The user email.
 */
void users_ui::verify_email(QString const & email)
{
    users::users * users_plugin(users::users::instance());
    auto user_info(users_plugin->get_user_info_by_email(email));

    QString current_email( user_info.get_value(users::name_t::SNAP_NAME_USERS_CURRENT_EMAIL).stringValue() );
    if(current_email.isEmpty())
    {
        // TODO: the email should always be defined, only we have
        //       legacy code which may skip on the matter and thus
        //       we want to have this fallback
        //
        current_email = email;
    }

    sendmail::sendmail::email e;

    // mark priority as High
    e.set_priority(sendmail::sendmail::email::email_priority_t::EMAIL_PRIORITY_HIGH);

    // destination email address
    e.add_header(sendmail::get_name(sendmail::name_t::SNAP_NAME_SENDMAIL_TO), current_email);

    e.add_parameter(sendmail::get_name(sendmail::name_t::SNAP_NAME_SENDMAIL_BYPASS_BLACKLIST), "true");

    // add the email subject and body using a page
    e.set_email_path("admin/email/users/verify");

    // verification makes use of a session identifier
    sessions::sessions::session_info info;
    info.set_session_type(sessions::sessions::session_info::session_info_type_t::SESSION_INFO_USER);
    info.set_session_id(users::users::USERS_SESSION_ID_VERIFY_EMAIL);
    info.set_plugin_owner(get_plugin_name()); // ourselves
    //info.set_page_path(); -- default is okay
    info.set_object_path( user_info.get_user_path(true) ); // sessions are always using the user id and not the email directly
    info.set_user_agent(f_snap->snapenv(snap::get_name(snap::name_t::SNAP_NAME_CORE_HTTP_USER_AGENT)));
    info.set_time_to_live(86400 * 3);  // 3 days
    QString const session(sessions::sessions::instance()->create_session(info));
    e.add_parameter(users::get_name(users::name_t::SNAP_NAME_USERS_VERIFY_EMAIL), session);

    // to allow a "resend" without regenerating a new session, we save
    // the session identifier--since those are short lived, it will anyway
    // not be extremely useful, but some plugins may use that once in a while
    libdbproxy::value session_value(session);
    int64_t const ttl(86400 * 3 - 86400 / 2); // keep in the database for a little less than the session itself
    session_value.setTtl(ttl);
    user_info.set_value( users::name_t::SNAP_NAME_USERS_LAST_VERIFICATION_SESSION, session_value );

    // send the email
    //
    // really this just saves it in the database, the sendmail itself
    // happens on the backend; see sendmail::on_backend_action()
    sendmail::sendmail::instance()->post_email(e);
}


/** \brief Resend a verification email.
 *
 * This function is a repeat of the verify_email() function. That is,
 * by default it attempts to reuse the same session information to
 * send the verification email to the user. It is generally used by
 * an administrator who registered a user on their behalf and is told
 * that the user did not receive their verification email.
 *
 * If the function is called too long after the session was created,
 * it will be erased by Cassandra so a new session gets created
 * instead. Unfortunately, there is no information to the end user
 * if that happens.
 *
 * If the verification email is not sent, then the function returns false.
 * This specifically happens if the users table does not have a user
 * with the specified email.
 *
 * \param[in] email  The user email.
 *
 * \return true if the email was sent, false otherwise.
 */
bool users_ui::resend_verification_email(QString const & email)
{
    users::users * users_plugin(users::users::instance());

    auto user_info(users_plugin->get_user_info_by_email(email));

    // to allow a "resend" without regenerating a new session, we save
    // the session identifier--since those are short lived, it will anyway
    // not be extremely useful, but some systems may use that once in a while
    if(!user_info.exists())
    {
        return false;
    }
    QString const session( user_info.get_value(users::name_t::SNAP_NAME_USERS_LAST_VERIFICATION_SESSION).stringValue() );
    if(session.isEmpty())
    {
        // no session, send a brand new verification email
        verify_email(email);
        return true;
    }

    QString current_email( user_info.get_value(users::name_t::SNAP_NAME_USERS_CURRENT_EMAIL).stringValue() );
    if(current_email.isEmpty())
    {
        // TODO: the email should always be defined, only we have
        //       legacy code which may skip on the matter and thus
        //       we want to have this fallback
        //
        current_email = email;
    }

    sendmail::sendmail::email e;

    // mark priority as High
    //
    e.set_priority(sendmail::sendmail::email::email_priority_t::EMAIL_PRIORITY_HIGH);

    // people would not be able to ever get a verification email without this one
    //
    e.add_parameter(sendmail::get_name(sendmail::name_t::SNAP_NAME_SENDMAIL_BYPASS_BLACKLIST), "true");

    // destination email address
    //
    e.add_header(sendmail::get_name(sendmail::name_t::SNAP_NAME_SENDMAIL_TO), current_email);

    // add the email subject and body using a page
    //
    e.set_email_path("admin/email/users/verify");

    // verification makes use of the existing session identifier
    //
    e.add_parameter(users::get_name(users::name_t::SNAP_NAME_USERS_VERIFY_EMAIL), session);

    // send the email
    //
    // really this just saves it in the database, the sendmail itself
    // happens on the backend; see sendmail::on_backend_action()
    //
    sendmail::sendmail::instance()->post_email(e);

    return true;
}


/** \brief Send an email to allow the user to change his password.
 *
 * This function generates an email and sends it to an active user. The
 * email is used to allow the user to change his password without having
 * to enter an old password.
 *
 * \param[in] user_info object containing the non-canonicalized user email and key.
 *
 * \todo replace user_key with user identifier
 */
void users_ui::forgot_password_email(users::users::user_info_t const & user_info)
{
    sendmail::sendmail::email e;

    // administrator can define this email address
    libdbproxy::value from(f_snap->get_site_parameter(get_name(snap::name_t::SNAP_NAME_CORE_ADMINISTRATOR_EMAIL)));
    if(from.nullValue())
    {
        from.setStringValue("contact@snapwebsites.com");
    }
    e.set_from(from.stringValue());

    // mark priority as High
    e.set_priority(sendmail::sendmail::email::email_priority_t::EMAIL_PRIORITY_HIGH);

    e.add_parameter(sendmail::get_name(sendmail::name_t::SNAP_NAME_SENDMAIL_BYPASS_BLACKLIST), "true");

    // destination email address
    e.add_header(sendmail::get_name(sendmail::name_t::SNAP_NAME_SENDMAIL_TO), user_info.get_user_email());

    // add the email subject and body using a page
    e.set_email_path("admin/email/users/forgot-password");

    // verification makes use of a session identifier
    sessions::sessions::session_info info;
    info.set_session_type(sessions::sessions::session_info::session_info_type_t::SESSION_INFO_USER);
    info.set_session_id(users::users::USERS_SESSION_ID_FORGOT_PASSWORD_EMAIL);
    info.set_plugin_owner(get_plugin_name()); // ourselves
    //info.set_page_path(); -- default is okay
    info.set_object_path( user_info.get_user_path(true) ); // sessions are always using the user id and not the email directly
    info.set_user_agent(f_snap->snapenv(snap::get_name(snap::name_t::SNAP_NAME_CORE_HTTP_USER_AGENT)));
    info.set_time_to_live(3600 * 8);  // 8 hours
    QString const session(sessions::sessions::instance()->create_session(info));
    e.add_parameter(users::get_name(users::name_t::SNAP_NAME_USERS_FORGOT_PASSWORD_EMAIL), session);

    // send the email
    //
    // really this just saves it in the database, the sendmail itself
    // happens on the backend; see sendmail::on_backend_action()
    sendmail::sendmail::instance()->post_email(e);
}


void users_ui::editor_widget_load_email_address(QDomElement & widget, QString const & id)
{
    // Get logged in user info
    //
    users::users::user_info_t const & user_info( users::users::instance()->get_user_info() );

    QDomDocument doc(widget.ownerDocument());
    QDomNodeList w(doc.elementsByTagName("widget"));
    int const max_widgets(w.size());
    for(int i(0); i < max_widgets; ++i)
    {
        QDomElement email_address_elm(w.at(i).toElement());
        if(email_address_elm.isNull())
        {
            // this should never happen!
            continue;
        }

        if( email_address_elm.attribute("id") == id )
        {
            // found it!
            QDomElement email_value_elm(doc.createElement("value"));
            QDomText email_text(doc.createTextNode(user_info.get_user_email()));
            email_value_elm.appendChild(email_text);
            email_address_elm.appendChild(email_value_elm);
            break;
        }
    }
}


void users_ui::on_init_editor_widget
    ( content::path_info_t & ipath
    , QString const & field_id
    , QString const & field_type
    , QDomElement & widget
    , libdbproxy::row::pointer_t row
    )
{
    NOTUSED(field_type);
    NOTUSED(row);

    // If some handling is done without the user logged in, then we can
    // add that here

    // what follows only interests logged in users
    users::users * users_plugin(users::users::instance());
    QString const user_path(users_plugin->user_is_logged_in() ? users_plugin->get_user_info().get_user_path(false) : "");
    if(user_path.isEmpty())
    {
        return;
    }

    QString const cpath(ipath.get_cpath());
    if(cpath.startsWith("change-email"))
    {
        if(field_id == "current_email_address" )
        {
            editor_widget_load_email_address(widget, field_id);
        }
    }
}


void users_ui::on_finish_editor_form_processing(content::path_info_t & ipath, bool & succeeded)
{
    if(!succeeded)
    {
        return;
    }

    if(ipath.get_cpath() != "admin/settings/users")
    {
        return;
    }

    content::content * content_plugin(content::content::instance());
    libdbproxy::table::pointer_t revision_table(content_plugin->get_revision_table());
    libdbproxy::row::pointer_t settings_row(revision_table->getRow(ipath.get_revision_key()));

    libdbproxy::value value;

    value = settings_row->getCell(users::get_name(users::name_t::SNAP_NAME_USERS_SOFT_ADMINISTRATIVE_SESSION))->getValue();
    if(value.size() == sizeof(int8_t))
    {
        f_snap->set_site_parameter(users::get_name(users::name_t::SNAP_NAME_USERS_SOFT_ADMINISTRATIVE_SESSION), value);
    }

    value = settings_row->getCell(users::get_name(users::name_t::SNAP_NAME_USERS_ADMINISTRATIVE_SESSION_DURATION))->getValue();
    if(value.size() == sizeof(int64_t))
    {
        f_snap->set_site_parameter(users::get_name(users::name_t::SNAP_NAME_USERS_ADMINISTRATIVE_SESSION_DURATION), value);
    }

    value = settings_row->getCell(users::get_name(users::name_t::SNAP_NAME_USERS_USER_SESSION_DURATION))->getValue();
    if(value.size() == sizeof(int64_t))
    {
        f_snap->set_site_parameter(users::get_name(users::name_t::SNAP_NAME_USERS_USER_SESSION_DURATION), value);
    }

    value = settings_row->getCell(users::get_name(users::name_t::SNAP_NAME_USERS_TOTAL_SESSION_DURATION))->getValue();
    if(value.size() == sizeof(int64_t))
    {
        f_snap->set_site_parameter(users::get_name(users::name_t::SNAP_NAME_USERS_TOTAL_SESSION_DURATION), value);
    }
}


void users_ui::on_save_editor_fields(editor::save_info_t & save_info)
{
    QString const cpath( save_info.ipath().get_cpath() );

    if( cpath.startsWith("change-email") && !save_info.has_errors() )
    {
        QString const new_email( editor::editor::instance()->get_post_value("email_address") );
        auto users_plugin(users::users::instance());
        users::users::user_info_t test_ui( users_plugin->get_user_info_by_email(new_email) );
        if( test_ui.exists() )
        {
            messages::messages::instance()->set_error(
                        "Email Address Already In Use!",
                        "The new email address you are trying to use is already in use on our system. Please use a different email address.",
                        QString("email address \"%1\" already in use!").arg(new_email),
                        false
                        );
        }
        else
        {
            // Save the new email address into the database
            //
            users::users::user_info_t & user_info(users_plugin->get_user_info());
            user_info.change_user_email( new_email );

            // TODO: implement the ability to send a confirmation email to the user first.
            // Also, it would be nice to send a follow up email to the old email indicating
            // the change.
        }
    }
}


SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
