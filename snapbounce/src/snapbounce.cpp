/////////////////////////////////////////////////////////////////////////////////
// Snap Bounced Email Processor

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
//
// This server reads in a configuration file and keeps specified services running.
// When signaled, it will terminate those services cleanly.
/////////////////////////////////////////////////////////////////////////////////

#include "version.h"

// snapwebsites lib
//
#include <snapwebsites/log.h>
#include <snapwebsites/not_reached.h>
#include <snapwebsites/qstring_stream.h>
#include <snapwebsites/snap_cassandra.h>
#include <snapwebsites/snap_config.h>
#include <snapwebsites/snap_exception.h>
#include <snapwebsites/snap_thread.h>
#include <snapwebsites/snapwebsites.h>

// contrib lib
//
#include <advgetopt/advgetopt.h>
#include <QtCassandra/QCassandraContext.h>
#include <QtCassandra/QCassandraTable.h>

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

// included last
//
#include <snapwebsites/poison.h>



namespace
{
    /** \brief List of configuration files.
     *
     * This variable is used as a list of configuration files. It may be
     * empty.
     */
    std::vector<std::string> const g_configuration_files;
    //{
    //    "@snapwebsites@",       // project name
    //    "/etc/snapwebsites/snapbounce.conf"
    //};

    /** \brief Command line options.
     *
     * This table includes all the options supported by the server.
     */
    advgetopt::getopt::option const g_snapbounce_options[] =
    {
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            NULL,
            NULL,
            "Usage: %p [-<opt>] <start|restart|stop>",
            advgetopt::getopt::argument_mode_t::help_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            NULL,
            NULL,
            "where -<opt> is one or more of:",
            advgetopt::getopt::argument_mode_t::help_argument
        },
        {
            'h',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            "help",
            nullptr,
            "[optional] Show usage and exit.",
            advgetopt::getopt::argument_mode_t::no_argument
        },
        {
            'n',
            advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
            "nolog",
            nullptr,
            "[optional] Only output to the console, not the syslog.",
            advgetopt::getopt::argument_mode_t::no_argument
        },
        {
            'c',
            advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            "config",
            nullptr,
            "[optional] Configuration file from which to get cassandra server details.",
            advgetopt::getopt::argument_mode_t::optional_argument
        },
        {
            'v',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            "version",
            nullptr,
            "[optional] show the version of %p and exit",
            advgetopt::getopt::argument_mode_t::no_argument
        },
        {
            's',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            "sender",
            nullptr,
            "[required] Sender of the email.",
            advgetopt::getopt::argument_mode_t::required_argument
        },
        {
            'r',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            "recipient",
            nullptr,
            "[required] Intended recipient of the email.",
            advgetopt::getopt::argument_mode_t::required_argument
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
}
//namespace


class snap_bounce
{
public:
    typedef std::shared_ptr<snap_bounce> pointer_t;

    static void create_instance( int argc, char * argv[] );
    static pointer_t instance();
    ~snap_bounce();

    void read_stdin();
    void store_email();

private:
    static pointer_t     f_instance;
    advgetopt::getopt    f_opt;
    snap::snap_config    f_config;
    snap::snap_cassandra f_cassandra;
    //std::string          f_recipient;
    QStringList          f_email_body;

    snap_bounce( int argc, char *argv[] );

    void usage();
};


snap_bounce::pointer_t snap_bounce::f_instance;


snap_bounce::snap_bounce( int argc, char * argv[] )
    : f_opt(argc, argv, g_snapbounce_options, g_configuration_files, "SNAPBOUNCE_OPTIONS")
    , f_config( "snapserver" ) // right now snapbounce does not really use any .conf data, it's just a filter, so we specify snapserver as a "fallback"
{
    if(f_opt.is_defined("version"))
    {
        std::cerr << SNAPBOUNCE_VERSION_STRING << std::endl;
        exit(0);
    }

    if( f_opt.is_defined( "help" ) || !f_opt.is_defined( "sender" ) || !f_opt.is_defined( "recipient" ) )
    {
        usage();
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
    f_instance.reset( new snap_bounce( argc, argv ) );
    Q_ASSERT(f_instance);
}


snap_bounce::pointer_t snap_bounce::instance()
{
    if( !f_instance )
    {
        throw std::invalid_argument( "snap_bounce instance must be created with create_instance()!" );
    }
    return f_instance;
}


void snap_bounce::usage()
{
    f_opt.usage( advgetopt::getopt::status_t::no_error, "snapbounce" );
    //throw std::invalid_argument( "usage" );
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
    QtCassandra::QCassandraContext::pointer_t context( f_cassandra.get_snap_context() );

    QtCassandra::QCassandraTable::pointer_t table(context->findTable("emails"));
    if(!table)
    {
        // We do not want to bother with trying to create the "emails" table.
        // If it is not there, then we will just have to lose this email for now.
        return;
    }

    QByteArray key;
    // get current time first so rows get sorted by date
    int64_t now(snap::snap_child::get_current_date());
    QtCassandra::appendInt64Value(key, now); // warning: use our append function so the int is inserted in big endian
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
    catch( snap::snap_exception const& except )
    {
        SNAP_LOG_FATAL("snap_bounce: snap_exception caught! ")(except.what());
        retval = 1;
    }
    catch( std::invalid_argument const& std_except )
    {
        SNAP_LOG_FATAL("snap_bounce: invalid argument: ")(std_except.what());
        retval = 1;
    }
    catch( std::exception const& std_except )
    {
        SNAP_LOG_FATAL("snap_bounce: std::exception caught! ")(std_except.what());
        retval = 1;
    }
    catch( ... )
    {
        SNAP_LOG_FATAL("snap_bounce: unknown exception caught!");
        retval = 1;
    }

    return 0;
}

// vim: ts=4 sw=4 et
