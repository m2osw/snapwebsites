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
#include    "snapdatabase/data/dbfile.h"


// C++ lib
//
#include    <map>


namespace snapdatabase
{


class table;
typedef std::shared_ptr<table>      table_pointer_t;

class structure;
typedef std::shared_ptr<structure>  structure_pointer_t;


class block
    : public std::enable_shared_from_this<block>
{
public:
    typedef std::shared_ptr<block>              pointer_t;
    typedef std::map<reference_t, pointer_t>    map_t;

                                block(block const & rhs) = delete;
                                ~block();

    block &                     operator = (block const & rhs) = delete;

    //static pointer_t            create_block(dbfile::pointer_t f, reference_t offset);
    //static pointer_t            create_block(dbfile::pointer_t f, dbtype_t type);

    table_pointer_t             get_table() const;
    void                        set_table(table_pointer_t table);
    structure_pointer_t         get_structure() const;

    dbtype_t                    get_dbtype() const;
    void                        set_dbtype(dbtype_t type);
    reference_t                 get_offset() const;
    void                        set_data(data_t data);
    data_t                      data(reference_t offset = 0);
    const_data_t                data(reference_t offset = 0) const;
    void                        sync(bool immediate);

protected:
                                block(dbfile::pointer_t f, reference_t offset);

    table_pointer_t             f_table = table_pointer_t();
    dbfile::pointer_t           f_file = dbfile::pointer_t();
    structure_pointer_t         f_structure = structure_pointer_t();
    reference_t                 f_offset = reference_t();

    dbtype_t                    f_dbtype = dbtype_t::BLOCK_TYPE_FREE_BLOCK;
    mutable data_t              f_data = nullptr;
};



} // namespace snapdatabase
// vim: ts=4 sw=4 et
