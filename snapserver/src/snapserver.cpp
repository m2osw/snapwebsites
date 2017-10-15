// Snap Websites Server -- snap websites server
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

#include "version.h"

#include <snapwebsites/log.h>
#include <snapwebsites/not_reached.h>
#include <snapwebsites/snap_exception.h>
#include <snapwebsites/snapwebsites.h>


// TODO:
// The follow would not work, we need to fix that problem... we should
// not have an instance if we are to be able to derive this class in
// various places...

//class the_server
//    : public snap::server
//{
//public:
//
//    virtual void show_version() override
//    {
//        std::cout << SNAPSERVER_VERSION_STRING << std::endl;
//    }
//};




int main(int argc, char *argv[])
{
    int exitval(1);
    try
    {
        // Initialize the logger as soon as possible so we get information
        // if we log anything before we fully setup the logger in the server
        // configuration (see server::config())
        //
        // This is important if the server crashes before it reaches the
        // right place in the config() function.
        //
        char const * progname = strrchr(argv[0], '/');
        if(progname == nullptr)
        {
            progname = argv[0];
        }
        else
        {
            ++progname;
        }
        snap::logging::set_progname(progname);
        snap::logging::configure_syslog();

        // create a server object
        snap::server::pointer_t s( snap::server::instance() );

        // parse the command line arguments
        s->config(argc, argv);

        // if possible, detach the server
        s->detach();
        // Only the child (server) process returns here

        // Now create the qt application instance
        //
        s->prepare_qtapp( argc, argv );

        // listen to connections
        s->listen();

        exitval = 0;
    }
    catch( snap::snap_exception const & except )
    {
        SNAP_LOG_FATAL("snapserver: snap_exception caught: ")(except.what());
    }
    catch( std::exception const & std_except )
    {
        SNAP_LOG_FATAL("snapserver: std::exception caught: ")(std_except.what());
    }
    catch( ... )
    {
        SNAP_LOG_FATAL("snapserver: unknown exception caught!");
    }

    // exit via the server so the server can clean itself up properly
    //
    snap::server::exit( exitval );
    snap::NOTREACHED();
    return 0;
}

// vim: ts=4 sw=4 et
