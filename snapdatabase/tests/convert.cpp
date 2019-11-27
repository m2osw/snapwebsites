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


// snaplogger lib
//
#include    <snapdatabase/exception.h>
#include    <snapdatabase/data/convert.h>


// C++ lib
//
#include    <fstream>


// C lib
//
#include    <sys/stat.h>
#include    <sys/types.h>





CATCH_TEST_CASE("8 bit convert", "[convert]")
{
    CATCH_START_SECTION("uint8_t")
    {
        for(uint32_t i(0); i < (1ULL << 8); ++i)
        {
            char buf[16];

            snprintf(buf, sizeof(buf), "%d", i);
            {
                uint64_t const c(snapdatabase::convert_to_int(buf, 8));
                CATCH_REQUIRE(c == i);
            }

            snprintf(buf, sizeof(buf), "0x%X", i);
            {
                uint64_t const c(snapdatabase::convert_to_int(buf, 8));
                CATCH_REQUIRE(c == i);
            }

            snprintf(buf, sizeof(buf), "0X%X", i);
            {
                uint64_t const c(snapdatabase::convert_to_int(buf, 8));
                CATCH_REQUIRE(c == i);
            }

            snprintf(buf, sizeof(buf), "x'%X'", i);
            {
                uint64_t const c(snapdatabase::convert_to_int(buf, 8));
                CATCH_REQUIRE(c == i);
            }

            snprintf(buf, sizeof(buf), "X'%X'", i);
            {
                uint64_t const c(snapdatabase::convert_to_int(buf, 8));
                CATCH_REQUIRE(c == i);
            }

            uint32_t v(i);
            std::string r;
            while(v != 0)
            {
                r += (v & 1) + '0';
                v >>= 1;
            }
            std::reverse(r.begin(), r.end());
            r = "0b" + r;
            {
                uint64_t const c(snapdatabase::convert_to_int(r, 8));
                CATCH_REQUIRE(c == i);
            }

            r[1] &= 0x5F;
            {
                uint64_t const c(snapdatabase::convert_to_int(r, 8));
                CATCH_REQUIRE(c == i);
            }
        }
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("16 bit convert", "[convert]")
{
    CATCH_START_SECTION("uint16_t")
    {
        for(uint32_t i(0); i < (1ULL << 16); i += rand() % 27)
        {
            char buf[32];

            snprintf(buf, sizeof(buf), "%d", i);
            {
                uint64_t const c(snapdatabase::convert_to_int(buf, 16));
                CATCH_REQUIRE(c == i);
            }

            snprintf(buf, sizeof(buf), "0x%X", i);
            {
                uint64_t const c(snapdatabase::convert_to_int(buf, 16));
                CATCH_REQUIRE(c == i);
            }

            snprintf(buf, sizeof(buf), "0X%X", i);
            {
                uint64_t const c(snapdatabase::convert_to_int(buf, 16));
                CATCH_REQUIRE(c == i);
            }

            snprintf(buf, sizeof(buf), "x'%X'", i);
            {
                uint64_t const c(snapdatabase::convert_to_int(buf, 16));
                CATCH_REQUIRE(c == i);
            }

            snprintf(buf, sizeof(buf), "X'%X'", i);
            {
                uint64_t const c(snapdatabase::convert_to_int(buf, 16));
                CATCH_REQUIRE(c == i);
            }

            uint32_t v(i);
            std::string r;
            while(v != 0)
            {
                r += (v & 1) + '0';
                v >>= 1;
            }
            std::reverse(r.begin(), r.end());
            r = "0b" + r;
            {
                uint64_t const c(snapdatabase::convert_to_int(r, 16));
                CATCH_REQUIRE(c == i);
            }

            r[1] &= 0x5F;
            {
                uint64_t const c(snapdatabase::convert_to_int(r, 16));
                CATCH_REQUIRE(c == i);
            }
        }
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("32 bit convert", "[convert]")
{
    CATCH_START_SECTION("uint32_t")
    {
        for(uint64_t i(0); i < (1ULL << 32); i += rand() % 60000)
        {
            char buf[64];

            snprintf(buf, sizeof(buf), "%ld", i);
            {
                uint64_t const c(snapdatabase::convert_to_int(buf, 32));
                CATCH_REQUIRE(c == i);
            }

            snprintf(buf, sizeof(buf), "0x%lX", i);
            {
                uint64_t const c(snapdatabase::convert_to_int(buf, 32));
                CATCH_REQUIRE(c == i);
            }

            snprintf(buf, sizeof(buf), "0X%lX", i);
            {
                uint64_t const c(snapdatabase::convert_to_int(buf, 32));
                CATCH_REQUIRE(c == i);
            }

            snprintf(buf, sizeof(buf), "x'%lX'", i);
            {
                uint64_t const c(snapdatabase::convert_to_int(buf, 32));
                CATCH_REQUIRE(c == i);
            }

            snprintf(buf, sizeof(buf), "X'%lX'", i);
            {
                uint64_t const c(snapdatabase::convert_to_int(buf, 32));
                CATCH_REQUIRE(c == i);
            }

            uint32_t v(i);
            std::string r;
            while(v != 0)
            {
                r += (v & 1) + '0';
                v >>= 1;
            }
            std::reverse(r.begin(), r.end());
            r = "0b" + r;
            {
                uint64_t const c(snapdatabase::convert_to_int(r, 32));
                CATCH_REQUIRE(c == i);
            }

            r[1] &= 0x5F;
            {
                uint64_t const c(snapdatabase::convert_to_int(r, 32));
                CATCH_REQUIRE(c == i);
            }
        }
    }
    CATCH_END_SECTION()
}



// vim: ts=4 sw=4 et
