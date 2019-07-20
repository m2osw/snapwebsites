// Snap Websites Server -- test UDP client/server
// Copyright (c) 2014-2019  Made to Order Software Corp.  All Rights Reserved
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
// This test works along the test_shutdown_server.cpp and implements the
// client's side. It connects to the server, sends a few messages (START,
// PAUSE x 4, STOP) and expects a HUP before quitting.
//
// To run the test, you first have to start the server otherwise the
// client won't be able to connect.
//


// snapwebsites lib
//
#include <snapwebsites/log.h>
#include <snapwebsites/qstring_stream.h>
#include <snapwebsites/snap_communicator.h>
#include <snapwebsites/udp_client_server.h>


// snapdev lib
//
#include <snapdev/not_reached.h>


// C++ lib
//
#include <iostream>


// last include
//
#include <snapdev/poison.h>








int main(int /*argc*/, char * /*argv*/[])
{
    try
    {
        snap::logging::set_progname("test_shutdown_server");
        snap::logging::configure_console();
        snap::logging::set_log_output_level(snap::logging::log_level_t::LOG_LEVEL_TRACE);

        udp_client_server::udp_server s("127.0.0.1", 4041);

        std::cerr << "socket = "
                  << s.get_socket()
                  << std::endl;

        return 0;
    }
    catch( snap::snap_exception const & e )
    {
        SNAP_LOG_FATAL("Caught a Snap! exception [")(e.what())("].");
    }
    catch( std::exception const & e )
    {
        SNAP_LOG_FATAL("Caught exception [")(e.what())("].");
    }

    return 1;
}

// vim: ts=4 sw=4 et
