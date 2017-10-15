// Snap Websites Server -- manage sessions for users, forms, etc.
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
 * \brief Session handling.
 *
 * Sessions are used to track anonymous and logged in users. Especially,
 * the users plugin make use of sessions.
 *
 * The "form" and "editor" plugins use sessions too so as to avoid robots
 * that just POST content. This is because a form includes a session
 * reference which changes for each user each time a form is loaded.
 * If a robot just sends a POST, it will not have a valid session
 * reference and it will be refused.
 *
 * Other plugins are welcome to make use of sessions, although, if possible
 * any data to carry for a user over multiple accesses should make use of
 * the attach_to_session() and detach_from_session() functions available in
 * the "users" plugin.
 *
 * Sessions include four main things:
 *
 * \li The plugin name, an identifier, a key, a random number
 *
 * We expect the name of the plugin owning this session (i.e. when
 * the "users" plugin creates a session, it saves "users" as the
 * owner of that session.) This allows for the session plugin to
 * be used with any plugin and still be safe by distinguishing
 * whether a certain session is owned by such and such plugin.
 *
 * The plugin is also expected to define an identifier for the
 * session. This identifier can be used by the plugin to know what
 * the session represents (i.e. the "users" plugin used that
 * identifier to know which form it was dealing with when receiving
 * a POST back from the client.)
 *
 * Further a session includes an object path and a page path. However,
 * this may not really define the unicity of the session.
 *
 * The session plugin creates a unique key with the create_session()
 * function. This is used as the key to access the session again later.
 * (i.e. the key used to index the session in the database). This is
 * the key saved in your \<input> tag, a session attribute for the
 * "editor" plugin, the cookie for a user cookie.
 *
 * The size of the session key depends on the type of session you
 * choose to use.
 *
 * The session key is accompagned by a random number. Only at this point
 * we ignore the random number because in most cases it does not work very
 * well (i.e. the cookie does not always get properly updated and as a
 * result the client may hit the server with the wrong random number. This
 * is a problem when you do multiple accesses and all replies do not happen
 * before further calls are made and as a result some requests may include
 * the old random number.) We may look into fixing this problem at some
 * point because having a random number allows us to make long term
 * sessions much safer.
 *
 * \li Client Unicity Parameters
 *
 * In order to better identify the client, we save the client Agent
 * string. We know that the agent string may change between accesses,
 * however, we enforce it in some circumstances. For example, a user
 * who registers an account using Firefox, cannot then finish the
 * creation of the account using Chrome, the Agent string will change
 * and the second access will be refused.
 *
 * In long term session, the agent can be used to make sure that the
 * type of browser does not change. However, we cannot match the
 * string one to one because the versions are likely to change once
 * in a while.
 *
 * \li A time limit
 *
 * We actually handle several time fields to limit various things in a
 * session.
 *
 * * First we have a relative time, see set_time_to_live(), which is used
 *   to define the TTL to use in the Cassandra database. By default, this
 *   value is set to five minute.
 *  
 *   The user plugin changes this value to one year because we try to keep
 *   user sessions as long as possible. The one year limit comes from the
 *   fact that a cookie cannot last more than one year, as per HTTP 1.1.
 *  
 *   Note that a user session may be given a new identifier once in a while
 *   even when it did not yet time out as per this TTL information.
 *
 *   This parameter can be set to zero, in which case the time limit will
 *   be used instead. If both, the time limit and the TTL are zero, then
 *   the limit reverts back to the default five minutes.
 *
 * * Second we have a time limit. This is a date at which the session
 *   is considered out of date. This does not prevent you from using the
 *   session, only the user cannot write permanent, public, or
 *   administrative data to the database anymore. We use long sessions so
 *   that way we can keep track of certain data a user enters while not
 *   logged in such as his cart or various other choices on the website.
 *   Once the user logs in, that data can be saved in his account more
 *   permanently if necessary.
 *  
 *   If the time limit represents a date further in the future than
 *   "now + TTL", then the TTL is pushed back to match the time limit.
 *   In other words, the time limit is authoritative over the TTL.
 *
 * * Third we have a logged in date. This date is a short period for
 *   which the user is logged in as an administrator. You reach this
 *   state either with a special login form or every time you re-log
 *   in. This time is not updated when you hit the website. It can
 *   only be extended when a user re-log in.
 *
 *   The logged in date is clamped to the time limit if it is larger.
 *
 * * Forth we save the date when the session is last saved in the database.
 *   This can be retrieved with the sessions::session_info::get_date()
 *   function. Although you can change this value with the set_date(),
 *   the save_session() function ignores it. It always makes use of the
 *   snap_child::get_start_date() when saving that field in the database.
 *
 * \li User data fields.
 *
 * A session can be used to save your own data linked with the user or
 * other object that is linked with this session (like a form).
 *
 * The "sessions" plugin offers three functions to deal with such data:
 *
 * * attach_to_session() saves the specified data to the session. The
 *   name is expected to be a valid field name (i.e. the name of your
 *   plugin as the namespace two colons and the name of the field.)
 *   For example, the "users" plugin saves the HTTP REFERER in case
 *   we need to ask for the user log in to continue. That way we can
 *   send the user right back where he was.
 *
 * * detach_from_session() reads the specified data from the session
 *   and removes it from the session (i.e. read ONCE.) This is the
 *   opposite of the attach_to_session(). It removes the value from
 *   the session assuming that you do not need it anymore once done.
 *   The returned value is equal to the last value that was saved
 *   using the attach_to_session() with the name.
 *
 * * get_from_session() reads the specified data from the session,
 *   but keeps the value in the session for later reads. This is used
 *   to check a value from a session, but keep it for later.
 */

#include "sessions.h"

#include "../output/output.h"

#include <snapwebsites/log.h>
#include <snapwebsites/not_reached.h>
#include <snapwebsites/not_used.h>
#include <snapwebsites/plugins.h>

#include <QtCassandra/QCassandraValue.h>

#include <iostream>

#include <openssl/rand.h>

#include <snapwebsites/poison.h>


SNAP_PLUGIN_START(sessions, 1, 1)


/** \brief Get a fixed sessions plugin name.
 *
 * The sessions plugin makes use of different names in the database. This
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
    case name_t::SNAP_NAME_SESSIONS_CHECK_FLAGS:
        return "sessions::check_flags";

    case name_t::SNAP_NAME_SESSIONS_CREATION_DATE:
        return "sessions::creation_date";

    case name_t::SNAP_NAME_SESSIONS_DATE:
        return "sessions::date";

    case name_t::SNAP_NAME_SESSIONS_ID:
        return "sessions::id";

    case name_t::SNAP_NAME_SESSIONS_LOGIN_LIMIT:
        return "sessions::login_limit";

    case name_t::SNAP_NAME_SESSIONS_OBJECT_PATH:
        return "sessions::object_path";

    case name_t::SNAP_NAME_SESSIONS_PAGE_PATH:
        return "sessions::page_path";

    case name_t::SNAP_NAME_SESSIONS_PLUGIN_OWNER:
        return "sessions::plugin_owner";

    case name_t::SNAP_NAME_SESSIONS_RANDOM:
        return "sessions::random";

    case name_t::SNAP_NAME_SESSIONS_REMOTE_ADDR:
        return "sessions::remote_addr";

    case name_t::SNAP_NAME_SESSIONS_TABLE:
        return "sessions";

    case name_t::SNAP_NAME_SESSIONS_TIME_LIMIT:
        return "sessions::time_limit";

    case name_t::SNAP_NAME_SESSIONS_TIME_TO_LIVE:
        return "sessions::time_to_live";

    case name_t::SNAP_NAME_SESSIONS_USED_UP:
        return "sessions::used_up";

    case name_t::SNAP_NAME_SESSIONS_USER_AGENT:
        return "sessions::user_agent";

    default:
        // invalid index
        throw snap_logic_exception("invalid name_t::SNAP_NAME_SESSIONS_...");

    }
    NOTREACHED();
}


/** \brief Initialize the session info object.
 *
 * By default a session object is initialized with the following parameters:
 *
 * \li type -- SESSION_INFO_SECURE, the most secure type of session (also the
 *                                  slowest)
 * \li session id -- 0, a default session identifier; this is specific to the
 *                      plugins using this session and 0 is not expected to be
 *                      a valid session identifier for any plugin;
 * \li session_key -- "", the key or name of the session, it is set when you
 *                        call the create_session() function;
 * \li session_random -- 0, a random number regenerated each time you save the
 *                          session
 * \li plugin owner -- "", the name of the plugin that created this session and
 *                         that will process the results
 * \li page path -- "", the path to the page being managed
 *                      (should be set to get_site_key_with_slash())
 * \li object path -- "", the object being built with this session
 *                        (i.e. the user block for the small user log in form)
 * \li time to live -- 300, five minutes which is about right for a secure
 *                          session; this is often changed to one day (86400)
 *                          for standard forms; and to one week (604800) for
 *                          fully public forms (i.e. search form)
 * \li time limit -- 0 (not limited), limit (set in seconds) of when the
 *                   session goes out of date; this is an exact date when
 *                   live, but always reset all sessions at midnight the
 *                   session expires (i.e. you may give a session 1h no
 *                   matter what)
 */
sessions::session_info::session_info()
    //: f_type(SESSION_INFO_SECURE) -- auto-init
    //, f_session_id(0) -- auto-init
    //, f_session_key("") -- auto-init
    //, f_session_random("") -- auto-init
    //, f_plugin_owner("") -- auto-init
    //, f_page_path("") -- auto-init
    //, f_object_path("") -- auto-init
    //, f_user_agent("") -- auto-init
    //, f_time_to_live(300) -- auto-init (5 min.)
    //, f_time_limit(0) -- auto-init
    //, f_login_limit(0) -- auto-init
    //, f_date(0) -- auto-init
    //, f_creation_date(0) -- auto-init
    //, f_check_flags(CHECK_HTTP_USER_AGENT) -- auto-init
{
}


/** \brief Set the type of session.
 *
 * By default a session object is marked as a secure session (SESSION_INFO_SECURE.)
 *
 * We currently support the following session types:
 *
 * \li SESSION_INFO_SECURE
 *
 * This type of session is expected to have a very short time to live (i.e. 5 min. on an
 * e-commerce site payment area, 1h for a standard logged in user.) This type of session
 * uses 128 bits.
 *
 * \li SESSION_INFO_USER
 *
 * This type of session is expected to be used for user cookies when not accessing an
 * e-commerce site. This type of session uses 64 bits. It should not be used with long
 * lasting logged in users (i.e. cookies that last more than a few hours should use a
 * secure session with 128 bits to better avoid hackers.)
 *
 * \li SESSION_INFO_FORM
 *
 * The form type of session is used to add an identifier in forms that hackers cannot
 * easily determine but without taking any processing time to generate on the server.
 * This type of session should never be used for a cookie representing a logged in
 * user. However, it could be used to track anonymous users. This type of session
 * uses 32 bits.
 *
 * \param[in] type  The new type for this session object.
 *
 * \sa get_session_type()
 */
void sessions::session_info::set_session_type(session_info_type_t type)
{
    f_type = type;
}


/** \brief Define a session identifier.
 *
 * This function accepts a session identifier (a number) which represents
 * what this session is about (i.e. the user log in form may use 1 and
 * the user registration may use 2, etc.)
 *
 * The identifier is used when a dynamic identifier is not required to
 * determine what we're expected to do once we get the identifier.
 *
 * \param[in] id  The identifier of this session.
 *
 * \sa get_session_id()
 */
void sessions::session_info::set_session_id(session_id_t id)
{
    f_session_id = id;
}


/** \brief Define a session key.
 *
 * This function is set whenever you call the create_session() function.
 * You should never set this value yourself since you do not actually
 * have any control over that value from the outside.
 *
 * This key is exactly what is sent to the user via a cookie.
 *
 * \param[in] key  The key of this session.
 *
 * \sa get_session_key()
 */
void sessions::session_info::set_session_key(QString const & key)
{
    f_session_key = key;
}


/** \brief Generate a ramdon session key.
 *
 * This function is called once each time you call the save_session().
 * This number should be saved in your cookie along the get_session_key()
 * string. This number changes each time the user access the server but
 * it should always match. If a mismatch is found, then the session may
 * have been hacked.
 *
 * To retrieve the session random key number use the get_session_random().
 *
 * \sa get_session_random()
 */
void sessions::session_info::set_session_random()
{
    // generate the session identifier
    do
    {
        int r(RAND_bytes(reinterpret_cast<unsigned char *>(&f_session_random), sizeof(f_session_random)));
        if(r != 1)
        {
            throw sessions_exception_no_random_data("RAND_bytes() could not generate a random number.");
        }
        // make it always positive, just in case
        f_session_random &= 0x7FFFFFFF;
    }
    while(f_session_random == 0);
    // we avoid zero because pretty much whatever would represent zero in
    // a string... so that's not a good choice
}


/** \brief Set the ramdon session key.
 *
 * This function is used to set the random session value in this
 * object. This function is used whenever we load sessions from
 * the database.
 *
 * \param[in] random  The new random value to save in the session.
 *
 * \sa get_session_random()
 */
void sessions::session_info::set_session_random(int32_t random)
{
    f_session_random = random;
}


/** \brief Set the session owner which is the name of a plugin.
 *
 * This function defined the session owner as the name of the a plugin.
 * This is used by the different low level functions to determine which
 * of the plugins is responsible to process a request.
 *
 * \param[in] plugin_owner  The name of the owner of that plugin.
 *
 * \sa get_plugin_owner()
 */
void sessions::session_info::set_plugin_owner(QString const & plugin_owner)
{
    f_plugin_owner = plugin_owner;
}


/** \brief The path to the page where this session identifier is used.
 *
 * For session identifiers that are specific to a page (i.e. a form) this is used to
 * link the session to the page so a user cannot use the same session identifier on
 * another page.
 *
 * Note that the page session identifier is only used for a form that this page
 * represents. Forms that appear in blocks make use of the object path.
 *
 * For cookies that track people this parameter can remain empty for anonymous users
 * and it is set to the user page for logged in users. This way if someone attempts
 * to use the wrong session identifier we can detect it.
 *
 * \param[in] page_path  The path representing the page in question for this session.
 *
 * \sa get_page_path()
 * \sa set_object_path()
 */
void sessions::session_info::set_page_path(QString const & page_path)
{
    f_page_path = page_path;
}


/** \brief The path to the page where this session identifier is used.
 *
 * This is just a helper function which accepts a content path_info_t
 * object instead of a string. It just retrieves the cpath from the
 * object.
 *
 * \param[in,out] page_ipath  The path representing the page in question for this session.
 *
 * \sa get_page_path()
 * \sa set_object_path()
 */
void sessions::session_info::set_page_path(content::path_info_t & page_ipath)
{
    f_page_path = page_ipath.get_cpath();
}


/** \brief The path of the object displaying this content.
 *
 * This path represents the object being displayed. For example, the smaller user
 * log in form (i.e. the log in block) is shown on many pages. Because of that, we
 * cannot use the path to the page and instead we use the path to the object
 * (i.e. the user log in block.)
 *
 * The path should be specified only if it is going to be checked later.
 *
 * \param[in] object_path  The path of the object in question.
 *
 * \sa get_object_path()
 * \sa set_page_path()
 */
void sessions::session_info::set_object_path(QString const & object_path)
{
    f_object_path = object_path;
}


/** \brief Save the user agent for this session.
 *
 * This function is used to save the user agent in the session. This is
 * useful for one simple reason: if a hacker wants to do a session fixation
 * he has to also have the exact user agent from the user he wants to
 * hack. This is probably very easy when you are targetting someone in
 * particular. Much less likely to happen if the users being targetted are
 * "random" (the first who falls for it.)
 *
 * The cookie reloading discards sessions with non-matching session user
 * agents.
 *
 * \todo
 * It seems that saving the user agent as is in the database "is not very
 * secure". It also seems to me that if the database is compromised it
 * is a much bigger problem than the user agent in clear and thus we
 * probably don't have to worry about it (this is, of course, assuming that
 * you do not offer a plugin that can peek at any value in the database
 * and send it to the client...) Anyway, at this point it is saved in
 * clear text for that reason.
 *
 * \param[in] user_agent  The user agent defined by the end user.
 *
 * \sa get_user_agent()
 */
void sessions::session_info::set_user_agent(QString const & user_agent)
{
    f_user_agent = user_agent;
}


/** \brief Save the client Remote IP address for this session.
 *
 * This function saves the IP address of the session loaded from the
 * database.
 *
 * The IP address cannot be changed in this way. When you save the
 * session, we force the remote IP address from the snap_child
 * REMOTE_ADDR parameter. We use the following line of code to
 * define that value:
 *
 * \code
 *      value.setStringValue(f_snap->snapenv(snap::get_name(snap::name_t::SNAP_NAME_CORE_REMOTE_ADDR)));
 * \endcode
 *
 * \param[in] remote_addr  The remote IP address defined.
 *
 * \sa get_remote_addr()
 */
void sessions::session_info::set_remote_addr(QString const & remote_addr)
{
    f_remote_addr = remote_addr;
}


/** \brief The time to live of this session.
 *
 * All sessions have a maximum life time of five minutes by default. This function
 * is used to change the default to a smaller (although unlikely it can be done)
 * or a larger number.
 *
 * Sessions that run out of time do not get deleted immediately from the database,
 * but they are not considered valid so attempting to use them fails with a time
 * out error.
 *
 * For example, a log in form may have a time out of 5 minutes. If you wait more
 * time before logging in your account, the form times out and trying to log in
 * fails (include a JavaScript and you can clearly time out the client side as
 * well.) Imagine this simple scenario:
 *
 * \li User comes to your site
 * \li User clicks to go to the log in screen
 * \li User enters user name and password
 * \li User does not log in yet (gets a phone call...)
 * \li User leaves the computer
 * \li Another user comes to the computer
 * \li That other user just has to click the "Log In" button...
 * \li The account is compromised!
 *
 * The minimum time to live accepted is 1 minute and one second (61 or more.) But
 * you probably should never create a session of less than 5 minutes (500).
 *
 * Setting a sessiong time to live to 0 means that the session never expire. It
 * should really only be used when a form doesn't need to expire (i.e. a search
 * form) but even though, it is most likely not a good idea. Although it can be
 * used when you set a hard coded time limit on a session.
 *
 * \sa get_time_to_live()
 * \sa set_time_limit()
 */
void sessions::session_info::set_time_to_live(int32_t time_to_live)
{
    f_time_to_live = time_to_live;
}


/** \brief Limit the time by date.
 *
 * This function saves the time limit of a session to the specified date.
 * In this case, the date is absolute. We use the standard Unix date (i.e.
 * number of seconds since Jan 1, 1970.)
 *
 * After that date the session becomes invalid. It is not necessarilly
 * removed from the database, although if a users attempts to use it, it
 * will fail.
 *
 * In most cases you want to use the time_to_live parameter rather than a
 * hard coded Unix timestamp.
 *
 * When creating a session, the limit must represent at least 1 minute and
 * one second from now (now is defined as the start date defined in the
 * snap_child object.)
 *
 * A limit of zero means that the time limit is not used.
 *
 * \param[in] time_limit  The time, in seconds, when the session becomes
 *                        invalid (note that if the time to live is larger
 *                        than the session will continue to exist even after
 *                        the time limit).
 *
 * \sa get_time_limit()
 * \sa set_time_to_live()
 */
void sessions::session_info::set_time_limit(time_t time_limit)
{
    f_time_limit = time_limit;
}


/** \brief Limit the time for a full login session.
 *
 * Any semi-secure information is not visible/editable past this time
 * unless the user goes through a log in session.
 *
 * More or less, the session goes from a fully logged in registered user
 * to a mostly logged in user.
 *
 * \param[in] time_limit  The time when the secure logged in session
 *                        becomes invalid.
 *
 * \sa get_time_limit()
 */
void sessions::session_info::set_administrative_login_limit(time_t time_limit)
{
    f_login_limit = time_limit;
}


/** \brief Timestamp of the session.
 *
 * This function saves a date in the session. This function is used when
 * loading a session. Note that this value is NOT used when saving a
 * session. The session plugin simply uses f_snap->start_date() to set
 * this value when saving a session.
 *
 * \param[in] date  The date when the session was last saved.
 *
 * \sa get_time_limit()
 * \sa set_time_to_live()
 */
void sessions::session_info::set_date(int64_t date)
{
    f_date = date;
}


/** \brief Timestamp of when the session was created.
 *
 * This function defines the date when the session was created. The date is
 * in microseconds.
 *
 * This function is called only when a session gets loaded. It should not
 * be changed otherwise.
 *
 * \note
 * If you se the creation date of your session_info object before calling
 * the create_session() function, then the save_session() function ends
 * up NOT saving the session creation date which creates a problem on
 * the load_session() because the row will be empty. As a result, you
 * get an exception and the session is considered invalid.
 *
 * \exception
 * The session creation date cannot be changed in the database, to
 * prevent such from happening the date cannot be set to 0 which is
 * a signal for the save_session() function that the session is brand
 * new.
 *
 * \param[in] date  The date when the session was created.
 *
 * \sa get_date()
 * \sa set_date()
 * \sa get_creation_date()
 */
void sessions::session_info::set_creation_date(int64_t date)
{
    if(date <= 0)
    {
        throw sessions_exception_invalid_range("sessions.cpp: sessions::session_info::set_creation_date() was called with date set to 0 or less.");
    }

    f_creation_date = date;
}


/** \brief Force the specified checks for this session.
 *
 * Sessions support a certain number of checks that are not mandatory.
 * It is possible, by code, to add and remove those checks.
 *
 * This function is generally only used when loading a session to restore
 * the value that was saved in the database. You probably want to use
 * the add_check_flags() and remove_check_flags() functions instead
 * because that way you do not take the risk of disturbing flags that you
 * do not know anything about (possibly because they were added after
 * you wrote your code.)
 *
 * Note that the add and remove functions can both be used to retrieve
 * the current set of flags by passing 0 as parameter:
 *
 * \code
 *      check_flag_t flags(info.add_check_flags(0));
 * \endcode
 *
 * \param[in] flags  The new check flags.
 *
 * \sa add_check_flags()
 * \sa remove_check_flags()
 */
void sessions::session_info::set_check_flags(check_flag_t flags)
{
    f_check_flags = flags;
}


/** \brief Force the specified checks for this session.
 *
 * Sessions support a certain number of checks that are not mandatory.
 * It is possible, by code, to add those checks.
 *
 * Some checks are set by default and can be removed. Use the
 * remove_check_flags() in that case.
 *
 * The checks that are options are defined as flags:
 *
 * \li CHECK_HTTP_USER_AGENT -- make sure to check the user agent string.
 *
 * \param[in] flags  The flags to add to the list of checks.
 *
 * \return The current set of flags.
 *
 * \sa set_check_flags()
 * \sa remove_check_flags()
 */
sessions::session_info::check_flag_t sessions::session_info::add_check_flags(check_flag_t flags)
{
    return f_check_flags |= flags;
}


/** \brief Remove the specified checks for this sessions.
 *
 * Sessions support a certain number of checks that are not mandatory.
 * It is possible, by code, to remove those checks.
 *
 * Some checks are not set by default and can be added. Use the
 * add_check_flags() in that case.
 *
 * For a list of checks that are currently optional, read the documentation
 * of the add_check_flags() function instead.
 *
 * Note that to clear a flag, its bit must be set. For for example, to
 * clear the CHECK_HTTP_USER_AGENT flag you would do:
 *
 * \code
 *      sessions::session_info info;
 *      ...
 *      info.remove_check_flags(info.CHECK_HTTP_USER_AGENT):
 * \endcode
 *
 * \param[in] flags  The flags to clear from the list of checks.
 *
 * \return The current set of flags.
 *
 * \sa set_check_flags()
 * \sa add_check_flags()
 */
sessions::session_info::check_flag_t sessions::session_info::remove_check_flags(check_flag_t flags)
{
    return f_check_flags &= ~flags;
}


/** \brief Retrieve the type of this session.
 *
 * This function is used to retrieve the type of this session.
 *
 * More details can be found in the set_session_type() function.
 *
 * \return The type of the session.
 *
 * \sa set_session_type()
 */
sessions::session_info::session_info_type_t sessions::session_info::get_session_type() const
{
    return f_type;
}


/** \brief Define a session identifier.
 *
 * This function returns the session identifier of this session.
 *
 * See the set_session_id() function for more information.
 *
 * \return The identifier of this session.
 *
 * \sa set_session_id()
 */
sessions::session_info::session_id_t sessions::session_info::get_session_id() const
{
    return f_session_id;
}


/** \brief Retrieve the session key.
 *
 * This function returns the session key of this session. The key is what
 * is randomly generated and used as the key of the row holding the
 * session data.
 *
 * See the set_session_key() function for more information.
 *
 * \return The key of this session.
 *
 * \sa set_session_key()
 */
QString const & sessions::session_info::get_session_key() const
{
    return f_session_key;
}


/** \brief Retrieve the session random key.
 *
 * This function returns the session random key which changes each time the
 * session gets saved. It is in general saved in the user's cookie and
 * checked on reload, and it should always match!
 *
 * See the set_session_random() function for more information.
 *
 * \return The random key of this session.
 *
 * \sa set_session_random()
 */
int32_t sessions::session_info::get_session_random() const
{
    return f_session_random;
}


/** \brief Set the session owner which is the name of a plugin.
 *
 * This function defined the session owner as the name of the a plugin.
 * This is used by the different low level functions to determine which
 * of the plugins is responsible to process a request.
 *
 * \sa set_plugin_owner()
 */
QString const& sessions::session_info::get_plugin_owner() const
{
    return f_plugin_owner;
}


/** \brief Retrieve the path of the page linked to this session.
 *
 * This function is used to retrieve the type of this session.
 *
 * More details can be found in the set_session_type() function.
 *
 * \return The type of the session.
 *
 * \sa set_page_path()
 * \sa get_object_path()
 */
QString const & sessions::session_info::get_page_path() const
{
    return f_page_path;
}


/** \brief Get the path of the attached object.
 *
 * A session may be created for a specific object that may appear on
 * many different pages. This object path can be retrieved using
 * this function.
 *
 * \return This session object path.
 *
 * \sa set_object_path()
 * \sa get_page_path()
 */
QString const & sessions::session_info::get_object_path() const
{
    return f_object_path;
}


/** \brief Get the user agent of the attached object.
 *
 * A session is always created for a specific user agent. This means a
 * user cannot take his credential from one browser to another browser
 * and continue as if he was logged in with the new browser. Instead
 * he has to properly log in with each browser as you would normally
 * expect. However, this is particularly useful against session ID
 * fixation attacks since the attacker must first be using the exact
 * same User Agent string as his victim.
 *
 * \return This session User Agent.
 *
 * \sa set_user_agent()
 */
QString const & sessions::session_info::get_user_agent() const
{
    return f_user_agent;
}


/** \brief Get the remote address of the attached object.
 *
 * A session is always created with the last remote IP address of the
 * client saved in it.
 *
 * Note that in most cases, quick accesses will have the same IP address
 * but you cannot hope that a client will not be given a new IP once
 * in a while.
 *
 * That being said, while loading one page, you are more than likely
 * going to receive all the requests from the exact same IP address.
 *
 * \return This session remote IP address when it was last saved.
 *
 * \sa set_remote_addr()
 */
QString const & sessions::session_info::get_remote_addr() const
{
    return f_remote_addr;
}


/** \brief Get the time to live of this session.
 *
 * This function returns the time this session will live. After that time, use of
 * the session fails (even if everything else is valid.)
 *
 * See the set_time_to_live() function for more information.
 *
 * \return The time to live of this session in seconds.
 *
 * \sa set_time_to_live()
 * \sa get_time_limit()
 */
int32_t sessions::session_info::get_time_to_live() const
{
    return f_time_to_live;
}


/** \brief Get the time limit of this session.
 *
 * This function returns the Unix date when the session goes out of business.
 * After that specific date, the session is considered invalid.
 *
 * See the set_time_limit() function for more information.
 *
 * \return The session Unix time limit (so it is in seconds).
 *
 * \sa set_time_limit()
 * \sa get_time_to_live()
 */
time_t sessions::session_info::get_time_limit() const
{
    return f_time_limit;
}


/** \brief Get the time limit of this logged in session.
 *
 * This function returns the Unix date when the logged in user is now
 * just considered a registered user. This date is not updated each
 * time the user accesses the site. This prevents an administrator from
 * staying logged in forever, but he can still quickly moderate the
 * website and add new content.
 *
 * \return The log in session Unix time limit.
 *
 * \sa set_administrative_login_limit()
 */
time_t sessions::session_info::get_administrative_login_limit() const
{
    return f_login_limit;
}


/** \brief Get the date when this session was last saved.
 *
 * This function returns the Unix date in microseconds when the session
 * was last saved in Cassandra. This is used by the plugins using sessions
 * to know whether a new random number should be used.
 *
 * If the session was never saved, the function returns zero.
 *
 * \return The Unix time in microseconds when the session was last saved.
 *
 * \sa set_date()
 */
int64_t sessions::session_info::get_date() const
{
    return f_date;
}


/** \brief Get the timestamp of when the session was created.
 *
 * This function returns the date when the session was created. The date is
 * in microseconds.
 *
 * \return The date, in microseconds, when the session was created.
 *
 * \sa get_date()
 * \sa set_date()
 * \sa set_creation_date()
 */
int64_t sessions::session_info::get_creation_date() const
{
    return f_creation_date;
}


/** \brief Get session type as a string.
 *
 * This function converts the session type to a string. This is particularly
 * useful to generate errors.
 *
 * \exception
 * When the input type is not valid, this exception is generated.
 *
 * \param[in] type  The type to convert to a string.
 *
 * \return A string representing the type.
 */
char const * sessions::session_info::session_type_to_string(session_info_type_t type)
{
    char const * type_names[] =
    {
        "SESSION_INFO_SECURE",
        "SESSION_INFO_USER",
        "SESSION_INFO_FORM",
        "SESSION_INFO_VALID",
        "SESSION_INFO_MISSING",
        "SESSION_INFO_OUT_OF_DATE",
        "SESSION_INFO_USED_UP",
        "SESSION_INFO_INCOMPATIBLE"
    };
    if(static_cast<uint32_t>(type) >= sizeof(type_names) / sizeof(type_names[0]))
    {
        throw sessions_exception_invalid_range("sessions.cpp: type is invalid while calling session_type_to_string()");
    }
    return type_names[static_cast<int>(type)];
}


/** \brief Define the TTL for a Cassandra value.
 *
 * This function defines the TTL value for a QCassandraValue object.
 *
 * The TTL is calculated from the time limit and the time to live.
 * The time to live has priority and if longer than the time limit,
 * it gets used and the time limit is totally ignored.
 *
 * If the time limit is after what now plus the time to live
 * represents then the TTL is set to the time limit.
 *
 * You may reuse the same value variable over and over again once the
 * TTL was setup once. It will be a lot faster than calling this
 * function each time you have to save a new value.
 *
 * \param[in,out] value  The QCassandraValue which TTL will be set.
 */
int32_t sessions::session_info::get_ttl(int64_t now) const
{
    // define timestamp for the session value in seconds
    int64_t timestamp(0);
    if(f_time_limit == 0)
    {
        if(f_time_to_live == 0)
        {
            // the default time to live is five minutes
            // as defined in the time_to_live_t controlled variable.
            //
            timestamp = now + DEFAULT_TIME_TO_LIVE;
        }
        else
        {
            timestamp = now + f_time_to_live;
        }
    }
    else
    {
        if(f_time_to_live == 0)
        {
            timestamp = f_time_limit;
        }
        else
        {
            timestamp = now + f_time_to_live;
            if(timestamp < f_time_limit)
            {
                // keep the largest dead line time
                timestamp = f_time_limit;
            }
        }
    }
    // keep it in the database for 1 more day than what we need it for
    // the difference should always fit 32 bits
    int64_t const ttl(timestamp + 86400LL - now);
    if(ttl < 0LL || ttl > 0x7FFFFFFFLL)
    {
        throw sessions_exception_invalid_range(QString("sessions::session_info::get_ttl(): the session computed ttl %1 is out of bounds (time to live: %2, time limit: %3).")
                        .arg(ttl).arg(f_time_to_live).arg(f_time_limit));
    }

    // we checked and know it fits 32 bits so this cast is safe
    //
    return static_cast<int32_t>(ttl);
}






/** \brief Initialize the sessions plugin.
 *
 * This function is used to initialize the sessions plugin object.
 */
sessions::sessions()
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Clean up the sessions plugin.
 *
 * Ensure the sessions object is clean before it is gone.
 */
sessions::~sessions()
{
}


/** \brief Get a pointer to the sessions plugin.
 *
 * This function returns an instance pointer to the sessions plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the sessions plugin.
 */
sessions * sessions::instance()
{
    return g_plugin_sessions_factory.instance();
}


/** \brief A path or URI to a logo for this plugin.
 *
 * This function returns a 64x64 icons representing this plugin.
 *
 * \return A path to the logo.
 */
QString sessions::icon() const
{
    return "/images/sessions/sessions-logo-64x64.png";
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
QString sessions::description() const
{
    return "The sessions plugin is used by many other plugins to generate"
          " session identifiers and save information about the given session."
          " This is useful for many different reasons. In case of a user, a"
          " session is used to make sure that the same user comes back to the"
          " website. It is also used by forms to make sure that a for submission"
          " is valid.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString sessions::dependencies() const
{
    return "|layout|output|";
}


/** \brief Check whether updates are necessary.
 *
 * This function updates the database when a newer version is installed
 * and the corresponding updates where not run yet.
 *
 * This works for newly installed plugins and older plugins that were
 * updated.
 *
 * \param[in] last_updated  The UTC Unix date when this plugin was last updated (in micro seconds).
 *
 * \return The UTC Unix date of the last update of this plugin.
 */
int64_t sessions::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2015, 5, 25, 17, 40, 0, clean_session_table);
    SNAP_PLUGIN_UPDATE(2016, 2, 21, 16, 30, 40, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Update the content with our references.
 *
 * Send our content to the database so the system can find us when a
 * user references our pages.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added to the database by this update (in micro-seconds).
 */
void sessions::content_update(int64_t variables_timestamp)
{
    NOTUSED(variables_timestamp);

    content::content::instance()->add_xml(get_plugin_name());
}


/** \brief Initialize the sessions.
 *
 * This function terminates the initialization of the sessions plugin
 * by registering for different events it supports.
 *
 * \param[in] snap  The child handling this request.
 */
void sessions::bootstrap(snap_child * snap)
{
    f_snap = snap;

    SNAP_LISTEN(sessions, "server", server, table_is_accessible, _1, _2);
}


/** \brief Clean up the sessions table from used up sessions.
 *
 * The session::used_up field is added to sessions as a marker to
 * avoid loading such a session (it was used up.)
 *
 * The field was supposed to be given the TTL of the other fields and
 * thus automatically get dropped with time. However, I used the same
 * QCassandraValue to test whether used_up exists. If not, I would reuse
 * that value which had lost its TTL.
 *
 * This upgrade goes through the table and check for sessions that are
 * marked as used up. When finding such a session, the function either
 * drops the column (i.e. no other columns exist) or it re-write the
 * used_up value with the same TTL as the other fields.
 *
 * \important
 * This was a one time update process. It is not used by newer
 * implementations.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added
 *            to the database by this update (in micro-seconds).
 */
void sessions::clean_session_table(int64_t variables_timestamp)
{
    NOTUSED(variables_timestamp);

    QString const used_up(get_name(name_t::SNAP_NAME_SESSIONS_USED_UP));
    QString const id(get_name(name_t::SNAP_NAME_SESSIONS_ID));

    QtCassandra::QCassandraTable::pointer_t sessions_table(get_sessions_table());
    sessions_table->clearCache();
    auto row_predicate = std::make_shared<QtCassandra::QCassandraRowPredicate>();
    row_predicate->setCount(1000);
    for(;;)
    {
        uint32_t const count(sessions_table->readRows(row_predicate));
        if(count == 0)
        {
            // no more sessions to process
            break;
        }
        QtCassandra::QCassandraRows const rows(sessions_table->rows());
        for(QtCassandra::QCassandraRows::const_iterator o(rows.begin());
                o != rows.end(); ++o)
        {
            // do not work on standalone websites
            if((*o)->exists(used_up))
            {
                if((*o)->exists(id))
                {
                    // read the value so that way we get the TTL
                    QtCassandra::QCassandraValue value((*o)->cell(id)->value());
                    value.setCharValue(1);
                    (*o)->cell(used_up)->setValue(value);
                }
                else
                {
                    // this is the last field, delete it
                    (*o)->dropCell(used_up);
                }
            }
        }
    }
}


/** \brief Initialize the sessions table.
 *
 * This function creates the sessions table if it does not exist yet.
 * Otherwise it simple returns the existing Cassandra table.
 *
 * If the function is not able to create the table an exception is raised.
 *
 * \note
 * At this point this function is private because we do not think it should
 * directly be accessible from the outside. Note that this table includes
 * all the sessions for all the websites running on a system!
 *
 * \return The pointer to the sessions table.
 */
QtCassandra::QCassandraTable::pointer_t sessions::get_sessions_table()
{
    return f_snap->get_table(get_name(name_t::SNAP_NAME_SESSIONS_TABLE));
}


/** \brief Generate the actual content of the statistics page.
 *
 * This function generates the contents of the statistics page of the
 * sessions plugin.
 *
 * \param[in,out] ipath  The path to this page.
 * \param[in,out] page  The page element being generated.
 * \param[in,out] body  The body element being generated.
 */
void sessions::on_generate_main_content(content::path_info_t & ipath, QDomElement & page, QDomElement & body)
{
    // generate the statistics in the body then call the content generator
    // (how do we do that at this point? do we assume that the backend takes
    // care of it?)
    output::output::instance()->on_generate_main_content(ipath, page, body);
}


/** \brief Create a new session.
 *
 * This function creates a new session using the specified information.
 * Later one can load a session to verify the validity of some data
 * such as a form post or a user cookie.
 *
 * If you modify a session, you can save it again with the save_session()
 * function. The info must include a session key. A key is generated by
 * the create_session() or loaded by the load_session() function.
 *
 * The function returns the session identifier which includes letters
 * and digits (A-Za-z0-9).
 *
 * The session must include a path (either the page or object path as
 * defined in the session \p info parameter.) This path is used as the
 * key to access the session information. It is also the path used to
 * verify the session again when necessary. If both, the page and the
 * object are defined, then the page has priority and it becomes the
 * session database key.
 *
 * \note
 * The info receives the result key that you can later retrieve using
 * the get_session_key(). The key does NOT include the website URI.
 *
 * \note
 * The bit size of the source of the entropy (random values) is more
 * important than the bit size of the actual session token. For example,
 * an MD5 hash produces a 128 bit value. However, the MD5 hash of
 * incremental values, a timestamp, or 8-bit random numbers are each
 * insecure because the source of the random values can be easily predicted.
 * Consequently, the 128 bit size does not represent an accurate measure
 * of the session token. The minimum size of the entropy source is 32 bits,
 * although larger pools (48 or 64 bits) may be necessary for sites with
 * over 10,000 concurrent users per hour.
 *
 * \warning
 * The function checks that the time the session will live is over 1
 * minute. Anything smaller than that and you get a throw.
 *
 * \bug
 * We are using the OpenSSL RAND_bytes() function that makes use of a
 * context to know which randomizer to use. We should look into forcing
 * a specific generator when called.
 *
 * \param[in,out] info  The session information.
 *
 * \return The new session key.
 *
 * \sa save_session
 * \sa load_session
 */
QString sessions::create_session(session_info & info)
{
    // creating a session of less than 1 minute?!
    time_t const time_limit(info.get_time_limit());
    int32_t const time_to_live(info.get_time_to_live());
    int64_t const now(f_snap->get_start_time());
    if((time_limit != 0 && time_limit <= now + 60)
    || (time_to_live != 0 && time_to_live <= 60))
    {
        throw sessions_exception_invalid_parameter("you cannot create a session of 1 minute or less.");
    }

    // make sure that we have at least one path defined
    // (this is our session key so it is required)
    if(info.get_page_path().isEmpty() && info.get_object_path().isEmpty())
    {
        throw sessions_exception_invalid_parameter("any session must have at least one path defined.");
    }

    if(info.get_user_agent().isEmpty())
    {
        throw sessions_exception_invalid_parameter("all sessions must have a user agent specified.");
    }

    if(info.get_creation_date() != 0)
    {
        throw sessions_exception_invalid_parameter("the sessions plugin is the only one in charge of setting up the creation date of the session.");
    }

    // TODO? Need we set a specific OpenSSL random generator?
    //       Although the default works for session identifiers
    //       someone could change that under our feet (since it
    //       looks like those functions have a global context)

    // the maximum size we currently use is 16 bytes (128 bits)
    unsigned char buf[16];

    int size(0);
    switch(info.get_session_type())
    {
    case session_info::session_info_type_t::SESSION_INFO_SECURE:
        size = 16;
        break;

    case session_info::session_info_type_t::SESSION_INFO_USER:
        size = 8;
        break;

    case session_info::session_info_type_t::SESSION_INFO_FORM:
        size = 4;
        break;

    default:
        throw snap_logic_exception("used an undefined session type in create_session()");

    }

    // generate the session identifier
    int const r(RAND_bytes(buf, size));
    if(r != 1)
    {
        throw sessions_exception_no_random_data("RAND_bytes() could not generate a random number.");
    }

    // make the key specific to that website and append the session identifier
    QString result;
    for(int i(0); i < size; ++i)
    {
        QString const hex(QString("%1").arg(static_cast<int>(buf[i]), 2, 16, static_cast<QChar>('0')));
        result += hex;
    }
    info.set_session_key(result);

    save_session(info, true);

    return result;
}


/** \brief Save the session.
 *
 * If you loaded a session, or created a session and made changes to
 * the session parameters (using one of the set_...() functions) then
 * you'll have to save the session again by calling this function.
 *
 * Forgetting to save the session has two side effects:
 *
 * \li All your changes are lost
 * \li The session deadline is not reset to match the time and date
 *     when those last changes were made and thus the session will
 *     be deleted early
 *
 * Note that the random session key is regenerated each time you call
 * this function (hence the \p info parameter is an in,out parameter.)
 *
 * By default the new_random flag should be true because it makes sense to
 * generate a new random session number on each access by the client.
 * However, that assumes that the user accesses the website in a very
 * serialized manner and even if the user doesn't open 3 windows at the
 * same time, that won't work because we server ALL the data (i.e. HTML,
 * JavaScript, CSS, Images, etc.) and browsers tend to send the requests
 * in parallel (Firefox has 2 at this time, but Chrome can have many more.)
 * Since we cannot really know whether the request is for a new HTML page
 * or any other part of a web page, it is just not really possible to
 * send a new random number everytime.
 *
 * There are two reasons to send a new random number:
 *
 * \li The user logs in at some level (we can have multiple log in levels!)
 * \li The user was inactive for long enough (i.e. over a minute?)
 *
 * \todo
 * Do some tests and see whether we can change the time_limit and TTL
 * values to zero after we did a load_session() and before we do a
 * save_session(). If it is possible, then we have to check that special
 * case in the save_session() function (it is checked in the create_session()
 * but not here!) because the load_session() hopes that this is always true.
 *
 * \param[in,out] info  The session info to save.
 * \param[in] new_random  Whether the session should be given a new random number.
 */
void sessions::save_session(session_info & info, bool const new_random)
{
    if(new_random)
    {
        info.set_session_random();
    }

    QString const key(f_snap->get_website_key() + "/" + info.get_session_key());

    QtCassandra::QCassandraTable::pointer_t table(get_sessions_table());
    QtCassandra::QCassandraRow::pointer_t row(table->row(key));

    QtCassandra::QCassandraValue value;
    value.setTtl(info.get_ttl(f_snap->get_start_time()));

    value.setInt32Value(info.get_session_id());
    row->cell(get_name(name_t::SNAP_NAME_SESSIONS_ID))->setValue(value);

    value.setStringValue(info.get_plugin_owner());
    row->cell(get_name(name_t::SNAP_NAME_SESSIONS_PLUGIN_OWNER))->setValue(value);

    value.setStringValue(info.get_page_path());
    row->cell(get_name(name_t::SNAP_NAME_SESSIONS_PAGE_PATH))->setValue(value);

    value.setStringValue(info.get_object_path());
    row->cell(get_name(name_t::SNAP_NAME_SESSIONS_OBJECT_PATH))->setValue(value);

    value.setStringValue(info.get_user_agent());
    row->cell(get_name(name_t::SNAP_NAME_SESSIONS_USER_AGENT))->setValue(value);

    value.setInt32Value(info.get_time_to_live());
    row->cell(get_name(name_t::SNAP_NAME_SESSIONS_TIME_TO_LIVE))->setValue(value);

    value.setInt64Value(info.get_time_limit());
    row->cell(get_name(name_t::SNAP_NAME_SESSIONS_TIME_LIMIT))->setValue(value);

    value.setInt64Value(info.get_administrative_login_limit());
    row->cell(get_name(name_t::SNAP_NAME_SESSIONS_LOGIN_LIMIT))->setValue(value);

    value.setInt64Value(f_snap->get_start_date());
    row->cell(get_name(name_t::SNAP_NAME_SESSIONS_DATE))->setValue(value);

    if(info.get_creation_date() == 0)
    {
        // save it in the info structure as well
        info.set_creation_date(f_snap->get_start_time());
        value.setInt64Value(f_snap->get_start_date());
        row->cell(get_name(name_t::SNAP_NAME_SESSIONS_CREATION_DATE))->setValue(value);
    }

    value.setStringValue(f_snap->snapenv(snap::get_name(snap::name_t::SNAP_NAME_CORE_REMOTE_ADDR)));
    row->cell(get_name(name_t::SNAP_NAME_SESSIONS_REMOTE_ADDR))->setValue(value);

    value.setInt32Value(info.get_session_random());
    row->cell(get_name(name_t::SNAP_NAME_SESSIONS_RANDOM))->setValue(value);

    value.setInt64Value(info.add_check_flags(0));
    row->cell(get_name(name_t::SNAP_NAME_SESSIONS_CHECK_FLAGS))->setValue(value);
}


/** \brief Load a session previously created with create_session().
 *
 * This function loads a session that one previously created with the
 * create_session() function.
 *
 * The info parameter gets reset by the function, which means that all
 * the input values are overwritten. It then sets the session type to
 * one of the following values to determine the validity of the data:
 *
 * \li SESSION_INFO_VALID -- the session is considered valid and it can
 * be used safely; the session info is perfectly valid
 *
 * \li SESSION_INFO_MISSING -- the session is missing; in most cases this
 * is because a hacker attempted to post a session and it was already
 * discarded (i.e. the hacker is reusing the same identifier over and
 * over again) or the hacker used a random number that doesn't exist in
 * the database; the session info is not valid
 *
 * \li SESSION_INFO_USED_UP -- the session was already used; it is not
 * possible to re-use it again; the session info is otherwise valid
 *
 * \li SESSION_INFO_INCOMPATIBLE -- the session is not compatible as some
 * parameters do not match the expected values; this can be set by the
 * caller (at this point this very function doesn't use this error code);
 * the session info is other valid
 *
 * \warning
 * You must check the session type before checking any of the other session
 * parameters. If the type is not VALID then the other parameters are
 * likely not defined or may have erroneous information.
 *
 * \todo
 * The use_once flag of this function has the side effect of completely
 * deleting a session. There is one problem with that and hackers wannabe...
 * If a hacker checks many sessions with say /verify/\<session-id>, the
 * result is that those sessions get deleted (so the verification fails
 * as expected, but the session may have been necessary for some other
 * reason...) Something to think about! One way to avoid some problems
 * would be to request that the session be at least of the right type
 * (i.e. right path for example). Then we can avoid the delete if the
 * type does not match and the other user can have his session.
 *
 * \param[in] session_key  The session key to load, on input the key does not include the URI.
 * \param[out] info  The variable where the session variables get saved.
 * \param[in] use_once  Whether this session can be used more than once.
 */
void sessions::load_session(QString const & session_key, session_info & info, bool use_once)
{
    // reset this info (although it is likely already brand new...)
    info = session_info();

    QString const key(f_snap->get_website_key() + "/" + session_key);

    QtCassandra::QCassandraTable::pointer_t table(get_sessions_table());
    if(!table->exists(key))
    {
        // if the key does not exist it was either tempered with
        // or the database already deleted it (i.e. it timed out)
        info.set_session_type(session_info::session_info_type_t::SESSION_INFO_MISSING);
        return;
    }

    QtCassandra::QCassandraRow::pointer_t row(table->row(key));
    if(!row)
    {
        // XXX
        // if we get a problem here it is probably something else
        // than a missing row...
        info.set_session_type(session_info::session_info_type_t::SESSION_INFO_MISSING);
        return;
    }

    // save the key as it is not unlikely that the rest will work
    info.set_session_key(session_key);

    QtCassandra::QCassandraValue value;

    value = row->cell(get_name(name_t::SNAP_NAME_SESSIONS_ID))->value();
    if(value.size() != sizeof(int32_t))
    {
        // row timed out between calls
        info.set_session_type(session_info::session_info_type_t::SESSION_INFO_MISSING);
        return;
    }
    info.set_session_id(value.int32Value());

    value = row->cell(get_name(name_t::SNAP_NAME_SESSIONS_PLUGIN_OWNER))->value();
    if(value.nullValue())
    {
        // row timed out between calls
        info.set_session_type(session_info::session_info_type_t::SESSION_INFO_MISSING);
        return;
    }
    info.set_plugin_owner(value.stringValue());

    value = row->cell(get_name(name_t::SNAP_NAME_SESSIONS_PAGE_PATH))->value();
    info.set_page_path(value.stringValue());

    value = row->cell(get_name(name_t::SNAP_NAME_SESSIONS_OBJECT_PATH))->value();
    info.set_object_path(value.stringValue());

    value = row->cell(get_name(name_t::SNAP_NAME_SESSIONS_USER_AGENT))->value();
    info.set_user_agent(value.stringValue());

    value = row->cell(get_name(name_t::SNAP_NAME_SESSIONS_REMOTE_ADDR))->value();
    info.set_remote_addr(value.stringValue());

    value = row->cell(get_name(name_t::SNAP_NAME_SESSIONS_CHECK_FLAGS))->value();
    if(value.size() != sizeof(int64_t))
    {
        // row timed out between calls
        info.set_session_type(session_info::session_info_type_t::SESSION_INFO_MISSING);
        return;
    }
    info.set_check_flags(value.int64Value());

    value = row->cell(get_name(name_t::SNAP_NAME_SESSIONS_TIME_TO_LIVE))->value();
    if(value.size() != sizeof(int32_t))
    {
        // row timed out between calls
        info.set_session_type(session_info::session_info_type_t::SESSION_INFO_MISSING);
        return;
    }
    info.set_time_to_live(value.int32Value());

    value = row->cell(get_name(name_t::SNAP_NAME_SESSIONS_TIME_LIMIT))->value();
    if(value.size() != sizeof(int64_t))
    {
        // row timed out between calls
        info.set_session_type(session_info::session_info_type_t::SESSION_INFO_MISSING);
        return;
    }
    info.set_time_limit(value.int64Value());

    value = row->cell(get_name(name_t::SNAP_NAME_SESSIONS_LOGIN_LIMIT))->value();
    if(value.size() != sizeof(int64_t))
    {
        // row timed out between calls
        info.set_session_type(session_info::session_info_type_t::SESSION_INFO_MISSING);
        return;
    }
    info.set_administrative_login_limit(value.int64Value());

    value = row->cell(get_name(name_t::SNAP_NAME_SESSIONS_DATE))->value();
    if(value.size() != sizeof(int64_t))
    {
        // row timed out between calls
        info.set_session_type(session_info::session_info_type_t::SESSION_INFO_MISSING);
        return;
    }
    info.set_date(value.int64Value());

    value = row->cell(get_name(name_t::SNAP_NAME_SESSIONS_CREATION_DATE))->value();
    if(value.size() != sizeof(int64_t))
    {
        // row timed out between calls
        info.set_session_type(session_info::session_info_type_t::SESSION_INFO_MISSING);
        return;
    }
    info.set_creation_date(value.int64Value());

    value = row->cell(get_name(name_t::SNAP_NAME_SESSIONS_RANDOM))->value();
    if(value.size() != sizeof(int32_t))
    {
        // row timed out between calls
        info.set_session_type(session_info::session_info_type_t::SESSION_INFO_MISSING);
        return;
    }
    info.set_session_random(value.int32Value());

    // check whether the session was already used up
    //
    // WARNING: do not use the 'value' variable here because otherwise
    //          we will destroy the Cassandra TTL used when saving the
    //          used up below
    //
    QtCassandra::QCassandraValue used_up_value(row->cell(get_name(name_t::SNAP_NAME_SESSIONS_USED_UP))->value());
    if(!used_up_value.nullValue())
    {
        info.set_session_type(session_info::session_info_type_t::SESSION_INFO_USED_UP);
        return;
    }

    // is that a session that is to be used just once?
    if(use_once)
    {
        // IMPORTANT NOTE:
        // As a side effect, since we just read values with a TTL
        // this 'value' variable already has the expected TTL!
        value.setCharValue(1);
        row->cell(get_name(name_t::SNAP_NAME_SESSIONS_USED_UP))->setValue(value);
    }

    // a session has three time limits:
    //
    //   1. the total time to live (TTL) -- the session will not get deleted
    //      for that long
    //
    //   2. a time limit which represents the time when the session is
    //      considered to have timed out; if that time limit is 0, we use
    //      the TTL to time out the session
    //
    //   3. a login / administrative time limit; this is not tested here,
    //      instead the users plugin makes use of that one; other plugins
    //      can also test this value
    //
    // A valid session cannot have a time to live and time limit that are
    // both zero (it is checked in the create_session() function.)
    //
    int64_t const now(f_snap->get_start_time());
    int64_t time_limit(info.get_time_limit());
    if(time_limit == 0)
    {
        int64_t const time_to_live(info.get_time_to_live());
        int64_t const creation_date(info.get_creation_date() / 1000000LL);
        time_limit = creation_date + time_to_live;
    }
    if(time_limit < now)
    {
        info.set_session_type(session_info::session_info_type_t::SESSION_INFO_OUT_OF_DATE);
        return;
    }

    // only case when it is 100% valid
    info.set_session_type(session_info::session_info_type_t::SESSION_INFO_VALID);
}


/** \brief Check whether a session exists.
 *
 * There are a few situations where it can be practical to know ahead
 * of time whether a certain session exists. This function offers the
 * called to specify the website key as well as the session identifier.
 *
 * The current website key is used if \p website_key is set to the empty
 * string.
 *
 * \note
 * We do not offer a load_session() from any website for security reasons.
 * However, knowing whether such a session exists is not much of security
 * risk. This generally happens in backend implementations where a table
 * may include data which is handled in such a way that it cannot
 * immediately and properly be segregated by website and once that process
 * happens, the website may or may not exist.
 *
 * \param[in] website_key  The key to the website whose session is being checekd.
 * \param[in] session_key  The identifier of the session to check the existance of.
 *
 * \return true if the session does exist.
 */
bool sessions::session_exists(QString const & website_key, QString const & session_key)
{
    QString const key((website_key.isEmpty() ? f_snap->get_website_key() : website_key)
                     + "/"
                     + session_key);

    QtCassandra::QCassandraTable::pointer_t table(get_sessions_table());
    return table->exists(key);
}


/** \brief Attach data to a session.
 *
 * This function allows you to attach data to an existing session.
 *
 * In most cases this is used with the user session (see the users plugin
 * functions of the same name.)
 *
 * \bug
 * Note that the TTL of the cell is set to the session TTL + 1 day
 * (as we do in the save_session() function.) Unfortunately, that TTL
 * does NOT get refreshed whenever someone calls save_session() since
 * we have no clue this cell even exists. Since it is expected to be
 * used just once and quickly, it is probably not a problem at this
 * point. Note also that when used from the on_attach_to_session()
 * and on_detach_from_session() events, all of these get refreshed each
 * time the user access the website so the data gets updated to TTL + 1
 * as expected.
 *
 * \param[in] info  The session key as defined when you called
 *                  create_session().
 * \param[in] name  The name of the cell where the data is saved.
 * \param[in] data  The data to save in this session.
 *
 * \sa detach_from_session()
 * \sa get_from_session()
 */
void sessions::attach_to_session(session_info const & info, QString const & name, QString const & data)
{
    QString const key(f_snap->get_website_key() + "/" + info.get_session_key());

    SNAP_LOG_DEBUG("sessions::attach_to_session(), key = ")(key)(", name = ")(name)(", data = ")(data);

    QtCassandra::QCassandraTable::pointer_t table(get_sessions_table());
    if(!table->exists(key))
    {
        return;
    }

    QtCassandra::QCassandraRow::pointer_t row(table->row(key));
    if(!row)
    {
        return;
    }

    QtCassandra::QCassandraValue value;
    value.setTtl(info.get_ttl(f_snap->get_start_time()));

    value.setStringValue(data);
    row->cell(name)->setValue(value);
}


/** \brief Detach data from a session.
 *
 * This function grabs data previous attached to a session and drop it
 * from the database.
 *
 * \warning
 * The "detach" means that the data is taken out of the session for good
 * and it is not available in the database after this call. To keep session
 * data in the session, use the get_from_session() function instead.
 *
 * \param[in] info  The key of the session to read from.
 * \param[in] name  The name of the cell where the data was saved.
 *
 * \return The data in a string.
 */
QString sessions::detach_from_session(session_info const & info, QString const & name)
{
    QString const key(f_snap->get_website_key() + "/" + info.get_session_key());

    QtCassandra::QCassandraTable::pointer_t table(get_sessions_table());
    if(!table->exists(key))
    {
        return "";
    }

    QtCassandra::QCassandraRow::pointer_t row(table->row(key));
    if(!row)
    {
        return "";
    }

    // if not defined, we will get an empty string which is what we expect
    QtCassandra::QCassandraValue value(row->cell(name)->value());

    // used once, so delete
    row->dropCell(name);

    return value.stringValue();
}


/** \brief Get a session variable and leave it in the session.
 *
 * Variables that have to leave across many accesses should be read using
 * the get_from_session() function which reads the variable but does not
 * delete it.
 *
 * \param[in] info  The key of the session to read from.
 * \param[in] name  The name of the cell where the data was saved.
 *
 * \return The data in a string.
 */
QString sessions::get_from_session(session_info const & info, QString const & name)
{
    QString key(f_snap->get_website_key() + "/" + info.get_session_key());

    QtCassandra::QCassandraTable::pointer_t table(get_sessions_table());
    if(!table->exists(key))
    {
        return "";
    }

    QtCassandra::QCassandraRow::pointer_t row(table->row(key));
    if(!row)
    {
        return "";
    }

    // if not defined, we will get an empty string which is what is expected
    QtCassandra::QCassandraValue value(row->cell(name)->value());

    return value.stringValue();
}


/** \brief Check whether the cell can securily be used in a script.
 *
 * This signal is sent by the cell() function of snap_expr objects.
 * The plugin receiving the signal can check the table, row, and cell
 * names and mark that specific cell as secure. This will prevent the
 * script writer from accessing that specific cell.
 *
 * In case of the content plugin, this is used to protect all contents
 * in the secret table.
 *
 * The \p secure flag is used to mark the cell as secure. Simply call
 * the mark_as_secure() function to do so.
 *
 * \param[in] table  The table being accessed.
 * \param[in] accessible  Whether the cell is secure.
 *
 * \return This function returns true in case the signal needs to proceed.
 */
void sessions::on_table_is_accessible(QString const & table_name, server::accessible_flag_t & accessible)
{
    if(table_name == get_name(name_t::SNAP_NAME_SESSIONS_TABLE))
    {
        // the sessions table includes all sorts of top-secret
        // identifiers so we do not want anyone to share such
        //
        accessible.mark_as_secure();
    }
}


SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
