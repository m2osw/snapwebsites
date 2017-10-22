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
 *      Copyright (c) 2012-2017 Made to Order Software Corp.
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

#include "snapbackup.h"
#include "snap_table_list.h"

#include <QCoreApplication>

#include <libdbproxy/qstring_stream.h>

#include <exception>
#include <iostream>

namespace
{
const std::vector<std::string> g_configuration_files; // Empty

const advgetopt::getopt::option g_snapbackup_options[] =
{
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        nullptr,
        nullptr,
        "Usage: %p [-<opt>]",
        advgetopt::getopt::argument_mode_t::help_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        nullptr,
        nullptr,
        "where -<opt> is one or more of:",
        advgetopt::getopt::argument_mode_t::help_argument
    },
    {
        '?',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "help",
        nullptr,
        "show this help output",
        advgetopt::getopt::argument_mode_t::no_argument
    },
    {
        'n',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "context-name",
        "snap_websites",
        "name of the context (or keyspace) to dump/restore (defaults to 'snap_websites')",
        advgetopt::getopt::argument_mode_t::optional_argument
    },
    {
        'd',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "dump-context",
        nullptr,
        "dump the snapwebsites context to SQLite database",
        advgetopt::getopt::argument_mode_t::required_argument
    },
    {
        'T',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "tables",
        nullptr,
        "specify the list of tables to dump to SQLite database, or restore from SQLite to Cassandra",
        advgetopt::getopt::argument_mode_t::required_multiple_argument
    },
    {
        'r',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "restore-context",
        nullptr,
        "restore the snapwebsites context from SQLite database (requires confirmation)",
        advgetopt::getopt::argument_mode_t::optional_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "drop-context",
        nullptr,
        "drop the snap_websites keyspace",
        advgetopt::getopt::argument_mode_t::no_argument
    },
    {
        'c',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "count",
        "100",
        "specify the page size in rows (default 100)",
        advgetopt::getopt::argument_mode_t::optional_argument
    },
    {
        'l',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "low-watermark",
        "0",
        "specify the low water mark bytes (default 0)",
        advgetopt::getopt::argument_mode_t::required_argument
    },
    {
        'm',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "high-watermark",
        "65536",
        "specify the high water mark bytes (default 0)",
        advgetopt::getopt::argument_mode_t::required_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "yes-i-know-what-im-doing",
        nullptr,
        "Force the dropping of context and overwriting of database, without warning and stdin prompt. Only use this if you know what you're doing!",
        advgetopt::getopt::argument_mode_t::no_argument
    },
    {
        'f',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "force-schema-creation",
        nullptr,
        "Force the creation of the context even if it already exists (default ignore)",
        advgetopt::getopt::argument_mode_t::no_argument
    },
    {
        'h',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "host",
        "localhost",
        "host IP address or name (defaults to localhost)",
        advgetopt::getopt::argument_mode_t::optional_argument
    },
    {
        'p',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "port",
        "9042",
        "port on the host to connect to (defaults to 9042)",
        advgetopt::getopt::argument_mode_t::optional_argument
    },
    {
        's',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "use-ssl",
        nullptr,
        "communicate with the Cassandra server using SSL encryption (defaults to false).",
        advgetopt::getopt::argument_mode_t::no_argument
    },
    {
        'V',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "version",
        nullptr,
        "show the version of %p and exit",
        advgetopt::getopt::argument_mode_t::no_argument
    },
    {
        '\0',
        0,
        nullptr,
        nullptr,
        nullptr,
        advgetopt::getopt::argument_mode_t::end_of_options
    }
};

bool confirm_drop_check( const QString& msg )
{
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
    app.setApplicationVersion( SNAPBACKUP_VERSION_STRING );
    app.setOrganizationDomain( "snapwebsites.org"        );
    app.setOrganizationName  ( "M2OSW"                   );

    try
    {
        getopt_ptr_t opt( new advgetopt::getopt( argc, argv, g_snapbackup_options, g_configuration_files, nullptr ) );

        if( opt->is_defined("help") )
        {
            opt->usage( advgetopt::getopt::status_t::error, "snapbackup" );
            //snap::NOTREACHED();
        }
        else if( opt->is_defined("version") )
        {
            std::cerr << SNAPBACKUP_VERSION_STRING << std::endl;
            exit(0);
        }

        snapTableList::initList();

        snapbackup  s(opt);
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
            throw std::runtime_error("You must specify --drop-context, --dump-context, or --restore-context!");
        }
    }
    catch(std::exception const& e)
    {
        std::cerr << "snapbackup: exception: " << e.what() << std::endl;
        retval = 1;
    }

    return retval;
}

// vim: ts=4 sw=4 et
