/*
 * Text:
 *      main.cpp
 *
 * Description:
 *      This contains the main() function.
 *
 * License:
 *      Copyright (c) 2016-2017 Made to Order Software Corp.
 *
 *      http://snapwebsites.org/
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

#include "snaplog.h"
#include "version.h"

// snapwebsites lib
//
#include <snapwebsites/log.h>


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
        snaplog logger( argc, argv );

        SNAP_LOG_INFO("--------------------------------- snaplog v" SNAPLOG_VERSION_STRING " started on ")(logger.server_name());

        // Now run!
        //
        logger.run();

        // exit normally (i.e. we received a STOP message on our
        // connection with the Snap! Communicator service.)
        //
        return 0;
    }
    catch( snap::snap_exception const & e )
    {
        SNAP_LOG_FATAL("snaplog: snap_exception caught! ")(e.what());
        if(g_isatty)
        {
            std::cerr << "snaplog: snap_exception caught! " << e.what() << std::endl;
        }
    }
    catch( std::invalid_argument const & e )
    {
        SNAP_LOG_FATAL("snaplog: invalid argument: ")(e.what());
        if(g_isatty)
        {
            std::cerr << "snaplog: invalid argument: " << e.what() << std::endl;
        }
    }
    catch( std::exception const & e )
    {
        SNAP_LOG_FATAL("snaplog: std::exception caught! ")(e.what());
        if(g_isatty)
        {
            std::cerr << "snaplog: std::exception caught! " << e.what() << std::endl;
        }
    }
    catch( ... )
    {
        SNAP_LOG_FATAL("snaplog: unknown exception caught!");
        if(g_isatty)
        {
            std::cerr << "snaplog: unknown exception caught! " << std::endl;
        }
    }

    return 1;
}

// vim: ts=4 sw=4 et
