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
#include    "snapdatabase/file_snap_database_table.h"


// last include
//
#include    <snapdev/poison.h>



namespace snapdatabase
{



namespace detail
{
}



// 'SDBT'
constexpr struct_description_t g_snap_database_table_description[] =
{
    define_description(
          FieldName("magic")    // dbtype_t = SDBT
        , FieldType(struct_type_t::STRUCT_TYPE_UINT32)
    ),
    define_description(
          FieldName("version")
        , FieldType(struct_type_t::STRUCT_TYPE_VERSION)
    ),
    define_description(
          FieldName("block_size")
        , FieldType(struct_type_t::STRUCT_TYPE_UINT32)
    ),
    define_description(
          FieldName("table_definition") // this is the schema
        , FieldType(struct_type_t::STRUCT_TYPE_REFERENCE)
    ),
    define_description(
          FieldName("first_free_block")
        , FieldType(struct_type_t::STRUCT_TYPE_REFERENCE)
    ),
    // at this time we do not allow dynamically created/dropped tables
    //define_description(
    //      FieldName("table_expiration_date")
    //    , FieldType(struct_type_t::STRUCT_TYPE_TIME)
    //),
    define_description(
          FieldName("indirect_index")
        , FieldType(struct_type_t::STRUCT_TYPE_REFERENCE)
    ),
    define_description(
          FieldName("last_oid")
        , FieldType(struct_type_t::STRUCT_TYPE_OID)
    ),
    define_description(
          FieldName("first_free_oid")
        , FieldType(struct_type_t::STRUCT_TYPE_OID)
    ),
    define_description(
          FieldName("first_compactable_block")
        , FieldType(struct_type_t::STRUCT_TYPE_REFERENCE)
    ),
    define_description(
          FieldName("top_key_index_block")
        , FieldType(struct_type_t::STRUCT_TYPE_REFERENCE)
    ),
    define_description(
          FieldName("expiration_index_block")
        , FieldType(struct_type_t::STRUCT_TYPE_REFERENCE)
    ),
    define_description(
          FieldName("secondary_index_block")
        , FieldType(struct_type_t::STRUCT_TYPE_REFERENCE)
    ),
    define_description(
          FieldName("tree_index_block")
        , FieldType(struct_type_t::STRUCT_TYPE_REFERENCE)
    ),
    define_description(
          FieldName("deleted_rows")
        , FieldType(struct_type_t::STRUCT_TYPE_UINT64)
    ),
    define_description( // bloom filters use separate files
          FieldName("bloom_filter_flags=algorithm:4/renewing")
        , FieldType(struct_type_t::STRUCT_TYPE_BITS32)
    ),
    end_descriptions()
};









file_snap_database_table::file_snap_database_table(dbfile::pointer_t f, reference_t offset)
    : block(f, offset)
{
    f_structure = std::make_shared<structure>(g_snap_database_table_description);
}


version_t file_snap_database_table::get_version() const
{
    return static_cast<version_t>(static_cast<uint32_t>(f_structure->get_uinteger("version")));
}


void file_snap_database_table::set_version(version_t v)
{
    f_structure->set_uinteger("version", v.to_binary());
}


uint32_t file_snap_database_table::get_block_size() const
{
    return static_cast<reference_t>(f_structure->get_uinteger("block_size"));
}


void file_snap_database_table::set_block_size(uint32_t offset)
{
    f_structure->set_uinteger("block_size", offset);
}


reference_t file_snap_database_table::get_first_free_block() const
{
    return static_cast<reference_t>(f_structure->get_uinteger("first_free_block"));
}


void file_snap_database_table::set_first_free_block(reference_t offset)
{
    f_structure->set_uinteger("first_free_block", offset);
}



} // namespace snapdatabase
// vim: ts=4 sw=4 et