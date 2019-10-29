/////////////////////////////////////////////////////////////////////////////////
// Snap Bounced Email Processor

// Copyright (c) 2011-2019  Made to Order Software Corp.  All Rights Reserved
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
// This server reads in a configuration file and keeps specified services running.
// When signaled, it will terminate those services cleanly.
/////////////////////////////////////////////////////////////////////////////////

#include "version.h"


// snapwebsites lib
//
#include <snapwebsites/log.h>
#include <snapwebsites/qstring_stream.h>
#include <snapwebsites/snap_cassandra.h>
#include <snapwebsites/snap_config.h>
#include <snapwebsites/snap_exception.h>
#include <snapwebsites/snapwebsites.h>


// snapdev lib
//
#include <snapdev/not_reached.h>


// advgetopt lib
//
#include <advgetopt/advgetopt.h>
#include <advgetopt/exception.h>


// libdbproxy lib
//
#include <libdbproxy/context.h>
#include <libdbproxy/table.h>


// Qt lib
//
#include <QFile>
#include <QTime>


// C lib
//
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <uuid/uuid.h>


// C++ lib
//
#include <exception>
#include <memory>
#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>


// last include
//
#include <snapdev/poison.h>



namespace
{



/** \brief Command line options.
 *
 * This table includes all the options supported by the server.
 */
advgetopt::option const g_snapbounce_options[] =
{
    {
        'n',
        advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_FLAG | advgetopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
        "nolog",
        nullptr,
        "Only output to the console, not the syslog.",
        nullptr
    },
    {
        'c',
        advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::GETOPT_FLAG_REQUIRED | advgetopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "config",
        nullptr,
        "Configuration file from which to get cassandra server details.",
        nullptr
    },
    {
        's',
        advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_REQUIRED | advgetopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "sender",
        nullptr,
        "Sender of the email [required].",
        nullptr
    },
    {
        'r',
        advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_REQUIRED | advgetopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "recipient",
        nullptr,
        "Intended recipient of the email [required].",
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
advgetopt::options_environment const g_snapbounce_options_environment =
{
    .f_project_name = "snapwebsites",
    .f_options = g_snapbounce_options,
    .f_options_files_directory = nullptr,
    .f_environment_variable_name = "SNAPBOUNCE_OPTIONS",
    .f_configuration_files = nullptr,
    .f_configuration_filename = nullptr,
    .f_configuration_directories = nullptr,
    .f_environment_flags = advgetopt::GETOPT_ENVIRONMENT_FLAG_PROCESS_SYSTEM_PARAMETERS,
    .f_help_header = "Usage: %p [-<opt>]\n"
                     "where -<opt> is one or more of:",
    .f_help_footer = "%c",
    .f_version = SNAPWEBSITES_VERSION_STRING,
    .f_license = "GNU GPL v2",
    .f_copyright = "Copyright (c) 2013-"
                   BOOST_PP_STRINGIZE(UTC_BUILD_YEAR)
                   " by Made to Order Software Corporation -- All Rights Reserved",
    //.f_build_date = UTC_BUILD_DATE,
    //.f_build_time = UTC_BUILD_TIME
};
#pragma GCC diagnostic pop




}
//namespace


class snap_bounce
{
public:
    typedef std::shared_ptr<snap_bounce> pointer_t;

    static void             create_instance( int argc, char * argv[] );
    static pointer_t        instance();

                            ~snap_bounce();

    void                    read_stdin();
    void                    store_email();

private:
                            snap_bounce( int argc, char *argv[] );

    static pointer_t        g_instance;

    advgetopt::getopt       f_opt;
    snap::snap_config       f_config = snap::snap_config();
    snap::snap_cassandra    f_cassandra = snap::snap_cassandra();
    //std::string             f_recipient;
    snap::snap_string_list  f_email_body = snap::snap_string_list();
};


snap_bounce::pointer_t snap_bounce::g_instance;


snap_bounce::snap_bounce( int argc, char * argv[] )
    : f_opt(g_snapbounce_options_environment, argc, argv)
    , f_config( "snapserver" ) // right now snapbounce does not really use any .conf data, it's just a filter, so we specify snapserver as a "fallback"
{
    if( !f_opt.is_defined( "sender" ) || !f_opt.is_defined( "recipient" ) )
    {
        std::cerr << "error: the --sender and --recipient command line arguments are required." << std::endl;
        std::cerr << f_opt.usage(advgetopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR);
        exit(1);
    }

    snap::logging::set_progname(f_opt.get_program_name());

    if( f_opt.is_defined( "nolog" ) )
    {
        snap::logging::configure_console();
    }
    else
    {
        snap::logging::configure_syslog();
    }

    if(f_opt.is_defined("config"))
    {
        f_config.set_configuration_path( f_opt.get_string("config") );
    }
}


snap_bounce::~snap_bounce()
{
    // Empty
}


void snap_bounce::create_instance( int argc, char * argv[] )
{
    g_instance.reset( new snap_bounce( argc, argv ) );
    Q_ASSERT(g_instance);
}


snap_bounce::pointer_t snap_bounce::instance()
{
    if( !g_instance )
    {
        throw std::invalid_argument( "snap_bounce instance must be created with create_instance()!" );
    }
    return g_instance;
}




void snap_bounce::read_stdin()
{
    f_email_body << QString("sender: %1").arg(f_opt.get_string("sender").c_str());
    f_email_body << QString("recipient: %1").arg(f_opt.get_string("recipient").c_str());
    f_email_body << "";
    while( (std::cin.rdstate() & std::ifstream::eofbit) == 0 )
    {
        std::string line;
        std::getline( std::cin, line );
        f_email_body << line.c_str();
    }
}


// I now create a key with `date + uuid` so the columns get sorted
// by date (i.e. we will always handle the oldest bounces first.)
//namespace
//{
//    QString generate_uuid()
//    {
//        uuid_t uuid;
//        uuid_generate_random( uuid );
//        char unique_key[37];
//        uuid_unparse( uuid, unique_key );
//        return unique_key;
//    }
//}


void snap_bounce::store_email()
{
    f_cassandra.connect();
    if( !f_cassandra.is_connected() )
    {
        throw snap::snap_exception( "Cannot connect to Cassandra!" );
    }

    // Send f_email_body's contents to cassandra, specifically in the "emails/bounced" table/row.
    //
    libdbproxy::context::pointer_t context( f_cassandra.get_snap_context() );

    libdbproxy::table::pointer_t table(context->findTable("emails"));
    if(!table)
    {
        // We do not want to bother with trying to create the "emails" table.
        // If it is not there, then we will just have to lose this email for now.
        return;
    }

    QByteArray key;
    // get current time first so rows get sorted by date
    int64_t now(snap::snap_child::get_current_date());
    libdbproxy::appendInt64Value(key, now); // warning: use our append function so the int is inserted in big endian
    // make sure it is unique
    uuid_t uuid;
    uuid_generate_random( uuid );
    key.append(reinterpret_cast<char const *>(uuid), sizeof(uuid));

    (*table)["bounced_raw"][key] = f_email_body.join("\n");

    // TODO: enable once we have our snapcommunicator
    //snap_communicator_message message;
    //message.set_service("sendmail");
    //message.set_command("PING");
    //message.add_parameter("bounce", "0"); // at this point the existance of the parameter is the important part in this message
    //snap::snap_communicator::snap_udp_server_message_connection::send_message("127.0.0.1", 4040, message);
}


int main(int argc, char *argv[])
{
    int retval = 0;

    try
    {
        // First, create the static snap_bounce object
        //
        snap_bounce::create_instance( argc, argv );

        // Now run our processes!
        //
        snap_bounce::pointer_t bounce( snap_bounce::instance() );
        bounce->read_stdin();
        bounce->store_email();
    }
    catch( advgetopt::getopt_exit const & except )
    {
        return except.code();
    }
    catch( snap::snap_exception const & except )
    {
        SNAP_LOG_FATAL("snap_bounce: snap_exception caught! ")(except.what());
        retval = 1;
    }
    catch( std::invalid_argument const & std_except )
    {
        SNAP_LOG_FATAL("snap_bounce: invalid argument: ")(std_except.what());
        retval = 1;
    }
    catch( std::exception const & std_except )
    {
        SNAP_LOG_FATAL("snap_bounce: std::exception caught! ")(std_except.what());
        retval = 1;
    }
    catch( ... )
    {
        SNAP_LOG_FATAL("snap_bounce: unknown exception caught!");
        retval = 1;
    }

    return retval;
}

// vim: ts=4 sw=4 et
