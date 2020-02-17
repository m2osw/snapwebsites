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
 * \brief Index Pointer block implementation.
 *
 * In a secondary index, one key match may not be unique. When that happens,
 * the list of rows that match the secondary index is listed in an
 * Index Pointer block. The address in the `EIDX` points to an array of
 * a list of pointers (`oid_t`, really).
 *
 * \todo
 * Determine how to properly grow such lists because that's not too easy
 * in the way it is defined now.
 */

// self
//
#include    "snapdatabase/block/block_index_pointers.h"

#include    "snapdatabase/block/block_header.h"


// last include
//
#include    <snapdev/poison.h>



namespace snapdatabase
{



namespace
{



// 'IDXP' -- index pointers
constexpr struct_description_t g_description[] =
{
    define_description(
          FieldName("header")
        , FieldType(struct_type_t::STRUCT_TYPE_STRUCTURE)
        , FieldSubDescription(detail::g_block_header)
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




block_index_pointers::block_index_pointers(dbfile::pointer_t f, reference_t offset)
    : block(g_descriptions_by_version, f, offset)
{
}






} // namespace snapdatabase
// vim: ts=4 sw=4 et
