/*
 * Text:
 *      snaplock/src/snaplock.cpp
 *
 * Description:
 *      A daemon to synchronize processes between any number of computers
 *      by blocking all of them but one.
 *
 * License:
 *      Copyright (c) 2016-2019  Made to Order Software Corp.  All Rights Reserved
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
#include "snaplock.h"


// snapmanager lib
//
#include "version.h"


// snapwebsites lib
//
#include <snapwebsites/log.h>
#include <snapwebsites/qstring_stream.h>
#include <snapwebsites/dbutils.h>
#include <snapwebsites/process.h>
#include <snapwebsites/snap_string_list.h>


// snapdev lib
//
#include <snapdev/tokenize_string.h>


// advgetopt lib
//
#include <advgetopt/advgetopt.h>


// C++ lib
//
#include <algorithm>
#include <iostream>
#include <sstream>


// openssl lib
//
#include <openssl/rand.h>


// last include
//
#include <snapdev/poison.h>



/** \file
 * \brief Implementation of the snap inter-process lock mechanism.
 *
 * This file implements an inter-process lock that functions between
 * any number of machines. The basic algorithm used is the Bakery
 * Algorithm by Lamport. The concept is simple: you get a waiting
 * ticket and loop into it is your turn.
 *
 * Contrary to a multi-processor environment thread synchronization,
 * this lock system uses messages and arrays to know its current
 * status. A user interested in obtaining a lock sends a LOCK
 * message. The snaplock daemon then waits until the lock is
 * obtained and sends a LOCKED as a reply. Once done with the lock,
 * the user sends UNLOCK.
 *
 * The implementation makes use of any number of snaplock instances.
 * The locking mechanism makes use of the QUORUM voting system to
 * know that enough of the other snaplock agree on a statement.
 * This allows the snaplock daemon to obtain/release locks in an
 * unknown network environment (i.e. any one of the machines may
 * be up or down and the locking mechanism still functions as
 * expected.)
 *
 * \note
 * The snaplock implementation checks parameters and throws away
 * messages that are definitely not going to be understood. However,
 * it is, like most Snap! daemons, very trustworthy of other snaplock
 * daemons and does not expect other daemons to mess around with its
 * sequence of lock message used to ensure that everything worked as
 * expected.
 */



namespace snaplock
{


namespace
{



advgetopt::option const g_options[] =
{
    {
        'c',
        advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::GETOPT_FLAG_REQUIRED | advgetopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "config",
        nullptr,
        "Path to snaplock and other configuration files.",
        nullptr
    },
    {
        '\0',
        advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::GETOPT_FLAG_FLAG,
        "debug",
        nullptr,
        "Start the snaplock daemon in debug mode.",
        nullptr
    },
    {
        '\0',
        advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::GETOPT_FLAG_FLAG,
        "debug-lock-messages",
        nullptr,
        "Log all the lock messages received by snaplock.",
        nullptr
    },
    {
        '\0',
        advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::GETOPT_FLAG_FLAG,
        "list",
        nullptr,
        "List existing tickets and exits.",
        nullptr
    },
    {
        'l',
        advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::GETOPT_FLAG_REQUIRED,
        "logfile",
        nullptr,
        "Full path to the snaplock logfile.",
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
    .f_options = g_options,
    .f_options_files_directory = nullptr,
    .f_environment_variable_name = "SNAPLOCK_OPTIONS",
    .f_configuration_files = nullptr,
    .f_configuration_filename = nullptr,
    .f_configuration_directories = nullptr,
    .f_environment_flags = advgetopt::GETOPT_ENVIRONMENT_FLAG_PROCESS_SYSTEM_PARAMETERS,
    .f_help_header = "Usage: %p [-<opt>]\n"
                     "where -<opt> is one or more of:",
    .f_help_footer = "%c",
    .f_version = SNAPLOCK_VERSION_STRING,
    .f_license = "GNU GPL v2",
    .f_copyright = "Copyright (c) 2013-"
                   BOOST_PP_STRINGIZE(UTC_BUILD_YEAR)
                   " by Made to Order Software Corporation -- All Rights Reserved",
    //.f_build_date = UTC_BUILD_DATE,
    //.f_build_time = UTC_BUILD_TIME
};
#pragma GCC diagnostic pop


}
// no name namespace




/** \brief List of snaplock commands
 *
 * The following table defines the commands understood by snaplock
 * that are not defined as a default by add_snap_communicator_commands().
 */
snap::dispatcher<snaplock>::dispatcher_match::vector_t const snaplock::g_snaplock_service_messages =
{
    {
        "ABSOLUTELY"
      , &snaplock::msg_absolutely
    },
    {
        "ACTIVATELOCK"
      , &snaplock::msg_activate_lock
    },
    {
        "ADDTICKET"
      , &snaplock::msg_add_ticket
    },
    {
        "CLUSTERUP"
      , &snaplock::msg_cluster_up
    },
    {
        "CLUSTERDOWN"
      , &snaplock::msg_cluster_down
    },
    {
        "DISCONNECTED"
      , &snaplock::msg_server_gone
    },
    {
        "DROPTICKET"
      , &snaplock::msg_drop_ticket
    },
    {
        "GETMAXTICKET"
      , &snaplock::msg_get_max_ticket
    },
    {
        "HANGUP"
      , &snaplock::msg_server_gone
    },
    {
        "LOCK"
      , &snaplock::msg_lock
    },
    {
        "LOCKACTIVATED"
      , &snaplock::msg_lock_activated
    },
    {
        "LOCKENTERED"
      , &snaplock::msg_lock_entered
    },
    {
        "LOCKENTERING"
      , &snaplock::msg_lock_entering
    },
    {
        "LOCKEXITING"
      , &snaplock::msg_lock_exiting
    },
    {
        "LOCKFAILED"
      , &snaplock::msg_lock_failed
    },
    {
        "LOCKLEADERS"
      , &snaplock::msg_lock_leaders
    },
    {
        "LOCKSTARTED"
      , &snaplock::msg_lock_started
    },
    {
        "LOCKSTATUS"
      , &snaplock::msg_lock_status
    },
    {
        "LOCKTICKETS"
      , &snaplock::msg_lock_tickets
    },
    {
        "LISTTICKETS"
      , &snaplock::msg_list_tickets
    },
    {
        "MAXTICKET"
      , &snaplock::msg_max_ticket
    },
    {
        "STATUS"
      , &snaplock::msg_status
    },
    {
        "TICKETADDED"
      , &snaplock::msg_ticket_added
    },
    {
        "TICKETREADY"
      , &snaplock::msg_ticket_ready
    },
    {
        "UNLOCK"
      , &snaplock::msg_unlock
    }
};







snaplock::computer_t::computer_t()
{
    // used for a remote computer, we'll eventually get a set_id() which
    // defines the necessary computer parameters
}


snaplock::computer_t::computer_t(QString const & name, uint8_t priority)
    : f_self(true)
    , f_priority(priority)
    , f_pid(getpid())
    , f_name(name)
{
    RAND_bytes(reinterpret_cast<unsigned char *>(&f_random_id), sizeof(f_random_id));

    snap::snap_config config("snapcommunicator");
    f_ip_address = config["listen"];
}


bool snaplock::computer_t::is_self() const
{
    return f_self;
}


void snaplock::computer_t::set_connected(bool connected)
{
    f_connected = connected;
}


bool snaplock::computer_t::get_connected() const
{
    return f_connected;
}


bool snaplock::computer_t::set_id(QString const & id)
{
    if(f_priority != PRIORITY_UNDEFINED)
    {
        throw snaplock_exception_content_invalid_usage("computer_t::set_id() can't be called more than once or on this snaplock computer");
    }

    snap::snap_string_list parts(id.split('|'));
    if(parts.size() != 5)
    {
        // do not throw in case something changes we do not want snaplock to
        // "crash" over and over again
        //
        SNAP_LOG_ERROR("received a computer id which does not have exactly 5 parts.");
        return false;
    }

    // base is VERY IMPORTANT for this one as we save priorities below ten
    // as 0n (01 to 09) so the sort works as expected
    //
    bool ok(false);
    f_priority = parts[0].toLong(&ok, 10); 
    if(!ok
    || f_priority < PRIORITY_USER_MIN
    || f_priority > PRIORITY_MAX)
    {
        SNAP_LOG_ERROR("priority is limited to a number between 0 and 15 inclusive.");
        return false;
    }

    f_random_id = parts[1].toULong(&ok, 10);

    f_ip_address = parts[2];
    if(f_ip_address.isEmpty())
    {
        SNAP_LOG_ERROR("the process IP cannot be an empty string.");
        return false;
    }

    f_pid = parts[3].toLong(&ok, 10);
    if(!ok || f_pid < 1 || f_pid > snap::process::get_pid_max())
    {
        SNAP_LOG_ERROR("a process identifier is 15 bits so ")(f_pid)(" does not look valid (0 is also not accepted).");
        return false;
    }

    f_name = parts[4];
    if(f_name.isEmpty())
    {
        SNAP_LOG_ERROR("the server name in the lockid cannot be empty.");
        return false;
    }

    f_id = id;

    return true;
}


snaplock::computer_t::priority_t snaplock::computer_t::get_priority() const
{
    return f_priority;
}


void snaplock::computer_t::set_start_time(time_t start_time)
{
    f_start_time = start_time;
}


time_t snaplock::computer_t::get_start_time() const
{
    return f_start_time;
}


QString const & snaplock::computer_t::get_name() const
{
    return f_name;
}


QString const & snaplock::computer_t::get_id() const
{
    if(f_id.isEmpty())
    {
        if(f_priority == PRIORITY_UNDEFINED)
        {
            throw snaplock_exception_content_invalid_usage("computer_t::get_id() can't be called when the priority is not defined");
        }
        if(f_ip_address.isEmpty())
        {
            throw snaplock_exception_content_invalid_usage("computer_t::get_id() can't be called when the address is empty");
        }
        if(f_pid == 0)
        {
            throw snaplock_exception_content_invalid_usage("computer_t::get_id() can't be called when the pid is not defined");
        }

        f_id = QString("%1|%2|%3|%4|%5")
                        .arg(f_priority, 2, 10, QChar('0'))
                        .arg(f_random_id)
                        .arg(f_ip_address)
                        .arg(f_pid)
                        .arg(f_name);
    }

    return f_id;
}


QString const & snaplock::computer_t::get_ip_address() const
{
    return f_ip_address;
}














/** \class snaplock
 * \brief Class handling intercomputer locking.
 *
 * This class is used in order to create an intercomputer lock on request.
 *
 * The class implements the Snap! Communicator messages and implements
 * the LOCK and UNLOCK commands and sends the LOCKED command to its
 * sender.
 *
 * The system makes use of the Lamport's Bakery Algorithm. This is
 * explained in the snaplock_ticket class.
 *
 * \note
 * At this time there is one potential problem that can arise: the
 * lock may fail to concretize because the computer to which you
 * first sent the LOCK message goes down in some way. The other
 * snaplock computers will have no clue by which computer the lock
 * was being worked on and whether one of them should take over.
 * One way to remediate is to run one instance of snaplock on each
 * computer on which a lock is likely to happen.
 *
 * \warning
 * The LOCK mechanism uses the system clock of each computer to know when
 * a lock times out. You are responsible for making sure that all those
 * computers have a synchronized clocked (i.e. run a timed daemon.)
 * The difference in time should be as small as possible. The precision
 * required by snaplock is around 1 second.
 *
 * The following shows the messages used to promote 3 leaders, in other
 * words it shows how the election process happens. The election itself
 * is done on the computer that is part of the cluster being up and which
 * has the smallest IP address. That's the one computer that will send the
 * LOCKLEADERS. As soon as that happens all the other nodes on the cluster
 * will know the leaders and inform new nodes through the LOCKSTARTED
 * message.
 *
 * \msc
 *  Communicator,A,B,C,D,E,F;
 *
 *  A->Communicator [label="REGISTER"];
 *  Communicator->A [label="HELP"];
 *  Communicator->A [label="READY"];
 *
 *  A->Communicator [label="CLUSTERSTATUS"];
 *  Communicator->A [label="CLUSTERUP"];
 *
 *  # Broadcast to B to F, but we do not know who's up at this point
 *  A->* [label="LOCKSTARTED"];
 *
 *  # A answers each one of those because for it B, C, D, ... are new
 *  B->A [label="LOCKSTARTED"];
 *  A->B [label="LOCKSTARTED"];
 *
 *  C->A [label="LOCKSTARTED"];
 *  A->C [label="LOCKSTARTED"];
 *
 *  D->A [label="LOCKSTARTED"];
 *  A->D [label="LOCKSTARTED"];
 *
 *  # When we reach here we have a CLUSTERUP in terms of snaplock daemons
 *  # Again here we broadcast, maybe we should send to known computers instead?
 *  # IMPORTANT: A determines the leaders only if its IP is the smallest
 *  A->* [label="LOCKLEADERS"];
 *
 *  # Here the replies from A will include the leaders
 *  # Of course, as shown above, E will have sent the message to all and
 *  # so it will receive the leaders from multiple sources
 *  E->A [label="LOCKSTARTED"];
 *  A->E [label="LOCKSTARTED"];
 *
 *  F->A [label="LOCKSTARTED"];
 *  A->F [label="LOCKSTARTED"];
 * \endmsc
 *
 * \sa snaplock_ticket
 */



/** \brief Initializes a snaplock object.
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
snaplock::snaplock(int argc, char * argv[])
    : dispatcher(this, g_snaplock_service_messages)
    , f_opt(g_options_environment, argc, argv)
    , f_config("snaplock")
{
    add_snap_communicator_commands();

    // read the configuration file
    //
    if(f_opt.is_defined("config"))
    {
        f_config.set_configuration_path(f_opt.get_string("config"));
    }

    // --debug
    f_debug = f_opt.is_defined("debug");

    // --debug-lock-messages
    f_debug_lock_messages = f_opt.is_defined("debug-lock-messages")     // command line
                         || f_config.has_parameter("debug_lock_messages");   // .conf file

    // set message trace mode if debug-lock-messages is defined
    //
    if(f_debug_lock_messages)
    {
        set_trace();
    }

    // get the server name using the library function
    //
    // TODO: if the name of the server is changed, we should reboot, but
    //       to the minimum we need to restart snaplock (among other daemons)
    //       remember that snapmanager.cgi gives you that option
    //
    f_server_name = QString::fromUtf8(snap::server::get_server_name().c_str());
#ifdef _DEBUG
    // to debug multiple snaplock on the same server each instance needs to
    // have a different server name
    //
    if(f_config.has_parameter("server_name"))
    {
        f_server_name = f_config["server_name"];
    }
#endif

    // local_listen=... -- from snapcommnicator.conf
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
        snap::logging::configure_logfile(QString::fromUtf8(f_opt.get_string("logfile").c_str()));
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

    if(f_debug)
    {
        // Force the logger level to DEBUG
        // (unless already lower)
        //
        snap::logging::reduce_log_output_level( snap::logging::log_level_t::LOG_LEVEL_DEBUG );
    }

#ifdef _DEBUG
    // for test purposes (i.e. to run any number of snaplock on a single
    // computer) we allow the administrator to change the name of the
    // server, but only in a debug version
    //
    if(f_config.has_parameter("service_name"))
    {
        f_service_name = f_config["service_name"];
    }
#endif

    int64_t priority = computer_t::PRIORITY_DEFAULT;
    if(f_opt.is_defined("candidate-priority"))
    {
        std::string const candidate_priority(f_opt.get_string("candidate-priority"));
        if(candidate_priority == "off")
        {
            priority = computer_t::PRIORITY_OFF;
        }
        else
        {
            priority = f_opt.get_long("candidate-priority"
                                    , 0
                                    , computer_t::PRIORITY_USER_MIN
                                    , computer_t::PRIORITY_MAX);
        }
    }
    else if(f_config.has_parameter("candidate_priority"))
    {
        QString const candidate_priority(f_config["candidate_priority"]);
        if(candidate_priority == "off")
        {
            // a priority 15 means that this computer is not a candidate
            // at all (useful for nodes that get dynamically added
            // and removed--i.e. avoid re-election each time that happens.)
            //
            priority = computer_t::PRIORITY_OFF;
        }
        else
        {
            bool ok(false);
            priority = candidate_priority.toLong(&ok, 10);
            if(!ok)
            {
                SNAP_LOG_FATAL("invalid candidate_priority, a valid decimal number was expected instead of \"")(candidate_priority)("\".");
                exit(1);
            }
            if(priority < computer_t::PRIORITY_USER_MIN
            || priority > computer_t::PRIORITY_MAX)
            {
                SNAP_LOG_FATAL("candidate_priority must be between 1 and 15, \"")(candidate_priority)("\" is not valid.");
                exit(1);
            }
        }
    }

    // make sure there are no standalone parameters
    //
    if(f_opt.is_defined("--"))
    {
        SNAP_LOG_FATAL("unexpected parameters found on snaplock daemon command line.");
        std::cerr << "error: unexpected parameter found on snaplock daemon command line." << std::endl;
        std::cerr << f_opt.usage(advgetopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR);
        exit(1);
        snap::NOTREACHED();
    }

    f_start_time = time(nullptr);

    // add ourselves to the list of computers
    //
    // mark ourselves as connected, obviously
    //
    // as a side effect: it generates our identifier
    //
    f_computers[f_server_name] = std::make_shared<computer_t>(f_server_name, priority);
    f_computers[f_server_name]->set_start_time(f_start_time);
    f_computers[f_server_name]->set_connected(true);
    f_my_id = f_computers[f_server_name]->get_id();
    f_my_ip_address = f_computers[f_server_name]->get_ip_address();
}


/** \brief Do some clean ups.
 *
 * At this point, the destructor is present mainly because we have
 * some virtual functions.
 */
snaplock::~snaplock()
{
}


/** \brief Run the snaplock daemon.
 *
 * This function is the core function of the daemon. It runs the loop
 * used to lock processes from any number of computers that have access
 * to the snaplock daemon network.
 */
void snaplock::run()
{
    // Stop on these signals, log them, then terminate.
    //
    signal( SIGSEGV, snaplock::sighandler );
    signal( SIGBUS,  snaplock::sighandler );
    signal( SIGFPE,  snaplock::sighandler );
    signal( SIGILL,  snaplock::sighandler );
    signal( SIGTERM, snaplock::sighandler );
    signal( SIGINT,  snaplock::sighandler );
    signal( SIGQUIT, snaplock::sighandler );

    // Continue, but let us know by adding one line to the logs
    //
    signal( SIGPIPE, snaplock::sigloghandler );

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
    f_interrupt.reset(new snaplock_interrupt(this));
    f_communicator->add_connection(f_interrupt);

    // timer so we can timeout locks
    //
    f_timer.reset(new snaplock_timer(this));
    f_communicator->add_connection(f_timer);

    // capture SIGUSR1 to print out information
    //
    f_info.reset(new snaplock_info(this));
    f_communicator->add_connection(f_info);

    // capture SIGUSR2 to print out information
    //
    f_debug_info.reset(new snaplock_debug_info(this));
    f_communicator->add_connection(f_debug_info);

    // create a messenger to communicate with the Snap Communicator process
    // and other services as required
    //
    if(f_opt.is_defined("list"))
    {
        snap::logging::set_log_output_level(snap::logging::log_level_t::LOG_LEVEL_ERROR);

        // in this case create a snaplock_tool() which means most messages
        // are not going to function; and once ready, it will execute the
        // function specified on the command line such as --list
        //
        f_service_name = "snaplocktool";
        f_messenger.reset(new snaplock_tool(this, f_communicator_addr.toUtf8().data(), f_communicator_port));
    }
    else
    {
        SNAP_LOG_INFO("--------------------------------- snaplock started.");

        f_messenger.reset(new snaplock_messenger(this, f_communicator_addr.toUtf8().data(), f_communicator_port));
        f_messenger->set_dispatcher(shared_from_this());
    }
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
 * The signals are setup after the construction of the snaplock
 * object because that is where we initialize the logger.
 *
 * \param[in] sig  The signal that was just emitted by the OS.
 */
void snaplock::sighandler(int sig)
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


void snaplock::sigloghandler(int sig)
{
    std::string signame;

    switch(sig)
    {
    case SIGPIPE:
        signame = "SIGPIPE";
        break;

    default:
        signame = "UNKNOWN";
        break;

    }

    SNAP_LOG_WARNING("POSIX signal caught: ")(signame);

    // in this case we return because we want the process to continue
    //
    return;
}





/** \brief Forward the message to the messenger.
 *
 * The dispatcher needs to be able to send messages (some replies are sent
 * from the dispatcher code directly). This function allows for such to
 * happen.
 *
 * The function simply forwards the messages to the messenger queue.
 *
 * \param[in] message  The message to be forwarded.
 * \param[in] cache  Whether the message can be cached if it can't be
 *                   dispatched immediately.
 */
bool snaplock::send_message(snap::snap_communicator_message const & message, bool cache)
{
    return f_messenger->send_message(message, cache);
}


/** \brief Return the number of known computers running snaplock.
 *
 * This function is used by the snaplock_ticket objects to calculate
 * the quorum so as to know how many computers need to reply to
 * our messages before we can be sure we got the correct
 * results.
 *
 * \return The number of instances of snaplock running and connected.
 */
int snaplock::get_computer_count() const
{
    return f_computers.size();
}


/** \brief Calculate the quorum number of computers.
 *
 * This function dynamically recalculates the QUORUM that is required
 * to make sure that a value is valid between all the running computers.
 *
 * Because the network can go up and down (i.e. clashes, busy, etc.)
 * the time it takes to get an answer from a computer can be really
 * high. This is generally not acceptable when attempting to do a
 * lock as quickly as possible (i.e. low microseconds).
 *
 * The way to avoid having to wait for all the computers to answer is
 * to use the quorum number of computers which is a little more than
 * half:
 *
 * \f[
 *      {number of computers} over 2 + 1
 * \f]
 *
 * So if you using 4 or 5 computers for the lock, we need an answer
 * from 3 computers to make sure that we have the correct value.
 *
 * As computers running snaplock appear and disappear, the quorum
 * number will change, dynamically.
 */
int snaplock::quorum() const
{
    return f_computers.size() / 2 + 1;
}


/** \brief Get the name of the server we are running on.
 *
 * This function returns the name of the server this instance of
 * snaplock is running. It is used by the ticket implementation
 * to know whether to send a reply to the snap_lock object (i.e.
 * at this time we can send messages to that object only from the
 * server it was sent from.)
 *
 * \return The name of the server snaplock is running on.
 */
QString const & snaplock::get_server_name() const
{
    return f_server_name;
}


/** \brief Check whethre snaplock is ready to process lock requests.
 *
 * This function checks whether snaplock is ready by looking at whether
 * it has leaders and if so, whether each leader is connected.
 *
 * Once both tests succeeds, this snaplock can forward the locks to
 * the leaders. If it is a leader itself, it can enter a ticket in
 * the selection and message both of the other leaders about it.
 *
 * \return true once locks can be processed.
 */
bool snaplock::is_ready() const
{
    // without at least one leader we are definitely not ready
    //
    if(f_leaders.empty())
    {
        SNAP_LOG_TRACE("not considered ready: no leaders.");
        return false;
    }

    // enough leaders for that cluster?
    //
    // we consider that having at least 2 leaders is valid because locks
    // will still work, an election should be happening when we lose a
    // leader fixing that temporary state
    //
    // the test below allows for the case where we have a single computer
    // too (i.e. "one neighbor")
    //
    // notice how not having received the CLUSTERUP would be taken in
    // account here since f_neighbors_count will still be 0 in that case
    // (however, the previous empty() test already take that in account)
    //
    if(f_leaders.size() == 1
    && f_neighbors_count != 1)
    {
        SNAP_LOG_TRACE("not considered ready: no enough leaders for this cluster.");
        return false;
    }

    // the election_status() function verifies that the quorum is
    // attained, but it can change if the cluster grows or shrinks
    // so we have to check here again as the lock system becomes
    // "unready" when the quorum is lost; see that other function
    // for additional info

    // this one probably looks complicated...
    //
    // if our quorum is 1 or 2 then we need a number of computers
    // equal to the total number of computers (i.e. a CLUSTERCOMPLETE
    // status which we compute here)
    //
    if(f_neighbors_quorum < 3
    && f_computers.size() < f_neighbors_count)
    {
        SNAP_LOG_TRACE("not considered ready: quorum changed, re-election expected soon.");
        return false;
    }

    // the neighbors count & quorum can change over time so
    // we have to verify that the number of computers is
    // still acceptable here
    //
    if(f_computers.size() < f_neighbors_quorum)
    {
        SNAP_LOG_TRACE("not considered ready: quorum lost, re-election expected soon.");
        return false;
    }

    // are all leaders connected to us?
    //
    for(auto const & l : f_leaders)
    {
        if(!l->get_connected())
        {
            SNAP_LOG_TRACE("not considered ready: no direct connection with leader: \"")
                          (l->get_name())
                          ("\".");

            // attempt resending a LOCKSTARTED because it could be that it
            // did not work quite right and the snaplock daemons are not
            // going to ever talk with each others otherwise
            //
            // we also make sure we do not send the message too many times,
            // in five seconds it should be resolved...
            //
            time_t const now(time(nullptr));
            if(now > f_pace_lockstarted)
            {
                // pause for 5 to 6 seconds in case this happens a lot
                //
                f_pace_lockstarted = now + 5;

                // only send it to that specific server snaplock daemon
                //
                snap::snap_communicator_message temporary_message;
                temporary_message.set_sent_from_server(l->get_name());
                temporary_message.set_sent_from_service("snaplock");
                const_cast<snaplock *>(this)->send_lockstarted(&temporary_message);
            }

            return false;
        }
    }

    // it looks like we are ready
    //
    return true;
}


/** \brief Check whether we are a leader.
 *
 * This function goes through the list of leaders to determine whether
 * this snaplock is one of them, if so it returns that leader
 * computer_t object. Otherwise it returns a null pointer.
 *
 * \warning
 * This function is considered slow since it goes through the list each
 * time. On the other hand, it's only 1 to 3 leaders. Yet, you should
 * cache the result within your function if you need to call the function
 * multiple times, as in:
 *
 * \code
 *      computer_t::pointer_t leader(is_leader());
 *      // ... then use `leader` any number of times ...
 * \endcode
 *
 * \par
 * This done that way so the function is dynamic and the result can
 * change over time.
 *
 * \param[in] id  The identifier of the leader to search, if empty, default
 *                to f_my_id (i.e. whether this snaplock is a leader).
 *
 * \return Our computer_t::pointer_t or a null pointer.
 */
snaplock::computer_t::pointer_t snaplock::is_leader(QString id) const
{
    if(id.isEmpty())
    {
        id = f_my_id;
    }

    for(auto const & l : f_leaders)
    {
        if(l->get_id() == id)
        {
            return l;
        }
    }

    return computer_t::pointer_t();
}


snaplock::computer_t::pointer_t snaplock::get_leader_a() const
{
#ifdef _DEBUG
    if(!is_leader())
    {
        throw snaplock_exception_content_invalid_usage("snaplock::get_leader_a(): only a leader can call this function.");
    }
#endif

    switch(f_leaders.size())
    {
    case 0:
    default:
        throw snaplock_exception_content_invalid_usage("snaplock::get_leader_a(): call this function only when leaders were elected.");

    case 1:
        return computer_t::pointer_t(nullptr);

    case 2:
    case 3:
        return f_leaders[f_leaders[0]->is_self() ? 1 : 0];

    }
}


snaplock::computer_t::pointer_t snaplock::get_leader_b() const
{
#ifdef _DEBUG
    if(!is_leader())
    {
        throw snaplock_exception_content_invalid_usage("snaplock::get_leader_b(): only a leader can call this function.");
    }
#endif

    switch(f_leaders.size())
    {
    case 0:
    default:
        throw snaplock_exception_content_invalid_usage("snaplock::get_leader_b(): call this function only when leaders were elected.");

    case 1:
    case 2: // we have a leader A but no leader B when we have only 2 leaders
        return computer_t::pointer_t(nullptr);

    case 3:
        return f_leaders[f_leaders[2]->is_self() ? 1 : 2];

    }
}



/** \brief Output various data about the snaplock current status.
 *
 * This function outputs the current status of a snaplock daemon to
 * the snaplock.log file.
 *
 * This is used to debug a snaplock instance and make sure that the
 * state is how you would otherwise expect it to be.
 */
void snaplock::info()
{
    SNAP_LOG_INFO("++++++++ SNAPLOCK INFO ++++++++");
    SNAP_LOG_INFO("My leader ID: ")(f_my_id);
    SNAP_LOG_INFO("My IP address: ")(f_my_ip_address);
    SNAP_LOG_INFO("Total number of computers: ")(f_neighbors_count)(" (quorum: ")(f_neighbors_quorum)(", leaders: ")(f_leaders.size())(")");
    SNAP_LOG_INFO("Known computers: ")(f_computers.size());
    for(auto const & c : f_computers)
    {
        auto const it(std::find_if(
                  f_leaders.begin()
                , f_leaders.end()
                , [&c](auto const & l)
                {
                    return c.second == l;
                }));
        QString leader;
        if(it != f_leaders.end())
        {
            leader = QString(" (LEADER #%1)").arg(it - f_leaders.begin());
        }
        SNAP_LOG_INFO(" --          Computer Name: ")(c.second->get_name())(leader);
        SNAP_LOG_INFO(" --            Computer ID: ")(c.second->get_id());
        SNAP_LOG_INFO(" --    Computer IP Address: ")(c.second->get_ip_address());
    }

}


void snaplock::debug_info()
{
#ifdef _DEBUG
SNAP_LOG_TRACE("++++ serialized tickets in debug_info(): ")(serialized_tickets().replace("\n", " --- "));
    //if(f_computers.size() != 100)
    //{
    //    SNAP_LOG_INFO("++++ COMPUTER ")(f_communicator_port)(" is not fully connected to all computers?");
    //}
    //if(f_leaders.size() != 3)
    //{
    //    SNAP_LOG_INFO("++++ COMPUTER ")(f_communicator_port)(" does not (yet?) have 3 leaders");
    //}
    //else
    //{
    //    SNAP_LOG_INFO(" -- ")(f_leaders[0]->get_name())(" + ")(f_leaders[1]->get_name())(" + ")(f_leaders[2]->get_name());
    //}
#else
    SNAP_LOG_INFO("this version of snaplock is not a debug version. The debug_info() function does nothing in this version.");
#endif
}




/** \brief Generate the output for "snaplock --list"
 *
 * This function loops over the list of tickets and outpurs a string that
 * it sends back to the `snaplock --list` command for printing to the
 * user.
 *
 * \param[in] message  The message to reply to.
 */
void snaplock::msg_list_tickets(snap::snap_communicator_message & message)
{
    QString ticketlist;
    for(auto const & obj_ticket : f_tickets)
    {
        for(auto const & key_ticket : obj_ticket.second)
        {
            QString const & obj_name(key_ticket.second->get_object_name());
            QString const & key(key_ticket.second->get_entering_key());
            snaplock_ticket::ticket_id_t const ticket_id(key_ticket.second->get_ticket_number());
            time_t const lock_timeout(key_ticket.second->get_lock_timeout());

            QString timeout_msg;
            if(lock_timeout == 0)
            {
                time_t const obtention_timeout(key_ticket.second->get_obtention_timeout());
                timeout_msg = QString("obtention %1 %2")
                            .arg(snap::snap_child::date_to_string(obtention_timeout * 1000000LL, snap::snap_child::date_format_t::DATE_FORMAT_SHORT))
                            .arg(snap::snap_child::date_to_string(obtention_timeout * 1000000LL, snap::snap_child::date_format_t::DATE_FORMAT_TIME));
            }
            else
            {
                timeout_msg = QString("timeout %1 %2")
                            .arg(snap::snap_child::date_to_string(lock_timeout * 1000000LL, snap::snap_child::date_format_t::DATE_FORMAT_SHORT))
                            .arg(snap::snap_child::date_to_string(lock_timeout * 1000000LL, snap::snap_child::date_format_t::DATE_FORMAT_TIME));
            }

            QString const msg(QString("ticket id: %1  object name: \"%2\"  key: %3  %4\n")
                            .arg(ticket_id)
                            .arg(obj_name)
                            .arg(key)
                            .arg(timeout_msg));
            ticketlist += msg;
        }
    }
    snap::snap_communicator_message list_message;
    list_message.set_command("TICKETLIST");
    list_message.reply_to(message);
    list_message.add_parameter("list", ticketlist);
    send_message(list_message);
}


/** \brief Send the CLUSTERSTATUS to snapcommunicator.
 *
 * This function builds a message and sends it to snapcommunicator.
 *
 * The CLUSTERUP and CLUSTERDOWN messages are sent only when that specific
 * event happen and until then we do not know what the state really is
 * (although we assume the worst and use CLUSTERDOWN until we get a reply.)
 */
void snaplock::ready(snap::snap_communicator_message & message)
{
    snap::NOTUSED(message);

    snap::snap_communicator_message clusterstatus_message;
    clusterstatus_message.set_command("CLUSTERSTATUS");
    clusterstatus_message.set_service("snapcommunicator");
    send_message(clusterstatus_message);
}


void snaplock::msg_cluster_up(snap::snap_communicator_message & message)
{
    f_neighbors_count = message.get_integer_parameter("neighbors_count");
    f_neighbors_quorum = f_neighbors_count / 2 + 1;

    SNAP_LOG_INFO("cluster is up with ")
                 (f_neighbors_count)
                 (" neighbors, attempt an election then check for leaders by sending a LOCKSTARTED message.");

    election_status();

    send_lockstarted(nullptr);
}


void snaplock::msg_cluster_down(snap::snap_communicator_message & message)
{
    snap::NOTUSED(message);

    // there is nothing to do here, when the cluster comes back up the
    // snapcommunicator will automatically send us a signal about it

    SNAP_LOG_INFO("cluster is down, canceling existing locks and we have to refuse any further lock requests for a while.");

    // in this case we just cannot keep the leaders
    //
    f_leaders.clear();

    // in case services listen to the NOLOCK, let them know it's gone
    //
    check_lock_status();

    // we do not call the lockgone() because the HANGUP will be sent
    // if required so we do not have to do that twice
}


void snaplock::election_status()
{
    // we already have election results?
    //
    if(!f_leaders.empty())
    {
        // the results may have been "temperred" with (i.e. one of
        // the leaders was lost)
        //
        if(f_leaders.size() == 3
        || (f_neighbors_count < 3 && f_leaders.size() == f_neighbors_count))
        {
            // this could have changed since we may get the list of
            // leaders with some of those leaders set to "disabled"
            //
            check_lock_status();
            return;
        }
    }

    // neighbors count is 0 until we receive a very first CLUSTERUP
    // (note that it does not go back to zero on CLUSTERDOWN, however,
    // the quorum as checked in the next if() is never going to be
    // reached if the cluster is down.)
    //
    if(f_neighbors_count == 0)
    {
        return;
    }

    // this one probably looks complicated...
    //
    // if our quorum is 1 or 2 then we need a number of computers
    // equal to the total number of computers (i.e. a CLUSTERCOMPLETE
    // status which we compute here)
    //
    if(f_neighbors_quorum < 3
    && f_computers.size() < f_neighbors_count)
    {
        return;
    }

    // since the neighbors count & quorum never go back to zero (on a
    // CLUSTERDOWN) we have to verify that the number of computers is
    // acceptable here
    //
    // Note: further we will not count computers marked disabled, which
    //       is done below when sorting by ID, however, that does not
    //       prevent the quorum to be attained, even with disabled
    //       computers
    //
    if(f_computers.size() < f_neighbors_quorum)
    {
        return;
    }

    // to proceed with an election we must have the smallest IP address
    // (it is not absolutely required, but that way we avoid many
    // consensus problems, in effect we have one "temporary-leader" that ends
    // up telling us who the final three leaders are.)
    //
    for(auto & c : f_computers)
    {
        // Note: the test fails when we compare to ourselves so we do not
        //       need any special case
        //
        if(c.second->get_ip_address() < f_my_ip_address)
        {
            return;
        }
    }

    // to select the leaders sort them by identifier and take the first
    // three (i.e. lower priority, random, IP, pid.)
    //
    int off(0);
    computer_t::map_t sort_by_id;
    for(auto c : f_computers)
    {
        // ignore nodes with a priority of 15 (i.e. OFF)
        //
        if(c.second->get_priority() != computer_t::PRIORITY_OFF)
        {
            QString id(c.second->get_id());

            // is this computer a leader?
            //
            auto it(std::find(f_leaders.begin(), f_leaders.end(), c.second));
            if(it != f_leaders.end())
            {
                // leaders have a priority of 00
                //
                id[0] = '0';
                id[1] = '0';
            }

            sort_by_id[id] = c.second;
        }
        else
        {
            ++off;
        }
    }

    if(f_computers.size() <= 3)
    {
        if(off != 0)
        {
            SNAP_LOG_FATAL(
                    "you cannot have any computer turned OFF when you"
                    " have three or less computers total in your cluster."
                    " The elections cannot be completed in these"
                    " conditions.");
            return;
        }
    }
    else if(f_computers.size() - off < 3)
    {
        SNAP_LOG_FATAL("you have a total of ")
                (f_computers.size())
                (" computers in your cluster. You turned off ")
                (off)
                (" of them, which means less than three are left"
                 " as candidates for leadership which is not enough."
                 " You can have a maximum of ")
                (f_computers.size() - 3)
                (" that are turned off on this cluster.");
        return;
    }

    if(sort_by_id.size() < 3
    && sort_by_id.size() != f_computers.size())
    {
        return;
    }

//std::cerr << f_communicator_port << " is conducting an election:\n";
//for(auto s : sort_by_id)
//{
//std::cerr << "  " << s.second->get_name() << "    " << s.first << "\n";
//}

    // the first three are the new leaders
    //
    snap::snap_communicator_message lockleaders_message;
    lockleaders_message.set_command("LOCKLEADERS");
    lockleaders_message.set_service("*");
    f_leaders.clear();
    f_election_date = snap::snap_child::get_current_date();
    lockleaders_message.add_parameter("election_date", f_election_date);
    auto leader(sort_by_id.begin());
    size_t const max(std::min(static_cast<computer_t::map_t::size_type>(3), sort_by_id.size()));
    for(size_t idx(0); idx < max; ++idx, ++leader)
    {
        lockleaders_message.add_parameter(QString("leader%1").arg(idx), leader->second->get_id());
        f_leaders.push_back(leader->second);
    }
    send_message(lockleaders_message);

SNAP_LOG_WARNING("election status = add leader(s)... ")(f_computers.size())(" comps and ")(f_leaders.size())(" leaders");

    // when the election succeeded we may have to send LOCK messages
    // assuming some were cached and did not yet time out
    //
    check_lock_status();
}


void snaplock::check_lock_status()
{
    bool const ready(is_ready());
    QString const current_status(ready ? "LOCKREADY" : "NOLOCK");

    if(f_lock_status != current_status)
    {
        f_lock_status = current_status;

        snap::snap_communicator_message status_message;
        status_message.set_command(current_status);
        status_message.set_service(".");
        status_message.add_parameter("cache", "no");
        send_message(status_message);

        if(ready
        && !f_message_cache.empty())
        {
            // we still have a cache of locks that can now be processed
            //
            // note:
            // although msg_lock() could re-add some of those messages
            // in the f_message_cache vector, it should not since it
            // calls the same is_read() function which we know returns
            // true and therefore no cache is required
            //
            message_cache::vector_t cache;
            cache.swap(f_message_cache);
            for(auto mc : cache)
            {
                msg_lock(mc.f_message);
            }
        }
    }
}


void snaplock::send_lockstarted(snap::snap_communicator_message const * message)
{
    // tell other snaplock instances that are already listening that
    // we are ready; this way we can calculate the number of computers
    // available in our network and use that to calculate the QUORUM
    //
    snap::snap_communicator_message lockstarted_message;
    lockstarted_message.set_command("LOCKSTARTED");
    if(message == nullptr)
    {
        lockstarted_message.set_service("*");

        // unfortunately, the following does NOT work as expected...
        // (i.e. the following ends up sending the message to ourselves only
        // and does not forward to any remote communicators.)
        //
        //lockstarted_message.set_server("*");
        //lockstarted_message.set_service("snaplock");
    }
    else
    {
        lockstarted_message.reply_to(*message);
    }

    // our info: server name and id
    //
    lockstarted_message.add_parameter("server_name", f_server_name);
    lockstarted_message.add_parameter("lockid", f_my_id);
    lockstarted_message.add_parameter("starttime", f_start_time);

    // include the leaders if present
    //
    if(!f_leaders.empty())
    {
        lockstarted_message.add_parameter("election_date", f_election_date);
        for(size_t idx(0); idx < f_leaders.size(); ++idx)
        {
            lockstarted_message.add_parameter(QString("leader%1").arg(idx), f_leaders[idx]->get_id());
        }
    }

    send_message(lockstarted_message);
}


void snaplock::msg_lock_leaders(snap::snap_communicator_message & message)
{
    f_election_date = message.get_integer_parameter("election_date");

    // save the new leaders in our own list
    //
    f_leaders.clear();
    for(int idx(0); idx < 3; ++idx)
    {
        QString const param_name(QString("leader%1").arg(idx));
        if(message.has_parameter(param_name))
        {
            computer_t::pointer_t leader(std::make_shared<computer_t>());
            QString const lockid(message.get_parameter(param_name));
            if(leader->set_id(lockid))
            {
                computer_t::map_t::iterator exists(f_computers.find(leader->get_name()));
                if(exists != f_computers.end())
                {
                    // it already exists, use our existing instance
                    //
                    f_leaders.push_back(exists->second);
                }
                else
                {
                    // we do not yet know of that computer, even though
                    // it is a leader! (i.e. we are not yet aware that
                    // somehow we are connected to it)
                    //
                    leader->set_connected(false);
                    f_computers[leader->get_name()] = leader;

                    f_leaders.push_back(leader);
                }
            }
        }
    }

    if(!f_leaders.empty())
    {
        synchronize_leaders();

        // set the round-robin position to a random value
        //
        // note: I know the result is likely skewed, c will be set to
        // a number between 0 and 255 and modulo 3 means that you get
        // one extra zero (255 % 3 == 0); however, there are 85 times
        // 3 in 255 so it probably won't be noticeable.
        //
        uint8_t c;
        RAND_bytes(reinterpret_cast<unsigned char *>(&c), sizeof(c));
        f_next_leader = c % f_leaders.size();
    }

    // the is_ready() function depends on having f_leaders defined
    // and when that happens we may need to empty our cache
    //
    check_lock_status();
}


/** \brief Called whenever a snaplock computer is acknowledging itself.
 *
 * This function gets called on a LOCKSTARTED event which is sent whenever
 * a snaplock process is initialized on a computer.
 *
 * The message is expected to include the computer name. At this time
 * we cannot handle having more than one instance one the same computer.
 *
 * \param[in] message  The LOCKSTARTED message.
 */
void snaplock::msg_lock_started(snap::snap_communicator_message & message)
{
    // get the server name (that other server telling us it is ready)
    //
    QString const server_name(message.get_parameter("server_name"));
    if(server_name.isEmpty())
    {
        // name missing
        //
        throw snap::snap_communicator_invalid_message("snaplock::msg_lockstarted(): Invalid server name (empty).");
    }

    // I do not think we would even message ourselves, but in case it happens
    // the rest of the function does not support that case well
    //
    if(server_name == f_server_name)
    {
        return;
    }

    time_t const start_time(message.get_integer_parameter("starttime"));

    computer_t::map_t::iterator it(f_computers.find(server_name));
    bool new_computer(it == f_computers.end());
    if(new_computer)
    {
        // create a computer instance so we know it exists
        //
        computer_t::pointer_t computer(std::make_shared<computer_t>());

        // fill the fields from the "lockid" parameter
        //
        if(!computer->set_id(message.get_parameter("lockid")))
        {
            // this is not a valid identifier, ignore altogether
            //
            return;
        }
        computer->set_start_time(start_time);

        f_computers[computer->get_name()] = computer;
    }
    else
    {
        if(!it->second->get_connected())
        {
            // we heard of this computer (because it is/was a leader) but
            // we had not yet received a LOCKSTARTED message from it; so here
            // we consider it a new computer and will reply to the LOCKSTARTED
            //
            new_computer = true;
            it->second->set_connected(true);
        }

        if(it->second->get_start_time() != start_time)
        {
            // when the start time changes that means snaplock
            // restarted which can happen without snapcommunicator
            // restarting so here we would not know about the feat
            // without this parameter and in this case it is very
            // much the same as a new computer so send it a
            // LOCKSTARTED message back!
            //
            new_computer = true;
            it->second->set_start_time(start_time);
        }
    }

    // keep the newest election results
    //
    if(message.has_parameter("election_date"))
    {
        int64_t const election_date(message.get_integer_parameter("election_date"));
        if(election_date > f_election_date)
        {
            f_election_date = election_date;
            f_leaders.clear();
        }
    }

    bool const set_my_leaders(f_leaders.empty());
    if(set_my_leaders)
    {
        for(int idx(0); idx < 3; ++idx)
        {
            QString const param_name(QString("leader%1").arg(idx));
            if(message.has_parameter(param_name))
            {
                computer_t::pointer_t leader(std::make_shared<computer_t>());
                QString const lockid(message.get_parameter(param_name));
                if(leader->set_id(lockid))
                {
                    computer_t::map_t::iterator exists(f_computers.find(leader->get_name()));
                    if(exists != f_computers.end())
                    {
                        // it already exists, use our existing instance
                        //
                        f_leaders.push_back(exists->second);
                    }
                    else
                    {
                        // we do not yet know of that computer, even though
                        // it is a leader! (i.e. we are not yet aware that
                        // somehow we are connected to it)
                        //
                        leader->set_connected(false);
                        f_computers[leader->get_name()] = leader;

                        f_leaders.push_back(leader);
                    }
                }
            }
        }
    }

    election_status();

    if(new_computer)
    {
        // send a reply if that was a new computer
        //
        send_lockstarted(&message);
    }
}


/** \brief A service asked about the lock status.
 *
 * The lock status is whether the snaplock service is ready to receive
 * LOCK messages (LOCKREADY) or is still waiting on a CLUSTERUP and
 * LOCKLEADERS to happen (NOLOCK.)
 *
 * Note that LOCK messages are accepted while the lock service is not
 * yet ready, however, those are cached and it is more likely that they
 * will timeout.
 *
 * \param[in] message  The message to reply to.
 */
void snaplock::msg_lock_status(snap::snap_communicator_message & message)
{
    snap::snap_communicator_message status_message;
    status_message.set_command(is_ready() ? "LOCKREADY" : "NOLOCK");
    status_message.reply_to(message);
    status_message.add_parameter("cache", "no");
    send_message(status_message);
}


/** \brief Another snaplock is sending us its list of tickets.
 *
 * Whenever a snaplock dies, a new one is quickly promoted as a leader
 * and that new leader would have no idea about the existing tickets
 * (locks) so the other two send it a LOCKTICKETS message.
 *
 * The tickets are defined in the parameter of the same name using
 * the serialization function to transform the objects in a string.
 * Here we can unserialize that string accordingly.
 *
 * First we extract the object name and entering key to see whether
 * we have that ticket already defined. If so, then we unserialize
 * in that existing object. The extraction is additive so we can do
 * it any number of times.
 *
 * \param[in] message  The message to reply to.
 */
void snaplock::msg_lock_tickets(snap::snap_communicator_message & message)
{
    QString const tickets(message.get_parameter("tickets"));

    // we have one ticket per line, so we first split per line and then
    // work on one line at a time
    //
    snap::snap_string_list const lines(tickets.split('\n'));
    for(auto const & l : lines)
    {
        snaplock_ticket::pointer_t ticket;
        snap::snap_string_list const vars(l.split('|'));
        auto object_name_value(std::find_if(
                  vars.begin()
                , vars.end()
                , [](QString const & vv)
                {
                    return vv.startsWith("object_name=");
                }));
        if(object_name_value != vars.end())
        {
            auto entering_key_value(std::find_if(
                      vars.begin()
                    , vars.end()
                    , [](QString const & vv)
                    {
                        return vv.startsWith("entering_key=");
                    }));
            if(entering_key_value != vars.end())
            {
                // extract the values which start after the '=' sign
                //
                QString const object_name(object_name_value->mid(12));
                QString const entering_key(entering_key_value->mid(13));

                auto entering_ticket(f_entering_tickets.find(object_name));
                if(entering_ticket != f_entering_tickets.end())
                {
                    auto key_ticket(entering_ticket->second.find(entering_key));
                    if(key_ticket != entering_ticket->second.end())
                    {
                        ticket = key_ticket->second;
                    }
                }
                if(ticket == nullptr)
                {
                    auto obj_ticket(f_tickets.find(object_name));
                    if(obj_ticket != f_tickets.end())
                    {
                        auto key_ticket(std::find_if(
                                  obj_ticket->second.begin()
                                , obj_ticket->second.end()
                                , [&entering_key](auto const & t)
                                {
                                    return t.second->get_entering_key() == entering_key;
                                }));
                        if(key_ticket != obj_ticket->second.end())
                        {
                            ticket = key_ticket->second;
                        }
                    }
                }

                // ticket exists? if not create a new one
                //
                bool const new_ticket(ticket == nullptr);
                if(new_ticket)
                {
                    // creaet a new ticket, some of the parameters are there just
                    // because they are required; they will be replaced by the
                    // unserialize call...
                    //
                    ticket = std::make_shared<snaplock_ticket>(
                                                  this
                                                , f_messenger
                                                , object_name
                                                , entering_key
                                                , snap::snap_lock::SNAP_LOCK_DEFAULT_TIMEOUT + time(nullptr)
                                                , snap::snap_lock::SNAP_LOCK_DEFAULT_TIMEOUT
                                                , f_server_name
                                                , "snaplock");
                }

                ticket->unserialize(l);

                // do a couple of additional sanity tests to
                // make sure that we want to keep new tickets
                //
                // first make sure it is marked as "locked"
                //
                // second check that the owner is a leader that
                // exists (the sender uses a LOCK message for
                // locks that are not yet locked or require
                // a new owner)
                //
                if(new_ticket
                && ticket->is_locked())
                {
                    auto li(std::find_if(
                              f_leaders.begin()
                            , f_leaders.end()
                            , [&ticket](auto const & c)
                            {
                                return ticket->get_owner() == c->get_name();
                            }));
                    if(li != f_leaders.end())
                    {
                        f_tickets[object_name][ticket->get_ticket_key()] = ticket;
                    }
                }
            }
        }
    }
}


/** \brief With the STATUS message we know of new snapcommunicators.
 *
 * This function captures the STATUS message and if it sees that the
 * name of the service is "remote communicator connection" then it
 * sends a new LOCKSTARTED message to make sure that all snaplock's
 * are aware of us.
 *
 * \param[in] message  The LOCKSTARTED message.
 */
void snaplock::msg_status(snap::snap_communicator_message & message)
{
    // check the service name, it has to be one that means it is a remote
    // connection with another snapcommunicator
    //
    QString const service(message.get_parameter("service"));
    if(service == "remote connection"                   // remote host connected to us
    || service == "remote communicator connection")     // we connected to remote host
    {
        // check what the status is now: "up" or "down"
        //
        QString const status(message.get_parameter("status"));
        if(status == "up")
        {
            // we already broadcast a LOCKSTARTED from CLUSTERUP
            // and that's enough
            //
        }
        else
        {
            // host is down, remove from our list of hosts
            //
            msg_server_gone(message);
        }
    }
}


/** \brief Called whenever a remote connection is disconnected.
 *
 * This function is used to know that a remote connection was
 * disconnected.
 *
 * We receive the HANGUP whenever a remote connection hangs
 * up or snapcommunicator received a DISCONNECT message.
 *
 * This allows us to manage the f_computers list of computers running
 * snaplock.
 *
 * \param[in] message  The LOCKSTARTED message.
 */
void snaplock::msg_server_gone(snap::snap_communicator_message & message)
{
    // was it a snaplock service at least?
    //
    QString const server_name(message.get_parameter("server_name"));
    if(server_name.isEmpty()
    || server_name == f_server_name)
    {
        // we never want to remove ourselves?!
        //
        return;
    }

    // is "server_name" known?
    //
    auto it(f_computers.find(server_name));
    if(it == f_computers.end())
    {
        // no computer found, nothing else to do here
        //
        return;
    }

    // got it, remove it
    //
    f_computers.erase(it);

    // is that computer a leader?
    //
    auto li(std::find(
              f_leaders.begin()
            , f_leaders.end()
            , it->second));
    if(li != f_leaders.end())
    {
        f_leaders.erase(li);

        // elect another computer in case the one we just erased was a leader
        //
        // (of course, no elections occur unless we are the computer with the
        // smallest IP address)
        //
        election_status();

        // if too many leaders were dropped, we may go back to the NOLOCK status
        //
        // we only send a NOLOCK if the election could not re-assign another
        // computer as the missing leader(s)
        //
        check_lock_status();
    }
}


/** \brief Called whenever we receive the STOP command or equivalent.
 *
 * This function makes sure the snaplock exits as quickly as
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
void snaplock::stop(bool quitting)
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
            cmd.add_parameter("service", f_service_name);
            send_message(cmd);
        }
    }

    if(f_communicator != nullptr)
    {
        f_communicator->remove_connection(f_interrupt);
        f_interrupt.reset();

        f_communicator->remove_connection(f_info);
        f_info.reset();

        f_communicator->remove_connection(f_debug_info);
        f_debug_info.reset();

        f_communicator->remove_connection(f_timer);
        f_timer.reset();
    }
}


/** \brief Try to get a set of parameters.
 *
 * This function attempts to get the specified set of parameters from the
 * specified message.
 *
 * The function throws if a parameter is missing or invalid (i.e. an
 * integer is not valid.)
 *
 * \note
 * The timeout parameter is always viewed as optional. It is set to
 * "now + DEFAULT_TIMEOUT" if undefined in the message. If specified in
 * the message, there is no minimum or maximum (i.e. it may already
 * have timed out.)
 *
 * \exception snap_communicator_invalid_message
 * If a required parameter (object_name, client_pid, or key) is missing
 * then this exception is raised. You will have to fix your software
 * to avoid the exception.
 *
 * \param[in] message  The message from which we get parameters.
 * \param[out] object_name  A pointer to a QString that receives the object name.
 * \param[out] client_pid  A pointer to a pid_t that receives the client pid.
 * \param[out] timeout  A pointer to an int64_t that receives the timeout date in seconds.
 * \param[out] key  A pointer to a QString that receives the key parameter.
 * \param[out] source  A pointer to a QString that receives the source parameter.
 *
 * \return true if all specified parameters could be retrieved.
 */
void snaplock::get_parameters(snap::snap_communicator_message const & message, QString * object_name, pid_t * client_pid, time_t * timeout, QString * key, QString * source)
{
    // get the "object name" (what we are locking)
    // in Snap, the object name is often a URI plus the action we are performing
    //
    if(object_name != nullptr)
    {
        *object_name = message.get_parameter("object_name");
        if(object_name->isEmpty())
        {
            // name missing
            //
            throw snap::snap_communicator_invalid_message("snaplock::get_parameters(): Invalid object name. We cannot lock the empty string.");
        }
    }

    // get the pid (process identifier) of the process that is
    // requesting the lock; this is important to be able to distinguish
    // multiple processes on the same computer requesting a lock
    //
    if(client_pid != nullptr)
    {
        *client_pid = message.get_integer_parameter("pid");
        if(*client_pid < 1)
        {
            // invalid pid
            //
            throw snap::snap_communicator_invalid_message(QString("snaplock::get_parameters(): Invalid pid specified for a lock (%1). It must be a positive decimal number.").arg(message.get_parameter("pid")).toUtf8().data());
        }
    }

    // get the time limit we will wait up to before we decide we
    // cannot obtain that lock
    //
    if(timeout != nullptr)
    {
        if(message.has_parameter("timeout"))
        {
            // this timeout may already be out of date in which case
            // the lock immediately fails
            //
            *timeout = message.get_integer_parameter("timeout");
        }
        else
        {
            *timeout = time(nullptr) + DEFAULT_TIMEOUT;
        }
    }

    // get the key of a ticket or entering object
    //
    if(key != nullptr)
    {
        *key = message.get_parameter("key");
        if(key->isEmpty())
        {
            // key missing
            //
            throw snap::snap_communicator_invalid_message("snaplock::get_parameters(): A key cannot be an empty string.");
        }
    }

    // get the key of a ticket or entering object
    //
    if(source != nullptr)
    {
        *source = message.get_parameter("source");
        if(source->isEmpty())
        {
            // source missing
            //
            throw snap::snap_communicator_invalid_message("snaplock::get_parameters(): A source cannot be an empty string.");
        }
    }
}


/** \brief Lock the resource after confirmation that client is alive.
 *
 * This message is expected just after we sent an ALIVE message to
 * the client.
 *
 * Whenever a leader dies, we suspect that the client may have died
 * with it so we send it an ALIVE message to know whether it is worth
 * the trouble of entering that lock.
 *
 * \param[in] message  The ABSOLUTE message to handle.
 */
void snaplock::msg_absolutely(snap::snap_communicator_message & message)
{
    QString const serial(message.get_parameter("serial"));
    snap::snap_string_list const segments(serial.split('/'));

    if(segments[0] == "relock")
    {
        // check serial as defined in msg_lock()
        // alive_message.add_parameter("serial", QString("relock/%1/%2").arg(object_name).arg(entering_key));
        //
        if(segments.size() != 4)
        {
            SNAP_LOG_WARNING("ABSOLUTELY reply has an invalid relock serial parameters \"")
                            (serial)
                            ("\" was expected to have exactly 4 segments.");

            snap::snap_communicator_message lock_failed_message;
            lock_failed_message.set_command("LOCKFAILED");
            lock_failed_message.reply_to(message);
            lock_failed_message.add_parameter("object_name", "unknown");
            lock_failed_message.add_parameter("error", "invalid");
            send_message(lock_failed_message);

            return;
        }

        // notice how the split() re-split the entering key
        //
        QString const object_name(segments[1]);
        QString const server_name(segments[2]);
        QString const client_pid(segments[3]);

        auto entering_ticket(f_entering_tickets.find(object_name));
        if(entering_ticket != f_entering_tickets.end())
        {
            QString const entering_key(QString("%1/%2").arg(server_name).arg(client_pid));
            auto key_ticket(entering_ticket->second.find(entering_key));
            if(key_ticket != entering_ticket->second.end())
            {
                // remove the alive timeout
                //
                key_ticket->second->set_alive_timeout(0);

                // got it! start the bakery algorithm
                //
                key_ticket->second->entering();
            }
        }
    }

    // ignore other messages
}


/** \brief Lock the resource.
 *
 * This function locks the specified resource \p object_name. It returns
 * when the resource is locked or when the lock timeout is reached.
 *
 * See the snaplock_ticket class for more details about the locking
 * mechanisms (algorithm and MSC implementation).
 *
 * Note that if lock() is called with an empty string then the function
 * unlocks the lock and returns immediately with false. This is equivalent
 * to calling unlock().
 *
 * \note
 * The function reloads all the parameters (outside of the table) because
 * we need to support a certain amount of dynamism. For example, an
 * administrator may want to add a new host on the system. In that case,
 * the list of host changes and it has to be detected here.
 *
 * \attention
 * The function accepts a "serial" parameter in the message. This is only
 * used internally when a leader is lost and a new one is assigned a lock
 * which would otherwise fail.
 *
 * \warning
 * The object name is left available in the lock table. Do not use any
 * secure/secret name/word, etc. as the object name.
 *
 * \bug
 * At this point there is no proper protection to recover from errors
 * that would happen while working on locking this entry. This means
 * failures may result in a lock that never ends.
 *
 * \param[in] message  The lock message.
 *
 * \return true if the lock was successful, false otherwise.
 *
 * \sa unlock()
 */
void snaplock::msg_lock(snap::snap_communicator_message & message)
{
    QString object_name;
    pid_t client_pid(0);
    time_t timeout(0);
    get_parameters(message, &object_name, &client_pid, &timeout, nullptr, nullptr);

    // do some cleanup as well
    //
    cleanup();

    // if we are a leader, create an entering key
    //
    QString const server_name(message.has_parameter("lock_proxy_server_name")
                                ? message.get_parameter("lock_proxy_server_name")
                                : message.get_sent_from_server());

    QString const service_name(message.has_parameter("lock_proxy_service_name")
                                ? message.get_parameter("lock_proxy_service_name")
                                : message.get_sent_from_service());

    QString const entering_key(QString("%1/%2").arg(server_name).arg(client_pid));

    if(timeout <= time(nullptr))
    {
        SNAP_LOG_WARNING("Lock on \"")
                        (object_name)
                        ("\" / \"")
                        (client_pid)
                        ("\" timed out before we could start the locking process.");

        snap::snap_communicator_message lock_failed_message;
        lock_failed_message.set_command("LOCKFAILED");
        lock_failed_message.reply_to(message);
        lock_failed_message.add_parameter("object_name", object_name);
        lock_failed_message.add_parameter("key", entering_key);
        lock_failed_message.add_parameter("error", "timedout");
        send_message(lock_failed_message);

        return;
    }

    snap::snap_lock::timeout_t const duration(message.get_integer_parameter("duration"));
    if(duration < snap::snap_lock::SNAP_LOCK_MINIMUM_TIMEOUT)
    {
        // invalid duration, minimum is 3
        //
        SNAP_LOG_ERROR(duration)
                      (" is an invalid duration, the minimum accepted is ")
                      (snap::snap_lock::SNAP_LOCK_MINIMUM_TIMEOUT)
                      (".");

        snap::snap_communicator_message lock_failed_message;
        lock_failed_message.set_command("LOCKFAILED");
        lock_failed_message.reply_to(message);
        lock_failed_message.add_parameter("object_name", object_name);
        lock_failed_message.add_parameter("key", entering_key);
        lock_failed_message.add_parameter("error", "invalid");
        send_message(lock_failed_message);

        return;
    }

    snap::snap_lock::timeout_t unlock_duration(snap::snap_lock::SNAP_UNLOCK_USES_LOCK_TIMEOUT);
    if(message.has_parameter("unlock_duration"))
    {
        unlock_duration = message.get_integer_parameter("unlock_duration");
        if(unlock_duration < snap::snap_lock::SNAP_UNLOCK_MINIMUM_TIMEOUT)
        {
            // invalid duration, minimum is 60
            //
            SNAP_LOG_ERROR(unlock_duration)
                          (" is an invalid unlock duration, the minimum accepted is ")
                          (snap::snap_lock::SNAP_UNLOCK_MINIMUM_TIMEOUT)
                          (".");

            snap::snap_communicator_message lock_failed_message;
            lock_failed_message.set_command("LOCKFAILED");
            lock_failed_message.reply_to(message);
            lock_failed_message.add_parameter("object_name", object_name);
            lock_failed_message.add_parameter("key", entering_key);
            lock_failed_message.add_parameter("error", "invalid");
            send_message(lock_failed_message);

            return;
        }
    }

    if(!is_ready())
    {
        SNAP_LOG_TRACE("caching LOCK message for \"")
                      (object_name)
                      ("\" as the snaplock system is not yet considered ready.");

        message_cache const mc
            {
                timeout,
                message
            };
        f_message_cache.push_back(mc);

        // make sure the cache gets cleaned up if the message times out
        //
        int64_t const timeout_date(f_messenger->get_timeout_date());
        if(timeout_date == -1
        || timeout_date > timeout)
        {
            f_timer->set_timeout_date(timeout);
        }
        return;
    }

    if(is_leader() == nullptr)
    {
        // we are not a leader, we need to forward the message to one
        // of the leaders instead
        //
        forward_message_to_leader(message);
        return;
    }

    // make sure there is not a ticket with the same name already defined
    //
    auto entering_ticket(f_entering_tickets.find(object_name));
    if(entering_ticket != f_entering_tickets.end())
    {
        auto key_ticket(entering_ticket->second.find(entering_key));
        if(key_ticket != entering_ticket->second.end())
        {
            // if this is a re-LOCK, then it may be a legitimate duplicate
            // in which case we do not want to generate a LOCKFAILED error
            //
            if(message.has_parameter("serial"))
            {
                snaplock_ticket::serial_t const serial(message.get_integer_parameter("serial"));
                if(key_ticket->second->get_serial() == serial)
                {
                    // legitimate double request from leaders
                    // (this happens when a leader dies and we have to restart
                    // a lock negotiation)
                    //
                    return;
                }
            }

            // the object already exists... do not allow duplicates
            //
            SNAP_LOG_ERROR("an entering ticket has the same object name \"")
                          (object_name)
                          ("\" and entering key \"")
                          (entering_key)
                          ("\".");

            snap::snap_communicator_message lock_failed_message;
            lock_failed_message.set_command("LOCKFAILED");
            lock_failed_message.reply_to(message);
            lock_failed_message.add_parameter("object_name", object_name);
            lock_failed_message.add_parameter("key", entering_key);
            lock_failed_message.add_parameter("error", "duplicate");
            send_message(lock_failed_message);

            return;
        }
    }

    // make sure there is not a ticket with the same name already defined
    //
    // (this is is really important so we can actually properly UNLOCK an
    // existing lock since we use the same search and if two entries were
    // to be the same we could not know which to unlock; there are a few
    // other places where such a search is used actually...)
    //
    auto obj_ticket(f_tickets.find(object_name));
    if(obj_ticket != f_tickets.end())
    {
        auto key_ticket(std::find_if(
                  obj_ticket->second.begin()
                , obj_ticket->second.end()
                , [&entering_key](auto const & t)
                {
                    return t.second->get_entering_key() == entering_key;
                }));
        if(key_ticket != obj_ticket->second.end())
        {
            // there is already a ticket with this name/entering key
            //
            SNAP_LOG_ERROR("a ticket has the same object name \"")
                          (object_name)
                          ("\" and entering key \"")
                          (entering_key)
                          ("\".");

            snap::snap_communicator_message lock_failed_message;
            lock_failed_message.set_command("LOCKFAILED");
            lock_failed_message.reply_to(message);
            lock_failed_message.add_parameter("object_name", object_name);
            lock_failed_message.add_parameter("key", entering_key);
            lock_failed_message.add_parameter("error", "duplicate");
            send_message(lock_failed_message);

            return;
        }
    }

    snaplock_ticket::pointer_t ticket(std::make_shared<snaplock_ticket>(
                                  this
                                , f_messenger
                                , object_name
                                , entering_key
                                , timeout
                                , duration
                                , server_name
                                , service_name));

    f_entering_tickets[object_name][entering_key] = ticket;

    // finish up ticket initialization
    //
    ticket->set_unlock_duration(unlock_duration);

    // general a serial number for that ticket
    //
    f_ticket_serial = (f_ticket_serial + 1) & 0x00FFFFFF;
    if(f_leaders[0]->get_id() != f_my_id)
    {
        if(f_leaders.size() >= 2
        && f_leaders[1]->get_id() != f_my_id)
        {
            f_ticket_serial |= 1 << 24;
        }
        else if(f_leaders.size() >= 3
             && f_leaders[2]->get_id() != f_my_id)
        {
            f_ticket_serial |= 2 << 24;
        }
    }
    ticket->set_serial(f_ticket_serial);

    if(message.has_parameter("serial"))
    {
        // if we have a "serial" number in that message, we lost a leader
        // and when that happens we are not unlikely to have lost the
        // client that requested the LOCK, send an ALIVE message to make
        // sure that the client still exists before entering the ticket
        //
        ticket->set_alive_timeout(5 + time(nullptr));

        snap::snap_communicator_message alive_message;
        alive_message.set_command("ALIVE");
        alive_message.set_server(server_name);
        alive_message.set_service(service_name);
        alive_message.add_parameter("serial", QString("relock/%1/%2").arg(object_name).arg(entering_key));
        alive_message.add_parameter("timestamp", time(nullptr));
        send_message(alive_message);
    }
    else
    {
        // act on the new ticket
        //
        ticket->entering();
    }

    // the list of tickets changed, make sure we update timeout timer
    //
    cleanup();
}


/** \brief Unlock the resource.
 *
 * This function unlocks the resource specified in the call to lock().
 *
 * \param[in] message  The unlock message.
 *
 * \sa lock()
 */
void snaplock::msg_unlock(snap::snap_communicator_message & message)
{
    if(!is_ready())
    {
        SNAP_LOG_ERROR("received an UNLOCK when snaplock is not ready to receive LOCK messages.");
        return;
    }

    if(is_leader() == nullptr)
    {
        // we are not a leader, we need to forward to a leader to handle
        // the message properly
        //
        forward_message_to_leader(message);
        return;
    }

    QString object_name;
    pid_t client_pid(0);
    get_parameters(message, &object_name, &client_pid, nullptr, nullptr, nullptr);

    // if the ticket still exists, send the UNLOCKED and then erase it
    //
    auto obj_ticket(f_tickets.find(object_name));
    if(obj_ticket != f_tickets.end())
    {
        QString const server_name(message.has_parameter("lock_proxy_server_name")
                                    ? message.get_parameter("lock_proxy_server_name")
                                    : message.get_sent_from_server());

        //QString const service_name(message.has_parameter("lock_proxy_service_name")
        //                            ? message.get_parameter("lock_proxy_service_name")
        //                            : message.get_sent_from_service());

        QString const entering_key(QString("%1/%2").arg(server_name).arg(client_pid));
        auto key_ticket(std::find_if(
                  obj_ticket->second.begin()
                , obj_ticket->second.end()
                , [&entering_key](auto const & t)
                {
                    return t.second->get_entering_key() == entering_key;
                }));
        if(key_ticket != obj_ticket->second.end())
        {
            // this function will send a DROPTICKET to the other leaders
            // and the UNLOCKED to the source (unless we already sent the
            // UNLOCKED which gets sent at most once.)
            //
            key_ticket->second->drop_ticket();

            obj_ticket->second.erase(key_ticket);
            if(obj_ticket->second.empty())
            {
                // we are done with this one!
                //
                f_tickets.erase(obj_ticket);
            }
        }
else SNAP_LOG_WARNING("and we could not find that key in that object's map...");
    }

    // reset the timeout with the other locks
    //
    cleanup();
}


/** \brief Remove a ticket we are done with (i.e. unlocked).
 *
 * This command drops the specified ticket (object_name).
 *
 * \param[in] message  The entering message.
 */
void snaplock::msg_lock_entering(snap::snap_communicator_message & message)
{
    QString object_name;
    time_t timeout(0);
    QString key;
    QString source;
    get_parameters(message, &object_name, nullptr, &timeout, &key, &source);

    // the server_name and client_pid never include a slash so using
    // such as separators is safe
    //
    if(timeout > time(nullptr))  // lock still in the future?
    {
        if(is_ready())              // still have leaders?
        {
            // the entering is just a flag (i.e. entering[i] = true)
            // in our case the existance of a ticket is enough to know
            // that we entered
            //
            bool allocate(true);
            auto const obj_ticket(f_entering_tickets.find(object_name));
            if(obj_ticket != f_entering_tickets.end())
            {
                auto const key_ticket(obj_ticket->second.find(key));
                allocate = key_ticket == obj_ticket->second.end();
            }
            if(allocate)
            {
                // ticket does not exist, so create it now
                // (note: ticket should only exist on originator)
                //
                int32_t const duration(message.get_integer_parameter("duration"));
                if(duration < snap::snap_lock::SNAP_LOCK_MINIMUM_TIMEOUT)
                {
                    // invalid duration, minimum is 3
                    //
                    SNAP_LOG_ERROR(duration)
                                  (" is an invalid duration, the minimum accepted is ")
                                  (snap::snap_lock::SNAP_LOCK_MINIMUM_TIMEOUT)
                                  (".");

                    snap::snap_communicator_message lock_failed_message;
                    lock_failed_message.set_command("LOCKFAILED");
                    lock_failed_message.reply_to(message);
                    lock_failed_message.add_parameter("object_name", object_name);
                    lock_failed_message.add_parameter("key", key);
                    lock_failed_message.add_parameter("error", "invalid");
                    send_message(lock_failed_message);

                    return;
                }

                int32_t unlock_duration(snap::snap_lock::SNAP_UNLOCK_USES_LOCK_TIMEOUT);
                if(message.has_parameter("unlock_duration"))
                {
                    unlock_duration = message.get_integer_parameter("unlock_duration");
                    if(unlock_duration != snap::snap_lock::SNAP_UNLOCK_USES_LOCK_TIMEOUT
                    && unlock_duration < snap::snap_lock::SNAP_UNLOCK_MINIMUM_TIMEOUT)
                    {
                        // invalid duration, minimum is 60
                        //
                        SNAP_LOG_ERROR(duration)
                                      (" is an invalid unlock duration, the minimum accepted is ")
                                      (snap::snap_lock::SNAP_UNLOCK_MINIMUM_TIMEOUT)
                                      (".");

                        snap::snap_communicator_message lock_failed_message;
                        lock_failed_message.set_command("LOCKFAILED");
                        lock_failed_message.reply_to(message);
                        lock_failed_message.add_parameter("object_name", object_name);
                        lock_failed_message.add_parameter("key", key);
                        lock_failed_message.add_parameter("error", "invalid");
                        send_message(lock_failed_message);

                        return;
                    }
                }

                // we have to know where this message comes from
                //
                snap::snap_string_list const source_segments(source.split("/"));
                if(source_segments.size() != 2)
                {
                    SNAP_LOG_ERROR("Invalid number of parameters in source (found ")
                                  (source_segments.size())
                                  (", expected 2.)");

                    snap::snap_communicator_message lock_failed_message;
                    lock_failed_message.set_command("LOCKFAILED");
                    lock_failed_message.reply_to(message);
                    lock_failed_message.add_parameter("object_name", object_name);
                    lock_failed_message.add_parameter("key", key);
                    lock_failed_message.add_parameter("error", "invalid");
                    send_message(lock_failed_message);

                    return;
                }

                snaplock_ticket::pointer_t ticket(std::make_shared<snaplock_ticket>(
                                          this
                                        , f_messenger
                                        , object_name
                                        , key
                                        , timeout
                                        , duration
                                        , source_segments[0]
                                        , source_segments[1]));

                f_entering_tickets[object_name][key] = ticket;

                // finish up on ticket initialization
                //
                ticket->set_owner(message.get_sent_from_server());
                ticket->set_unlock_duration(unlock_duration);
                ticket->set_serial(message.get_integer_parameter("serial"));
            }

            snap::snap_communicator_message reply;
            reply.set_command("LOCKENTERED");
            reply.reply_to(message);
            reply.add_parameter("object_name", object_name);
            reply.add_parameter("key", key);
            send_message(reply);
        }
        else
        {
            SNAP_LOG_DEBUG("received LOCKENTERING while we are thinking we are not ready.");
        }
    }

    cleanup();
}


/** \brief Tell all the tickets that we received a LOCKENTERED message.
 *
 * This function calls all the tickets entered() function which
 * process the LOCKENTERED message.
 *
 * We pass the key and "our ticket" number along so it can actually
 * create the ticket if required.
 *
 * \param[in] message  The LOCKENTERED message.
 */
void snaplock::msg_lock_entered(snap::snap_communicator_message & message)
{
    QString object_name;
    QString key;
    get_parameters(message, &object_name, nullptr, nullptr, &key, nullptr);

    auto const obj_entering_ticket(f_entering_tickets.find(object_name));
    if(obj_entering_ticket != f_entering_tickets.end())
    {
        auto const key_entering_ticket(obj_entering_ticket->second.find(key));
        if(key_entering_ticket != obj_entering_ticket->second.end())
        {
            key_entering_ticket->second->entered();
        }
    }
}


/** \brief Remove a ticket we are done with (i.e. unlocked).
 *
 * This command drops the specified ticket (object_name).
 *
 * \param[in] message  The entering message.
 */
void snaplock::msg_lock_exiting(snap::snap_communicator_message & message)
{
    QString object_name;
    QString key;
    get_parameters(message, &object_name, nullptr, nullptr, &key, nullptr);

    // when exiting we just remove the entry with that key
    //
    auto const obj_entering(f_entering_tickets.find(object_name));
    if(obj_entering != f_entering_tickets.end())
    {
        auto const key_entering(obj_entering->second.find(key));
        if(key_entering != obj_entering->second.end())
        {
            obj_entering->second.erase(key_entering);

            // we also want to remove it from the ticket f_entering
            // map if it is there (older ones are there!)
            //
            bool run_activation(false);
            auto const obj_ticket(f_tickets.find(object_name));
            if(obj_ticket != f_tickets.end())
            {
                for(auto const & key_ticket : obj_ticket->second)
                {
                    key_ticket.second->remove_entering(key);
                    run_activation = true;
                }
            }
            if(run_activation)
            {
                // try to activate the lock right now since it could
                // very well be the only ticket and that is exactly
                // when it is viewed as active!
                //
                // Note: this is from my old version, if I am correct
                //       it cannot happen anymore because (1) this is
                //       not the owner so the activation would not
                //       take anyway and (2) the ticket is not going
                //       to be marked as being ready at this point
                //       (that happens later)
                //
                //       XXX we probably should remove this statement
                //           and the run_activation flag which would
                //           then be useless
                //
                activate_first_lock(object_name);
            }

            if(obj_entering->second.empty())
            {
                f_entering_tickets.erase(obj_entering);
            }
        }
    }

    // the list of tickets is not unlikely changed so we need to make
    // a call to cleanup to make sure the timer is reset appropriately
    //
    cleanup();
}


/** \brief One of the snaplock processes asked for a ticket to be dropped.
 *
 * This function searches for the specified ticket and removes it from
 * this snaplock.
 *
 * If the specified ticket does not exist, nothing happens.
 *
 * \warning
 * The DROPTICKET event receives either the ticket key (if available)
 * or the entering key (when the ticket key was not yet available.)
 * Note that the ticket key should always exists by the time a DROPTICKET
 * happens, but just in case this allows the drop a ticket at any time.
 *
 * \param[in] message  The message just received.
 */
void snaplock::msg_drop_ticket(snap::snap_communicator_message & message)
{
    QString object_name;
    QString key;
    get_parameters(message, &object_name, nullptr, nullptr, &key, nullptr);

    snap::snap_string_list const segments(key.split('/'));

    // drop the regular ticket
    //
    // if we have only 2 segments, then there is no corresponding ticket
    // since tickets are added only once we have a ticket_id
    //
    QString entering_key;
    if(segments.size() == 3)
    {
        auto obj_ticket(f_tickets.find(object_name));
        if(obj_ticket != f_tickets.end())
        {
            auto key_ticket(obj_ticket->second.find(key));
            if(key_ticket != obj_ticket->second.end())
            {
                obj_ticket->second.erase(key_ticket);
            }

            if(obj_ticket->second.empty())
            {
                f_tickets.erase(obj_ticket);
            }

            // one ticket was erased, another may be first now
            //
            activate_first_lock(object_name);
        }

        // we received the ticket_id in the message, so
        // we have to regenerate the entering_key without
        // the ticket_id (which is the first element)
        //
        entering_key = QString("%1/%2").arg(segments[1]).arg(segments[2]);
    }
    else
    {
        // we received the entering_key in the message, use as is
        //
        entering_key = key;
    }

    // drop the entering ticket
    //
    auto obj_entering_ticket(f_entering_tickets.find(object_name));
    if(obj_entering_ticket != f_entering_tickets.end())
    {
        auto key_entering_ticket(obj_entering_ticket->second.find(entering_key));
        if(key_entering_ticket != obj_entering_ticket->second.end())
        {
            obj_entering_ticket->second.erase(key_entering_ticket);
        }

        if(obj_entering_ticket->second.empty())
        {
            f_entering_tickets.erase(obj_entering_ticket);
        }
    }

    // the list of tickets is not unlikely changed so we need to make
    // a call to cleanup to make sure the timer is reset appropriately
    //
    cleanup();
}


/** \brief Search for the largest ticket.
 *
 * This function searches the list of tickets for the largest one
 * and returns that number.
 *
 * \return The largest ticket number that currently exist in the list
 *         of tickets.
 */
void snaplock::msg_get_max_ticket(snap::snap_communicator_message & message)
{
    QString object_name;
    QString key;
    get_parameters(message, &object_name, nullptr, nullptr, &key, nullptr);

    // remove any f_tickets that timed out by now because these should
    // not be taken in account in the max. computation
    //
    cleanup();

    snaplock_ticket::ticket_id_t last_ticket(get_last_ticket(object_name));

    snap::snap_communicator_message reply;
    reply.set_command("MAXTICKET");
    reply.reply_to(message);
    reply.add_parameter("object_name", object_name);
    reply.add_parameter("key", key);
    reply.add_parameter("ticket_id", last_ticket);
    send_message(reply);
}


/** \brief Search for the largest ticket.
 *
 * This function searches the list of tickets for the largest one
 * and records that number.
 *
 * If a quorum is reached when adding this ticket, then an ADDTICKET reply
 * is sent back to the sender.
 *
 * \param[in] message  The MAXTICKET message being handled.
 */
void snaplock::msg_max_ticket(snap::snap_communicator_message & message)
{
    QString object_name;
    QString key;
    get_parameters(message, &object_name, nullptr, nullptr, &key, nullptr);

    // the MAXTICKET is an answer that has to go in a still un-added ticket
    //
    auto const obj_entering_ticket(f_entering_tickets.find(object_name));
    if(obj_entering_ticket != f_entering_tickets.end())
    {
        auto const key_entering_ticket(obj_entering_ticket->second.find(key));
        if(key_entering_ticket != obj_entering_ticket->second.end())
        {
            key_entering_ticket->second->max_ticket(message.get_integer_parameter("ticket_id"));
        }
    }
}


/** \brief Add a ticket from another snaplock.
 *
 * Tickets get duplicated on the snaplock leaders.
 *
 * \note
 * Although we only need a QUORUM number of nodes to receive a copy of
 * the data, the data still get broadcast to all the snaplock leaders.
 * After this message arrives any one of the snaplock process can
 * handle the unlock if the UNLOCK message gets sent to another process
 * instead of the one which first created the ticket. This is the point
 * of the implementation since we want to be fault tolerant (as in if one
 * of the leaders goes down, the locking mechanism still works.)
 *
 * \param[in] message  The ADDTICKET message being handled.
 */
void snaplock::msg_add_ticket(snap::snap_communicator_message & message)
{
    QString object_name;
    QString key;
    time_t timeout;
    get_parameters(message, &object_name, nullptr, &timeout, &key, nullptr);

#ifdef _DEBUG
    {
        auto const obj_ticket(f_tickets.find(object_name));
        if(obj_ticket != f_tickets.end())
        {
            auto const key_ticket(obj_ticket->second.find(key));
            if(key_ticket != obj_ticket->second.end())
            {
                // this ticket exists on this system
                //
                throw std::logic_error("snaplock::add_ticket() ticket already exists");
            }
        }
    }
#endif

    // the client_pid parameter is part of the key (3rd segment)
    //
    snap::snap_string_list const segments(key.split('/'));
    if(segments.size() != 3)
    {
        SNAP_LOG_ERROR("Expected exactly 3 segments in \"")
                      (key)
                      ("\" to add a ticket.");

        snap::snap_communicator_message lock_failed_message;
        lock_failed_message.set_command("LOCKFAILED");
        lock_failed_message.reply_to(message);
        lock_failed_message.add_parameter("object_name", object_name);
        lock_failed_message.add_parameter("key", key);
        lock_failed_message.add_parameter("error", "invalid");
        send_message(lock_failed_message);

        return;
    }

    bool ok(false);
    uint32_t const number(segments[0].toUInt(&ok, 16));
    if(!ok)
    {
        SNAP_LOG_ERROR("somehow ticket number \"")
                      (segments[0])
                      ("\" is not a valid hexadecimal number");

        snap::snap_communicator_message lock_failed_message;
        lock_failed_message.set_command("LOCKFAILED");
        lock_failed_message.reply_to(message);
        lock_failed_message.add_parameter("object_name", object_name);
        lock_failed_message.add_parameter("key", key);
        lock_failed_message.add_parameter("error", "invalid");
        send_message(lock_failed_message);

        return;
    }

    // by now all existing snaplock instances should already have
    // an entering ticket for that one ticket
    //
    auto const obj_entering_ticket(f_entering_tickets.find(object_name));
    if(obj_entering_ticket == f_entering_tickets.end())
    {
        SNAP_LOG_ERROR("Expected entering ticket object for \"")
                      (object_name)
                      ("\" not found when adding a ticket.");

        snap::snap_communicator_message lock_failed_message;
        lock_failed_message.set_command("LOCKFAILED");
        lock_failed_message.reply_to(message);
        lock_failed_message.add_parameter("object_name", object_name);
        lock_failed_message.add_parameter("key", key);
        lock_failed_message.add_parameter("error", "invalid");
        send_message(lock_failed_message);

        return;
    }

    // the key we need to search is not the new ticket key but the
    // entering key, build it from the segments
    //
    QString const entering_key(QString("%1/%2").arg(segments[1]).arg(segments[2]));
    auto const key_entering_ticket(obj_entering_ticket->second.find(entering_key));
    if(key_entering_ticket == obj_entering_ticket->second.end())
    {
        SNAP_LOG_ERROR("Expected entering ticket key for \"")
                      (object_name)
                      ("\" not found when adding a ticket.");

        snap::snap_communicator_message lock_failed_message;
        lock_failed_message.set_command("LOCKFAILED");
        lock_failed_message.reply_to(message);
        lock_failed_message.add_parameter("object_name", object_name);
        lock_failed_message.add_parameter("key", key);
        lock_failed_message.add_parameter("error", "invalid");
        send_message(lock_failed_message);

        return;
    }

    // make it an official ticket now
    //
    // this should happen on all snaplock other than the one that
    // first received the LOCK message
    //
    set_ticket(object_name, key, key_entering_ticket->second);

    // WARNING: the set_ticket_number() function has the same side
    //          effects as the add_ticket() function without the
    //          send_message() call
    //
    f_tickets[object_name][key]->set_ticket_number(number);

    snap::snap_communicator_message ticket_added_message;
    ticket_added_message.set_command("TICKETADDED");
    ticket_added_message.reply_to(message);
    ticket_added_message.add_parameter("object_name", object_name);
    ticket_added_message.add_parameter("key", key);
    send_message(ticket_added_message);
}


/** \brief Acknowledgement that the ticket was properly added.
 *
 * This function gets called whenever the ticket was added on another
 * leader.
 *
 * \param[in] message  The TICKETADDED message being handled.
 */
void snaplock::msg_ticket_added(snap::snap_communicator_message & message)
{
    QString object_name;
    QString key;
    get_parameters(message, &object_name, nullptr, nullptr, &key, nullptr);

    auto const obj_ticket(f_tickets.find(object_name));
    if(obj_ticket != f_tickets.end())
    {
        auto const key_ticket(obj_ticket->second.find(key));
        if(key_ticket != obj_ticket->second.end())
        {
            // this ticket exists on this system
            //
            auto const obj_entering_ticket(f_entering_tickets.find(object_name));
            if(obj_entering_ticket == f_entering_tickets.end())
            {
                // this happens all the time because the entering ticket
                // gets removed on the first TICKETADDED we receive so
                // on the second one we get here...
                //
                SNAP_LOG_TRACE("called with object \"")
                              (object_name)
                              ("\" not present in f_entering_ticket (key: \"")
                              (key)
                              ("\".)");
                return;
            }
            key_ticket->second->ticket_added(obj_entering_ticket->second);
        }
        else
        {
            SNAP_LOG_DEBUG("found object \"")
                          (object_name)
                          ("\" but could not find a ticket with key \"")
                          (key)
                          ("\"...");
        }
    }
    else
    {
        SNAP_LOG_DEBUG("object \"")
                      (object_name)
                      ("\" not found.");
    }
}


/** \brief Let other leaders know that the ticket is ready.
 *
 * This message is received when the owner of a ticket marks a
 * ticket as ready. This means the ticket is available for locking.
 *
 * \param[in] message  The TICKETREADY message.
 */
void snaplock::msg_ticket_ready(snap::snap_communicator_message & message)
{
    QString object_name;
    QString key;
    get_parameters(message, &object_name, nullptr, nullptr, &key, nullptr);

    auto obj_ticket(f_tickets.find(object_name));
    if(obj_ticket != f_tickets.end())
    {
        auto key_ticket(obj_ticket->second.find(key));
        if(key_ticket != obj_ticket->second.end())
        {
            // we can mark this ticket as activated
            //
            key_ticket->second->set_ready();
        }
    }
}


/** \brief Acknowledge the ACTIVATELOCK with what we think is our first lock.
 *
 * This function replies to an ACTIVATELOCK request with what we think is
 * the first lock for the spcified object.
 *
 * Right now, we disregard the specified key. There is nothing we can really
 * do with it here.
 *
 * If we do not have a ticket for the specified object (something that could
 * happen if the ticket just timed out) then we still have to reply, only
 * we let the other leader know that we have no clue what he is talking about.
 *
 * \param[in] message  The message being processed.
 */
void snaplock::msg_activate_lock(snap::snap_communicator_message & message)
{
    QString object_name;
    QString key;
    get_parameters(message, &object_name, nullptr, nullptr, &key, nullptr);

    QString first_key("no-key");

    auto ticket(find_first_lock(object_name));
    if(ticket != nullptr)
    {
        // found it!
        //
        first_key = ticket->get_ticket_key();

        if(key == first_key)
        {
            // we can mark this ticket as activated
            //
            ticket->lock_activated();
        }
    }

    // always reply, if we could not find the key, then we returned 'no-key'
    // as the key parameter
    //
    snap::snap_communicator_message lock_activated_message;
    lock_activated_message.set_command("LOCKACTIVATED");
    lock_activated_message.reply_to(message);
    lock_activated_message.add_parameter("object_name", object_name);
    lock_activated_message.add_parameter("key", key);
    lock_activated_message.add_parameter("other_key", first_key);
    send_message(lock_activated_message);

    // the list of tickets is not unlikely changed so we need to make
    // a call to cleanup to make sure the timer is reset appropriately
    //
    cleanup();
}


/** \brief Acknowledgement of the lock to activate.
 *
 * This function is an acknowledgement that the lock can now be
 * activated. This is true only if the 'key' and 'other_key'
 * are a match, though.
 *
 * \param[in] message  The message to be managed.
 */
void snaplock::msg_lock_activated(snap::snap_communicator_message & message)
{
    QString object_name;
    QString key;
    get_parameters(message, &object_name, nullptr, nullptr, &key, nullptr);

    QString const & other_key(message.get_parameter("other_key"));
    if(other_key == key)
    {
        auto obj_ticket(f_tickets.find(object_name));
        if(obj_ticket != f_tickets.end())
        {
            auto key_ticket(obj_ticket->second.find(key));
            if(key_ticket != obj_ticket->second.end())
            {
                // that key is still here!
                // time to activate
                //
                key_ticket->second->lock_activated();
            }
        }
    }
}


/** \brief Acknowledgement a lock failure.
 *
 * This function handles the LOCKFAILED event that another leader may send
 * to us. In that case we have to stop the process.
 *
 * LOCKFAILED can happen mainly because of tainted data so we should never
 * get here within a leader. However, with time we may add a few errors
 * which could happen for other reasons than just tainted data.
 *
 * When this function finds an entering ticket or a plain ticket to remove
 * according to the object name and key found in the LOCKFAILED message,
 * it forwards the LOCKFAILED message to the server and service found in
 * the ticket.
 *
 * \todo
 * This function destroys a ticket even if it is already considered locked.
 * Make double sure that this is okay with a LOCKFAILED sent to the client.
 *
 * \warning
 * Although this event should not occur, it is problematic since anyone
 * can send a LOCKFAILED message here and as a side effect destroy a
 * perfectly valid ticket.
 *
 * \param[in] message  The message to be managed.
 */
void snaplock::msg_lock_failed(snap::snap_communicator_message & message)
{
    QString object_name;
    QString key;
    get_parameters(message, &object_name, nullptr, nullptr, &key, nullptr);

    QString forward_server;
    QString forward_service;

    // remove f_entering_tickets entries if we find matches there
    //
    auto obj_entering(f_entering_tickets.find(object_name));
    if(obj_entering != f_entering_tickets.end())
    {
        auto key_entering(obj_entering->second.find(key));
        if(key_entering != obj_entering->second.end())
        {
            forward_server = key_entering->second->get_server_name();
            forward_service = key_entering->second->get_service_name();

            obj_entering->second.erase(key_entering);
        }

        if(obj_entering->second.empty())
        {
            obj_entering = f_entering_tickets.erase(obj_entering);
        }
        else
        {
            ++obj_entering;
        }
    }

    // remove any f_tickets entries if we find matches there
    //
    auto obj_ticket(f_tickets.find(object_name));
    if(obj_ticket != f_tickets.end())
    {
        bool try_activate(false);
        auto key_ticket(obj_ticket->second.find(key));
        if(key_ticket == obj_ticket->second.end())
        {
            key_ticket = std::find_if(
                      obj_ticket->second.begin()
                    , obj_ticket->second.end()
                    , [&key](auto const & t)
                    {
                        return t.second->get_entering_key() == key;
                    });
        }
        if(key_ticket != obj_ticket->second.end())
        {
            // Note: if we already found it in the f_entering_tickets then
            //       the server and service names are going to be exactly
            //       the same so there is no need to test that here
            //
            forward_server = key_ticket->second->get_server_name();
            forward_service = key_ticket->second->get_service_name();

            obj_ticket->second.erase(key_ticket);
            try_activate = true;
        }

        if(obj_ticket->second.empty())
        {
            obj_ticket = f_tickets.erase(obj_ticket);
        }
        else
        {
            if(try_activate)
            {
                // something was erased, a new ticket may be first
                //
                activate_first_lock(obj_ticket->first);
            }

            ++obj_ticket;
        }
    }

    if(!forward_server.isEmpty()
    && !forward_service.isEmpty())
    {
        // we deleted an entry, forward the message to the service
        // that requested that lock
        //
        message.set_server(forward_server);
        message.set_service(forward_service);
        send_message(message);
    }

    // the list of tickets is not unlikely changed so we need to make
    // a call to cleanup to make sure the timer is reset appropriately
    //
    cleanup();
}


/** \brief Make sure the very first ticket is marked as LOCKED.
 *
 * This function is called whenever the f_tickets map changes
 * (more specifically, one of its children) to make sure
 * that the first ticket is clearly marked as being locked.
 * Most of the time this happens when we add and when we remove
 * tickets.
 *
 * Note that the function may be called many times even though the
 * first ticket does not actually change. Generally this is fine 
 * although each time it sends an ACTIVATELOCK message so we want
 * to limit the number of calls to make sure we do not send too
 * many possibly confusing messages.
 *
 * \note
 * We need the ACTIVATELOCK and LOCKACTIVATED messages to make sure
 * that we only activate the very first lock which we cannot be sure
 * of on our own because all the previous messages are using the
 * QUORUM as expected and thus our table of locks may not be complete
 * at any one time.
 *
 * \param[in] object_name  The name of the object which very
 *                         first ticket may have changed.
 */
void snaplock::activate_first_lock(QString const & object_name)
{
    auto ticket(find_first_lock(object_name));

    if(ticket != nullptr)
    {
        // there is what we think is the first ticket
        // that should be actived now; we need to share
        // with the other 2 leaders to make sure of that
        //
        ticket->activate_lock();
    }
}


snaplock_ticket::pointer_t snaplock::find_first_lock(QString const & object_name)
{
    snaplock_ticket::pointer_t first_ticket;
    auto const obj_ticket(f_tickets.find(object_name));

    if(obj_ticket != f_tickets.end())
    {
        // loop through making sure that we activate a ticket only
        // if the obtention date was not already reached; if that
        // date was reached before we had the time to activate the
        // lock, then the client should have abandonned the lock
        // request anyway...
        //
        // (this is already done in the cleanup(), but a couple of
        // other functions may call the activate_first_lock()
        // function!)
        //
        for(auto key_ticket(obj_ticket->second.begin()); key_ticket != obj_ticket->second.end(); )
        {
            if(key_ticket->second->timed_out())
            {
                // that ticket timed out, send an UNLOCK or LOCKFAILED
                // message and get rid of it
                //
                key_ticket->second->lock_failed();
                if(key_ticket->second->timed_out())
                {
                    // still timed out, remove it
                    //
                    key_ticket = obj_ticket->second.erase(key_ticket);
                }
            }
            else
            {
                if(first_ticket == nullptr)
                {
                    first_ticket = key_ticket->second;
                }
                ++key_ticket;
            }
        }

        if(obj_ticket->second.empty())
        {
            // it is empty now, get rid of that set of tickets
            //
            f_tickets.erase(obj_ticket);
        }
    }

    return first_ticket;
}


/** \brief Synchronize leaders.
 *
 * This function sends various events to the other two leaders in order
 * to get them to synchronize the tickets this snaplock currently holds.
 *
 * Only leaders make use of this function.
 *
 * Synchronization is necessary whenever a leader dies and another gets
 * elected as a replacement. The replacement would have no idea of the
 * old tickets. This function makes sure that such doesn't occur.
 *
 * \note
 * Our test checks the validity when ONE leader is lost. If TWO of the
 * leaders are lost at once, the algorithm may not be 100% reliable.
 * Especially, the remaining leader may not have all the necessary
 * information to restore all the tickets as they were expected to be.
 *
 * \warning
 * A ticket that just arrived to a leader and was not yet forwarded to
 * the others with the LOCKENTERING message is going to be lost no
 * matter what.
 */
void snaplock::synchronize_leaders()
{
    // there is nothing to do if we are by ourselves because we cannot
    // gain any type of concensus unless we are expected to be the only
    // one in which case there is no synchronization requirements anyway
    //
    if(f_leaders.size() <= 1)
    {
        return;
    }

    // only leaders can synchronize each others
    // (other snaplocks do not have any tickets to synchronize)
    //
    if(!is_leader())
    {
        return;
    }

    // determine whether we are leader #0 or not, if zero, then we
    // call msg_lock() directly, otherwise we do a send_message()
    //
    bool const leader0(f_leaders[0]->get_id() == f_my_id);

    // a vector of messages for which we have to call msg_lock()
    //
    snap::snap_communicator_message::vector_t local_locks;

    // if entering a ticket is definitely not locked, although it
    // could be ready (one step away from being locked!) we still
    // restart the whole process with the new leaders if such
    // exist
    //
    // Note: of course we restart the process only if the owner
    //       was that one leader that disappeared, not if the
    //       ticket is owned by a remaining leader
    //
    for(auto obj_entering(f_entering_tickets.begin()); obj_entering != f_entering_tickets.end(); ++obj_entering)
    {
        for(auto key_entering(obj_entering->second.begin()); key_entering != obj_entering->second.end(); )
        {
            QString const owner_name(key_entering->second->get_owner());
            auto key_leader(std::find_if(
                      f_leaders.begin()
                    , f_leaders.end()
                    , [&owner_name](auto const & l)
                    {
                        return l->get_name() == owner_name;
                    }));
            if(key_leader == f_leaders.end())
            {
                // give new ownership to leader[0]
                //
                snap::snap_communicator_message lock_message;
                lock_message.set_command("LOCK");
                lock_message.set_server(f_leaders[0]->get_name());
                lock_message.set_service("snaplock");
                lock_message.set_sent_from_server(key_entering->second->get_server_name());
                lock_message.set_sent_from_service(key_entering->second->get_service_name());
                lock_message.add_parameter("object_name", key_entering->second->get_object_name());
                lock_message.add_parameter("pid", key_entering->second->get_client_pid());
                lock_message.add_parameter("timeout", key_entering->second->get_obtention_timeout());
                lock_message.add_parameter("duration", key_entering->second->get_lock_duration());
                lock_message.add_parameter("unlock_duration", key_entering->second->get_unlock_duration());
                if(leader0)
                {
                    // we are leader #0 so directly call msg_lock()
                    //
                    // first we remove the entry otherwise we get a duplicate
                    // error since we try to readd the same ticket
                    //
                    key_entering = obj_entering->second.erase(key_entering);
                    local_locks.push_back(lock_message);
                }
                else
                {
                    // we are not leader #0, so send the message to it
                    //
                    ++key_entering;
                    lock_message.add_parameter("serial", key_entering->second->get_serial());
                    send_message(lock_message);
                }
            }
            else
            {
                ++key_entering;
            }
        }
    }

    // a ticket may still be unlocked in which case we want to
    // restart the lock process as if still entering
    //
    // if locked, a ticket is assigned leader0 as its new owner so
    // further work on that ticket works as expected
    //
    QString serialized;
    for(auto obj_ticket(f_tickets.begin()); obj_ticket != f_tickets.end(); ++obj_ticket)
    {
        for(auto key_ticket(obj_ticket->second.begin()); key_ticket != obj_ticket->second.end(); )
        {
            QString const owner_name(key_ticket->second->get_owner());
            auto key_leader(std::find_if(
                      f_leaders.begin()
                    , f_leaders.end()
                    , [&owner_name](auto const & l)
                    {
                        return l->get_name() == owner_name;
                    }));
            if(key_ticket->second->is_locked())
            {
                // if ticket was locked by the leader that disappeared, we
                // transfer ownership to the new leader #0
                //
                if(key_leader == f_leaders.end())
                {
                    key_ticket->second->set_owner(f_leaders[0]->get_name());
                }

                // and send that ticket to the other leaders to make sure
                // they all agree on its current state
                //
                serialized += key_ticket->second->serialize();
                serialized += QChar('\n');

                ++key_ticket;
            }
            else
            {
                // it was not locked yet, restart the LOCK process from
                // the very beginning
                //
                if(key_leader == f_leaders.end())
                {
                    // give new ownership to leader[0]
                    //
                    snap::snap_communicator_message lock_message;
                    lock_message.set_command("LOCK");
                    lock_message.set_server(f_leaders[0]->get_name());
                    lock_message.set_service("snaplock");
                    lock_message.set_sent_from_server(key_ticket->second->get_server_name());
                    lock_message.set_sent_from_service(key_ticket->second->get_service_name());
                    lock_message.add_parameter("object_name", key_ticket->second->get_object_name());
                    lock_message.add_parameter("pid", key_ticket->second->get_client_pid());
                    lock_message.add_parameter("timeout", key_ticket->second->get_obtention_timeout());
                    lock_message.add_parameter("duration", key_ticket->second->get_lock_duration());
                    lock_message.add_parameter("unlock_duration", key_ticket->second->get_unlock_duration());
                    if(leader0)
                    {
                        // we are leader #0 so directly call msg_lock()
                        //
                        key_ticket = obj_ticket->second.erase(key_ticket);
                        local_locks.push_back(lock_message);
                    }
                    else
                    {
                        // we are not leader #0, so send the message to it
                        //
                        ++key_ticket;
                        lock_message.add_parameter("serial", key_ticket->second->get_serial());
                        send_message(lock_message);
                    }
                }
                else
                {
                    ++key_ticket;
                }
            }
        }
    }

    // we send those after the loops above because the msg_lock() is
    // not unlikely to change the f_entering_tickets map and looping
    // through it when another function is going to modify it is not
    // wise
    //
    for(auto lm : local_locks)
    {
        msg_lock(lm);
    }

    // send LOCKTICkETS if there is serialized ticket data
    //
    if(!serialized.isEmpty())
    {
        snap::snap_communicator_message lock_tickets_message;
        lock_tickets_message.set_command("LOCKTICKETS");
        lock_tickets_message.set_service("snaplock");
        lock_tickets_message.add_parameter("tickets", serialized);

        auto const la(get_leader_a());
        if(la != nullptr)
        {
            lock_tickets_message.set_server(la->get_name());
            send_message(lock_tickets_message);

            auto const lb(get_leader_b());
            if(lb != nullptr)
            {
                lock_tickets_message.set_server(lb->get_name());
                send_message(lock_tickets_message);
            }
        }
    }
}


/** \brief Forward a user message to a leader.
 *
 * The user may send a LOCK or an UNLOCK command to the snaplock system.
 * Those messages need to be forwarded to a leader to work as expected.
 * If we are not a leader, then we need to call this function to
 * forward the message.
 *
 * Note that we do not make a copy of the message because we do not expect
 * it to be used any further after this call so we may as well update that
 * message. It should not be destructive at all anyway.
 *
 * \param[in,out] message  The message being forwarded to a leader.
 */
void snaplock::forward_message_to_leader(snap::snap_communicator_message & message)
{
    // we are not a leader, we work as a proxy by forwarding the
    // message to a leader, we add our trail so the LOCKED and
    // other messages can be proxied back
    //
    // Note: using the get_sent_from_server() means that we may not
    //       even see the return message, it may be proxied to another
    //       server directly or through another route
    //
    message.set_service("snaplock");
    message.add_parameter("lock_proxy_server_name", message.get_sent_from_server());
    message.add_parameter("lock_proxy_service_name", message.get_sent_from_service());

    f_next_leader = (f_next_leader + 1) % f_leaders.size();
    message.set_server(f_leaders[f_next_leader]->get_name());

    send_message(message);
}


/** \brief Clean timed out entries if any.
 *
 * This function goes through the list of tickets and entering
 * entries and removes any one of them that timed out. This is
 * important if a process dies and does not properly remove
 * its locks.
 */
void snaplock::cleanup()
{
    time_t next_timeout(std::numeric_limits<time_t>::max());

    // when we receive LOCK requests before we have leaders elected, they
    // get added to our cache, so do some cache clean up when not empty
    //
    for(auto c(f_message_cache.begin()); c != f_message_cache.end(); )
    {
        if(c->f_timeout <= time(nullptr))
        {
            QString object_name;
            pid_t client_pid(0);
            time_t timeout(0);
            get_parameters(c->f_message, &object_name, &client_pid, &timeout, nullptr, nullptr);

            SNAP_LOG_WARNING("Lock on \"")(object_name)("\" / \"")(client_pid)("\" timed out before leaders were known.");

            QString const server_name(c->f_message.has_parameter("lock_proxy_server_name")
                                        ? c->f_message.get_parameter("lock_proxy_server_name")
                                        : c->f_message.get_sent_from_server());
            QString const entering_key(QString("%1/%2").arg(server_name).arg(client_pid));

            snap::snap_communicator_message lock_failed_message;
            lock_failed_message.set_command("LOCKFAILED");
            lock_failed_message.reply_to(c->f_message);
            lock_failed_message.add_parameter("object_name", object_name);
            lock_failed_message.add_parameter("key", entering_key);
            lock_failed_message.add_parameter("error", "timedout");
            send_message(lock_failed_message);

            c = f_message_cache.erase(c);
        }
        else
        {
            if(c->f_timeout < next_timeout)
            {
                next_timeout = c->f_timeout;
            }
            ++c;
        }
    }

    // remove any f_tickets that timed out
    //
    for(auto obj_ticket(f_tickets.begin()); obj_ticket != f_tickets.end(); )
    {
        bool try_activate(false);
        for(auto key_ticket(obj_ticket->second.begin()); key_ticket != obj_ticket->second.end(); )
        {
            if(key_ticket->second->timed_out())
            {
                key_ticket->second->lock_failed();
                if(key_ticket->second->timed_out())
                {
                    // still timed out, remove it
                    //
                    key_ticket = obj_ticket->second.erase(key_ticket);
                    try_activate = true;
                }
            }
            else
            {
                if(key_ticket->second->get_current_timeout() < next_timeout)
                {
                    next_timeout = key_ticket->second->get_current_timeout();
                }
                ++key_ticket;
            }
        }

        if(obj_ticket->second.empty())
        {
            obj_ticket = f_tickets.erase(obj_ticket);
        }
        else
        {
            if(try_activate)
            {
                // something was erased, a new ticket may be first
                //
                activate_first_lock(obj_ticket->first);
            }

            ++obj_ticket;
        }
    }

    // remove any f_entering_tickets that timed out
    //
    for(auto obj_entering(f_entering_tickets.begin()); obj_entering != f_entering_tickets.end(); )
    {
        for(auto key_entering(obj_entering->second.begin()); key_entering != obj_entering->second.end(); )
        {
            if(key_entering->second->timed_out())
            {
                key_entering->second->lock_failed();
                if(key_entering->second->timed_out())
                {
                    // still timed out, remove it
                    //
                    key_entering = obj_entering->second.erase(key_entering);
                }
            }
            else
            {
                if(key_entering->second->get_current_timeout() < next_timeout)
                {
                    next_timeout = key_entering->second->get_current_timeout();
                }
                ++key_entering;
            }
        }

        if(obj_entering->second.empty())
        {
            obj_entering = f_entering_tickets.erase(obj_entering);
        }
        else
        {
            ++obj_entering;
        }
    }

    // got a new timeout?
    //
    if(next_timeout != std::numeric_limits<time_t>::max())
    {
        // out timeout is in seconds, snap_communicator expects
        // micro seconds so multiply by 1 million
        //
        // we add +1 to the second to avoid looping like crazy
        // if we timeout just around the "wrong" time
        //
        f_timer->set_timeout_date((next_timeout + 1) * 1000000LL);
    }
    else
    {
        f_timer->set_timeout_date(-1);
    }
}


/** \brief Determine the last ticket defined in this snaplock.
 *
 * This function loops through the existing tickets and returns the
 * largest ticket number it finds.
 *
 * Note that the number returned is the last ticket. At some point
 * the algoritym need to add one to it before assigning the number to
 * a new ticket.
 *
 * If no ticket were defined for \p object_name or we are dealing with
 * that object very first ticket, then the function returns NO_TICKET
 * (which is 0.)
 *
 * \param[in] object_name  The name of the object for which the last ticket
 *                         number is being sought.
 *
 * \return The last ticket number or NO_TICKET.
 */
snaplock_ticket::ticket_id_t snaplock::get_last_ticket(QString const & object_name)
{
    snaplock_ticket::ticket_id_t last_ticket(snaplock_ticket::NO_TICKET);

    // Note: There is no need to check the f_entering_tickets list
    //       since that one does not yet have any ticket number assigned
    //       and thus the maximum there would return 0 every time
    //
    auto obj_ticket(f_tickets.find(object_name));
    if(obj_ticket != f_tickets.end())
    {
        // note:
        // the std::max_element() algorithm would require many more
        // get_ticket_number() when our loop uses one per ticket max.
        //
        for(auto key_ticket : obj_ticket->second)
        {
            snaplock_ticket::ticket_id_t const ticket_number(key_ticket.second->get_ticket_number());
            if(ticket_number > last_ticket)
            {
                last_ticket = ticket_number;
            }
        }
    }

    return last_ticket;
}


/** \brief Set the ticket.
 *
 * Once a ticket was assigned a valid identifier (see get_last_ticket())
 * it can be assigned as a ticket. This function does that. Now this is
 * an official ticket.
 *
 * \param[in] object_name  The name of the object being locked.
 * \param[in] key  The ticket key (3 segments).
 * \param[in] ticket  The ticket object being added.
 */
void snaplock::set_ticket(QString const & object_name, QString const & key, snaplock_ticket::pointer_t ticket)
{
    f_tickets[object_name][key] = ticket;
}


/** \brief Get a reference to the list of entering tickets.
 *
 * This function returns a constant reference to the list of entering
 * tickets. This is used by the snaplock_ticket::add_ticket() function
 * in order to know once all entering tickets are done so the algoritm
 * can move forward.
 *
 * \param[in] name  The name of the object being locked.
 *
 * \return A constant copy of the list of entering tickets.
 */
snaplock_ticket::key_map_t const snaplock::get_entering_tickets(QString const & object_name)
{
    auto const it(f_entering_tickets.find(object_name));
    if(it == f_entering_tickets.end())
    {
        return snaplock_ticket::key_map_t();
    }

    return it->second;
}


/** \brief Used to simulate a LOCKEXITING message.
 *
 * This function is called to simulate sending a LOCKEXITING to the
 * snaplock object from the snaplock_ticket object.
 *
 * \param[in] message  The LOCKEXITING message with proper object name and key.
 */
void snaplock::lock_exiting(snap::snap_communicator_message & message)
{
    msg_lock_exiting(message);
}



/** \brief Process a message received from Snap! Communicator.
 *
 * This function gets called whenever the Snap! Communicator sends
 * us a message while we act as a tool (opposed to being a daemon.)
 *
 * \param[in] message  The message we just received.
 */
void snaplock::tool_message(snap::snap_communicator_message const & message)
{
    SNAP_LOG_TRACE("tool received message [")(message.to_message())("] for ")(f_server_name);

    QString const command(message.get_command());

    switch(command[0].unicode())
    {
    case 'H':
        if(command == "HELP")
        {
            // Snap! Communicator is asking us about the commands that we support
            //
            snap::snap_communicator_message reply;
            reply.set_command("COMMANDS");

            // list of commands understood by service
            // (many are considered to be internal commands... users
            // should look at the LOCK and UNLOCK messages only)
            //
            reply.add_parameter("list", "CLUSTERDOWN,CLUSTERUP,HELP,QUITTING,READY,STOP,TICKETLIST,UNKNOWN");

            send_message(reply);
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
            if(f_opt.is_defined("list"))
            {
                snap::snap_communicator_message list_message;
                list_message.set_command("LISTTICKETS");
                list_message.set_service("snaplock");
                list_message.set_server(f_server_name);
                list_message.add_parameter("cache", "no");
                list_message.add_parameter("transmission_report", "failure");
                send_message(list_message);
            }
            return;
        }
        break;

    case 'S':
        if(command == "STOP")
        {
            // Someone is asking us to leave
            //
            stop(false);
            return;
        }
        break;

    case 'T':
        if(command == "TICKETLIST")
        {
            // received the answer to our LISTTICKETS request
            //
            ticket_list(message);
            stop(false);
            return;
        }
        else if(command == "TRANSMISSIONREPORT")
        {
            QString const status(message.get_parameter("status"));
            if(status == "failed")
            {
                SNAP_LOG_ERROR("the transmission of our TICKLIST message failed to travel to a snaplock service");
                stop(false);
            }
            return;
        }
        break;

    case 'U':
        if(command == "UNKNOWN")
        {
            // we sent a command that Snap! Communicator did not understand
            //
            SNAP_LOG_ERROR("we sent unknown command \"")(message.get_parameter("command"))("\" and probably did not get the expected result (2).");
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
        send_message(reply);
    }

    return;
}


/** \brief Print out the resulting list of tickets
 *
 * \param[in] message  The TICKETLIST message.
 */
void snaplock::ticket_list(snap::snap_communicator_message const & message)
{
    QString const list(message.get_parameter("list"));

    // add newlines for people who have TRACE mode would otherwise have
    // a hard time to find the actual list
    //
    if(list.isEmpty())
    {
        // TODO: add a --quiet command line option
        //
        std::cout << std::endl << "...no locks found..." << std::endl;
    }
    else
    {
        std::cout << std::endl << list << std::endl;
    }
}


QString snaplock::serialized_tickets()
{
    QString result;

    for(auto const & obj_ticket : f_tickets)
    {
        for(auto const & key_ticket : obj_ticket.second)
        {
            result += key_ticket.second->serialize();
            result += QChar('\n');
        }
    }

    return result;
}



}
// snaplock namespace
// vim: ts=4 sw=4 et
