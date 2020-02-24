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
 * \brief Block representing the Primary Key top index.
 *
 * The primary key makes use of a first index table to cut down the size
 * of the search by roughly 1/256th.
 */

// self
//
#include    "snapdatabase/data/structure.h"



namespace snapdatabase
{



class block_primary_index
    : public block
{
public:
    typedef std::shared_ptr<block_primary_index>       pointer_t;

                                block_primary_index(dbfile::pointer_t f, reference_t offset);

    std::uint8_t                get_size() const;
    std::uint32_t               key_to_index(buffer_t key) const;

    reference_t                 get_top_index(buffer_t const & key) const;
    void                        set_top_index(buffer_t const & key, reference_t offset);

private:
};



} // namespace snapdatabase
// vim: ts=4 sw=4 et
