// Snap Websites Server -- snap watchdog library
// Copyright (c) 2011-2019  Made to Order Software Corp.  All Rights Reserved
//
// https://snapwebsites.org/
// contact@m2osw.com
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
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA


// self
//
#include "snapwatchdog/snapwatchdog.h"


// snapwatchdog lib
//
#include "snapwatchdog/version.h"


// snapwebsites lib
//
#include <snapwebsites/email.h>
#include <snapwebsites/log.h>
#include <snapwebsites/mkdir_p.h>
#include <snapwebsites/qdomhelpers.h>
#include <snapwebsites/snap_cassandra.h>
#include <snapwebsites/xslt.h>


// snapdev lib
//
#include <snapdev/not_reached.h>
#include <snapdev/not_used.h>


// Qt lib
//
#include <QFile>


// C++ lib
//
#include <algorithm>
#include <fstream>


// C lib
//
#include <sys/wait.h>


// last include
//
#include <snapdev/poison.h>





/** \file
 * \brief This file represents the Snap! Watchdog daemon.
 *
 * The snapwatchdog.cpp and corresponding header file represents the Snap!
 * Watchdog daemon. This is not exactly a server, although it somewhat
 * (mostly) behaves like one. This tool is used as a daemon to make
 * sure that various resources on a server remain available as expected.
 */


/** \mainpage
 * \brief Snap! Watchdog Documentation
 *
 * \section introduction Introduction
 *
 * The Snap! Watchdog is a tool that works in unisson with Snap! C++.
 * It is used to monitor all the servers used with Snap! in order to
 * ensure that they all continuously work as expected.
 */


namespace snap
{

// definitions from the plugins so we can define the name and filename of
// the server plugin
namespace plugins
{
extern QString g_next_register_name;
extern QString g_next_register_filename;
}


namespace
{



/** \brief The snap communicator singleton.
 *
 * This variable holds a copy of the snap communicator singleton.
 * It is a null pointer until the watchdog() function is called.
 */
snap_communicator::pointer_t g_communicator;


/** \brief Handle the SIGINT that is expected to stop the server.
 *
 * This class is an implementation of the snap_signal that listens
 * on the SIGINT.
 */
class watchdog_interrupt
    : public snap::snap_communicator::snap_signal
{
public:
    typedef std::shared_ptr<watchdog_interrupt>     pointer_t;

                        watchdog_interrupt(watchdog_server::pointer_t ws);
    virtual             ~watchdog_interrupt() override {}

    // snap::snap_communicator::snap_signal implementation
    virtual void        process_signal() override;

private:
    watchdog_server::pointer_t  f_watchdog_server;
};


/** \brief The tick timer.
 *
 * We create one tick timer. It is saved in this variable if needed.
 */
watchdog_interrupt::pointer_t             g_interrupt;


/** \brief The interrupt initialization.
 *
 * The interrupt uses the signalfd() function to obtain a way to listen on
 * incoming Unix signals.
 *
 * Specifically, it listens on the SIGINT signal, which is the equivalent
 * to the Ctrl-C.
 *
 * \param[in] ws  The server we are listening for.
 */
watchdog_interrupt::watchdog_interrupt(watchdog_server::pointer_t ws)
    : snap_signal(SIGINT)
    , f_watchdog_server(ws)
{
    unblock_signal_on_destruction();
    set_name("watchdog interrupt");
}


/** \brief Call the stop function of the snaplock object.
 *
 * When this function is called, the signal was received and thus we are
 * asked to quit as soon as possible.
 */
void watchdog_interrupt::process_signal()
{
    // we simulate the STOP, so pass 'false' (i.e. not quitting)
    //
    f_watchdog_server->stop(false);
}



/** \brief Timer to poll Cassandra's availability.
 *
 * This class is specifically used to pretend that we received a
 * CASSANDRAREADY even when not sent to us. This is because when
 * we check for the availability of Cassandra, it may not have the
 * context and tables available yet. In that case, we would just
 * fall asleep and do nothing more.
 *
 * This timer allows us to re-check for the Cassandra context and
 * mandatory table as expected on a CASSANDRAREADY message.
 */
class cassandra_check_timer
    : public snap_communicator::snap_timer
{
public:
    typedef std::shared_ptr<cassandra_check_timer>  pointer_t;

                            cassandra_check_timer(watchdog_server::pointer_t ws);
    virtual                 ~cassandra_check_timer() override {}

    // snap_communicator::snap_connection implementation
    virtual void            process_timeout() override;

private:
    // TBD: should this be a weak pointer?
    watchdog_server::pointer_t  f_watchdog_server;
};


/** \brief The tick timer.
 *
 * We create one tick timer. It is saved in this variable if needed.
 */
cassandra_check_timer::pointer_t    g_cassandra_check_timer;


/** \brief Initialize the timer as required.
 *
 * This disables the timer and sets up its ticks to send us a timeout
 * event once per minute.
 *
 * So by default this timer does nothing (since it is disabled).
 *
 * If the check_cassandra() function somehow fails in a way that means
 * we would never get awaken again, then this timer gets turned on.
 * It will be awaken be a timeout and send us a CASSANDRAREADY to
 * simulate that something happened and we better recheck whether
 * the Cassandra connection is now truly available.
 *
 * \param[in] s  The server pointer.
 */
cassandra_check_timer::cassandra_check_timer(watchdog_server::pointer_t ws)
    : snap_timer(60LL * 1000000LL)
    , f_watchdog_server(ws)
{
    set_name("cassandra check timer");
    set_enable(false);
}


/** \brief The timer ticked.
 *
 * This function gets called each time the timer ticks. This is once
 * per minute for this timer (see constructor).
 *
 * The timer is turned off (disabled) by default. It is used only if
 * there is an error while trying to get the snap_websites context or a
 * mandatory table.
 *
 * The function simulate a CASSANDRAREADY message as if the snapdbproxy
 * service had sent it to us.
 */
void cassandra_check_timer::process_timeout()
{
    // disable ourselves, if the Cassandra cluster is still not ready,
    // then we will automatically be re-enabled
    //
    set_enable(false);

    // simulate a CASSANDRAREADY message
    //
    snap_communicator_message cassandra_ready;
    cassandra_ready.set_command("CASSANDRAREADY");
    f_watchdog_server->dispatch(cassandra_ready);
}




/** \brief The timer to produce ticks once every minute.
 *
 * This timer is the one used to know when to gather data again.
 *
 * By default the interval is set to one minute, although it is possible
 * to change that amount in the configuration file.
 */
class tick_timer
    : public snap::snap_communicator::snap_timer
{
public:
    typedef std::shared_ptr<tick_timer>        pointer_t;

                                tick_timer( watchdog_server::pointer_t ws, int64_t interval );
    virtual                     ~tick_timer() override {}

    // snap::snap_communicator::snap_timer implementation
    virtual void                process_timeout() override;

private:
    watchdog_server::pointer_t  f_watchdog_server;
};


/** \brief The tick timer.
 *
 * We create one tick timer. It is saved in this variable if needed.
 */
tick_timer::pointer_t             g_tick_timer;


/** \brief Initializes the timer with a pointer to the snap backend.
 *
 * The constructor saves the pointer of the snap_backend object so
 * it can later be used when the process times out.
 *
 * The timer is setup to trigger immediately after creation.
 * This is what starts the snap backend process.
 *
 * \param[in] sb  A pointer to the snap_backend object.
 */
tick_timer::tick_timer(watchdog_server::pointer_t ws, int64_t interval)
    : snap_timer(interval)
    , f_watchdog_server(ws)
{
    set_name("watchdog_server tick_timer");

    // start right away, but we do not want to use snap_timer(0)
    // because otherwise we will not get ongoing ticks as expected
    //
    set_timeout_date(snap_communicator::get_current_date());
}


/** \brief The timeout happened.
 *
 * This function gets called once every minute (although the interval can
 * be changed, it is 1 minute by default.) Whenever it happens, the
 * watchdog runs all the plugins once.
 */
void tick_timer::process_timeout()
{
    f_watchdog_server->process_tick();
}





/** \brief Handle messages from the Snap Communicator server.
 *
 * This class is an implementation of the TCP client message connection
 * so we can handle incoming messages.
 */
class messenger
    : public snap_communicator::snap_tcp_client_permanent_message_connection
{
public:
    typedef std::shared_ptr<messenger>    pointer_t;

                                messenger(watchdog_server::pointer_t ws, std::string const & addr, int port);
    virtual                     ~messenger() override {}

    // snap::snap_communicator::snap_tcp_client_permanent_message_connection implementation
    //virtual void                process_message(snap::snap_communicator_message const & message) override; //-- we set the dispatcher on this one too
    virtual void                process_connection_failed(std::string const & error_message) override;
    virtual void                process_connected() override;

private:
    // this is owned by a server function so no need for a smart pointer
    watchdog_server::pointer_t  f_watchdog_server;
};


/** \brief The messenger.
 *
 * We create only one messenger. It is saved in this variable.
 */
messenger::pointer_t             g_messenger;


/** \brief The messenger initialization.
 *
 * The messenger is a connection to the snapcommunicator server.
 *
 * In most cases we receive STOP and LOG messages from it. We implement
 * a few other messages too (HELP, READY...)
 *
 * We use a permanent connection so if the snapcommunicator restarts
 * for whatever reason, we reconnect automatically.
 *
 * \param[in] ws  The snap watchdog server we are listening for.
 * \param[in] addr  The address to connect to. Most often it is 127.0.0.1.
 * \param[in] port  The port to listen on (4040).
 */
messenger::messenger(watchdog_server::pointer_t ws, std::string const & addr, int port)
    : snap_tcp_client_permanent_message_connection(
                addr,
                port,
                tcp_client_server::bio_client::mode_t::MODE_PLAIN,
                snap_communicator::snap_tcp_client_permanent_message_connection::DEFAULT_PAUSE_BEFORE_RECONNECTING,
                false) // do not use a separate thread, we do many fork()'s
    , f_watchdog_server(ws)
{
    set_name("watchdog_server messenger");
}


/** \brief The messenger could not connect to snapcommunicator.
 *
 * This function is called whenever the messengers fails to
 * connect to the snapcommunicator server. This could be
 * because snapcommunicator is not running or because the
 * information given to the snapwatchdog is wrong...
 *
 * With systemd the snapcommunicator should already be running
 * although this is not 100% guaranteed. So getting this
 * error from time to time is considered normal.
 *
 * \param[in] error_message  An error message.
 */
void messenger::process_connection_failed(std::string const & error_message)
{
    SNAP_LOG_ERROR("connection to snapcommunicator failed (")(error_message)(")");

    // also call the default function, just in case
    snap_tcp_client_permanent_message_connection::process_connection_failed(error_message);
    f_watchdog_server->set_snapcommunicator_connected(false);
}


/** \brief The connection was established with Snap! Communicator.
 *
 * Whenever the connection is establied with the Snap! Communicator,
 * this callback function is called.
 *
 * The messenger reacts by REGISTERing "snapwatchdog" service with the
 * Snap! Communicator.
 */
void messenger::process_connected()
{
    snap_tcp_client_permanent_message_connection::process_connected();

    snap::snap_communicator_message register_backend;
    register_backend.set_command("REGISTER");
    register_backend.add_parameter("service", "snapwatchdog");
    register_backend.add_parameter("version", snap::snap_communicator::VERSION);
    send_message(register_backend);
    f_watchdog_server->set_snapcommunicator_connected(true);
}




/** \brief Handle the death of a child process.
 *
 * This class is an implementation of the snap signal connection so we can
 * get an event whenever our child dies.
 */
class sigchld_connection
    : public snap::snap_communicator::snap_signal
{
public:
    typedef std::shared_ptr<sigchld_connection>    pointer_t;

                                sigchld_connection(watchdog_server::pointer_t ws);
    virtual                     ~sigchld_connection() override {}

    // snap::snap_communicator::snap_signal implementation
    virtual void                process_signal() override;

private:
    // this is owned by a server function so no need for a smart pointer
    watchdog_server::pointer_t  f_watchdog_server;
};


/** \brief A pointer to the child signal connection.
 *
 * When adding connections we include this one so we can capture the
 * death of the child we create to run the statistic gathering using
 * plugins.
 */
sigchld_connection::pointer_t       g_sigchld_connection;


/** \brief The SIGCHLD signal initialization.
 *
 * The constructor defines this signal connection as a listener for
 * the SIGCHLD signal.
 *
 * \param[in] ws  The watchdog server we are listening for.
 */
sigchld_connection::sigchld_connection(watchdog_server::pointer_t ws)
    : snap_signal(SIGCHLD)
    , f_watchdog_server(ws)
{
    set_name("snapwatchdog sigchld");
}


/** \brief Process the child death signal.
 *
 * The watchdog_server process received a SIGCHLD. We can call the
 * process_sigchld() function of the watchdog_server object.
 */
void sigchld_connection::process_signal()
{
    // we can call the same function
    f_watchdog_server->process_sigchld();
}




/** \brief List of snapwatchdog commands
 *
 * The following table defines the commands understood by snapwatchdog
 * that are not defined as a default by add_snap_communicator_commands().
 */
snap::dispatcher<watchdog_server>::dispatcher_match::vector_t const g_snapwatchdog_service_messages =
{
    {
        "CASSANDRAREADY"
      , &watchdog_server::msg_cassandraready
    },
    {
        "NOCASSANDRA"
      , &watchdog_server::msg_nocassandra
    },
    {
        "RELOADCONFIG"
      , &watchdog_server::msg_reload_config
    },
    {
        "RUSAGE"
      , &watchdog_server::msg_rusage
    }
};


} // no name namespace


namespace watchdog
{

/** \brief Get a fixed watchdog plugin name.
 *
 * The watchdog plugin makes use of different fixed names. This function
 * ensures that you always get the right spelling for a given name.
 *
 * \param[in] name  The name to retrieve.
 *
 * \return A pointer to the name.
 */
char const * get_name(name_t name)
{
    switch(name)
    {
    case name_t::SNAP_NAME_WATCHDOG_ADMINISTRATOR_EMAIL:
        return "administrator_email";

    case name_t::SNAP_NAME_WATCHDOG_CACHE_PATH:
        return "cache_path";

    case name_t::SNAP_NAME_WATCHDOG_DATA_PATH:
        return "data_path";

    case name_t::SNAP_NAME_WATCHDOG_DEFAULT_LOG_PATH:
        return "/var/log/snapwebsites";

    case name_t::SNAP_NAME_WATCHDOG_ERROR_REPORT_CRITICAL_PRIORITY:
        return "error_report_critical_priority";

    case name_t::SNAP_NAME_WATCHDOG_ERROR_REPORT_LOW_PRIORITY:
        return "error_report_low_priority";

    case name_t::SNAP_NAME_WATCHDOG_ERROR_REPORT_MEDIUM_PRIORITY:
        return "error_report_medium_priority";

    case name_t::SNAP_NAME_WATCHDOG_ERROR_REPORT_SETTLE_TIME:
        return "error_report_settle_time";

    case name_t::SNAP_NAME_WATCHDOG_FROM_EMAIL:
        return "from_email";

    case name_t::SNAP_NAME_WATCHDOG_LOG_DEFINITIONS_PATH:
        return "watchdog_log_definitions_path";

    case name_t::SNAP_NAME_WATCHDOG_LOG_PATH:
        return "log_path";

    case name_t::SNAP_NAME_WATCHDOG_SERVER_NAME:
        return "server_name";

    case name_t::SNAP_NAME_WATCHDOG_SERVERSTATS:
        return "serverstats";

    case name_t::SNAP_NAME_WATCHDOG_STATISTICS_FREQUENCY:
        return "statistics_frequency";

    case name_t::SNAP_NAME_WATCHDOG_STATISTICS_PERIOD:
        return "statistics_period";

    case name_t::SNAP_NAME_WATCHDOG_STATISTICS_TTL:
        return "statistics_ttl";

    case name_t::SNAP_NAME_WATCHDOG_USER_GROUP:
        return "user_group";

    default:
        // invalid index
        throw snap_logic_exception("Invalid SNAP_NAME_WATCHDOG_...");

    }
    snapdev::NOT_REACHED();
}



}


/** \brief Initialize the watchdog server.
 *
 * This constructor makes sure to setup the correct filename for the
 * snapwatchdog server configuration file.
 */
watchdog_server::watchdog_server()
    : dispatcher(this, g_snapwatchdog_service_messages)
    , f_server_start_date(time(nullptr))
    , f_snapcommunicator_disconnected(snap_child::get_current_date())
{
    server::set_config_filename("snapwatchdog");

    add_snap_communicator_commands();
#ifdef _DEBUG
    set_trace();
#endif
}


/** \brief Retrieve a pointer to the watchdog server.
 *
 * This function retrieve an instance pointer of the watchdog server.
 * If the instance does not exist yet, then it gets created. A
 * server is also a plugin which is named "server".
 *
 * \return A manager pointer to the watchdog server.
 */
watchdog_server::pointer_t watchdog_server::instance()
{
    server::pointer_t s(get_instance());
    if(!s)
    {
        plugins::g_next_register_name = "server";
        plugins::g_next_register_filename = __FILE__;

        s = set_instance(server::pointer_t(new watchdog_server));

        plugins::g_next_register_name.clear();
        plugins::g_next_register_filename.clear();
    }
    return std::dynamic_pointer_cast<watchdog_server>(s);
}


/** \brief Get the time in seconds when the server started.
 *
 * This function is used to know at what time this watchdog instance
 * was started. We avoid sending emails within the first 5 minutes.
 *
 * \return The time, in seconds, when the server started.
 */
time_t watchdog_server::get_server_start_date() const
{
    return f_server_start_date;
}


/** \brief Print the version string to stderr.
 *
 * This function prints out the version string of this server to the standard
 * error stream.
 *
 * This is a virtual function so that way servers and daemons that derive
 * from snap::server have a chance to show their own version.
 */
void watchdog_server::show_version()
{
    std::cerr << SNAPWATCHDOG_VERSION_STRING << std::endl;
}


/** \brief Finish watchdog initialization and start the event loop.
 *
 * This function finishes the initialization such as defining the
 * server name, check that cassandra is available, and create various
 * connections such as the messenger to communicate with the
 * snapcommunicator service.
 */
void watchdog_server::watchdog()
{
    SNAP_LOG_INFO("------------------------------------ snapwatchdog started on ")(get_server_name());

    init_parameters();

    // TODO: test that the "sites" table is available?
    //       (we will not need any such table here)

    g_communicator = snap_communicator::instance();

    // capture Ctrl-C (SIGINT)
    //
    g_interrupt.reset(new watchdog_interrupt(instance()));
    g_communicator->add_connection(g_interrupt);

    // in case we cannot properly connect to Cassandra
    //
    g_cassandra_check_timer.reset(new cassandra_check_timer(instance()));
    g_communicator->add_connection(g_cassandra_check_timer);

    // get the snapcommunicator IP and port
    QString communicator_addr("127.0.0.1");
    int communicator_port(4040);
    tcp_client_server::get_addr_port(f_parameters(QString("snapcommunicator"), "local_listen"), communicator_addr, communicator_port, "tcp");

    // create the messenger, a connection between the snapwatchdogserver
    // and the snapcommunicator which allows us to communicate with
    // the watchdog (STATUS and STOP especially, more later)
    //
    g_messenger.reset(new messenger(instance(), communicator_addr.toUtf8().data(), communicator_port));
    g_communicator->add_connection(g_messenger);
    g_messenger->set_dispatcher(shared_from_this());

    // add the ticker, this wakes the system up once in a while so
    // we can gather statistics at a given interval
    //
    g_tick_timer.reset(new tick_timer(instance(), f_statistics_frequency));
    g_communicator->add_connection(g_tick_timer);

    // create a signal handler that knows when the child dies.
    //
    g_sigchld_connection.reset(new sigchld_connection(instance()));
    g_communicator->add_connection(g_sigchld_connection);

    server_loop_ready();

    // now start the run() loop
    //
    g_communicator->run();

    // got a RELOADCONFIG message?
    // (until our daemons are capable of reloading configuration files)
    //
    if(f_force_restart)
    {
        exit(1);
    }
}


/** \brief Send a message via the messenger.
 *
 * This function is an override which allows the watchdog server to
 * handle messages through the dispatcher.
 */
bool watchdog_server::send_message(snap_communicator_message const & message, bool cache)
{
    return g_messenger->send_message(message, cache);
}


/** \brief Process one tick.
 *
 * This function is called once a minute (by default). It goes and gather
 * all the data from all the plugins and then save that in the database.
 *
 * In case the tick happens too often, the function makes sure that the
 * child process is started at most once.
 */
void watchdog_server::process_tick()
{
    // Can connect to Cassandra yet?
    //
    // We now support running without Cassandra
    //
    //if(f_snapdbproxy_addr.isEmpty())
    //{
    //    SNAP_LOG_TRACE("cannot connect to Cassandra, f_snapdbproxy_addr is undefined.");
    //    return;
    //}

    // make sure we do not start more than one tick process because that
    // would cause horrible problems (i.e. many fork()'s, heavy memory
    // usage, CPU usage, incredible I/O, etc.) although that should not
    // happen because the tick happens only once per minute, you never
    // know what can happen in advance...
    //
    if(f_processes.end() == std::find_if(
                f_processes.begin(),
                f_processes.end(),
                [](auto const & child)
                {
                    return child->is_tick();
                })
    )
    {
        // create a new child object
        //
        watchdog_child::pointer_t child(std::make_shared<watchdog_child>(instance(), true));

        // start the watchdog plugins (it will fork() and return so we can
        // continue to wait for signals in our run() function.)
        //
        if(child->run_watchdog_plugins())
        {
            // the fork() succeeded, add to the list of processes
            //
            f_processes.push_back(child);
        }
    }
    else
    {
        SNAP_LOG_TRACE("previous watchdog_child still running, ignore this tick");
    }
}


/** \brief The process detected that its child died.
 *
 * The watchdog starts a child to run watchdog plugins to check various
 * things on each server (i.e. whether a process is running, etc.)
 *
 * This callback is run whenever the SIGCHLD is received. The function
 * waits on the child to remove the zombie and then it resets the
 * child process object.
 */
void watchdog_server::process_sigchld()
{
    // check for the children that are done, we cannot block here
    // especially because a child may not always signal us properly
    // (especially because we are using the signalfd capability...)
    //
    for(;;)
    {
        int status(0);
        pid_t const the_pid(waitpid(-1, &status, WNOHANG));
        if(the_pid == 0)
        {
            // no more zombie, move on
            //
            break;
        }

        if( the_pid == -1 )
        {
            // waitpid() may return with ECHILD and -1 instead of 0
            // in the_pid variable when no children are available
            //
            if(errno == ECHILD)
            {
                break;
            }

            // the waitpid() should never fail... we just generate a log and
            // go on
            //
            int const e(errno);
            SNAP_LOG_ERROR("waitpid() returned an error (")(strerror(e))(").");
        }
        else
        {
            f_processes.erase(
                    std::remove_if(
                        f_processes.begin(),
                        f_processes.end(),
                        [&the_pid](watchdog_child::pointer_t child)
                        {
                            return child->get_child_pid() == the_pid;
                        }),
                    f_processes.end());

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
            if(WIFEXITED(status))
            {
                int const exit_code(WEXITSTATUS(status));

                if( exit_code == 0 )
                {
                    // when this happens there is not really anything to tell about
                    SNAP_LOG_DEBUG("\"snapwatchdog\" statistics plugins terminated normally.");
                }
                else
                {
                    SNAP_LOG_INFO("\"snapwatchdog\" statistics plugins terminated normally, but with exit code ")(exit_code);
                }
            }
            else if(WIFSIGNALED(status))
            {
                int const signal_code(WTERMSIG(status));
                bool const has_code_dump(!!WCOREDUMP(status));

                SNAP_LOG_ERROR("\"snapwatchdog\" statistics plugins terminated because of OS signal \"")
                              (strsignal(signal_code))
                              ("\" (")
                              (signal_code)
                              (")")
                              (has_code_dump ? " and a core dump was generated" : "")
                              (".");
            }
            else
            {
                // I do not think we can reach here...
                //
                SNAP_LOG_ERROR("\"snapwatchdog\" statistics plugins terminated abnormally in an unknown way.");
            }
#pragma GCC diagnostic pop

        }
    }

    if(f_stopping
    && f_processes.empty())
    {
        g_communicator->remove_connection(g_sigchld_connection);
    }
}


/** \brief Initialize the watchdog server parameters.
 *
 * This function gets the parameters from the watchdog configuration file
 * and convert them for use by the watchdog_server implementation.
 *
 * If a parameter is not valid, the function calls exit(1) so the server
 * does not do anything.
 */
void watchdog_server::init_parameters()
{
    // Time Frequency (how often we gather the stats)
    {
        QString const statistics_frequency(get_parameter(get_name(watchdog::name_t::SNAP_NAME_WATCHDOG_STATISTICS_FREQUENCY)));
        bool ok(false);
        f_statistics_frequency = static_cast<int64_t>(statistics_frequency.toLongLong(&ok));
        if(!ok)
        {
            SNAP_LOG_FATAL("statistic frequency \"")(statistics_frequency)("\" is not a valid number.");
            exit(1);
        }
        if(f_statistics_frequency < 0)
        {
            SNAP_LOG_FATAL("statistic frequency (")(statistics_frequency)(") cannot be a negative number.");
            exit(1);
        }
        if(f_statistics_frequency < 60)
        {
            // minimum is 1 minute
            f_statistics_frequency = 60;
        }
        f_statistics_frequency *= 1000000; // the value in microseconds
    }

    // Time Period (how many stats we keep in the db)
    {
        QString const statistics_period(get_parameter(get_name(watchdog::name_t::SNAP_NAME_WATCHDOG_STATISTICS_PERIOD)));
        bool ok(false);
        f_statistics_period = static_cast<int64_t>(statistics_period.toLongLong(&ok));
        if(!ok)
        {
            SNAP_LOG_FATAL("statistic period \"")(statistics_period)("\" is not a valid number.");
            exit(1);
        }
        if(f_statistics_period < 0)
        {
            SNAP_LOG_FATAL("statistic period (")(statistics_period)(") cannot be a negative number.");
            exit(1);
        }
        if(f_statistics_period < 3600)
        {
            // minimum is 1 hour
            //
            f_statistics_period = 3600;
        }
        // round up to the hour, but keep it in seconds
        //
        f_statistics_period = (f_statistics_period + 3599) / 3600 * 3600;
    }

    // Time To Live (TTL, used to make sure we do not overcrowd the database)
    {
        QString const statistics_ttl(get_parameter(get_name(watchdog::name_t::SNAP_NAME_WATCHDOG_STATISTICS_TTL)));
        if(statistics_ttl == "off")
        {
            f_statistics_ttl = 0;
        }
        else if(statistics_ttl == "use-period")
        {
            f_statistics_ttl = f_statistics_period;
        }
        else
        {
            bool ok(false);
            f_statistics_ttl = static_cast<int64_t>(statistics_ttl.toLongLong(&ok));
            if(!ok)
            {
                SNAP_LOG_FATAL("statistic ttl \"")(statistics_ttl)("\" is not a valid number.");
                exit(1);
            }
            if(f_statistics_ttl < 0)
            {
                SNAP_LOG_FATAL("statistic ttl (")(statistics_ttl)(") cannot be a negative number.");
                exit(1);
            }
            if(f_statistics_ttl != 0
            && f_statistics_ttl < 3600)
            {
                // minimum is 1 hour
                //
                f_statistics_ttl = 3600;
            }
        }
    }

    // Amount of time before we start sending reports by email
    {
        QString const settle_time(get_parameter(get_name(watchdog::name_t::SNAP_NAME_WATCHDOG_ERROR_REPORT_SETTLE_TIME)));
        if(!settle_time.isEmpty())
        {
            bool ok(false);
            f_error_report_settle_time = static_cast<int64_t>(settle_time.toLongLong(&ok));
            if(!ok)
            {
                SNAP_LOG_FATAL("error report settle time \"")
                              (settle_time)
                              ("\" is not a valid number.");
                exit(1);
            }
            if(f_error_report_settle_time < 0)
            {
                SNAP_LOG_FATAL("error report settle time (")
                              (settle_time)
                              (") cannot be a negative number.");
                exit(1);
            }
            if(f_error_report_settle_time < 60)
            {
                // minimum is 1 minute
                //
                f_error_report_settle_time = 60;
            }
            // TBD: should we have a maximum like 1h?
        }
    }

    // Low priority and span
    {
        QString const low_priority(get_parameter(get_name(watchdog::name_t::SNAP_NAME_WATCHDOG_ERROR_REPORT_LOW_PRIORITY)));
        if(!low_priority.isEmpty())
        {
            snap::snap_string_list prio_span(low_priority.split(','));
            if(prio_span.size() > 2)
            {
                SNAP_LOG_FATAL("error report low priority \"")
                              (low_priority)
                              ("\" is expect to have two numbers separated by one comma. The second number is optional.");
                exit(1);
            }

            bool ok(false);
            f_error_report_low_priority = static_cast<int64_t>(prio_span[0].toLongLong(&ok));
            if(!ok)
            {
                SNAP_LOG_FATAL("error report low priority \"")
                              (low_priority)
                              ("\" is not a valid number.");
                exit(1);
            }
            if(f_error_report_low_priority < 1)
            {
                SNAP_LOG_FATAL("error report low priority (")
                              (low_priority)
                              (") cannot be negative or null.");
                exit(1);
            }
            if(f_error_report_low_priority > 50)
            {
                // maximum is 50
                //
                f_error_report_low_priority = 50;
            }

            if(prio_span.size() == 2
            && !prio_span[1].isEmpty())
            {
                f_error_report_low_span = static_cast<int64_t>(prio_span[1].toLongLong(&ok));
                if(!ok)
                {
                    SNAP_LOG_FATAL("error report low span \"")
                                  (low_priority)
                                  ("\" is not a valid number.");
                    exit(1);
                }
                if(f_error_report_low_span < 0)
                {
                    SNAP_LOG_FATAL("error report low span (")
                                  (low_priority)
                                  (") cannot be negative or null.");
                    exit(1);
                }
                if(f_error_report_low_span < 86400)
                {
                    // minimum is one day
                    //
                    f_error_report_low_span = 86400;
                }
            }
        }
    }

    // Medium priority and span
    {
        QString const medium_priority(get_parameter(get_name(watchdog::name_t::SNAP_NAME_WATCHDOG_ERROR_REPORT_MEDIUM_PRIORITY)));
        if(!medium_priority.isEmpty())
        {
            snap::snap_string_list prio_span(medium_priority.split(','));
            if(prio_span.size() > 2)
            {
                SNAP_LOG_FATAL("error report medium priority \"")
                              (medium_priority)
                              ("\" is expect to have two numbers separated by one comma. The second number is optional.");
                exit(1);
            }

            bool ok(false);
            f_error_report_medium_priority = static_cast<int64_t>(prio_span[0].toLongLong(&ok));
            if(!ok)
            {
                SNAP_LOG_FATAL("error report medium priority \"")
                              (medium_priority)
                              ("\" is not a valid number.");
                exit(1);
            }
            if(f_error_report_medium_priority < 1)
            {
                SNAP_LOG_FATAL("error report medium priority (")
                              (medium_priority)
                              (") cannot be negative or null.");
                exit(1);
            }

            if(prio_span.size() == 2
            && !prio_span[1].isEmpty())
            {
                f_error_report_medium_span = static_cast<int64_t>(prio_span[1].toLongLong(&ok));
                if(!ok)
                {
                    SNAP_LOG_FATAL("error report medium span \"")
                                  (medium_priority)
                                  ("\" is not a valid number.");
                    exit(1);
                }
                if(f_error_report_medium_span < 0)
                {
                    SNAP_LOG_FATAL("error report medium span (")
                                  (medium_priority)
                                  (") cannot be negative or null.");
                    exit(1);
                }
                if(f_error_report_medium_span < 3600)
                {
                    // minimum is one hour
                    //
                    f_error_report_medium_span = 3600;
                }
            }
        }
    }

    // Critical priority and span
    {
        QString const critical_priority(get_parameter(get_name(watchdog::name_t::SNAP_NAME_WATCHDOG_ERROR_REPORT_CRITICAL_PRIORITY)));
        if(!critical_priority.isEmpty())
        {
            snap::snap_string_list prio_span(critical_priority.split(','));
            if(prio_span.size() > 2)
            {
                SNAP_LOG_FATAL("error report critical priority \"")
                              (critical_priority)
                              ("\" is expect to have two numbers separated by one comma. The second number is optional.");
                exit(1);
            }

            bool ok(false);
            f_error_report_critical_priority = static_cast<int64_t>(prio_span[0].toLongLong(&ok));
            if(!ok)
            {
                SNAP_LOG_FATAL("error report critical priority \"")
                              (critical_priority)
                              ("\" is not a valid number.");
                exit(1);
            }
            if(f_error_report_critical_priority < 1)
            {
                SNAP_LOG_FATAL("error report critical priority (")
                              (critical_priority)
                              (") cannot be negative or null.");
                exit(1);
            }
            if(f_error_report_critical_priority > 100)
            {
                // TBD: should we just clamp silently instead of a fatal error?
                //
                SNAP_LOG_FATAL("error report critical priority (")
                              (critical_priority)
                              (") cannot be larger than 100.");
                exit(1);
            }

            if(prio_span.size() == 2
            && !prio_span[1].isEmpty())
            {
                f_error_report_critical_span = static_cast<int64_t>(prio_span[1].toLongLong(&ok));
                if(!ok)
                {
                    SNAP_LOG_FATAL("error report critical span \"")
                                  (critical_priority)
                                  ("\" is not a valid number.");
                    exit(1);
                }
                if(f_error_report_critical_span < 0)
                {
                    SNAP_LOG_FATAL("error report critical span (")
                                  (critical_priority)
                                  (") cannot be negative or null.");
                    exit(1);
                }
                if(f_error_report_critical_span < 300)
                {
                    // minimum is five minutes
                    //
                    f_error_report_critical_span = 300;
                }
            }
        }
    }

    // now that all the priority & span numbers are defined we can verify
    // that they are properly sorted
    //
    if(f_error_report_medium_priority < f_error_report_low_priority)
    {
        SNAP_LOG_FATAL("error report medium priority (")
                      (f_error_report_medium_priority)
                      (") cannot be less than the low priority (")
                      (f_error_report_low_priority)
                      (").");
        exit(1);
    }
    if(f_error_report_critical_priority < f_error_report_medium_priority)
    {
        SNAP_LOG_FATAL("error report critical priority (")
                      (f_error_report_critical_priority)
                      (") cannot be less than the medium priority (")
                      (f_error_report_medium_priority)
                      (").");
        exit(1);
    }

    if(f_error_report_medium_span > f_error_report_low_span)
    {
        SNAP_LOG_FATAL("error report medium span (")
                      (f_error_report_medium_span)
                      (") cannot be more than the low span (")
                      (f_error_report_low_span)
                      (").");
        exit(1);
    }
    if(f_error_report_critical_span > f_error_report_medium_span)
    {
        SNAP_LOG_FATAL("error report critical span (")
                      (f_error_report_critical_span)
                      (") cannot be more than the medium span (")
                      (f_error_report_medium_span)
                      (").");
        exit(1);
    }
}


void watchdog_server::msg_nocassandra(snap::snap_communicator_message & message)
{
    snapdev::NOT_USED(message);

    // we lost Cassandra, "disconnect" from snapdbproxy until we
    // get CASSANDRAREADY again
    //
    f_snapdbproxy_addr.clear();
    f_snapdbproxy_port = 0;
}


void watchdog_server::msg_cassandraready(snap::snap_communicator_message & message)
{
    snapdev::NOT_USED(message);

    // connect to Cassandra and verify that a "serverstats"
    // table exists
    //
    bool timer_required(false);
    if(!check_cassandra(get_name(watchdog::name_t::SNAP_NAME_WATCHDOG_SERVERSTATS), timer_required))
    {
        if(timer_required
        && g_cassandra_check_timer != nullptr)
        {
            // it did not quite work, setup a timer so the snapwatchdog
            // daemon gets awaken again later to attempt a new connect
            //
            g_cassandra_check_timer->set_enable(true);
        }
    }
}


void watchdog_server::msg_rusage(snap::snap_communicator_message & message)
{
    // Can connect to Cassandra yet?
    //
    // We now support running without Cassandra
    //
    //if(f_snapdbproxy_addr.isEmpty())
    //{
    //    return;
    //}

    // a process just sent us its RUSAGE just before exiting
    // (note that a UDP message is generally used to send that info
    // so we are likely to miss some of those statistics)
    //
    watchdog_child::pointer_t child(std::make_shared<watchdog_child>(instance(), false));

    // we use a child because we need to connect to the database
    // so that call returns immediately after the fork() call
    //
    if(child->record_usage(message))
    {
        // the fork() succeeded, keep the child as a process
        //
        f_processes.push_back(child);
    }
}


void watchdog_server::ready(snap::snap_communicator_message & message)
{
    snapdev::NOT_USED(message);

    // TBD: should we wait on this signal before we start the g_tick_timer?
    //      since we do not need the snap communicator, probably not useful
    //      (however, we like to have Cassandra and we know Cassandra is
    //      ready only after a we got the CASSANDRAREADY anyway...)
    //

    // request snapdbproxy to send us a status signal about
    // Cassandra, after that one call, we will receive the
    // changes in status just because we understand them.
    //
    snap::snap_communicator_message isdbready_message;
    isdbready_message.set_command("CASSANDRASTATUS");
    isdbready_message.set_service("snapdbproxy");
    g_messenger->send_message(isdbready_message);
}


void watchdog_server::msg_reload_config(snap::snap_communicator_message & message)
{
    snapdev::NOT_USED(message);

    f_force_restart = true;
    stop(false);
}


int64_t watchdog_server::get_error_report_settle_time() const
{
    return f_error_report_settle_time;
}


int64_t watchdog_server::get_error_report_low_priority() const
{
    return f_error_report_low_priority;
}


int64_t watchdog_server::get_error_report_low_span() const
{
    return f_error_report_low_span;
}


int64_t watchdog_server::get_error_report_medium_priority() const
{
    return f_error_report_medium_priority;
}


int64_t watchdog_server::get_error_report_medium_span() const
{
    return f_error_report_medium_span;
}


int64_t watchdog_server::get_error_report_critical_priority() const
{
    return f_error_report_critical_priority;
}


int64_t watchdog_server::get_error_report_critical_span() const
{
    return f_error_report_critical_span;
}





void watchdog_server::stop(bool quitting)
{
    SNAP_LOG_INFO("Stopping watchdog server.");

    f_stopping = true;

    if(g_messenger != nullptr)
    {
        if(quitting || !g_messenger->is_connected())
        {
            // turn off that connection now, we cannot UNREGISTER since
            // we are not connected to snapcommunicator
            //
            g_communicator->remove_connection(g_messenger);
            g_messenger.reset();
        }
        else
        {
            g_messenger->mark_done();

            // if not snapcommunicator is not quitting, send an UNREGISTER
            //
            snap_communicator_message unregister;
            unregister.set_command("UNREGISTER");
            unregister.add_parameter("service", "snapwatchdog");
            g_messenger->send_message(unregister);
        }
    }

    g_communicator->remove_connection(g_interrupt);
    g_communicator->remove_connection(g_cassandra_check_timer);
    g_communicator->remove_connection(g_tick_timer);
    if(f_processes.empty())
    {
        g_communicator->remove_connection(g_sigchld_connection);
    }
}



void watchdog_server::set_snapcommunicator_connected(bool status)
{
    if(status)
    {
        f_snapcommunicator_connected = snap_child::get_current_date();
    }
    else
    {
        f_snapcommunicator_disconnected = snap_child::get_current_date();
    }
}



bool watchdog_server::get_snapcommunicator_is_connected() const
{
    return f_snapcommunicator_disconnected < f_snapcommunicator_connected;
}



int64_t watchdog_server::get_snapcommunicator_connected_on() const
{
    return f_snapcommunicator_connected;
}



int64_t watchdog_server::get_snapcommunicator_disconnected_on() const
{
    return f_snapcommunicator_disconnected;
}



/** \brief Initialize the watchdog child.
 *
 * This function saves the server pointer so it can be accessed
 * as we do in plugins with the f_snap pointer.
 *
 * \param[in] s  The server watchdog.
 * \param[in] tick  Whether the child is used to work on a tick or not.
 */
watchdog_child::watchdog_child(server_pointer_t s, bool tick)
    : snap_child(s)
    , f_tick(tick)
{
}


/** \brief Clean up the watchdog child.
 *
 * Make sure the child is cleaned.
 */
watchdog_child::~watchdog_child()
{
}


/** \brief Check whether this child was created to process a tick.
 *
 * If this function returns true, then this indicates that the process
 * was created to process a timer tick. This is run once per minute to
 * generate a constant stream of statistic data.
 *
 * Other children may be created for other purposes in which case this
 * function returns false (i.e. the RUSAGE implementation.)
 *
 * \return true if created to process the timer tick.
 */
bool watchdog_child::is_tick() const
{
    return f_tick;
}


/** \brief Get the path to a file in the snapwatchdog cache.
 *
 * This function returns a full path to the snapwatchdog cache plus
 * the specified filename.
 *
 * \note
 * The function ensures that the snapwatchdog sub-directory exists.
 * i.e. the /var/cache/snapwebsites directory exists, however,
 * the /var/cache/snapwebsites/snapwatchdog directory may not exist
 * yet.
 *
 * \param[in] filename  The name of the file to access in the cache.
 *
 * \return The full path including your filename.
 */
QString watchdog_child::get_cache_path(QString const & filename)
{
    if(f_cache_path.isEmpty())
    {
        // get the path specified by the administrator
        //
        f_cache_path = get_server_parameter(snap::watchdog::get_name(snap::watchdog::name_t::SNAP_NAME_WATCHDOG_CACHE_PATH));
        if(f_cache_path.isEmpty())
        {
            // no administrator path, use the default
            //
            f_cache_path = "/var/cache/snapwebsites/snapwatchdog";
        }

        // the path to "/var/cache/snapwebsites" will always
        // exists, however "/var/cache/snapwebsites/snapwatchdog"
        // may get deleted once in a while, we have to create it
        //
        mkdir_p(f_cache_path);
    }

    // append the name of the file to check out in the path
    //
    return QString("%1/%2").arg(f_cache_path).arg(filename);
}


/** \brief Run watchdog plugins.
 *
 * This function runs all the watchdog plugins and saves the results
 * in a file and the database.
 *
 * If no plugins are are defined, the result will be empty.
 *
 * The function makes use of a try/catch block to avoid ending the
 * process if an error occurs. However, problems should get fixed
 * or you will certainly not get the results you are looking for.
 */
bool watchdog_child::run_watchdog_plugins()
{
    // create a child process so the data between sites does not get
    // shared (also the Cassandra data would remain in memory increasing
    // the foot print each time we run a new website,) but the worst
    // are the plugins; we can request a plugin to be unloaded but
    // frankly the system is not very well written to handle that case.
    //
    f_child_pid = fork_child();
    if(f_child_pid != 0)
    {
        int const e(errno);

        // parent process
        if(f_child_pid == -1)
        {
            // we do not try again, we just abandon the whole process
            //
            SNAP_LOG_ERROR("watchdog_child::run_watchdog_plugins() could not create child process, fork() failed with errno: ")(e)(" -- ")(strerror(e))(".");
            return false;
        }

        SNAP_LOG_TRACE("new watchdog_child started, pid = ")(f_child_pid);

        return true;
    }

    // we are the child, run the watchdog_process() signal
    try
    {
        f_ready = false;

        // on fork() we lose the configuration so we have to reload it
        //
        logging::reconfigure();

        init_start_date();

        // the usefulness of the weak pointer is questionable here since
        // we have it locked for the rest of the child process
        //
        auto server(std::dynamic_pointer_cast<watchdog_server>(f_server.lock()));
        if(server == nullptr)
        {
            throw snap_child_exception_no_server("watchdog_child::run_watchdog_plugins(): The f_server weak pointer could not be locked");
        }

        if(server->snapdbproxy_addr().isEmpty())
        {
            // no need to test if the address is empty
            //
            f_has_cassandra = false;
        }
        else
        {
            f_has_cassandra = connect_cassandra(false);
        }

        // initialize the plugins
        //
        // notice the introducer, it's important since all the watchdog
        // plugin names start wit "lib" + "watchdog_" + <name> + ".so"
        //
        init_plugins(false, "watchdog");

        f_ready = true;

        // create the watchdog document
        //
        QDomDocument doc("watchdog");

        // it doesn't look like Qt supports such
        //QDomProcessingInstruction stylesheet(doc.createProcessingInstruction("xml-stylesheet", QString()));
        //stylesheet.setAttribute("type", "text/xsl");
        //stylesheet.setAttribute("href", "file:///home/snapwebsites/snapcpp/snapwebsites/snapwatchdog/conf/snapwatchdog-data.xsl");
        //doc.appendChild(stylesheet);

        // run each plugin watchdog function
        //
        {
            // if we are in debug mode, let all messages go through,
            // otherwise raise the level to WARNING to limit the messages
            // because with a large number of plugins it generates a large
            // number of log every single minute!
            //
            snap::logging::raii_log_level save_log_level(server->is_debug()
                                            ? snap::logging::get_log_output_level()
                                            : snap::logging::log_level_t::LOG_LEVEL_WARNING);
            server->process_watch(doc);
        }

        // verify and save the results accordingly
        //
        QString result(doc.toString());
        if(result.isEmpty())
        {
            static bool err_once(true);
            if(err_once)
            {
                err_once = false;
                SNAP_LOG_ERROR("watchdog_child::run_watchdog_plugins() generated a completely empty result. This can happen if you do not define any watchdog plugins.");
            }
        }
        else
        {
            int64_t const start_date(get_start_date());

            // round to the minute first, then apply period
            //
            int64_t const date((start_date / (1000000LL * 60LL) * 60LL) % server->get_statistics_period());

            // add the date in us to this result
            //
            QDomElement watchdog_tag(snap_dom::create_element(doc, "watchdog"));
            watchdog_tag.setAttribute("date", static_cast<qlonglong>(start_date));
            int64_t const current_date(get_current_date());
            watchdog_tag.setAttribute("end-date", static_cast<qlonglong>(current_date));
            result = "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
                     "<?xml-stylesheet"
                               " type=\"text/xsl\""
                               " href=\"/snapwatchdog-data.xsl\"?>"
                     + doc.toString(-1);
            result.remove("<!DOCTYPE watchdog>");

            // save the result in a file first
            //
            QString data_path(server->get_parameter(watchdog::get_name(watchdog::name_t::SNAP_NAME_WATCHDOG_DATA_PATH)));
            if(!data_path.isEmpty())
            {
                data_path += QString("/data/%1.xml").arg(date);

                std::ofstream out;
                out.open(data_path.toUtf8().data(), std::ios_base::binary);
                if(out.is_open())
                {
                    // result already ends with a "\n"
                    out << result;
                }
            }

            // if there is an <error> tag, send an email about it
            //
            // give 5 min. to the server to get everything started, though,
            // because otherwise we'd get a lot of false positive
            //
            size_t count(0);
            time_t const server_start_date(server->get_server_start_date());
            time_t const now(time(nullptr));
            int64_t diff(now - server_start_date);
            if(diff >= server->get_error_report_settle_time())
            {
                QDomElement error(snap_dom::get_child_element(doc, "watchdog/error"));
                if(!error.isNull())
                {
                    // there is an <error> tag, report it, however, we do not
                    // want to send more than one email every 15 min. unless
                    // there is an error with a priority of 90 or more
                    //
                    int max_priority(0);
                    for(QDomNode n(error.firstChild()); !n.isNull(); n = n.nextSibling())
                    {
                        if(n.isElement())
                        {
                            QDomElement msg(n.toElement());

                            QString const attr_str(msg.attribute("priority"));
                            bool ok(false);
                            int const p(attr_str.toInt(&ok, 10));
                            if(ok
                            && p > max_priority)
                            {
                                max_priority = p;
                            }

                            ++count;
                        }
                    }

                    // if too low a priority then ignore the errors altogether
                    //
                    // TODO: make this "10" a parameter in the watchdog.conf
                    //       file so the user can choose what to receive
                    //
                    if(max_priority >= server->get_error_report_low_priority())
                    {
                        // how often to send an email depends on the priority
                        // and the span parameters
                        //
                        // note that too often on a large cluster and you'll
                        // die under the pressure! (some even call it spam)
                        // so we limit the emails quite a bit by default...
                        // admins can check the status any time from the
                        // server side in snapmanager anyway and also the
                        // priorities and span parameters can be changed
                        // in the configuration file (search for
                        // `error_report_` parameters in snapwatchdog.conf)
                        //
                        // note that the span does last across restarts of
                        // the application
                        //
                        // the defaults at this time are:
                        //
                        // +----------+----------+--------+
                        // | name     | priority | span   |
                        // +----------+----------+--------+
                        // | low      |       10 | 1 week |
                        // | medium   |       50 | 3 days |
                        // | critical |       90 | 1 day  |
                        // +----------+----------+--------+
                        //
                        int64_t const span(max_priority >= server->get_error_report_critical_priority()
                                                ? server->get_error_report_critical_span()
                                                : (max_priority >= server->get_error_report_medium_priority()
                                                    ? server->get_error_report_medium_span()
                                                    : server->get_error_report_low_span()));

                        // use a file in the cache area since we are likely
                        // to regenerate it often or just ignore it for a
                        // while (and if ignored for a while it could as
                        // well be deleted)
                        //
                        QString const last_email_filename(get_cache_path("last_email_time.txt"));

                        bool send_email(true);
                        bool file_opened(false);
                        QFile file(last_email_filename);
                        if(file.exists())
                        {
                            // when the file exists we want to read it
                            // first and determine whether 'span' has
                            // passed, if so, we write 'now' in the file
                            // and send the email
                            //
                            if(file.open(QIODevice::ReadOnly))
                            {
                                file_opened = true;
                                QByteArray const value(file.readAll());
                                QString const last_mail_date_str(value);
                                bool ok(false);
                                int64_t const last_mail_date(last_mail_date_str.toLongLong(&ok, 10));
                                if(ok)
                                {
                                    if(now - last_mail_date < span)
                                    {
                                        // span has not yet elapsed, keep
                                        // the file as is and don't send
                                        // the email
                                        //
                                        send_email = false;
                                    }
                                }
                                file.close();
                            }
                        }

                        if(send_email)
                        {
                            // first save the time when we are sending the email
                            //
                            if(file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
                            {
                                QString const data_str(QString("%1").arg(now));
                                QByteArray const data(data_str.toUtf8());
                                file.write(data.data(), data.size());
                                file.close();
                            }

                            // get the emails where to send the data
                            // if not available, it "breaks" the process
                            //
                            QString from_email(get_server_parameter(get_name(watchdog::name_t::SNAP_NAME_WATCHDOG_FROM_EMAIL)));
                            QString administrator_email(get_server_parameter(get_name(watchdog::name_t::SNAP_NAME_WATCHDOG_ADMINISTRATOR_EMAIL)));
                            if(!from_email.isEmpty()
                            && !administrator_email.isEmpty())
                            {
                                // create the email and add a few headers
                                //
                                email e;
                                e.set_from(from_email);
                                e.set_to(administrator_email);
                                e.set_priority(email::priority_t::EMAIL_PRIORITY_URGENT);

                                char hostname[HOST_NAME_MAX + 1];
                                if(gethostname(hostname, sizeof(hostname)) != 0)
                                {
                                    strncpy(hostname, "<unknown>", sizeof(hostname));
                                }
                                QString const subject(QString("snapwatchdog: found %1 error%2 on %3")
                                                .arg(count)
                                                .arg(count == 1 ? "" : "s")
                                                .arg(hostname));
                                e.set_subject(subject);

                                e.add_header("X-SnapWatchdog-Version", SNAPWATCHDOG_VERSION_STRING);

                                // prevent blacklisting
                                // (since we won't run the `sendmail` plugin validation, it's not necessary)
                                //e.add_parameter(sendmail::get_name(sendmail::name_t::SNAP_NAME_SENDMAIL_BYPASS_BLACKLIST), "true");

                                // generate a body in HTML
                                //
                                QByteArray data;
                                {
                                    QString const error_to_email_filename(":/xsl/layout/error-to-email.xsl");
                                    QFile error_to_email_file(error_to_email_filename);
                                    if(error_to_email_file.open(QIODevice::ReadOnly))
                                    {
                                        data = error_to_email_file.readAll();
                                    }
                                }
                                email::attachment html;
                                QString const xsl(QString::fromUtf8(data.data(), data.size()));
                                if(xsl.isEmpty())
                                {
                                    SNAP_LOG_ERROR("could not read error-to-email.xsl from resources.");
                                    html.quoted_printable_encode_and_set_data("<html><body><p>Sorry! Could not find error-to-email.xsl in the resources. See Snap! Watchdog errors in attached XML.</p></body></html>", "text/html");
                                }
                                else
                                {
                                    xslt x;
                                    x.set_xsl(xsl);
                                    x.set_document(doc);
                                    QDomDocument doc_email("html");
                                    x.evaluate_to_document(doc_email);
                                    html.quoted_printable_encode_and_set_data(doc_email.toString(-1).toUtf8(), "text/html");
                                }
                                e.set_body_attachment(html);

                                // add the XML as an attachment
                                //
                                email::attachment a;
                                a.quoted_printable_encode_and_set_data(result.toUtf8(), "application/xml");
                                a.set_content_disposition("snapwatchdog.xml");
                                a.add_header("X-Start-Date", QString("%1").arg(start_date));
                                e.add_attachment(a);

                                // finally send email
                                //
                                e.send();
                            }
                        }
                    }
                }
            }
            else
            {
                // let us know with a debug in case error go unreported
                // because they happen early on; these could be of interest
                // in some cases
                //
                SNAP_LOG_DEBUG("found errors, but not reporting them because it has been less than 5 min. that this daemon started.");
            }

            // save the number of errors to a file so the snapmanager
            // can actually pick that information and display it
            // (through the snapwatchdog plugin extension to the
            // snapmanager.)
            //
            QString const last_result_filename(get_cache_path("last_results.txt"));
            std::ofstream info;
            info.open(last_result_filename.toUtf8().data(), std::ios_base::binary | std::ios_base::trunc | std::ios_base::out);
            if(info.is_open())
            {
                info << "# This is an auto-generated file. Do not edit." << std::endl;
                info << "error_count=" << count << std::endl;
                info << "data_path=" << data_path << std::endl;
            }

            // then try to save it in the Cassandra database
            // (if the cluster is not available, we still have the files!)
            //
            // retrieve server statistics table
            //
            int64_t const ttl(server->get_statistics_ttl());
            if(f_has_cassandra
            && ttl > 0)
            {
                QString const table_name(get_name(watchdog::name_t::SNAP_NAME_WATCHDOG_SERVERSTATS));
                libdbproxy::table::pointer_t table(f_context->getTable(table_name));

                libdbproxy::value value;
                value.setStringValue(result);
                value.setTtl(ttl);
                QByteArray cell_key;
                libdbproxy::setInt64Value(cell_key, date);
                table->getRow(QString::fromUtf8(server->get_server_name().c_str()) + "/system-statistics")->getCell(cell_key)->setValue(value);
            }
        }

        // the child has to exit()
        exit(0);
        snapdev::NOT_REACHED();
    }
    catch(snap_exception const & e)
    {
        SNAP_LOG_FATAL("watchdog_child::run_watchdog_plugins(): exception caught ")(e.what());
    }
    catch(std::exception const & e)
    {
        SNAP_LOG_FATAL("watchdog_child::run_watchdog_plugins(): exception caught ")(e.what());
    }
    catch(...)
    {
        SNAP_LOG_FATAL("watchdog_child::run_watchdog_plugins(): unknown exception caught!");
    }
    exit(1);
    snapdev::NOT_REACHED();
    return false; // not reached, true or false is pretty much the same here
}


/** \brief Process a RUSAGE message.
 *
 * This function processes an RUSAGE message. Since it requires access to
 * the database which the server does not have, we create a child process
 * to do the work.
 *
 * \param[in] message  The message we just received.
 */
bool watchdog_child::record_usage(snap::snap_communicator_message const & message)
{
    // create a child process so the data between sites does not get
    // shared (also the Cassandra data would remain in memory increasing
    // the foot print each time we run a new website,) but the worst
    // are the plugins; we can request a plugin to be unloaded but
    // frankly the system is not very well written to handle that case.
    //
    f_child_pid = fork_child();
    if(f_child_pid != 0)
    {
        int const e(errno);

        // parent process
        //
        if(f_child_pid == -1)
        {
            // we do not try again, we just abandon the whole process
            //
            SNAP_LOG_ERROR("watchdog_child::record_usage() could not create child process, fork() failed with errno: ")(e)(" -- ")(strerror(e))(".");
            return false;
        }
        return true;
    }

    // we are the child, run the actual record_usage() function
    try
    {
        f_ready = false;

        // on fork() we lose the configuration so we have to reload it
        //
        logging::reconfigure();

        init_start_date();

        auto server(std::dynamic_pointer_cast<watchdog_server>(f_server.lock()));
        if(server == nullptr)
        {
            throw snap_child_exception_no_server("watchdog_child::record_usage(): The f_server weak pointer could not be locked");
        }

        if(server->snapdbproxy_addr().isEmpty())
        {
            // no need to test if the address is empty
            //
            f_has_cassandra = false;
        }
        else
        {
            f_has_cassandra = connect_cassandra(false);
        }

        QDomDocument doc("watchdog");
        QDomElement parent(snap_dom::create_element(doc, "watchdog"));
        QDomElement e(snap_dom::create_element(parent, "rusage"));

        QString const process_name(message.get_parameter("process_name"));
        QString const pid(message.get_parameter("pid"));
        e.setAttribute("process_name",                  process_name);
        e.setAttribute("pid",                           pid);
        e.setAttribute("user_time",                     message.get_parameter("user_time"));
        e.setAttribute("system_time",                   message.get_parameter("system_time"));
        e.setAttribute("maxrss",                        message.get_parameter("maxrss"));
        e.setAttribute("minor_page_fault",              message.get_parameter("minor_page_fault"));
        e.setAttribute("major_page_fault",              message.get_parameter("major_page_fault"));
        e.setAttribute("in_block",                      message.get_parameter("in_block"));
        e.setAttribute("out_block",                     message.get_parameter("out_block"));
        e.setAttribute("volontary_context_switches",    message.get_parameter("volontary_context_switches"));
        e.setAttribute("involontary_context_switches",  message.get_parameter("involontary_context_switches"));

        int64_t const start_date(get_start_date());

        // add the date in us to this result
        //
        QDomElement watchdog_tag(snap_dom::create_element(doc, "watchdog"));
        watchdog_tag.setAttribute("date", static_cast<qlonglong>(start_date));
        QString const result = doc.toString(-1);

        // save the result in a file first
        //
        QString data_path(server->get_parameter(watchdog::get_name(watchdog::name_t::SNAP_NAME_WATCHDOG_DATA_PATH)));
        if(!data_path.isEmpty())
        {
            data_path += QString("/rusage/%1.xml").arg(pid);

            std::ofstream out;
            out.open(data_path.toUtf8().data(), std::ios_base::binary);
            if(out.is_open())
            {
                // result already ends with a "\n"
                //
                out << result;
            }
        }

        int64_t const ttl(server->get_statistics_ttl());
        if(f_has_cassandra
        && ttl > 0)
        {
            QString const table_name(get_name(watchdog::name_t::SNAP_NAME_WATCHDOG_SERVERSTATS));
            libdbproxy::table::pointer_t table(f_context->getTable(table_name));

            libdbproxy::value value;
            value.setStringValue(result);
            value.setTtl(ttl);
            QString const cell_key(QString("%1::%2").arg(process_name).arg(start_date));
            table->getRow(QString::fromUtf8(server->get_server_name().c_str()) + "/rusage")->getCell(cell_key)->setValue(value);
        }

        // the child has to exit()
        exit(0);
        snapdev::NOT_REACHED();
    }
    catch(snap_exception const & e)
    {
        SNAP_LOG_FATAL("watchdog_child::record_usage(): exception caught ")(e.what());
    }
    catch(std::exception const & e)
    {
        SNAP_LOG_FATAL("watchdog_child::record_usage(): exception caught ")(e.what());
    }
    catch(...)
    {
        SNAP_LOG_FATAL("watchdog_child::record_usage(): unknown exception caught!");
    }
    exit(1);
    snapdev::NOT_REACHED();
    return false; // not reached, true or false is pretty much the same here
}


/** \brief Return the child pid.
 *
 * This function can be used to retrieve the PID of the child process.
 * The child is created after when the run_watchdog_plugins() is called.
 * Until then it is -1. Note that in the child process, this function
 * will return 0.
 *
 * \return The child process identifier.
 */
pid_t watchdog_child::get_child_pid() const
{
    return f_child_pid;
}


/** \brief Attach an error to the specified \p doc DOM.
 *
 * This function creates an \<error> element and add the specified
 * message to it. The message can be any text you'd like.
 *
 * The plugin_name is expected to match the name of your plugin one to one.
 *
 * The priority is used to know whether an email will be sent to the user
 * or not. By default it is 50 and the configuration file says to send
 * emails if the priority is 1 or more. We expect numbers between 0 and 100.
 *
 * \param[in] doc  The DOM document where the \<error> tag is created.
 * \param[in] plugin_name  The name of the plugin generating this error.
 * \param[in] message  The error message. This is free form. It can't include
 *            tags (\< and \> will be inserted as \&lt; and \&gt;.)
 * \param[in] priority  The priority of this error. The higher the greater the
 *            priority and the sooner the error will be sent to the
 *            administrator.
 */
void watchdog_child::append_error(QDomDocument doc, QString const & plugin_name, QString const & message, int priority)
{
    // log the error so we have a trace
    //
    QString clean_message(message);
    clean_message.replace("\n", " -- ");
    SNAP_LOG_ERROR("plugin \"")
                  (plugin_name)
                  ("\" detected an error: ")
                  (clean_message)
                  (" (")
                  (priority)
                  (")");

    if(priority < 0 || priority > 100)
    {
        throw snapwatchdog_exception_invalid_parameters(
                  "priority must be between 0 and 100 inclusive, "
                + std::to_string(priority)
                + " is not valid.");
    }

    QDomElement parent(snap_dom::create_element(doc, "watchdog"));
    QDomElement err(snap_dom::create_element(parent, "error"));

    // use createElement() so we get one message per call
    //QDomElement msg(snap_dom::create_element(doc, "message"));
    QDomElement msg(doc.createElement("message"));
    err.appendChild(msg);

    msg.setAttribute("plugin_name", plugin_name);
    msg.setAttribute("priority", priority);

    // handle new lines so the emails look good, but keep the rest as
    // plain text
    //
    QStringList lines(message.split("\n"));

    if(!lines.isEmpty())
    {
        QDomText text(doc.createTextNode(lines[0]));
        msg.appendChild(text);
        for(int l(1); l < lines.size(); ++l)
        {
            QDomElement br(doc.createElement("br"));
            msg.appendChild(br);

            QDomText following_line(doc.createTextNode(lines[l]));
            msg.appendChild(following_line);
        }
    }
}


void watchdog_child::append_error(QDomDocument doc, QString const & plugin_name, std::string const & message, int priority)
{
    append_error(doc, plugin_name, QString::fromUtf8(message.c_str()), priority);
}


void watchdog_child::append_error(QDomDocument doc, QString const & plugin_name, char const * message, int priority)
{
    append_error(doc, plugin_name, QString::fromUtf8(message), priority);
}


watchdog_server::pointer_t watchdog_child::get_server()
{
    auto server(std::dynamic_pointer_cast<watchdog_server>(f_server.lock()));
    if(server == nullptr)
    {
        throw snap_child_exception_no_server("watchdog_child::get_server(): The f_server weak pointer could not be locked");
    }
    return server;
}


/** \brief Make sure to clean up then exit the child process.
 *
 * This function cleans up the child and then calls the
 * server::exit() function to give the server a chance to
 * also clean up. Then it exists by calling the exit(3)
 * function of the C library.
 *
 * \note
 * We reimplement the snap_child::exit() function because
 * the default function sends a message to the watchdog and
 * that would create a loop. So to avoid that loop, we
 * reimplement the function without sending the message.
 *
 * \param[in] code  The exit code, generally 0 or 1.
 */
void watchdog_child::exit(int code)
{
    // make sure the socket data is pushed to the caller
    f_client.reset();

    server::exit(code);
    snapdev::NOT_REACHED();
}


} // namespace snap
// vim: ts=4 sw=4 et
