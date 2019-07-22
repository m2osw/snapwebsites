/* catch_snapuri.cpp
 * Copyright (c) 2011-2019  Made to Order Software Corp.  All Rights Reserved
 *
 * Project: https://snapwebsites.org/project/snapwebsites
 *
 * Permission is hereby granted, free of charge, to any
 * person obtaining a copy of this software and
 * associated documentation files (the "Software"), to
 * deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the
 * following conditions:
 *
 * The above copyright notice and this permission notice
 * shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF
 * ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
 * LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO
 * EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/** \file
 * \brief Verify the snap_uri class.
 *
 * This file implements tests to verify that the snap_uri
 * class functions as expected.
 */

// self
//
#include "catch_tests.h"

// libsnapwebsites
//
#include "snapwebsites/snap_uri.h"


CATCH_TEST_CASE( "snap_uri", "[domain]" )
{
    // "normal" (canonicalized) URI
    CATCH_GIVEN("domain \"http://snap.website/\"")
    {
        snap::snap_uri uri("http://snap.website/");

        CATCH_SECTION("verify object")
        {
            CATCH_REQUIRE(uri.domain() == "snap");
            CATCH_REQUIRE(uri.top_level_domain() == ".website");
        }
    }

    // without a '/' after domain
    CATCH_GIVEN("domain \"http://snap.website\"")
    {
        snap::snap_uri uri("http://snap.website");

        CATCH_SECTION("verify object")
        {
            CATCH_REQUIRE(uri.domain() == "snap");
            CATCH_REQUIRE(uri.top_level_domain() == ".website");
        }
    }
    CATCH_GIVEN("domain \"http://snap.website//\"")
    {
        snap::snap_uri uri("http://snap.website//");

        CATCH_SECTION("verify object")
        {
            CATCH_REQUIRE(uri.domain() == "snap");
            CATCH_REQUIRE(uri.top_level_domain() == ".website");
        }
    }
    CATCH_GIVEN("domain \"http://snap.website///and/a/path\"")
    {
        snap::snap_uri uri("http://snap.website///and/a/path");

        CATCH_SECTION("verify object")
        {
            CATCH_REQUIRE(uri.domain() == "snap");
            CATCH_REQUIRE(uri.top_level_domain() == ".website");
            CATCH_REQUIRE(uri.path() == "and/a/path");
        }
    }
}


// vim: ts=4 sw=4 et
