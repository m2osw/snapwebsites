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


/** \file
 * \brief Block Primary Index implementation.
 *
 * The Primary Index is used to very quickly kill one layer in our search
 * without doing a search. Instead this index makes use of the last byte
 * of the Murmur3 hash to pick a block reference to use to do the search
 * of the data by primary key.
 *
 * In other words, this feature cuts down the search by a factor 256. To
 * search 1 million items using a binary search, you need 20 iterations
 * (assuming all one million items are in one table ready to be searched).
 *
 * When using the Primary Index, we cut down the 1 million by 256 (roughly)
 * which means we have to search about 3,900 items, which reduces the
 * binary search iterations to about 12.
 *
 * Obviously, in our case we use blocks so the search uses a B+tree and
 * it can take time to load said blocks, the number of items per block
 * defines a level which varies, etc. so the number of iterations can
 * vary wildly.
 *
 * On small tables, this step can be made optional. However, adding this
 * block later means having to rebuild the entire Primary Index.
 *
 * \todo
 * We use pages that have a size multiple of the system page size (small
 * or huge) but with the header it often breaks the possibility to use
 * the entire page. For this one (`PIDX`), it is a particularly bad hit
 * since we waste 50% of the page, which I think is really bad. I want
 * to find a way to change that which I think would be to define the
 * header in another location (or not have a header for these pages;
 * it is useful to allocate them in memory, but we can always have an
 * exception here and there, that's what software is good at...)
 */

// self
//
#include    "snapdatabase/block/block_primary_index.h"

#include    "snapdatabase/block/block_header.h"
#include    "snapdatabase/database/table.h"


// last include
//
#include    <snapdev/poison.h>



namespace snapdatabase
{





namespace
{



// 'PIDX' -- primary index
constexpr struct_description_t g_description[] =
{
    define_description(
          FieldName("header")
        , FieldType(struct_type_t::STRUCT_TYPE_STRUCTURE)
        , FieldSubDescription(detail::g_block_header)
    ),
    define_description(
          FieldName("size")
        , FieldType(struct_type_t::STRUCT_TYPE_UINT8)
    ),
    // we manage the array by hand, it's easier
    //define_description(
    //      FieldName("index")
    //    , FieldType(struct_type_t::STRUCT_TYPE_ARRAY32)
    //    , FieldDescription(g_index_description) -- this is just one STRUCT_TYPE_REFERENCE per entry
    //),
    end_descriptions()
};


constexpr descriptions_by_version_t const g_descriptions_by_version[] =
{
    define_description_by_version(
        DescriptionVersion(0, 1),
        DescriptionDescription(g_description)
    ),
    end_descriptions_by_version()
};



}
// no name namespace




block_primary_index::block_primary_index(dbfile::pointer_t f, reference_t offset)
    : block(g_descriptions_by_version, f, offset)
{
}


uint8_t block_primary_index::get_size() const
{
    return static_cast<uint32_t>(f_structure->get_uinteger("size"));
}


void block_primary_index::set_size(uint8_t size)
{
    if(size > 32)
    {
        throw out_of_bounds(
              "Size "
            + std::to_string(static_cast<int>(size))
            + " is too large for the Primary Index (PIDX).");
    }

    f_structure->set_uinteger("size", size);
}


/** \brief Assign the maximum size the current page size allows us to use.
 *
 * This function allows you to assign the maximum possible size to this
 * index. This is the best way to assign the value. The minimum is 8
 * since we limit the block size to 4Kb minimum. The maximum, assuming
 * 2Mb pages, is 20 bits.
 */
void block_primary_index::set_max_size()
{
    // calculate the maximum number of bits that can be used
    // and assign it to the block
    //
    //    floor(log(page_size - sizeof(header)) / log(2))
    //
    // the size (page_size - sizeof(header)) is further made a multiple of
    // sizeof(reference_t), just in case
    //
    size_t const page_size(f_table->get_page_size());
    std::uint32_t const header_size(round_up(f_structure->get_size(), sizeof(reference_t)));
    double bits(floor(log(static_cast<double>(page_size - header_size)) / log(2.0)));

    // the number of bits will further be clamped to the size of the key
    // but our size uses 8 bits so we want to make sure we do not have
    // an overflow; that being said, we use 32 bits numbers to do our
    // sub-key generation so we clamp to 32 bits which represents a page
    // of 4Gb which I don't think anyone would ever want to use!
    //
    if(bits > 32.0)
    {
        bits = 32.0;
    }

    set_size(static_cast<std::uint8_t>(bits));
}


reference_t block_primary_index::find_index(buffer_t key) const
{
    // retrieve `size` bits from the end of key
    //
    // note: at this time we conside that the maximum number of bits
    // is going to be 20, so we can use 32 bit numbers
    //
    std::uint8_t const size(get_size());
    std::uint32_t bytes(divide_rounded_up(size, 8));
    size_t idx(key.size());
    if(bytes > idx)
    {
        bytes = idx;
    }
    std::uint32_t shift(0);
    std::uint32_t k(0);
    while(bytes > 0)
    {
        --bytes;
        --idx;
        k |= key[idx] << shift;
        shift += 8;
    }

    std::uint32_t mask(-1);
    mask <<= size;
    k &= ~mask;

    std::uint32_t const offset(round_up(f_structure->get_size(), sizeof(reference_t)));

    std::uint8_t const * buffer(data() + offset);
    return reinterpret_cast<reference_t const *>(buffer)[k];
}



} // namespace snapdatabase
// vim: ts=4 sw=4 et
