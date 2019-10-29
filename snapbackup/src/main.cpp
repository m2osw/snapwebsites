/*
 * Text:
 *      snapbackup/src/main.cpp
 *
 * Description:
 *      Reads and describes a Snap database. This ease checking out the
 *      current content of the database as the cassandra-cli tends to
 *      show everything in hexadecimal number which is quite unpractical.
 *      Now we do it that way for runtime speed which is much more important
 *      than readability by humans, but we still want to see the data in an
 *      easy practical way which this tool offers.
 *
 *      This contains the main() function.
 *
 * License:
 *      Copyright (c) 2012-2019  Made to Order Software Corp.  All Rights Reserved
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
#include "snapbackup.h"
#include "snap_table_list.h"

// libsnapwebsites lib
//
#include <libdbproxy/qstring_stream.h>
#include <snapwebsites/version.h>

// advgetopt lib
//
#include <advgetopt/exception.h>

// boost lib
//
#include <boost/preprocessor/stringize.hpp>

// Qt lib
//
#include <QCoreApplication>

// C++ lib
//
#include <exception>
#include <iostream>



namespace
{

const advgetopt::option g_snapbackup_options[] =
{
    {
        '?',
        advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_FLAG | advgetopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "help",
        nullptr,
        "show this help output",
        nullptr
    },
    {
        'n',
        advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_REQUIRED,
        "context-name",
        "snap_websites",
        "name of the context (or keyspace) to dump/restore (defaults to 'snap_websites')",
        nullptr
    },
    {
        'd',
        advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_REQUIRED | advgetopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "dump-context",
        nullptr,
        "dump the snapwebsites context to SQLite database",
        nullptr
    },
    {
        'T',
        advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_REQUIRED | advgetopt::GETOPT_FLAG_MULTIPLE,
        "tables",
        nullptr,
        "specify the list of tables to dump to SQLite database, or restore from SQLite to Cassandra",
        nullptr
    },
    {
        'r',
        advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_REQUIRED | advgetopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "restore-context",
        nullptr,
        "restore the snapwebsites context from SQLite database (requires confirmation)",
        nullptr
    },
    {
        '\0',
        advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_FLAG | advgetopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "drop-context",
        nullptr,
        "drop the snap_websites keyspace",
        nullptr
    },
    {
        'c',
        advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_REQUIRED,
        "count",
        "100",
        "specify the page size in rows (default 100)",
        nullptr
    },
    {
        'l',
        advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_REQUIRED,
        "low-watermark",
        "0",
        "specify the low water mark bytes (default 0)",
        nullptr
    },
    {
        'm',
        advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_REQUIRED,
        "high-watermark",
        "65536",
        "specify the high water mark bytes (default 0)",
        nullptr
    },
    {
        '\0',
        advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_FLAG,
        "yes-i-know-what-im-doing",
        nullptr,
        "Force the dropping of context and overwriting of database, without warning and stdin prompt. Only use this if you know what you're doing!",
        nullptr
    },
    {
        'f',
        advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_FLAG,
        "force-schema-creation",
        nullptr,
        "Force the creation of the context even if it already exists (default ignore)",
        nullptr
    },
    {
        'h',
        advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_REQUIRED,
        "host",
        "localhost",
        "host IP address or name (defaults to localhost)",
        nullptr
    },
    {
        'p',
        advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_REQUIRED,
        "port",
        "9042",
        "port on the host to connect to (defaults to 9042)",
        nullptr
    },
    {
        's',
        advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_FLAG,
        "use-ssl",
        nullptr,
        "communicate with the Cassandra server using SSL encryption (defaults to false).",
        nullptr
    },
    {
        'v',
        advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_FLAG,
        "verbose",
        nullptr,
        "print out various message to console",
        nullptr
    },
    {
        'V',
        advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_FLAG | advgetopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "version",
        nullptr,
        "show the version of %p and exit",
        nullptr
    },
    {
        '\0',
        advgetopt::GETOPT_FLAG_END,
        nullptr,
        nullptr,
        nullptr,
        nullptr
    }
};



// until we have C++20 remove warnings this way
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
advgetopt::options_environment const g_snapbackup_options_environment =
{
    .f_project_name = "snapwebsites",
    .f_options = g_snapbackup_options,
    .f_options_files_directory = nullptr,
    .f_environment_variable_name = nullptr,
    .f_configuration_files = nullptr,
    .f_configuration_filename = nullptr,
    .f_configuration_directories = nullptr,
    .f_environment_flags = advgetopt::GETOPT_ENVIRONMENT_FLAG_PROCESS_SYSTEM_PARAMETERS,
    .f_help_header = "Usage: %p [-<opt>] ...\n"
                     "where -<opt> is one or more of:",
    .f_help_footer = "%c",
    .f_version = SNAPWEBSITES_VERSION_STRING,
    .f_license = "GNU GPL v2",
    .f_copyright = "Copyright (c) 2012-"
                   BOOST_PP_STRINGIZE(UTC_BUILD_YEAR)
                   " by Made to Order Software Corporation -- All Rights Reserved",
    //.f_build_date = UTC_BUILD_DATE,
    //.f_build_time = UTC_BUILD_TIME
};
#pragma GCC diagnostic pop








bool confirm_drop_check( const QString& msg )
{
    // background task can't be interactive
    //
    if(!isatty(fileno(stdout)))
    {
        return false;
    }

    std::cout << "WARNING! " << msg << std::endl
              << "         This action is IRREVERSIBLE!" << std::endl
              << std::endl
              << "Make sure you know what you are doing and have appropriate backups" << std::endl
              << "before proceeding!" << std::endl
              << std::endl
              << "Are you really sure you want to do this?" << std::endl
              << "(type in \"Yes I know what I'm doing!\" and press ENTER): "
                 ;
    std::string input;
    std::getline( std::cin, input );
    bool const confirm( (input == "Yes I know what I'm doing!") );
    if( !confirm )
    {
        std::cerr << "warning: Not overwriting database, so exiting." << std::endl;
    }
    return confirm;
}

}
// no name namespace

int main(int argc, char *argv[])
{
    int retval = 0;

    QCoreApplication app(argc, argv);
    app.setApplicationName   ( "snapbackup"              );
    app.setApplicationVersion( SNAPWEBSITES_VERSION_STRING );
    app.setOrganizationDomain( "snapwebsites.org"        );
    app.setOrganizationName  ( "M2OSW"                   );

    try
    {
        getopt_ptr_t opt( new advgetopt::getopt( g_snapbackup_options_environment, argc, argv ) );

        snapTableList::initList();

        snapbackup s(opt);
        s.connectToCassandra();

        if( opt->is_defined("drop-context") )
        {
            if( opt->is_defined("yes-i-know-what-im-doing")
                || confirm_drop_check( "This command is about to drop the Snap context on the server completely!" ) )
            {
                s.dropContext();
            }
        }
        else if( opt->is_defined("dump-context") )
        {
            s.dumpContext();
        }
        else if( opt->is_defined("restore-context") )
        {
            if( opt->is_defined("yes-i-know-what-im-doing")
                || confirm_drop_check( "This command is about to overwrite the Snap context on the server!" ) )
            {
                s.restoreContext();
            }
        }
        else
        {
            throw std::runtime_error("You must specify one of --drop-context, --dump-context, or --restore-context!");
        }
    }
    catch( advgetopt::getopt_exit const & except )
    {
        return except.code();
    }
    catch(std::exception const & e)
    {
        std::cerr << "snapbackup: exception: " << e.what() << std::endl;
        retval = 1;
    }

    return retval;
}

// vim: ts=4 sw=4 et
