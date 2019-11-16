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
#include    "snapdatabase/file_bloom_filter.h"


// last include
//
#include    <snapdev/poison.h>



namespace snapdatabase
{



// 'BLMF'
constexpr struct_description_t g_bloom_filter_description[] =
{
    define_description(
          FieldName("magic")    // dbtype_t = BLMF
        , FieldType(struct_type_t::STRUCT_TYPE_UINT32)
    ),
    end_descriptions()
};




file_bloom_filter::file_bloom_filter(dbfile::pointer_t f, reference_t offset)
    : block(f, offset)
{
    f_structure = std::make_shared<structure>(g_bloom_filter_description);
}



} // namespace snapdatabase
// vim: ts=4 sw=4 et