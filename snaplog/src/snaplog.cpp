/*
 * Text:
 *      snaplog/src/snaplog.cpp
 *
 * Description:
 *      Logger for the Snap! system. This service uses snapcommunicator to
 *      listen to all "SNAPLOG" messages. It records each message into a MySQL
 *      database for later retrieval, making reporting a lot easier for
 *      the admin.
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

const std::vector<std::string> g_configuration_files; // Empty

const advgetopt::getopt::option g_snaplog_options[] =
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
        'c',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "config",
        nullptr,
        "Configuration file to initialize snaplog.",
        advgetopt::getopt::argument_mode_t::optional_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
        "debug",
        nullptr,
        "Start the snaplog in debug mode.",
        advgetopt::getopt::argument_mode_t::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "help",
        nullptr,
        "show this help output",
        advgetopt::getopt::argument_mode_t::no_argument
    },
    {
        'l',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
        "logfile",
        nullptr,
        "Full path to the snaplog logfile.",
        advgetopt::getopt::argument_mode_t::optional_argument
    },
    {
        'n',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
        "nolog",
        nullptr,
        "Only output to the console, not a log file.",
        advgetopt::getopt::argument_mode_t::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "version",
        nullptr,
        "show the version of %p and exit.",
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


/** \brief The instance of the snaplog.
 *
 * This is the instance of the snaplog. The variable where the pointer
 * is kept.
 */
snaplog::pointer_t                    snaplog::g_instance;


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
    : f_opt( argc, argv, g_snaplog_options, g_configuration_files, nullptr )
    , f_config( "snaplog" )
{
    // --help
    if( f_opt.is_defined( "help" ) )
    {
        usage(advgetopt::getopt::status_t::no_error);
        snap::NOTREACHED();
    }

    // --version
    if(f_opt.is_defined("version"))
    {
        std::cerr << SNAPLOG_VERSION_STRING << std::endl;
        exit(0);
        snap::NOTREACHED();
    }

    // read the configuration file
    //
    if(f_opt.is_defined( "config"))
    {
        f_config.set_configuration_path( f_opt.get_string("config") );
    }

    // --debug
    f_debug = f_opt.is_defined("debug");

    // local_listen=... from snapcommunicator.conf
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
        usage(advgetopt::getopt::status_t::error);
        snap::NOTREACHED();
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


/** \brief Print out this server usage and exit.
 *
 * This function calls the advanced option library to have it print
 * out the list of acceptable command line options.
 */
void snaplog::usage(advgetopt::getopt::status_t status)
{
    f_opt.usage( status, "snaplog" );
    exit(1);
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
 * This specific daemon listens for two sets of events:
 *
 * \li Events sent via the snapcommunicator system; mainly used to
 *     REGISTER this as a server; tell the snapinit service that we
 *     are running; and accept a STOP to quit the application
 * \li New network connections to process Cassandra CQL commands.
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

    // create a messenger to communicate with the Snap Communicator process
    // and snapinit as required
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
    snap::NOTREACHED();
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

    // TODO: We need something to do. Set a flag? We don't need to send anything
    //       across snapcommunicator like snapdbproxy does.
}


void snaplog::no_mysql()
{
    SNAP_LOG_TRACE("no_mysql() called.");

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

    // TODO: add a record to the MySQL database
    //
    SNAP_LOG_TRACE("SNAPLOG command received: server=[")(message.get_server())("], service=[")(message.get_service())("]");
    auto const all_parms = message.get_all_parameters();
#if 0
    for( auto key : all_parms.keys() )
    {
        SNAP_LOG_TRACE("parm {")(key)("} = [")(all_parms[key])("]");
    }
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
        no_mysql();
    }
}


/** \brief Process a message received from Snap! Communicator.
 *
 * This function gets called whenever the Snap! Communicator sends
 * us a message. This includes the READY and HELP commands, although
 * the most important one is certainly the STOP command.
 *
 * \param[in] message  The message we just received.
 */
void snaplog::process_message(snap::snap_communicator_message const & message)
{
    SNAP_LOG_TRACE("received messenger message [")(message.to_message())("] for ")(f_server_name);

    QString const command(message.get_command());

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
        // Someone is asking us to leave (probably snapinit)
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
        f_ready = true;

        // Snap! Communicator received our REGISTER command
        //
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
        reply.add_parameter("list", "SNAPLOG,HELP,LOG,QUITTING,READY,RELOADCONFIG,STOP,UNKNOWN");

        f_messenger->send_message(reply);
        return;
    }

    if(command == "SNAPLOG")
    {
        add_message_to_db( message );
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
