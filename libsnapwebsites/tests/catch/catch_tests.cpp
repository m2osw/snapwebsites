// libsnapwebsites -- Test Suite
// Copyright (C) 2015-2017  Made to Order Software Corp.
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

// C++ lib
//
#include <cstring>

// C lib
//
#include <stdlib.h>


namespace libsnapwebsites_test
{

char * g_progname;

} // libsnapwebsites_test namespace


namespace
{
    struct UnitTestCLData
    {
        bool        help = false;
        bool        version = false;
        int         seed = 0;
        int         tcp_port = -1;
    };

    void remove_from_args( std::vector<std::string> & vect, std::string const & long_opt, std::string const & short_opt )
    {
        auto iter = std::find_if( vect.begin(), vect.end(), [long_opt, short_opt]( std::string const & arg )
        {
            return arg == long_opt || arg == short_opt;
        });

        if( iter != vect.end() )
        {
            auto next_iter = iter;
            vect.erase( ++next_iter );
            vect.erase( iter );
        }
    }
}


int main(int argc, char * argv[])
{
    // define program name
    //
    libsnapwebsites_test::g_progname = argv[0];
    char * e(strrchr(libsnapwebsites_test::g_progname, '/'));
    if(e)
    {
        libsnapwebsites_test::g_progname = e + 1; // LCOV_EXCL_LINE
    }

    UnitTestCLData configData;
    Catch::Clara::CommandLine<UnitTestCLData> cli;

    cli["-?"]["-h"]["--help"]
        .describe("display usage information")
        .bind(&UnitTestCLData::help);

    cli["-S"]["--seed"]
        .describe("value to seed the randomizer, if not specified, randomize")
        .bind(&UnitTestCLData::seed, "seed");

    cli["-V"]["--version"]
        .describe("print out the advgetopt library version these unit tests pertain to")
        .bind(&UnitTestCLData::version);

    cli.parseInto( argc, argv, configData );

    // User requested help?
    //
    if(configData.help)
    {
        cli.usage(std::cout, argv[0]); // print out specialized (our additional) help info
        Catch::Session().run(argc, argv); // print out main help
        exit(1);
    }

    // User requested version information?
    //
    if(configData.version)
    {
        std::cout << SNAPWEBSITES_VERSION_STRING << std::endl;
        exit(0);
    }

    // to be able to remove user arguments, we copy them in a vector
    //
    std::vector<std::string> arg_list;
    for(int i(0); i < argc; ++i)
    {
        arg_list.push_back(argv[i]);
    }

    // Seed the system rand() function, by default use time() otherwise
    // use the user specified value and remove it from the list of
    // arguments
    //
    unsigned int seed(static_cast<unsigned int>(time(NULL)));
    if(configData.seed != 0)
    {
        seed = static_cast<unsigned int>(configData.seed);
        remove_from_args(arg_list, "--seed", "-S");
    }
    srand(seed);

    // Give test details, it should always be the first time unless an
    // error occurs, --help, --version are used
    //
    std::cout << libsnapwebsites_test::g_progname
              << "[" << getpid() << "]"
              << ": version " << SNAPWEBSITES_VERSION_STRING
              << ", seed is " << seed << std::endl;

    // Generate a new argv parameter so we can call run() below
    //
    std::vector<char *> new_argv;
    std::for_each(
        arg_list.begin(),
        arg_list.end(),
        [&new_argv](const std::string& arg)
            {
                new_argv.push_back(const_cast<char *>(arg.c_str()));
            });

    return Catch::Session().run(static_cast<int>(new_argv.size()), &new_argv[0]);
}

// vim: ts=4 sw=4 et
