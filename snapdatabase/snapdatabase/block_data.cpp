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
#include    "snapdatabase/block_data.h"

#include    "snapdatabase/table.h"

// last include
//
#include    <snapdev/poison.h>



namespace snapdatabase
{



namespace detail
{
}



// 'DATA'
constexpr struct_description_t g_data_description[] =
{
    define_description(
          FieldName("magic")    // dbtype_t = DATA
        , FieldType(struct_type_t::STRUCT_TYPE_UINT32)
    ),
    end_descriptions()
};


block_data::block_data(dbfile::pointer_t f, reference_t offset)
    : block(f, offset)
{
    f_structure = std::make_shared<structure>(g_data_description);
}


uint32_t block_data::block_total_space(table_pointer_t t)
{
    return t->get_page_size() - sizeof(uint32_t);
}



} // namespace snapdatabase
// vim: ts=4 sw=4 et
