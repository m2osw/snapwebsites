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
 * \brief The virtual buffer declarations.
 *
 * When dealing with a block, we at times have to reduce or enlarge it.
 * Several resizing events may occur before it settles. It is best not
 * to resize the entire block for each event. _To ease the damage,_ we
 * want to use separate memory buffer to handle growths. Once we are
 * done with a structure, we can then request for the final data to
 * be written to file.
 *
 * Another case is when a structure ends up being larger than one block.
 * For example, the table schema can end up requiring 2 or 3 blocks.
 * To handle that case, we use avirtual buffer as well. This is very
 * practical because that way we do not have to handle the fact that
 * the buffer is multiple buffers. The virtual buffer gives us one
 * linear offset starting at `0` and going up to `size - 1`.
 */

// self
//
#include    "snapdatabase/block/block.h"


// C++ lib
//
#include    <deque>



namespace snapdatabase
{



typedef std::vector<uint8_t>            buffer_t;


class virtual_buffer
{
public:
    typedef std::shared_ptr<virtual_buffer> pointer_t;

                                        virtual_buffer();
                                        virtual_buffer(block::pointer_t b, std::uint64_t offset, std::uint64_t size);

    void                                add_buffer(block::pointer_t b, std::uint64_t offset, std::uint64_t size);

    bool                                modified() const;
    std::size_t                         count_buffers() const;
    std::uint64_t                       size() const;
    bool                                is_data_available(std::uint64_t offset, std::uint64_t size) const;

    int                                 pread(void * buf, std::uint64_t size, std::uint64_t offset, bool full = true) const;
    int                                 pwrite(void const * buf, std::uint64_t size, std::uint64_t offset, bool allow_growth = false);
    int                                 pinsert(void const * buf, std::uint64_t size, std::uint64_t offset);
    int                                 perase(std::uint64_t size, std::uint64_t offset);

private:
    struct vbuf_t
    {
        typedef std::deque<vbuf_t>          deque_t;

                                            vbuf_t();
                                            vbuf_t(block::pointer_t b, std::uint64_t offset, std::uint64_t size);

        block::pointer_t                    f_block = block::pointer_t();
        buffer_t                            f_data = buffer_t();    // data not (yet) in the block(s)
        std::uint64_t                       f_offset = 0;
        std::uint64_t                       f_size = 0;
    };

    vbuf_t::deque_t                     f_buffers = vbuf_t::deque_t();
    std::uint64_t                       f_total_size = 0;
    bool                                f_modified = false;
};



std::ostream & operator << (std::ostream & out, virtual_buffer const & v);



} // namespace snapdatabase
// vim: ts=4 sw=4 et
