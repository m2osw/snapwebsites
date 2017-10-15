// Snap Websites Server -- users logged info handling
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
 * \brief Users logged info structure handling.
 *
 * This file is the implementation of the user_logged_info_t class used
 * to let other plugins know the status of the user.
 */

#include "users.h"

#include <snapwebsites/poison.h>


SNAP_PLUGIN_EXTENSION_START(users)



/** \brief Initialize the user logged info object.
 *
 * This constructor saves the specified snap pointer in the
 * user_logged_info_t object.
 *
 * \param[in] snap  A pointer to the snap_child object.
 */
users::user_logged_info_t::user_logged_info_t(snap_child * snap, users::user_info_t const & user_info )
    : f_snap(snap)
    , f_user_info(user_info)
{
}


/** \brief The ipath to the user's account.
 *
 * This is the path_info_t to the user account in the
 * content/branch/revision area. The path is setup
 * immediately.
 *
 * \return A reference to the path_info_t object.
 */
content::path_info_t users::user_logged_info_t::user_ipath() const
{
    content::path_info_t ipath;
    ipath.set_path(f_user_info.get_user_path(false /*leading slash*/));
    return ipath;
}


/** \brief Save the user identifier.
 *
 * This function is used to save the user identifier in this object.
 * The identifier is a number which was assigned to the user when
 * he created his account.
 *
 * \param[in] identifier  The identifier of the user.
 *
 * \sa get_identifier()
 */
void users::user_logged_info_t::set_identifier(int64_t identifier)
{
    f_identifier = identifier;
}


/** \brief Retrieve the user identifier.
 *
 * This function returns the user identifier as defined by the
 * set_identifier() function.
 *
 * \return The logged in user identifier.
 *
 * \sa set_identifier()
 */
int64_t users::user_logged_info_t::get_identifier() const
{
    return f_identifier;
}


/** \brief Change the password policy.
 *
 * This function sets up the password policy in use for this login
 * process. This can be useful for the password plugin to check
 * various parameters that may change with time such as whether
 * the password of that user needs to be changed.
 *
 * At this time, I have two variants: the "users" for regular users
 * and the "oauth2" policy when logging in with a software. The
 * main difference between both will probably be the fact that
 * the "oauth2" password will not time out.
 *
 * \param[in] policy  The password policy used while logging the user in.
 *
 * \sa get_password_policy()
 */
void users::user_logged_info_t::set_password_policy(QString const & password_policy)
{
    f_password_policy = password_policy;
}


/** \brief Retrieve the password policy.
 *
 * This function returns the name of the password policy in use while
 * logging the user in.
 *
 * The password policy should not be changed to anything else than "users"
 * with users who have administrative rights on your website.
 *
 * \return The name of the user policy used to log in.
 *
 * \sa set_password_policy()
 */
QString const & users::user_logged_info_t::get_password_policy() const
{
    return f_password_policy;
}

/** \brief Defines the user info object.
 *
 * This function sets the user_info_t member to the specified parameter.
 * The user info is not immediately available. We set it right before calling the
 * user_logged_in() signal, but it is not set earlier because we do not
 * have it available right away.
 *
 * \param[in] user_info  The info of the user account.
 *
 * \sa get_user_info()
 */
void users::user_logged_info_t::set_user_info(user_info_t const & user_info)
{
    f_user_info = user_info;
}


/** \brief Retrieve the user info object.
 *
 * This function returns the info of the user that was just logged in.
 *
 * \return The user info object
 *
 * \sa set_user_info()
 */
users::user_info_t const & users::user_logged_info_t::get_user_info() const
{
    return f_user_info;
}


/** \brief Mark that the user has to change his password.
 *
 * This function is generally used internally to request that the
 * user changes his password. This is called when the system detects
 * that the user status is set to "PASSWORD".
 *
 * This function does not change the user's status.
 *
 * \sa is_password_change_required()
 * \sa force_user_to_change_password()
 */
void users::user_logged_info_t::force_password_change()
{
    f_force_password_change = true;
}


/** \brief Mark that the user has to change his password.
 *
 * This function requests that the user changes his password,
 * just like force_password_change() and it changes the status
 * of the user to "PASSWORD" meaning that whether or not the
 * plugin that genrated this request is removed in between,
 * the user will still be required to change his password
 * until he does.
 *
 * \todo
 * We probably want to change that PASSWORD status with a separate
 * name because otherwise we cannot have a new or some other user
 * forced to change his password.
 *
 * \sa is_password_change_required()
 * \sa force_password_change()
 */
void users::user_logged_info_t::force_user_to_change_password()
{
    // here we have to:
    //
    // (1) mark that a plugin just requested that the password
    //     is required
    //
    // (2) add the link so we force that change on a future
    //     login request if such occurs; however, we do so
    //     only if the status is currently "VALID" (i.e.
    //     no status link, meaning that the user is considered
    //     valid)
    //
    force_password_change();

    // first check whether the link exists
    //
    content::path_info_t ipath(user_ipath());
    QString const link_name(get_name(name_t::SNAP_NAME_USERS_STATUS));
    links::link_info user_status_info(link_name, true, ipath.get_key(), ipath.get_branch());
    QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(user_status_info));
    links::link_info status_info;
    if(!link_ctxt->next_link(status_info))
    {
        // no link, create one with "PASSWORD"
        //
        bool const source_unique(true);
        links::link_info source(link_name, source_unique, ipath.get_key(), ipath.get_branch(true, "xx"));

        QString const link_to(get_name(name_t::SNAP_NAME_USERS_STATUS));
        bool const destination_unique(false);
        content::path_info_t dpath;
        dpath.set_path(get_name(name_t::SNAP_NAME_USERS_PASSWORD_PATH));
        links::link_info destination(link_to, destination_unique, dpath.get_key(), dpath.get_branch());

        links::links::instance()->create_link(source, destination);
    }
}


/** \brief Returns whether the password has to be changed.
 *
 * This function returns true if the user has to change his password.
 *
 * This was requested with force_password_change() or
 * force_user_to_change_password().
 *
 * \return true if the user is to be forcibly sent to his
 *         "change my password" form.
 *
 * \sa force_user_to_change_password()
 * \sa force_password_change()
 */
bool users::user_logged_info_t::is_password_change_required() const
{
    return f_force_password_change;
}


/** \brief Force user to that URI.
 *
 * This function is used by plugins who want to send the user to
 * a specific page immediately after he logged in.
 *
 * Users who have to change their password will ignore this URI.
 * The order is as follow:
 *
 * \li If user has to change his password, go to "user/password".
 * \li If a plugin forces the user to a URI, go to that URI.
 *     (The URI this function defines.)
 * \li If the user was sent to the login screen from another page,
 *     return to that page.
 * \li If the administrator defined a redirect to a certain page
 *     on login, then send the user there.
 * \li Send the user to "user/me".
 *
 * \note
 * If your plugin has a low priority and the URI is not empty,
 * you may want to avoid overwriting the value.
 *
 * \warning
 * This function is offered because you want ALL the plugins to
 * run their user_logged_in() and therefore none of them can
 * directly call the snap_child::page_redirect() function since
 * that function never returns.
 *
 * \param[in] uri  The URI where the user should be sent on log in.
 *
 * \sa get_uri()
 * \sa snap_child::page_redirect()
 */
void users::user_logged_info_t::set_uri(QString const & uri) const
{
    f_uri = uri;
}


/** \brief Retrieve the plugin defined redirect URI.
 *
 * This function reads the URI used to redirect the user on log in.
 * This is dynamically defined by plugins that require the user
 * to go to a certain page. This is generally more effective than
 * letting the system send the user to the default ("user/me") and
 * then force redirect from that page to the "correct" page as
 * per that plugin.
 *
 * \return The URI where the user is to be sent after a login.
 *
 * \sa set_uri()
 * \sa snap_child::page_redirect()
 */
QString const & users::user_logged_info_t::get_uri() const
{
    return f_uri;
}



SNAP_PLUGIN_EXTENSION_END()

// vim: ts=4 sw=4 et
