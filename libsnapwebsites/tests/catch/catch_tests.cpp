// libsnapwebsites -- Test Suite
// Copyright (c) 2015-2019  Made to Order Software Corp.  All Rights Reserved
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
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

/** \file
 * \brief libsnapwebsites main unit test for XML/HTML/HTTP functions/objects.
 *
 * This test suite uses catch.hpp, for details see:
 *
 *   https://github.com/philsquared/Catch/blob/master/docs/tutorial.md
 */

// Tell catch we want it to add the runner code in this file.
#define CATCH_CONFIG_RUNNER

// self
//
#include "catch_tests.h"

// libsnapwebsites
//
#include "snapwebsites/version.h"

// libexcept lib
//
#include "libexcept/exception.h"

// C++ lib
//
#include <cstring>

// C lib
//
#include <stdlib.h>


namespace SNAP_CATCH2_NAMESPACE
{


} // SNAP_CATCH2_NAMESPACE namespace




int main(int argc, char * argv[])
{
    return SNAP_CATCH2_NAMESPACE::snap_catch2_main(
              "libsnapwebsites"
            , SNAPWEBSITES_VERSION_STRING
            , argc
            , argv
            , []() { libexcept::set_collect_stack(false); }
        );
}

// vim: ts=4 sw=4 et
