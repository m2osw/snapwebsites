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



/** \brief The type of Bloom Filter.
 *
 * We want to support multiple implementations to help with the ignorance
 * of what is best.
 *
 * Note that a useful Bloom Filter needs to have a size of at least about
 * 8 times larger than the total number of rows in your table. So they do
 * tend to get pretty large. A table that grows to 1 million rows requires
 * 8 Mb of data. If we use `N` buffers, then you get a number around 56 Mb
 * to 184 Mb of data. Also, growing the size of the Bloom Filter requires
 * us to recalculate all of the hashes for all the rows.
 *
 * * None
 *
 *     Means that no Bloom Filter is used (good for _tiny_ tables--here tiny
 *     means a size such that all the OIDs can fit in one block or even two
 *     levels: about 250,000 rows with 4Kb blocks)
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
 *     many deletions occur because ultimately you get so many false
 *     positives that the filter could just be ignored. To fix the
 *     problem you have to regenerate the Bloom Filter from scratch.
 *
 * * Counters
 *
 *     Means that we use 8 bits and count how many rows make use
 *     of that hash. That way we can decrement the counter later when
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

                                file_snap_database_table(dbfile::pointer_t f, reference_t offset);

    version_t                   get_file_version() const;
    void                        set_file_version(version_t v);
    uint32_t                    get_block_size() const;
    void                        set_block_size(uint32_t size);
    reference_t                 get_table_definition() const;
    void                        set_table_definition(reference_t offset);
    reference_t                 get_first_free_block() const;
    void                        set_first_free_block(reference_t offset);
    reference_t                 get_indirect_index() const;
    void                        set_indirect_index(reference_t offset);
    oid_t                       get_last_oid() const;
    void                        set_last_oid(oid_t oid);
    oid_t                       get_first_free_oid() const;
    void                        set_first_free_oid(oid_t oid);
    oid_t                       get_update_last_oid() const;
    void                        set_update_last_oid(oid_t oid);
    oid_t                       get_update_oid() const;
    void                        set_update_oid(oid_t oid);
    reference_t                 get_blobs_with_free_space() const;
    void                        set_blobs_with_free_space(reference_t reference);
    reference_t                 get_first_compactable_block() const;
    void                        set_first_compactable_block(reference_t reference);
    reference_t                 get_primary_index_block() const;
    void                        set_primary_index_block(reference_t reference);
    reference_t                 get_primary_index_reference_zero() const;
    void                        set_primary_index_reference_zero(reference_t reference);
    reference_t                 get_expiration_index_block() const;
    void                        set_expiration_index_block(reference_t reference);
    reference_t                 get_secondary_index_block() const;
    void                        set_secondary_index_block(reference_t reference);
    reference_t                 get_tree_index_block() const;
    void                        set_tree_index_block(reference_t reference);
    reference_t                 get_deleted_rows() const;
    void                        set_deleted_rows(reference_t reference);
    reference_t                 get_bloom_filter_flags() const;
    void                        set_bloom_filter_flags(flags_t flags);

private:
    //schema_table::pointer_t     f_schema = schema_table::pointer_t();
};



} // namespace snapdatabase
// vim: ts=4 sw=4 et
