// Snap Websites Server -- test against the snap_exception class
// Copyright (C) 2014-2017 Made to Order Software Corp.
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

//
// This test verifies that names, versions, and browsers are properly
// extracted from filenames and dependencies and then that the resulting
// versioned_filename and dependency objects compare against each others
// as expected.
//

#include <snapwebsites/log.h>
#include <snapwebsites/not_reached.h>
#include <snapwebsites/qstring_stream.h>
#include <snapwebsites/snap_exception.h>

#include <iostream>

#include <QDir>


int main(int /*argc*/, char * /*argv*/[])
{
    snap::logging::configure_console();

    SNAP_LOG_INFO("test_snap_exception started!");
    try
    {
        SNAP_LOG_INFO("Testing regular exception:");
        throw snap::snap_exception( "This is an exception!" );
        snap::NOTREACHED();
    }
    catch( snap::snap_exception & except )
    {
        SNAP_LOG_INFO() << "Caught snap exception [" << except.what() << "].";
    }

    SNAP_LOG_INFO("test_snap_exception finished.");

    return 0;
}

// vim: ts=4 sw=4 et
