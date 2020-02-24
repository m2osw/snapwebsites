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
 * \brief Block representing the database file header.
 *
 */

// self
//
#include    "snapdatabase/data/structure.h"



namespace snapdatabase
{


constexpr std::uint8_t      ENTRY_INDEX_FLAG_COMPLETE = 0x01;
constexpr std::uint8_t      ENTRY_INDEX_FLAG_MULTIPLE = 0x02;   // the reference is to an IDXP


class block_entry_index
    : public block
{
public:
    typedef std::shared_ptr<block_entry_index>       pointer_t;

                                block_entry_index(dbfile::pointer_t f, reference_t offset);

    std::uint32_t               get_count() const;
    void                        set_count(std::uint32_t count);
    std::uint32_t               get_size() const;
    void                        set_size(std::uint32_t size);
    void                        set_key_size(std::uint32_t size);
    reference_t                 get_next() const;
    void                        set_next(reference_t offset);
    reference_t                 get_previous() const;
    void                        set_previous(reference_t offset);

    oid_t                       find_entry(buffer_t const & key) const;
    std::uint32_t               get_position() const;
    void                        add_entry(buffer_t const & key, oid_t position_oid, std::int32_t close_position = -1);

private:
    mutable std::uint32_t       f_position = 0;
};



} // namespace snapdatabase
// vim: ts=4 sw=4 et
