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
 * \brief This block is used to define secondary indexes.
 *
 * Our database model allows for any number of indexes to be defined on
 * each table. This is quite practical because it is always going to be
 * a lot faster to have the low level system to handle the sorting of
 * your data.
 *
 * Secondary indexes are defined in the schema, but they require their
 * own blocks to actually generate the indexes. The entries make use
 * of your data as the index key. The key generation can make use of
 * C-like computations (i.e. just like an SQL `WHERE` can make use
 * of expressions to filter your data, although on our end we use this
 * feature to also sort the data).
 */

// self
//
#include    "snapdatabase/data/structure.h"





namespace snapdatabase
{



class block_secondary_index
    : public block
{
public:
    typedef std::shared_ptr<block_secondary_index>       pointer_t;

                                block_secondary_index(dbfile::pointer_t f, reference_t offset);

    uint32_t                    get_id() const;
    void                        set_id(uint32_t id);
    uint64_t                    get_number_of_rows() const;
    void                        set_number_of_rows(uint64_t count);
    reference_t                 get_top_index() const;
    void                        set_top_index(reference_t offset);
    uint32_t                    get_bloom_filter_flags() const;
    void                        set_bloom_filter_flags(uint32_t flags);

private:
};



} // namespace snapdatabase
// vim: ts=4 sw=4 et
