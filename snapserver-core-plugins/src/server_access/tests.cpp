// Snap Websites Server -- tests for the server_access plugin
// Copyright (c) 2012-2019  Made to Order Software Corp.  All Rights Reserved
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


// self
//
#include    "server_access.h"


// other plugins
//
#include    "../content/content.h"


// snaplogger
//
#include    <snaplogger/message.h>


// C++
//
#include    <iostream>


// last include
//
#include    <snapdev/poison.h>



namespace snap
{
namespace server_access
{



SNAP_TEST_PLUGIN_SUITE(server_access)
    SNAP_TEST_PLUGIN_TEST(server_access, test_ajax)
SNAP_TEST_PLUGIN_SUITE_END()


SNAP_TEST_PLUGIN_TEST_IMPL(server_access, test_ajax)
{
    // since the unit tests are always run using AJAX the
    // following should always be true
    SNAP_TEST_PLUGIN_SUITE_ASSERT(is_ajax_request());
}



} // namespace server_access
} // namespace snap
// vim: ts=4 sw=4 et
