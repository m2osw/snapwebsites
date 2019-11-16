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
 * \brief Database file implementation.
 *
 * Each table uses one or more files. Each file is handled by a dbfile
 * object and a corresponding set of blocks.
 */

// self
//
#include    "snapdatabase/row.h"


// last include
//
#include    <snapdev/poison.h>



namespace snapdatabase
{



row::row(table::pointer_t t)
    : f_table(t)
{
}


buffer_t row::to_binary()
{
    buffer_t result;

    return result;
}


void row::from_binary(buffer_t const & blob) const
{
    table::pointer_t t(f_table.lock());
    schema_column::map_by_id_t columns(t->columns_by_id());

    size_t pos(0);
    for(auto c : columns)
    {
        column_id_t const id((blob[pos    ] << 0)
                           + (blob[pos + 1] << 8));

        auto it(columns.find(id));
        if(it == columns.end())
        {
            throw column_not_found(
                    "column "
                    + std::to_string(static_cast<int>(id))
                    + " not found.");
        }
    }
}



} // namespace snapdatabase
// vim: ts=4 sw=4 et
