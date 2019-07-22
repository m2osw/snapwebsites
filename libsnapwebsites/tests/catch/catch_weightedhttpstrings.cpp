/* catch_weightedhttpstrings.cpp
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
 * \brief Verify the WeightedHttpStrings class.
 *
 * This file implements a tests to verify that the WeightedHttpString
 * class functions as expected.
 */

// self
//
#include "catch_tests.h"

// libsnapwebsites
//
#include "snapwebsites/http_strings.h"


CATCH_TEST_CASE( "weightedhttpstring", "[string]" )
{
    CATCH_GIVEN("string \"en\"")
    {
        snap::http_strings::WeightedHttpString locale("en");

        CATCH_SECTION("verify object, except parts")
        {
            // no error occurred
            //
            CATCH_REQUIRE(locale.error_messages().isEmpty());

            // original string does not change
            //
            CATCH_REQUIRE(locale.get_string() == "en");

            // get_level() with correct and wrong names
            //
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("en"), snap::http_strings::WeightedHttpString::part_t::DEFAULT_LEVEL()));
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("fr"), snap::http_strings::WeightedHttpString::part_t::UNDEFINED_LEVEL()));

            // convert back to a string
            //
            CATCH_REQUIRE(locale.to_string() == "en");
        }

        CATCH_SECTION("verify parts")
        {
            // make sure the result is 1 part
            //
            snap::http_strings::WeightedHttpString::part_t::vector_t & p(locale.get_parts());
            CATCH_REQUIRE(p.size() == 1);

            // also get the constant version
            //
            snap::http_strings::WeightedHttpString::part_t::vector_t const & pc(locale.get_parts());
            CATCH_REQUIRE(pc.size() == 1);

            // now verify that the part is correct
            //
            CATCH_REQUIRE(p[0].get_name() == "en");
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[0].get_level(), snap::http_strings::WeightedHttpString::part_t::DEFAULT_LEVEL()));
            CATCH_REQUIRE(p[0].get_parameter("test") == "");
            CATCH_REQUIRE(p[0].to_string() == "en");
        }
    }

    CATCH_GIVEN("string \"en-US,en;q=0.8,fr-FR;q=0.5,fr;q=0.3\"")
    {
        snap::http_strings::WeightedHttpString locale("en-US,en;q=0.8,fr-FR;q=0.5,fr;q=0.3");

        CATCH_SECTION("verify object")
        {
            // no error occurred
            //
            CATCH_REQUIRE(locale.error_messages().isEmpty());

            // original string does not change
            //
            CATCH_REQUIRE(locale.get_string() == "en-US,en;q=0.8,fr-FR;q=0.5,fr;q=0.3");

            // get_level() with correct and wrong names
            //
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("en-US"), snap::http_strings::WeightedHttpString::part_t::DEFAULT_LEVEL()));
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("en"),    0.8f));
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("fr-FR"), 0.5f));
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("fr"),    0.3f));

            // convert back to a string
            //
            CATCH_REQUIRE(locale.to_string() == "en-US, en; q=0.8, fr-FR; q=0.5, fr; q=0.3");
        }

        CATCH_SECTION("verify part")
        {
            // make sure the result is 4 parts
            //
            snap::http_strings::WeightedHttpString::part_t::vector_t & p(locale.get_parts());
            CATCH_REQUIRE(p.size() == 4);

            // also get the constant version
            //
            snap::http_strings::WeightedHttpString::part_t::vector_t const & pc(locale.get_parts());
            CATCH_REQUIRE(pc.size() == 4);

            // now verify that the parts are correct
            //
            CATCH_REQUIRE(p[0].get_name() == "en-US");
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[0].get_level(), snap::http_strings::WeightedHttpString::part_t::DEFAULT_LEVEL()));
            CATCH_REQUIRE(p[0].get_parameter("test") == "");
            CATCH_REQUIRE(p[0].to_string() == "en-US");

            CATCH_REQUIRE(p[1].get_name() == "en");
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[1].get_level(), 0.8f));
            CATCH_REQUIRE(p[1].get_parameter("test") == "");
            CATCH_REQUIRE(p[1].to_string() == "en; q=0.8");

            CATCH_REQUIRE(p[2].get_name() == "fr-FR");
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[2].get_level(), 0.5f));
            CATCH_REQUIRE(p[2].get_parameter("test") == "");
            CATCH_REQUIRE(p[2].to_string() == "fr-FR; q=0.5");

            CATCH_REQUIRE(p[3].get_name() == "fr");
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[3].get_level(), 0.3f));
            CATCH_REQUIRE(p[3].get_parameter("test") == "");
            CATCH_REQUIRE(p[3].to_string() == "fr; q=0.3");

            CATCH_REQUIRE(pc[0].get_name() == "en-US");
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(pc[0].get_level(), snap::http_strings::WeightedHttpString::part_t::DEFAULT_LEVEL()));
            CATCH_REQUIRE(pc[0].get_parameter("test") == "");
            CATCH_REQUIRE(pc[0].to_string() == "en-US");

            CATCH_REQUIRE(pc[1].get_name() == "en");
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(pc[1].get_level(), 0.8f));
            CATCH_REQUIRE(pc[1].get_parameter("test") == "");
            CATCH_REQUIRE(pc[1].to_string() == "en; q=0.8");

            CATCH_REQUIRE(pc[2].get_name() == "fr-FR");
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(pc[2].get_level(), 0.5f));
            CATCH_REQUIRE(pc[2].get_parameter("test") == "");
            CATCH_REQUIRE(pc[2].to_string() == "fr-FR; q=0.5");

            CATCH_REQUIRE(pc[3].get_name() == "fr");
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(pc[3].get_level(), 0.3f));
            CATCH_REQUIRE(pc[3].get_parameter("test") == "");
            CATCH_REQUIRE(pc[3].to_string() == "fr; q=0.3");
        }

        CATCH_SECTION("sort has no effect if weights are equal")
        {
            locale.sort_by_level();

            // make sure the result is 3 parts
            //
            snap::http_strings::WeightedHttpString::part_t::vector_t & p(locale.get_parts());
            CATCH_REQUIRE(p.size() == 4);

            // now verify that the parts are correct
            //
            CATCH_REQUIRE(p[0].get_name() == "en-US");
            CATCH_REQUIRE(p[1].get_name() == "en");
            CATCH_REQUIRE(p[2].get_name() == "fr-FR");
            CATCH_REQUIRE(p[3].get_name() == "fr");
        }
    }

    CATCH_GIVEN("string \"de, en, fr\"")
    {
        snap::http_strings::WeightedHttpString locale("de, en, fr");

        CATCH_SECTION("verify object")
        {
            // no error occurred
            //
            CATCH_REQUIRE(locale.error_messages().isEmpty());

            // original string does not change
            //
            CATCH_REQUIRE(locale.get_string() == "de, en, fr");

            // get_level() with correct and wrong names
            //
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("de"), snap::http_strings::WeightedHttpString::part_t::DEFAULT_LEVEL()));
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("en"), snap::http_strings::WeightedHttpString::part_t::DEFAULT_LEVEL()));
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("fr"), snap::http_strings::WeightedHttpString::part_t::DEFAULT_LEVEL()));
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("es"), snap::http_strings::WeightedHttpString::part_t::UNDEFINED_LEVEL()));

            // convert back to a string
            //
            CATCH_REQUIRE(locale.to_string() == "de, en, fr");
        }

        CATCH_SECTION("verify part")
        {
            // make sure the result is 3 parts
            //
            snap::http_strings::WeightedHttpString::part_t::vector_t & p(locale.get_parts());
            CATCH_REQUIRE(p.size() == 3);

            // also get the constant version
            //
            snap::http_strings::WeightedHttpString::part_t::vector_t const & pc(locale.get_parts());
            CATCH_REQUIRE(pc.size() == 3);

            // now verify that the parts are correct
            //
            CATCH_REQUIRE(p[0].get_name() == "de");
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[0].get_level(), snap::http_strings::WeightedHttpString::part_t::DEFAULT_LEVEL()));
            CATCH_REQUIRE(p[0].get_parameter("test") == "");
            CATCH_REQUIRE(p[0].to_string() == "de");

            CATCH_REQUIRE(p[1].get_name() == "en");
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[1].get_level(), snap::http_strings::WeightedHttpString::part_t::DEFAULT_LEVEL()));
            CATCH_REQUIRE(p[1].get_parameter("test") == "");
            CATCH_REQUIRE(p[1].to_string() == "en");

            CATCH_REQUIRE(p[2].get_name() == "fr");
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[2].get_level(), snap::http_strings::WeightedHttpString::part_t::DEFAULT_LEVEL()));
            CATCH_REQUIRE(p[2].get_parameter("test") == "");
            CATCH_REQUIRE(p[2].to_string() == "fr");

            CATCH_REQUIRE(pc[0].get_name() == "de");
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(pc[0].get_level(), snap::http_strings::WeightedHttpString::part_t::DEFAULT_LEVEL()));
            CATCH_REQUIRE(pc[0].get_parameter("test") == "");
            CATCH_REQUIRE(pc[0].to_string() == "de");

            CATCH_REQUIRE(pc[1].get_name() == "en");
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(pc[1].get_level(), snap::http_strings::WeightedHttpString::part_t::DEFAULT_LEVEL()));
            CATCH_REQUIRE(pc[1].get_parameter("test") == "");
            CATCH_REQUIRE(pc[1].to_string() == "en");

            CATCH_REQUIRE(pc[2].get_name() == "fr");
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(pc[2].get_level(), snap::http_strings::WeightedHttpString::part_t::DEFAULT_LEVEL()));
            CATCH_REQUIRE(pc[2].get_parameter("test") == "");
            CATCH_REQUIRE(pc[2].to_string() == "fr");
        }

        CATCH_SECTION("sort has no effect if weights are equal")
        {
            locale.sort_by_level();

            // make sure the result is 3 parts
            //
            snap::http_strings::WeightedHttpString::part_t::vector_t & p(locale.get_parts());
            CATCH_REQUIRE(p.size() == 3);

            // now verify that the parts are correct
            //
            CATCH_REQUIRE(p[0].get_name() == "de");
            CATCH_REQUIRE(p[1].get_name() == "en");
            CATCH_REQUIRE(p[2].get_name() == "fr");
        }
    }

    CATCH_GIVEN("string \"fr, za, en\", names are not in alphabetical order and do not get sorted")
    {
        snap::http_strings::WeightedHttpString locale("fr, za, en");

        CATCH_SECTION("verify object")
        {
            // no error occurred
            //
            CATCH_REQUIRE(locale.error_messages().isEmpty());

            // original string does not change
            //
            CATCH_REQUIRE(locale.get_string() == "fr, za, en");

            // get_level() with correct and wrong names
            //
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("fr"), snap::http_strings::WeightedHttpString::part_t::DEFAULT_LEVEL()));
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("za"), snap::http_strings::WeightedHttpString::part_t::DEFAULT_LEVEL()));
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("en"), snap::http_strings::WeightedHttpString::part_t::DEFAULT_LEVEL()));
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("de"), snap::http_strings::WeightedHttpString::part_t::UNDEFINED_LEVEL()));

            // convert back to a string
            //
            CATCH_REQUIRE(locale.to_string() == "fr, za, en");
        }

        CATCH_SECTION("verify part")
        {
            // make sure the result is 3 parts
            //
            snap::http_strings::WeightedHttpString::part_t::vector_t & p(locale.get_parts());
            CATCH_REQUIRE(p.size() == 3);

            // also get the constant version
            //
            snap::http_strings::WeightedHttpString::part_t::vector_t const & pc(locale.get_parts());
            CATCH_REQUIRE(pc.size() == 3);

            // now verify that the parts are correct
            //
            CATCH_REQUIRE(p[0].get_name() == "fr");
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[0].get_level(), snap::http_strings::WeightedHttpString::part_t::DEFAULT_LEVEL()));
            CATCH_REQUIRE(p[0].get_parameter("test") == "");
            CATCH_REQUIRE(p[0].to_string() == "fr");

            CATCH_REQUIRE(p[1].get_name() == "za");
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[1].get_level(), snap::http_strings::WeightedHttpString::part_t::DEFAULT_LEVEL()));
            CATCH_REQUIRE(p[1].get_parameter("test") == "");
            CATCH_REQUIRE(p[1].to_string() == "za");

            CATCH_REQUIRE(p[2].get_name() == "en");
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[2].get_level(), snap::http_strings::WeightedHttpString::part_t::DEFAULT_LEVEL()));
            CATCH_REQUIRE(p[2].get_parameter("test") == "");
            CATCH_REQUIRE(p[2].to_string() == "en");

            CATCH_REQUIRE(pc[0].get_name() == "fr");
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(pc[0].get_level(), snap::http_strings::WeightedHttpString::part_t::DEFAULT_LEVEL()));
            CATCH_REQUIRE(pc[0].get_parameter("test") == "");
            CATCH_REQUIRE(pc[0].to_string() == "fr");

            CATCH_REQUIRE(pc[1].get_name() == "za");
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(pc[1].get_level(), snap::http_strings::WeightedHttpString::part_t::DEFAULT_LEVEL()));
            CATCH_REQUIRE(pc[1].get_parameter("test") == "");
            CATCH_REQUIRE(pc[1].to_string() == "za");

            CATCH_REQUIRE(pc[2].get_name() == "en");
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(pc[2].get_level(), snap::http_strings::WeightedHttpString::part_t::DEFAULT_LEVEL()));
            CATCH_REQUIRE(pc[2].get_parameter("test") == "");
            CATCH_REQUIRE(pc[2].to_string() == "en");
        }

        CATCH_SECTION("sort has no effect if weights are equal")
        {
            locale.sort_by_level();

            // make sure the result is 3 parts
            //
            snap::http_strings::WeightedHttpString::part_t::vector_t & p(locale.get_parts());
            CATCH_REQUIRE(p.size() == 3);

            // now verify that the parts are correct
            //
            CATCH_REQUIRE(p[0].get_name() == "fr");
            CATCH_REQUIRE(p[1].get_name() == "za");
            CATCH_REQUIRE(p[2].get_name() == "en");
        }
    }

    CATCH_GIVEN("string \"fr;q=0, za; q=0.6,en; q=0.4\"")
    {
        snap::http_strings::WeightedHttpString locale("fr;q=0, za; q=0.6,en; q=0.4");

        CATCH_SECTION("verify object")
        {
            // no error occurred
            //
            CATCH_REQUIRE(locale.error_messages().isEmpty());

            // original string does not change
            //
            CATCH_REQUIRE(locale.get_string() == "fr;q=0, za; q=0.6,en; q=0.4");

            // get_level() with correct and wrong names
            //
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("fr"), 0.0f));
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("za"), 0.6f));
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("en"), 0.4f));
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("de"), snap::http_strings::WeightedHttpString::part_t::UNDEFINED_LEVEL()));

            // convert back to a string
            //
            CATCH_REQUIRE(locale.to_string() == "fr; q=0, za; q=0.6, en; q=0.4");
        }

        CATCH_SECTION("verify part")
        {
            // make sure the result is 3 parts
            //
            snap::http_strings::WeightedHttpString::part_t::vector_t & p(locale.get_parts());
            CATCH_REQUIRE(p.size() == 3);

            // also get the constant version
            //
            snap::http_strings::WeightedHttpString::part_t::vector_t const & pc(locale.get_parts());
            CATCH_REQUIRE(pc.size() == 3);

            // now verify that the parts are correct
            //
            CATCH_REQUIRE(p[0].get_name() == "fr");
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[0].get_level(), 0.0f));
            CATCH_REQUIRE(p[0].get_parameter("test") == "");
            CATCH_REQUIRE(p[0].to_string() == "fr; q=0");

            CATCH_REQUIRE(p[1].get_name() == "za");
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[1].get_level(), 0.6f));
            CATCH_REQUIRE(p[1].get_parameter("test") == "");
            CATCH_REQUIRE(p[1].to_string() == "za; q=0.6");

            CATCH_REQUIRE(p[2].get_name() == "en");
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[2].get_level(), 0.4f));
            CATCH_REQUIRE(p[2].get_parameter("test") == "");
            CATCH_REQUIRE(p[2].to_string() == "en; q=0.4");

            CATCH_REQUIRE(pc[0].get_name() == "fr");
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(pc[0].get_level(), 0.0f));
            CATCH_REQUIRE(pc[0].get_parameter("test") == "");
            CATCH_REQUIRE(pc[0].to_string() == "fr; q=0");

            CATCH_REQUIRE(pc[1].get_name() == "za");
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(pc[1].get_level(), 0.6f));
            CATCH_REQUIRE(pc[1].get_parameter("test") == "");
            CATCH_REQUIRE(pc[1].to_string() == "za; q=0.6");

            CATCH_REQUIRE(pc[2].get_name() == "en");
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(pc[2].get_level(), 0.4f));
            CATCH_REQUIRE(pc[2].get_parameter("test") == "");
            CATCH_REQUIRE(pc[2].to_string() == "en; q=0.4");
        }

        CATCH_SECTION("sort by level (weight)")
        {
            locale.sort_by_level();

            // make sure the result is 3 parts
            //
            snap::http_strings::WeightedHttpString::part_t::vector_t & p(locale.get_parts());
            CATCH_REQUIRE(p.size() == 3);

            // now verify that the parts are sorted by level
            //    "fr; q=0, za; q=0.6, en; q=0.4"
            //
            CATCH_REQUIRE(p[0].get_name() == "za");
            CATCH_REQUIRE(p[1].get_name() == "en");
            CATCH_REQUIRE(p[2].get_name() == "fr");

            // convert back to a string in the new order and with spaces
            //
            CATCH_REQUIRE(locale.to_string() == "za; q=0.6, en; q=0.4, fr; q=0");
        }
    }

    CATCH_GIVEN("string \"  fr;  q=0,  za;  q=0.6,  en;  q=0.4  \", with extra spaces")
    {
        snap::http_strings::WeightedHttpString locale("  fr;  q=0,  za;  q=0.6,  en;  q=0.4  ");

        CATCH_SECTION("verify object")
        {
            // no error occurred
            //
            CATCH_REQUIRE(locale.error_messages().isEmpty());

            // original string does not change
            //
            CATCH_REQUIRE(locale.get_string() == "  fr;  q=0,  za;  q=0.6,  en;  q=0.4  ");

            // get_level() with correct and wrong names
            //
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("fr"), 0.0f));
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("za"), 0.6f));
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("en"), 0.4f));
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("de"), -1.0f));

            // convert back to a string
            //
            CATCH_REQUIRE(locale.to_string() == "fr; q=0, za; q=0.6, en; q=0.4");
        }

        CATCH_SECTION("verify part")
        {
            // make sure the result is 3 parts
            //
            snap::http_strings::WeightedHttpString::part_t::vector_t & p(locale.get_parts());
            CATCH_REQUIRE(p.size() == 3);

            // also get the constant version
            //
            snap::http_strings::WeightedHttpString::part_t::vector_t const & pc(locale.get_parts());
            CATCH_REQUIRE(pc.size() == 3);

            // now verify that the parts are correct
            //
            CATCH_REQUIRE(p[0].get_name() == "fr");
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[0].get_level(), 0.0f));
            CATCH_REQUIRE(p[0].get_parameter("test") == "");
            CATCH_REQUIRE(p[0].to_string() == "fr; q=0");

            CATCH_REQUIRE(p[1].get_name() == "za");
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[1].get_level(), 0.6f));
            CATCH_REQUIRE(p[1].get_parameter("test") == "");
            CATCH_REQUIRE(p[1].to_string() == "za; q=0.6");

            CATCH_REQUIRE(p[2].get_name() == "en");
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[2].get_level(), 0.4f));
            CATCH_REQUIRE(p[2].get_parameter("test") == "");
            CATCH_REQUIRE(p[2].to_string() == "en; q=0.4");

            CATCH_REQUIRE(pc[0].get_name() == "fr");
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(pc[0].get_level(), 0.0f));
            CATCH_REQUIRE(pc[0].get_parameter("test") == "");
            CATCH_REQUIRE(pc[0].to_string() == "fr; q=0");

            CATCH_REQUIRE(pc[1].get_name() == "za");
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(pc[1].get_level(), 0.6f));
            CATCH_REQUIRE(pc[1].get_parameter("test") == "");
            CATCH_REQUIRE(pc[1].to_string() == "za; q=0.6");

            CATCH_REQUIRE(pc[2].get_name() == "en");
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(pc[2].get_level(), 0.4f));
            CATCH_REQUIRE(pc[2].get_parameter("test") == "");
            CATCH_REQUIRE(pc[2].to_string() == "en; q=0.4");
        }

        CATCH_SECTION("sort by level (weight)")
        {
            locale.sort_by_level();

            // make sure the result is 3 parts
            //
            snap::http_strings::WeightedHttpString::part_t::vector_t & p(locale.get_parts());
            CATCH_REQUIRE(p.size() == 3);

            // now verify that the parts are sorted by level
            //
            CATCH_REQUIRE(p[0].get_name() == "za");
            CATCH_REQUIRE(p[1].get_name() == "en");
            CATCH_REQUIRE(p[2].get_name() == "fr");
        }
    }

    CATCH_GIVEN("string \"  fr;  q=0,  za,  en;  q=0.4  ,es;q=1.0\", with extra spaces")
    {
        snap::http_strings::WeightedHttpString locale("  fr;  q=0,  za,  en;  q=0.4  ,es;q=1.0");

        CATCH_SECTION("verify object")
        {
            // no error occurred
            //
            CATCH_REQUIRE(locale.error_messages().isEmpty());

            // original string does not change
            //
            CATCH_REQUIRE(locale.get_string() == "  fr;  q=0,  za,  en;  q=0.4  ,es;q=1.0");

            // get_level() with correct and wrong names
            //
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("fr"), 0.0f));
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("za"), snap::http_strings::WeightedHttpString::part_t::DEFAULT_LEVEL()));
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("en"), 0.4f));
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("es"), 1.0f));
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("de"), -1.0f));

            // convert back to a string
            //
            CATCH_REQUIRE(locale.to_string() == "fr; q=0, za, en; q=0.4, es; q=1.0");
        }

        CATCH_SECTION("verify part")
        {
            // make sure the result is 3 parts
            //
            snap::http_strings::WeightedHttpString::part_t::vector_t & p(locale.get_parts());
            CATCH_REQUIRE(p.size() == 4);

            // also get the constant version
            //
            snap::http_strings::WeightedHttpString::part_t::vector_t const & pc(locale.get_parts());
            CATCH_REQUIRE(pc.size() == 4);

            // now verify that the parts are correct
            //
            CATCH_REQUIRE(p[0].get_name() == "fr");
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[0].get_level(), 0.0f));
            CATCH_REQUIRE(p[0].get_parameter("test") == "");
            CATCH_REQUIRE(p[0].to_string() == "fr; q=0");

            CATCH_REQUIRE(p[1].get_name() == "za");
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[1].get_level(), snap::http_strings::WeightedHttpString::part_t::DEFAULT_LEVEL()));
            CATCH_REQUIRE(p[1].get_parameter("test") == "");
            CATCH_REQUIRE(p[1].to_string() == "za");

            CATCH_REQUIRE(p[2].get_name() == "en");
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[2].get_level(), 0.4f));
            CATCH_REQUIRE(p[2].get_parameter("test") == "");
            CATCH_REQUIRE(p[2].to_string() == "en; q=0.4");

            CATCH_REQUIRE(p[3].get_name() == "es");
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[3].get_level(), 1.0f));
            CATCH_REQUIRE(p[3].get_parameter("test") == "");
            CATCH_REQUIRE(p[3].to_string() == "es; q=1.0");

            CATCH_REQUIRE(pc[0].get_name() == "fr");
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(pc[0].get_level(), 0.0f));
            CATCH_REQUIRE(pc[0].get_parameter("test") == "");
            CATCH_REQUIRE(pc[0].to_string() == "fr; q=0");

            CATCH_REQUIRE(pc[1].get_name() == "za");
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(pc[1].get_level(), snap::http_strings::WeightedHttpString::part_t::DEFAULT_LEVEL()));
            CATCH_REQUIRE(pc[1].get_parameter("test") == "");
            CATCH_REQUIRE(pc[1].to_string() == "za");

            CATCH_REQUIRE(pc[2].get_name() == "en");
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(pc[2].get_level(), 0.4f));
            CATCH_REQUIRE(pc[2].get_parameter("test") == "");
            CATCH_REQUIRE(pc[2].to_string() == "en; q=0.4");

            CATCH_REQUIRE(p[3].get_name() == "es");
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[3].get_level(), 1.0f));
            CATCH_REQUIRE(p[3].get_parameter("test") == "");
            CATCH_REQUIRE(p[3].to_string() == "es; q=1.0");
        }

        CATCH_SECTION("sort by level (weight)")
        {
            locale.sort_by_level();

            // make sure the result is 3 parts
            //
            snap::http_strings::WeightedHttpString::part_t::vector_t & p(locale.get_parts());
            CATCH_REQUIRE(p.size() == 4);

            // now verify that the parts are sorted by level
            //
            CATCH_REQUIRE(p[0].get_name() == "za");
            CATCH_REQUIRE(p[1].get_name() == "es");
            CATCH_REQUIRE(p[2].get_name() == "en");
            CATCH_REQUIRE(p[3].get_name() == "fr");
        }
    }

    CATCH_GIVEN("string \"de\", then \"en\", then \"fr\"")
    {
        snap::http_strings::WeightedHttpString locale("de");

        CATCH_SECTION("verify object")
        {
            // no error occurred
            //
            CATCH_REQUIRE(locale.error_messages().isEmpty());

            // original string does not change
            //
            CATCH_REQUIRE(locale.get_string() == "de");

            // get_level() with correct and wrong names
            //
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("de"), snap::http_strings::WeightedHttpString::part_t::DEFAULT_LEVEL()));
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("en"), snap::http_strings::WeightedHttpString::part_t::UNDEFINED_LEVEL()));
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("fr"), snap::http_strings::WeightedHttpString::part_t::UNDEFINED_LEVEL()));
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("es"), snap::http_strings::WeightedHttpString::part_t::UNDEFINED_LEVEL()));

            // convert back to a string
            //
            CATCH_REQUIRE(locale.to_string() == "de");

            // make sure the result is 1 part
            //
            snap::http_strings::WeightedHttpString::part_t::vector_t & p(locale.get_parts());
            CATCH_REQUIRE(p.size() == 1);

            // now verify that the parts are correct
            //
            CATCH_REQUIRE(p[0].get_name() == "de");
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[0].get_level(), snap::http_strings::WeightedHttpString::part_t::DEFAULT_LEVEL()));
            CATCH_REQUIRE(p[0].get_parameter("test") == "");
            CATCH_REQUIRE(p[0].to_string() == "de");
        }

        CATCH_SECTION("add \"en\"")
        {
            // the parse is expected to work (return true)
            //
            CATCH_REQUIRE(locale.parse("en"));

            // no error occurred
            //
            CATCH_REQUIRE(locale.error_messages().isEmpty());

            // original string does not change
            //
            CATCH_REQUIRE(locale.get_string() == "de,en");

            // get_level() with correct and wrong names
            //
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("de"), snap::http_strings::WeightedHttpString::part_t::DEFAULT_LEVEL()));
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("en"), snap::http_strings::WeightedHttpString::part_t::DEFAULT_LEVEL()));
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("fr"), snap::http_strings::WeightedHttpString::part_t::UNDEFINED_LEVEL()));
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("es"), snap::http_strings::WeightedHttpString::part_t::UNDEFINED_LEVEL()));

            // convert back to a string
            //
            CATCH_REQUIRE(locale.to_string() == "de, en");

            // make sure the result is 2 parts
            //
            snap::http_strings::WeightedHttpString::part_t::vector_t & p(locale.get_parts());
            CATCH_REQUIRE(p.size() == 2);

            // now verify that the parts are correct
            //
            CATCH_REQUIRE(p[0].get_name() == "de");
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[0].get_level(), snap::http_strings::WeightedHttpString::part_t::DEFAULT_LEVEL()));
            CATCH_REQUIRE(p[0].get_parameter("test") == "");
            CATCH_REQUIRE(p[0].to_string() == "de");

            CATCH_REQUIRE(p[1].get_name() == "en");
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[1].get_level(), snap::http_strings::WeightedHttpString::part_t::DEFAULT_LEVEL()));
            CATCH_REQUIRE(p[1].get_parameter("test") == "");
            CATCH_REQUIRE(p[1].to_string() == "en");
        }

        CATCH_SECTION("add \"en\" and then \"fr\"")
        {
            // the parse is expected to work (return true)
            //
            CATCH_REQUIRE(locale.parse("en"));
            CATCH_REQUIRE(locale.parse("fr"));

            // no error occurred
            //
            CATCH_REQUIRE(locale.error_messages().isEmpty());

            // original string does not change
            //
            CATCH_REQUIRE(locale.get_string() == "de,en,fr");

            // get_level() with correct and wrong names
            //
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("de"), snap::http_strings::WeightedHttpString::part_t::DEFAULT_LEVEL()));
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("en"), snap::http_strings::WeightedHttpString::part_t::DEFAULT_LEVEL()));
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("fr"), snap::http_strings::WeightedHttpString::part_t::DEFAULT_LEVEL()));
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("es"), snap::http_strings::WeightedHttpString::part_t::UNDEFINED_LEVEL()));

            // convert back to a string
            //
            CATCH_REQUIRE(locale.to_string() == "de, en, fr");

            // make sure the result is 3 parts
            //
            snap::http_strings::WeightedHttpString::part_t::vector_t & p(locale.get_parts());
            CATCH_REQUIRE(p.size() == 3);

            // now verify that the parts are correct
            //
            CATCH_REQUIRE(p[0].get_name() == "de");
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[0].get_level(), snap::http_strings::WeightedHttpString::part_t::DEFAULT_LEVEL()));
            CATCH_REQUIRE(p[0].get_parameter("test") == "");
            CATCH_REQUIRE(p[0].to_string() == "de");

            CATCH_REQUIRE(p[1].get_name() == "en");
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[1].get_level(), snap::http_strings::WeightedHttpString::part_t::DEFAULT_LEVEL()));
            CATCH_REQUIRE(p[1].get_parameter("test") == "");
            CATCH_REQUIRE(p[1].to_string() == "en");

            CATCH_REQUIRE(p[2].get_name() == "fr");
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[2].get_level(), snap::http_strings::WeightedHttpString::part_t::DEFAULT_LEVEL()));
            CATCH_REQUIRE(p[2].get_parameter("test") == "");
            CATCH_REQUIRE(p[2].to_string() == "fr");
        }

        CATCH_SECTION("replace with \"mo\"")
        {
            // the parse is expected to work (return true)
            //
            CATCH_REQUIRE(locale.parse("mo", true));

            // no error occurred
            //
            CATCH_REQUIRE(locale.error_messages().isEmpty());

            // original string does not change
            //
            CATCH_REQUIRE(locale.get_string() == "mo");

            // get_level() with correct and wrong names
            //
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("mo"), snap::http_strings::WeightedHttpString::part_t::DEFAULT_LEVEL()));
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("en"), snap::http_strings::WeightedHttpString::part_t::UNDEFINED_LEVEL()));
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("fr"), snap::http_strings::WeightedHttpString::part_t::UNDEFINED_LEVEL()));
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(locale.get_level("es"), snap::http_strings::WeightedHttpString::part_t::UNDEFINED_LEVEL()));

            // convert back to a string
            //
            CATCH_REQUIRE(locale.to_string() == "mo");

            // make sure the result is 1 part
            //
            snap::http_strings::WeightedHttpString::part_t::vector_t & p(locale.get_parts());
            CATCH_REQUIRE(p.size() == 1);

            // now verify that the parts are correct
            //
            CATCH_REQUIRE(p[0].get_name() == "mo");
            CATCH_REQUIRE(SNAP_CATCH2_NAMESPACE::nearly_equal(p[0].get_level(), snap::http_strings::WeightedHttpString::part_t::DEFAULT_LEVEL()));
            CATCH_REQUIRE(p[0].get_parameter("test") == "");
            CATCH_REQUIRE(p[0].to_string() == "mo");
        }
    }
}


// vim: ts=4 sw=4 et
