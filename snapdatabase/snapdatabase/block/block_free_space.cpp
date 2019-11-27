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
 * \brief Database file implementation.
 *
 * Each table uses one or more files. Each file is handled by a dbfile
 * object and a corresponding set of blocks.
 */

// self
//
#include    "snapdatabase/block/block_free_space.h"

#include    "snapdatabase/block/block_data.h"
#include    "snapdatabase/data/dbfile.h"
#include    "snapdatabase/data/dbtype.h"
#include    "snapdatabase/database/table.h"

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
 * Right now the header of this block is just the magic word. If for some
 * reasons we need to add more information, we want to easily be able to
 * adjust the offset.
 *
 * At this time, there is no need so it is zero.
 *
 * \note
 * It is zero because we do not support an offset at position 0 (because
 * a size of 0 cannot be allocated).
 */
constexpr uint64_t          FREE_SPACE_OFFSET   = 0;


/** \brief Avoid small allocation wasting space.
 *
 * This parameter is used in an attempt to avoid wasting too much space
 * when allocating a new block. Small blocks are likely to be allocated
 * anyway so we leave them behind and start using blocks having space
 * for `FREE_SPACE_JUMP` references.
 */
constexpr uint64_t          FREE_SPACE_JUMP     = sizeof(reference_t) * 32;



// we can use bits 0 to 7 for our free space flags
//
// bits 8 to 31 are used by the other blocks (mainly the DATA block,
// maybe the BLOB too?) these other bits are defined in how header since
// they need to be accessible from te outside
//
constexpr uint32_t          FREE_SPACE_FLAG_ALLOCATED = 0x01;

struct free_space_meta_t
{
    uint32_t            f_size = 0;
    uint32_t            f_flags = 0;
};


static_assert(sizeof(free_space_meta_t) == sizeof(reference_t)
            , "the free_space_meta_t structure must be exactly equal to one reference_t in size");


struct free_space_link_t
{
    free_space_meta_t   f_meta = free_space_meta_t();
    reference_t         f_next = 0;
    reference_t         f_previous = 0;
};


static_assert(sizeof(free_space_link_t) % sizeof(reference_t) == 0
            , "the free_soace_link_t structure must have a size which is a multiple of sizeof(reference_t)");


constexpr uint32_t      FREE_SPACE_SIZE_MULTIPLE = sizeof(reference_t) * 4;

static_assert(FREE_SPACE_SIZE_MULTIPLE >= sizeof(free_space_link_t)
            , "FREE_SPACE_SIZE_MULTIPLE must be at least equal to sizeof(fre_space_link_t)");



// 'FSPC'
constexpr struct_description_t g_free_space_description[] =
{
    define_description(
          FieldName("magic")    // dbtype_t = FSPC
        , FieldType(struct_type_t::STRUCT_TYPE_UINT32)
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



class block_free_space_impl
{
public:
                                block_free_space_impl(block & b);
                                block_free_space_impl(block_free_space_impl const & rhs) = delete;

    block_free_space_impl       operator = (block_free_space_impl const & rhs) = delete;

    free_space_t                get_free_space(uint32_t minimum_size);
    void                        release_space(reference_t offset);

private:
    reference_t *               get_free_space_pointer(uint32_t size);
    void                        link_space(reference_t link_offset, free_space_link_t * link);
    free_space_link_t *         get_link(reference_t r);
    void                        unlink_space(free_space_link_t * link);
    uint32_t                    total_space_available_in_one_data_block();

    block &                     f_block;
};



block_free_space_impl::block_free_space_impl(block & b)
    : f_block(b)
{
}


reference_t * block_free_space_impl::get_free_space_pointer(uint32_t size)
{
    if(size == 0)
    {
        throw snapdatabase_logic_error("You cannot call get_free_space_pointer() with a size of 0.");
    }

    size += FREE_SPACE_SIZE_MULTIPLE - 1;
    size /= FREE_SPACE_SIZE_MULTIPLE;

    return reinterpret_cast<reference_t *>(f_block.data() + FREE_SPACE_OFFSET) + size;
}


void block_free_space_impl::link_space(reference_t link_offset, free_space_link_t * link)
{
    reference_t * d(get_free_space_pointer(link->f_meta.f_size));

    link->f_meta.f_flags &= ~FREE_SPACE_FLAG_ALLOCATED;
    link->f_next = *d;
    link->f_previous = 0;
    *d = link_offset;
}


free_space_link_t * block_free_space_impl::get_link(reference_t r)
{
    if(r == 0)
    {
        throw snapdatabase_logic_error("You cannot call get_link() with a reference of 0.");
    }

    block::pointer_t b(f_block.get_table()->get_block(r));

    return reinterpret_cast<free_space_link_t *>(b->data(r));
}


void block_free_space_impl::unlink_space(free_space_link_t * link)
{
    if(link->f_previous != 0)
    {
        free_space_link_t * p(get_link(link->f_previous));
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

    if(link->f_next != 0)
    {
        free_space_link_t * n(get_link(link->f_next));
        n->f_previous = link->f_previous;
    }

    // this is not needed we're about to smash that data anyway and it is
    // safe (in case it did not get overwritten, it's not secret information)
    //
    //link->f_next = 0;
    //link->f_previous = 0;
}


uint32_t block_free_space_impl::total_space_available_in_one_data_block()
{
    static uint32_t total_space(0);
    
    if(total_space == 0)
    {
        total_space = block_data::block_total_space(f_block.get_table());
        total_space -= total_space % sizeof(reference_t);
    }

    return total_space;
}


free_space_t block_free_space_impl::get_free_space(uint32_t minimum_size)
{
    // we always keep the size & flags
    //
    // (i.e. we return the free space reference + sizeof(free_space_meta_t)
    // so we have to allocate that much more space; that's a loss of about
    // 4Mb/1,000,000 rows)
    //
    minimum_size += sizeof(free_space_meta_t);

    // make the size a multiple of free_space_link_t rounded up
    //
    minimum_size += sizeof(free_space_link_t) - 1;
    minimum_size -= minimum_size % sizeof(free_space_link_t);

    uint32_t const total_space(total_space_available_in_one_data_block());
    if(minimum_size > total_space)
    {
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

    if(result.f_reference != 0)
    {
        // we got an extact match! remove that space from the list
        //
        unlink_space(get_link(result.f_reference));
        result.f_reference += sizeof(free_space_meta_t);
        result.f_block = f_block.get_table()->get_block(result.f_reference);
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

    // the linked list is sizeof(free_space_link_t) which is why we divide
    // the get_page_size() by sizeof(free_space_link_t)
    //
    // TODO: adjust the maximum with the `DATA` header subtracted since
    //       we cannot ever allocate an entire block; it may save us one
    //       iteration on each allocation attempt?
    //
    reference_t * end(get_free_space_pointer(f_block.get_table()->get_page_size()));

    // search for a large free space and allocate the buffer you are
    // asking for
    //
    while(d < end)
    {
        result.f_reference = *reinterpret_cast<reference_t *>(d);
        if(result.f_reference != 0)
        {
            free_space_link_t * link(get_link(result.f_reference));
            unlink_space(link);

            // enough space to break free space in two?
            //
            uint32_t new_size(link->f_meta.f_size - minimum_size);
            if(new_size >= sizeof(free_space_meta_t))
            {
                data_t data(reinterpret_cast<data_t>(link));
                free_space_link_t * new_link(reinterpret_cast<free_space_link_t *>(data + minimum_size));
                new_link->f_meta.f_size = link->f_meta.f_size - minimum_size;
                link->f_meta.f_size = minimum_size;
                new_link->f_meta.f_flags = 0;
                link_space(result.f_reference + minimum_size, new_link);
            }
            else
            {
                result.f_size = link->f_meta.f_size;
            }

            link->f_meta.f_flags |= FREE_SPACE_FLAG_ALLOCATED;

            result.f_reference += sizeof(free_space_meta_t);
            result.f_block = f_block.get_table()->get_block(result.f_reference);
            return result;
        }
    }

    // no space available, we have to allocate a new `DATA` block
    //
    {
        block::pointer_t data_block(f_block.get_table()->allocate_new_block(dbtype_t::BLOCK_TYPE_DATA));

        // the following skips the header and aligns "start" to the next
        // sizeof(reference_t) boundary
        //
        reference_t start(f_block.get_table()->get_page_size() - total_space);
        result.f_reference = start;
        free_space_link_t * link(reinterpret_cast<free_space_link_t *>(data_block->data() + start));
        start += minimum_size;
        uint32_t const remaining_size(total_space - minimum_size);
        if(remaining_size >= sizeof(free_space_meta_t))
        {
            free_space_link_t * new_link(reinterpret_cast<free_space_link_t *>(data_block->data() + start));
            new_link->f_meta.f_size = remaining_size;
            new_link->f_meta.f_flags = 0;
            link_space(data_block->get_offset() + start, new_link);

            link->f_meta.f_size = total_space - remaining_size;
        }
        else
        {
            link->f_meta.f_size = total_space;
        }

        link->f_meta.f_flags = FREE_SPACE_FLAG_ALLOCATED;

        result.f_reference += sizeof(free_space_meta_t);
        result.f_block = f_block.get_table()->get_block(result.f_reference);
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
            ;
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





block_free_space::block_free_space(dbfile::pointer_t f, reference_t offset)
    : block(f, offset)
    , f_impl(std::make_unique<detail::block_free_space_impl>(*this))
{
    f_structure = std::make_shared<structure>(detail::g_free_space_description);
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


bool block_free_space::get_flag(data_t ptr, uint32_t flag)
{
    detail::free_space_meta_t * meta(reinterpret_cast<detail::free_space_meta_t *>(ptr) - 1);
    return (meta->f_flags & flag) != 0;
}


void block_free_space::set_flag(data_t ptr, uint32_t flag)
{
    detail::free_space_meta_t * meta(reinterpret_cast<detail::free_space_meta_t *>(ptr) - 1);
    meta->f_flags |= flag;
}


void block_free_space::clear_flag(data_t ptr, uint32_t flag)
{
    detail::free_space_meta_t * meta(reinterpret_cast<detail::free_space_meta_t *>(ptr) - 1);
    meta->f_flags &= ~flag;
}



} // namespace snapdatabase
// vim: ts=4 sw=4 et
