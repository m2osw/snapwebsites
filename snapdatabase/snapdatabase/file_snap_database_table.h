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
//#include    "snapdatabase/dbfile.h"





namespace snapdatabase
{



/** \brief The type of Bloom Filter.
 *
 * We want to support multiple implementations to help with the ignorance
 * of what is best.
 *
 * * None
 *
 *     Means that no Bloom Filter is used (good for _tiny_ tables).
 *
 * * One
 *
 *     Means that we use a single buffer for all the hashes. That means
 *     we may have some overlap (although this is how it usually is
 *     implemented).
 *
 * * N
 *
 *     Means we use one buffer per hash. No overlap, but instead of
 *     a one time 250Kb buffer, we need something like N x 250Kb
 *     (where N is the number of hashes).
 *
 *     Keep in mind that N is generally pretty large (i.e. 7 to 23).
 *     So it's not cheap.
 *
 * * Bits
 *
 *     Means that the filter is just bits: 0 no luck, 1 row exists.
 *
 *     As a result, this Bloom Filters are not good with tables where
 *     many deletion occur because ultimately you get so many false
 *     positives that the filter could just be ignored. To fix the
 *     problem you have to regenerate the Bloom Filter from scratch.
 *
 * * Counters
 *
 *     Means that we use 8 bits and count how many rows make use
 *     of that bit. That way we can decrement the counter later when
 *     the row gets deleted. So this is best for tables that have many
 *     deletes.
 *
 *     Note that if the counter reaches the maximum (255 for us since we
 *     plane to use 8 bits for each counter), you have a similar problem
 *     as with the Bits version above. You have to reference the
 *     entire filter with a large Bloom Filter.
 */
enum class bloom_filter_algorithm_t : uint8_t
{
    BLOOM_FILTER_ALGORITHM_NONE          = 0,
    BLOOM_FILTER_ALGORITHM_ONE_BITS      = 1,
    BLOOM_FILTER_ALGORITHM_ONE_COUNTERS  = 2,
    BLOOM_FILTER_ALGORITHM_N_BITS        = 3,
    BLOOM_FILTER_ALGORITHM_N_COUNTERS    = 4,
};



class file_snap_database_table
    : public block
{
public:
    typedef std::shared_ptr<file_snap_database_table>       pointer_t;

                                file_snap_database_table(dbfile::pointer_t f, file_addr_t offset);

    file_addr_t                 get_first_free_block();
    void                        set_first_free_block(file_addr_t offset);

private:
    structure                   f_structure = structure();
    schema_table::pointer_t     f_schema = schema_table::pointer_t();
};



} // namespace snapdatabase
// vim: ts=4 sw=4 et
