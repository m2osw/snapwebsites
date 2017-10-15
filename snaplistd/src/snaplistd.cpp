/*
 * Text:
 *      snapwebsites/snaplistd/src/snaplistd.cpp
 *
 * Description:
 *      A daemon to collect all the changing website pages so the pagelist
 *      can manage them.
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
#include "snaplistd.h"

#include "version.h"

// our lib
//
#include <snapwebsites/log.h>
#include <snapwebsites/qstring_stream.h>
#include <snapwebsites/snap_string_list.h>

// advgetopt lib
//
#include <advgetopt/advgetopt.h>

// Qt lib
//
#include <QtCore>
#include <QtSql>

// C++ lib
//
#include <algorithm>
#include <iostream>
#include <sstream>

// included last
//
#include <snapwebsites/poison.h>

/** \file
 * \brief Implementation of the snaplistd daemon.
 *
 * This file implements the daemon responsible for saving the list data
 * that other computers generate.
 *
 * Whenever a page changes, the list plugin will save a reference to
 * that page in a journal.
 *
 * Whenever new data is detected in that journal, the `listjournal`
 * backend reads it and sends it to the `snaplistd` daemon which in
 * most cases is going to be on another computer. This is done through
 * snapcommunicator using the `LISTDATA` message.
 *
 * The `snaplistd` daemon is responsible for save the list data to
 * a MySQL database which will next be handled by the `pagelist`
 * backend. The MySQL database is used because we want to sort all
 * the entries in such a way that they can be processed in the
 * correct order (i.e. certain pages are given a much higher
 * priority than others.)
 *
 * Once the `snaplistd` daemon is done, it sends an acknowledgement
 * to the client using the `GOTLISTDATA` message. If somehow the
 * handling fails, the daemon sends `LISTDATAFAILED` instead. It is
 * very important for the client to not delete its data in case of
 * a failure since it means that the data was not saved in the
 * MySQL database.
 */

namespace
{
    const std::vector<std::string> g_configuration_files; // Empty

    advgetopt::getopt::option const g_snaplistd_options[] =
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
            "Configuration file to initialize snaplistd.",
            advgetopt::getopt::argument_mode_t::optional_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
            "debug",
            nullptr,
            "Start the snaplistd daemon in debug mode.",
            advgetopt::getopt::argument_mode_t::no_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
            "debug-listd-messages",
            nullptr,
            "Log all the listd messages received by snaplistd.",
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
            "Full path to the snaplistd logfile.",
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


/** \class snaplistd
 * \brief Class handling the transfer of list journal data to MySQL.
 *
 * This class initializes various message handlers and processes
 * LISTDATA messages to save them to a MySQL database called `journal`.
 */



/** \brief Initializes a snaplistd object.
 *
 * This function parses the command line arguments, reads configuration
 * files, setups the logger.
 *
 * It also immediately executes a --help or a --version command line
 * option and exits the process if these are present.
 *
 * \param[in] argc  The number of arguments in the argv array.
 * \param[in] argv  The array of argument strings.
 *
 */
snaplistd::snaplistd(int argc, char * argv[])
    : f_opt( argc, argv, g_snaplistd_options, g_configuration_files, nullptr )
    , f_config("snaplistd")
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
        std::cerr << SNAPLISTD_VERSION_STRING << std::endl;
        exit(1);
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

    // --debug-listd-messages
    f_debug_listd_messages = f_opt.is_defined("debug-listd-messages")     // command line
                          || f_config.has_parameter("debug_listd_messages");   // .conf file

    // get the server name using the library function
    f_server_name = QString::fromUtf8(snap::server::get_server_name().c_str());

    // local_listen=... -- from snapcommnicator.conf
    tcp_client_server::get_addr_port(QString::fromUtf8(f_config("snapcommunicator", "local_listen").c_str()), f_communicator_addr, f_communicator_port, "tcp");

    // setup the logger: --nolog, --logfile, or config file log_config
    //
    if(f_opt.is_defined("nolog"))
    {
        snap::logging::configure_console();
    }
    else if(f_opt.is_defined("logfile"))
    {
        snap::logging::configure_logfile(QString::fromUtf8(f_opt.get_string( "logfile" ).c_str()));
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

    if(f_debug)
    {
        // Force the logger level to DEBUG
        // (unless already lower)
        //
        snap::logging::reduce_log_output_level( snap::logging::log_level_t::LOG_LEVEL_DEBUG );
    }

    // make sure there are no standalone parameters
    if( f_opt.is_defined( "--" ) )
    {
        std::cerr << "error: unexpected parameter found on snaplistd daemon command line." << std::endl;
        usage(advgetopt::getopt::status_t::error);
    }
}


/** \brief Do some clean ups.
 *
 * At this point, the destructor is present mainly because we have
 * some virtual functions.
 */
snaplistd::~snaplistd()
{
}


/** \brief Print out usage and exit with 1.
 *
 * This function prints out the usage of the snaplistd daemon and
 * then it exits.
 *
 * \param[in] status  The reason why the usage is bring printed: error
 *                    and no_error are currently supported.
 */
void snaplistd::usage(advgetopt::getopt::status_t status)
{
    f_opt.usage( status, "snaplistd" );
    exit(1);
}


/** \brief Run the snaplistd daemon.
 *
 * This function is the core function of the daemon. It runs the loop
 * used to listd processes from any number of computers that have access
 * to the snaplistd daemon network.
 */
void snaplistd::run()
{
    // Stop on these signals, log them, then terminate.
    //
    // Note: the handler uses the logger which the create_instance()
    //       initializes
    //
    signal( SIGSEGV, snaplistd::sighandler );
    signal( SIGBUS,  snaplistd::sighandler );
    signal( SIGFPE,  snaplistd::sighandler );
    signal( SIGILL,  snaplistd::sighandler );
    signal( SIGTERM, snaplistd::sighandler );
    signal( SIGINT,  snaplistd::sighandler );
    signal( SIGQUIT, snaplistd::sighandler );

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
    f_interrupt = std::make_shared<snaplistd_interrupt>(this);
    f_communicator->add_connection(f_interrupt);

    // create a timer, it will immediately kick in and attempt a connection
    // to MySQL. If it fails, it will continue to tick until it works.
    //
    f_mysql_timer = std::make_shared<snaplistd_mysql_timer>(this);
    f_communicator->add_connection(f_mysql_timer);

    // create a messenger to communicate with the Snap Communicator process
    // and other services as required
    //
    SNAP_LOG_INFO("--------------------------------- snaplistd started.");

    f_messenger = std::make_shared<snaplistd_messenger>(this, f_communicator_addr.toUtf8().data(), f_communicator_port);
    f_communicator->add_connection(f_messenger);

    // now run our listening loop
    //
    f_communicator->run();
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
 * The signals are setup after the construction of the snaplistd
 * object because that is where we initialize the logger.
 *
 * \param[in] sig  The signal that was just emitted by the OS.
 */
void snaplistd::sighandler( int sig )
{
    QString signame;
    bool show_stack(true);
    switch(sig)
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
        show_stack = false;
        break;

    case SIGINT:
        signame = "SIGINT";
        show_stack = false;
        break;

    case SIGQUIT:
        signame = "SIGQUIT";
        show_stack = false;
        break;

    default:
        signame = "UNKNOWN";
        break;

    }

    if(show_stack)
    {
        snap::snap_exception_base::output_stack_trace();
    }

    SNAP_LOG_FATAL("Fatal signal caught: ")(signame);

    // Exit with error status
    //
    ::exit(1);
    snap::NOTREACHED();
}


/** \brief Get the name of the server we are running on.
 *
 * This function returns the name of the server this instance of
 * snaplistd is running. It is used by the ticket implementation
 * to know whether to send a reply to the snap_listd object (i.e.
 * at this time we can send messages to that object only from the
 * server it was sent from.)
 *
 * \return The name of the server snaplistd is running on.
 */
QString const & snaplistd::get_server_name() const
{
    return f_server_name;
}


/** \brief Process a message received from Snap! Communicator.
 *
 * This function gets called whenever the Snap! Communicator sends
 * us a message. This includes the READY and HELP commands, although
 * the most important one is certainly the STOP command.
 *
 * \param[in] message  The message we just received.
 */
void snaplistd::process_message(snap::snap_communicator_message const & message)
{
    // This adds way too many messages! By default we want these to be hidden
    // use the --debug-messages command line flag to see the message debug flags
    // (i.e. just the --debug flag is not enough)
    if(f_debug_listd_messages)
    {
        SNAP_LOG_TRACE("received messenger message [")(message.to_message())("] for ")(f_server_name);
    }

    QString const command(message.get_command());

    switch(command[0].unicode())
    {
    case 'H':
        if(command == "HELP")
        {
            // Snap! Communicator is asking us about the commands that we support
            //
            snap::snap_communicator_message commands;
            commands.set_command("COMMANDS");
            commands.add_parameter("list", "HELP,LISTDATA,LOG,QUITTING,READY,STOP,UNKNOWN");
            f_messenger->send_message(commands);

            // At this time a PING requires a URI which we do not have as
            // such (i.e. we want to wake up all the `listjournal`...
            //snap::snap_communicator_message ping;
            //ping.set_command("PING");
            //ping.set_service("listjournal");
            //ping.add_parameter("uri", ""); -- not valid...
            //f_messenger->send_message(ping);

            return;
        }
        break;

    case 'L':
        if(command == "LISTDATA")
        {
            // the message we are the most interested in
            //
            list_data(message);
            return;
        }
        break;

    case 'Q':
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
        break;

    case 'R':
        if(command == "READY")
        {
            return;
        }
        break;

    case 'S':
        if(command == "STOP")
        {
            // Someone is asking us to leave (probably snapinit)
            //
            stop(false);
            return;
        }
        break;

    case 'U':
        if(command == "UNKNOWN")
        {
            // we sent a command that Snap! Communicator did not understand
            //
            SNAP_LOG_ERROR("we sent unknown command \"")(message.get_parameter("command"))("\" and probably did not get the expected result.");
            return;
        }
        break;

    }

    // unknown commands get reported and process goes on
    //
    SNAP_LOG_ERROR("unsupported command \"")(command)("\" was received on the connection with Snap! Communicator.");
    {
        snap::snap_communicator_message reply;
        reply.set_command("UNKNOWN");
        reply.add_parameter("command", command);
        f_messenger->send_message(reply);
    }

    return;
}


/** \brief Called whenever we receive the STOP command or equivalent.
 *
 * This function makes sure the snaplistd exits as quickly as
 * possible.
 *
 * \li Marks the messenger as done.
 * \li UNREGISTER from snapcommunicator.
 *
 * \note
 * If the f_messenger is still in place, then just sending the
 * UNREGISTER is enough to quit normally. The socket of the
 * f_messenger will be closed by the snapcommunicator server
 * and we will get a HUP signal. However, we get the HUP only
 * because we first mark the messenger as done.
 *
 * \param[in] quitting  Set to true if we received a QUITTING message.
 */
void snaplistd::stop(bool quitting)
{
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
            cmd.add_parameter("service", "snaplistd");
            f_messenger->send_message(cmd);
        }
    }

    if(f_communicator != nullptr)
    {
        f_communicator->remove_connection(f_interrupt);
        f_interrupt.reset();
        f_communicator->remove_connection(f_mysql_timer);
        f_mysql_timer.reset();
    }
}


/** \brief Handle one LISTDATA message.
 *
 * This function handles one LISTDATA message the list daemon just
 * received.
 *
 * \param[in] message  The message with the list data parameters.
 */
void snaplistd::list_data(snap::snap_communicator_message const & message)
{
    //
    // Get the data we are interested in:
    //   . URI
    //   . Date and time when the item should be checked by the backend
    //   . Priority
    //

    if(QSqlDatabase::database().isOpen())
    {
        QString const uri           (message.get_parameter("uri"));
        QString const key_start_date(message.get_parameter("key_start_date"));
        QString const priority      (message.get_parameter("priority"));

        snap::snap_uri const uri_data(uri);
        QString const domain(uri_data.get_website_uri());

        // do that work in a transaction because we do not want another
        // INSERT or UDPATE or DELETE to happen while we do this work
        //
        bool success(false);
        QSqlDatabase::database().transaction();

        // We have a few cases here:
        //
        // 1. The journal has no references to URI, then just INSERT
        //
        // 2. The journal has a reference to URI, the existing one has a timestamp
        //    in the future, we just update priority if the new one is smaller
        //
        // 3. The journal has a reference to URI, the existing one has a timestamp
        //    in the past, we update priority if smaller and timestamp
        //
        // 4. The journal has a reference to URI, the existing one has a timestamp
        //    in the future and a lower priority, do nothing
        //
        QString const q_str(
                "SELECT id, priority, key_start_date"
                    " FROM snaplist.journal"
                    " WHERE uri = :uri AND status IS NULL;\n"
            );

        QSqlQuery q;
        q.setForwardOnly(true);
        q.prepare(q_str);
        q.bindValue(":uri", uri);
        if(q.exec())
        {
            // if there is any data that matched, then q.next() returns
            // true and we can gather said data, otherwise, just do an
            // INSERT INTO...
            //
            if(q.next())
            {
                int const id_field_no(q.record().indexOf("id"));
                QVariant const id(q.value(id_field_no));

                int const priority_field_no(q.record().indexOf("priority"));
                int64_t const db_priority(q.value(priority_field_no).toInt());

                int const key_start_date_field_no(q.record().indexOf("key_start_date"));
                int64_t const db_key_start_date(q.value(key_start_date_field_no).toLongLong());

                int const msg_priority(priority.toInt());
                int64_t const msg_key_start_date(key_start_date.toLongLong());

                if(msg_key_start_date > db_key_start_date
                || msg_priority < db_priority)
                {
                    QString const qupdate_str("UPDATE snaplist.journal"
                            " SET key_start_date = GREATEST(key_start_date, :key_start_date),"
                                " priority = LEAST(priority, :priority)"
                            " WHERE id = :id");

                    QSqlQuery qupdate;
                    qupdate.prepare(qupdate_str);
                    qupdate.bindValue(":priority",       priority);
                    qupdate.bindValue(":key_start_date", key_start_date);
                    qupdate.bindValue(":id",             id);
                    if(qupdate.exec())
                    {
                        success = true;
                    }
                    else
                    {
                        SNAP_LOG_ERROR("Query error! [")(qupdate.lastError().text())("], lastQuery=[")(qupdate.lastQuery())("]");
                    }
                }
                else
                {
                    // no UPDATE required so avoid sending the order
                    // (nothing would happen, but it is much faster
                    // if we do not do anything)
                    //
                    success = true;
                }
            }
            else
            {
                // the SELECT returned empty handed, INSERT the new data
                //
                QString const qinsert_str("INSERT INTO snaplist.journal"
                                 " ( domain,  priority,  key_start_date,  uri)"
                          " VALUES (:domain, :priority, :key_start_date, :uri)"
                      );

                QSqlQuery qinsert;
                qinsert.prepare(qinsert_str);
                qinsert.bindValue(":domain",         domain);
                qinsert.bindValue(":priority",       priority);
                qinsert.bindValue(":key_start_date", key_start_date);
                qinsert.bindValue(":uri",            uri);
                if(qinsert.exec())
                {
                    success = true;
                }
                else
                {
                    SNAP_LOG_ERROR("Query error! [")(qinsert.lastError().text())("], lastQuery=[")(qinsert.lastQuery())("]");
                }
            }

            // done with the SELECT
            // (this is important or the commit() below may fail)
            //
            q.finish();
        }
        else
        {
            SNAP_LOG_ERROR("Query error! [")(q.lastError().text())("], lastQuery=[")(q.lastQuery())("]");
        }

        if(success
        && QSqlDatabase::database().commit())
        {
            // it all worked, reply positively
            //
            snap::snap_communicator_message reply;
            reply.set_command("GOTLISTDATA");
            reply.reply_to(message);
            reply.add_parameter("listdata_id", message.get_parameter("listdata_id"));
            f_messenger->send_message(reply);

            return;
        }

        // if not commiting, then true a rollback() and ignore the
        // result since it's not useful
        //
        if(!QSqlDatabase::database().rollback())
        {
            SNAP_LOG_DEBUG("rollback() failed.");
        }
    }

    no_mysql();

    // something went wrong, reply negatively
    //
    snap::snap_communicator_message reply;
    reply.set_command("LISTDATAFAILED");
    reply.add_parameter("listdata_id", message.get_parameter("listdata_id"));
    f_messenger->send_message(reply);
}


/** \brief Setup a timer to retry connecting to MySQL.
 *
 * This function is used any time we have a problem connecting/using
 * the MySQL connection. It sets up a timer that will be used to
 * attempt a reconnect to the MySQL server.
 */
void snaplistd::no_mysql()
{
    SNAP_LOG_TRACE("no_mysql() called.");

    if(f_mysql_timer != nullptr)
    {
        f_mysql_timer->set_enable(true);
        f_mysql_timer->set_timeout_delay(static_cast<int64_t>(f_mysql_connect_timer_index) * 1000000LL);
    }
}


/** \brief Process timer tick.
 *
 * This function processes a timer tick. In most cases, it runs once at
 * the start and then the connection remains up and running forever.
 */
void snaplistd::process_timeout()
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
        db.setUserName     ( "snaplist" );
        db.setPassword     ( "snaplist" );
        db.setDatabaseName ( "snaplist" );
        if( !db.open() )
        {
            std::string const error( "Cannot open MySQL database snaplist!" );
            SNAP_LOG_ERROR( error );
            throw std::runtime_error(error.c_str());
        }

        // the connection succeeded, turn off the timer we do not need
        // it for now...
        //
        f_mysql_timer->set_enable(false);

        // reset the delay to about 1 second
        // (we use 1.625 so that way we will have 1s, 3s, 7s, 15s, 30s, 60s
        // and thus 1 minute.)
        //
        f_mysql_connect_timer_index = 1.625f;
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


// vim: ts=4 sw=4 et
