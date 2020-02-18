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
 * \brief Implementation of the Top Indirect Index block.
 *
 * This file implements the functionality of the Top Indirect Index block.
 * This is a very fast index used with the OID column. It allows us to
 * easily move the data anywhere and still very quickly access it. All
 * the indexes use that OID instead of a file offset.
 *
 * The top level is an array of offset to either another top level or
 * directly an indirect index.
 */

// self
//
#include    "snapdatabase/block/block_top_indirect_index.h"

#include    "snapdatabase/block/block_header.h"
#include    "snapdatabase/block/block_indirect_index.h"
#include    "snapdatabase/database/table.h"


// C++ lib
//
#include    <iostream>


// last include
//
#include    <snapdev/poison.h>



namespace snapdatabase
{


// We don't want to do that because we'd have an extra size which is not
// useful (i.e. we consider the entire block as being the array)
//
//struct_description_t g_index_description[] =
//{
//    define_description(
//          FieldName("pointer")
//        , FieldType(struct_type_t::STRUCT_TYPE_REFERENCE)
//    ),
//};



namespace
{



// 'TIND' -- top indirect index
constexpr struct_description_t g_description[] =
{
    define_description(
          FieldName("header")
        , FieldType(struct_type_t::STRUCT_TYPE_STRUCTURE)
        , FieldSubDescription(detail::g_block_header)
    ),
    define_description(
          FieldName("block_level")
        , FieldType(struct_type_t::STRUCT_TYPE_UINT8)
    ),
    //define_description(
    //      FieldName("references")
    //    , FieldType(struct_type_t::STRUCT_TYPE_ARRAY32)
    //    , FieldDescription(g_index_description)
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




block_top_indirect_index::block_top_indirect_index(dbfile::pointer_t f, reference_t offset)
    : block(g_descriptions_by_version, f, offset)
{
}


size_t block_top_indirect_index::get_start_offset()
{
    structure::pointer_t structure(std::make_shared<structure>(g_description));
    return round_up(structure->get_size(), sizeof(reference_t));
}


size_t block_top_indirect_index::get_max_count() const
{
    if(f_count == 0)
    {
        // the structure size should be known at compile time, however the
        // page size can be different for various tables
        //
        // it is very important for all the blocks to have the exact same
        // size so I use the std::max() of this block and the `INDR` block
        //
        // WARNING: if the size of that structure changes, then an existing
        //          database may not be compatible at all anymore
        //
        f_start_offset = std::max(round_up(f_structure->get_size(), sizeof(reference_t))
                                , block_indirect_index::get_start_offset());
        size_t const page_size(get_table()->get_page_size());
        size_t const available_size(page_size - f_start_offset);
        f_count = available_size / sizeof(reference_t);
    }

// useful to quickly test the allocation of TIND and INDR
// but make sure to do the ssame in block_indirect_index::get_max_count()
//return 8;

    return f_count;
}


uint8_t block_top_indirect_index::get_block_level() const
{
    return static_cast<uint8_t>(f_structure->get_uinteger("block_level"));
}


void block_top_indirect_index::set_block_level(uint8_t level)
{
    f_structure->set_uinteger("block_level", level);
}


/** \brief This function retrieves the reference for the given OID.
 *
 * This function is used to read the reference of a specific OID
 * at this level. It updates the \p id parameter to be compatible
 * with the next level and also returns the offset to either
 * the next level (a `TIND` or `INDR` or a block of data whether the
 * row is found).
 *
 * \warning
 * The input object identifiers get updated so it is valid for the next
 * level.
 *
 * \raise snapdatabase_logic_error
 * This exception is raised if the OID represents a position out of bounds
 * compared to what is currently available in the database and the
 * \p must_exist parameter is true. Otherwise, a non-existant position
 * means the function returns the special address FILE_ADDR_MISSING.
 *
 * \param[in,out] id  The object identifier to search for.
 * \param[in] must_exist  The position must exist.
 *
 * \return The reference of another TIND or INDR block.
 */
reference_t block_top_indirect_index::get_reference(oid_t & id, bool must_exist) const
{
    reference_t const * refs(nullptr);
    std::uint64_t const position(get_position(id, refs));
    if(refs == nullptr)
    {
        if(must_exist)
        {
            throw snapdatabase_logic_error("somehow a Top Indirect Index position is out of bounds calling get_reference().");
        }
        return MISSING_FILE_ADDR;
    }

    return refs[position];
}


/** \brief This function sets the reference for the given OID.
 *
 * This function is used to update the reference at the specified OID
 * offset. It has to be a reference to the next level (another `TIND`
 * or an `INDR`) or to the data of a row.
 *
 * \warning
 * The input object identifiers get updated so it is valid for the next
 * level.
 *
 * \param[in,out] id  The object identifier to search for.
 * \param[in] offset  The reference of another TIND or INDR block to save here.
 */
void block_top_indirect_index::set_reference(oid_t & id, reference_t offset)
{
    reference_t const * refs(nullptr);
    std::uint64_t const position(get_position(id, refs));
    if(refs == nullptr)
    {
        throw snapdatabase_logic_error("somehow a Top Indirect Index position is out of bounds calling set_reference().");
    }
    const_cast<reference_t *>(refs)[position] = offset;
}


std::uint64_t block_top_indirect_index::get_position(oid_t & id, reference_t const * & refs) const
{
    size_t const count(get_max_count());
    uint8_t const level(get_block_level());

    uint64_t power(count);
    for(int l(1); l < level; ++l)
    {
        power *= count;
    }

    // the first OID is 1, the value 0 represents a "null", however, there
    // is really no need in our indirect index to support an offset of 0
    // therefore we decrement before using id and the output id is incremented
    // back
    //
    --id;
    std::uint64_t const position(id / power);
    id %= power;
    ++id;

    if(position >= count)
    {
        refs = nullptr;
        return 0;
    }

    refs = reinterpret_cast<reference_t const *>(data(f_start_offset));
    return position;
}



} // namespace snapdatabase
// vim: ts=4 sw=4 et
