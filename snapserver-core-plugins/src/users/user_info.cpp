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


SNAP_PLUGIN_EXTENSION_START(users)


users::user_info_t::user_info_t()
{
}


/** \brief Initialize a user_info_t structure from the specified string.
 *
 * This function expects the string to either be a user path (.../user/<id>)
 * or an email address. If neither, then the resulting user_info_t object
 * will be consided invalid (is_valid() will return false.)
 *
 * The user path may include the full domain name or just start with "user/".
 *
 * Note that the special path "user" represents the anonymous user and
 * as such it is considered valid and can be used to initialize the
 * user_info_t object as representing the anonymous user.
 *
 * \param[in] email_or_path  String representing either a user path or an email.
 */
users::user_info_t::user_info_t( QString const & email_or_path )
{
    // val == "user/<id>" ?
    //
    f_identifier = get_user_id_by_path( email_or_path );
    if( !is_valid() )
    {
        // val == "user@domain.tld" ?
        //
        f_user_email = email_or_path;
        get_user_id_by_email();

        // is_valid() may still be false here, but we do not want to throw
    }
    else if( !is_anonymous() )
    {
        f_user_email = get_value(name_t::SNAP_NAME_USERS_CURRENT_EMAIL).stringValue();
        if(f_user_email.isEmpty())
        {
            // no corresponding email, that is not a valid user.
            //
            reset();
        }
    }
}


/** \brief Initialize a user_info_t structure from a user identifier.
 *
 * This function initializes a user_into_t structure from a user identifier.
 * The identifier may be set to IDENTIFIER_INVALID or IDENTIFIER_ANONYMOUS.
 *
 * \param[in] id  The identifier of the user to be assigned to this user_info_t.
 */
users::user_info_t::user_info_t( identifier_t const id )
    : f_identifier(id)
{
    if(is_user())
    {
        // make sure there is a corresponding email address
        //
        f_user_email = get_value( name_t::SNAP_NAME_USERS_CURRENT_EMAIL ).stringValue();
        if(f_user_email.isEmpty())
        {
            // no corresponding email, that is not a valid user.
            //
            reset();
        }
    }
}


/** \brief Retrieve the user identifier from his path.
 *
 * This function extract the user identifier from a path that looks like:
 *
 * \code
 *      .../user/<id>
 * \endcode
 *
 * The path may or not include the protocol and domain name.
 *
 * The function recognizes the anonymous path and returns the
 * corresponding identifier number when it does get detected
 * (i.e. IDENTIFIER_ANONYMOUS.)
 *
 * If the path does not represent a user path, then the function
 * returns IDENTIFIER_INVALID.
 *
 * \param[in] user_path  The path to parse for an identifier.
 *
 * \return The user identifier, IDENTIFIER_ANONYMOUS, or IDENTIFIER_INVALID.
 */
users::identifier_t users::user_info_t::get_user_id_by_path( QString const & user_path )
{
    content::path_info_t ipath;
    ipath.set_path(user_path);
    QString const cpath(ipath.get_cpath());

    // standard user/<id> path?
    //
    QString const site_users_path(QString("%1/").arg(get_name(name_t::SNAP_NAME_USERS_PATH)));
    if(cpath.startsWith(site_users_path))
    {
        // looks like a user/<id> path, check the <id> part
        //
        QString const identifier_string(cpath.mid(site_users_path.length()));
        bool ok(false);
        int64_t const identifier(identifier_string.toLongLong(&ok, 10));
        if(ok)
        {
            return identifier;
        }
    }
    else if(user_path == get_name(name_t::SNAP_NAME_USERS_ANONYMOUS_PATH))
    {
        // this path is specifically the anonymous user path
        //
        return IDENTIFIER_ANONYMOUS;
    }

    return IDENTIFIER_INVALID;
}


/** \brief Retrieve the slashed anonymous user path.
 *
 * The anonymous path is just "user". This function returns a slashed
 * anonymous path as in: "/user/". We use it in many places so it was
 * practical to have such a function. Note that the function is marked
 * as "static const" so it should be compiled out.
 *
 * \return The user anonymous path.
 */
QString users::user_info_t::get_full_anonymous_path()
{
    return QString("/%1/").arg(get_name(name_t::SNAP_NAME_USERS_ANONYMOUS_PATH));
}


/** \brief Replace a user email address with another.
 *
 * This function replaces the current email address with the newly
 * specified \p new_user_email address.
 *
 * The function makes sure to update the '*index*' row by adding the
 * new email and removing the old one.
 *
 * Just in case, the function also moves the emails through a history.
 * This ensures that if a user gets his email changed under his feet,
 * we may have a chance to restore it.
 *
 * \param[in] new_user_email  The new email to assign to that user.
 */
void users::user_info_t::change_user_email( QString const & new_user_email )
{
    auto user_row(get_user_row());

    // Rotate the history, adding the current email to the top.
    //
    QString const email_history_list_base( get_name(name_t::SNAP_NAME_USERS_EMAIL_HISTORY_LIST_BASE) );
    QStringList new_history_list;
    new_history_list << get_user_key();
    for( int i = 0; i < MAX_EMAIL_BACKUPS; ++i )
    {
        QString const history_entry_name( QString("%1_%2").arg(email_history_list_base).arg(i) );
        if( user_row->exists(history_entry_name) )
        {
            new_history_list << user_row->cell(history_entry_name)->value().stringValue();
        }
    }

    for( int i = 0; i < new_history_list.size(); ++i )
    {
        QString const history_entry_name( QString("%1_%2").arg(email_history_list_base).arg(i) );
        user_row->cell(history_entry_name)->setValue( new_history_list[i] );
    }

    // Set the new email address into this object.
    //
    auto const old_user_key( get_user_key() );
    f_user_email = new_user_email;
    f_user_key.clear();
    user_row->cell(get_name(name_t::SNAP_NAME_USERS_CURRENT_EMAIL))->setValue(f_user_email);

    // Now change the index to match
    //
    auto index_row( f_users_table->row(get_name(name_t::SNAP_NAME_USERS_INDEX_ROW)) );
    index_row->dropCell( old_user_key );
    QtCassandra::QCassandraValue id_value( f_identifier );
    index_row->cell(get_user_key())->setValue( id_value.binaryValue() );

    SNAP_LOG_TRACE("user_info_t::change_user_email(): old_user_key=")(old_user_key)(", f_user_email=")(f_user_email)(", f_user_key=")(get_user_key());
}


/** \brief Check whether the specified user is marked as being an example.
 *
 * You may call this function to determine whether a user is marked as
 * an example. This happens whenever a user is created with an example
 * email address such as john@example.com.
 *
 * The function expects the email address of the user. It first canonicolize
 * the email and then checks in the database to see whether the user is
 * considered an example or not.
 *
 * We use the database instead of parsing the email so really any user
 * can be marked as an example user.
 *
 * \warning
 * If the user_info_t does not represent a registered user, then the
 * function always returns false.
 *
 * \param[in] email  The email address of the user to check.
 *
 * \return true if the user is marked as being an example.
 */
bool users::user_info_t::user_is_an_example_from_email() const
{
    if( !exists() )
    {
        return false;
    }
    return get_value(name_t::SNAP_NAME_USERS_EXAMPLE).safeSignedCharValue() != 0;
}


/** \brief Get the current user identifer.
 *
 * This function returns the user_info_t user identifier. If we do not
 * have the user key then the function returns IDENTIFIER_INVALID. The
 * function may also return the anonymous user identifier:
 * IDENTIFIER_ANONYMOUS.
 *
 * \warning
 * The identifier returned may NOT be from a logged in user. We may know
 * the user key (his identifier) and yet not have a logged in user. Whether
 * the user is logged in can be checked with one of the
 * user_is_logged_in() or user_has_administrative_rights() functions.
 *
 * \return The identifer of the current user.
 *
 * \sa user_is_logged_in()
 * \sa user_has_administrative_rights()
 */
users::identifier_t users::user_info_t::get_identifier() const
{
    return f_identifier;
}


/** \brief Create a new user with identifier and email address.
 *
 * This function is used to create a new user with his identifier
 * and email address as he typed it.
 *
 * The identifier is a number which was assigned to the user when
 * he creates his account.
 *
 * \warning
 * It is not really possible to create the user from outside of
 * the user_info_t without having to replicate the get_user_key()
 * function among a few other things so this is why have this
 * define_user() function. However, you should not be using this
 * function to register a new user. Instead, look into using the
 * users::register_user() function which does all the necessary
 * work for you.
 *
 * \param[in] identifier  The identifier of the user.
 * \param[in] user_email  The email of the user.
 *
 * \sa get_identifier()
 * \sa get_user_email()
 * \sa get_user_key()
 */
void users::user_info_t::define_user( identifier_t const identifier, QString const & user_email )
{
    // make sure that the user is not trying to change a valid user,
    // which is something that is not possible to do
    //
    if(is_valid())
    {
        throw users_exception_invalid_object("you cannot change the user identifier, set_identifier() can only be used to define the identifier of a new user.");
    }

    f_identifier = identifier;

    if(!is_anonymous())
    {
        f_user_email = user_email;
        f_user_key.clear();

        // set_value() prevents ousiders from changing the identifier,
        // so we have a copy here...
        //
        value_t const id_value(f_identifier);
        get_cell(get_name(name_t::SNAP_NAME_USERS_IDENTIFIER))->setValue(id_value);

        // also save the email address, this is also the original
        //
        set_value( name_t::SNAP_NAME_USERS_ORIGINAL_EMAIL, f_user_email );
        set_value( name_t::SNAP_NAME_USERS_CURRENT_EMAIL , f_user_email );

        // we must save the email address in the index because otherwise
        // the lock used in register_user() would not be useful...
        //
        auto index_row(f_users_table->row(get_name(name_t::SNAP_NAME_USERS_INDEX_ROW)));
        index_row->cell(get_user_key())->setValue(id_value.binaryValue());
    }
}


/** \brief Check whether a certain value exists in the database.
 *
 * This function checks whether a named value exists in the database.
 * You should use it with values other than the user email and
 * identifier since you can test the user email just with:
 *
 * \code
 *      user_info.get_user_email().isEmpty()
 * \endcode
 *
 * And the identifier will be considered valid with one of those
 * calls:
 *
 * \code
 *      user_info.is_valid(); // anonymous or a registered user
 *      user_info.is_user();  // a registered user
 * \endcode
 *
 * Also we offer another function called exists() which one can
 * use to check whether the user identifier is indeed defined
 * in the database. However, any function that accesses the
 * database will be slower than memory only functions.
 *
 * \sa get_user_email()
 * \sa is_valid()
 * \sa is_user()
 * \sa exists()
 */
bool users::user_info_t::value_exists( QString const & v ) const
{
    if(is_user())
    {
        return get_user_row()->exists(v);
    }
    return false;
}


/** \brief Check whether a certain value exists in the database.
 *
 * This function checks whether a named value exists in the database.
 * You should use it with values other than the user email and
 * identifier since you can test the user email just with:
 *
 * \code
 *      user_info.get_user_email().isEmpty()
 * \endcode
 *
 * And the identifier will be considered valid with one of those
 * calls:
 *
 * \code
 *      user_info.is_valid(); // anonymous or a registered user
 *      user_info.is_user();  // a registered user
 * \endcode
 *
 * Also we offer another function called exists() which one can
 * use to check whether the user identifier is indeed defined
 * in the database. However, any function that accesses the
 * database will be slower than memory only functions.
 *
 * \sa get_user_email()
 * \sa is_valid()
 * \sa is_user()
 * \sa exists()
 */
bool users::user_info_t::value_exists( name_t const v ) const
{
    return value_exists(get_name(v));
}


/** \brief Retrieve a reference to the named cell.
 *
 * This function can be used to retrieve a reference to the named cell
 * from this user's row.
 *
 * \param[in] name  The name of the cell to retrieve.
 *
 * \return A cell smart pointer.
 */
users::user_info_t::cell_t users::user_info_t::get_cell( QString const & name ) const
{
    // avoid accessing the database if the identifier is -1 or 0
    // (i.e. invalid or anonymous)
    //
    if(is_user())
    {
        return get_user_row()->cell(name);
    }

    return cell_t();
}


/** \brief Retrieve a reference to the named cell.
 *
 * This function can be used to retrieve a reference to the named cell
 * from this user's row.
 *
 * \param[in] name  The name of the cell to retrieve.
 *
 * \return A cell smart pointer.
 */
users::user_info_t::cell_t users::user_info_t::get_cell( name_t const name ) const
{
    return get_cell(get_name(name));
}


/** \brief Retrieve the value of a user field.
 *
 * This function reads the QtCassandraValue of the named field from the
 * user's row.
 *
 * If the user is not valid or is the anonymous user, an empty value is
 * returned (nullValue() will be true).
 *
 * \param[in] name  The name of the cell to read.
 *
 * \return A copy of the retrieved value.
 */
users::user_info_t::value_t const & users::user_info_t::get_value( QString const & name ) const
{
    static value_t empty;

    // only users have a value in the database
    //
    if(is_user())
    {
        return get_cell(name)->value();
    }

    // since we return a const, the caller "cannot" modify the value
    // therefore it will remain empty
    //
    return empty;
}


/** \brief Retrieve the value of a user field.
 *
 * This function reads the QtCassandraValue of the named field from the
 * user's row.
 *
 * If the user is not valid or is the anonymous user, an empty value is
 * returned (nullValue() will be true).
 *
 * \param[in] name  The name of the cell to read.
 *
 * \return A copy of the retrieved value.
 */
users::user_info_t::value_t const & users::user_info_t::get_value( name_t const name ) const
{
    return get_value(get_name(name));
}


/** \brief Set or replace the value of a cell.
 *
 * This function sets the value of the named cell in the user's row.
 *
 * If the user is not valid, nothing happens.
 *
 * \exception users_exception_invalid_object
 * This exception is raised if the specified name is
 * name_t::SNAP_NAME_USERS_IDENTIFIER because we do not let you change
 * that parameter in the users table. That parameter is read-only and
 * set at the time the user is created with define_user().
 *
 * \param[in] name  The name of the cell to update.
 * \param[in] value  The value to be saved in that cell.
 *
 * \sa define_user()
 */
void users::user_info_t::set_value( QString const & name, value_t const & value )
{
    // prevent caller from changing the identifier, it cannot be changed
    // (only set once on creation of the user--see set_identifier() instead.)
    //
    if(name == get_name(name_t::SNAP_NAME_USERS_IDENTIFIER))
    {
        throw users_exception_invalid_object("you cannot change the user identifier with user_info_t::set_value()");
    }

    if(is_user())
    {
        get_cell(name)->setValue(value);
    }
}


/** \brief Set or replace the value of a cell.
 *
 * This function sets the value of the named cell in the user's row.
 *
 * If the user is not valid, nothing happens.
 *
 * \param[in] name  The name of the cell to update.
 * \param[in] value  The value to be saved in that cell.
 */
void users::user_info_t::set_value( name_t const name, value_t const & value )
{
    set_value( get_name(name), value );
}


/** \brief Drop a cell.
 *
 * This function drops the named cell from the user's row.
 *
 * If the user is not valid, nothing happens.
 *
 * \param[in] name  The name of the cell to drop.
 */
void users::user_info_t::delete_value( QString const & name )
{
    if(is_user())
    {
        get_user_row()->dropCell(name);
    }
}


/** \brief Drop a cell.
 *
 * This function drops the named cell from the user's row.
 *
 * If the user is not valid, nothing happens.
 *
 * \param[in] name  The name of the cell to drop.
 */
void users::user_info_t::delete_value( name_t const name )
{
    delete_value( get_name(name) );
}


/** \brief Canonicalize the user email to use in the "users" table.
 *
 * The "users" table defines each user by email address. The email address
 * is kept as is in the user account itself, but for us to access the
 * database, we have to have a canonicalized user email address.
 *
 * The domain name part (what appears after the AT (@) character) is
 * always made to lowercase. The username is also made lowercase by
 * default. However, a top notch geek website can offer its end
 * users to have lower and upper case usernames in their email
 * address. This is generally fine, although it means you may get
 * entries such as:
 *
 * \code
 *    me@snap.website
 *    Me@snap.website
 *    ME@snap.website
 *    mE@snap.website
 * \endcode
 *
 * and each one will be considered a different account. This can be
 * really frustrating for users who don't understand emails though.
 *
 * The default mode does not require any particular setup.
 * The "Unix" (or geek) mode requires that you set the
 * "users::force_lowercase" field in the sites table to 1.
 * To go back to the default, either set "users::force_lowercase"
 * to 0 or delete it.
 *
 * \note If you change the "users::force_lowercase" setting, you must restart the plugin
 *       due to the static value being preserved.
 *
 * \param[in] email  The email of the user.
 *
 * \return The user_key based on the email all or mostly in lowercase or not.
 *
 * \sa users::basic_email_canonicalization()
 */
QString const & users::user_info_t::get_user_key() const
{
    // This is the only function that defines the f_user_key, you should
    // never directly access that variable, always call the get_user_key()
    // after the first time it will be fast since it gets cached
    //
    if(f_user_key.isEmpty()
    && !f_user_email.isEmpty())
    {
        f_user_key = get_user_key(f_user_email);
    }

    return f_user_key;
}


/** \brief Convert a user email to a user key.
 *
 * This function converts a user email into a usable user key that will
 * be used to access the user email index in the database.
 *
 * \warning
 * This function does not cache the resulting key. It also uses the input
 * parameter and not the f_user_email field of this user_info_t object.
 * This is useful in the users::register_user() function. There should
 * be no reason to use this function anywhere else.
 *
 * \note
 * This function cannot be static because there is a required to obtain a site
 * parameter below, which requires a pointer to the snap_child object. However,
 * is const and aside from having to access the f_snap member, no other data
 * members are accessed.
 *
 * \param[in] user_email  The email to convert.
 *
 * \return The converted email as a user key.
 */
QString users::user_info_t::get_user_key(QString const & user_email) const
{
    if(user_email.isEmpty())
    {
        return QString();
    }

    // It is better to use the new strongly typed enumerations.
    //
    enum class force_lowercase_t
    {
        UNDEFINED,
        YES,
        NO
    };

    static force_lowercase_t force_lowercase(force_lowercase_t::UNDEFINED);

    //if(force_lowercase == force_lowercase_t::UNDEFINED) -- not currently required, it will always be true
    {
        QtCassandra::QCassandraValue const force_lowercase_parameter(get_snap()->get_site_parameter(get_name(name_t::SNAP_NAME_USERS_FORCE_LOWERCASE)));
        if(force_lowercase_parameter.nullValue()
        || force_lowercase_parameter.safeSignedCharValue())
        {
            // this is the default if undefined
            force_lowercase = force_lowercase_t::YES;
        }
        else
        {
            force_lowercase = force_lowercase_t::NO;
        }
    }

    if(force_lowercase == force_lowercase_t::YES)
    {
        // in this case, it is easy we can force the entire email to lowercase
        //
        return user_email.toLower();
    }
    else
    {
        // if not forcing the username to lowercase, we still need to force
        // the domain name to lowercase
        //
        return users::basic_email_canonicalization(user_email);
    }
}




/** \brief Retrieve the email address of the user.
 *
 * Return the email address of the user the way it was entered by
 * the user (same case). This is the exact email address we use
 * to send emails to that user. However, we do not use it to
 * access our database index because emails may need to be canonicalized
 * before hitting the database. The canonicalization is done by
 * the get_user_key() function. The get_user_key() function
 * returns that canonicalized key.
 *
 * \return The user email as it was entered.
 */
QString const & users::user_info_t::get_user_email() const
{
    return f_user_email;
}


/** \brief Get the path to a user from an email.
 *
 * This function returns the path of the user corresponding to the specified
 * email. The function returns an the ANONYMOUS path if the user is not found
 * or the user_info_t object represents an invalid user.
 *
 * The path can be used with a content::path_info_t object. It will not
 * include the domain or a leading slash.
 *
 * Anonymous or invalid user path: "user".
 *
 * Valid user path: "user/<id>".
 *
 * \param[in] email  The email of the user to search the path for.
 *
 * \return The path to the user.
 */
QString users::user_info_t::get_user_path( bool const leading_slash ) const
{
    if( exists() )
    {
        return QString("%1%2/%3")
            .arg(leading_slash ? "/" : "")
            .arg(get_name(name_t::SNAP_NAME_USERS_PATH))
            .arg(get_identifier());
    }

    // TODO: should this be an empty string?
    //       at this time many functions expect the "/user/" path...
    //       see also permissions::get_user_path()
    //
    return QString("%1%2")
            .arg(leading_slash ? "/" : "")
            .arg(get_name(name_t::SNAP_NAME_USERS_ANONYMOUS_PATH));
}


/** \brief Set the user status.
 *
 * The status is determined by the users plugin while authenticating
 * the user and saved in the user_info_t object using this function.
 *
 * \param[in] v  The new user status.
 */
void users::user_info_t::set_status( status_t const & v )
{
    f_status = v;
}


/** \brief Retrieve the user status.
 *
 * This function returns the current user status (i.e. whether it is a
 * new user, the user needs to change his password, the user was blocked,
 * etc.)
 *
 * \return A status_t enumeration value.
 */
users::status_t users::user_info_t::get_status() const
{
    return f_status;
}


/** \brief Whether the user is considered valid.
 *
 * This function returns true if the specified user_info_t object is
 * considered valid.
 *
 * \warning
 * The anonymous user is also considered valid. If you want to know
 * whether the user is valid and not anonymous, use the is_user()
 * function instead.
 *
 * \return true if the object identifier is not IDENTIFIER_INVALID.
 */
bool users::user_info_t::is_valid() const
{
    return f_identifier != IDENTIFIER_INVALID;
}


/** \brief Whether this user_info_t represents the anonymous user.
 *
 * This function returns true if the user_info_t current represents
 * the anonymous user. This means the user identifier is
 * IDENTIFIER_ANONYMOUS (0).
 *
 * The function returns false for any user or if the object is
 * currently considered invalid.
 *
 * \return true if the object identifier if IDENTIFIER_ANONYMOUS.
 */
bool users::user_info_t::is_anonymous() const
{
    return f_identifier == IDENTIFIER_ANONYMOUS;
}


/** \brief The user object represents a valid user.
 *
 * This function returns true if the user_info_t object is a valid user
 * and does not represent the anonymous user. If you want a way to test
 * that the user is valid including the anonymous user, use the is_valid()
 * function instead.
 *
 * \return true if the object is not IDENTIFIER_INVALID nor
 *         IDENTIFIER_ANONYMOUS.
 */
bool users::user_info_t::is_user() const
{
    return f_identifier != IDENTIFIER_INVALID
        && f_identifier != IDENTIFIER_ANONYMOUS;
}


/** \brief A valid user may not exist.
 *
 * A user identifier may be valid, that does not mean the user exists
 * in the database. This function checks that case. A valid user that
 * exists in the database (i.e. a row exists with his identifier) is
 * 100% valid.
 *
 * \note
 * A user may exists in a certain website but not another. So the user
 * may be valid (is_valid() and is_user() return true), but it may not
 * exist in the data.
 *
 * \warning
 * The anonymous user is never created in the database so this function
 * always returns false for the anonymous user.
 *
 * \return true if the user exists in the database.
 */
bool users::user_info_t::exists() const
{
    // only valid users except the anonymous user may exist in the database
    //
    if(!is_user())
    {
        // i.e. no need to check with -1 or 0 because we never create those
        //      page, although a funny programmer could do that and this
        //      still prevents such invalid users (as far as the database
        //      is concerned) will thus never be viewed as existing.
        //
        return false;
    }

    init_tables();
    return f_users_table->exists(QtCassandra::QCassandraValue(f_identifier).binaryValue());
}


/** \brief Reset the user info to an invalid status.
 *
 * This function resets the user to an invalid status.
 *
 * You may use the set_identifier() again after this call.
 */
void users::user_info_t::reset()
{
    // leave f_snap and f_users_table alone, they do not change within a run
    //
    f_identifier = IDENTIFIER_INVALID;
    f_user_key.clear();
    f_user_email.clear();
    f_status = status_t::STATUS_UNDEFINED;
}


/** \brief Get the pointer to snap_child.
 *
 * This function gets the snap_child pointer. If it is not yet defined, it
 * retrieves a copy from the content plugin.
 *
 * This function cannot fail.
 *
 * \return The snap_child pointer.
 */
snap_child * users::user_info_t::get_snap() const
{
    if( f_snap == nullptr )
    {
        f_snap = content::content::instance()->get_snap();
    }
    return f_snap;
}


/** \brief Initializes the table pointers we use in various functions.
 *
 * This function retrieves the f_users_table pointer once and caches
 * it.
 */
void users::user_info_t::init_tables() const
{
    if(f_users_table == nullptr)
    {
        f_users_table = users::users::instance()->get_users_table();
    }
}


/** \brief Save a user parameter.
 *
 * This function is used to save a field directly in the "users" table.
 * Whether the user is already a registered user does not matter, the
 * function accepts to save the parameter. This is particularly important
 * for people who want to register for a newsletter or unsubscribe from
 * the website as a whole (See the sendmail plugin).
 *
 * If a value with the same field name exists, it gets overwritten.
 *
 * \param[in] field_name  The name of the field where value gets saved.
 * \param[in] value  The value of the field to save.
 *
 * \sa load_user_parameter()
 */
void users::user_info_t::save_user_parameter(QString const & field_name, QtCassandra::QCassandraValue const & value)
{
    int64_t const start_date(get_snap()->get_start_date());

    // mark when we created the user if that is not yet defined
    if( !value_exists(name_t::SNAP_NAME_USERS_CREATED_TIME) )
    {
        set_value( name_t::SNAP_NAME_USERS_CREATED_TIME, start_date );
    }

    // save the external plugin parameter
    set_value( field_name, value );

    // mark the user as modified
    set_value( name_t::SNAP_NAME_USERS_MODIFIED, start_date );
}


void users::user_info_t::save_user_parameter(QString const & field_name, QString const & value)
{
    QtCassandra::QCassandraValue v(value);
    save_user_parameter(field_name, v);
}


void users::user_info_t::save_user_parameter(QString const & field_name, int64_t const & value)
{
    QtCassandra::QCassandraValue v(value);
    save_user_parameter(field_name, v);
}


/** \brief Retrieve a user parameter.
 *
 * This function is used to read a field directly from the "users" table.
 * If the value exists, then the function returns true and the \p value
 * parameter is set to its content. If the field cannot be found, then
 * the function returns false.
 *
 * If your value cannot be an empty string, then just testing whether
 * value is the empty string on return is enough to know whether the
 * field was defined in the database.
 *
 * \param[in] field_name  The name of the field being checked.
 * \param[out] value  The value of the field, empty if undefined.
 *
 * \return true if the value was read from the database.
 *
 * \sa save_user_parameter()
 */
bool users::user_info_t::load_user_parameter(QString const & field_name, QtCassandra::QCassandraValue & value) const
{
    // reset the input value by default
    value.setNullValue();

    // make sure that row (a.k.a. user) exists before accessing it
    if( !exists() )
    {
        return false;
    }

    // row exists, make sure the user field exists
    if( !value_exists(field_name) )
    {
        return false;
    }

    // retrieve that parameter
    value = get_value(field_name);

    return true;
}


bool users::user_info_t::load_user_parameter(QString const & field_name, QString & value) const
{
    QtCassandra::QCassandraValue v;
    if(load_user_parameter(field_name, v))
    {
        value = v.stringValue();
        return true;
    }
    return false;
}


bool users::user_info_t::load_user_parameter(QString const & field_name, int64_t & value) const
{
    QtCassandra::QCassandraValue v;
    if(load_user_parameter(field_name, v))
    {
        value = v.safeInt64Value();
        return true;
    }
    return false;
}


/** \brief Load user info indexed by an email address.
 *
 * This function canonicalizes the email address of the user and then
 * attempts to load the user identifier. If that is found, then the
 * function saves the identifier in f_identifier in effect marking the
 * structure valid.
 */
void users::user_info_t::get_user_id_by_email()
{
    init_tables();
    auto row( f_users_table->row(get_name(name_t::SNAP_NAME_USERS_INDEX_ROW))  );

    if(row->exists(get_user_key()))
    {
        // found the user, retrieve the current id
        //
        f_identifier = row->cell(get_user_key())->value().int64Value();
        if(!is_user())
        {
            // the identifier is unfortunately not correct
            //
            reset();
        }
    }
    else
    {
        // it did not work, reset the object
        //
        reset();
    }
}


/** \brief Get the row object to this user information.
 *
 * This function makes sure that the f_users_table pointer is defined
 * and then retrieves the row corresponding to the current user
 * (which is defined by the f_identifier of the user.)
 */
QtCassandra::QCassandraRow::pointer_t users::user_info_t::get_user_row() const
{
    init_tables();
    return f_users_table->row(QtCassandra::QCassandraValue(f_identifier).binaryValue());
}


SNAP_PLUGIN_EXTENSION_END()

// vim: ts=4 sw=4 et
