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
 * \brief The virtual buffer implementation.
 *
 * The virtual buffer allows us to access data which is not defined in one
 * straight memory buffer but instead scattered between blocks and memory
 * buffers (when the amount of data increases we allocate temporary memory
 * buffers until we flush the data to file).
 */

// self
//
#include    "snapdatabase/virtual_buffer.h"

#include    "snapdatabase/exception.h"

// last include
//
#include    <snapdev/poison.h>



namespace snapdatabase
{



virtual_buffer::vbuf_t::vbuf_t()
{
}


virtual_buffer::vbuf_t::vbuf_t(block::pointer_t b, std::uint64_t offset, std::uint64_t size)
    : f_block(b)
    , f_offset(offset)
    , f_size(size)
{
}



virtual_buffer::virtual_buffer()
{
}


virtual_buffer::virtual_buffer(block::pointer_t b, std::uint64_t offset, std::uint64_t size)
{
    add_buffer(b, offset, size);
}


void virtual_buffer::add_buffer(block::pointer_t b, std::uint64_t offset, std::uint64_t size)
{
    if(f_modified)
    {
        throw snapdatabase_logic_error(
                "Virtual buffer was already modified, you can't add"
                " another buffer until you commit this virtual buffer.");
    }

    vbuf_t const append(b, offset, size);
    f_buffers.push_back(append);
}


bool virtual_buffer::modified() const
{
    return f_modified;
}


std::size_t virtual_buffer::count_buffers() const
{
    return f_buffers.size();
}


std::uint64_t virtual_buffer::size() const
{
    return f_total_size;
}


bool virtual_buffer::is_data_available(std::uint64_t size, std::uint64_t offset) const
{
    return offset + size <= f_total_size;
}


int virtual_buffer::pread(void * buf, std::uint64_t size, std::uint64_t offset, bool full) const
{
    if(full
    && !is_data_available(size, offset))
    {
        throw invalid_size(
                "Not enough data to process virtual buffer pread(). Read "
                + std::to_string(offset)
                + " bytes, "
                + std::to_string(size)
                + " still missing.");
    }

    std::uint64_t bytes_read(0);
    for(auto b : f_buffers)
    {
        if(offset >= b.f_size)
        {
            offset -= b.f_size;
        }
        else
        {
            auto sz(size);
            if(sz > b.f_size - offset)
            {
                sz = b.f_size - offset;
            }
            if(b.f_block != nullptr)
            {
                memcpy(buf, b.f_block->data() + b.f_offset + offset, sz);
            }
            else
            {
                memcpy(buf, b.f_data.data() + offset, sz);
            }
            size -= sz;
            bytes_read += sz;

            if(size == 0)
            {
                return bytes_read;
            }

            offset = 0;
        }
    }

    return bytes_read;
}


int virtual_buffer::pwrite(void const * buf, std::uint64_t size, std::uint64_t offset, bool allow_growth)
{
    if(!allow_growth
    && !is_data_available(offset, size))
    {
        throw invalid_size(
                "Not enough space to write to virtual buffer. Requested to write "
                + std::to_string(size)
                + " bytes at "
                + std::to_string(offset)
                + ", when the buffer is "
                + std::to_string(f_total_size)
                + " bytes only.");
    }

    std::uint8_t const * in(reinterpret_cast<std::uint8_t const *>(buf));
    std::uint64_t bytes_written(0);

    for(auto b : f_buffers)
    {
        if(offset >= b.f_size)
        {
            offset -= b.f_size;
        }
        else
        {
            auto sz(size);
            if(sz > b.f_size - offset)
            {
                sz = b.f_size - offset;
            }
            if(b.f_block != nullptr)
            {
                memcpy(b.f_block->data() + offset, in, sz);
            }
            else
            {
                memcpy(b.f_data.data() + offset, in, sz);
            }
            size -= sz;
            bytes_written += sz;

            if(size == 0)
            {
                f_modified = bytes_written != 0;
                return bytes_written;
            }

            in += sz;

            offset = 0;
        }
    }

    // this can happen if the caller calls us with an empty buffer
    //
    if(size == 0)
    {
        f_modified = bytes_written != 0;
        return bytes_written;
    }

    if(f_buffers.back().f_block == nullptr)
    {
        std::uint64_t const available(f_buffers.back().f_data.capacity() - f_buffers.back().f_size);
        if(available > 0)
        {
            auto const sz(std::min(available, size));
            f_buffers.back().f_data.resize(f_buffers.back().f_size + sz);
            memcpy(f_buffers.back().f_data.data() + f_buffers.back().f_size, in, sz);
            size -= sz;
            bytes_written += sz;
            f_total_size += sz;

            if(size == 0)
            {
                f_modified = bytes_written != 0;
                return bytes_written;
            }

            in += sz;
        }
    }

    // TBD: we may want to allocate multiple buffers of 4Kb instead of
    //      a buffer large enough for this data? At the same time, we
    //      can't save exactly 4Kb of data in the blocks anyway...
    //
    //      however, we could probably use a form of binary search to
    //      reduce the number iterations; however, after _many_ insertions
    //      it is likely that such a search would not be working very well
    //      and either way we'd need to update offsets though the entire
    //      list of items
    //
    //      on the other hand maybe we could use a larger buffer such
    //      as 64Kb at once to avoid too many allocations total
    //      (or use a hint / user settings / stats / ...)
    //
    vbuf_t append;
    append.f_data.resize((size + 4095) & -4096);
    append.f_size = size;

    memcpy(append.f_data.data(), in, size);

    f_buffers.push_back(append);

    bytes_written += size;
    f_total_size += size;

    f_modified = bytes_written != 0;
    return bytes_written;
}


int virtual_buffer::pinsert(void const * buf, std::uint64_t size, std::uint64_t offset)
{
    // avoid an insert if possible
    //
    if(size == 0)
    {
        return 0;
    }

    if(offset >= f_total_size)
    {
        return pwrite(buf, size, offset, true);
    }

    // insert has to happen... search the buffer where it will happen
    //
    std::uint8_t const * in(reinterpret_cast<std::uint8_t const *>(buf));
    for(auto b : f_buffers)
    {
        if(offset >= b.f_size)
        {
            offset -= b.f_size;
        }
        else
        {
            if(b.f_block != nullptr)
            {
                // if inserting within a block, we have to break the block
                // in two
                {
                    vbuf_t append;
                    append.f_block = b.f_block;
                    append.f_size = b.f_size - offset;
                    append.f_offset = b.f_offset + offset;
                    f_buffers.insert(&b + 1, append);
                }

                b.f_size = offset;

                {
                    vbuf_t append;
                    append.f_size = size;
                    memcpy(append.f_data.data(), in, size);
                    f_buffers.insert(&b + 1, append);
                }
            }
            else
            {
                b.f_data.insert(f_data.begin() + offset, in, in + size);
            }
            f_total_size += size;
            f_modified = size != 0;
            return size;
        }
    }

    throw snapdatabase_logic_error("Reached the end of the pinsert() function. It should never happen.");
}


int virtual_buffer::perase(std::uint64_t size, std::uint64_t offset)
{
    if(size == 0)
    {
        return 0;
    }

    if(offset >= f_total_size)
    {
        return 0;
    }

    // clamp the amount of data we can erase
    //
    if(size > f_total_size - offset)
    {
        size = f_total_size - offset;
    }

    // since we are going to erase/add some buffers (eventually)
    // we need to use our own iterator
    //
    std::uint64_t bytes_erased(0);
    for(auto it(f_buffers.begin()); it != f_buffers.end() && size > 0; )
    {
        auto next(it + 1);
        if(offset >= it->f_size)
        {
            offset -= it->f_size;
        }
        else
        {
            if(size >= it->f_size)
            {
                if(offset == 0)
                {
                    // remove this entry entirely
                    //
                    size -= it->f_size;
                    f_total_size -= it->f_size;
                    bytes_erased += it->f_size;
                    f_buffers.erase(it);
                }
                else
                {
                    // remove end of this block
                    //
                    std::uint64_t const sz(it->f_size - offset);
                    it->f_size = offset;
                    f_total_size -= sz;
                    size -= sz;
                    bytes_erased += sz;
                    offset = 0;
                }
            }
            else
            {
                if(offset == 0)
                {
                    // remove the start of this block
                    //
                    if(it->f_block != nullptr)
                    {
                        it->f_offset += size;
                    }
                    else
                    {
                        it->f_data.erase(it->f_data.begin(), it->f_data.begin() + size);
                    }
                    it->f_size -= size;
                }
                else
                {
                    // remove data from the middle of the block
                    //
                    if(it->f_block != nullptr)
                    {
                        if(offset + size >= it->f_size)
                        {
                            it->f_offset += offset;
                            it->f_size -= size;
                        }
                        else
                        {
                            vbuf_t append;
                            append.f_block = it->f_block;
                            append.f_size = it->f_size - size - offset;
                            append.f_offset = it->f_offset + size + offset;
                            f_buffers.insert(it, append);

                            it->f_size = offset;
                        }
                    }
                    else
                    {
                        it->f_data.erase(it->f_data.begin() + offset, it->f_data.begin() + offset + size);
                        it->f_size -= size;
                    }
                }
                f_total_size -= size;
                bytes_erased += size;
                f_modified = bytes_erased != 0;
                return bytes_erased;
            }
        }
        it = next;
    }

    f_modified = bytes_erased != 0;
    return bytes_erased;
}



} // namespace snapdatabase
// vim: ts=4 sw=4 et
