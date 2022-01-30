/*
 * Description:
 *      Logger for the Snap! system. This service uses snapcommunicator to
 *      listen to all "SNAPLOG" messages. It records each message into a MySQL
 *      database for later retrieval, making reporting a lot easier for
 *      the admin.
 *
 * License:
 *      Copyright (c) 2016-2020  Made to Order Software Corp.  All Rights Reserved
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

// ourselves
//
#include "snaplog.h"
#include "version.h"

// snapwebsites lib
//
#include <snapwebsites/log.h>
#include <snapwebsites/qstring_stream.h>
#include <snapwebsites/dbutils.h>

// 3rd party libs
//
#include <QtCore>
#include <QtSql>
#include <advgetopt/advgetopt.h>

// system (C++)
//
#include <algorithm>
#include <exception>
#include <iostream>
#include <sstream>

namespace
{


const advgetopt::option g_options[] =
{
    {
        'c',
        advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::GETOPT_FLAG_REQUIRED | advgetopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "config",
        nullptr,
        "Configuration file to initialize snaplog.",
        nullptr
    },
    {
        '\0',
        advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_FLAG | advgetopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
        "debug",
        nullptr,
        "Start the snaplog in debug mode.",
        nullptr
    },
    {
        'l',
        advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::GETOPT_FLAG_REQUIRED,
        "logfile",
        nullptr,
        "Full path to the snaplog logfile.",
        nullptr
    },
    {
        'n',
        advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::GETOPT_FLAG_FLAG,
        "nolog",
        nullptr,
        "Only output to the console, not a log file.",
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
advgetopt::options_environment const g_options_environment =
{
    .f_project_name = "snapwebsites",
    .f_group_name = nullptr,
    .f_options = g_options,
    .f_options_files_directory = nullptr,
    .f_environment_variable_name = "SNAPLOG_OPTIONS",
    .f_section_variables_name = nullptr,
    .f_configuration_files = nullptr,
    .f_configuration_filename = nullptr,
    .f_configuration_directories = nullptr,
    .f_environment_flags = advgetopt::GETOPT_ENVIRONMENT_FLAG_PROCESS_SYSTEM_PARAMETERS,
    .f_help_header = "Usage: %p [-<opt>]\n"
                     "where -<opt> is one or more of:",
    .f_help_footer = "%c",
    .f_version = SNAPLOG_VERSION_STRING,
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




/** \class snaplog
 * \brief Class handling the proxying of the database requests and answers.
 *
 * This class is used to proxy messages from our other parts and send
 * these messages to the Cassandra cluster. Once we get an answer from
 * Cassandra, we then send the results back to the client.
 *
 * The application makes use of threads the process each incoming
 * message and send replies. That way multiple clients can all be
 * services "simultaneously."
 */


/** \brief Initializes a snaplog object.
 *
 * This function parses the command line arguments, reads configuration
 * files, setup the logger.
 *
 * It also immediately executes a --help or a --version command line
 * option and exits the process if these are present.
 *
 * \param[in] argc  The number of arguments in the argv array.
 * \param[in] argv  The array of argument strings.
 */
snaplog::snaplog(int argc, char * argv[])
    : f_opt( g_options_environment, argc, argv )
    , f_config( "snaplog" )
{
    // read the configuration file
    //
    if(f_opt.is_defined("config"))
    {
        f_config.set_configuration_path( f_opt.get_string("config") );
    }

    // --debug
    //
    f_debug = f_opt.is_defined("debug");

    // local_listen=... from snapcommunicator.conf
    //
    tcp_client_server::get_addr_port(QString::fromUtf8(f_config("snapcommunicator", "local_listen").c_str()), f_communicator_addr, f_communicator_port, "tcp");

    // setup the logger: --nolog, --logfile, or config file log_config
    //
    if(f_opt.is_defined("nolog"))
    {
        snap::logging::configure_console();
    }
    else if(f_opt.is_defined("logfile"))
    {
        snap::logging::configure_logfile( QString::fromUtf8(f_opt.get_string("logfile").c_str()) );
    }
    else
    {
        if(f_config.has_parameter("log_config"))
        {
            // use .conf definition when available
            //
            f_log_conf = f_config["log_config"];
        }
        snap::logging::configure_conffile(f_log_conf);
    }

    if( f_debug )
    {
        // Force the logger level to DEBUG
        // (unless already lower)
        //
        snap::logging::reduce_log_output_level(snap::logging::log_level_t::LOG_LEVEL_DEBUG);
    }

    // get the server name from the snapcommunicator.conf or hostname()
    //
    f_server_name = snap::server::get_server_name();

    // make sure there are no standalone parameters
    if( f_opt.is_defined( "--" ) )
    {
        std::cerr << "error: unexpected parameter found on daemon command line." << std::endl;
        std::cerr << f_opt.usage(advgetopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR);
        exit(1);
        snapdev::NOT_REACHED();
    }
}


/** \brief Clean up the snap dbproxy object.
 *
 * This function is used to do some clean up of the snaplog
 * environment.
 */
snaplog::~snaplog()
{
}


/** \brief Retrieve the server name.
 *
 * This function returns a copy of the server name. Since the constructor
 * defines the server name, it is available at all time after that.
 *
 * \return The server name.
 */
std::string snaplog::server_name() const
{
    return f_server_name;
}


/** \brief Start the Snap! Communicator and wait for events.
 *
 * This function initializes the snaplog object further and then
 * listens for events.
 *
 * This specific daemon listens for SNAPLOG events from snapcommunicator.
 */
void snaplog::run()
{
    // Stop on these signals, log them, then terminate.
    //
    signal( SIGCHLD, snaplog::sighandler );
    signal( SIGSEGV, snaplog::sighandler );
    signal( SIGBUS,  snaplog::sighandler );
    signal( SIGFPE,  snaplog::sighandler );
    signal( SIGILL,  snaplog::sighandler );
    signal( SIGTERM, snaplog::sighandler );
    signal( SIGINT,  snaplog::sighandler );
    signal( SIGQUIT, snaplog::sighandler );

    // ignore console signals
    //
    signal( SIGTSTP,  SIG_IGN );
    signal( SIGTTIN,  SIG_IGN );
    signal( SIGTTOU,  SIG_IGN );

    // initialize the communicator and its connections
    //
    f_communicator = snap::snap_communicator::instance();

    // capture Ctrl-C (SIGINT)
    //
    f_interrupt.reset(new snaplog_interrupt(this));
    f_communicator->add_connection(f_interrupt);

    // create a messenger to communicate with the snapcommunicator process
    //
    f_messenger = std::make_shared<snaplog_messenger>(this, f_communicator_addr.toUtf8().data(), f_communicator_port);
    f_communicator->add_connection(f_messenger);

    // create a timer, it will immediately kick in and attempt a connection
    // to Cassandra; if it fails, it will continue to tick until it works.
    //
    f_timer = std::make_shared<snaplog_timer>(this);
    f_communicator->add_connection(f_timer);

    // now run our listening loop
    //
    f_communicator->run();

    if(f_force_restart)
    {
        // by exiting with 1 systemd thinks we have failed and restarts
        // us automatically...
        //
        exit(1);
    }
}


/** \brief A static function to capture various signals.
 *
 * This function captures unwanted signals like SIGSEGV and SIGILL.
 *
 * The handler logs the information and then the service exists.
 * This is done mainly so we have a chance to debug problems even
 * when it crashes on a remote server.
 *
 * \warning
 * The signals are setup after the construction of the snaplog
 * object because that is where we initialize the logger.
 *
 * \param[in] sig  The signal that was just emitted by the OS.
 */
void snaplog::sighandler( int sig )
{
    QString signame;
    bool show_stack_output(true);
    switch( sig )
    {
    case SIGSEGV:
        signame = "SIGSEGV";
        break;

    case SIGBUS:
        signame = "SIGBUS";
        break;

    case SIGFPE:
        signame = "SIGFPE";
        break;

    case SIGILL:
        signame = "SIGILL";
        break;

    case SIGTERM:
        signame = "SIGTERM";
        show_stack_output = false;
        break;

    case SIGINT:
        signame = "SIGINT";
        show_stack_output = false;
        break;

    case SIGQUIT:
        signame = "SIGQUIT";
        show_stack_output = false;
        break;

    default:
        signame = "UNKNOWN";
        break;

    }

    if(show_stack_output)
    {
        snap::snap_exception_base::output_stack_trace();
    }
    SNAP_LOG_FATAL("Fatal signal caught: ")(signame);

    // Exit with error status
    //
    ::exit( 1 );
    snapdev::NOT_REACHED();
}


void snaplog::process_timeout()
{
    try
    {
        SNAP_LOG_TRACE("Attempting to connect to MySQL database");

        QSqlDatabase db = QSqlDatabase::addDatabase( "QMYSQL" );
        if( !db.isValid() )
        {
            std::string const error( "QMYSQL database is not valid for some reason!" );
            SNAP_LOG_ERROR(error);
            throw std::runtime_error(error.c_str());
        }

        if( QSqlDatabase::database().isOpen() )
        {
            QSqlDatabase::database().close();
        }
        //
        db.setHostName     ( "localhost" );
        db.setUserName     ( "snaplog"   );
        db.setPassword     ( "snaplog"   );
        db.setDatabaseName ( "snaplog"   );
        if( !db.open() )
        {
            std::string const error( "Cannot open MySQL database snaplog!" );
            SNAP_LOG_ERROR( error );
            throw std::runtime_error(error.c_str());
        }

        // the connection succeeded, turn off the timer we do not need
        // it for now...
        //
        f_timer->set_enable(false);

        // reset the delay to about 1 second
        // (we use 1.625 so that way we will have 1s, 3s, 7s, 15s, 30s, 60s
        // and thus 1 minute.)
        //
        f_mysql_connect_timer_index = 1.625f;

        mysql_ready();
    }
    catch(std::runtime_error const & e)
    {
        SNAP_LOG_WARNING("Cannot connect to MySQL database: retrying... (")(e.what())(")");

        // the connection failed, keep the timeout enabled and try again
        // on the next tick
        //
        no_mysql();

        if(f_mysql_connect_timer_index < 60.0f)
        {
            // increase the delay between attempts up to 1 min.
            //
            f_mysql_connect_timer_index *= 2.0f;
        }
    }
}


void snaplog::mysql_ready()
{
    SNAP_LOG_INFO("MySQL database is ready to receive requests.");

    // TODO: We need something to do. Set a flag? We don't need to send
    //       anything across snapcommunicator like snapdbproxy does.
}


void snaplog::no_mysql()
{
    SNAP_LOG_TRACE("no_mysql() called.");

    // if still marked as open by Qt, make sure to close the database
    // since this function says that it's closed!
    //
    if( QSqlDatabase::database().isOpen() )
    {
        QSqlDatabase::database().close();
    }

    if(f_timer != nullptr)
    {
        f_timer->set_enable( true );
        f_timer->set_timeout_delay(static_cast<int64_t>(f_mysql_connect_timer_index) * 1000000LL);
    }
}


void snaplog::add_message_to_db( snap::snap_communicator_message const & message )
{
    if( !QSqlDatabase::database().isOpen() )
    {
        no_mysql();
        return;
    }

    // add a record to the MySQL database
    //
    auto const all_parms = message.get_all_parameters();

#ifdef _DEBUG
    // this is way too much for a live server and should not be that useful
    //
    SNAP_LOG_TRACE("SNAPLOG command received: server=[")(message.get_server())("], service=[")(message.get_service())("]");
#if 0
    for( auto key : all_parms.keys() )
    {
        SNAP_LOG_TRACE("parm {")(key)("} = [")(all_parms[key])("]");
    }
#endif
#endif

    QString const q_str("INSERT INTO snaplog.log "
            "(server, service, level, msgid, ipaddr, file, line, func, message ) "
            "VALUES "
            "(:server, :service, :level, :msgid, :ipaddr, :file, :line, :func, :message );");
    QSqlQuery q;
    q.prepare( q_str );
    //
    q.bindValue( ":server",  message.get_sent_from_server()    );
    q.bindValue( ":service", message.get_sent_from_service()   );
    q.bindValue( ":level",   all_parms["level"]                );
    q.bindValue( ":msgid",   all_parms["broadcast_msgid"]      );
    q.bindValue( ":ipaddr",  all_parms["broadcast_originator"] );
    q.bindValue( ":file",    all_parms["file"]                 );
    q.bindValue( ":line",    all_parms["line"]                 );
    q.bindValue( ":func",    all_parms["func"]                 );
    q.bindValue( ":message", all_parms["message"]              );
    //
    if( !q.exec() )
    {
        SNAP_LOG_ERROR("Query error! [")(q.lastError().text())("], lastQuery=[")(q.lastQuery())("]");

        // the following will close the database if still open
        //
        no_mysql();
    }
}


/** \brief Process a message received from Snap! Communicator.
 *
 * This function gets called whenever the Snap! Communicator sends
 * us a message. This includes the READY and HELP commands, although
 * the most important one is certainly the STOP command.
 *
 * \todo
 * Convert to using dispatcher.
 *
 * \param[in] message  The message we just received.
 */
void snaplog::process_message(snap::snap_communicator_message const & message)
{
#ifdef _DEBUG
    // this may be very useful to debug snaplog, but on a live system
    // it is a x2 of all the logs (once in their respective file and
    // once in the snaplog.log file) when the idea is to stick the
    // log in MySQL, so go check it there!
    //
    SNAP_LOG_TRACE("received messenger message [")(message.to_message())("] for ")(f_server_name);
#endif

    QString const command(message.get_command());

// TODO: make use of a switch() or even better: a map a la snapinit -- see SNAP-464
//       (I have it written, it uses a map like scheme, we now need to convert all
//       the process_message() in using the new scheme which can calls a separate
//       function for each message you support!)
//       examples: snapwatchdog and snaplock at this time

    if(command == "SNAPLOG")
    {
        add_message_to_db( message );
        return;
    }

    if(command == "LOG")
    {
        // logrotate just rotated the logs, we have to reconfigure
        //
        SNAP_LOG_INFO("Logging reconfiguration.");
        snap::logging::reconfigure();
        return;
    }

    if(command == "STOP")
    {
        // Someone is asking us to leave
        //
        stop(false);
        return;
    }

    if(command == "QUITTING")
    {
        // If we received the QUITTING command, then somehow we sent
        // a message to Snap! Communicator, which is already in the
        // process of quitting... we should get a STOP too, but we
        // can just quit ASAP too
        //
        stop(true);
        return;
    }

    if(command == "READY")
    {
        // Snap! Communicator received our REGISTER command
        //
        f_ready = true;

        if( QSqlDatabase::database().isOpen() )
        {
            mysql_ready();
        }
        return;
    }

    if(command == "RELOADCONFIG")
    {
        f_force_restart = true;
        stop(false);
        return;
    }

    if(command == "HELP")
    {
        // Snap! Communicator is asking us about the commands that we support
        //
        snap::snap_communicator_message reply;
        reply.set_command("COMMANDS");

        // list of commands understood by service
        //
        reply.add_parameter("list", "HELP,LOG,QUITTING,READY,RELOADCONFIG,SNAPLOG,STOP,UNKNOWN");

        f_messenger->send_message(reply);
        return;
    }

    if(command == "UNKNOWN")
    {
        // we sent a command that Snap! Communicator did not understand
        //
        SNAP_LOG_ERROR("we sent unknown command \"")(message.get_parameter("command"))("\" and probably did not get the expected result.");
        return;
    }

    // unknown command is reported and process goes on
    //
    SNAP_LOG_ERROR("unsupported command \"")(command)("\" was received on the connection with Snap! Communicator.");
    {
        snap::snap_communicator_message reply;
        reply.set_command("UNKNOWN");
        reply.add_parameter("command", command);
        f_messenger->send_message(reply);
    }
}

/** \brief Called whenever we receive the STOP command or equivalent.
 *
 * This function makes sure the snaplock exits as quickly as
 * possible.
 *
 * \li Marks the messenger as done.
 * \li UNREGISTER from snapcommunicator.
 * \li Remove the listener.
 *
 * \note
 * If the g_messenger is still in place, then just sending the
 * UNREGISTER is enough to quit normally. The socket of the
 * g_messenger will be closed by the snapcommunicator server
 * and we will get a HUP signal. However, we get the HUP only
 * because we first mark the messenger as done.
 *
 * \param[in] quitting  Set to true if we received a QUITTING message.
 */
void snaplog::stop(bool quitting)
{
    SNAP_LOG_INFO("Stopping server.");

    if(f_messenger != nullptr)
    {
        if(quitting || !f_messenger->is_connected())
        {
            // turn off that connection now, we cannot UNREGISTER since
            // we are not connected to snapcommunicator
            //
            f_communicator->remove_connection(f_messenger);
            f_messenger.reset();
        }
        else
        {
            f_messenger->mark_done();

            // unregister if we are still connected to the messenger
            // and Snap! Communicator is not already quitting
            //
            snap::snap_communicator_message cmd;
            cmd.set_command("UNREGISTER");
            cmd.add_parameter("service", "snaplog");
            f_messenger->send_message(cmd);
        }
    }

    if(f_communicator != nullptr)
    {
        f_communicator->remove_connection(f_interrupt);
        f_interrupt.reset();
        f_communicator->remove_connection(f_timer);
        f_timer.reset();
    }
}


// vim: ts=4 sw=4 et
