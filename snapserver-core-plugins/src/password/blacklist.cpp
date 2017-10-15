// Snap Websites Server -- password policy handling
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

#include "password.h"

#include <QRegExp>

#include <snapwebsites/poison.h>


SNAP_PLUGIN_EXTENSION_START(password)



void blacklist_t::add_passwords(QString const & passwords)
{
    QtCassandra::QCassandraTable::pointer_t table(password::password::instance()->get_password_table());
    QtCassandra::QCassandraValue value;
    value.setSignedCharValue(1);
    QString const exists_in_blacklist(get_name(name_t::SNAP_NAME_PASSWORD_EXISTS_IN_BLACKLIST));

    passwords_to_list(passwords);
    int const max_passwords(f_list.size());
    for(int idx(0); idx < max_passwords; ++idx)
    {
        QString const user_password(f_list[idx]);
        if(table->exists(user_password))
        {
            ++f_skipped;
        }
        else
        {
            // add a new entry
            //
            ++f_count;
            table->row(user_password)->cell(exists_in_blacklist)->setValue(value);
        }
    }
}


void blacklist_t::remove_passwords(QString const & passwords)
{
    QtCassandra::QCassandraTable::pointer_t table(password::password::instance()->get_password_table());
    QString const exists_in_blacklist(get_name(name_t::SNAP_NAME_PASSWORD_EXISTS_IN_BLACKLIST));

    passwords_to_list(passwords);
    int const max_passwords(f_list.size());
    for(int idx(0); idx < max_passwords; ++idx)
    {
        QString const user_password(f_list[idx]);
        if(table->exists(user_password))
        {
            // remove an entry
            //
            ++f_count;
            table->dropRow(user_password);
        }
        else
        {
            ++f_skipped;
        }
    }
}


void blacklist_t::reset_counters()
{
    f_count = 0;
    f_skipped = 0;
}


size_t blacklist_t::passwords_applied() const
{
    return f_count;
}


size_t blacklist_t::passwords_skipped() const
{
    return f_skipped;
}


void blacklist_t::passwords_to_list(QString const & passwords)
{
    QRegExp re("<br *\\/?>|\\n|\\r");
    f_list = passwords.toLower().split(re, QString::SkipEmptyParts);
}


SNAP_PLUGIN_EXTENSION_END()

// vim: ts=4 sw=4 et
