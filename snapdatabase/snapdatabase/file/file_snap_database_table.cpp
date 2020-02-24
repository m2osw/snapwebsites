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
#include    "snapdatabase/file/file_snap_database_table.h"

#include    "snapdatabase/block/block_header.h"


// C++ lib
//
#include    <iostream>


// last include
//
#include    <snapdev/poison.h>



namespace snapdatabase
{



namespace detail
{
}



namespace
{



// 'SDBT' -- snapdatabase file
constexpr struct_description_t g_description[] =
{
    define_description(
          FieldName("header")
        , FieldType(struct_type_t::STRUCT_TYPE_STRUCTURE)
        , FieldSubDescription(detail::g_block_header)
    ),
    define_description(
          FieldName("file_version")
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
          FieldName("update_last_oid")
        , FieldType(struct_type_t::STRUCT_TYPE_OID)
    ),
    define_description(
          FieldName("update_oid")
        , FieldType(struct_type_t::STRUCT_TYPE_OID)
    ),
    define_description(
          FieldName("blobs_with_free_space")
        , FieldType(struct_type_t::STRUCT_TYPE_REFERENCE)
    ),
    define_description(
          FieldName("first_compactable_block")
        , FieldType(struct_type_t::STRUCT_TYPE_REFERENCE)
    ),
    define_description(
          FieldName("primary_index_block")
        , FieldType(struct_type_t::STRUCT_TYPE_REFERENCE)
    ),
    define_description(
          FieldName("primary_index_reference_zero")
        , FieldType(struct_type_t::STRUCT_TYPE_REFERENCE)
    ),
    define_description( // likely to point to a secondary index (TBD)
          FieldName("top_branch_index_block")
        , FieldType(struct_type_t::STRUCT_TYPE_REFERENCE)
    ),
    define_description( // likely to point to a secondary index (TBD)
          FieldName("top_revision_index_block")
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









file_snap_database_table::file_snap_database_table(dbfile::pointer_t f, reference_t offset)
    : block(g_descriptions_by_version, f, offset)
{
}


version_t file_snap_database_table::get_file_version() const
{
    return static_cast<version_t>(static_cast<uint32_t>(f_structure->get_uinteger("file_version")));
}


void file_snap_database_table::set_file_version(version_t v)
{
    f_structure->set_uinteger("file_version", v.to_binary());
}


uint32_t file_snap_database_table::get_block_size() const
{
    return static_cast<reference_t>(f_structure->get_uinteger("block_size"));
}


void file_snap_database_table::set_block_size(uint32_t size)
{
    f_structure->set_uinteger("block_size", size);
}


reference_t file_snap_database_table::get_table_definition() const
{
    return static_cast<reference_t>(f_structure->get_uinteger("table_definition"));
}


void file_snap_database_table::set_table_definition(reference_t offset)
{
    f_structure->set_uinteger("table_definition", offset);
}


reference_t file_snap_database_table::get_first_free_block() const
{
    return static_cast<reference_t>(f_structure->get_uinteger("first_free_block"));
}


void file_snap_database_table::set_first_free_block(reference_t offset)
{
    f_structure->set_uinteger("first_free_block", offset);
}


reference_t file_snap_database_table::get_indirect_index() const
{
    return static_cast<reference_t>(f_structure->get_uinteger("indirect_index"));
}


void file_snap_database_table::set_indirect_index(reference_t reference)
{
    f_structure->set_uinteger("indirect_index", reference);
}


oid_t file_snap_database_table::get_last_oid() const
{
    return static_cast<oid_t>(f_structure->get_uinteger("last_oid"));
}


void file_snap_database_table::set_last_oid(oid_t oid)
{
    f_structure->set_uinteger("last_oid", oid);
}


oid_t file_snap_database_table::get_first_free_oid() const
{
    return static_cast<oid_t>(f_structure->get_uinteger("first_free_oid"));
}


void file_snap_database_table::set_first_free_oid(oid_t oid)
{
    f_structure->set_uinteger("first_free_oid", oid);
}


oid_t file_snap_database_table::get_update_last_oid() const
{
    return static_cast<oid_t>(f_structure->get_uinteger("update_last_oid"));
}


void file_snap_database_table::set_update_last_oid(oid_t oid)
{
    f_structure->set_uinteger("update_last_oid", oid);
}


oid_t file_snap_database_table::get_update_oid() const
{
    return static_cast<oid_t>(f_structure->get_uinteger("update_oid"));
}


void file_snap_database_table::set_update_oid(oid_t oid)
{
    f_structure->set_uinteger("update_oid", oid);
}


reference_t file_snap_database_table::get_blobs_with_free_space() const
{
    return static_cast<reference_t>(f_structure->get_uinteger("blobs_with_free_space"));
}


void file_snap_database_table::set_blobs_with_free_space(reference_t reference)
{
    f_structure->set_uinteger("blobs_with_free_space", reference);
}


reference_t file_snap_database_table::get_first_compactable_block() const
{
    return static_cast<reference_t>(f_structure->get_uinteger("first_compactable_block"));
}


void file_snap_database_table::set_first_compactable_block(reference_t reference)
{
    f_structure->set_uinteger("first_compactable_block", reference);
}


reference_t file_snap_database_table::get_primary_index_block() const
{
    return static_cast<reference_t>(f_structure->get_uinteger("primary_index_block"));
}


void file_snap_database_table::set_primary_index_block(reference_t reference)
{
    f_structure->set_uinteger("primary_index_block", reference);
}


reference_t file_snap_database_table::get_primary_index_reference_zero() const
{
    return static_cast<reference_t>(f_structure->get_uinteger("primary_index_reference_zero"));
}


void file_snap_database_table::set_primary_index_reference_zero(reference_t reference)
{
    f_structure->set_uinteger("primary_index_reference_zero", reference);
}


reference_t file_snap_database_table::get_expiration_index_block() const
{
    return static_cast<reference_t>(f_structure->get_uinteger("expiration_index_block"));
}


void file_snap_database_table::set_expiration_index_block(reference_t reference)
{
    f_structure->set_uinteger("expiration_index_block", reference);
}


reference_t file_snap_database_table::get_secondary_index_block() const
{
    return static_cast<reference_t>(f_structure->get_uinteger("secondary_index_block"));
}


void file_snap_database_table::set_secondary_index_block(reference_t reference)
{
    f_structure->set_uinteger("secondary_index_block", reference);
}


reference_t file_snap_database_table::get_tree_index_block() const
{
    return static_cast<reference_t>(f_structure->get_uinteger("tree_index_block"));
}


void file_snap_database_table::set_tree_index_block(reference_t reference)
{
    f_structure->set_uinteger("tree_index_block", reference);
}


reference_t file_snap_database_table::get_deleted_rows() const
{
    return static_cast<reference_t>(f_structure->get_uinteger("deleted_rows"));
}


void file_snap_database_table::set_deleted_rows(reference_t reference)
{
    f_structure->set_uinteger("deleted_rows", reference);
}


reference_t file_snap_database_table::get_bloom_filter_flags() const
{
    return static_cast<reference_t>(f_structure->get_uinteger("bloom_filter_flags"));
}


void file_snap_database_table::set_bloom_filter_flags(flags_t flags)
{
    f_structure->set_uinteger("bloom_filter_flags", flags);
}



} // namespace snapdatabase
// vim: ts=4 sw=4 et
