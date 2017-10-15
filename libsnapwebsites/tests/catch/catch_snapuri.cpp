/* catch_snapuri.cpp
 * Copyright (C) 2011-2017  Made to Order Software Corporation
 *
 * Project: http://snapwebsites.org/project/snapwebsites
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
 * This file implements a tests to verify that the snap_uri
 * class functions as expected.
 */

// self
//
#include "catch_tests.h"

// libsnapwebsites
//
#include "snapwebsites/snap_uri.h"


TEST_CASE( "snap_uri", "[domain]" )
{
    // "normal" (canonicalized) URI
    GIVEN("domain \"http://snap.website/\"")
    {
        snap::snap_uri uri("http://snap.website/");

        SECTION("verify object")
        {
            REQUIRE(uri.domain() == "snap");
            REQUIRE(uri.top_level_domain() == ".website");
        }
    }

    // without a '/' after domain
    GIVEN("domain \"http://snap.website\"")
    {
        snap::snap_uri uri("http://snap.website");

        SECTION("verify object")
        {
            REQUIRE(uri.domain() == "snap");
            REQUIRE(uri.top_level_domain() == ".website");
        }
    }
    GIVEN("domain \"http://snap.website//\"")
    {
        snap::snap_uri uri("http://snap.website//");

        SECTION("verify object")
        {
            REQUIRE(uri.domain() == "snap");
            REQUIRE(uri.top_level_domain() == ".website");
        }
    }
    GIVEN("domain \"http://snap.website///and/a/path\"")
    {
        snap::snap_uri uri("http://snap.website///and/a/path");

        SECTION("verify object")
        {
            REQUIRE(uri.domain() == "snap");
            REQUIRE(uri.top_level_domain() == ".website");
            REQUIRE(uri.path() == "and/a/path");
        }
    }
}


// vim: ts=4 sw=4 et
