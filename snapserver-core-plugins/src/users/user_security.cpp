// Snap Websites Server -- users security handling
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
 * \brief Users security check structure handling.
 *
 * This file is the implementation of the user_security_t class used
 * to check whether a user is considered valid before registering him
 * or sending an email to him.
 */

#include "users.h"

#include <snapwebsites/poison.h>


SNAP_PLUGIN_EXTENSION_START(users)


/** \brief Setup the user_security_t object.
 *
 * This function sets the user_info, email (optional) and whether example
 * email addresses are allowed.
 *
 * Note that the get_email() function returns the email address in
 * the user_info object if defined (which means that user is considered
 * valid.) If the user_info object has no email, then the \p email
 * parameter specified in this function is returned instead. This is
 * important when creating a new user since at that point the user_info
 * structure does not have a valid email address.
 *
 * If you are unsure, pass QString() as the email address. It is likely
 * the correct value in all cases except in users::register_user().
 *
 * \param[in] user_info  The user being checked. The function makes a copy.
 * \param[in] email  The email address of the user or QString().
 * \param[in] allow_example_domain  The domain name may be an example too.
 *
 * \sa get_email()
 * \sa get_user_info()
 * \sa get_allow_example_domain()
 * \sa users::register_user()
 */
void users::user_security_t::set_user_info(user_info_t const & user_info, QString const & email, bool const allow_example_domain)
{
    f_user_info            = user_info;
    f_email                = email;
    f_allow_example_domain = allow_example_domain;
}


void users::user_security_t::set_password(QString const & password)
{
    f_password = password;
}


void users::user_security_t::set_policy(QString const & policy)
{
    f_policy = policy;
}


void users::user_security_t::set_bypass_blacklist(bool const bypass)
{
    f_bypass_blacklist = bypass;
}


void users::user_security_t::set_example(bool const example)
{
    f_example = example;
}


void users::user_security_t::set_status(status_t status)
{
    // we can only change the status once from valid to something else
    //
    if(f_status == status_t::STATUS_VALID)
    {
        f_status = status;
    }
}


/** \brief Return the user email address.
 *
 * In most cases this function will return the email address
 * defined in the user_info parameter passed to set_user_info().
 *
 * If we are creating the user, though, that email address will
 * be empty so instead we return our f_email variable member.
 * This normally includes the user email if available but it
 * does not yet represent a valid user in the database.
 * (i.e. when one called the users::register_user() function.)
 *
 * It is still possible that this function return an empty string
 * although it is not likely.
 *
 * \return The user email address or an empty string.
 *
 * \sa set_user_info()
 * \sa users::register_user()
 */
QString const & users::user_security_t::get_email() const
{
    return f_user_info.get_user_email().isEmpty() ? f_email : f_user_info.get_user_email();
}


bool users::user_security_t::has_password() const
{
    return f_password != "!";
}


users::user_info_t const & users::user_security_t::get_user_info() const
{
    return f_user_info;
}


QString const & users::user_security_t::get_password() const
{
    return f_password;
}


QString const & users::user_security_t::get_policy() const
{
    return f_policy;
}


bool users::user_security_t::get_bypass_blacklist() const
{
    return f_bypass_blacklist;
}


bool users::user_security_t::get_allow_example_domain() const
{
    return f_allow_example_domain;
}


bool users::user_security_t::get_example() const
{
    return f_example;
}


content::permission_flag &  users::user_security_t::get_secure()
{
    return f_secure;
}


users::users::status_t users::user_security_t::get_status() const
{
    return f_status;
}



SNAP_PLUGIN_EXTENSION_END()

// vim: ts=4 sw=4 et
