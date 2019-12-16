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
#include    "snapdatabase/block/block_blob.h"

#include    "snapdatabase/block/block_header.h"


// last include
//
#include    <snapdev/poison.h>



namespace snapdatabase
{



namespace
{



// 'BLOB'
constexpr struct_description_t g_description[] =
{
    define_description(
          FieldName("header")
        , FieldType(struct_type_t::STRUCT_TYPE_STRUCTURE)
        , FieldSubDescription(detail::g_block_header)
    ),
    define_description(
          FieldName("size")
        , FieldType(struct_type_t::STRUCT_TYPE_UINT32)
    ),
    define_description(
          FieldName("next_blob")    // blob is so big we need multiple buffers
        , FieldType(struct_type_t::STRUCT_TYPE_REFERENCE)
    ),
    end_descriptions()
};


constexpr descriptions_by_version_t const g_descriptions_by_version[] =
{
    define_description_by_version(
        DescriptionVersion(0, 1),
        DescriptionDescription(g_description)
    ),
    end_descriptions_by_version()
};



}
// no name namespace




block_blob::block_blob(dbfile::pointer_t f, reference_t offset)
    : block(g_descriptions_by_version, f, offset)
{
}


uint32_t block_blob::get_size()
{
    return static_cast<uint32_t>(f_structure->get_uinteger("size"));
}


void block_blob::set_size(uint32_t size)
{
    f_structure->set_uinteger("size", size);
}


reference_t block_blob::get_next_blob()
{
    return static_cast<reference_t>(f_structure->get_uinteger("next_blob"));
}


void block_blob::set_next_blob(reference_t offset)
{
    f_structure->set_uinteger("next_blob", offset);
}





} // namespace snapdatabase
// vim: ts=4 sw=4 et
