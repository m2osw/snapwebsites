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
#pragma once


/** \file
 * \brief Block representing actual data.
 *
 * This block is where we save the actual data.
 */

// self
//
#include    "snapdatabase/data/schema.h"



namespace snapdatabase
{




class block_data
    : public block
{
public:
    typedef std::shared_ptr<block_data>       pointer_t;

    static constexpr std::uint32_t
                                HEADER_SIZE = round_up(sizeof(std::uint32_t) + sizeof(version_t), sizeof(reference_t));

                                block_data(dbfile::pointer_t f, reference_t offset);

    std::uint8_t *              data_start();
    static std::uint32_t        block_total_space(table_pointer_t t);

private:
    schema_table::pointer_t     f_schema = schema_table::pointer_t();
};



} // namespace snapdatabase
// vim: ts=4 sw=4 et
