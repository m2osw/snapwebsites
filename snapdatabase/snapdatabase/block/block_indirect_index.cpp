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
#include    "snapdatabase/block/block_indirect_index.h"

#include    "snapdatabase/block/block_header.h"
#include    "snapdatabase/block/block_top_indirect_index.h"
#include    "snapdatabase/database/table.h"


// C++ lib
//
#include    <iostream>


// last include
//
#include    <snapdev/poison.h>



namespace snapdatabase
{



namespace
{



// 'INDR' -- indirect index
struct_description_t g_description[] =
{
    define_description(
          FieldName("header")
        , FieldType(struct_type_t::STRUCT_TYPE_STRUCTURE)
        , FieldSubDescription(detail::g_block_header)
    ),
    // we assume that the entire block is used, by default the unused OIDs
    // are null (0); that's all but the array is always considered full and
    // aligned (there is no need to limit the array to a smaller size)
    //define_description(
    //      FieldName("size")
    //    , FieldType(the struct_type_t::STRUCT_TYPE_ARRAY32 data)
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



block_indirect_index::block_indirect_index(dbfile::pointer_t f, reference_t offset)
    : block(g_descriptions_by_version, f, offset)
{
}


size_t block_indirect_index::get_start_offset()
{
    structure::pointer_t structure(std::make_shared<structure>(g_description));
    return round_up(structure->get_size(), sizeof(reference_t));
}


size_t block_indirect_index::get_max_count() const
{
    if(f_count == 0)
    {
        // the structure size should be known at compile time, however the
        // page size can be different for various tables
        //
        // it is very important for all the blocks to have the exact same
        // size so I use the std::max() of this block and the `TIND` block
        //
        // WARNING: if the size of that structure changes, then an existing
        //          database may not be compatible at all anymore
        //
        f_start_offset = std::max(round_up(f_structure->get_size(), sizeof(reference_t))
                                , block_top_indirect_index::get_start_offset());
        size_t const page_size(get_table()->get_page_size());
        size_t const available_size(page_size - f_start_offset);
        f_count = available_size / sizeof(reference_t);
    }

// useful to quickly test the allocation of TIND and INDR
// but make sure to do the ssame in block_top_indirect_index::get_max_count()
//return 8;

    return f_count;
}


/** \brief Get a reference.
 *
 * \warning
 * This function may return MISSING_FILE_ADDR which is different from
 * NULL_FILE_ADDR in that the position is out of bounds whereas a null
 * means that there is currently no `INDR` or `TIND` at that location.
 *
 * \exception snapdatabase_logic_error
 * If the \p id parameter is out of bounds and \p must_exist is true, then
 * this exception is raised. When \p must_exist is false, the function
 * returns MISSING_FILE_ADDR instead.
 *
 * \param[in,out] id  The identifier of the index being sought.
 * \param[in] must_exist  Whether the position defined by \p id must exist or
 * can be out of bound.
 *
 * \return The reference to the next level to the `DATA` block with this row's
 * data, or `NULL_FILE_ADDR`, or `MISSING_FILE_ADDR`.
 */
reference_t block_indirect_index::get_reference(oid_t & id, bool must_exist) const
{
    reference_t const * refs(nullptr);
    std::uint64_t const position(get_position(id, refs));
    if(refs == nullptr)
    {
        if(must_exist)
        {
            throw snapdatabase_logic_error("somehow an Indirect Index position is out of bounds.");
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
 * \exception snapdatabase_logic_error
 * If the \p id parameter is out of bounds, this exception is raised.
 * Contrary to the get_reference() which may return MISSING_FILE_ADDR
 * instead.
 *
 * \param[in,out] id  The object identifier to search for.
 */
void block_indirect_index::set_reference(oid_t & id, reference_t offset)
{
    reference_t const * refs(nullptr);
    std::uint64_t const position(get_position(id, refs));
    if(refs == nullptr)
    {
        throw snapdatabase_logic_error("somehow an Indirect Index position is out of bounds.");
    }
    const_cast<reference_t *>(refs)[position] = offset;
}


std::uint64_t block_indirect_index::get_position(oid_t id, reference_t const * & refs) const
{
    std::uint64_t const position(static_cast<std::uint64_t>(id - 1));
    if(position >= get_max_count())
    {
        refs = nullptr;
        return 0;
    }

    refs = reinterpret_cast<reference_t const *>(data(f_start_offset));
    return position;
}





} // namespace snapdatabase
// vim: ts=4 sw=4 et
