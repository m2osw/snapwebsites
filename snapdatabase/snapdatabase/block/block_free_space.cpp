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
 * \brief Free Space Block implementation.
 *
 * Whenever we allocate a new block of data, we are very likely to also have
 * some free space available in that block. When a row gets deleted, this is
 * free space we want to be able to reclaim. Similarly, when a row is updated
 * and doesn't fit in the same amount of space, we look at having a new free
 * space area or need to delete the row and move it to another location.
 *
 * In all those cases, we need to have a list of spaces that we can allocate
 * to put new data. This block manages that list.
 *
 * \note
 * The list is double linked so that way we can easily tweak a block in the
 * middle of the list, which may be useful when we compress a block (i.e.
 * move all the empty spaces as a one area at the end of the block.) For
 * that to work 100% we may need to change the rows too with a one byte set
 * of flags which allows us to determine whether an entry is a free block
 * or a row (that will require the rows to include a size too... yuck... or
 * we'd have to read the row to determine its size, which may be better just
 * so we can save all of that space because we'd need 32 bits just for that!)
 */

// self
//
#include    "snapdatabase/block/block_free_space.h"

#include    "snapdatabase/block/block_data.h"
#include    "snapdatabase/block/block_header.h"
#include    "snapdatabase/data/dbfile.h"
#include    "snapdatabase/data/dbtype.h"
#include    "snapdatabase/database/table.h"


// C++ lib
//
#include    <iostream>


// boost lib
//
#include    <boost/preprocessor/stringize.hpp>


// last include
//
#include    <snapdev/poison.h>



namespace snapdatabase
{




namespace detail
{

/** \brief The offset is in case the header of this block grows.
 *
 * Right now the header of this block is just the magic word and version.
 * If for some reasons we need to add more information, we want to easily
 * be able to adjust the offset.
 *
 * At this time, there is no need so it is zero.
 *
 * \note
 * It is zero because we do not support an offset at position 0 (because
 * a size of 0 cannot be allocated) and position 0 holds the magic and
 * the version (2 x 32 bits).
 */
constexpr std::uint64_t     FREE_SPACE_OFFSET   = sizeof(std::uint32_t) + sizeof(std::uint32_t);


/** \brief Avoid small allocation wasting space.
 *
 * This parameter is used in an attempt to avoid wasting too much space
 * when allocating a new block. Small blocks are likely to be allocated
 * anyway so we leave them behind and start using blocks having space
 * for `FREE_SPACE_JUMP` references.
 *
 * \note
 * This parameter is ignored when dealing with a fixed table. We don't
 * have very many of those (any?) in Snap!, but that's a much better
 * optimization because otherwise you end up wasting one whole entry
 * per DATA block.
 */
constexpr std::uint64_t     FREE_SPACE_JUMP     = sizeof(reference_t) * 32;



#define                     SPACE_BITS      24
#define                     FLAGS_BITS       8

static_assert((SPACE_BITS + FLAGS_BITS) == 32
            , "SPACE_BITS + FLAGS_BITS must be exactly 32 bits total");



// we can use bits 0 to 3 for our free space flags
//
// bits 4 to 7 are used by the other blocks (just the DATA block at this point),
// these other bits are defined in the header since they need to be accessible
// from te outside
//
constexpr std::uint32_t     FREE_SPACE_FLAG_ALLOCATED = 0x01;

/** \brief Free space meta data.
 *
 * This meta data defines (1) the size of the free or allocated block of
 * data and a few flags defining what the data represents.
 *
 * \li FREE_SPACE_FLAG_ALLOCATED -- when this flag is set, this block of
 * data is allocated. When not set, it is a free block of data.
 * \li ALLOCATED_SPACE_FLAG_MOVED -- the data was moved, the blob includes
 * the location of where the data was moved; this happens when we do not
 * update the OID in the indirect index immediately and instead just allocate
 * a new entry.
 * \li ALLOCATED_SPACE_FLAG_DELETED -- in some cases we want to delete rows
 * and keep their data (i.e. move rows to a trashcan); in those cases, we
 * use this flag.
 *
 * \note
 * Right now we support page sizes of up to 2Mb so having 24 bits for the
 * size of a block of data is enough (at this time we really only need
 * 18 of the 24, but we do not need more than 3 out of 8 bits for the flags).
 * \note
 * Also using only 32 bits for the meta data saves us 4 bytes for every
 * single row of data.
 *
 * \note
 * The "... = 0" is only available on C++20 so we'll have to uncomment that
 * once we're on Ubuntu 2020 and the compile works as expected.
 * \note
 * Also the code never creates such a structure. It only casts a pointer to
 * an existing location on disk (which we mmap() in memory). In other words,
 * we would not benefit from these initializers anyway.
 */
struct free_space_meta_t
{
    std::uint32_t       f_size : SPACE_BITS /* = 0 */;
    std::uint32_t       f_flags : FLAGS_BITS /* = 0 */;
};


static_assert(sizeof(free_space_meta_t) <= sizeof(reference_t)
            , "the free_space_meta_t structure must at most have a size equal to one reference_t in size");


struct free_space_link_t
{
    free_space_meta_t   f_meta = free_space_meta_t();
    std::uint32_t       f_padding = 0;      // currently unused
    reference_t         f_next = 0;
    reference_t         f_previous = 0;
};


static_assert(sizeof(free_space_link_t) % sizeof(reference_t) == 0
            , "the free_soace_link_t structure must have a size which is a multiple of sizeof(reference_t)");


constexpr std::uint32_t FREE_SPACE_SIZE_MULTIPLE = sizeof(reference_t) * 4;

static_assert(FREE_SPACE_SIZE_MULTIPLE >= sizeof(free_space_link_t)
            , "FREE_SPACE_SIZE_MULTIPLE must be at least equal to sizeof(fre_space_link_t)");





class block_free_space_impl
{
public:
                                block_free_space_impl(block & b);
                                block_free_space_impl(block_free_space_impl const & rhs) = delete;

    block_free_space_impl       operator = (block_free_space_impl const & rhs) = delete;

    free_space_t                get_free_space(std::uint32_t minimum_size);
    void                        release_space(reference_t offset);

private:
    reference_t *               get_free_space_pointer(std::uint32_t size);
    void                        link_space(reference_t link_offset, free_space_link_t * link);
    free_space_link_t *         get_link(reference_t r, block::pointer_t & remember_block);
    void                        unlink_space(free_space_link_t * link);
    std::uint32_t               total_space_available_in_one_data_block();

    block &                     f_block;
};



block_free_space_impl::block_free_space_impl(block & b)
    : f_block(b)
{
}


reference_t * block_free_space_impl::get_free_space_pointer(std::uint32_t size)
{
    if(size < FREE_SPACE_OFFSET)
    {
        throw snapdatabase_logic_error(
                  "You cannot call get_free_space_pointer() with a size less than "
                + std::to_string(FREE_SPACE_OFFSET)
                + ", so "
                + std::to_string(size)
                + " is too small.");
    }

    size = round_up(size, sizeof(reference_t));
    return reinterpret_cast<reference_t *>(f_block.data(size));
}


void block_free_space_impl::link_space(reference_t link_offset, free_space_link_t * link)
{
    reference_t * d(get_free_space_pointer(link->f_meta.f_size));

    link->f_meta.f_flags &= ~FREE_SPACE_FLAG_ALLOCATED;
    link->f_padding = 0;    // just in case for forward compatibility
    link->f_next = *d;
    link->f_previous = NULL_FILE_ADDR;

    if(link->f_next != NULL_FILE_ADDR)
    {
        block::pointer_t next_block;
        free_space_link_t * next_link(get_link(link->f_next, next_block));
        next_link->f_previous = link_offset;
    }

    *d = link_offset;
}


/** \brief Retrieve a pointer to the specified free_space_link_t.
 *
 * This function reads the block at the specified offset (\p r) and then
 * returns the pointer.
 *
 * The \p remember_block paramter is used to save a pointer to the block
 * at offset \p r since you must hold on that block until you are done
 * with the returned pointer. Otherwise, the data could be removed from
 * under our feet.
 *
 * \param[in] r  The reference of the free_space_link to read.
 * \param[out] remember_block  The block hold the returned pointer structure.
 *
 * \return A pointer to a free_space_link_t structure in \p remember_block.
 */
free_space_link_t * block_free_space_impl::get_link(reference_t r, block::pointer_t & remember_block)
{
    if(r == NULL_FILE_ADDR)
    {
        throw snapdatabase_logic_error("You cannot call get_link() with a reference of NULL_FILE_ADDR.");
    }

    remember_block = f_block.get_table()->get_block(r);

    return reinterpret_cast<free_space_link_t *>(remember_block->data(r));
}


void block_free_space_impl::unlink_space(free_space_link_t * link)
{
    if(link->f_previous != NULL_FILE_ADDR)
    {
        block::pointer_t previous_block;
        free_space_link_t * p(get_link(link->f_previous, previous_block));
        p->f_next = link->f_next;
    }
    else
    {
        // this is a "first link" which means this pointer appears in the
        // free space array
        //
        reference_t * d(get_free_space_pointer(link->f_meta.f_size));
        *d = link->f_next;
    }

    if(link->f_next != NULL_FILE_ADDR)
    {
        block::pointer_t next_block;
        free_space_link_t * n(get_link(link->f_next, next_block));
        n->f_previous = link->f_previous;
    }

    // this is not needed we're about to smash that data anyway and it is
    // safe (in case it did not get overwritten, it's not secret information)
    //
    //link->f_next = NULL_FILE_ADDR;
    //link->f_previous = NULL_FILE_ADDR;
}


std::uint32_t block_free_space_impl::total_space_available_in_one_data_block()
{
    static std::uint32_t total_space(0);

    if(total_space == 0)
    {
        total_space = round_down(block_data::block_total_space(f_block.get_table()), sizeof(reference_t));
    }

    return total_space;
}


/** \brief Allocate a block of data,
 *
 * This function is used to allocate space in a `DATA` block. The space
 * is to be used to save a row. The space is preceeded by flags and the
 * size of that space (32 bits each, see the free_space_meta_t structure).
 * The function returns a structure with the information of the newly
 * allocated block:
 *
 * \li f_block -- the `DATA` block where the space was just allocated
 * \li f_reference -- the reference to that block (i.e. to save as the
 * OID reference); that includes the offset to be used to get a pointer
 * of data in that `DATA` block
 * \li f_size -- the total size of the allocated buffer including this
 * structure (so larger than what can fit in the space you were returned,
 * i.e. your space is `minimum_size`, not `result.f_size`)
 *
 * \todo
 * Add support for large blocks (i.e. right now a block that's larger than
 * a `DATA` block can't be saved in the database).
 *
 * \param[in] minimum_size  The minimum number of bytes you need to allocate.
 *
 * \return A structure with the block, reference, and size.
 */
free_space_t block_free_space_impl::get_free_space(std::uint32_t minimum_size)
{
    // we always keep the size & flags
    //
    // (i.e. we return the free space reference + sizeof(free_space_meta_t)
    // so we have to allocate that much more space; that's a loss of about
    // 4Mb/1,000,000 rows)
    //
    // also make the size a multiple of free_space_link_t rounded up
    //
    minimum_size = round_up(minimum_size + sizeof(free_space_meta_t), sizeof(reference_t));

    std::uint32_t const total_space(total_space_available_in_one_data_block());
    if(minimum_size > total_space)
    {
        // TODO: add support for `BLOB` blocks for rows that are larged
        //       than what can fit in a `DATA` block
        //
        throw snapdatabase_not_yet_implemented(
                  "get_free_space() called with a minimum_size ("
                + std::to_string(minimum_size)
                + " > "
                + std::to_string(total_space)
                + ") larger than what can be allocated.");
    }
    if(minimum_size >= (1 << SPACE_BITS))
    {
        // at this time, we limit the total maximum size of a row to this
        // size; since the entire row may not be saved in one `DATA` block
        // it is not likely an issue; the previous test is the one we need
        // to work on to allow really large sizes; this size here is in
        // case a user somehow allows blocks larger than (1 << 24), which
        // is 16Mb--right now we limit such to 2Mb so we should be fine
        //
        throw snapdatabase_logic_error(
                  "get_free_space() called with a minimum_size ("
                + std::to_string(minimum_size)
                + " > "
                + std::to_string(total_space)
                + ") larger than what can be allocated.");
    }

    free_space_t result;
    result.f_size = minimum_size;

    reference_t * d(get_free_space_pointer(minimum_size));
    result.f_reference = *d;

    if(result.f_reference != NULL_FILE_ADDR)
    {
        // we got an extact match! remove that space from the list
        //
        free_space_link_t * link(get_link(result.f_reference, result.f_block));
        unlink_space(link);
        link->f_meta.f_size = minimum_size;
        link->f_meta.f_flags |= FREE_SPACE_FLAG_ALLOCATED;
        result.f_reference += sizeof(free_space_meta_t);
        return result;
    }

    // if allocating a rather small space, jump to a larger one at once
    // which allows us to keep smaller free spaces intact instead of
    // breaking them into even smaller free spaces
    //
    if(minimum_size < FREE_SPACE_JUMP)
    {
        d = get_free_space_pointer(FREE_SPACE_JUMP);
    }
    else
    {
        ++d;
    }

    // search for a larger free space to allocate the buffer
    //
    // WARNING: the page size MUST be added outside too, passing that size to
    //          the get_free_space_pointer() is equivalent to passing zero
    //          (except that zero is refused because it is considered invalid)
    //          which means we would not have a pointer at the very end
    //
    for(reference_t * end(get_free_space_pointer(f_block.get_table()->get_page_size()) + f_block.get_table()->get_page_size() / sizeof(reference_t));
        d < end;
        ++d)
    {
        result.f_reference = *d;
        if(result.f_reference != NULL_FILE_ADDR)
        {
            free_space_link_t * link(get_link(result.f_reference, result.f_block));
            unlink_space(link);

            // enough space to break this free space in two?
            //
            std::uint32_t remaining_size(link->f_meta.f_size - minimum_size);
            if(remaining_size >= sizeof(free_space_link_t))
            {
                data_t data(reinterpret_cast<data_t>(link));
                free_space_link_t * new_link(reinterpret_cast<free_space_link_t *>(data + minimum_size));
                new_link->f_meta.f_size = remaining_size;
                link->f_meta.f_size = minimum_size;
                new_link->f_meta.f_flags = 0;
                link_space(result.f_reference + minimum_size, new_link);
            }
            else
            {
                // in this case we end up returning more space
                // (although we don't expect the caller to make use of the
                // extra space, it can become useful for an update)
                //
                result.f_size = link->f_meta.f_size;
            }

            link->f_meta.f_flags |= FREE_SPACE_FLAG_ALLOCATED;

            result.f_reference += sizeof(free_space_meta_t);
            return result;
        }
    }

    // no existing space available, we have to allocate a new `DATA` block
    //
    {
        result.f_block = f_block.get_table()->allocate_new_block(dbtype_t::BLOCK_TYPE_DATA);

        // the following skips the `DATA` block header
        //
        reference_t const start(f_block.get_table()->get_page_size() - total_space);
        result.f_reference = result.f_block->get_offset() + start;

        // since we just got the block we can directly allocate the link here
        //
        free_space_link_t * link(reinterpret_cast<free_space_link_t *>(result.f_block->data(result.f_reference)));
        std::uint32_t const remaining_size(total_space - minimum_size);
        if(remaining_size >= sizeof(free_space_meta_t))
        {
            data_t data(reinterpret_cast<data_t>(link));
            free_space_link_t * new_link(reinterpret_cast<free_space_link_t *>(data + minimum_size));
            new_link->f_meta.f_size = remaining_size;
            new_link->f_meta.f_flags = 0;
            link_space(result.f_reference + minimum_size, new_link);

            link->f_meta.f_size = total_space - remaining_size;
        }
        else
        {
            link->f_meta.f_size = total_space;
        }

        link->f_meta.f_flags = FREE_SPACE_FLAG_ALLOCATED;

        result.f_reference += sizeof(free_space_meta_t);
        return result;
    }
}


void block_free_space_impl::release_space(reference_t offset)
{
    if(offset % sizeof(reference_t) != 0)
    {
        throw snapdatabase_logic_error(
                  "release_space() called with an invalid offset ("
                + std::to_string(offset)
                + "); it must be a multiple of "
                + BOOST_PP_STRINGIZE(sizeof(reference_t))
                + ".");
    }

    offset -= sizeof(free_space_meta_t);
    block::pointer_t b(f_block.get_table()->get_block(offset));
    free_space_link_t * link(reinterpret_cast<free_space_link_t *>(b->data(offset)));

    if(f_block.get_table()->is_secure())
    {
        // keep reseting the data when releasing it
        //
        memset(reinterpret_cast<data_t>(link + 1), 0, link->f_meta.f_size - sizeof(link->f_meta));
    }

    uint32_t const next_pos(link->f_meta.f_size + offset % f_block.get_table()->get_page_size());
    uint32_t const total_space(total_space_available_in_one_data_block());
    if(next_pos + sizeof(free_space_link_t) < total_space)
    {
        free_space_link_t * next_link(reinterpret_cast<free_space_link_t *>(b->data(next_pos)));
        if((next_link->f_meta.f_flags & FREE_SPACE_FLAG_ALLOCATED) != 0)
        {
            // merge the next with this one
            //
            link->f_meta.f_size += next_link->f_meta.f_size;
            unlink_space(next_link);
        }
    }

    reference_t start(f_block.get_table()->get_page_size() - total_space);
    if((link - reinterpret_cast<free_space_link_t *>(0)) % f_block.get_table()->get_page_size() > start)
    {
        free_space_link_t * previous_link(reinterpret_cast<free_space_link_t *>(b->data(start)));
        for(reference_t o(start);;)
        {
            free_space_link_t * link_after(reinterpret_cast<free_space_link_t *>(reinterpret_cast<data_t>(previous_link) + previous_link->f_meta.f_size));
            if(link_after == link)
            {
                if((previous_link->f_meta.f_flags & FREE_SPACE_FLAG_ALLOCATED) == 0)
                {
                    unlink_space(previous_link);
                    previous_link->f_meta.f_size += link->f_meta.f_size;
                    link_space(o, previous_link);

                    link = previous_link;
                }
                break;
            }
            else if(link_after > link)
            {
                break;
            }
            o += previous_link->f_meta.f_size;
            previous_link = link_after;
        }
    }

    // for the previous we need to start searching from the beginning of the
    // block; the `DATA` is an array of 
    //

    link_space(offset, link);

    // TODO: look into agglomerating multiple free spaces if immediately
    //       possible; here are the 4 possible cases
    //
    //       1. nothing before, nothing after, done
    //       2. something before, enlarge previous block
    //       3. something after, enlarge & move next block
    //       4. something before and after, enlarge previous block, drop next block
    //
    //       right now, though we do not have a good way of finding a
    //       previous block; the next block, though, should be at this
    //       block + size so that one is easy and the block header is
    //       enough to find it in our FSPC table
}



} // detail namespace



namespace
{



// 'FSPC'
constexpr struct_description_t g_description[] =
{
    define_description(
          FieldName("header")
        , FieldType(struct_type_t::STRUCT_TYPE_STRUCTURE)
        , FieldSubDescription(detail::g_block_header)
    ),
    // TBD: it may be useful to determine a minimum size larger than
    //      sizeof(free_space_link_t) for some tables and use that to
    //      make sure we don't break blocks to sizes smaller than that
    //define_description(
    //      FieldName("minimum_size")
    //    , FieldType(struct_type_t::STRUCT_TYPE_UINT32)
    //),
    // the following is an aligned array of references
    //define_description(
    //      FieldName("free_space")
    //    , FieldType(struct_type_t::STRUCT_TYPE_UINT128)
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




block_free_space::block_free_space(dbfile::pointer_t f, reference_t offset)
    : block(g_descriptions_by_version, f, offset)
    , f_impl(std::make_unique<detail::block_free_space_impl>(*this))
{
}


block_free_space::~block_free_space()
{
}


free_space_t block_free_space::get_free_space(uint32_t minimum_size)
{
    return f_impl->get_free_space(minimum_size);
}


void block_free_space::release_space(reference_t offset)
{
    return f_impl->release_space(offset);
}


bool block_free_space::get_flag(const_data_t ptr, std::uint8_t flag)
{
    detail::free_space_meta_t const * meta(reinterpret_cast<detail::free_space_meta_t const *>(ptr) - 1);
    return (meta->f_flags & flag) != 0;
}


void block_free_space::set_flag(data_t ptr, std::uint8_t flag)
{
    detail::free_space_meta_t * meta(reinterpret_cast<detail::free_space_meta_t *>(ptr) - 1);
    meta->f_flags |= flag;
}


void block_free_space::clear_flag(data_t ptr, std::uint8_t flag)
{
    detail::free_space_meta_t * meta(reinterpret_cast<detail::free_space_meta_t *>(ptr) - 1);
    meta->f_flags &= ~flag;
}


std::uint32_t block_free_space::get_size(const_data_t ptr)
{
    detail::free_space_meta_t const * meta(reinterpret_cast<detail::free_space_meta_t const *>(ptr) - 1);
    return meta->f_size;
}



} // namespace snapdatabase
// vim: ts=4 sw=4 et
