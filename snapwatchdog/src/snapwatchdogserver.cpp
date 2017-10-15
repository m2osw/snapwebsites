// Snap Websites Server -- snap websites backend tool
// Copyright (C) 2011-2017  Made to Order Software Corp.
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

#include "snapwatchdog.h"

#include <snapwebsites/log.h>
#include <snapwebsites/not_reached.h>
#include <snapwebsites/snap_exception.h>


int main(int argc, char * argv[])
{
    int exitval = 1;
    try
    {
        // create a server object
        snap::watchdog_server::pointer_t s( snap::watchdog_server::instance() );
        s->setup_as_backend();

        // parse the command line arguments
        s->config(argc, argv);

        // if possible, detach the server
        s->detach();
        // Only the child (backend) process returns here

        // now create the qt application instance
        s->prepare_qtapp( argc, argv );

        // listen to connections
        s->watchdog();

        exitval = 0;
    }
    catch( snap::snap_exception const& except )
    {
        SNAP_LOG_FATAL("snapbackend: snap_exception caught: ")(except.what());
    }
    catch( std::exception const& std_except )
    {
        SNAP_LOG_FATAL("snapbackend: std::exception caught: ")(std_except.what());
    }
    catch( ... )
    {
        SNAP_LOG_FATAL("snapbackend: unknown exception caught!");
    }

    // exit via the server so the server can clean itself up properly
    //
    snap::watchdog_server::exit( exitval );
    snap::NOTREACHED();
    return 0;
}

// vim: ts=4 sw=4 et
