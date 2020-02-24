// Copyright (c) 2019  Made to Order Software Corp.  All Rights Reserved
//
// https://snapwebsites.org/project/snapdatabase
// contact@m2osw.com
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
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.


/** \file
 * \brief Conditions implementation.
 *
 * When querying a table, you need to have a condition object. This allows
 * you to define which index you want to use (an equivalent to an ORDER BY)
 * and which columns to check for equality, minimum, maximum values.
 */

// self
//
#include    "snapdatabase/database/conditions.h"

#include    "snapdatabase/database/row.h"


// C++ lib
//
#include    <iostream>


// last include
//
#include    <snapdev/poison.h>



namespace snapdatabase
{



/** \brief Set the list of columns the user wants returned.
 *
 * For some tables, returning all the columns may often be a waste of time
 * and bandwidth (especially if one of the columns is really large).
 *
 * This function lets you define the list of columns that you want returned.
 * By default the list is empty meaning that all the columns will be
 * returned.
 *
 * \param[in] column_names  The name of the columns to return in your cursor.
 */
void conditions::set_columns(column_names_t const & column_names)
{
    f_column_names = column_names;
}


column_names_t const & conditions::get_columns() const
{
    return f_column_names;
}


void conditions::set_offset(count_t offset)
{
    f_offset = offset;
}


count_t conditions::get_offset() const
{
    return f_offset;
}


void conditions::set_count(count_t count)
{
    f_count = count;
}


count_t conditions::get_count() const
{
    return f_count;
}


void conditions::set_limit(count_t limit)
{
    f_limit = limit;
}


count_t conditions::get_limit() const
{
    return f_limit;
}


void conditions::set_key(std::string const & index_name, row::pointer_t min_key, row::pointer_t max_key)
{
    f_index_name = index_name;

    f_min_key = min_key;
    f_max_key = max_key;
}


std::string const & conditions::get_index_name() const
{
    return f_index_name;
}


row::pointer_t conditions::get_min_key() const
{
    return f_min_key;
}


row::pointer_t conditions::get_max_key() const
{
    return f_max_key;
}


buffer_t const & conditions::get_murmur_key() const
{
    if(f_min_key == nullptr)
    {
        throw snapdatabase_logic_error(
                    "the conditions::get_murmur_key() can only be used if"
                    " the minimum key is defined.");
    }

    if(f_murmur_key.empty())
    {
        f_murmur_key.resize(16);
        f_min_key->generate_mumur3(f_murmur_key);
    }

    return f_murmur_key;
}


void conditions::set_filter(row::pointer_t min_key, row::pointer_t max_key)
{
    f_min_filter = min_key;
    f_max_filter = max_key;
}


row::pointer_t conditions::get_min_filter() const
{
    return f_min_filter;
}


row::pointer_t conditions::get_max_filter() const
{
    return f_max_filter;
}

void conditions::set_nulls(null_mode_t mode)
{
    f_null_mode = mode;
}


null_mode_t conditions::get_nulls() const
{
    return f_null_mode;
}


void conditions::set_reverse(bool reverse)
{
    f_reverse = reverse;
}


bool conditions::get_reverse() const
{
    return f_reverse;
}



} // namespace snapdatabase
// vim: ts=4 sw=4 et
