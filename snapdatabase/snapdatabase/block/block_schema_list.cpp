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
 * \brief Schema list implementation.
 *
 * When a table schema is modified, we want to keep track of old schema until
 * all the rows are known to have been converted to the new format (which is
 * easy to implement using the indirect index).
 *
 * This is when the schema list gets used. The table definition will point
 * to this `SCHL` block instead of directly to the `SCHM` block. The block
 * holds up to 310 references to other schema. If more change requests
 * happen before the whole table can be updated, an error occurs since we
 * just can't support more than 310 schemata at one time. In that situation,
 * the latest schema defined in your XML file is ignored until the update
 * is complete and the `SCHL` block gets removed.
 *
 * \todo
 * Actually fully implement this functionality. Also look into having proper
 * OID management for the background process to run the update and measure
 * the load to know when to let for more important tasks (such as INSERT,
 * SELECT, DELETE commands instead of updates to the newer schema.)
 */

// self
//
#include    "snapdatabase/block/block_schema_list.h"

#include    "snapdatabase/block/block_header.h"
#include    "snapdatabase/block/block_schema.h"
#include    "snapdatabase/database/table.h"


// C++ lib
//
#include    <iostream>


// last include
//
#include    <snapdev/poison.h>



namespace snapdatabase
{


// We don't want to do that because we want to support a binary search
//
//struct_description_t g_array_description[] =
//{
//    define_description(
//          FieldName("version")
//        , FieldType(struct_type_t::STRUCT_TYPE_VERSION)
//    ),
//    define_description(
//          FieldName("reference")
//        , FieldType(struct_type_t::STRUCT_TYPE_REFERENCE)
//    ),
//};



namespace
{



// 'SCHL' -- schema list
constexpr struct_description_t g_description[] =
{
    define_description(
          FieldName("header")
        , FieldType(struct_type_t::STRUCT_TYPE_STRUCTURE)
        , FieldSubDescription(detail::g_block_header)
    ),
    define_description(
          FieldName("count")
        , FieldType(struct_type_t::STRUCT_TYPE_UINT16)
    ),
    //define_description(
    //      FieldName("schemata")
    //    , FieldType(struct_type_t::STRUCT_TYPE_ARRAY16)
    //    , FieldDescription(g_array_description)
    //),
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




block_schema_list::block_schema_list(dbfile::pointer_t f, reference_t offset)
    : block(g_descriptions_by_version, f, offset)
{
}


std::uint32_t block_schema_list::get_count() const
{
    return static_cast<std::uint32_t>(f_structure->get_uinteger("count"));
}


void block_schema_list::set_count(std::uint32_t id)
{
    f_structure->set_uinteger("count", id);
}


reference_t block_schema_list::get_schema(version_t const & version) const
{
    // make sure we have valid data
    //
    std::uint32_t const count(get_count());
    if(count < 2)
    {
        throw snapdatabase_logic_error(
                  "block_schema_list::get_schema() has a count of "
                + std::to_string(count)
                + ", which is too small (expected at least 2).");
    }

    size_t const offset(f_structure->get_size());
    std::uint8_t const * buffer(data() + offset);

    // when requesting with version (0, 0), we return the most current
    // schema which happens to be the very first one
    //
    if(version == version_t())
    {
        // we use a memcpy() because the reference is very likely unaligned
        //
        reference_t result;
        memcpy(&result, buffer + sizeof(std::uint32_t), sizeof(result));
        return result;
    }

    // the following uses a binary search, however, the data is an array of
    // `version_t` + `reference_t` which are not aligned (4 + 8 bytes)
    // hence the "weird" offset calculation and memcmp()
    //
    std::uint32_t const version_number(version.to_binary());
    int i(0);
    int j(count);
    while(i < j)
    {
        int const p((j - i) / 2 + i);
        uint8_t const * ptr(buffer + p * (sizeof(version_number) + sizeof(reference_t)));
        int const r(memcmp(ptr, &version_number, sizeof(version_number)));
        if(r > 0)       // WARNING: reversed, the table is sorted DESC
        {
            i = p + 1;
        }
        else if(r < 0)  // WARNING: reversed, the table is sorted DESC
        {
            j = p;
        }
        else
        {
            // found it
            //
            // we use a memcpy() because the reference is very likely unaligned
            //
            reference_t result;
            memcpy(&result, ptr + sizeof(version_number), sizeof(result));
            return result;
        }
    }

    throw schema_not_found(
              "schema with version "
            + version.to_string()
            + " was not found in this table.");
}


void block_schema_list::add_schema(schema_table::pointer_t schema)
{
// TODO: implement
//
// 1. verify that there is space in table
// 2. insert at correct location (it should always be at the start
//    since a new schema is going to have a larger version than any
//    existing schemata--maybe we will set the version here)
// 3. increment counter by one
//
    // make sure we have a valid version (0.0 is considered invalid here)
    // note that by default a schema is assigned version 1.0
    //
    if(schema->schema_version() == version_t())
    {
        throw snapdatabase_logic_error("the add_schema() can't be called with an unversioned schema.");
    }

    // make sure yet another schema can be added
    //
    std::uint32_t const count(get_count());
    size_t const offset(f_structure->get_size());
    size_t const page_size(get_table()->get_page_size());
    size_t const available_size(page_size - offset);
    size_t const max_count(available_size / (sizeof(std::uint32_t) + sizeof(reference_t)));
    if(count >= max_count)
    {
        // Note: throwing here is "not very smart" since we probably want to
        //       let the database update itself and thus not die out (TODO),
        //       however, that means you would not be running with the correct
        //       schema; so a solution is to run the update as fast as possible
        //       ignoring any user requests until the table is fully updated
        //       then remove all the old schemata
        //
        //       also that algorithm (update the whole table at once) should
        //       probably run when `count == max_count`; thus before you get
        //       the overflow and that may make it easier (i.e. the current
        //       schema is managed "the usual way")
        //
        throw block_full("Schema List Block is full, you can't change the schema at the moment. Wait until all the existing rows were updated to the newer schema first.");
    }

    std::uint8_t * buffer(data() + offset);
    std::uint32_t latest_version(0);
    memcpy(&latest_version, buffer, sizeof(latest_version));
    version_t version(latest_version);
    version.next_revision();
    schema->set_schema_version(version);

    memmove(buffer + (sizeof(latest_version) + sizeof(reference_t)), buffer, count * (sizeof(latest_version) + sizeof(reference_t)));
    latest_version = version.to_binary();
    memcpy(buffer, &latest_version, sizeof(latest_version));
    reference_t schema_offset(schema->schema_offset());
    memcpy(buffer + sizeof(latest_version), &schema_offset, sizeof(offset));
}



} // namespace snapdatabase
// vim: ts=4 sw=4 et
