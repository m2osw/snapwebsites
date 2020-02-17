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
 * \brief Block representing the top indirect index.
 *
 * Each row is assigned an Object Identifier (OID). That OID is used to
 * find the row in the table file using a straight forward index (i.e.
 * no search involved, it's an array).
 *
 * The Indirect Index blocks represent a lower level index which includes
 * the offset to the data in the file. When more rows than can fit in one
 * array are created, additional block levels are created. Those are called
 * Top Indirect Indexes and this block represents such.
 *
 * Pointers from a Top Indirect Index may pointer to other Top Indirect
 * Index blocks or to an Indirect Index block. The total number of rows
 * determines the level, however, it is not practical to use that number
 * as it can change under our feet. Instead we use a level in the Top
 * Indirect Blocks. That levels defines how many blocks are held in this
 * block and the blocks below this block.
 */

// self
//
#include    "snapdatabase/block/block_indirect_index.h"



namespace snapdatabase
{



class block_top_indirect_index
    : public block
{
public:
    typedef std::shared_ptr<block_top_indirect_index>
                                pointer_t;

                                block_top_indirect_index(dbfile::pointer_t f, reference_t offset);

    static size_t               get_start_offset();
    size_t                      get_max_count() const;

    std::uint8_t                get_block_level() const;
    void                        set_block_level(std::uint8_t block_level);

    reference_t                 get_reference(oid_t & id, bool must_exist) const;
    void                        set_reference(oid_t & id, reference_t offset);

private:
    std::uint64_t               get_position(oid_t & id, reference_t const * & refs) const;

    mutable std::uint32_t       f_start_offset = 0;
    mutable size_t              f_count = 0;
};



} // namespace snapdatabase
// vim: ts=4 sw=4 et
