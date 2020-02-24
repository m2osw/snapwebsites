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
 * \brief Cursor file implementation.
 *
 * The select() function takes in a condition and attaches a table to a
 * cursor which can then be used to read rows from that table.
 *
 * The concept is pretty simple. Once you have a cursor handy, you can
 * just read all the rows using the next_row() function. The condition
 * is something similar to the WHERE clause on a SELECT. We also
 * offer the ability to include a list of column names so only the
 * data in those columns is returned (which could save a fair bit of
 * time transfering data between the server and the client).
 *
 * The order in which the rows are returned is defined by the index
 * used to retrieve the rows. The cursor will not modify that order.
 *
 * Before reading anything, you may want to set the cache flag to true
 * if you plan to go through the list of rows multiple times and you
 * don't expect to have millions of them. This asks the cursor object
 * to save all that data in a vector and allows for instant retrieval.
 * Note that this flag doesn't get propagated to the backend. So on
 * the server, we never cache all the rows in a cursor (the table
 * object may (is likely to) cache many rows already).
 *
 * \warning
 * The cursor is not idempotent. If you rewind() and next_row() again and
 * again, the list of rows returned by the cursor may change on each run.
 * This is because we do not freeze the state of the database at the
 * time the cursor is created. One way to partially avoid this \em strange
 * side effect is to check the system "_last_updated" column against the
 * time at which you created the cursor. You may still see rows disappearing
 * from the list (because of a delete), but you will not see new rows added
 * after the cursor was created.
 */

// self
//
#include    "snapdatabase/database/cursor.h"

#include    "snapdatabase/database/row.h"


// C++ lib
//
#include    <iostream>


// last include
//
#include    <snapdev/poison.h>



namespace snapdatabase
{



cursor::cursor(table_pointer_t table, detail::cursor_state_pointer_t state, conditions const & cond)
    : f_table(table)
    , f_cursor_state(state)
    , f_conditions(cond)
{
}


conditions const & cursor::get_conditions() const
{
    return f_conditions;
}


bool cursor::empty() const
{
    return f_complete && f_global_position == 0;
}


bool cursor::complete() const
{
    return f_complete;
}


void cursor::rewind()
{
    f_local_position = 0;

    if(!f_cache && f_global_position > 0)
    {
        f_global_position = 0;
        f_complete = false;
    }
}


size_t cursor::get_position() const
{
std::cerr << "position global = " << f_global_position << " local = " << f_local_position << "\n";
    return f_global_position + f_local_position;
}


row::pointer_t cursor::next_row()
{
std::cerr << "get next row: " << f_local_position << " vs " << f_rows.size() << " cache " << f_cache << "\n";
    if(f_local_position >= f_rows.size())
    {
        if(f_complete)
        {
            return row::pointer_t();
        }

        // read some more rows
        //
        if(!f_cache)
        {
            f_multiple_pages = true;
            f_global_position += f_rows.size();
            f_rows.clear();
            f_local_position = 0;
        }

        f_table->read_rows(shared_from_this());

        // this happens when no new rows were added by the read_rows() call
        //
        if(f_local_position >= f_rows.size())
        {
            return row::pointer_t();
        }
    }

    row::pointer_t result(f_rows[f_local_position]);
    ++f_local_position;
    return result;
}


row::pointer_t cursor::previous_row()
{
    if(f_local_position == 0)
    {
        if(f_global_position == 0)
        {
            return row::pointer_t();
        }

        // read some previous rows (again)
        //
        f_multiple_pages = true;
        f_rows.clear();

        size_t const count(f_conditions.get_count());
        if(f_global_position > count)
        {
            f_global_position -= count;
        }
        else
        {
            f_global_position = 0;
        }

        f_table->read_rows(shared_from_this());

        f_local_position = f_rows.size();

        // this happens if the rows we read earlier do not match anymore
        // (this means the `limit` calculation can be quite skewed)
        //
        if(f_local_position == 0)
        {
            return row::pointer_t();
        }
    }

    --f_local_position;
    return f_rows[f_local_position];
}


bool cursor::get_cache() const
{
    return f_cache;
}


void cursor::set_cache(bool cache)
{
    if(cache)
    {
        // TODO: add test to accept calls when we're still in the very first
        //       block (how to test that?)
        //
        if(!f_multiple_pages)
        {
            f_cache = true;
            return;
        }
    }
    else
    {
        // we can always turn it off
        //
        f_cache = false;
        return;
    }

    throw snapdatabase_logic_error("cursor::set_cache() called too late.");
}


bool cursor::forget(row::pointer_t row)
{
    auto it(std::find(f_rows.begin(), f_rows.end(), row));
    if(it != f_rows.end())
    {
        f_rows.erase(it);
        return true;
    }

    return false;
}


detail::cursor_state_pointer_t cursor::get_state()
{
    return f_cursor_state;
}


row::vector_t & cursor::get_rows()
{
    return f_rows;
}



} // namespace snapdatabase
// vim: ts=4 sw=4 et
