// Snap Websites Server -- tests for the server_access plugin
// Copyright (C) 2012-2017  Made to Order Software Corp.
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

#include "server_access.h"

#include "../content/content.h"

#include <snapwebsites/log.h>

#include <iostream>

#include <snapwebsites/poison.h>

SNAP_PLUGIN_EXTENSION_START(server_access)


SNAP_TEST_PLUGIN_SUITE(server_access)
    SNAP_TEST_PLUGIN_TEST(server_access, test_ajax)
SNAP_TEST_PLUGIN_SUITE_END()


SNAP_TEST_PLUGIN_TEST_IMPL(server_access, test_ajax)
{
    // since the unit tests are always run using AJAX the
    // following should always be true
    SNAP_TEST_PLUGIN_SUITE_ASSERT(is_ajax_request());
}


SNAP_PLUGIN_EXTENSION_END()

// vim: ts=4 sw=4 et
