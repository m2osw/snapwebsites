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
 * \brief Conditions file header.
 *
 * The conditions class defines a set of conditions used to query the
 * database. This is similar to the information you would have in an
 * SQL SELECT statement.
 */

// self
//
#include    "snapdatabase/data/virtual_buffer.h"


// C++ lib
//
#include    <memory>
#include    <string>
#include    <vector>



namespace snapdatabase
{



class row;
typedef std::shared_ptr<row>                    row_pointer_t;
typedef std::vector<std::string>                column_names_t;

enum class null_mode_t
{
    NULL_MODE_SORTED,       // keep in order found in index (default)
    NULL_MODE_IGNORE,       // remove any row with nulls in the key
    NULL_MODE_FIRST,        // return the rows with nulls first
    NULL_MODE_LAST          // return the rows with nulls last
};

typedef size_t                                  count_t;
constexpr count_t                               DEFAULT_CURSOR_COUNT = 100;
constexpr count_t                               CURSOR_NO_LIMIT = 0;


// TODO: at some point we want to consider supporting aggregates
//       (look at enhancing the "column_names_t" to expressions that allow
//       us to have operations and aggregates such as COUNT(), AVG(), SUM(),
//       MIN(), MAX()...)
//
// TODO: look into having an array of filters like we can have many comparison
//       operations in a WHERE statement; right now we only support:
//           a BETWEEN b AND c
//       also our filter is applied to all the columns defined in there instead
//       of just a few... so it needs help
//
//       Note: that with the BETWEEN operation we can already implement all
//             of the following:
//
//               =
//               <
//               <=
//               >
//               >=
//               BETWEEN
//
//             what is definitely missing are:
//
//               <>
//               IN
//               NOT IN
//
//       also we're missing all sorts of dynamic possibilities (i.e. compare
//       columns between each others, and we can dream: have cross products)
//
// In other words, the filtering should be an array of filter objects that
// allow us to do "anything we want" instead of just that BETWEEN support.
//
class conditions
{
public:
    void                                        set_columns(column_names_t const & column_names);
    column_names_t const &                      get_columns() const;

    void                                        set_offset(count_t offset);
    count_t                                     get_offset() const;

    void                                        set_count(count_t count);
    count_t                                     get_count() const;

    void                                        set_limit(count_t limit);
    count_t                                     get_limit() const;

    void                                        set_key(std::string const & index_name, row_pointer_t min_key, row_pointer_t max_key);
    std::string const &                         get_index_name() const;
    row_pointer_t                               get_min_key() const;
    row_pointer_t                               get_max_key() const;
    buffer_t const &                            get_murmur_key() const;

    void                                        set_filter(row_pointer_t min_key, row_pointer_t max_key);
    row_pointer_t                               get_min_filter() const;
    row_pointer_t                               get_max_filter() const;

    void                                        set_nulls(null_mode_t mode);
    null_mode_t                                 get_nulls() const;

    void                                        set_reverse(bool reverse = true);
    bool                                        get_reverse() const;

private:
    column_names_t                              f_column_names = column_names_t();  // if empty, all columns
    size_t                                      f_offset = 0;
    size_t                                      f_count = DEFAULT_CURSOR_COUNT;     // number of rows per batch (i.e. transferred between server/client)
    size_t                                      f_limit = CURSOR_NO_LIMIT;          // total number of rows to read
    std::string                                 f_index_name = std::string();
    row_pointer_t                               f_min_key = row_pointer_t();
    row_pointer_t                               f_max_key = row_pointer_t();
    row_pointer_t                               f_min_filter = row_pointer_t();
    row_pointer_t                               f_max_filter = row_pointer_t();
    mutable buffer_t                            f_murmur_key = buffer_t();
    null_mode_t                                 f_null_mode = null_mode_t::NULL_MODE_SORTED;
    bool                                        f_reverse = false;
};




} // namespace snapdatabase
// vim: ts=4 sw=4 et
