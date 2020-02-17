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
#include    "snapdatabase/block/block_data.h"

#include    "snapdatabase/block/block_header.h"
#include    "snapdatabase/database/table.h"


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


// 'DATA'
constexpr struct_description_t const g_description[] =
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


block_data::block_data(dbfile::pointer_t f, reference_t offset)
    : block(g_descriptions_by_version, f, offset)
{
}


std::uint8_t * block_data::data_start()
{
    return data() + HEADER_SIZE;
}


std::uint32_t block_data::block_total_space(table::pointer_t t)
{
    return t->get_page_size() - HEADER_SIZE;
}



} // namespace snapdatabase
// vim: ts=4 sw=4 et
