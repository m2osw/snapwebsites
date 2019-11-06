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
#include    "snapdatabase/block_blob.h"


// last include
//
#include    <snapdev/poison.h>



namespace snapdatabase
{



namespace detail
{
}



// 'BLOB'
struct_description_t * g_block_blob =
{
    define_description(
          FieldName("magic")    // dbtype_t = BLOB
        , FieldType(struct_type_t::STRUCT_TYPE_UINT32)
    ),
    define_description(
          FieldName("size")
        , FieldType(struct_type_t::STRUCT_TYPE_UINT32)
    ),
    define_description(
          FieldName("next_blob")
        , FieldType(struct_type_t::STRUCT_TYPE_REFERENCE)
    ),
    end_descriptions()
};



block_blob::block_blob(dbfile::pointer_t f, file_addr_t offset)
    : block(f, offset)
    , f_structure(g_block_blob, data(), offset)
{
}


uint32_t block_blob::get_size()
{
    return static_cast<uint32_t>(f_structure.get_uinteger("size"));
}


void block_blob::set_size(uint32_t size)
{
    f_structure.set_uinteger("size", size);
}


file_addr_t block_blob::get_next_blob()
{
    return static_cast<file_addr_t>(f_structure.get_uinteger("next_free_block"));
}


void block_blob::set_next_blob(file_addr_t offset)
{
    f_structure.set_uinteger("next_free_block", offset);
}





} // namespace snapdatabase
// vim: ts=4 sw=4 et
