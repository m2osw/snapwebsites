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
#include    "snapdatabase/block/block_entry_index.h"


// last include
//
#include    <snapdev/poison.h>



namespace snapdatabase
{



// 'EIDX'
constexpr struct_description_t g_entry_index_description[] =
{
    define_description(
          FieldName("magic")    // dbtype_t = EIDX
        , FieldType(struct_type_t::STRUCT_TYPE_UINT32)
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



//constexpr uint8_t ENTRY_INDEX_ITEM_FLAG_COMPLETE        = 0x01;
//constexpr uint8_t ENTRY_INDEX_ITEM_FLAG_INDP            = 0x02;     // "data" field
//constexpr uint8_t ENTRY_INDEX_ITEM_FLAG_INDP_IDENTIFIER = 0x04;     // "index_identifier" field
//
//constexpr struct_description_t g_entry_index_item[] =
//{
//    define_description(
//          FieldName("flags=complete/index_pointer")
//        , FieldType(struct_type_t::STRUCT_TYPE_BITS8)
//    ),
//    define_description(
//          FieldName("data") // reference to `DATA` or row ID (`INDP`)
//        , FieldType(struct_type_t::STRUCT_TYPE_UINT64)
//    ),
//    define_description(
//          FieldName("index_identifier") // reference to a list of `DATA` references or row IDs
//        , FieldType(struct_type_t::STRUCT_TYPE_REFERENCE)
//    ),
//    // This is dynamically accessed by this block implementation
//    //define_description(
//    //      FieldName("key")
//    //    , FieldType(struct_type_t::STRUCT_TYPE_UINT8 * n)
//    //),
//    end_descriptions()
//};

    //- Flags (`uint8_t`, 8 bits for the flags)
    //  - Flag: Key is Complete (0x80); which means we do not have to compare
    //    again once we access the data
    //- Size defines the size of the entire Index and thus the Key Data
    //- Pointer to `DATA` or `INDP` (Type: `uint64_t`)
    //- Index Identifier (optional, see flags, Type: `uint64_t`)
    //- Key Data


block_entry_index::block_entry_index(dbfile::pointer_t f, reference_t offset)
    : block(f, offset)
{
    f_structure = std::make_shared<structure>(g_entry_index_description);
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
