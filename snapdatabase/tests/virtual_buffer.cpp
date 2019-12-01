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

// self
//
#include    "main.h"


// snapdatabase lib
//
#include    <snapdatabase/data/virtual_buffer.h>


// advgetopt lib
//
#include    <advgetopt/options.h>




CATCH_TEST_CASE("VirtualBuffer", "[virtual-buffer]")
{
    CATCH_START_SECTION("simple write + read")
    {
        snapdatabase::virtual_buffer::pointer_t v(std::make_shared<snapdatabase::virtual_buffer>());
        CATCH_REQUIRE(v->size() == 0);
        CATCH_REQUIRE(v->count_buffers() == 0);

        constexpr size_t const buf_size(1024);
        char buf[buf_size];
        for(size_t i(0); i < sizeof(buf); ++i)
        {
            buf[i] = rand();
        }
        CATCH_REQUIRE(v->pwrite(buf, sizeof(buf), 0, true) == sizeof(buf));

        CATCH_REQUIRE(v->size() == sizeof(buf));
        CATCH_REQUIRE(v->count_buffers() == 1);  // one write means at most 1 buffer

        char saved[buf_size];
        CATCH_REQUIRE(v->pread(saved, sizeof(saved), 0, true) == sizeof(saved));

        CATCH_REQUIRE(memcmp(buf, saved, sizeof(buf)) == 0);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("write once + read many times")
    {
        snapdatabase::virtual_buffer::pointer_t v(std::make_shared<snapdatabase::virtual_buffer>());
        CATCH_REQUIRE(v->size() == 0);
        CATCH_REQUIRE(v->count_buffers() == 0);

        constexpr size_t const buf_size(1024);
        char buf[buf_size];
        for(size_t i(0); i < sizeof(buf); ++i)
        {
            buf[i] = rand();
        }
        CATCH_REQUIRE(v->pwrite(buf, sizeof(buf), 0, true) == sizeof(buf));

        CATCH_REQUIRE(v->size() == sizeof(buf));
        CATCH_REQUIRE(v->count_buffers() == 1);  // one write means at most 1 buffer

        for(size_t i(0); i < sizeof(buf); ++i)
        {
            char c;
            CATCH_REQUIRE(v->pread(&c, sizeof(c), i, true) == sizeof(c));
            CATCH_REQUIRE(buf[i] == c);
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("short write + read several times")
    {
        snapdatabase::virtual_buffer::pointer_t v(std::make_shared<snapdatabase::virtual_buffer>());
        CATCH_REQUIRE(v->size() == 0);
        CATCH_REQUIRE(v->count_buffers() == 0);

        constexpr size_t const buf_size(1024);
        char buf[buf_size];
        for(size_t i(0); i < sizeof(buf); ++i)
        {
            buf[i] = rand();
        }
        CATCH_REQUIRE(v->pwrite(buf, sizeof(buf), 0, true) == sizeof(buf));

        CATCH_REQUIRE(v->size() == sizeof(buf));
        CATCH_REQUIRE(v->count_buffers() == 1);  // one write means at most 1 buffer

        // update the first 4 bytes
        //
        buf[0] = rand();
        buf[1] = rand();
        buf[2] = rand();
        buf[3] = rand();
        CATCH_REQUIRE(v->pwrite(buf, 4, 0, false) == 4);

        CATCH_REQUIRE(v->size() == sizeof(buf));
        CATCH_REQUIRE(v->count_buffers() == 1);  // one write means at most 1 buffer

        for(size_t i(0); i < sizeof(buf); ++i)
        {
            char c;
            CATCH_REQUIRE(v->pread(&c, sizeof(c), i, true) == sizeof(c));
            CATCH_REQUIRE(buf[i] == c);
        }
    }
    CATCH_END_SECTION()
}


// vim: ts=4 sw=4 et
