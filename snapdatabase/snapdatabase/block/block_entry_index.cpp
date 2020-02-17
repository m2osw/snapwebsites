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
 * \brief Block Entry Index implementation.
 *
 * The data is indexed using a Block Entry Index as the bottom block. This
 * is the block which includes the remainder of the key and then a pointer
 * to the actual data or to an `IDXP` if the entry points to multiple
 * rows (i.e. secondary index allowing duplicates).
 */

// self
//
#include    "snapdatabase/block/block_entry_index.h"

#include    "snapdatabase/block/block_header.h"


// last include
//
#include    <snapdev/poison.h>



namespace snapdatabase
{



namespace
{



// 'EIDX'
constexpr struct_description_t g_description[] =
{
    define_description(
          FieldName("header")
        , FieldType(struct_type_t::STRUCT_TYPE_STRUCTURE)
        , FieldSubDescription(detail::g_block_header)
    ),
    define_description(
          FieldName("count")
        , FieldType(struct_type_t::STRUCT_TYPE_UINT32)
    ),
    define_description(
          FieldName("size")
        , FieldType(struct_type_t::STRUCT_TYPE_UINT32)
    ),
    define_description(
          FieldName("next")
        , FieldType(struct_type_t::STRUCT_TYPE_REFERENCE)
    ),
    define_description(
          FieldName("previous")
        , FieldType(struct_type_t::STRUCT_TYPE_REFERENCE)
    ),
    // followed by actual index entries (g_entry_index_item)
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




block_entry_index::block_entry_index(dbfile::pointer_t f, reference_t offset)
    : block(g_descriptions_by_version, f, offset)
{
}


uint32_t block_entry_index::get_count() const
{
    return static_cast<uint32_t>(f_structure->get_uinteger("count"));
}


void block_entry_index::set_count(uint32_t count)
{
    f_structure->set_uinteger("count", count);
}


uint32_t block_entry_index::get_size() const
{
    return static_cast<uint32_t>(f_structure->get_uinteger("size"));
}


void block_entry_index::set_size(uint32_t size)
{
    f_structure->set_uinteger("size", size);
}


reference_t block_entry_index::get_next() const
{
    return static_cast<reference_t>(f_structure->get_uinteger("next"));
}


void block_entry_index::set_next(reference_t offset)
{
    f_structure->set_uinteger("next", offset);
}


reference_t block_entry_index::get_previous() const
{
    return static_cast<reference_t>(f_structure->get_uinteger("previous"));
}


void block_entry_index::set_previous(reference_t offset)
{
    f_structure->set_uinteger("previous", offset);
}







} // namespace snapdatabase
// vim: ts=4 sw=4 et
