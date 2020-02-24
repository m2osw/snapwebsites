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
#pragma once


/** \file
 * \brief Cursor file header.
 *
 * The cursor class handles the reading of rows from the database file.
 * It requires a set of conditions (from the class of that name) and
 * it returns a vector of rows.
 *
 * It is setup as a cursor so you do not directly get a vector of rows.
 * Instead it offers a way to read the next row and the previous row.
 */

// self
//
#include    "snapdatabase/database/conditions.h"



namespace snapdatabase
{



class table;
typedef std::shared_ptr<table>                  table_pointer_t;
typedef std::vector<row_pointer_t>              row_vector_t;

namespace detail
{
class cursor_state;
typedef std::shared_ptr<cursor_state>           cursor_state_pointer_t;
} // detail namespace


class cursor
    : public std::enable_shared_from_this<cursor>
{
public:
    typedef std::shared_ptr<cursor>             pointer_t;

                                                cursor(table_pointer_t table, detail::cursor_state_pointer_t state, conditions const & cond);

    conditions const &                          get_conditions() const;

    bool                                        empty() const;
    bool                                        complete() const;
    void                                        rewind();
    size_t                                      get_position() const;
    row_pointer_t                               next_row();
    row_pointer_t                               previous_row();

    bool                                        get_cache() const;
    void                                        set_cache(bool cache = true);
    bool                                        forget(row_pointer_t row);

    // the following functions are considered private, only use them inside
    // the library; they may change or even be removed at any time
    //
    detail::cursor_state_pointer_t              get_state();
    row_vector_t &                              get_rows();

private:
    table_pointer_t                             f_table;
    detail::cursor_state_pointer_t              f_cursor_state = detail::cursor_state_pointer_t();
    conditions const                            f_conditions;
    size_t                                      f_global_position = 0;      // current position in the index
    size_t                                      f_local_position = 0;       // current position in f_rows
    bool                                        f_multiple_pages = false;   // whether we handle a 2nd page before calling set_cache()
    bool                                        f_cache = false;            // keep all the data; otherwise keep at most conditions.f_count rows
    bool                                        f_complete = false;         // if true we found the end of the data
    row_vector_t                                f_rows = row_vector_t();
};



} // namespace snapdatabase
// vim: ts=4 sw=4 et
