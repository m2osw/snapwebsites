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
#include    "snapdatabase/block_secondary_index.h"


// last include
//
#include    <snapdev/poison.h>



namespace snapdatabase
{



// 'SIDX'
constexpr struct_description_t g_secondary_index_description[] =
{
    define_description(
          FieldName("magic")    // dbtype_t = SIDX
        , FieldType(struct_type_t::STRUCT_TYPE_UINT32)
    ),
    define_description(
          FieldName("id")
        , FieldType(struct_type_t::STRUCT_TYPE_UINT32)
    ),
    define_description(
          FieldName("number_of_rows")
        , FieldType(struct_type_t::STRUCT_TYPE_UINT64)
    ),
    define_description(
          FieldName("top_index")
        , FieldType(struct_type_t::STRUCT_TYPE_REFERENCE)
    ),
    //define_description(
    //      FieldName("first_index_block_with_free_space")
    //    , FieldType(struct_type_t::STRUCT_TYPE_REFERENCE)
    //),
    define_description(
          FieldName("bloom_filter_flags=algorithm:4/renewing")
        , FieldType(struct_type_t::STRUCT_TYPE_BITS32)
    ),
    end_descriptions()
};




block_secondary_index::block_secondary_index(dbfile::pointer_t f, reference_t offset)
    : block(f, offset)
{
    f_structure = std::make_shared<structure>(g_secondary_index_description);
}


uint32_t block_secondary_index::get_id() const
{
    return static_cast<uint32_t>(f_structure->get_uinteger("id"));
}


void block_secondary_index::set_id(uint32_t id)
{
    f_structure->set_uinteger("id", id);
}


uint64_t block_secondary_index::get_number_of_rows() const
{
    return static_cast<reference_t>(f_structure->get_uinteger("number_of_rows"));
}


void block_secondary_index::set_number_of_rows(uint64_t count)
{
    f_structure->set_uinteger("number_of_rows", count);
}


reference_t block_secondary_index::get_top_index() const
{
    return static_cast<reference_t>(f_structure->get_uinteger("top_index"));
}


void block_secondary_index::set_top_index(reference_t offset)
{
    f_structure->set_uinteger("top_index", offset);
}


uint32_t block_secondary_index::get_bloom_filter_flags() const
{
    return static_cast<reference_t>(f_structure->get_uinteger("bloom_filter_flags"));
}


void block_secondary_index::set_bloom_filter_flags(uint32_t flags)
{
    f_structure->set_uinteger("bloom_filter_flags", flags);
}





} // namespace snapdatabase
// vim: ts=4 sw=4 et
