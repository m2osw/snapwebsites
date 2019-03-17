// Snap! Websites -- Test Suite main()
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
 * \brief Snap! Websites main unit test.
 *
 * This file includes code common to all our tests. At this time it is
 * mainly the main() function that checks the command line arguments.
 *
 * This test suite uses catch.hpp, for details see:
 *
 *   https://github.com/philsquared/Catch/blob/master/docs/tutorial.md
 */

// Tell catch we want it to add the runner code in this file.
#define CATCH_CONFIG_RUNNER

#include "snap_tests.h"

#include <snapwebsites/qstring_stream.h>

#include <cstring>

#include <stdlib.h>

int main(int argc, char * argv[])
{
    // define program name
    snap_test::progname(argv[0]);
    const auto& pn(snap_test::progname());
    size_t e1(pn.rfind('/'));
    size_t e2(pn.rfind('\\'));
    if(e1 != std::string::npos && e1 > e2)
    {
        snap_test::progname( pn.substr(e1 + 1) ); // LCOV_EXCL_LINE
        snap_test::progdir( pn.substr(0, e1) );
    }
    else if(e2 != std::string::npos && e2 > e1)
    {
        snap_test::progname( pn.substr(e2 + 1) ); // LCOV_EXCL_LINE
        snap_test::progdir( pn.substr( 0, e2 ) );
    }
    if(snap_test::progdir().empty())
    {
        // no directory in argv[0], use "." as the default
        snap_test::progdir( "." );
    }

    unsigned int seed(static_cast<unsigned int>(time(nullptr)));
    bool help(false);
    //
    std::vector<char*> catch_args;
    catch_args.push_back(argv[0]);
    //
    for( int i(1); i < argc; ++i )
    {
        const std::string arg( argv[i] );
        if( arg == "-h" || arg == "--help" )
        {
            help = true; // LCOV_EXCL_LINE
            catch_args.push_back( argv[i] );
        }
        else if( arg == "--seed" )
        {
            if( i + 1 >= argc ) // LCOV_EXCL_LINE
            {
                std::cerr << "error: --seed needs to be followed by the actual seed." << std::endl; // LCOV_EXCL_LINE
                exit(1); // LCOV_EXCL_LINE
            }
            seed = atoll(argv[++i]); // LCOV_EXCL_LINE
        }
        else if( arg == "--host" )
        {
            if( i + 1 >= argc ) // LCOV_EXCL_LINE
            {
                std::cerr << "error: --host needs to be followed by the host name." << std::endl; // LCOV_EXCL_LINE
                exit(1); // LCOV_EXCL_LINE
            }
            snap_test::host( argv[++i] ); // LCOV_EXCL_LINE
        }
        else if( arg == "--verbose" )
        {
            snap_test::verbose( true );
        }
        else if( arg == "--version" )
        {
            std::cout << SNAPWEBSITES_VERSION_STRING << std::endl;
            exit(0);
        }
        else
        {
            catch_args.push_back( argv[i] );
        }
    }
    srand(seed);
    std::cout << snap_test::progname() << "[" << getpid() << "]" << ": version " << SNAPWEBSITES_VERSION_STRING << ", seed is " << seed << std::endl;

    if(help)
    {
        std::cout << std::endl // LCOV_EXCL_LINE
                  << "WARNING: at this point we hack the main() to add the following options:" << std::endl // LCOV_EXCL_LINE
                  << "  --seed <seed>             to force the seed at the start of the process to a specific value (i.e. to reproduce the exact same test over and over again)" << std::endl // LCOV_EXCL_LINE
                  << "  --verbose                 request for the errors to always be printed in std::cerr" << std::endl // LCOV_EXCL_LINE
                  << "  --version                 print out the version of this test and exit with 0" << std::endl // LCOV_EXCL_LINE
                  << std::endl; // LCOV_EXCL_LINE
    }

    return Catch::Session().run(catch_args.size(), &catch_args[0]);
}

// Local Variables:
// mode: cpp
// indent-tabs-mode: nil
// c-basic-offset: 4
// tab-width: 4
// End:

// vim: ts=4 sw=4 et
