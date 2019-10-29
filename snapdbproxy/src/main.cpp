/*
 * Text:
 *      snapdbproxy/src/main.cpp
 *
 * Description:
 *      Proxy database access for two main reasons:
 *
 *      1. keep connections between this computer and the database
 *         computer open (i.e. opening remote TCP connections taken
 *         "much" longer than opening local connections.)
 *
 *      2. remove threads being forced on us by the C/C++ driver from
 *         cassandra (this causes problems with the snapserver that
 *         uses fork() to create the snap_child processes.)
 *
 *      This contains the main() function.
 *
 * License:
 *      Copyright (c) 2016-2019  Made to Order Software Corp.  All Rights Reserved
 *
 *      https://snapwebsites.org/
 *      contact@m2osw.com
 *
 *      Permission is hereby granted, free of charge, to any person obtaining a
 *      copy of this software and associated documentation files (the
 *      "Software"), to deal in the Software without restriction, including
 *      without limitation the rights to use, copy, modify, merge, publish,
 *      distribute, sublicense, and/or sell copies of the Software, and to
 *      permit persons to whom the Software is furnished to do so, subject to
 *      the following conditions:
 *
 *      The above copyright notice and this permission notice shall be included
 *      in all copies or substantial portions of the Software.
 *
 *      THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 *      OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *      MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *      IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 *      CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 *      TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 *      SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

// self
//
#include "snapdbproxy.h"
#include "version.h"

// snapwebsites lib
//
#include <snapwebsites/log.h>

// advgetopt library
//
#include <advgetopt/exception.h>



/** \brief Define whether the standard output stream is a TTY.
 *
 * This function defines whether 'stderr' is a TTY or not. If not
 * we assume that we were started as a deamon and we do not spit
 * out errors in stderr. If it is a TTY, then we also print a
 * message in the console making it easier to right away know
 * that the tool detected an error and did not start in the
 * background.
 */
bool g_isatty = false;


int main(int argc, char * argv[])
{
    g_isatty = isatty(STDERR_FILENO);

    try
    {
        // create an instance of the snap_firewall object
        //
        snapdbproxy dbproxy(argc, argv);

        SNAP_LOG_INFO("--------------------------------- snapdbproxy v" SNAPDBPROXY_VERSION_STRING " started on ")(dbproxy.server_name());

        // Now run!
        //
        dbproxy.run();

        // exit normally (i.e. we received a STOP message on our
        // connection with the Snap! Communicator service.)
        //
        return 0;
    }
    catch( advgetopt::getopt_exit const & except )
    {
        return except.code();
    }
    catch(snap::snap_exception const & e)
    {
        SNAP_LOG_FATAL("snapdbproxy: snap_exception caught! ")(e.what());
        if(g_isatty)
        {
            std::cerr << "snapdbproxy: snap_exception caught! " << e.what() << std::endl;
        }
    }
    catch(std::invalid_argument const & e)
    {
        SNAP_LOG_FATAL("snapdbproxy: invalid argument: ")(e.what());
        if(g_isatty)
        {
            std::cerr << "snapdbproxy: invalid argument: " << e.what() << std::endl;
        }
    }
    catch(std::exception const & e)
    {
        SNAP_LOG_FATAL("snapdbproxy: std::exception caught! ")(e.what());
        if(g_isatty)
        {
            std::cerr << "snapdbproxy: std::exception caught! " << e.what() << std::endl;
        }
    }
    catch(...)
    {
        SNAP_LOG_FATAL("snapdbproxy: unknown exception caught!");
        if(g_isatty)
        {
            std::cerr << "snapdbproxy: unknown exception caught! " << std::endl;
        }
    }

    return 1;
}

// vim: ts=4 sw=4 et
