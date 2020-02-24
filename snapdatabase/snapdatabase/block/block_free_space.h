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
 * \brief Block used to list data blocks with free space.
 *
 */

// self
//
#include    "snapdatabase/data/structure.h"



namespace snapdatabase
{



namespace detail
{
class block_free_space_impl;
}


// bits 0 to 3 are reserved by the block_free_space (also right now we only use 0x01)
//
constexpr std::uint8_t          ALLOCATED_SPACE_FLAG_MOVED      = 0x10;
constexpr std::uint8_t          ALLOCATED_SPACE_FLAG_DELETED    = 0x20;


struct free_space_t
{
    block::pointer_t            f_block = block::pointer_t();
    reference_t                 f_reference = 0;
    std::uint32_t               f_size = 0;
};


class block_free_space
    : public block
{
public:
    typedef std::shared_ptr<block_free_space>       pointer_t;

                                block_free_space(dbfile::pointer_t f, reference_t offset);
                                ~block_free_space();

    free_space_t                get_free_space(std::uint32_t minimum_size);
    void                        release_space(reference_t offset);

    static bool                 get_flag(const_data_t ptr, std::uint8_t flag);
    static void                 set_flag(data_t ptr, std::uint8_t flag);
    static void                 clear_flag(data_t ptr, std::uint8_t flag);
    static std::uint32_t        get_size(const_data_t ptr);

private:
    std::unique_ptr<detail::block_free_space_impl>
                                f_impl;
};



} // namespace snapdatabase
// vim: ts=4 sw=4 et
