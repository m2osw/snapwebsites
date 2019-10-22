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
 * \brief Base block implementation.
 *
 * The block base class handles the loading of the block in memory using
 * mmap() and gives information such as its type and location.
 */

// self
//
//#include    "snapdatabase/block.h"


// last include
//
#include    <snapdev/poison.h>



namespace snapdatabase
{


typedef uint64_t        file_addr_t;


class block
{
public:
    typedef std::shared_ptr<block>      pointer_t;

#define BLOCK_NAME(s)       ((snapdatabase::block::type_t)((s[0]<<24)|(s[1]<<16)|(s[2]<<8)|(s[3]<<0)))

    enum class type_t
    {
        BLOCK_TYPE_BLOB                 = BLOCK_NAME("BLOB"),
        BLOCK_TYPE_DATA                 = BLOCK_NAME("DATA"),
        BLOCK_TYPE_ENTRY_INDEX          = BLOCK_NAME("EIDX"),
        BLOCK_TYPE_FREE_BLOCK           = BLOCK_NAME("FREE"),
        BLOCK_TYPE_FREE_SPACE           = BLOCK_NAME("FSPC"),
        BLOCK_TYPE_INDEX_POINTERS       = BLOCK_NAME("IDXP"),
        BLOCK_TYPE_INDIRECT_INDEX       = BLOCK_NAME("INDR"),
        BLOCK_TYPE_SECONDARY_INDEX      = BLOCK_NAME("SIDX"),
        BLOCK_TYPE_SCHEMA               = BLOCK_NAME("SCHM"),
        BLOCK_TYPE_SNAP_DATABASE_TABLE  = BLOCK_NAME("SDBT"),
        BLOCK_TYPE_TOP_INDEX            = BLOCK_NAME("TIDX"),
    };

                                block(dbfile::pointer_t f);
                                block(block const & rhs) = delete;

    block &                     operator = (block const & rhs) = delete;

    type_t                      get_type() const;
    size_t                      size() const;
    virtual_buffer::pointer_t   data();

private:
    dbfile::pointer_t           f_file = dbfile::pointer_t();

    type_t                      f_type = BLOCK_TYPE_FREE_BLOCK;
    //size_t                      f_size = 0;
    //uint8_t *                   f_data = nullptr;
    virtual_buffer::pointer_t   f_buffer = virtual_buffer::pointer_t();
};



} // namespace snapdatabase
// vim: ts=4 sw=4 et
