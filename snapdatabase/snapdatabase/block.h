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
#include    "snapdatabase/dbfile.h"



namespace snapdatabase
{



class block
{
public:
    typedef std::shared_ptr<block>      pointer_t;

                                block(block const & rhs) = delete;
    block &                     operator = (block const & rhs) = delete;

    static pointer_t            create_block(dbfile::pointer_t f, file_addr_t offset);
    static pointer_t            create_block(dbfile::pointer_t f, dbtype_t type);

    dbtype_t                    get_dbtype() const;
    size_t                      size() const;
    virtual_buffer::pointer_t   data();

protected:
                                block(dbfile::pointer_t f, file_addr_t offset);

    dbfile::pointer_t           f_file = dbfile::pointer_t();
    file_addr_t                 f_offset = file_addr_t();

    dbtype_t                    f_dbtype = BLOCK_TYPE_FREE_BLOCK;
    size_t                      f_size = 0;
    uint8_t *                   f_data = nullptr;
    //virtual_buffer::pointer_t   f_buffer = virtual_buffer::pointer_t();
};



} // namespace snapdatabase
// vim: ts=4 sw=4 et
