/*
 * Text:
 *      snapwebsites/snapdbproxy/snapdbproxy.cpp
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
#include "snapdbproxy.h"
#include "version.h"

// snapwebsites lib
//
#include <snapwebsites/log.h>
#include <snapwebsites/qstring_stream.h>
#include <snapwebsites/dbutils.h>

// 3rd party libs
//
#include <QtCore>
#include <libdbproxy/libdbproxy.h>
#include <advgetopt/advgetopt.h>

// system (C++)
//
#include <algorithm>
#include <iostream>
#include <sstream>

namespace
{

    const std::vector<std::string> g_configuration_files; // Empty

    const advgetopt::getopt::option g_snapdbproxy_options[] =
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
            "Configuration file to initialize snapdbproxy.",
            advgetopt::getopt::argument_mode_t::optional_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
            "debug",
            nullptr,
            "Start the snapdbproxy in debug mode.",
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
            "Full path to the snapdbproxy logfile.",
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


}
//namespace


void snapdbproxy_timer::process_timeout()
{
    f_snapdbproxy->process_timeout();
}




/** \class snapdbproxy
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


/** \brief The instance of the snapdbproxy.
 *
 * This is the instance of the snapdbproxy. The variable where the pointer
 * is kept.
 */
snapdbproxy::pointer_t                    snapdbproxy::g_instance;


/** \brief Initializes a snapdbproxy object.
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
snapdbproxy::snapdbproxy(int argc, char * argv[])
    : f_opt( argc, argv, g_snapdbproxy_options, g_configuration_files, nullptr )
    , f_config( "snapdbproxy" )
    , f_session( casswrapper::Session::create() )
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
        std::cerr << SNAPDBPROXY_VERSION_STRING << std::endl;
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

    // listen=... from snapdbproxy.conf
    tcp_client_server::get_addr_port(QString::fromUtf8(f_config("listen").c_str()), f_snapdbproxy_addr, f_snapdbproxy_port, "tcp");

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

    // from config file only
    if(f_config.has_parameter("cassandra_host_list"))
    {
        f_cassandra_host_list = f_config[ "cassandra_host_list" ];
        if(f_cassandra_host_list.isEmpty())
        {
            throw snap::snapwebsites_exception_invalid_parameters("cassandra_host_list cannot be empty.");
        }
    }
    if(f_config.has_parameter("cassandra_port"))
    {
        std::size_t pos(0);
        std::string const port(f_config["cassandra_port"]);
        f_cassandra_port = std::stoi(port, &pos, 10);
        if(pos != port.length()
        || f_cassandra_port < 0
        || f_cassandra_port > 65535)
        {
            throw snap::snapwebsites_exception_invalid_parameters("cassandra_port to connect to Cassandra must be defined between 0 and 65535.");
        }
    }

    // offer the user to setup the maximum number of pending connections
    // from services that want to connect to Cassandra (this is only
    // the maximum number of "pending" connections and not the total
    // number of acceptable connections)
    //
    if(f_config.has_parameter("max_pending_connections"))
    {
        QString const max_connections(f_config["max_pending_connections"]);
        if(!max_connections.isEmpty())
        {
            bool ok;
            f_max_pending_connections = max_connections.toInt(&ok);
            if(!ok)
            {
                SNAP_LOG_FATAL("invalid max_pending_connections, a valid number was expected instead of \"")(max_connections)("\".");
                exit(1);
            }
            if(f_max_pending_connections < 1)
            {
                SNAP_LOG_FATAL("max_pending_connections must be positive, \"")(max_connections)("\" is not valid.");
                exit(1);
            }
        }
    }

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
 * This function is used to do some clean up of the snapdbproxy
 * environment.
 */
snapdbproxy::~snapdbproxy()
{
}


/** \brief Print out this server usage and exit.
 *
 * This function calls the advanced option library to have it print
 * out the list of acceptable command line options.
 */
void snapdbproxy::usage(advgetopt::getopt::status_t status)
{
    f_opt.usage( status, "snapdbproxy" );
    exit(1);
}


/** \brief Retrieve the server name.
 *
 * This function returns a copy of the server name. Since the constructor
 * defines the server name, it is available at all time after that.
 *
 * \return The server name.
 */
std::string snapdbproxy::server_name() const
{
    return f_server_name;
}


/** \brief Use SSL for Cassandra connections
 *
 * This checks the configuration settings for "cassandra_use_ssl".
 * If present and set to "true", this method returns true, false
 * otherwise.
 */
bool snapdbproxy::use_ssl() const
{
    if( f_config.has_parameter("cassandra_use_ssl") )
    {
        return f_config["cassandra_use_ssl"] == "true";
    }

    return false;
}


/** \brief Start the Snap! Communicator and wait for events.
 *
 * This function initializes the snapdbproxy object further and then
 * listens for events.
 *
 * This specific daemon listens for two sets of events:
 *
 * \li Events sent via the snapcommunicator system; mainly used to
 *     REGISTER this as a server; tell the snapinit service that we
 *     are running; and accept a STOP to quit the application
 * \li New network connections to process Cassandra CQL commands.
 */
void snapdbproxy::run()
{
    // Stop on these signals, log them, then terminate.
    //
    signal( SIGCHLD, snapdbproxy::sighandler );
    signal( SIGSEGV, snapdbproxy::sighandler );
    signal( SIGBUS,  snapdbproxy::sighandler );
    signal( SIGFPE,  snapdbproxy::sighandler );
    signal( SIGILL,  snapdbproxy::sighandler );
    signal( SIGTERM, snapdbproxy::sighandler );
    signal( SIGINT,  snapdbproxy::sighandler );
    signal( SIGQUIT, snapdbproxy::sighandler );

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
    f_interrupt.reset(new snapdbproxy_interrupt(this));
    f_communicator->add_connection(f_interrupt);

    // capture "nocassandra" signal (SIGUSR1)
    //
    f_nocassandra.reset(new snapdbproxy_nocassandra(this));
    f_communicator->add_connection(f_nocassandra);

    // create a listener
    //
    // Note that the listener changes its priority to 30 in order to
    // make sure that it gets called first in case multiple events
    // arrive simultaneously.
    //
    f_listener = std::make_shared<snapdbproxy_listener>(this, f_snapdbproxy_addr.toUtf8().data(), f_snapdbproxy_port, f_max_pending_connections, true);
    f_communicator->add_connection(f_listener);

    // create a messenger to communicate with the Snap Communicator process
    // and snapinit as required
    //
    f_messenger = std::make_shared<snapdbproxy_messenger>(this, f_communicator_addr.toUtf8().data(), f_communicator_port);
    f_communicator->add_connection(f_messenger);

    // Add the logging server through snapcommunicator:
    //
    snap::logging::set_log_messenger( f_messenger );

    // create a timer, it will immediately kick in and attempt a connection
    // to Cassandra; if it fails, it will continue to tick until it works.
    //
    f_timer = std::make_shared<snapdbproxy_timer>(this);
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
 * The signals are setup after the construction of the snapdbproxy
 * object because that is where we initialize the logger.
 *
 * \param[in] sig  The signal that was just emitted by the OS.
 */
void snapdbproxy::sighandler( int sig )
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


/** \brief Process a message received from Snap! Communicator.
 *
 * This function gets called whenever the Snap! Communicator sends
 * us a message. This includes the READY and HELP commands, although
 * the most important one is certainly the STOP command.
 *
 * \param[in] message  The message we just received.
 */
void snapdbproxy::process_message(snap::snap_communicator_message const & message)
{
    SNAP_LOG_TRACE("received messenger message [")(message.to_message())("] for ")(f_server_name);

    QString const command(message.get_command());

// TODO: use a map statement (see poor old snapinit...)

    if(command == "CASSANDRASTATUS")
    {
        // immediately reply with the current status
        //
        snap::snap_communicator_message reply;
        reply.reply_to(message);
        reply.set_command(f_session->isConnected() ? "CASSANDRAREADY" : "NOCASSANDRA");
        reply.add_parameter("cache", "no");
        f_messenger->send_message(reply);
        return;
    }

    if( command == "CASSANDRAKEY" )
    {
        QDir key_path(f_session->get_keys_path());
        if( !key_path.exists() )
        {
            SNAP_LOG_TRACE("First time receiving any cert keys, so creating path.");
            // Make sure the key path exists...if not,
            // then create it.
            //
            key_path.mkdir(f_session->get_keys_path());
        }

        // Open the file...
        //
        QString listen_address_us( message.get_parameter("listen_address") );
        listen_address_us.replace( '.', '_' );
        QString const full_path( QString("%1client_%2.pem")
                                 .arg(f_session->get_keys_path())
                                 .arg(listen_address_us)
                                 );
        try
        {
            QFile file( full_path );
            if( file.exists() )
            {
                if( message.has_parameter("force") )
                {
                    SNAP_LOG_INFO("User has requested that the key for [")
                            (message.get_parameter("listen_address"))
                            ("] be overridden, even though we have it already.");
                }
                else
                {
                    // We already have the file, so ignore this.
                    //
                    SNAP_LOG_TRACE("We already have cert file [")(full_path)("], so ignoring.");
                    return;
                }
            }
            //
            if( !file.open( QIODevice::WriteOnly | QIODevice::Truncate ) )
            {
                QString const errmsg = QString("Cannot open '%1' for writing!").arg(file.fileName());
                SNAP_LOG_ERROR(errmsg);
                return;
            }
            //
            // ...and stream the file out to disk
            //
            QString const key( message.get_parameter("key") );
            QTextStream out( &file );
            out << key;

            // Make sure it's imported into the session...
            //
            SNAP_LOG_TRACE("Received cert file [")(full_path)("], adding into current session.");
            f_session->add_ssl_trusted_cert( key );
        }
        catch( std::exception const& ex )
        {
            SNAP_LOG_ERROR("Cannot add SSL CERT file! what=[")(ex.what())("]");
        }
        catch( ... )
        {
            SNAP_LOG_ERROR("Cannot write SSL CERT file! Unknown error!");
        }

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

        if( use_ssl() )
        {
            // Ask for server certs first from each snapmanager cassandra
            // throughout the entire cluster.
            //
            snap::snap_communicator_message request;
            request.set_command("CASSANDRAKEYS");
            request.set_service("*");
            request.add_parameter("cache", "ttl=60");
            f_messenger->send_message(request);
        }

        // Snap! Communicator received our REGISTER command
        //
        if(f_session->isConnected())
        {
            cassandra_ready();
        }

        return;
    }

    if(command == "RELOADCONFIG")
    {
        f_force_restart = true;
        stop(false);
        return;
    }

    if(command == "NEWTABLE")
    {
        // a package just got installed and that package included a
        // table definition
        //

SNAP_LOG_WARNING("NEWTABLE not yet implemented...");

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
        reply.add_parameter("list", "CASSANDRAKEY,CASSANDRASTATUS,HELP,LOG,NEWTABLE,QUITTING,READY,RELOADCONFIG,STOP,UNKNOWN");

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


/** \brief Call whenever a new connection was received.
 *
 * This function adds a new connection to the snapdbproxy daemon. A
 * connection is a blocking socket handled by a thread.
 *
 * The snapdbproxy_listener does the listen() and accept() calls.
 * Here we dispatch the call to a thread using a snapdbproxy_thread.
 * The connection is then handled by the runner which is the
 * snapdbproxy_connection.
 *
 * \param[in] client  The client the connection threads becomes the owner of.
 */
void snapdbproxy::process_connection(tcp_client_server::bio_client::pointer_t & client)
{
    // only the main process calls this function so we can take the time
    // to check the f_connections vector and remove dead threads
    //
    f_connections.erase(
            std::remove_if(
                f_connections.begin(),
                f_connections.end(),
                [](auto const & c)
                {
                    return !c->is_running();
                }),
            f_connections.end());

    if(!f_session->isConnected())
    {
        no_cassandra();
    }

    // create one thread per connection
    //
    // TODO: look into having either worker threads, or at least a pool
    //       that we keep around
    //
    // The snapdbproxy_thread constructor is expected to start the thread
    // although it may fail; if it does fail, we avoid adding the thread
    // to the f_connections vector; that way the socket gets closed
    // (the only case where the socket does not get closed is and
    // std::bad_alloc exception which we do not capture here.)
    //
    // WARNING: the client is passed as a reference so we can cleanly take
    //          ownership in the snapdbproxy_connection object
    //
    snapdbproxy_thread::pointer_t thread(std::make_shared<snapdbproxy_thread>
                                         ( this
                                          , f_session
                                          , client
                                          , f_cassandra_host_list
                                          , f_cassandra_port
                                          , use_ssl()
                                          ));
    if(thread && thread->is_running())
    {
        f_connections.push_back(thread);
    }
}


/** \brief Attempt to connect to the Cassandra cluster.
 *
 * This function calls connect() in order to create a network connection
 * between this computer and a Cassandra node. Later the driver may
 * connect to additional nodes to better balance work load.
 *
 * \note
 * Since attempts to connect to Cassandra are blocking, we probably want
 * to move this timer processing to a thread instead.
 */
void snapdbproxy::process_timeout()
{
    try
    {
        // connect to Cassandra
        //
        // The Cassandra C/C++ driver is responsible to actually create
        // "physical" connections to any number of nodes so we do not
        // need to monitor those connections.
        //
        f_session->connect( f_cassandra_host_list, f_cassandra_port, use_ssl() ); // throws on failure!

        // the connection succeeded, turn off the timer we do not need
        // it for now...
        //
        f_timer->set_enable(false);

        // reset that flag!
        //
        f_no_cassandra_sent = false;

        // reset the delay to about 1 second
        // (we use 1.625 so that way we will have 1s, 3s, 7s, 15s, 30s, 60s
        // and thus 1 minute.)
        //
        f_cassandra_connect_timer_index = 1.625f;

        cassandra_ready();
    }
    catch(std::runtime_error const &)
    {
        // the connection failed, keep the timeout enabled and try again
        // on the next tick
        //
        no_cassandra();
    }
}


/** \brief Send a NOCASSANDRA message.
 *
 * Let snapcommunicator and other services know that we do not
 * have a connection to Cassandra. Computers running snap.cgi should
 * react by not connecting to this computer since snapserver will not
 * work in that case.
 *
 * Obviously, if we cannot find a Cassandra node, we probably
 * have another bigger problem and snapcommunicator is probably
 * not connected to anyone else either...
 */
void snapdbproxy::no_cassandra()
{
    if(!f_no_cassandra_sent)
    {
        f_no_cassandra_sent = true;
        snap::snap_communicator_message cmd;
        cmd.set_command("NOCASSANDRA");
        cmd.set_service(".");
        cmd.add_parameter("cache", "no");
        f_messenger->send_message(cmd);
    }

    // make sure the timer is on when we do not have a Cassandra connection
    //
    f_timer->set_enable(true);

    // try again soon
    //
    f_timer->set_timeout_delay(static_cast<int64_t>(f_cassandra_connect_timer_index) * 1000000LL);
    if(f_cassandra_connect_timer_index < 60.0f)
    {
        // increase the delay between attempts up to 1 min.
        //
        f_cassandra_connect_timer_index *= 2.0f;
    }
}


void snapdbproxy::cassandra_ready()
{
    if(f_ready)
    {
        // let other services know when cassandra is (finally) ready
        //
        snap::snap_communicator_message cmd;
        cmd.set_command("CASSANDRAREADY");
        cmd.set_service(".");
        cmd.add_parameter("cache", "no");
        f_messenger->send_message(cmd);
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
void snapdbproxy::stop(bool quitting)
{
    SNAP_LOG_INFO("Stopping snapdbproxy server.");

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
            f_messenger->mark_done(true);

            // unregister if we are still connected to the messenger
            // and Snap! Communicator is not already quitting
            //
            snap::snap_communicator_message cmd;
            cmd.set_command("UNREGISTER");
            cmd.add_parameter("service", "snapdbproxy");
            f_messenger->send_message(cmd);
        }
    }

    // also remove the listener, we will not accept any more
    // database commands...
    //
    if(f_communicator != nullptr)
    {
        f_communicator->remove_connection(f_timer);
        f_timer.reset();

        f_communicator->remove_connection(f_listener);
        f_listener.reset(); // TBD: in snapserver I do not reset these pointers...

        f_communicator->remove_connection(f_interrupt);
        f_interrupt.reset();

        f_communicator->remove_connection(f_nocassandra);
        f_nocassandra.reset();
    }
}


// vim: ts=4 sw=4 et
