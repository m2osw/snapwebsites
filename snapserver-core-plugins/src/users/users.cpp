// Snap Websites Server -- users handling
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
 * \brief Users handling.
 *
 * This plugin handles the low level user functions such as the
 * authentication and user sessions.
 *
 * \li Authenticate user given a certain set of parameters (log in name
 *     and password or a cookie.)
 * \li Log user out of his account.
 * \li Create new user accounts.
 * \li Blocking user accounts.
 * \li A few other things....
 *
 * The Snap! Websites Core offers a separate User UI plugin to access
 * those functions (see plugins/users_ui/...).
 *
 * User sessions currently support several deadlines as defined here:
 *
 * \li Login Limit
 *
 * This is a Unix time_t value defining a hard (non moving) limit
 * of when the user becomes a non-administrator. By default this
 * limit is set to 3 hours, which should be plenty for an
 * administrator to do whatever he needs to do.
 *
 * This limit can be a security issue if too large.
 *
 * \li Time Limit
 *
 * This is a Unix time_t value defining a soft (moving) limit of
 * when the user completely loses all of his log rights. This limit
 * is viewed as a soft limit because each time you hit the website
 * it is reset to the current time plus duration of such a session.
 *
 * The default duration of this session limit is 5 days.
 *
 * \li Time to Live
 *
 * This is a duration in second of how long the session is kept alive.
 * Whether the user is logged in or not, we like to keep a session in
 * order to track various things that the user may do. For example,
 * if the user added items to our e-Commerce cart, then we want to
 * be able to present that cart back to him at a later time.
 *
 * The default duration of the session as a whole is one whole year.
 * Note that the e-Commerce cart may have its own timeout which could
 * be shorter than the user session.
 *
 * The time to live limit is also a soft (moving) limit. Each time
 * the user accesses the site, the session time to live remains the
 * same so the dead line for the death of the session is automatically
 * pushed back, whether the user is logged in or not.
 */

#include "users.h"

#include "../output/output.h"
#include "../list/list.h"
#include "../locale/snap_locale.h"
#include "../messages/messages.h"
#include "../server_access/server_access.h"

#include <snapwebsites/log.h>
#include <snapwebsites/not_reached.h>
#include <snapwebsites/not_used.h>
#include <snapwebsites/qdomhelpers.h>
#include <snapwebsites/qstring_stream.h>
#include <snapwebsites/snap_lock.h>

#include <iostream>

#include <QFile>

#include <openssl/evp.h>
#include <openssl/rand.h>

#include <snapwebsites/poison.h>


SNAP_PLUGIN_START(users, 1, 0)


namespace
{

const int SALT_SIZE = 32;
// the salt size must be even
BOOST_STATIC_ASSERT((SALT_SIZE & 1) == 0);

const int COOKIE_NAME_SIZE = 12; // the real size is (COOKIE_NAME_SIZE / 3) * 4
// we want 3 bytes to generate 4 characters
BOOST_STATIC_ASSERT((COOKIE_NAME_SIZE % 3) == 0);


} // no name namespace


/** \brief Get a fixed users plugin name.
 *
 * The users plugin makes use of different names in the database. This
 * function ensures that you get the right spelling for a given name.
 *
 * \param[in] name  The name to retrieve.
 *
 * \return A pointer to the name.
 */
const char * get_name(name_t name)
{
    switch(name)
    {
    case name_t::SNAP_NAME_USERS_ADMINISTRATIVE_SESSION_DURATION:
        return "users::administrative_session_duration";

    case name_t::SNAP_NAME_USERS_ANONYMOUS_PATH:
        return "user";

    case name_t::SNAP_NAME_USERS_AUTHOR:
        return "users::author";

    case name_t::SNAP_NAME_USERS_AUTHORED_PAGES:
        return "users::authored_pages";

    case name_t::SNAP_NAME_USERS_AUTO_PATH:
        return "types/users/auto";

    case name_t::SNAP_NAME_USERS_EMAIL_HISTORY_LIST_BASE:
        return "users::email_history";

    case name_t::SNAP_NAME_USERS_BLACK_LIST:
        return "*black_list*";

    case name_t::SNAP_NAME_USERS_BLOCKED_PATH:
        return "types/users/blocked";

    case name_t::SNAP_NAME_USERS_CHANGING_PASSWORD_KEY:
        return "users::changing_password_key";

    case name_t::SNAP_NAME_USERS_CREATED_TIME:
        return "users::created_time";

    case name_t::SNAP_NAME_USERS_CURRENT_EMAIL:
        return "users::current_email";

    case name_t::SNAP_NAME_USERS_EXAMPLE:
        return "users::example";

    case name_t::SNAP_NAME_USERS_FORCE_LOWERCASE:
        return "users::force_lowercase";

    case name_t::SNAP_NAME_USERS_FORGOT_PASSWORD_EMAIL:
        return "users::forgot_password_email";

    case name_t::SNAP_NAME_USERS_FORGOT_PASSWORD_IP:
        return "users::forgot_password_ip";

    case name_t::SNAP_NAME_USERS_FORGOT_PASSWORD_ON:
        return "users::forgot_password_on";

    case name_t::SNAP_NAME_USERS_HIT_CHECK:
        return "check";

    case name_t::SNAP_NAME_USERS_HIT_TRANSPARENT:
        return "transparent";

    case name_t::SNAP_NAME_USERS_HIT_USER:
        return "user";

    case name_t::SNAP_NAME_USERS_IDENTIFIER:
        return "users::identifier";

    case name_t::SNAP_NAME_USERS_ID_ROW:
        return "*id_row*";

    case name_t::SNAP_NAME_USERS_INDEX_ROW:
        return "*index_row*";

    case name_t::SNAP_NAME_USERS_LAST_USER_PATH:
        return "users::last_user_path";

    case name_t::SNAP_NAME_USERS_LAST_VERIFICATION_SESSION:
        return "users::last_verification_session";

    case name_t::SNAP_NAME_USERS_LOCALE: // format locale for dates/numbers
        return "users::locale";

    case name_t::SNAP_NAME_USERS_LOCALES: // browser/page languages
        return "users::locales";

    case name_t::SNAP_NAME_USERS_LOGIN_IP:
        return "users::login_ip";

    case name_t::SNAP_NAME_USERS_LOGIN_ON:
        return "users::login_on";

    case name_t::SNAP_NAME_USERS_LOGIN_REDIRECT:
        return "users::loging_redirect";

    case name_t::SNAP_NAME_USERS_LOGIN_REFERRER:
        return "users::login_referrer";

    case name_t::SNAP_NAME_USERS_LOGIN_SESSION:
        return "users::login_session";

    case name_t::SNAP_NAME_USERS_LOGOUT_IP:
        return "users::logout_ip";

    case name_t::SNAP_NAME_USERS_LOGOUT_ON:
        return "users::logout_on";

    case name_t::SNAP_NAME_USERS_LONG_SESSIONS:
        return "users::long_sessions";

    case name_t::SNAP_NAME_USERS_MODIFIED:
        return "users::modified";

    case name_t::SNAP_NAME_USERS_MULTISESSIONS:
        return "users::multisessions";

    case name_t::SNAP_NAME_USERS_MULTIUSER:
        return "users::multiuser";

    case name_t::SNAP_NAME_USERS_NAME:
        return "users::name";

    case name_t::SNAP_NAME_USERS_NEW_PATH:
        return "types/users/new";

    case name_t::SNAP_NAME_USERS_NOT_MAIN_PAGE:
        return "users::not_main_page";

    case name_t::SNAP_NAME_USERS_ORIGINAL_EMAIL:
        return "users::original_email";

    case name_t::SNAP_NAME_USERS_ORIGINAL_IP:
        return "users::original_ip";

    case name_t::SNAP_NAME_USERS_PASSWORD:
        return "users::password";

    case name_t::SNAP_NAME_USERS_PASSWORD_BLOCKED:
        return "users::password::blocked";

    case name_t::SNAP_NAME_USERS_PASSWORD_DIGEST:
        return "users::password::digest";

    case name_t::SNAP_NAME_USERS_PASSWORD_MODIFIED:
        return "users::password::modified";

    case name_t::SNAP_NAME_USERS_PASSWORD_PATH:
        return "types/users/password";

    case name_t::SNAP_NAME_USERS_PASSWORD_SALT:
        return "users::password::salt";

    case name_t::SNAP_NAME_USERS_PATH:
        return "user";

    case name_t::SNAP_NAME_USERS_PICTURE:
        return "users::picture";

    case name_t::SNAP_NAME_USERS_PREVIOUS_LOGIN_IP:
        return "users::previous_login_ip";

    case name_t::SNAP_NAME_USERS_PREVIOUS_LOGIN_ON:
        return "users::previous_login_on";

    case name_t::SNAP_NAME_USERS_SOFT_ADMINISTRATIVE_SESSION:
        return "users::soft_administrative_session";

    // WARNING: We do not use a statically defined name!
    //          To be more secure each Snap! website can use a different
    //          cookie name; possibly one that changes over time and
    //          later by user...
    //case name_t::SNAP_NAME_USERS_SESSION_COOKIE:
    //    // cookie names cannot include ':' so I use "__" to represent
    //    // the namespace separation
    //    return "users__snap_session";

    case name_t::SNAP_NAME_USERS_STATUS:
        return "users::status";

    case name_t::SNAP_NAME_USERS_TABLE:
        return "users";

    case name_t::SNAP_NAME_USERS_TIMEZONE: // user timezone for dates/calendars
        return "users::timezone";

    case name_t::SNAP_NAME_USERS_TOTAL_SESSION_DURATION:
        return "users::total_session_duration";

    case name_t::SNAP_NAME_USERS_USERNAME:
        return "users::username";

    case name_t::SNAP_NAME_USERS_USER_PAGE_PATH:
        return "types/taxonomy/system/content-types/user-page";

    case name_t::SNAP_NAME_USERS_USER_SESSION_DURATION:
        return "users::user_session_duration";

    case name_t::SNAP_NAME_USERS_VERIFIED_IP:
        return "users::verified_ip";

    case name_t::SNAP_NAME_USERS_VERIFIED_ON:
        return "users::verified_on";

    case name_t::SNAP_NAME_USERS_VERIFY_EMAIL:
        return "users::verify_email";

    case name_t::SNAP_NAME_USERS_VERIFY_IGNORE_USER_AGENT:
        return "users::verify_ignore_user_agent";

    case name_t::SNAP_NAME_USERS_VERIFY_IGNORE_USER_AGENT_FOR_PASSWORD:
        return "users::verify_ignore_user_agent_for_password";

    case name_t::SNAP_NAME_USERS_WEBSITE_REFERENCE:
        return "users::website_reference";

    default:
        // invalid index
        throw snap_logic_exception(QString("invalid name_t::SNAP_NAME_USERS_... (%1)").arg(static_cast<int>(name)));

    }
    NOTREACHED();
}


/** \class users
 * \brief The users plugin to handle user accounts.
 *
 * The class handles the low level authentication procedure with
 * credentials (login and password) or a cookie.
 *
 * It also offers ways to create new users and block existing users.
 *
 * To enhance the security of the user session we randomly assign the name
 * of the user session cookie. This way robots have a harder time to
 * break-in since each Snap! website will have a different cookie name
 * to track users (and one website may change the name at any time.)

 * \todo
 * To make it even harder we should look into a way to use a cookie
 * that has a different name per user and changes name each time the
 * user logs in. This should be possible since the list of cookies is
 * easy to parse on the server side, then we can test each cookie for
 * valid snap data which have the corresponding snap cookie name too.
 * (i.e. the session would save the cookie name too!)
 *
 * \todo
 * Add a Secure Cookie which is only secure... and if not present
 * renders the logged in user quite less logged in (i.e. "returning
 * registered user".)
 */


/** \brief Initialize the users plugin.
 *
 * This function initializes the users plugin.
 */
users::users()
    //: f_snap(nullptr) -- auto-init
    //, f_user_info() -- auto-init
    //, f_user_logged_in(false) -- auto-init
    //, f_user_changing_password_key() -- auto-init
    //, f_info(nullptr) -- auto-init
{
}


/** \brief Destroy the users plugin.
 *
 * This function cleans up the users plugin.
 */
users::~users()
{
}


/** \brief Get a pointer to the users plugin.
 *
 * This function returns an instance pointer to the users plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the users plugin.
 */
users * users::instance()
{
    return g_plugin_users_factory.instance();
}


/** \brief Send users to the plugin settings.
 *
 * This path represents this plugin settings.
 */
QString users::settings_path() const
{
    return "/admin/settings/users";
}


/** \brief A path or URI to a logo for this plugin.
 *
 * This function returns a 64x64 icons representing this plugin.
 *
 * \return A path to the logo.
 */
QString users::icon() const
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
QString users::description() const
{
    return "The users plugin manages all the users on a website. It is also"
           " capable to create new users which is a Snap! wide feature.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString users::dependencies() const
{
    return "|filter|locale|output|path|server_access|sessions|";
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
int64_t users::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2017, 1, 4, 0, 42, 55, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Update the database as needed
 *
 * \param[in] last_updated  The UTC Unix date when the website was last
 *                          updated (in micro seconds).
 *
 * \return The UTC Unix date of the last update of this plugin.
 */
int64_t users::do_dynamic_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2016, 12, 14, 18, 06, 32, user_identifier_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Update the users plugin content.
 *
 * This function updates the contents in the database using the
 * system update settings found in the resources.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added
 *                                 to the database by this update
 *                                 (in micro-seconds).
 */
void users::content_update(int64_t variables_timestamp)
{
    NOTUSED(variables_timestamp);
    content::content::instance()->add_xml(get_plugin_name());
}


/** \brief Update the users table to use identifiers
 *
 * This function converts the users table to use identifiers as keys
 * as opposed to using an email address (user_key).
 *
 * See SNAP-258.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added
 *                                 to the database by this update
 *                                 (in micro-seconds).
 */
void users::user_identifier_update(int64_t variables_timestamp)
{
    SNAP_LOG_TRACE( "users::user_identifier_update()" );
    NOTUSED(variables_timestamp);

    auto users_table     ( get_users_table() );
    auto index_row_name  ( get_name(name_t::SNAP_NAME_USERS_INDEX_ROW)  );
    auto id_row_name     ( get_name(name_t::SNAP_NAME_USERS_ID_ROW)     );
    auto identifier_name ( get_name(name_t::SNAP_NAME_USERS_IDENTIFIER) );

    // Drop the index table, and rebuild it below.
    //
    users_table->clearCache();

    libdbproxy::row_predicate::pointer_t row_predicate( std::make_shared<libdbproxy::row_predicate>() );
    row_predicate->setCount(100);

    // Now go through and find all user rows and change them to ids. Add index entries.
    //
    for( ;; )
    {
        uint32_t const count(users_table->readRows(row_predicate));
        if( count == 0 )
        {
            // last page was processed, done.
            break;
        }

        auto row_list(users_table->getRows());
        for( QByteArray const & row_key : row_list.keys() )
        {
            libdbproxy::value const email( row_key );
            QString email_name(email.stringValue());
            SNAP_LOG_TRACE("checking email_name=")(email_name);
            if( email_name == id_row_name || email_name == index_row_name )
            {
                SNAP_LOG_TRACE("ignoring special row");
                // Ignore the id and index rows
                continue;
            }
            if( !email_name.contains(QRegExp(".*@.*")) )
            {
                // Not an email address
                //
                SNAP_LOG_TRACE("not an email address");
                continue;
            }

            libdbproxy::row::pointer_t  const & row     ( row_list[row_key] );
            libdbproxy::cell::pointer_t const & id_cell ( row->getCell(identifier_name) );
            libdbproxy::value const             id      ( id_cell->getValue() );
            libdbproxy::row::pointer_t  const & new_row ( users_table->getRow(id.binaryValue()) );
            SNAP_LOG_TRACE("found email [")(email_name)("], converting to id=[")(id.int64Value());

            // Now create the new row
            //
            libdbproxy::cell_predicate::pointer_t crp( std::make_shared<libdbproxy::cell_predicate>() );
            crp->setCount( 10000 );
            row->readCells( crp );
            auto cell_list(row->getCells());
            for( QByteArray const & cell_key : cell_list.keys() )
            {
                libdbproxy::value value( cell_list[cell_key]->getValue() );
                libdbproxy::cell::pointer_t new_cell( new_row->getCell(cell_key) );
                new_cell->setValue( value );
            }

            // Drop the old row
            //
            users_table->dropRow(email_name);
        }
    }

    // Now create the index row
    //
    users_table->dropRow( index_row_name );
    users_table->clearCache();

    // Now go through and find all user rows and change them to ids. Add index entries.
    //
    SNAP_LOG_TRACE("Creating *index_row*");
    {
        auto current_email_name( get_name(name_t::SNAP_NAME_USERS_CURRENT_EMAIL) );
        libdbproxy::row::pointer_t const & index_row( users_table->getRow(index_row_name) );
        for( ;; )
        {
            uint32_t const count(users_table->readRows(row_predicate));
            if( count == 0 )
            {
                // last page was processed, done.
                break;
            }

            auto row_list(users_table->getRows());
            for( QByteArray const & row_key : row_list.keys() )
            {
                libdbproxy::value const id( row_key );
                QString row_text(id.stringValue());
                if( row_text == id_row_name || row_text == index_row_name )
                {
                    SNAP_LOG_TRACE("not adding to index row_text=")(row_text);
                    // Ignore special rows
                    continue;
                }

                libdbproxy::row::pointer_t  const & row           ( row_list[row_key] );
                libdbproxy::value           const & current_email ( row->getCell(current_email_name)->getValue() );
                libdbproxy::value           const & identifier    ( row_key );

                SNAP_LOG_TRACE("Creating current_email entry: first=")(current_email.stringValue())
                        (", second=")(identifier.int64Value());
                index_row->getCell( current_email.binaryValue() )->setValue( identifier.binaryValue() );
            }
        }
    }
}


/** \brief Bootstrap the users.
 *
 * This function adds the events the users plugin is listening for.
 *
 * \param[in] snap  The child handling this request.
 */
void users::bootstrap(snap_child * snap)
{
    f_snap = snap;

    SNAP_LISTEN0 ( users, "server",  server,           process_cookies                     );
    SNAP_LISTEN0 ( users, "server",  server,           attach_to_session                   );
    SNAP_LISTEN0 ( users, "server",  server,           detach_from_session                 );
    SNAP_LISTEN  ( users, "server",  server,           define_locales,          _1         );
    SNAP_LISTEN  ( users, "server",  server,           improve_signature,       _1, _2, _3 );
    SNAP_LISTEN  ( users, "server",  server,           table_is_accessible,     _1, _2     );
    SNAP_LISTEN0 ( users, "locale",  locale::locale,   set_locale                          );
    SNAP_LISTEN0 ( users, "locale",  locale::locale,   set_timezone                        );
    SNAP_LISTEN  ( users, "content", content::content, create_content,          _1, _2, _3 );
    SNAP_LISTEN  ( users, "layout",  layout::layout,   generate_header_content, _1, _2, _3 );
    SNAP_LISTEN  ( users, "layout",  layout::layout,   generate_page_content,   _1, _2, _3 );
    SNAP_LISTEN  ( users, "filter",  filter::filter,   replace_token,           _1, _2, _3 );
    SNAP_LISTEN  ( users, "filter",  filter::filter,   token_help,              _1         );

    f_info.reset(new sessions::sessions::session_info);
}


/** \brief Initialize the users table.
 *
 * This function creates the users table if it doesn't exist yet. Otherwise
 * it simple returns the existing Cassandra table.
 *
 * If the function is not able to create the table an exception is raised.
 *
 * The table is a list of emails (row keys) and passwords. Additional user
 * data is generally added by other plugins (i.e. address, phone number,
 * what the user bought before, etc.)
 *
 * \return The pointer to the users table.
 */
libdbproxy::table::pointer_t users::get_users_table() const
{
    return f_snap->get_table(get_name(name_t::SNAP_NAME_USERS_TABLE));
}


/** \brief Retrieve the total duration of the session.
 *
 * Whenever a user visits a Snap! website, he is given a cookie with
 * a session identifier. This session has a very long duration. By
 * default it is actually set to 1 year which is the maximum duration
 * for a cookie (although browsers are free to delete cookies sooner
 * than that, obviously.)
 *
 * The default duration of the session is 365 days.
 *
 * \note
 * The function is considered internal although it can be called by
 * other plugins.
 *
 * \warning
 * The value is read once and statically cached by this function.
 *
 * \return The duration of the administrative session in seconds.
 */
int64_t users::get_total_session_duration()
{
    int64_t const default_total_session_duration(365LL * 24LL * 60LL); // 1 year by default, in minutes
    static int64_t cached_duration(-1);

    if(cached_duration == -1)
    {
        libdbproxy::value const total_session_duration(f_snap->get_site_parameter(get_name(name_t::SNAP_NAME_USERS_TOTAL_SESSION_DURATION)));
        // value in database is in days
        cached_duration = total_session_duration.safeInt64Value(0, default_total_session_duration) * 60LL;
    }

    return cached_duration;
}


/** \brief Retrieve the duration of the user session.
 *
 * The user has three types of session durations, as defined in the
 * authorize_user() function. This function returns the duration
 * of the user login session.
 *
 * The default duration of the user session is 5 days.
 *
 * User sessions are considered soft. This means they get extended
 * each time the user accesses the website.
 *
 * \note
 * The function is considered internal although it can be called by
 * other plugins.
 *
 * \warning
 * The value is read once and statically cached by this function.
 *
 * \return The duration of the user session in seconds.
 */
int64_t users::get_user_session_duration()
{
    int64_t const default_user_session_duration(5LL * 24LL * 60LL); // 5 days by default, in minutes
    static int64_t cached_duration(-1);

    if(cached_duration == -1)
    {
        libdbproxy::value const user_session_duration(f_snap->get_site_parameter(get_name(name_t::SNAP_NAME_USERS_USER_SESSION_DURATION)));
        // value in database is in minutes
        cached_duration = user_session_duration.safeInt64Value(0, default_user_session_duration) * 60LL;
    }

    return cached_duration;
}


/** \brief Retrieve the duration of the administrative session.
 *
 * The user has three types of session durations, as defined in the
 * authorize_user() function. This function returns the duration
 * of the administrative login session.
 *
 * The default duration of the administrative session is 3 hours.
 *
 * \note
 * The function is considered internal although it can be called by
 * other plugins.
 *
 * \warning
 * The value is read once and statically cached by this function.
 *
 * \return The duration of the administrative session in seconds.
 */
int64_t users::get_administrative_session_duration()
{
    int64_t const default_administrative_session_duration(3LL * 60LL);
    static int64_t cached_duration(-1);

    if(cached_duration == -1)
    {
        libdbproxy::value const administrative_session_duration(f_snap->get_site_parameter(get_name(name_t::SNAP_NAME_USERS_ADMINISTRATIVE_SESSION_DURATION)));
        // value in database is in minutes
        cached_duration = administrative_session_duration.safeInt64Value(0, default_administrative_session_duration) * 60LL;
    }

    return cached_duration;
}


/** \brief Check whether the administrative session is soft or not.
 *
 * By default, the administrative session is considered a hard session.
 * This means that the duration of that session is hard coded once when
 * the user logs in and stays that way until it times out. After that
 * the user must re-login.
 *
 * There is more information in the authenticated_user() function.
 *
 * The default value for this field is 'false'.
 *
 * \note
 * The function is considered internal although it can be called by
 * other plugins.
 *
 * \warning
 * The value is read once and statically cached by this function.
 *
 * \return Whether the administrative session has been soften.
 */
bool users::get_soft_administrative_session()
{
    signed char const default_soft_administrative_session(0);
    static signed char cached_soft_session(-1);

    if(cached_soft_session == -1)
    {
        libdbproxy::value const soft_administrative_session(f_snap->get_site_parameter(get_name(name_t::SNAP_NAME_USERS_SOFT_ADMINISTRATIVE_SESSION)));
        cached_soft_session = soft_administrative_session.safeSignedCharValue(0, default_soft_administrative_session);
    }

    return cached_soft_session != 0;
}


/** \brief Retrieve the user cookie name.
 *
 * This function retrieves the user cookie name. This can be changed on
 * each restart of the server or after a period of time. The idea is to
 * not allow robots to use one statically defined cookie name on all
 * Snap! websites. It is probably easy for them to find out what the
 * current cookie name is, but it's definitively additional work for
 * the hackers.
 *
 * Also since the cookie is marked as HttpOnly, it is even harder for
 * hackers to do much with those.
 *
 * \return The current user cookie name for this website.
 */
QString users::get_user_cookie_name()
{
    QString user_cookie_name(f_snap->get_site_parameter(snap::get_name(snap::name_t::SNAP_NAME_CORE_USER_COOKIE_NAME)).stringValue());
    if(user_cookie_name.isEmpty())
    {
        // user cookie name not yet assigned or reset so a new name
        // gets assigned
        //
        unsigned char buf[COOKIE_NAME_SIZE];
        int const r(RAND_bytes(buf, sizeof(buf)));
        if(r != 1)
        {
            f_snap->die(snap_child::http_code_t::HTTP_CODE_SERVICE_UNAVAILABLE,
                    "Service Not Available",
                    "The server was not able to generate a safe random number. Please try again in a moment.",
                    "User cookie name could not be generated as the RAND_bytes() function could not generate enough random data");
            NOTREACHED();
        }
        // actually most ASCII characters are allowed, but to be fair, it
        // is not safe to use most so we limit using a simple array
        //
        char allowed_characters[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_.";
        for(int i(0); i < (COOKIE_NAME_SIZE - 2); i += 3)
        {
            // we can generate 4 characters with every 3 bytes we read
            int a(buf[i + 0] & 0x3F);
            int b(buf[i + 1] & 0x3F);
            int c(buf[i + 2] & 0x3F);
            int d((buf[i + 0] >> 6) | ((buf[i + 1] >> 4) & 0x0C) | ((buf[i + 2] >> 2) & 0x30));
            if(i == 0 && a >= 52)
            {
                a &= 0x1F; // force a letter as character 0
            }
            user_cookie_name += allowed_characters[a];
            user_cookie_name += allowed_characters[b];
            user_cookie_name += allowed_characters[c];
            user_cookie_name += allowed_characters[d];
        }

        // TODO: allow other plugins to prevent certain names?
        //       (i.e. the cookie_consent_sliktide JavaScript code creates
        //       a cookie to know that the user consents to use the website
        //       and we would not want to clash with that cookie.)
        //
        //bool interfering(false);
        //interfering_cookie(user_cookie_name, interfering);
        //if(interfering) ... generate a new cookie name ...

        f_snap->set_site_parameter(snap::get_name(snap::name_t::SNAP_NAME_CORE_USER_COOKIE_NAME), user_cookie_name);
    }
    return user_cookie_name;
}


/** \brief Process the cookies.
 *
 * This function is our opportunity to log the user in. We check for the
 * user cookie and use it to know whether the user is currently logged in
 * or not.
 *
 * Note that this session is always created and is used by all the other
 * plugins as the current user session.
 *
 * Only this very function also checks whether the user is currently
 * logged in and defines the user identifier if so. Otherwise the
 * session can be used for things such as saving messages between redirects.
 *
 * \important
 * This function cannot be called more than once. It would not properly
 * reset variables if called again.
 */
void users::on_process_cookies()
{
    // prevent cookies on a set of method that do not require them
    QString const method(f_snap->snapenv(get_name(snap::name_t::SNAP_NAME_CORE_REQUEST_METHOD)));
    if(method == "HEAD"
    || method == "TRACE")
    {
        // do not log the user on HEAD and TRACE methods
        return;
    }

    bool create_new_session(true);

    // get cookie name
    QString const user_cookie_name(get_user_cookie_name());

    // any snap session?
    if(f_snap->cookie_is_defined(user_cookie_name))
    {
        // is that session a valid user session?
        QString const session_cookie(f_snap->cookie(user_cookie_name));
        if(load_login_session(session_cookie, *f_info, false) == LOGIN_STATUS_OK)
        {
            // this session qualifies as a log in session
            // so now verify the user
            QString const path(f_info->get_object_path());
            bool ok(false);
            identifier_t const id( path.mid(6).toLongLong(&ok) );
            if( !ok || !authenticated_user(id, nullptr) )
            {
                // we are logged out because the session timed out
                //
                // TODO: this is actually wrong, we do not want to lose the
                //       user path, but it will do for now...
                //
                f_info->set_object_path(user_info_t::get_full_anonymous_path()); // no user id for the anonymous user
            }
            create_new_session = false;
        }
    }

    // complete reset?
    if(create_new_session)
    {
        // we may have some spurious data in the f_info structure
        // so we do a complete reset first
        //
        sessions::sessions::session_info new_session;
        *f_info = new_session;
    }

    // There is a login limit so we do not need to "randomly" limit
    // a visitor user session to a ridiculously small amount unless
    // we think that could increase the database size too much...
    // two reasons to have a very long time to live are:
    //   1) user created a cart and we want the items he put in his
    //      cart to stay there "forever" (at least a year)
    //   2) user was sent to the site through an affiliate link, we
    //      want to reward the affiliate whether the user was sent
    //      there 1 day or 1 year ago
    //
    // To satisfy any user, we need this to be an administrator setup
    // value. By default we use one whole year. (note that this time
    // to live default is also what's defined in the sessions plugin.)
    //
    int64_t const total_session_duration(get_total_session_duration());
    f_info->set_time_to_live(total_session_duration);

    // check the type of hit unless we are anyway
    // creating a new session
    //
    {
        f_hit = get_name(name_t::SNAP_NAME_USERS_HIT_USER);
        QString const qs_hit(f_snap->get_server_parameter("qs_hit"));
        snap_uri const & uri(f_snap->get_uri());
        if(uri.has_query_option(qs_hit))
        {
            // the user specified an action
            f_hit = uri.query_option(qs_hit);
            if(f_hit != get_name(name_t::SNAP_NAME_USERS_HIT_USER)
            && f_hit != get_name(name_t::SNAP_NAME_USERS_HIT_CHECK)
            && f_hit != get_name(name_t::SNAP_NAME_USERS_HIT_TRANSPARENT))
            {
                SNAP_LOG_WARNING("received an unknown type of hit \"")(f_hit)("\", forcing to \"user\"");
                f_hit = get_name(name_t::SNAP_NAME_USERS_HIT_USER);
            }
        }
    }

    // if we are just checking the session, do that and exit right away
    //
    // TODO: we should move the f_hit initialization and 'check'
    //       to another function
    //
    if(f_hit == get_name(name_t::SNAP_NAME_USERS_HIT_CHECK))
    {
        snap_string_list result;

        if(f_info->get_object_path() != "/user/") // if anonymous, ignore the time limits, they do not matter
        {
            // is the standard user session over?
            //
            time_t const start_time(f_snap->get_start_time());
            if(start_time < f_info->get_time_limit())
            {
                result << "standard";

                // the standard session must be active to have a chance to
                // also have an administrative session still active
                //
                if(start_time < f_info->get_administrative_login_limit())
                {
                    result << "admin";
                }
            }
            else
            {
                result << "soft";
            }
        }

        if(result.isEmpty())
        {
            // no session is active
            //
            result << "none";
        }

        server_access::server_access * server_access_plugin(server_access::server_access::instance());
        if(server_access_plugin->is_ajax_request())
        {
            // BUG: here main_ipath will NOT include all the correct info
            //      since 'path::execute()' did not run yet...
            //
            content::path_info_t main_ipath;
            main_ipath.set_path(f_snap->get_uri().path());
            server_access_plugin->create_ajax_result(main_ipath, true);
            server_access_plugin->ajax_append_data("users__session_status", result.join(",").toUtf8());

            time_t const start_time(f_snap->get_start_time());
            if(start_time < f_info->get_time_limit())
            {
                server_access_plugin->ajax_append_data("users__session_time_limit", QString("%1").arg(f_info->get_time_limit()).toUtf8());

                // if we have a user session, we may also have an
                // administrative session...
                //
                if(start_time < f_info->get_administrative_login_limit())
                {
                    server_access_plugin->ajax_append_data("users__administrative_login_time_limit", QString("%1").arg(f_info->get_administrative_login_limit()).toUtf8());
                }
            }
            server_access_plugin->ajax_output();
        }
        else
        {
            // in this case the user is checking without AJAX so we have to
            // reply with plain text, so here it goes
            //
            f_snap->set_header("Content-Type", "text/plain; charset=utf-8");
            QString text(result.join(","));
            time_t const start_time(f_snap->get_start_time());
            if(start_time < f_info->get_time_limit())
            {
                text += "\nusers__session_time_limit=";
                text += QString("%1").arg(f_info->get_time_limit());

                // if we have a user session, we may also have an
                // administrative session...
                //
                if(start_time < f_info->get_administrative_login_limit())
                {
                    text += "\nusers__administrative_login_time_limit=";
                    text += QString("%1").arg(f_info->get_administrative_login_limit());
                }
            }
            f_snap->output(text);
        }

        // TBD: since we return very quickly, we are probably missing a
        //      set of headers?
        //
        return;
    }

    // create or refresh the session
    //
    if(create_new_session)
    {
        // create a new session
        //
        f_info->set_session_type(sessions::sessions::session_info::session_info_type_t::SESSION_INFO_USER);
        f_info->set_session_id(USERS_SESSION_ID_LOG_IN_SESSION);
        f_info->set_plugin_owner(get_plugin_name()); // ourselves
        //f_info->set_page_path(); -- default is fine, we do not use the path
        f_info->set_object_path(user_info_t::get_full_anonymous_path()); // no user id for the anonymous user
        f_info->set_user_agent(f_snap->snapenv(snap::get_name(snap::name_t::SNAP_NAME_CORE_HTTP_USER_AGENT)));
        sessions::sessions::instance()->create_session(*f_info);
    }
    else
    {
        // TODO: change the 5 minutes with a parameter the admin can change
        //       if the last session was created more than 5 minutes ago then
        //       we generate a new random identifier (doing it on each access
        //       generates a lot of problems when the browser tries to load
        //       many things at the same time)
        //
        // TBD: random is not working right if the user attempts to open
        //      multiple pages in a row "very" quickly
        //
        bool const new_random(f_info->get_date() + NEW_RANDOM_INTERVAL < f_snap->get_start_date());
        sessions::sessions::instance()->save_session(*f_info, new_random);
    }

    // push new cookie info back to the browser
    http_cookie cookie(
            f_snap,
            user_cookie_name,
            QString("%1/%2").arg(f_info->get_session_key()).arg(f_info->get_session_random())
        );
    cookie.set_expire_in(f_info->get_time_to_live());
    cookie.set_http_only(); // make it a tad bit safer
    f_snap->set_cookie(cookie);
//std::cerr << "user session id [" << f_info->get_session_key() << "] [" << f_user_key << "]\n";

    if(f_user_info.is_user())
    {
        // make sure user locale/timezone get used on next
        // locale/timezone access
        locale::locale::instance()->reset_locale();

        // send a signal that the user is ready (this signal is also
        // sent when we have a valid cookie)
        logged_in_user_ready();
    }
}


/** \brief Mark this hit as transparent.
 *
 * This function should be called to make sure that a hit becomes
 * transparent which means that the time limit will not be updated
 * and thus not extended.
 *
 * \note
 * This is generally done for all content other than the main page.
 * Although the current default is the other way around so you have
 * to be pro-active and call this function. At some point, we may
 * want to reverse that.
 */
void users::transparent_hit()
{
    f_hit = get_name(name_t::SNAP_NAME_USERS_HIT_TRANSPARENT);
}


/** \brief Check whether this is a transparent hit.
 *
 * This function can be called to know whether the hit was marked as being
 * transparent.
 *
 * \note
 * When you call this function, the hit may not yet have been marked as
 * transparent. Since it is rather rare that this function gets call in
 * those cases, we currently let it be.
 */
bool users::is_transparent_hit()
{
    return f_hit == get_name(name_t::SNAP_NAME_USERS_HIT_TRANSPARENT);
}


/** \brief Load a user login session.
 *
 * This function loads a session used to know whether a user is logged
 * in or not. The users and OAuth2 plugins make use of it.
 *
 * \p session_cookie is expected to include a session key and
 * the corresponding random number. At this point the random number
 * is optional although we do not desperate and will most certainly
 * reintroduce it at some point. That being said, if specified it
 * gets checked. If not specified, it is plainly ignored.
 *
 * \note
 * The authenticated_user() function verifies that the user is still
 * logged in in terms of login time limit. If your function is not
 * going to call the authenticated_user() function, then you will
 * want to set the check_time_limit variable to true and the time
 * limit will be checked here instead.
 *
 * \todo
 * Look further into the time check because the way it is does not
 * make a lot of sense. The load_login_session() may need to be
 * smarter and be capable of deleting the session as the
 * authenticated_user() function does now...
 *
 * \param[in] session_key  The key of the session to be loaded.
 * \param[out] info  The session information are loaded in this variable.
 * \param[in,out] check_time_limit  Set to true if you are NOT going to call
 *                                  authenticated_user() afterward.
 *
 * \return LOGIN_STATUS_OK (0) if the load succeeds and the user is
 *         considered to have logged in successfully in the past. HOWEVER,
 *         that does not mean the user is logged in. You still need to call
 *         authenticated_user() to make sure of that.
 */
users::login_status_t users::load_login_session(QString const & session_cookie, sessions::sessions::session_info & info, bool const check_time_limit)
{
    login_status_t authenticated(LOGIN_STATUS_OK);

    snap_string_list const parameters(session_cookie.split("/"));
    QString const session_key(parameters[0]);
    int random_value(-1);
    if(parameters.size() > 1)
    {
        bool ok(false);
        random_value = parameters[1].toInt(&ok);
        if(!ok || random_value < 0)
        {
            SNAP_LOG_INFO("cookie included an invalid random key, ")(parameters[1])(" is not a valid decimal number or is negative.");
            authenticated |= LOGIN_STATUS_INVALID_RANDOM_NUMBER;
        }
    }

    // load the session in the specified info object
    sessions::sessions::instance()->load_session(session_key, info, false);

    // the session must be be valid (duh!)
    //
    // Note that a user session marked out of date is a valid session, only
    // the time limit was passed, meaning that the user is not logged in
    // anymore. It is very important to keep such sessions if we want to
    // properly track things long term.
    //
    sessions::sessions::session_info::session_info_type_t session_type(info.get_session_type());
    if(session_type != sessions::sessions::session_info::session_info_type_t::SESSION_INFO_VALID
    && session_type != sessions::sessions::session_info::session_info_type_t::SESSION_INFO_OUT_OF_DATE)
    {
        SNAP_LOG_INFO("cookie refused because session is not marked as valid, ")(static_cast<int>(session_type));
        authenticated |= LOGIN_STATUS_INVALID_SESSION;
    }

    // the session must be of the right type otherwise it was not a log in session...
    if(info.get_session_id() != USERS_SESSION_ID_LOG_IN_SESSION
    || info.get_plugin_owner() != get_plugin_name())
    {
        SNAP_LOG_INFO("cookie refused because this is not a user session, ")(info.get_session_id());
        authenticated |= LOGIN_STATUS_SESSION_TYPE_MISMATCH;
    }

    // check whether the random number is valid (not a real factor at this point though)
    if(random_value >= 0
    && info.get_session_random() != random_value)
    {
        SNAP_LOG_INFO("cookie would be refused because random key ")(random_value)(" does not match ")(info.get_session_random());
        //authenticated |= LOGIN_STATUS_RANDOM_MISMATCH;
        //                       -- there should be a flag because
        //                          in many cases it kicks someone
        //                          out even when it should not...
        //
        // From what I can tell, this mainly happens if someone uses two
        // tabs accessing the same site. But I have seen it quite a bit
        // if the system crashes and thus does not send the new random
        // number to the user. We could also look into a way to allow
        // the previous random for a while longer.
    }

    // user agent cannot change, frankly! who copies their cookies between
    // devices or browsers?
    //
    // TODO: we actually need to not check the agent version; although
    //       having to log back in whenever you do an upgrade of your
    //       browser is probably fine
    //
    if(info.get_user_agent() != f_snap->snapenv(snap::get_name(snap::name_t::SNAP_NAME_CORE_HTTP_USER_AGENT)))
    {
        SNAP_LOG_INFO("cookie refused because user agent \"")(f_snap->snapenv(snap::get_name(snap::name_t::SNAP_NAME_CORE_HTTP_USER_AGENT)))
                        ("\" does not match \"")(f_info->get_user_agent())("\"");
        authenticated |= LOGIN_STATUS_USER_AGENT_MISMATCH;
    }

    // path must start with "/user/"
    QString const path(info.get_object_path());
    if(path.left(6) != user_info_t::get_full_anonymous_path())
    {
        SNAP_LOG_INFO("cookie refused because the path does not start with \"/user/\", ")(path);
        authenticated |= LOGIN_STATUS_UNEXPECTED_PATH;
    }

    // early check on the login time limit because the caller may
    // not want to call the authenticated_user() function and yet
    // they may want to know whether the user has a chance to be
    // logged in for real without actually making this user the
    // logged in user
    //
    // time limit is a time_t value
    //
    if(check_time_limit
    && f_snap->get_start_time() >= info.get_time_limit())
    {
        SNAP_LOG_INFO("cookie is acceptable but time limit is passed. Now: ")(f_snap->get_start_time())(" >= Limit: ")(info.get_time_limit());
        authenticated |= LOGIN_STATUS_PASSED_LOGIN_LIMIT;
    }

    return authenticated;
}


/** \brief Allow other plugins to authenticate a user.
 *
 * We use a cookie to authenticate a returning user. The cookie
 * holds a session identifier. This function checks that
 * the session is still valid and mark the user as logged in if so.
 *
 * Note that the function returns with one of the following states:
 *
 * \li User is not logged in, the function returns false and there
 *     is no user key to speak of... the user can still be tracked
 *     with the cookie, but the data cannot be attached to an account
 *
 *       . f_user_info.is_user() -- false
 *       . f_user_logged_in -- false
 *       . f_administrative_logged_in -- false
 *
 * \li User is "logged in", the function returns true; the login
 *     status is one of following statuses:
 *
 * \li User is strongly logged in, meaning that he has administrative
 *     rights at this time; by default this is true for 3h after an
 *     active log in; the administrative rights are dropped after 3h
 *     and you need to re-login to gain the administrative rights
 *     again. This type of session is NOT extended by default. That
 *     means it lasts 3h then times out, whether or not the user is
 *     accessing/using the website administratively or otherwise.
 *     This can be changed to function like the soft login though
 *     each access by the user can extend the current timeout to
 *     "now + 3h". If you choose to do that, you probably want to
 *     reduce the time to something much shorter like 15 or 30 min.
 *
 *       . f_user_info.is_user() -- true
 *       . f_user_logged_in -- true
 *       . f_administrative_logged_in -- true
 *
 * \li User is softly logged in, meaning that he has read/write access
 *     to everything except administrative tasks; when the user tries
 *     to access an administrative task, he is sent to the login screen
 *     in an attempt to see whether we can grant the user such rights...
 *     The soft login time limit gets extended each time the user hits
 *     the website. So the duration can be very long assuming the user
 *     comes to the website at least once a day or so.
 *
 *       . f_user_info.is_user() -- true
 *       . f_user_logged_in -- true
 *       . f_administrative_logged_in -- false
 *
 * \li User is weakly logged in, meaning that he was logged in on the
 *     website in the past, although the logging session still exists,
 *     it does not grant much write access at all (if any, it is
 *     really very safe tasks...); the user is asked to log back in
 *     to edit content. Note that this is called Long Session, it is
 *     turned on by default, but it can be turned off.
 *
 *       . f_user_info.is_user() -- true
 *       . f_user_logged_in -- false
 *       . f_administrative_logged_in -- false
 *
 * \note
 * At this time, if f_user_logged_in is false, then
 * f_administrative_logged_in is false too.
 *
 * If no session is passed in, the users plugin f_info session information
 * is used to check the time limits of the session. If the time
 * limits indicate that the user has waited too long, he does
 * not get strongly or softly logged in as indicated above.
 *
 * If the path of the main URI starts with /logout then the user
 * is forcibly logged out instead of logged in. You do not have
 * direct control over this path unless you change the main URI
 * before the call.
 *
 * \note
 * The specified \p info session data is saved in the users'
 * plugin f_info variable member only if the user gets authenticated
 * and the pointer is not nullptr.
 *
 * \note
 * The \p id must be valid for the function to succeed.
 *
 * \warning
 * The user may be marked as known / valid, and even the function may
 * return true and yet the user is not considered logged in. This is
 * the side effect of the long sessions scheme. This scheme gives
 * us the possibility to offer a certain number of functionalities to
 * the user at a reduced level of permissions (i.e. returning registered
 * user opposed to a fully registered user.) To determine whether
 * the user is indeed logged in, please make sure to check the
 * f_user_logged_in flag. From the outside of the users plugin,
 * this is what the user_is_logged_in() or user_has_administrative_rights()
 * functions return.
 *
 * \param[in] id  The user identifier.
 * \param[in] info  A pointer to the user's session information to be used.
 *
 * \return true if the user gets authenticated, false in all other cases.
 */
bool users::authenticated_user(identifier_t const id, sessions::sessions::session_info * info)
{
    user_info_t user_info(get_user_info_by_id(id));

    if( !user_info.is_user() )
    {
        SNAP_LOG_INFO("cannot authenticate user without a valid key (anonymous users also get this message).");
        return false;
    }

    // verify that the user is really properly registered
    //
    if( !user_info.exists() )
    {
        SNAP_LOG_INFO("user key \"")(user_info.get_identifier())("\" was not found in the users table");
        return false;
    }

    // is the user/application trying to log out
    //
    QString const uri_path(f_snap->get_uri().path());
    if(uri_path == "logout" || uri_path.startsWith("logout/"))
    {
        // the user is requesting to be logged out, here we avoid
        // dealing with all the session information again this
        // way we right away cancel the log in but we actually
        // keep the session
        //
        // this may look weird but we cannot call user_logout()
        // without the f_user_info setup properly...
        //
        f_user_info = user_info;
        if(info != nullptr)
        {
            *f_info = *info;
        }
        user_logout();
        return false;
    }

    // the user still has a valid session, but he may
    // not be fully logged in... (i.e. not have as much
    // permission as given with a fresh log in)
    //
    // TODO: we need an additional form to authorize
    //       the user to do more
    //
    time_t const limit(info != nullptr ? info->get_time_limit() : f_info->get_time_limit());
    f_user_logged_in = f_snap->get_start_time() < limit;
    if(!f_user_logged_in)
    {
        SNAP_LOG_TRACE("user authentication timed out by ")(limit - f_snap->get_start_time())(" seconds");

        // just in case, make sure the administrative logged in variable
        // is also false
        //
        f_administrative_logged_in = false;
    }
    else
    {
        // the user may also be "administratively" logged in
        //
        time_t const admin_limit(info != nullptr ? info->get_administrative_login_limit() : f_info->get_administrative_login_limit());
        f_administrative_logged_in = f_snap->get_start_time() < admin_limit;
        if(!f_administrative_logged_in)
        {
            SNAP_LOG_TRACE("user administrative authentication timed out by ")(admin_limit - f_snap->get_start_time())(" seconds");
        }
    }

    // the website may opt out of the long session scheme
    // the following loses the user key if the website
    // administrator said so...
    //
    // long sessions allows us to track the user even after
    // the time limit was reached (i.e. returning user,
    // opposed to just a returning visitor)
    //
    libdbproxy::value const long_sessions(f_snap->get_site_parameter(get_name(name_t::SNAP_NAME_USERS_LONG_SESSIONS)));
    if(f_user_logged_in
    || (long_sessions.nullValue() || long_sessions.signedCharValue()))
    {
        f_user_info = user_info;
        if(info != nullptr)
        {
            *f_info = *info;
        }
        return true;
    }

    return false;
}


/** \brief This function can be used to log the user out.
 *
 * If your software detects a situation where a currently logged in
 * user should be forcibly logged out, this function can be called.
 * The result is to force the user to log back in.
 *
 * Note that you should let the user know why you are kicking him
 * or her out otherwise they are likely to try to log back in again
 * and again and possibly get locked out (i.e. too many loggin
 * attempts.) In most cases, an error or warning message and a
 * redirect will do. This function does not do either so it is
 * likely that the user will be redirect to the log in page if
 * you do not do a redirect yourself.
 *
 * \note
 * The function does nothing if no user is currently logged
 * in.
 *
 * \warning
 * The function should never be called before the process_cookies()
 * signal gets processed, although this function should work if called
 * from within the user_logged_in() function.
 *
 * \warning
 * If you return from your function (instead of redirecting the user)
 * you may get unwanted results (i.e. the user could still be shown
 * the page accessed.)
 */
void users::user_logout()
{
    if( !f_user_info.is_user() )
    {
        // just in case, make sure the flag is false
        f_user_logged_in = false;
        return;
    }

    // drop the referrer if there is one, it is a security
    // issue to keep that info on an explicit log out!
    //
    NOTUSED(detach_referrer(f_user_info));

    // the software is requesting to log the user out
    //
    // "cancel" the session
    f_info->set_object_path(user_info_t::get_full_anonymous_path());

    // extend the session even on logout
    int64_t const total_session_duration(get_total_session_duration());
    f_info->set_time_to_live(total_session_duration);

    // Save the date when the user logged out
    libdbproxy::value value;
    value.setInt64Value(f_snap->get_start_date());
    f_user_info.set_value(name_t::SNAP_NAME_USERS_LOGOUT_ON, value);

    // Save the user IP address when logged out
    value.setStringValue(f_snap->snapenv(snap::get_name(snap::name_t::SNAP_NAME_CORE_REMOTE_ADDR)));
    f_user_info.set_value(name_t::SNAP_NAME_USERS_LOGOUT_IP, value);

    sessions::sessions::instance()->save_session(*f_info, false);

    // Login session was destroyed so we really do not need it here anymore
    QString const last_login_session(f_user_info.get_value(get_name(name_t::SNAP_NAME_USERS_LOGIN_SESSION)).stringValue());
    if(last_login_session == f_info->get_session_key())
    {
        // when clicking the "Log Out" button, we may already have been
        // logged out and if that is the case the session may not be
        // the same, hence the previous test to make sure we only delete
        // the session identifier that correspond to the last session
        //
        f_user_info.delete_value(name_t::SNAP_NAME_USERS_LOGIN_SESSION);
    }

    f_user_info.reset();
    f_user_logged_in = false;
}


users::user_info_t & users::get_user_info()
{
    return f_user_info;
}


users::user_info_t const & users::get_user_info() const
{
    return f_user_info;
}


/** \brief Do a basic canonicalization on the specified email.
 *
 * Any email must have its domain name canonicalized, meaning that it
 * has to be made lowercase. This function does just that.
 *
 * That means the part before the '@' character is untouched. The
 * part after the '@' is transformed to lowercase.
 *
 * It is very important to at least call this function to get a
 * valid email to check with the libtld functions because those
 * functions really only accept lowercase characters.
 *
 * \note
 * The user plugin still saves the raw emails of users registering
 * on a website. In other words, the email saved as the current user
 * email, the first email used to register, etc. may all include
 * upper and lower case characters.
 *
 * \param[in] email  The email to canonicalize.
 *
 * \return The email with the domain name part canonicalized.
 *
 * \sa email_to_user_key()
 */
QString users::basic_email_canonicalization(QString const & email)
{
    int const pos(email.indexOf('@'));
    if(pos <= 0)
    {
        throw users_exception_invalid_email(QString("email \"%1\" does not include an AT ('@') character or it is the first character.").arg(email));
    }

    return email.mid(0, pos + 1) + email.mid(pos + 1).toLower();
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
bool users::on_path_execute(content::path_info_t & ipath)
{
    f_snap->output(layout::layout::instance()->apply_layout(ipath, this));

    return true;
}


void users::on_generate_main_content(content::path_info_t & ipath, QDomElement & page, QDomElement & body)
{
    // TODO: see SNAP-272 -- remove
    output::output::instance()->on_generate_main_content(ipath, page, body);
}

void users::on_generate_boxes_content(content::path_info_t & page_ipath, content::path_info_t & ipath, QDomElement & page, QDomElement & boxes)
{
    NOTUSED(page_ipath);
    output::output::instance()->on_generate_main_content(ipath, page, boxes);
}


void users::on_generate_header_content(content::path_info_t & ipath, QDomElement & header, QDomElement & metadata)
{
    NOTUSED(ipath);

    QDomDocument doc(header.ownerDocument());

    //libdbproxy::table::pointer_t users_table(get_users_table());

    // retrieve the row for that user
    if(f_user_info.exists())
    {
        {   // snap/head/metadata/desc[@type='users::email']/data
            QDomElement desc(doc.createElement("desc"));
            desc.setAttribute("type", "users::email");
            metadata.appendChild(desc);
            QDomElement data(doc.createElement("data"));
            desc.appendChild(data);
            QDomText text(doc.createTextNode(f_user_info.get_user_key()));
            data.appendChild(text);
        }

        {   // snap/head/metadata/desc[@type='users::name']/data
            libdbproxy::value const value(f_user_info.get_value(name_t::SNAP_NAME_USERS_USERNAME));
            if(!value.nullValue())
            {
                QDomElement desc(doc.createElement("desc"));
                desc.setAttribute("type", get_name(name_t::SNAP_NAME_USERS_NAME));
                metadata.appendChild(desc);
                QDomElement data(doc.createElement("data"));
                desc.appendChild(data);
                QDomText text(doc.createTextNode(value.stringValue()));
                data.appendChild(text);
            }
        }

        {   // snap/head/metadata/desc[@type='users::created']/data
            libdbproxy::value const value(f_user_info.get_value(name_t::SNAP_NAME_USERS_CREATED_TIME));
            if(!value.nullValue())
            {
                QDomElement desc(doc.createElement("desc"));
                desc.setAttribute("type", "users::created"); // NOTE: in the database it is named "users::created_time"
                metadata.appendChild(desc);
                QDomElement data(doc.createElement("data"));
                desc.appendChild(data);
                QDomText text(doc.createTextNode(f_snap->date_to_string(value.int64Value())));
                data.appendChild(text);
            }
        }

        time_t time_to_live(f_info->get_time_to_live());
        {   // snap/head/metadata/desc[@type='users::session_time_to_live']/data
            if(time_to_live < 0)
            {
                time_to_live = 0;
            }
            QDomElement desc(doc.createElement("desc"));
            desc.setAttribute("type", "users::session_time_to_live");
            metadata.appendChild(desc);
            QDomElement data(doc.createElement("data"));
            desc.appendChild(data);
            QDomText text(doc.createTextNode(QString("%1").arg(time_to_live)));
            data.appendChild(text);
        }

        time_t user_time_limit(f_info->get_time_limit());
        {   // snap/head/metadata/desc[@type='users::session_time_limit']/data
            if(user_time_limit < 0)
            {
                user_time_limit = 0;
            }
            QDomElement desc(doc.createElement("desc"));
            desc.setAttribute("type", "users::session_time_limit");
            metadata.appendChild(desc);
            QDomElement data(doc.createElement("data"));
            desc.appendChild(data);
            QDomText text(doc.createTextNode(QString("%1").arg(user_time_limit)));
            data.appendChild(text);
        }

        time_t administrative_login_time_limit(f_info->get_administrative_login_limit());
        {   // snap/head/metadata/desc[@type='users::administrative_login_time_limit']/data
            if(administrative_login_time_limit < 0)
            {
                administrative_login_time_limit = 0;
            }
            QDomElement desc(doc.createElement("desc"));
            desc.setAttribute("type", "users::administrative_login_time_limit");
            metadata.appendChild(desc);
            QDomElement data(doc.createElement("data"));
            desc.appendChild(data);
            QDomText text(doc.createTextNode(QString("%1").arg(administrative_login_time_limit)));
            data.appendChild(text);
        }

        // save those values in an inline JavaScript snippet
        QString const code(QString(
            "/* users plugin */"
            "users__session_time_to_live=%1;"
            "users__session_time_limit=%2;"
            "users__administrative_login_time_limit=%3;")
                    .arg(time_to_live)
                    .arg(user_time_limit)
                    .arg(administrative_login_time_limit));
        content::content * content_plugin(content::content::instance());
        content_plugin->add_inline_javascript(doc, code);
        content_plugin->add_javascript(doc, "users");
    }
}


void users::on_generate_page_content(content::path_info_t & ipath, QDomElement & page, QDomElement & body)
{
    // TODO: convert using field_search
    QDomDocument doc(page.ownerDocument());

    // retrieve the authors
    // TODO: add support to retrieve the "author" who last modified this
    //       page (i.e. user reference in the last revision)
    libdbproxy::table::pointer_t content_table(content::content::instance()->get_content_table());
    QString const link_name(get_name(name_t::SNAP_NAME_USERS_AUTHOR));
    links::link_info author_info(link_name, true, ipath.get_key(), ipath.get_branch());
    QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(author_info));
    links::link_info user_info;
    if(link_ctxt->next_link(user_info))
    {
        // an author is attached to this page!
        //
        // all we want to offer here is the author details defined in the
        // /user/... location although we may want access to his email
        // address too (to display to an admin for example)
        content::path_info_t user_ipath;
        user_ipath.set_path(user_info.key());

        int64_t const user_id(user_info_t::get_user_id_by_path( user_ipath.get_key() ));

        {   // snap/page/body/author[@type="users::identifier"]/data
            QDomElement author(doc.createElement("author"));
            author.setAttribute("type", get_name(name_t::SNAP_NAME_USERS_IDENTIFIER));
            body.appendChild(author);
            QDomElement data(doc.createElement("data"));
            author.appendChild(data);
            QDomText text(doc.createTextNode(QString("%1").arg(user_id)));
            data.appendChild(text);
        }

        {   // snap/page/body/author[@type="users::email"]/data
            QDomElement author(doc.createElement("author"));
            author.setAttribute("type", get_name(name_t::SNAP_NAME_USERS_IDENTIFIER));
            body.appendChild(author);
            QDomElement data(doc.createElement("data"));
            author.appendChild(data);
            QString const user_email(get_user_email(user_id));
            QDomText text(doc.createTextNode(user_email));
            data.appendChild(text);
        }

        {   // snap/page/body/author[@type="users::name"]/data
            libdbproxy::value const value(content_table->getRow(user_ipath.get_key())->getCell(get_name(name_t::SNAP_NAME_USERS_USERNAME))->getValue());
            if(!value.nullValue())
            {
                QDomElement author(doc.createElement("author"));
                author.setAttribute("type", get_name(name_t::SNAP_NAME_USERS_NAME));
                body.appendChild(author);
                QDomElement data(doc.createElement("data"));
                author.appendChild(data);
                QDomText text(doc.createTextNode(value.stringValue()));
                data.appendChild(text);
            }
        }

        // TODO test whether the author has a public profile, if so then
        //      add a link to the account
    }
}



void users::on_create_content(content::path_info_t & ipath, QString const & owner, QString const & type)
{
    NOTUSED(owner);
    NOTUSED(type);

    if(f_user_info.exists())
    {
        libdbproxy::value const value(f_user_info.get_value(name_t::SNAP_NAME_USERS_IDENTIFIER));
        if(!value.nullValue())
        {
            QString const site_key(f_snap->get_site_key_with_slash());
            QString const user_path(QString("%1%2/%3")
                                    .arg(site_key)
                                    .arg(get_name(name_t::SNAP_NAME_USERS_PATH))
                                    .arg(value.int64Value())
                                    );

            content::path_info_t user_ipath;
            user_ipath.set_path(user_path);

            QString const link_name(get_name(name_t::SNAP_NAME_USERS_AUTHOR));
            bool const source_unique(true);
            links::link_info source(link_name, source_unique, ipath.get_key(), ipath.get_branch());
            QString const link_to(get_name(name_t::SNAP_NAME_USERS_AUTHORED_PAGES));
            bool const destination_multi(false);
            links::link_info destination(link_to, destination_multi, user_ipath.get_key(), user_ipath.get_branch());
            links::links::instance()->create_link(source, destination);
        }
    }
}


/** \brief Verification of a user.
 *
 * Whenever we generate a registration thank you email, we include a link
 * so the user can verify his email address. This verification happens
 * when the user clicks on the link and is sent to this very function.
 *
 * The path will look like this:
 *
 * \code
 * http[s]://<domain-name>/<path>/verify/<session>
 * \endcode
 *
 * The result is a verified tag on the user so that way we can let the
 * user log in without additional anything.
 *
 * Note that the user agent check can be turned off by software.
 *
 * \todo
 * As an additional verification we could use the cookie that was setup
 * to make sure that the user is the same person. This means the cookie
 * should not be deleted on closure in the event the user is to confirm
 * his email later and wants to close everything in the meantime. Also
 * that would not be good if user A creates an account for user B...
 *
 * \param[in,out] ipath  The path used to access this page.
 */
void users::verify_user(content::path_info_t & ipath)
{
    libdbproxy::table::pointer_t users_table(get_users_table());

    if(f_user_info.is_user())
    {
        // TODO: consider moving this parameter to the /admin/settings/users
        //       page instead (unless we want to force a "save to sites table"?)
        //
        libdbproxy::value const multiuser(f_snap->get_site_parameter(get_name(name_t::SNAP_NAME_USERS_MULTIUSER)));
        if(multiuser.nullValue() || !multiuser.signedCharValue())
        {
            // user is logged in already, just send him to his profile
            // (if logged in he was verified in some way!)
            //
            f_snap->page_redirect("user/me", snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
            NOTREACHED();
        }

        // this computer is expected to be used by multiple users, the
        // link to /verify/### and /verify/send may be followed on a
        // computer with a logged in user (because we provide those
        // in the email we send just after registration)
        //
        // So in this case we want to log out the current user and
        // process the form as if no one had been logged in.
        //
        f_info->set_object_path(user_info_t::get_full_anonymous_path());

        int32_t const total_session_duration(get_total_session_duration());
        f_info->set_time_to_live(total_session_duration);

        bool const new_random(f_info->get_date() + NEW_RANDOM_INTERVAL < f_snap->get_start_date());

        // drop the referrer if there is one, it is a security
        // issue to keep that info on an almost explicit log out!
        //
        NOTUSED(detach_referrer(f_user_info));

        sessions::sessions::instance()->save_session(*f_info, new_random);

        QString const user_cookie_name(get_user_cookie_name());
        http_cookie cookie(
                f_snap,
                user_cookie_name,
                QString("%1/%2").arg(f_info->get_session_key()).arg(f_info->get_session_random())
            );
        cookie.set_expire_in(f_info->get_time_to_live());
        cookie.set_http_only(); // make it a tad bit safer
        f_snap->set_cookie(cookie);

        // Save the date when the user logged out
        //
        libdbproxy::value value;
        value.setInt64Value(f_snap->get_start_date());
        f_user_info.set_value(name_t::SNAP_NAME_USERS_LOGOUT_ON, value);

        // Save the user IP address when logged out
        //
        value.setStringValue(f_snap->snapenv(snap::get_name(snap::name_t::SNAP_NAME_CORE_REMOTE_ADDR)));
        f_user_info.set_value(name_t::SNAP_NAME_USERS_LOGOUT_IP, value);

        // Login session was destroyed so we really do not need it here anymore
        //
        QString const last_login_session(f_user_info.get_value(name_t::SNAP_NAME_USERS_LOGIN_SESSION).stringValue());
        if(last_login_session == f_info->get_session_key())
        {
            // when clicking the "Log Out" button, we may already have been
            // logged out and if that is the case the session may not be
            // the same, hence the previous test to make sure we only delete
            // the session identifier that correspond to the last session
            //
            f_user_info.delete_value(name_t::SNAP_NAME_USERS_LOGIN_SESSION);
        }

        f_user_info.reset();
    }

    // remove "verify/" to retrieve the session ID
    //
    QString const session_id(ipath.get_cpath().mid(7));
    sessions::sessions::session_info info;
    sessions::sessions * session(sessions::sessions::instance());
    // TODO: remove the ending characters such as " ", "/", "\" and "|"?
    //       (it happens that people add those by mistake at the end of a URI...)
    session->load_session(session_id, info);
    libdbproxy::value verify_ignore_user_agent(f_snap->get_site_parameter(get_name(name_t::SNAP_NAME_USERS_VERIFY_IGNORE_USER_AGENT)));
    QString const path(info.get_object_path());
    bool ok(false);
    libdbproxy::value const id_val(static_cast<identifier_t>(path.mid(6).toLongLong(&ok))); // this is the identifier from the session (SNAP-258)
    //
    if( (info.get_session_type() != sessions::sessions::session_info::session_info_type_t::SESSION_INFO_VALID)
        || ((info.add_check_flags(0) & info.CHECK_HTTP_USER_AGENT) != 0
                && verify_ignore_user_agent.safeSignedCharValue(0, 0) == 0
                && info.get_user_agent() != f_snap->snapenv(snap::get_name(snap::name_t::SNAP_NAME_CORE_HTTP_USER_AGENT)))
        || (path.mid(0, 6) != user_info_t::get_full_anonymous_path())
        || !ok
      )
    {
        // it failed, the session could not be loaded properly
        SNAP_LOG_WARNING("users::verify_user() could not load the user session ")
                            (session_id)(" properly. Session error: ")
                            (sessions::sessions::session_info::session_type_to_string(info.get_session_type()))(".");

        // TODO change message support to use strings from the database so they can get translated
        if(info.get_session_type() == sessions::sessions::session_info::session_info_type_t::SESSION_INFO_OUT_OF_DATE)
        {
            messages::messages::instance()->set_warning(
                "Expired User Verification Code",
                QString("The specified email verification code (%1) expired."
                        " Please <a href=\"/verify/resend\">get a new code</a> and try verifying it again."
                        " The system gives you 3 days to take care of your email verification.")
                        .arg(session_id),
                QString("user trying his verification with code \"%1\" got error: %2.")
                        .arg(session_id)
                        .arg(sessions::sessions::session_info::session_type_to_string(info.get_session_type()))
            );
        }
        else
        {
            messages::messages::instance()->set_error(
                "Invalid User Verification Code",
                QString("The specified email verification code (%1) is not correct."
                        " Please verify that you used the correct link or try to use the form below to enter your verification code."
                        " If you already followed the link once, then you already were verified and all you need to do is click the log in link below.")
                        .arg(session_id),
                QString("user trying his verification with code \"%1\" got error: %2.")
                        .arg(session_id)
                        .arg(sessions::sessions::session_info::session_type_to_string(info.get_session_type())),
                true
            );
        }

        // redirect the user to the verification form
        f_snap->page_redirect("verify", snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
        NOTREACHED();
    }

    // it looks like the session is valid, get the user email and verify
    // that the account exists in the database
    //
    if(!users_table->exists(id_val.binaryValue()))
    {
        // This should never happen...
        messages::messages::instance()->set_error(
            "Could Not Find Your Account",
            "Somehow we could not find your account on this system.",
            QString("user account for \"%1\" does not exist at this point").arg( id_val.stringValue()),
            true
        );
        // redirect the user to the log in page

        f_snap->page_redirect("login", snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
        NOTREACHED();
    }

    // SNAP-258: use identifier instead of user key (canonicialized email address).
    //
    identifier_t const identifier(id_val.int64Value());
    user_info_t user_info( get_user_info_by_id(identifier) );
    if( !user_info.is_user() )
    {
        SNAP_LOG_FATAL("users::verify_user() could not load the user information! (user_key=")
                        (user_info.get_user_key())(")");
        // redirect the user to the verification form although it won't work
        // next time either...
        f_snap->page_redirect("verify", snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
        NOTREACHED();
    }

    content::path_info_t user_ipath;
    user_ipath.set_path(QString("%1/%2").arg(get_name(name_t::SNAP_NAME_USERS_PATH)).arg(identifier));

    // before we actually accept this verification code, we must make sure
    // the user is still marked as a new user (he should or the session
    // would be invalid, but for security it is better to check again)
    links::link_info user_status_info(get_name(name_t::SNAP_NAME_USERS_STATUS), true, user_ipath.get_key(), user_ipath.get_branch());
    QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(user_status_info));
    links::link_info status_info;
    if(!link_ctxt->next_link(status_info))
    {
        // This should never happen... because the session should logically
        // prevent it from happening (i.e. the status link should always be
        // there) although maybe the admin could delete this link somehow?
        messages::messages::instance()->set_error(
            "Not a New Account",
            "Your account is not marked as a new account. The verification failed.",
            QString("user account for \"%1\", which is being verified, is not marked as being a new account").arg(user_info.get_user_email()),
            true
        );
        // redirect the user to the log in page
        f_snap->page_redirect("login", snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
        NOTREACHED();
    }

    // a status link exists...
    QString const site_key(f_snap->get_site_key_with_slash());
    if(status_info.key() != site_key + get_name(name_t::SNAP_NAME_USERS_NEW_PATH))
    {
        // This should never happen... because the session should logically
        // prevent it from happening (i.e. the status link should always be
        // there) although maybe the admin could delete this link somehow?
        messages::messages::instance()->set_error(
            "Not a New Account",
            "Your account is not marked as a new account. The verification failed. You may have been blocked.",
            QString("user account for \"%1\", which is being verified, is not marked as being a new account: %2").arg(user_info.get_user_email()).arg(status_info.key()),
            true
        );
        // redirect the user to the log in page? (XXX should this be the registration page instead?)
        f_snap->page_redirect("login", snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
        NOTREACHED();
    }
    // remove the "user/new" status link so the user can now log in
    // he was successfully verified
    links::links::instance()->delete_link(user_status_info);

    // Save the date when the user verified
    libdbproxy::value value;
    value.setInt64Value(f_snap->get_start_date());
    user_info.set_value(name_t::SNAP_NAME_USERS_VERIFIED_ON, value);

    // Save the user IP address when verified
    value.setStringValue(f_snap->snapenv(snap::get_name(snap::name_t::SNAP_NAME_CORE_REMOTE_ADDR)));
    user_info.set_value(name_t::SNAP_NAME_USERS_VERIFIED_IP, value);

    // tell other plugins that a new user was created and let them add
    // bells and whisles to the new account
    user_verified(user_ipath, identifier);

    // TODO offer an auto-log in feature
    //      (TBD: this could be done by another plugin via the
    //      user_verified() signal although it makes a lot more sense to
    //      let the users plugin to do such a thing!)

    // send the user to the log in page since he got verified now
    messages::messages::instance()->set_info(
        "Verified!",
        "Thank you for registering an account with us. Your account is now verified! You can now log in with the form below."
    );
    f_snap->page_redirect("login", snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
    NOTREACHED();
}


/** \fn void users::user_verified(content::path_info_t& ipath, int64_t identifier)
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


/** \brief Log a user in.
 *
 * This function can be used to log a user in. You have to be extremely
 * careful to not create a way to log a user without proper credential.
 * This is generally used when a mechanism such a third party authentication
 * mechanism is used to log the user in his account.
 *
 * If the \p password parameter is empty, the system creates a user session
 * without verify the user password. This is the case where another
 * mechanism must have been used to properly log the user before calling
 * this function.
 *
 * The function still verifies that the user was properly verified and
 * not blocked. It also makes sure that the user password does not need
 * to be changed. If a password change is required for that user, then
 * the login fails.
 *
 * \param[in] email  The user email address.
 * \param[in] password  The password to log the user in.
 * \param[out] validation_required  Whether the user needs to validate his account.
 * \param[in] login_mode  The mode used to log in: full, verification.
 * \param[in] password_policy  The policy used to log the user in.
 *
 * \return A string representing an error, an empty string if the login
 *         worked and the user is not being redirected. If the error is
 *         "user validation required" then the validation_required flag
 *         is set to false.
 */
QString users::login_user(QString const & email, QString const & password, bool & validation_required, login_mode_t login_mode, QString const & password_policy)
{
    validation_required = false;
    user_info_t user_info(get_user_info_by_email(email));
//SNAP_LOG_TRACE("email=")(email);

    if(user_info.exists())
    {
//SNAP_LOG_TRACE("user exists");
        libdbproxy::value value;

        // existing users have a unique identifier
        if( !user_info.is_user() )
        {
            messages::messages::instance()->set_error(
                "Could Not Log You In",
                "Somehow your user identifier is not available. Without it we cannot log your in.",
                QString("users::login_user() could not load the user identifier, the row exists but the cell did not make it (%1/%2).")
                        .arg(user_info.get_user_key())
                        .arg(get_name(name_t::SNAP_NAME_USERS_IDENTIFIER)),
                false
            );
            if(login_mode == login_mode_t::LOGIN_MODE_VERIFICATION)
            {
                // force a log out because the user should not be remotely
                // logged in in any way...
                f_snap->page_redirect("logout", snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
            }
            else
            {
                // XXX should we redirect to some error page in that regard?
                //     (i.e. your user account is messed up, please contact us?)
                f_snap->page_redirect("verify", snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
            }
            NOTREACHED();
        }

        user_logged_info_t logged_info( f_snap, user_info );
        logged_info.set_password_policy(password_policy);
        logged_info.set_identifier(user_info.get_identifier());

        // although the user exists, as in, has an account on this Snap!
        // website, that account may not be attached to this website so
        // we need to verify that before moving further.
        libdbproxy::table::pointer_t content_table(content::content::instance()->get_content_table());
        content::path_info_t ipath(logged_info.user_ipath());
        if(!content_table->exists(ipath.get_key()))
        {
            return "it looks like you have an account on this Snap! system but not this specific website. Please register on this website and try again";
        }

        // before we actually log the user in we must make sure he is
        // not currently blocked or not yet active
        links::link_info user_status_info(get_name(name_t::SNAP_NAME_USERS_STATUS), true, ipath.get_key(), ipath.get_branch());
        QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(user_status_info));
        links::link_info status_info;
        bool valid(true);
        if(link_ctxt->next_link(status_info))
        {
            QString const site_key(f_snap->get_site_key_with_slash());

//std::cerr << "***\n*** Current user status on log in is [" << status_info.key() << "] / [" << (site_key + get_name(name_t::SNAP_NAME_USERS_PASSWORD_PATH)) << "]\n***\n";
            // the status link exists...
            // this means the user is either a new user (not yet verified)
            // or he is blocked
            // either way it means he cannot log in at this time!
            if(status_info.key() == site_key + get_name(name_t::SNAP_NAME_USERS_NEW_PATH))
            {
                validation_required = true;
                return "user's account is not yet active (not yet verified)";
            }
            else if(status_info.key() == site_key + get_name(name_t::SNAP_NAME_USERS_BLOCKED_PATH))
            {
                return "user's account is blocked";
            }
            else if(status_info.key() == site_key + get_name(name_t::SNAP_NAME_USERS_AUTO_PATH))
            {
                return "user did not register, this is an auto-account only";
            }
            else if(status_info.key() == site_key + get_name(name_t::SNAP_NAME_USERS_PASSWORD_PATH))
            {
                if(password.isEmpty())
                {
                    return "user has to update his password, this application cannot currently log this user in";
                }
                // user requested a new password but it looks like he
                // remembered the old one in between; for redirect this user
                // to the password form
                //
                // since the user knows his old password, we can log him in
                // and send him to the full fledge password change form
                //
                // note that the status will not change until the user saves
                // his new password so this redirection will happen again and
                // again until the password gets changed
                logged_info.force_password_change();
            }
            // ignore other statuses at this point
        }
        if(valid)
        {
//SNAP_LOG_TRACE("is valid");
            bool valid_password(password.isEmpty());
            if(!valid_password)
            {
                // compute the hash of the password
                // (1) get the digest
                value = user_info.get_value(name_t::SNAP_NAME_USERS_PASSWORD_DIGEST);
                QString const digest(value.stringValue());

                // (2) we need the passord (passed as a parameter now)
                //QString const password(f_snap->postenv("password"));

                // (3) get the salt in a buffer
                value = user_info.get_value(name_t::SNAP_NAME_USERS_PASSWORD_SALT);
                QByteArray const salt(value.binaryValue());

                // (4) compute the expected hash
                QByteArray hash;
                encrypt_password(digest, password, salt, hash);

                // (5) retrieved the saved hash
                value = user_info.get_value(name_t::SNAP_NAME_USERS_PASSWORD);
                QByteArray const saved_hash(value.binaryValue());

                // (6) compare both hashes
                // (note: at this point I do not trust the == operator of the QByteArray
                // object; will it work with '\0' bytes???)
                valid_password = hash.size() == saved_hash.size()
                              && memcmp(hash.data(), saved_hash.data(), hash.size()) == 0;

                // make sure the user password was not blocked
                //
                //libdbproxy::value password_blocked;
                if(user_info.value_exists(name_t::SNAP_NAME_USERS_PASSWORD_BLOCKED))
                {
                    // increase the 503s counter and block the IP if
                    // we received too many attempts
                    //
                    if(!valid_password)
                    {
                        blocked_user(user_info, "users");
                    }

                    f_snap->die(snap_child::http_code_t::HTTP_CODE_SERVICE_UNAVAILABLE,
                            "Service Not Available",
                            // WARNING: with the password was valid CANNOT be
                            //          given to the client since this could
                            //          be the hacker, thus this message does
                            //          not change either way.
                            "The server is not currently available for users to login.",
                            (valid_password
                                ? "This time the user entered the correct password, unfortunately, the password has been blocked earlier"
                                : "Trying to reject a hacker since we got too many attempts at login in with an invalid password"));
                    NOTREACHED();
                }
            }

            if(valid_password)
            {
//SNAP_LOG_TRACE("valid password");
                // User credentials are correct, create a session & cookie
                create_logged_in_user_session(user_info);

                // Copy the previous login date and IP to the previous fields
                if(user_info.value_exists(get_name(name_t::SNAP_NAME_USERS_LOGIN_ON)))
                {
                    user_info.set_value( name_t::SNAP_NAME_USERS_PREVIOUS_LOGIN_ON, user_info.get_value(name_t::SNAP_NAME_USERS_LOGIN_ON) );
                }
                if(user_info.value_exists(get_name(name_t::SNAP_NAME_USERS_LOGIN_IP)))
                {
                    user_info.set_value( name_t::SNAP_NAME_USERS_PREVIOUS_LOGIN_IP, user_info.get_value(name_t::SNAP_NAME_USERS_LOGIN_IP) );
                }

                // Save the date when the user logged in
                value.setInt64Value(f_snap->get_start_date());
                user_info.set_value(name_t::SNAP_NAME_USERS_LOGIN_ON, value);

                // Save the user IP address when logging in
                value.setStringValue(f_snap->snapenv(snap::get_name(snap::name_t::SNAP_NAME_CORE_REMOTE_ADDR)));
                user_info.set_value(name_t::SNAP_NAME_USERS_LOGIN_IP, value);

                // Save the user latest session so we can implement the
                // "one session per user" feature (which is the default)
                user_info.set_value(name_t::SNAP_NAME_USERS_LOGIN_SESSION, f_info->get_session_key());

                // Tell all the other plugins that the user is now logged in
                // you may specify a URI to where the user should be sent on
                // log in, used in the redirect below, although we will go
                // to user/password whatever the path is specified here
                //
                // Also, put a fresh copy of the user info into the logged_info object.
                //
                logged_info.set_user_info(user_info);
                user_logged_in(logged_info);

                // user got logged out by a plugin and not redirected?!
                if(f_user_info.is_user())
                {
                    // make sure user locale/timezone get used on next
                    // locale/timezone access
                    locale::locale::instance()->reset_locale();

                    // send a signal that the user is ready (this signal is also
                    // sent when we have a valid cookie)
                    logged_in_user_ready();

                    if(password.isEmpty())
                    {
                        // This looks like an API login someone, we just
                        // return and let the caller handle the rest
                        //
                        return QString();
                    }

                    if(logged_info.is_password_change_required())
                    {
                        // this URI has priority over other plugins URIs
                        //
                        logged_info.set_uri("user/password");
                    }
                    else if(logged_info.get_uri().isEmpty())
                    {
                        // here we detach from the session since we want to
                        // redirect only once to that page
                        //
                        QString const referrer(detach_referrer(f_user_info));
                        logged_info.set_uri(referrer);
                        if(logged_info.get_uri().isEmpty())
                        {
                            // User is now logged in, redirect him
                            //
                            libdbproxy::value login_redirect(f_snap->get_site_parameter(get_name(name_t::SNAP_NAME_USERS_LOGIN_REDIRECT)));
                            if(login_redirect.nullValue())
                            {
                                // by default redirect to user profile
                                //
                                logged_info.set_uri("user/me");
                            }
                            else
                            {
                                // administrator changed the default redirect
                                // on log in to the value in login_redirect
                                //
                                logged_info.set_uri(login_redirect.stringValue());
                            }
                        }
                    }
                    f_snap->page_redirect(logged_info.get_uri(), snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
                    NOTREACHED();
                }

                // user does not have enough permission to log in?
                // (i.e. a pay for website where the account has no more
                //       credit and this very user is not responsible for
                //       the payment)
//SNAP_LOG_TRACE("good credentials, invalid status");
                return "good credentials, invalid status according to another plugin that logged the user out immediately";
            }
            else
            {
//SNAP_LOG_TRACE("invalid credentials: password mismatch");
                // user mistyped his password
                //
                invalid_password(user_info, "users");
                return "invalid credentials (password does not match)";
            }
        }
    }

    // user mistyped his email or is not registered?
//SNAP_LOG_TRACE("invalid credentials: email does not exist");
    return "invalid credentials (user with specified email does not exist)";
}


/** \brief Actually mark user as logged in.
 *
 * NEVER call that function to log a user in. This function is called
 * once all the credentials for a user were checked and accepted. This
 * will mark the user as logged in.
 *
 * The session generates a warning message if there was another session
 * in another browser or another computer (i.e. a different session
 * identifier.)
 *
 * \param[in] user_info  The key to the user (NOT the raw email address).
 */
void users::create_logged_in_user_session( user_info_t const & user_info )
{
    // log the user in by adding the correct object path
    // the other parameters were already defined in the
    // on_process_cookies() function
    //
    QString const user_basepath(user_info.get_user_path(true));
    f_info->set_object_path(user_basepath);

    // define the total duration of the session (usually 1 year)
    //
    int64_t const total_session_duration(get_total_session_duration());
    f_info->set_time_to_live(total_session_duration);

    // define the user duration (standard login)
    //
    int64_t const user_session_duration(get_user_session_duration());
    f_info->set_time_limit(f_snap->get_start_time() + user_session_duration);

    // define the administrator duration (admin login)
    //
    int64_t const administrative_session_duration(get_administrative_session_duration());
    f_info->set_administrative_login_limit(f_snap->get_start_time() + administrative_session_duration);

    // save the info in the session
    //
    sessions::sessions::instance()->save_session(*f_info, true); // force new random session number

    // add another parameter so we always know whether the user was
    // logged in before even if he logs out and becomes anonymous again
    //
    // this should never be detached, only retrieved
    //
    attach_to_session(get_name(name_t::SNAP_NAME_USERS_LAST_USER_PATH), user_basepath);

    // if there was another active login for that very user,
    // we want to cancel it and also display a message to the
    // user about the fact
    //
    QString const previous_session(user_info.get_value(name_t::SNAP_NAME_USERS_LOGIN_SESSION).stringValue());
    if(!previous_session.isEmpty() && previous_session != f_info->get_session_key())
    {
        // Administrator can turn off that feature
        //
        libdbproxy::value const multisessions(f_snap->get_site_parameter(get_name(name_t::SNAP_NAME_USERS_MULTISESSIONS)));
        if(multisessions.nullValue() || !multisessions.signedCharValue())
        {
            // close other session
            //
            sessions::sessions::session_info old_session;
            login_status_t const display_warning(load_login_session(previous_session, old_session, true));

            // whether the user could have been logged in, make sure to close the session
            //
            old_session.set_object_path(user_info_t::get_full_anonymous_path());

            // drop the referrers if there are any, it is a security
            // issue to keep that info on an "explicit" log out!
            //
            // IMPORTANT: we use the session call directly because we
            //            are detaching from "old_session" and not the
            //            current session
            //
            NOTUSED(sessions::sessions::instance()->detach_from_session(old_session, get_name(name_t::SNAP_NAME_USERS_LOGIN_REFERRER)));
            NOTUSED(sessions::sessions::instance()->detach_from_session(old_session, referrer_identifier(user_info)));

            sessions::sessions::instance()->save_session(old_session, false);

            // if the user could have been logged in, emit a warning
            //
            // We ignore the User Agent error since in many cases
            // the log fails because you try to log in a different
            // browser in which case you always need a new session.
            //
            if((display_warning & ~LOGIN_STATUS_USER_AGENT_MISMATCH) == LOGIN_STATUS_OK)
            {
                messages::messages::instance()->set_warning(
                    "Two Sessions",
                    "We detected that you had another session opened. The other session was closed.",
                    QString("users::login_user() deleted old session \"%1\" for user \"%2\".")
                                 .arg(old_session.get_session_key())
                                 .arg(user_info.get_user_key())
                );

                // go on, this is not a fatal error
            }
        }
    }

    QString const user_cookie_name(get_user_cookie_name());
    http_cookie cookie(
                f_snap,
                user_cookie_name,
                QString("%1/%2").arg(f_info->get_session_key()).arg(f_info->get_session_random())
            );
    cookie.set_expire_in(f_info->get_time_to_live());
    cookie.set_http_only(); // make it a tad bit safer
    f_snap->set_cookie(cookie);

    // this is now the current user
    f_user_info = user_info;
    // we just logged in so we are logged in
    // (although the user_logged_in() signal could log the
    // user out if something is awry)
    f_user_logged_in = true;
}


/** \fn void users::user_logged_in(user_logged_info_t & logged_info)
 * \brief Tell plugins that the user is now logged in.
 *
 * This signal is used to tell plugins that the user is now logged in.
 *
 * Note I: this signal only happens at the time the user logs in, not
 * each time the user accesses the server.
 *
 * Note II: a plugin has the capability to log the user out by calling
 * the user_logout() function; this means when your callback gets called
 * the user may not be logged in anymore! This means you should always
 * make a call as follow to verify that the user is indeed logged in
 * before making use of the user's information:
 *
 * \code
 *      // this:
 *      if(!users::users::instance()->user_has_administrative_rights())
 *      {
 *          return;
 *      }
 *      // or this:
 *      if(!users::users::instance()->user_is_logged_in())
 *      {
 *          return;
 *      }
 * \endcode
 *
 * In most cases the plugins are expected to check one thing or another that
 * may be important for that user and act accordingly. If the result is that
 * the user should be sent to a specific page, then the plugin can set the
 * f_uri parameter of the logged_in parameter to that page URI.
 *
 * Note that if multiple plugins want to redirect the user, then which URI
 * should be used is not defined. We may later do a 303 where the system lets
 * the user choose which page to go to. At this time, the last plugin that
 * sets the URI has priority. Note that of course a plugin can decide not
 * to change the URI if it is already set.
 *
 * If your plugin determines that the user should change his password,
 * then it can use one of the two functions in the user_logged_info_t
 * class to enforce such.
 *
 * \note
 * It is important to remind you that if the system has to send the user to
 * change his password, it will do so, whether a plugin sets another URI
 * or not.
 *
 * \param[in] logged_info  The user login information.
 */


/** \brief Check the current status of the specified user.
 *
 * This function checks the status of the user specified by an
 * email address.
 *
 * \note
 * The function returns STATUS_UNDEFINED if the email address is
 * the empty string.
 *
 * \note
 * The function returns STATUS_UNKNOWN if the status is not known
 * by the users plugin. The status itself is saved in the status_key
 * parameter so one can further check what the status is and act on
 * it appropriately.
 *
 * \todo
 * Allow the use of the user path and user identifier instead of
 * just the email address.
 *
 * \param[in] email  The email address of the user being checked. It does
 *                   not need to be canonicalized yet (i.e. a user_key.)
 * \param[out] status_key  Return the status key if available.
 *
 * \return The status of the user.
 */
users::status_t users::user_status_from_email(QString const & email, QString & status_key)
{
    status_key.clear();

    if(email.isEmpty())
    {
        return status_t::STATUS_UNDEFINED;
    }

    // user_info does the necessary email to user_key conversion
    //
    user_info_t user_info(get_user_info_by_email(email));
    QString const user_path( user_info.get_user_path(false) );
    if(user_path == get_name(name_t::SNAP_NAME_USERS_ANONYMOUS_PATH))
    {
        return status_t::STATUS_NOT_FOUND;
    }

    return user_status_from_user_path(user_path, status_key);
}


/** \brief Check the current status of the specified user.
 *
 * This function checks the status of the user specified by an
 * email address.
 *
 * \note
 * The function returns STATUS_UNDEFINED if the email address is
 * the empty string.
 *
 * \note
 * The function returns STATUS_UNKNOWN if the status is not known
 * by the users plugin. The status itself is saved in the status_key
 * parameter so one can further check what the status is and act on
 * it appropriately.
 *
 * \todo
 * Allow the use of the user path and user identifier instead of
 * just the email address.
 *
 * \param[in] email  The email address of the user being checked. It does
 *                   not need to be canonicalized yet (i.e. a user_key.)
 * \param[out] status_key  Return the status key if available.
 *
 * \return The status of the user.
 */
users::status_t users::user_status_from_identifier(int64_t identifier, QString & status_key)
{
    status_key.clear();

    if(identifier <= 0)
    {
        return status_t::STATUS_UNDEFINED;
    }

    return user_status_from_user_path(QString("user/%1").arg(identifier), status_key);
}


/** \brief Check the current status of the specified user.
 *
 * This function checks the status of the user specified by an
 * email address.
 *
 * \note
 * The function returns STATUS_UNDEFINED if the email address is
 * the empty string.
 *
 * \note
 * The function returns STATUS_UNKNOWN if the status is not known
 * by the users plugin. The status itself is saved in the status_key
 * parameter so one can further check what the status is and act on
 * it appropriately.
 *
 * \todo
 * Allow the use of the user path and user identifier instead of
 * just the email address.
 *
 * \param[in] user_path  The path to the user (i.e. "/user/3")
 * \param[out] status_key  Return the status key if available.
 *
 * \return The status of the user.
 */
users::status_t users::user_status_from_user_path(QString const & user_path, QString & status_key)
{
    status_key.clear();

    content::path_info_t user_ipath;
    user_ipath.set_path(user_path);

    // before we actually accept this verification code, we must make sure
    // the user is still marked as a new user (he should or the session
    // would be invalid, but for security it is better to check again)
    links::link_info user_status_info(get_name(name_t::SNAP_NAME_USERS_STATUS), true, user_ipath.get_key(), user_ipath.get_branch());
    QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(user_status_info));
    links::link_info status_info;
    if(!link_ctxt->next_link(status_info))
    {
        // if the status link does not exist, then the user is considered
        // verified and valid
        return status_t::STATUS_VALID;
    }
    status_key = status_info.key();

    // a status link exists... check that the user is not marked as a NEW user
    QString const site_key(f_snap->get_site_key_with_slash());
    if(status_key == site_key + get_name(name_t::SNAP_NAME_USERS_NEW_PATH))
    {
        return status_t::STATUS_NEW;
    }
    if(status_key == site_key + get_name(name_t::SNAP_NAME_USERS_BLOCKED_PATH))
    {
        return status_t::STATUS_BLOCKED;
    }
    if(status_key == site_key + get_name(name_t::SNAP_NAME_USERS_AUTO_PATH))
    {
        return status_t::STATUS_AUTO;
    }
    if(status_key == site_key + get_name(name_t::SNAP_NAME_USERS_PASSWORD_PATH))
    {
        return status_t::STATUS_PASSWORD;
    }

    SNAP_LOG_WARNING("Unknown user status \"")(status_key)("\" in user_status(). [")(site_key + get_name(name_t::SNAP_NAME_USERS_PASSWORD_PATH))("]");

    // anything else we do not know what the heck it is
    // (we will need a signal to allow for extensions by other plugins)
    return status_t::STATUS_UNKNOWN;
}


/** \brief Given a user path, return his email address.
 *
 * This function transforms the specified user path and transforms it
 * in his identifier and then it calls the other get_user_email()
 * function.
 *
 * The user path may or not include the site key. Both cases function
 * perfectly.
 *
 * \warning
 * This function returns the current email exactly as provided by the
 * end user when registering or changing email. The user key may be
 * different (i.e. generally all written in lowercase.) You can transform
 * this email to a valid user key (to query the users table) by calling
 * the email_to_user_key() function. If you already have the email, just
 * calling email_to_user_key() is the fastest way to get the user key.
 *
 * \param[in] user_path  The path to the user data in the content table.
 *
 * \return The email address of the user, if the user is defined in the
 *         database.
 */
QString users::get_user_email(QString const & user_path) const
{
    return get_user_email(user_info_t::get_user_id_by_path(user_path));
}


/** \brief Given a user identifier, return his email address.
 *
 * The email address of a user is the key used to access his private
 * data in the users table.
 *
 * Note that an invalid identifier will make this function return an
 * empty string (i.e. not such user.)
 *
 * \warning
 * This function returns the current email exactly as provided by the
 * end user when registering or changing email. The user key may be
 * different (i.e. generally all written in lowercase.) You can transform
 * this email to a valid user key (to query the users table) by calling
 * the email_to_user_key() function. If you already have the email, just
 * calling email_to_user_key() is the fastest way to get the user key.
 *
 * \param[in] identifier  The identifier of the user to retrieve the email for.
 *
 * \return The email address of the user, if the user is defined in the
 *         database.
 *
 * \sa email_to_user_key()
 */
QString users::get_user_email(identifier_t const identifier) const
{
    if(identifier > 0)
    {
        user_info_t user_info(get_user_info_by_id(identifier));
        if(user_info.exists())
        {
            // found the user, retrieve the current email
            return user_info.get_user_email();
        }
    }

    return QString();
}


/** \brief Get the path to a user from an email.
 *
 * This function returns the path of the user corresponding to the specified
 * email. The function returns an empty string if the user is not found.
 *
 * \param[in] email  The email of the user to search the path for.
 *
 * \return The path to the user.
 */
QString users::get_user_path(QString const & email) const
{
    user_info_t user_info(get_user_info_by_email(email));
    return user_info.get_user_path(false);
}


users::user_info_t users::get_user_info_by_id( identifier_t const & id )
{
    user_info_t user_info(id);
    return user_info;
}


users::user_info_t users::get_user_info_by_email( QString const & email )
{
    // user_info(QString) expects either an email or a path...
    // so this does the same as users::get_user_info_by_path()
    //
    user_info_t user_info(email);
    return user_info;
}


users::user_info_t users::get_user_info_by_path( QString const & path )
{
    // user_info(QString) expects either an email or a path...
    // so this does the same as users::get_user_info_by_email()
    //
    user_info_t user_info(path);
    return user_info;
}


users::user_info_t users::get_user_info_by_name( QString const & name )
{
    user_info_t user_info(name);
    return user_info;
}


users::user_info_t users::get_last_logged_in_user_info() const
{
    // if the current session includes a SNAP_NAME_USERS_LAST_USER_PATH
    // field then return the user_info_t for that user
    //
    QString const user_path(get_from_session(get_name(name_t::SNAP_NAME_USERS_LAST_USER_PATH)));
    return get_user_info_by_path(user_path);
}


/** \brief Define the login referrer field name.
 *
 * If the specified user_info is not a nullptr and an identifier is defined
 * (i.e. it is not set to IDENTIFIER_INVALID) then the function returns a
 * field name including the user identifier:
 *
 * \code
 *      "users::login_referrer::123"
 * \endcode
 *
 * If no user is defined, then the base field name is returned:
 *
 * \code
 *      "users::login_referrer"
 * \endcode
 *
 * \param[in] user_info  The information about the user or nullptr.
 *
 * \return The field name to access the login referrer.
 */
QString users::referrer_identifier(user_info_t const & user_info)
{
    identifier_t id(user_info.get_identifier());
    if(!user_info.is_user())
    {
        user_info_t const last_user_info(get_last_logged_in_user_info());
        if(!last_user_info.is_user())
        {
            // cannot determine the user, make it user agnostic
            //
            return QString(get_name(name_t::SNAP_NAME_USERS_LOGIN_REFERRER));
        }
        id = last_user_info.get_identifier();
    }

    return QString("%1::%2")
            .arg(get_name(name_t::SNAP_NAME_USERS_LOGIN_REFERRER))
            .arg(id);
}


/** \brief Register a new user in the database
 *
 * If you find out that a user is not yet registered but still want to
 * save some information about that user (i.e. when sending an email to
 * someone) then this function is used for that purpose.
 *
 * This function accepts an email and a password. The password can be set
 * to "!" to prevent that user from logging in (password too small!) but
 * still have an account. The account can later be activated, which
 * happens whenever the user decides to register "for real" (i.e. the
 * "!" accounts are often used for users added to mailing lists and alike.)
 *
 * If you are creating a user as an administrator or similar role, you
 * may want to give the user a full account. This is doable by creating
 * a random password and passing that password to this function. The
 * user will be considered fully registered in that case. The password
 * can be generated using the create_password() function.
 *
 * \important
 * The \p email parameter is expected to be the email exactly the way
 * the user typed it. This can be important in the event the user
 * mail system expects the case of the username to match one to one.
 *
 * \param[in] email  The email of the user. It must be a valid email address.
 * \param[in] password  The password of the user or "!".
 * \param[out] reason  If the function returns something other than STATUS_NEW
 *                     or STATUS_VALID, the reason for the error.
 * \param[in] allow_example_domain  Whether an email address with the name
 *                                  example is allowed or not.
 *
 * \return STATUS_NEW if the user was just created and a verification email
 *         is expected to be sent to him or her;
 *         STATUS_VALID if the user was accepted in this website and already
 *         verified his email address;
 *         STATUS_BLOCKED if this email address is blocked on this website
 *         or entire Snap! environment or the user already exists but was
 *         blocked by an administrator;
 *         STATUS_PASSWORD and !reason.isEmpty() if the password is considered
 *         insecure: too simple or found in the password blacklist;
 */
users::status_t users::register_user(QString const & email, QString const & password, QString & reason, bool allow_example_domain)
{
    reason.clear();

    user_info_t user_info(get_user_info_by_email(email));

    // WARNING:
    //
    //   Here we use the function that computes the key directly from
    //   our email, this is because the get_user_info_by_email() will
    //   return a user_info object without an email address if the
    //   user is not valid (if the user_info_t constructor cannot
    //   find an identifier from the passed email address, it sends
    //   ends up calling the reset() function and thus removing the
    //   email address.)
    //
    //   For this reason, we call the get_user_key() function that
    //   accepts a parameter and that function computes the key from
    //   that variable instead of a currently empty variable in
    //   user_info.
    //
    //   The user_info will be invalid until we call the define_user()
    //   function. After that it can be used as is.
    //
    auto const user_key(user_info.get_user_key(email));

    libdbproxy::table::pointer_t content_table(content::content::instance()->get_content_table());
    libdbproxy::table::pointer_t users_table(get_users_table());

    libdbproxy::value value;
    value.setConsistencyLevel(libdbproxy::CONSISTENCY_LEVEL_QUORUM);
    value.setStringValue(email); // this is what we save in the user table, with upper/lowercase as given by the end user

    int64_t identifier(0);
    status_t status(status_t::STATUS_NEW);
    QString const user_identifier_lock("global-users-lock#identifier");
    QString const id_row_name(get_name(name_t::SNAP_NAME_USERS_ID_ROW));
    QString const identifier_key(get_name(name_t::SNAP_NAME_USERS_IDENTIFIER));
    QString const email_key(get_name(name_t::SNAP_NAME_USERS_ORIGINAL_EMAIL));
    QString const user_path(get_name(name_t::SNAP_NAME_USERS_PATH));
    libdbproxy::value new_identifier;
    new_identifier.setConsistencyLevel(libdbproxy::CONSISTENCY_LEVEL_QUORUM);

    // Note that the email was already checked when coming from the Register
    // form, however, it was checked for validity as an email, not checked
    // against a black list or verified in other ways; also the password
    // can this way be checked by another plugin (i.e. password database)
    //
    user_security_t security;
    security.set_user_info(user_info, email, allow_example_domain);
    security.set_password(password);
    security.set_bypass_blacklist(true);
    check_user_security(security);
    SNAP_LOG_DEBUG("security.get_secure().allowed()")(security.get_secure().allowed());
    if(!security.get_secure().allowed())
    {
        // well... someone said "do not save that user in there"!
        SNAP_LOG_ERROR("user security says no to \"")(email)("\": ")(security.get_secure().reason());
        reason = security.get_secure().reason();
        return security.get_status();
    }

    // we got as much as we could ready before locking
    bool new_user(false);
    {
        // first make sure this email is unique
        //
        snap_lock lock(user_identifier_lock);

        // TODO: when this got converted, Doug continued to check the email
        //       address but this time, in the user object itself; it seems
        //       to me that I was checking the ID in the index before, not
        //       100% sure though...
        //
        libdbproxy::value email_data;
        libdbproxy::cell::pointer_t cell(user_info.get_cell(email_key));
        if(cell != nullptr)
        {
            cell->setConsistencyLevel(libdbproxy::CONSISTENCY_LEVEL_QUORUM);
            email_data = cell->getValue();
        }
        if(!email_data.nullValue())
        {
            // TODO: move this case from under the locked block since
            //       the lock is not necessary to do this work
            //
            // "someone else" already registered with that email
            // first check whether that user exists on this website
            //
            libdbproxy::value const existing_identifier(user_info.get_value(identifier_key));
            if(existing_identifier.size() != sizeof(int64_t))
            {
                // this means no user can register until this value gets
                // fixed somehow!
                //
                messages::messages::instance()->set_error(
                    "Failed Creating User Account",
                    "Somehow we could not determine your user identifier. Please try again later.",
                    QString("users::register_user() could not load the identifier of an existing user,"
                            " the user seems to exist but the users::identifier cell seems wrong (%1/%2/%3).")
                            .arg(email)
                            .arg(user_key)
                            .arg(identifier_key),
                    false
                );
                // XXX redirect user to an error page instead?
                //     if they try again it will fail again until the
                //     database gets fixed properly...
                return status_t::STATUS_UNDEFINED;
            }
            identifier = existing_identifier.int64Value();

            // okay, so the user exists on at least one website
            // check whether it exists on this website and if not add it
            //
            // TBD: should we also check the cell with the website reference
            //      in the user table? (users::website_reference::<site_key>)
            //
            content::path_info_t existing_ipath;
            existing_ipath.set_path(QString("%1/%2").arg(user_path).arg(identifier));
            if(content_table->exists(existing_ipath.get_key()))
            {
                // it exists, just return the current status of that existing user
                QString ignore_status_key;
                status = user_status_from_email(email, ignore_status_key);
                SNAP_LOG_INFO("user \"")(email)("\" (")(user_key)(") already exists, just return its current status: ")(static_cast<int>(status))(".");
                return status;
            }
            // user exists in the Snap! system but not this website
            // so we want to add it to this website, but we will return
            // its current status "instead" of STATUS_NEW (note that
            // the current status could be STATUS_NEW if the user
            // registered in another website but did not yet verify his
            // email address.)
            //
            status = status_t::STATUS_VALID;
        }
        else
        {
            // we are the first to lock this row, the user is therefore unique
            // so go on and register him

            // In order to register the user in the contents we want a
            // unique identifier for each user, for that purpose we use
            // a special row in the users table and since we have a lock
            // we can safely do a read-increment-write cycle.
            //
            if(users_table->exists(id_row_name))
            {
                libdbproxy::row::pointer_t id_row(users_table->getRow(id_row_name));
                libdbproxy::cell::pointer_t id_cell(id_row->getCell(identifier_key));
                id_cell->setConsistencyLevel(libdbproxy::CONSISTENCY_LEVEL_QUORUM);
                libdbproxy::value const current_identifier(id_cell->getValue());
                if(current_identifier.size() != sizeof(int64_t))
                {
                    // this means no user can register until this value gets
                    // fixed somehow!
                    messages::messages::instance()->set_error(
                        "Failed Creating User Account",
                        "Somehow we could not generate a user identifier for your account. Please try again later.",
                        QString("users::register_user() could not load the *id_row* identifier, the row exists but the cell did not make it (%1/%2)")
                                .arg(id_row_name)
                                .arg(identifier_key),
                        false
                    );
                    // XXX redirect user to an error page instead?
                    //     if they try again it will fail again until the
                    //     database gets fixed properly...
                    reason = "the system failed creating a new user identifier";
                    return status_t::STATUS_UNDEFINED;
                }
                identifier = current_identifier.int64Value();
            }

            // Create a new user entry
            //
            ++identifier; // next identifier
            user_info.define_user( identifier, email );

            new_user = true;

            // save the new identifier back in the database
            //
            new_identifier.setInt64Value(identifier);
            users_table->getRow(id_row_name)->getCell(identifier_key)->setValue(new_identifier);
        }
        // the lock automatically goes away here
    }

    // WARNING: if this breaks, someone probably changed the value
    //          content; it should be the user email
    //
    int64_t const created_date(f_snap->get_start_date());
    if(new_user)
    {
        save_password( user_info, password, "users" );

        // Save the user IP address when registering
        //
        value.setStringValue(f_snap->snapenv(snap::get_name(snap::name_t::SNAP_NAME_CORE_REMOTE_ADDR)));
        user_info.set_value( name_t::SNAP_NAME_USERS_ORIGINAL_IP, value );

        // Date when the user was created (i.e. now)
        //
        // If that field does not exist yet (it could if the user unsubscribe
        // from a mailing list or something similar)
        //
        if(!user_info.value_exists(get_name(name_t::SNAP_NAME_USERS_CREATED_TIME)))
        {
            user_info.set_value( name_t::SNAP_NAME_USERS_CREATED_TIME, created_date );
        }
    }

    // If the email was an example email, then mark the account as an
    // example account (it can still be used to mark pages authored by
    // this user, etc.)
    //
    if(security.get_example())
    {
        signed char c(1);
        user_info.set_value( name_t::SNAP_NAME_USERS_EXAMPLE, c );
    }

    // Add a reference back to the website were the user is being added so
    // that way we can generate a list of such websites in the user's account
    // the reference appears in the cell name and the value is the time when
    // the user registered for that website
    //
    QString const site_key(f_snap->get_site_key_with_slash());
    QString const website_reference(QString("%1::%2")
            .arg(get_name(name_t::SNAP_NAME_USERS_WEBSITE_REFERENCE))
            .arg(site_key));
    user_info.set_value( website_reference, created_date );

    // Now create the user in the contents
    // (nothing else should be create at the path until now)
    content::path_info_t user_ipath;
    user_ipath.set_path(QString("%1/%2").arg(user_path).arg(identifier));
    content::content * content_plugin(content::content::instance());
    snap_version::version_number_t const branch_number(content_plugin->get_current_user_branch(user_ipath.get_key(), "", true));
    user_ipath.force_branch(branch_number);
    // default revision when creating a new branch
    user_ipath.force_revision(static_cast<snap_version::basic_version_number_t>(snap_version::SPECIAL_VERSION_FIRST_REVISION));
    user_ipath.force_locale("xx");
    content_plugin->create_content(user_ipath, get_plugin_name(), "user-page");

    // mark when the user was created in the branch
    libdbproxy::table::pointer_t branch_table(content_plugin->get_branch_table());
    libdbproxy::row::pointer_t branch_row(branch_table->getRow(user_ipath.get_branch_key()));
    branch_row->getCell(content::get_name(content::name_t::SNAP_NAME_CONTENT_CREATED))->setValue(created_date);

    // save a default title and body
    libdbproxy::table::pointer_t revision_table(content_plugin->get_revision_table());
    libdbproxy::row::pointer_t revision_row(revision_table->getRow(user_ipath.get_revision_key()));
    revision_row->getCell(content::get_name(content::name_t::SNAP_NAME_CONTENT_CREATED))->setValue(created_date);
    // no title or body by default--other plugins could set those to the
    //                              user name or other information
    QString const empty_string;
    revision_row->getCell(content::get_name(content::name_t::SNAP_NAME_CONTENT_TITLE))->setValue(empty_string);
    revision_row->getCell(content::get_name(content::name_t::SNAP_NAME_CONTENT_BODY))->setValue(empty_string);

    // if already marked as valid, for sure do not mark this user as new!?
    if(status != status_t::STATUS_VALID)
    {
        // The "public" user account (i.e. in the content table) is limited
        // to the identifier at this point
        //
        // however, we also want to include a link defined as the status
        // at first the user is marked as being new
        // the destination URL is defined in the <link> content
        QString const link_name(get_name(name_t::SNAP_NAME_USERS_STATUS));
        bool const source_unique(true);
        // TODO: determine whether "xx" is the correct locale here (we could also
        //       have "" and a default website language...) -- this is the
        //       language of the profile, not the language of the website...
        links::link_info source(link_name, source_unique, user_ipath.get_key(), user_ipath.get_branch(true, "xx"));
        QString const link_to(get_name(name_t::SNAP_NAME_USERS_STATUS));
        bool const destination_unique(false);
        content::path_info_t dpath;
        dpath.set_path(get_name(name_t::SNAP_NAME_USERS_NEW_PATH));
        links::link_info destination(link_to, destination_unique, dpath.get_key(), dpath.get_branch());
        links::links::instance()->create_link(source, destination);
    }

    user_registered(user_ipath, identifier);

    return status;
}


/** \brief Signal that a user is about to get a new account.
 *
 * This signal is called before a new user gets created or when a
 * user gets re-registered.
 *
 * The function is given the user key, original user email, the
 * password, and a secure flag to set to "not permitted" if there
 * is a reason for which that user should be barred from the system.
 *
 * The implementations are expected to check for various things in
 * regard to that user:
 *
 * \li check whether the email address is valid
 * \li check the password against the password policy of the website
 * \li check whether the user was blocked
 * \li check whether the user is a spammer, hacker, impolite user, etc.
 *
 * \note
 * In your implementation, you should quit early if the secure flag
 * is already marked as not secure; something like this:
 *
 * \code
 * void my_plugin::on_check_user_security(user_security_t & security)
 * {
 *     if(!security.get_secure.allowed())
 *     {
 *         return;
 *     }
 *
 *     // add your tests here:
 *     ...
 * }
 * \endcode
 *
 * \param[in,out] security  The variable user security parameters.
 *
 * \return true if this very function thinks that the user is still
 *         considered valid; false if it already knows otherwise
 */
bool users::check_user_security_impl(user_security_t & security)
{
    auto const & email(security.get_email());
    if(!email.isEmpty())
    {
        // make sure that the user email is valid
        // this snap_child function throws if the email is not acceptable
        // (i.e. the validate_email() signal expects the function to only
        // be called with a valid email)
        //
        try
        {
            snap_child::verified_email_t const ve(f_snap->verify_email(email, 1, security.get_allow_example_domain()));
            SNAP_LOG_DEBUG("++++ ve=")(static_cast<int>(ve));
            if(ve == snap_child::verified_email_t::VERIFIED_EMAIL_EXAMPLE)
            {
                // Note: if not EXAMPLE then it is STANDARD because we only
                //       pass one email address and MIXED would require at
                //       least 2 emails...
                //
                security.set_example(true);
            }
        }
        catch(snap_child_exception_invalid_email const & e)
        {
            SNAP_LOG_ERROR("Exception caught! what=")(e.what());
            security.get_secure().not_permitted(
                        QString("\"%1\" does not look like a valid email address. Reason='%2'")
                            .arg(email)
                            .arg(e.what())
                        );
            security.set_status(status_t::STATUS_INVALID_EMAIL);
            return false;
        }

        // a user may be marked as a spammer whenever his IP
        // address was blocked or some other anti-spam measure
        // returns true...
        //
        if(user_is_a_spammer())
        {
            // this is considered a spammer, just tell the user that the email is
            // considered blocked.
            //
            security.get_secure().not_permitted(QString("\"%1\" is blocked.").arg(email));
            security.set_status(status_t::STATUS_BLOCKED);
            return false;
        }
    }

    // let other plugins take over for a while
    //
    return true;
}


/** \brief Final check on the emails.
 *
 * The validation does a final check here. If the statis is still
 * set to STATUS_NOT_FOUND, then the function checks the user
 * status. If not considered valid (i.e. new, password, valid...)
 * then STATUS_SPAMMER is returned.
 *
 * \param[in,out] security  The variable user security parameters.
 */
void users::check_user_security_done(user_security_t & security)
{
    auto const & email(security.get_email());

    // if the user is not yet blocked, do a final test with the user
    // current status
    //
    if(security.get_secure().allowed()
    && !email.isEmpty())
    {
        QString status_key;
        status_t const status(user_status_from_email(email, status_key));
        if(status != status_t::STATUS_NOT_FOUND
        && status != status_t::STATUS_VALID
        && status != status_t::STATUS_NEW
        && status != status_t::STATUS_AUTO
        && status != status_t::STATUS_PASSWORD
        && status != status_t::STATUS_UNKNOWN) // a status from another plugin than the "users" plugin
        {
            // This may be a spammer, hacker, impolite person, etc.
            //
            security.get_secure().not_permitted(QString("\"%1\" is blocked.").arg(email));
            security.set_status(status_t::STATUS_BLOCKED);
            return;
        }
    }
}


/** \fn void users::user_registered(content::path_info_t& ipath, int64_t identifier)
 * \brief Signal telling other plugins that a user just registered.
 *
 * Note that this signal is sent when the user was registered and NOT when
 * the user verified his account. This means the user is not really fully
 * authorized on the system yet.
 *
 * \param[in,out] ipath  The path to the new user's account (/user/\<identifier\>)
 * \param[in] identifier  The user identifier.
 */


/** \brief Get a constant reference to the session information.
 *
 * This function can be used to retrieve a reference to the session
 * information of the current user. Note that could be an anonymous
 * user. It is up to you to determine whether the user is logged in
 * if the intend is to use the session information only of logged in
 * users.
 *
 * \exception snap_logic_exception
 * This exception is raised if the f_info pointer is still null.
 * This means you called this function too early (i.e. in your
 * bootstrap() function and you appear before the users plugin).
 *
 * \return A constant reference to this user session information.
 */
sessions::sessions::session_info const & users::get_session() const
{
    if(f_info)
    {
        return *f_info;
    }

    throw snap_logic_exception("users::get_sessions() called when the session point is still nullptr");
}


/** \brief Save the specified data to the user session.
 *
 * This function is used to attach data to the current user session so it
 * can be retrieved on a later request. Note that the detach_from_session()
 * will also delete the data from the session as it is expected to only be
 * used once. If you need it again, then call the attach_to_session()
 * function again (in the grand scheme of things it should be 100%
 * automatic!)
 *
 * The \p name parameter should be qualified (i.e. "messages::messages").
 *
 * The data to be attached must be in the form of a string. If you are saving
 * a large structure, or set of structures, make sure to use serialization
 * first.
 *
 * \note
 * The data string cannot be an empty string. Cassandra does not like that
 * and on read, an empty string is viewed as "that data is undefined."
 *
 * \param[in] name  The name of the cell that is to be used to save the data.
 * \param[in] data  The data to save in the session.
 *
 * \sa detach_from_session()
 */
void users::attach_to_session(QString const & name, QString const & data)
{
    sessions::sessions::instance()->attach_to_session(*f_info, name, data);
}


/** \brief Retrieve the specified data from the user session.
 *
 * This function is used to retrieve data that was previously attached
 * to the user session with a call to the attach_to_session() function.
 *
 * Note that the data retreived in this way is deleted from the session
 * since we do not want to offer this data more than once (although in
 * some cases it may be necessary to do so, then the attach_to_session()
 * should be called again.)
 *
 * \note
 * The function is NOT a constant since it modifies the database by
 * deleting the data being detached.
 *
 * \param[in] name  The name of the cell that is to be used to save the data.
 *
 * \return The data read from the session if any, otherwise an empty string.
 *
 * \sa attach_to_session()
 */
QString users::detach_from_session(QString const & name)
{
    return sessions::sessions::instance()->detach_from_session(*f_info, name);
}


/** \brief Retrieve data that was attached to the user session.
 *
 * This function can be used to read a session entry from the user session
 * without having to detach that information from the session. This is
 * useful in cases where data is expected to stay in the session for
 * long period of time (i.e. the cart of a user).
 *
 * If no data was attached to that named session field, then the function
 * returns an empty string. Remember that saving an empty string as session
 * data is not possible.
 *
 * \param[in] name  The name of the parameter to retrieve.
 *
 * \return The data attached to the named session field.
 */
QString users::get_from_session(QString const & name) const
{
    return sessions::sessions::instance()->get_from_session(*f_info, name);
}


/** \brief Detach the user referrer.
 *
 * This function detaches the login referrer for the specified user.
 *
 * On an auto-logout, a referrer has to be specific to a user because
 * otherwise we could end up redirecting a different user to a page
 * which could be "secret" (it is likely in the browsers cache anyway,
 * although we want to fix that too at some point.)
 *
 * \param[in] user_info  The concerned user.
 *
 * \return If user_info represents a user and it has a referrer, that
 *         referrer, which is also getting removed from the session.
 *         Otherwise an empty string.
 */
QString users::detach_referrer(user_info_t const & user_info)
{
    // always detach the default referrer because we do not want
    // to keep two redirects around
    //
    QString const default_referrer(detach_from_session(get_name(name_t::SNAP_NAME_USERS_LOGIN_REFERRER)));

    // if we have a user, check the specific referrer first
    //
    if(user_info.is_user())
    {
        QString const specific_referrer(detach_from_session(referrer_identifier(user_info)));
        if(!specific_referrer.isEmpty())
        {
            return specific_referrer;
        }
    }

    // use the non-specific referrer if there is one...
    //
    return default_referrer;
}


/** \brief Set the referrer path for the current session.
 *
 * Call this function instead of trying to directly save the login referrer.
 *
 * This function verifies that the path is not:
 *
 * \li empty -- there is no point in saving such a path and prefer to keep
 *              the existing path
 * \li inexistant -- there is no page with that name
 * \li AJAX requests -- the path represents an AJAX request
 * \li not-main-page -- a page marked as "not-main-page" cannot be redirected
 *                      to with this function
 * \li already defined -- if the referrer is already defined, do not overwrite
 * \li "forgot-password" -- page is not accessible once logged in
 * \li "login" -- the page you are trying to save as a referrer is the login
 *                page to which one cannot go once logged in
 * \li "logout" -- the page you are trying to save as a referrer is the logout
 *                 page, which if we were to send a newly logged in user he
 *                 would be unlogged immediately
 * \li "new-password" -- page is not accessible once logged in
 * \li "register" -- page is not accessible once logged in
 * \li "verify" -- page is not accessible once logged in
 * \li "verify-credentials" -- page is not accessible once logged in
 * \li "verify/resend" -- page is not accessible once logged in
 *
 * So a line of code like this is going to be completely wrong:
 *
 * \code
 *      attach_to_session( name_t::SNAP_NAME_USERS_LOGIN_REFERRER, path );
 * \endcode
 *
 * The detach_referrer() function is used to retrieve a path saved by this
 * function. If a referrer was saved, then it gets used at the time the
 * user logs in.
 *
 * \note
 * This function ensures that the path gets canonicalized before it
 * gets used.
 *
 * \todo
 * Maybe we could add a callback that would determine whether that path
 * is allowed to be a referrer or not. That way other plugins could
 * prevent certain paths using a technique other than the "not-main-page"
 * link which may not always be possible (i.e. dynamic pages).
 *
 * \bug
 * The function may not currently support saving a dynamic path to
 * the session because such do not exist in the database...
 *
 * \param[in] path  The path to the page being viewed as the referrer.
 * \param[in] user_info  If the user is logged in at the time, the user info
 *                       or nullptr.
 *
 * \sa detach_referrer()
 * \sa attach_to_session()
 * \sa detach_from_session()
 */
void users::set_referrer( QString path, user_info_t const & user_info )
{
    // this is acceptable and it happens
    //
    // (note that if you want to go to the home page, you may want
    // to use f_snap->get_site_key_with_slash() instead of "" or "/")
    //
    if(path.isEmpty())
    {
        return;
    }

    // canonicalize the path
    //
    content::path_info_t ipath;
    ipath.set_path(path);
    path = ipath.get_key();  // make sure it is canonicalized

    // verify that the path is not one of the user login related paths
    // because once logged in, the user does not have permission to go
    // to those pages
    //
    // The /logout page is particularly funny in this situation: it would
    // log the user out right after he logged in and it would get readded
    // and thus create a loop where the user cannot really log in for any
    // amount of time.
    //
    QString const cpath(ipath.get_cpath());
    if(cpath == "forgot-password"
    || cpath == "login"
    || cpath == "logout"
    || cpath == "new-password"
    || cpath == "register"
    || cpath == "verify"
    || cpath == "verify-credentials"  // this form could have a redirect, but there is probably no real reason to do it...
    || cpath == "verify/resend")
    {
        return;
    }

    // if there is already a referrer, do not overwrite it
    //
    // Note: if both types of referrers are defined, we won't catch the
    //       second one here, the detach function will properly handle
    //       the deleting, but we cannot avoid defining the general
    //       referrer (without a user ID) here if we do not have a
    //       such an ID in order to determine whether a user specific
    //       referrer exists...
    //
    QString login_referrer(referrer_identifier(user_info));
    if( !get_from_session(login_referrer).isEmpty() )
    {
        return;
    }

    // make sure it is a valid page
    //
    libdbproxy::table::pointer_t content_table(content::content::instance()->get_content_table());
    if(!content_table->exists(ipath.get_key())
    && ipath.get_real_key().isEmpty())
    {
        // TODO: dynamic pages are expected to end up as a "real key" entry
        //       we will need to do more tests to make sure this works as
        //       expected, although this code should work already
        //
        SNAP_LOG_ERROR("path \"")(path)("\" was not found in the database?!");
        return;
    }

    // check whether this is our current page
    //
    content::path_info_t main_ipath;
    main_ipath.set_path(f_snap->get_uri().path());
    if(path == main_ipath.get_key())
    {
        // this is the main page, verify it is not an AJAX path
        // because redirects to those fail big time
        // (we really need a much stronger way of testing such!)
        //
        // TBD:  the fact that the request is AJAX does not 100%
        //       of the time mean that it could not be a valid
        //       referrer, but close enough at this point
        //
        if(server_access::server_access::instance()->is_ajax_request())
        {
            return;
        }
    }

    // if the page is linked to the "not-main-page" type, then it cannot
    // be a referrer so we drop it right here (this is used by pages such
    // as boxes and other pages that are not expected to become main pages)
    // note that this does not prevent one from going to the page, only
    // the system will not redirect one to such a page
    //
    QString const link_name(get_name(name_t::SNAP_NAME_USERS_NOT_MAIN_PAGE));
    links::link_info not_main_page_info(link_name, true, path, ipath.get_branch());
    QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(not_main_page_info));
    links::link_info has_link_info;
    if(link_ctxt->next_link(has_link_info))
    {
        return;
    }

    SNAP_LOG_DEBUG("referrer \"")
                  (login_referrer)
                  ("\" being set to \"")
                  (path)
                  ("\" for page \"")
                  (f_info->get_page_path())
                  ("\" with user path \"")
                  (f_info->get_object_path())
                  ("\"");

    // everything okay!
    //
    attach_to_session( login_referrer, path );
}


/** \brief Save the user session identifier on password change.
 *
 * To avoid loggin people before they are done changing their password,
 * so that way they cannot go visit all the private pages on the website,
 * we use a session variable to save the information about the user who
 * is changing his password.
 */
void users::on_attach_to_session()
{
    // if this access was not marked as a tranparent hit, then we want to
    // update the session time limit
    //
    if(f_hit != get_name(name_t::SNAP_NAME_USERS_HIT_TRANSPARENT))
    {
        // is the session over?  if so, do not extend it
        //
        // (we should not have to do that, but in case someone else
        // "tempered" with the time limit, we want to re-check here)
        //
        time_t const start_time(f_snap->get_start_time());
        if(start_time <= f_info->get_time_limit())
        {
            // extend the user session, it is always a soft session
            //
            int64_t const user_session_duration(get_user_session_duration());
            f_info->set_time_limit(start_time + user_session_duration);
            if(get_soft_administrative_session())
            {
                // website administrator asked that the administrative session
                // be extended each time the administrator accesses the site
                //
                int64_t const administrative_session_duration(get_administrative_session_duration());
                f_info->set_administrative_login_limit(start_time + administrative_session_duration);
            }

            // save the new date(s)
            //
            sessions::sessions::instance()->save_session(*f_info, false);
        }
    }

    // the messages handling is here because the messages plugin cannot have
    // a dependency on the users plugin
    messages::messages * messages_plugin(messages::messages::instance());
    if(messages_plugin->get_message_count() > 0)
    {
        // note that if we lose those "website" messages,
        // they will still be in our logs
        //
        QString const data(messages_plugin->serialize());
        attach_to_session(messages::get_name(messages::name_t::SNAP_NAME_MESSAGES_MESSAGES), data);
        messages_plugin->clear_messages();
    }
    else if(f_has_user_messages)
    {
        // we had messages when on_detach_from_session() was called,
        // so we have to drop them now
        //
        NOTUSED(detach_from_session(messages::get_name(messages::name_t::SNAP_NAME_MESSAGES_MESSAGES)));
    }
}


/** \brief Retrieve data that was attached to a session.
 *
 * This function is the opposite of the on_attach_to_session(). It is
 * called before the execute() to reinitialize objects that previously
 * saved data in the user session.
 */
void users::on_detach_from_session()
{
    // the message handling is here because the messages plugin cannot have
    // a dependency on the users plugin which is the one handling the session
    //
    QString const data(get_from_session(messages::get_name(messages::name_t::SNAP_NAME_MESSAGES_MESSAGES)));
    f_has_user_messages = !data.isEmpty();
    if(f_has_user_messages)
    {
        messages::messages::instance()->unserialize(data);
    }
}


/** \brief Get the user selected language if user did that.
 *
 * The user can select the language in which he will see most of the
 * website (assuming most was translated in those languages.)
 *
 * \param[in,out] locales  Locales as defined by the user.
 */
void users::on_define_locales(http_strings::WeightedHttpString & locales)
{
    // if we know the user and it still exists in our database, then check
    // whether he has a locales defined, if so use it.
    //
    if(f_user_info.is_user()
    && f_user_info.exists())
    {
        libdbproxy::value const value(f_user_info.get_value(name_t::SNAP_NAME_USERS_LOCALES));
        if(!value.nullValue())
        {
            locales.parse(value.stringValue());
        }
    }
}


/** \brief Create a default password.
 *
 * In some cases an administrator may want to create an account for a user
 * which should then have a valid, albeit unknown, password.
 *
 * This function can be used to create that password.
 *
 * It is strongly advised to NOT send such passwords to the user via email
 * because they may contain "strange" characters and emails are notoriously
 * not safe.
 *
 * \todo
 * Look into defining a set of character in each language instead of
 * just basic ASCII.
 *
 * \return The string with the new password.
 */
QString users::create_password()
{
    // a "large" set of random bytes
    const int PASSWORD_SIZE = 256;
    unsigned char buf[PASSWORD_SIZE];

    QString result;
    do
    {
        // get the random bytes
        RAND_bytes(buf, sizeof(buf));

        for(int i(0); i < PASSWORD_SIZE; ++i)
        {
            // only use ASCII characters
            if(buf[i] >= ' ' && buf[i] < 0x7F)
            {
                result += buf[i];
            }
        }
    }
    while(result.length() < 64); // just in case, make sure it is long enough

    return result;
}


/** \brief Create a new salt for a password.
 *
 * Every time you get to encrypt a new password, call this function to
 * get a new salt. This is important to avoid having the same hash for
 * the same password for multiple users.
 *
 * Imagine a user creating 3 accounts and each time using the exact same
 * password. Just using an md5sum it would encrypt that password to
 * exactly the same 16 bytes. In other words, if you crack one, you
 * crack all 3 (assuming you have access to the database you can
 * immediately see that all those accounts have the exact same password.)
 *
 * The salt prevents such problems. Plus we add 256 bits of completely
 * random entropy to the digest used to encrypt the passwords. This
 * in itself makes it for a much harder to decrypt hash.
 *
 * The salt is expected to be saved in the database along the password.
 *
 * \param[out] salt  The byte array receiving the new salt.
 */
void users::create_password_salt(QByteArray & salt)
{
    // we use 16 bytes before and 16 bytes after the password
    // so create a salt of SALT_SIZE bytes (256 bits at time of writing)
    //
    unsigned char buf[SALT_SIZE];
    int const r(RAND_bytes(buf, sizeof(buf)));
    if(r != 1)
    {
        // something happened, RAND_bytes() failed!
        char err[256];
        ERR_error_string_n(ERR_peek_last_error(), err, sizeof(err));
        throw users_exception_size_mismatch(
            QString("RAND_bytes() error, it could not properly fill the salt buffer (%1: %2)")
                    .arg(ERR_peek_last_error())
                    .arg(err));
    }
    salt.clear();
    salt.append(reinterpret_cast<char *>(buf), sizeof(buf));
}


/** \brief Encrypt a password.
 *
 * This function generates a strong hash of a user password to prevent
 * easy brute force "decryption" of the password. (i.e. an MD5 can be
 * decrypted in 6 hours, and a SHA1 password, in about 1 day, with a
 * $100 GPU as of 2012.)
 *
 * Here we use 2 random salts (using RAND_bytes() which is expected to
 * be random enough for encryption like algorithms) and the specified
 * digest to encrypt (okay, hash--a one way "encryption") the password.
 *
 * Read more about hash functions on
 * http://ehash.iaik.tugraz.at/wiki/The_Hash_Function_Zoo
 *
 * \exception users_exception_size_mismatch
 * This exception is raised if the salt byte array is not exactly SALT_SIZE
 * bytes. For new passwords, you want to call the create_password_salt()
 * function to create the salt buffer.
 *
 * \exception users_exception_digest_not_available
 * This exception is raised if any of the OpenSSL digest functions fail.
 * This include an invalid digest name and adding/retrieving data to/from
 * the digest.
 *
 * \param[in] digest  The name of the digest to use (i.e. "sha512").
 * \param[in] password  The password to encrypt.
 * \param[in] salt  The salt information, necessary to encrypt passwords.
 * \param[out] hash  The resulting password hash.
 */
void users::encrypt_password(QString const & digest, QString const & password, QByteArray const & salt, QByteArray & hash)
{
    // it is an out only so reset it immediately
    hash.clear();

    // verify the size
    if(salt.size() != SALT_SIZE)
    {
        throw users_exception_size_mismatch("salt buffer must be exactly SALT_SIZE bytes (missed calling create_password_salt()?)");
    }
    unsigned char buf[SALT_SIZE];
    memcpy(buf, salt.data(), SALT_SIZE);

    // Initialize so we gain access to all the necessary digests
    OpenSSL_add_all_digests();

    // retrieve the digest we want to use
    // (TODO: allows website owners to change this value)
    EVP_MD const * md(EVP_get_digestbyname(digest.toUtf8().data()));
    if(md == nullptr)
    {
        throw users_exception_digest_not_available("the specified digest could not be found");
    }

    // initialize the digest context
    EVP_MD_CTX mdctx;
    EVP_MD_CTX_init(&mdctx);
    if(EVP_DigestInit_ex(&mdctx, md, nullptr) != 1)
    {
        throw users_exception_encryption_failed("EVP_DigestInit_ex() failed digest initialization");
    }

    // add first salt
    if(EVP_DigestUpdate(&mdctx, buf, SALT_SIZE / 2) != 1)
    {
        throw users_exception_encryption_failed("EVP_DigestUpdate() failed digest update (salt1)");
    }

    // add password (encrypt to UTF-8)
    const char *pwd(password.toUtf8().data());
    if(EVP_DigestUpdate(&mdctx, pwd, strlen(pwd)) != 1)
    {
        throw users_exception_encryption_failed("EVP_DigestUpdate() failed digest update (password)");
    }

    // add second salt
    if(EVP_DigestUpdate(&mdctx, buf + SALT_SIZE / 2, SALT_SIZE / 2) != 1)
    {
        throw users_exception_encryption_failed("EVP_DigestUpdate() failed digest update (salt2)");
    }

    // retrieve the result of the hash
    unsigned char md_value[EVP_MAX_MD_SIZE];
    unsigned int md_len(EVP_MAX_MD_SIZE);
    if(EVP_DigestFinal_ex(&mdctx, md_value, &md_len) != 1)
    {
        throw users_exception_encryption_failed("EVP_DigestFinal_ex() digest finalization failed");
    }
    hash.append(reinterpret_cast<char *>(md_value), md_len);

    // clean up the context
    // (note: the return value is not documented so we ignore it)
    EVP_MD_CTX_cleanup(&mdctx);
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
void users::on_replace_token(content::path_info_t & ipath, QDomDocument & xml, filter::filter::token_info_t & token)
{
    NOTUSED(ipath);
    NOTUSED(xml);

    if(token.f_name.length() <= 7
    || !token.is_namespace("users::"))
    {
        // not a users plugin token
        return;
    }

    switch(token.f_name[7].unicode())
    {
    case 'c':
        if(token.is_token("users::count"))
        {
            token_user_count(token);
            return;
        }
        break;

    }

    if(!f_user_info.is_user())
    {
        // unknown used (it may be the anonymous used too)
        //
        return;
    }

    if( !f_user_info.exists() )
    {
        // cannot find user?!?
        //
        return;
    }

    auto const & user_key(f_user_info.get_user_key());
    switch(token.f_name[7].unicode())
    {
    case 'e':
        if(token.is_token("users::email"))
        {
            token.f_replacement = user_key;
            return;
        }
        if(token.is_token("users::email_anchor"))
        {
            // TODO: replace user_key with the user first/last names in the
            //       anchor text when available AND authorized
            //
            // TODO: replace with id?
            //
            token.f_replacement = "<a href=\"mailto:" + user_key + "\">" + user_key + "</a>";
            return;
        }
        break;

    }

    // anything else requires the user to be verified
    libdbproxy::value const verified_on(f_user_info.get_value(name_t::SNAP_NAME_USERS_LOCALES));
    if(verified_on.nullValue())
    {
        // not verified yet
        return;
    }

    switch(token.f_name[7].unicode())
    {
    case 's':
        if(token.is_token("users::since"))
        {
            // TODO: add support for a user defined date format
            libdbproxy::value const value(f_user_info.get_value( name_t::SNAP_NAME_USERS_CREATED_TIME ));
            int64_t date(value.int64Value());
            token.f_replacement = QString("%1 %2")
                    .arg(f_snap->date_to_string(date, snap_child::date_format_t::DATE_FORMAT_SHORT))
                    .arg(f_snap->date_to_string(date, snap_child::date_format_t::DATE_FORMAT_TIME));
            // else use was not yet verified
            return;
        }
        break;

    }
}


/** \brief Gather all the tokens and a quick help.
 *
 * This function is used by the info system to present the user with all
 * the available tokens.
 *
 * \param[in,out] help  The existing help data.
 */
void users::on_token_help(filter::filter::token_help_t & help)
{
    help.add_token("users::count",
        "Output the number of registered users, all inclusive (verified"
        " and unverified).");

    help.add_token("users::email",
        "The current user email address.");

    help.add_token("users::email_anchor",
        "The current user email address as an anchor (using mailto: as the protocol).");

    help.add_token("users::since",
        "The date and time the user registered his account.");
}


/** \brief Replace the token with the number of registered users.
 *
 * This function replaces the [users::count] token with the number of
 * registered users.
 *
 * \todo
 * Implement a list of verified users instead of all registrations.
 *
 * \param[in,out] token  The token where the replacement takes place.
 */
void users::token_user_count(filter::filter::token_info_t & token)
{
    content::content * content_plugin(content::content::instance());
    libdbproxy::table::pointer_t branch_table(content_plugin->get_branch_table());

    content::path_info_t user_count_ipath;
    user_count_ipath.set_path(get_name(name_t::SNAP_NAME_USERS_PATH));
    int32_t const count(branch_table->getRow(user_count_ipath.get_branch_key())->getCell(list::get_name(list::name_t::SNAP_NAME_LIST_NUMBER_OF_ITEMS))->getValue().safeInt32Value());
    token.f_replacement = QString("%1").arg(count);
}


/** \brief Determine whether the current user is considered to be a spammer.
 *
 * This function checks the user IP address and if black listed, then we
 * return true meaning that we consider that user as a spammer. This limits
 * access to the bare minimum which generally are:
 *
 * \li The home page
 * \li The privacy policy
 * \li The terms and conditions
 * \li The files referenced by those items (CSS, JavaScript, images, etc.)
 *
 * \todo
 * We probably want to extend this test with a signal so other plugins
 * have a chance to define a user as a spammer.
 *
 * \return true if the user is considered to be a spammer.
 */
bool users::user_is_a_spammer()
{
    libdbproxy::table::pointer_t users_table(get_users_table());
    QString const black_list(get_name(name_t::SNAP_NAME_USERS_BLACK_LIST));
    if(users_table->exists(black_list))
    {
        // the row exists, check the IP
        //
        // TODO: canonicalize the IP address so it matches every time
        //       (i.e. IPv4 and IPv6 have several ways of being written)
        //       see for example: tracker::on_detach_from_session()
        //       The best will certainly be to have a function such as:
        //
        //           f_snap->get_canonicalized_remote_ip()
        //
        QString const ip(f_snap->snapenv(snap::get_name(snap::name_t::SNAP_NAME_CORE_REMOTE_ADDR)));
        libdbproxy::row::pointer_t row(users_table->getRow(black_list));
        if(row->exists(ip))
        {
            // "unfortunately" this user is marked as a spammer
            return true;
        }
    }
    return false;
}


/** \brief Whether the user was logged in recently.
 *
 * This function MUST be called to know whether the user is a logged in
 * user who has read and write access to the website, or just a
 * registered user with a valid session.
 *
 * Make sure to call the user_has_administrative_rights() if he needs
 * administrative rights.
 *
 * The details about the various possible logged in user states are
 * defined in authenticated_user(().
 *
 * \return true if the user was logged in recently enough that he still
 *         has read/write access to the website. However, he may not have
 *         have administrative rights anymore.
 *
 * \sa user_has_administrative_rights()
 */
bool users::user_is_logged_in() const
{
    return f_user_logged_in;
}


/** \brief Whether the user was logged in recently.
 *
 * This function MUST be called to know whether the user is a logged in
 * user who logged in very recently, sufficiently recently so as to
 * be given access to the most advanced administrative tasks.
 *
 * The details about the various possible logged in user states are
 * defined in authenticated_user().
 *
 * \return true if the user was logged in very recently (by default,
 *         this means within the last 3h).
 *
 * \sa user_is_logged_in()
 */
bool users::user_has_administrative_rights() const
{
    return f_administrative_logged_in;
}


/** \brief Determines when the session was created.
 *
 * This function returns true if the session is considered "pretty old"
 * which by default means about 12h old. Such a user is considered a
 * returning user and thus may be given slightly different permissions.
 *
 * \return true if the user's session is considered old.
 */
bool users::user_session_is_old() const
{
    // user came back at least 1 day ago, then session is considered "old"
    return (f_snap->get_start_date() - f_info->get_creation_date()) > 86400LL * 1000000LL;
}


/** \brief Improves the error signature.
 *
 * This function adds the user profile link to the brief signature of die()
 * errors. This is done only if the user is logged in.
 *
 * \param[in] path  The path to the page that generated the error.
 * \param[in] doc  The DOM document.
 * \param[in,out] signature_tag  The DOM element where signature anchors are added.
 */
void users::on_improve_signature(QString const & path, QDomDocument doc, QDomElement signature_tag)
{
    NOTUSED(path);

    if(f_user_info.is_user())
    {
        // add a space between the previous link and this one
        snap_dom::append_plain_text_to_node(signature_tag, " ");

        // add a link to the user account
        QDomElement a_tag(doc.createElement("a"));
        a_tag.setAttribute("class", "user-account");
        a_tag.setAttribute("target", "_top");
        a_tag.setAttribute("href", f_user_info.get_user_path(true));
        // TODO: translate
        snap_dom::append_plain_text_to_node(a_tag, "My Account");

        signature_tag.appendChild(a_tag);
    }
}


/** \brief Signal called when a plugin requests the locale to be set.
 *
 * This signal is called whenever a plugin requests that the locale be
 * set before using a function that is affected by locale parameters.
 *
 * This very function setups the locale to the user locale if the
 * user is logged in.
 *
 * If the function is called before the user is logged in, then nothing
 * happens. The users plugin makes sure to reset the locale information
 * once the user gets logged in.
 */
void users::on_set_locale()
{
    if( !f_user_info.is_user() )
    {
        return;
    }

    // we may have a user defined locale
    QString const user_path(f_user_info.get_user_path(false));
    if(user_path != get_name(name_t::SNAP_NAME_USERS_ANONYMOUS_PATH))
    {
        content::content * content_plugin(content::content::instance());
        libdbproxy::table::pointer_t revision_table(content_plugin->get_revision_table());

        content::path_info_t user_ipath;
        user_ipath.set_path(user_path);

        libdbproxy::row::pointer_t revision_row(revision_table->getRow(user_ipath.get_revision_key()));
        QString const user_locale(revision_row->getCell(get_name(name_t::SNAP_NAME_USERS_LOCALE))->getValue().stringValue());
        if(!user_locale.isEmpty())
        {
            locale::locale::instance()->set_current_locale(user_locale);
        }
    }
}


/** \brief Signal called when a plugin requests the timezone to be set.
 *
 * This signal is called whenever a plugin requests that the timezone be
 * set before using a function that is affected by the timezone parameter.
 *
 * This very function setups the timezone to the user timezone if the
 * user is logged in.
 *
 * If the function is called before the user is logged in, then nothing
 * happens. The users plugin makes sure to reset the timezone information
 * once the user gets logged in.
 */
void users::on_set_timezone()
{
    if( !f_user_info.is_user() )
    {
        return;
    }

    // we may have a user defined timezone
    //
    QString const user_path(f_user_info.get_user_path(false));
    if(user_path != get_name(name_t::SNAP_NAME_USERS_ANONYMOUS_PATH))
    {
        content::content * content_plugin(content::content::instance());
        libdbproxy::table::pointer_t revision_table(content_plugin->get_revision_table());

        content::path_info_t user_ipath;
        user_ipath.set_path(user_path);

        libdbproxy::row::pointer_t revision_row(revision_table->getRow(user_ipath.get_revision_key()));
        QString const user_timezone(revision_row->getCell(get_name(name_t::SNAP_NAME_USERS_TIMEZONE))->getValue().stringValue());
        if(!user_timezone.isEmpty())
        {
            locale::locale::instance()->set_current_timezone(user_timezone);
        }
    }
}


/** \brief Repair the author link.
 *
 * When cloning a page, we repair the author link and then add
 * a "cloned by" link to the current user.
 *
 * The "cloned by" link does NOT ever get "repaired".
 */
void users::repair_link_of_cloned_page(QString const& clone, snap_version::version_number_t branch_number, links::link_info const& source, links::link_info const& destination, bool const cloning)
{
    NOTUSED(cloning);

    if(source.name() == get_name(name_t::SNAP_NAME_USERS_AUTHOR)
    && destination.name() == get_name(name_t::SNAP_NAME_USERS_AUTHORED_PAGES))
    {
        links::link_info src(get_name(name_t::SNAP_NAME_USERS_AUTHOR), true, clone, branch_number);
        links::links::instance()->create_link(src, destination);
    }
    // else ...
    // users also have a status, but no one should allow a user to be cloned
    // and thus the status does not need to be handled here (what would we
    // do really with it here? mark the user as blocked?)
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
void users::on_table_is_accessible(QString const & table_name, server::accessible_flag_t & accessible)
{
    if(table_name == get_name(name_t::SNAP_NAME_USERS_TABLE))
    {
        // the users table includes the user passwords, albeit
        // encrypted, we just do not ever want to share any of
        // that
        //
        accessible.mark_as_secure();
    }
}


/** \brief Save a new password for the specified user.
 *
 * This function accepts a \p row which points to a user's account in
 * the users table and a new \p user_password to save in that user's
 * account.
 *
 * The password can be set to "!" when no password is given to a certain
 * account. No one can log in such accounts.
 *
 * \param[in] user_info  The user_info_t object containing the row to the user's
 *                       account in the users table (key is the user's email address).
 * \param[in] user_password  The new password to save in that user's account.
 * \param[in] password_policy  The policy used to handle this password.
 */
void users::save_password_done(user_info_t & user_info, QString const & user_password, QString const & password_policy)
{
    NOTUSED(password_policy);

    QByteArray salt;
    QByteArray hash;
    libdbproxy::value digest(f_snap->get_site_parameter(get_name(name_t::SNAP_NAME_USERS_PASSWORD_DIGEST)));
    if(user_password == "!")
    {
        // special case; these users cannot log in
        // (probably created because they signed up to a newsletter or comments)
        //
        digest.setStringValue("no password");
        salt = "no salt";
        hash = "!";
    }
    else
    {
        if(digest.nullValue())
        {
            digest.setStringValue("sha512");
        }
        create_password_salt(salt);
        encrypt_password(digest.stringValue(), user_password, salt, hash);
    }

    int64_t const start_date(f_snap->get_start_date());

    libdbproxy::value value;

    // save the hashed password (never the original password!)
    //
    value.setBinaryValue(hash);
    user_info.set_value( name_t::SNAP_NAME_USERS_PASSWORD, value );

    // to be able to time out a password, we have to save when it was
    // last modified and this is where we do so
    //
    user_info.set_value( name_t::SNAP_NAME_USERS_PASSWORD_MODIFIED, start_date );

    // save the password salt (otherwise we could not check whether the user
    // knows his password!)
    //
    value.setBinaryValue(salt);
    user_info.set_value( name_t::SNAP_NAME_USERS_PASSWORD_SALT, value );

    // also save the digest since it could change en-route
    //
    user_info.set_value( name_t::SNAP_NAME_USERS_PASSWORD_DIGEST, digest );

    // the user was just modified
    //
    user_info.set_value( name_t::SNAP_NAME_USERS_MODIFIED, start_date );
}



SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
