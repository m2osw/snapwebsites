// Snap Websites Server -- snap websites serving children
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

#include "snapwebsites/snap_backend.h"

#include "snapwebsites/snapwebsites.h"
#include "snapwebsites/log.h"
#include "snapwebsites/not_used.h"
#include "snapwebsites/snap_lock.h"

#include <wait.h>
#include <fcntl.h>
#include <sys/resource.h>

#include "snapwebsites/poison.h"


namespace snap
{

/** \class snap_backend
 * \brief Backend process class.
 *
 * This class handles backend processing for the snapserver.
 *
 * The process for backends works this way:
 *
 * \li Backend tool prepares the server
 * \li Backend tool creates a snap_backend object.
 * \li Backend tool calls run_backend()
 * \li run_backend() connects to the database
 * \li run_backend() checks whether the "sites" table exists
 * \li if not ready -- wait until the "sites" table exists
 * \li -- while waiting for the "sites" table, we also listen for
 *     incoming messages such as STOP and LOG
 *
 * Note that the backend, like pretty much all other Snap processes
 * is event based. It receives messages from various sources and
 * deals with those as required. The following describes those
 * messages:
 *
 * \msc
 * hscale = 2;
 * a [label="snapcommunicator"],
 * b [label="snapbackend"],
 * c [label="child process"],
 * d [label="wakeup timer"],
 * e [label="cassandra"];
 *
 * #
 * # Register to the snap communicator
 * #
 * b=>b [label="open socket to snapcommunicator"];
 * b->a [label="REGISTER service=<service name>;version=<version>"];
 * a->b [label="READY"];
 * a->b [label="HELP"];
 * b->a [label="COMMANDS list=HELP,LOG,PING,READY,..."];
 *
 * #
 * # Start a child at a specified time
 * #
 * b=>b [label="wakeup timer timed out"];
 * b->e [label="lock website"];
 * b->c [label="start child"];
 * c->b [label="child died"];
 * b->e [label="unlock website"];
 *
 * #
 * # Start a child periodically
 * #
 * b=>b [label="tick timer timed out"];
 * b->e [label="lock website"];
 * b->c [label="start child"];
 * c->b [label="child died"];
 * b->e [label="unlock website"];
 *
 * #
 * # When the child dies
 * #
 * b=>b [label="another run is already schedule"];
 * b->e [label="lock website"];
 * b->c [label="start child"];
 * c->b [label="child died"];
 * b->e [label="unlock website"];
 *
 * #
 * # PING is received
 * #
 * a->b [label="PING sent to backend"];
 * b=>b [label="register request in database"];
 * b->e [label="lock website"];
 * b->c [label="start child"];
 * c->b [label="child died"];
 * b->e [label="unlock website"];
 * \endmsc
 *
 * \note
 * If a child is already running, then it does not get started a
 * second time. This is quite important since if you have a large
 * number of websites (say 1,000) then you could otherwise get that
 * many processes running simultaneously... Instead we run at most
 * one child per instance of the snapbackend process. You may, however,
 * have one instance per computer in your cluster so as to alleviate
 * the load through multi-processing.
 *
 * \sa snap_child
 */



namespace plugins
{
extern QString g_next_register_name;
extern QString g_next_register_filename;
}

namespace
{


/** \brief The communicator.
 *
 * This is the pointer to the communicator. Since the communicator is
 * a singleton, we have only one and thus can keep a copy here.
 */
snap_communicator::pointer_t        g_communicator;


/** \brief Handle the SIGINT that is expected to stop the server.
 *
 * This class is an implementation of the snap_signal that listens
 * on the SIGINT.
 */
class backend_interrupt
        : public snap::snap_communicator::snap_signal
{
public:
    typedef std::shared_ptr<backend_interrupt>  pointer_t;

                        backend_interrupt(snap_backend * b);
    virtual             ~backend_interrupt() override {}

    // snap::snap_communicator::snap_signal implementation
    virtual void        process_signal() override;

private:
    snap_backend *      f_snap_backend = nullptr;
};


backend_interrupt::pointer_t       g_interrupt;


/** \brief The interrupt initialization.
 *
 * The interrupt uses the signalfd() function to obtain a way to listen on
 * incoming Unix signals.
 *
 * Specifically, it listens on the SIGINT signal, which is the equivalent
 * to the Ctrl-C.
 *
 * \param[in] b  The server we are listening for.
 */
backend_interrupt::backend_interrupt(snap_backend * b)
    : snap_signal(SIGINT)
    , f_snap_backend(b)
{
    unblock_signal_on_destruction();
    set_name("snap_backend interrupt");
}


/** \brief Call the stop function of the snaplock object.
 *
 * When this function is called, the signal was received and thus we are
 * asked to quit as soon as possible.
 */
void backend_interrupt::process_signal()
{
    // we simulate the STOP, so pass 'false' (i.e. not quitting)
    //
    f_snap_backend->stop(false);
}





/** \brief Capture children death.
 *
 * This class used used to create a connection on started that allows
 * us to know when a child dies. Whenever that happens, we get a call
 * to the process_signal() callback.
 */
class signal_child_death
        : public snap_communicator::snap_signal
{
public:
    typedef std::shared_ptr<signal_child_death>     pointer_t;

                            signal_child_death(snap_backend * sb);

    // snap_communicator::snap_signal implementation
    virtual void            process_signal();

private:
    // TBD: should this be a weak pointer?
    snap_backend *          f_snap_backend = nullptr;
};


signal_child_death::pointer_t       g_signal_child_death;


/** \brief Initialize the child death signal.
 *
 * The function initializes the snap_signal to listen on the SIGCHLD
 * Unix signal. It also saves the pointer \p s to the server so
 * it can be used to call various functions in the server whenever
 * the signal occurs.
 *
 * \param[in] s  The server pointer.
 */
signal_child_death::signal_child_death(snap_backend * sb)
    : snap_signal(SIGCHLD)
    , f_snap_backend(sb)
{
    set_name("snap_backend signal_child_death");
}


/** \brief Callback called each time the SIGCHLD signal occurs.
 *
 * This function gets called each time a child dies.
 *
 * The function checks all the children and removes zombies.
 */
void signal_child_death::process_signal()
{
    // check all our children and remove zombies
    //
    f_snap_backend->capture_zombies(get_child_pid());
}



/** \brief Time the CASSANDRASTATUS message.
 *
 * When the snapbackend process is started and it never receives a replies
 * to its CASSANDRASTATUS, it has to time out. This timer is used for that
 * purpose.
 */
class cassandra_timer
        : public snap::snap_communicator::snap_timer
{
public:
    typedef std::shared_ptr<cassandra_timer>        pointer_t;

    static int64_t const        MAX_START_INTERVAL = 60LL * 1000000LL; // 1 minute in microseconds

                                cassandra_timer(snap_backend * sb);

    // snap::snap_communicator::snap_timer implementation
    virtual void                process_timeout();

private:
    snap_backend *              f_snap_backend = nullptr;
};


/** \brief The reconnect timer.
 *
 * We create one cassandra timer. It is saved in this variable if needed.
 */
cassandra_timer::pointer_t             g_cassandra_timer;


/** \brief Initializes the timer with a pointer to the snap backend.
 *
 * The constructor saves the pointer of the snap_backend object so
 * it can later be used when the process times out.
 *
 * The timer is setup to trigger after 30 seconds when enabled.
 *
 * \param[in] sb  A pointer to the snap_backend object.
 */
cassandra_timer::cassandra_timer(snap_backend * sb)
    : snap_timer(-1)
    , f_snap_backend(sb)
{
    set_name("snap_backend cassandra_timer");
}


/** \brief The timeout happened.
 *
 * This function gets called after a few seconds whenever it is enabled,
 * it will allow us to exit the snapbackend tool if it never connects
 * to Cassandra.
 *
 * The function calls the stop() function of the snap_backend class.
 */
void cassandra_timer::process_timeout()
{
    f_snap_backend->stop(false);
}




/** \brief The timer used when a connection to Cassandra fails.
 *
 * When we receive the CASSANDRAREADY event, the connection is likely to
 * work. However, over time, while reading data in various loops, we may
 * end up with an exception and that stops the connection right there.
 * In other words, on return the f_cassandra pointer will be reset back
 * to a null pointer.
 *
 * To allow for a little bit of time before reconnecting, we use this
 * timer. Because in most cases this happens when Cassandra is rather
 * overloaded so trying to reconnect immediately at this stage is not
 * a good plan.
 *
 * At this time we setup the timer to 30 seconds. A snapbackend child
 * continues to be fully functional if its connection did not die, so
 * a longer pause should not be much of a problem.
 *
 * This will be much faster than the 5min to 1h auto-restart delay in
 * the various snapbackend service files.
 */
class reconnect_timer
        : public snap::snap_communicator::snap_timer
{
public:
    typedef std::shared_ptr<reconnect_timer>        pointer_t;

                                reconnect_timer(snap_backend * sb);

    // snap::snap_communicator::snap_timer implementation
    virtual void                process_timeout();

private:
    snap_backend *              f_snap_backend = nullptr;
};


/** \brief The reconnect timer.
 *
 * We create one reconnect timer. It is saved in this variable if needed.
 */
reconnect_timer::pointer_t             g_reconnect_timer;


/** \brief Initializes the timer with a pointer to the snap backend.
 *
 * The constructor saves the pointer of the snap_backend object so
 * it can later be used when the process times out.
 *
 * The timer is setup to trigger after 30 seconds when enabled.
 *
 * \param[in] sb  A pointer to the snap_backend object.
 */
reconnect_timer::reconnect_timer(snap_backend * sb)
    : snap_timer(-1)
    , f_snap_backend(sb)
{
    set_name("snap_backend reconnect_timer");
}


/** \brief The timeout happened.
 *
 * This function gets called after a few seconds whenever it is enabled,
 * it will allow us to reconnect to the Cassandra database.
 */
void reconnect_timer::process_timeout()
{
    f_snap_backend->process_reconnect();
}




/** \brief The timer to produce ticks once every five minutes.
 *
 * This timer makes sure that every website is re-added to the
 * backend table once every five minutes. Whether the backend
 * needs to be run against that website is ignored in this case.
 */
class tick_timer
        : public snap::snap_communicator::snap_timer
{
public:
    typedef std::shared_ptr<tick_timer>        pointer_t;

    static int64_t const        MAX_START_INTERVAL = 5LL * 60LL * 1000000LL; // 5 minutes in microseconds

                                tick_timer( snap_backend * sb );

    void                        configure(QDomElement e, QString const & binary_path, bool const debug);

    // snap::snap_communicator::snap_timer implementation
    virtual void                process_timeout();

private:
    snap_backend *              f_snap_backend = nullptr;
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
tick_timer::tick_timer(snap_backend * sb)
    : snap_timer(MAX_START_INTERVAL)
    , f_snap_backend(sb)
{
    set_name("snap_backend tick_timer");

    // prevent tick_timer() from starting, we want to Cassandra
    // connection to be ready first
    //
    set_enable(false);

    // make sure it starts right away once we receive the CASSANDRAREADY
    // message
    //
    // we do not want to use snap_timer(0) because otherwise we will not
    // get ongoing ticks as expected
    //
    set_timeout_date(snap_communicator::get_current_date());
}


/** \brief The timeout happened.
 *
 * This function gets called once every five minutes, which is used to reset
 * the backend table so the backend processes are run against every websites
 * over and over again.
 */
void tick_timer::process_timeout()
{
    f_snap_backend->process_tick();
}




/** \brief The timer to produce wake up calls once in a while.
 *
 * This timer is used to wake us once in a while as determined by other
 * features. The date feature is always used on this timer (i.e. wake up
 * the process at a specific date and time in microseconds.)
 */
class wakeup_timer
        : public snap::snap_communicator::snap_timer
{
public:
    typedef std::shared_ptr<wakeup_timer>        pointer_t;

                                wakeup_timer( snap_backend * sb );

    void                        configure(QDomElement e, QString const & binary_path, bool const debug);

    // snap::snap_communicator::snap_timer implementation
    virtual void                process_timeout();

private:
    snap_backend *              f_snap_backend = nullptr;
};


/** \brief The wakeup timer.
 *
 * We create one wakeup timer. It is saved in this variable if needed.
 */
wakeup_timer::pointer_t             g_wakeup_timer;


/** \brief Initializes the timer with a pointer to the snap backend.
 *
 * The constructor saves the pointer of the snap_backend object so
 * it can later be used when the process times out.
 *
 * By default the timer is "off" meaning that it will not trigger
 * a process_timeout() call until you turn it on.
 *
 * \param[in] sb  A pointer to the snap_backend object.
 */
wakeup_timer::wakeup_timer(snap_backend * sb)
    : snap_timer(-1)
    , f_snap_backend(sb)
{
    set_name("snap_backend wakeup_timer");
}


/** \brief The wake up timer timed out.
 *
 * The wake up timer is used to know when we can start another child.
 *
 * Whenever the current child dies, we check when the next child should
 * be started. If the backend table is empty, then the wake up timer is
 * not set and nothing happens. However, when the backend table has an
 * entry, we get the first one and use that date and the next trigger
 * (if the trigger is now or in the past, then it is not used, we
 * directly create the next child instance.)
 *
 * The messenger may receive a PING in between in which case the timer
 * may be reset to a different date and time at which to wake up.
 */
void wakeup_timer::process_timeout()
{
    f_snap_backend->process_timeout();
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

                        messenger(snap_backend * sb, QString const & action, std::string const & addr, int port);

    // snap::snap_communicator::snap_tcp_client_permanent_message_connection implementation
    virtual void        process_message(snap::snap_communicator_message const & message);
    virtual void        process_connection_failed(std::string const & error_message);
    virtual void        process_connected();

private:
    // this is owned by a server function so no need for a smart pointer
    snap_backend *      f_snap_backend = nullptr;
    QString             f_action;
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
 * \param[in] sb  The snap backend server we are listening for.
 * \param[in] action  The action for which this messenger is created, it is
 *                    sent to the snapcommunicator server when we REGISTER.
 * \param[in] addr  The address to connect to. Most often it is 127.0.0.1.
 * \param[in] port  The port to listen on (4040).
 */
messenger::messenger(snap_backend * sb, QString const & action, std::string const & addr, int port)
    : snap_tcp_client_permanent_message_connection(
                addr,
                port,
                tcp_client_server::bio_client::mode_t::MODE_PLAIN,
                snap_communicator::snap_tcp_client_permanent_message_connection::DEFAULT_PAUSE_BEFORE_RECONNECTING,
                false) // do not use a separate thread, we do many fork()'s
    , f_snap_backend(sb)
    , f_action(action)
{
    set_name("snap_backend messenger");
}


/** \brief Pass messages to the Snap Backend.
 *
 * This callback is called whenever a message is received from
 * Snap! Communicator. The message is immediately forwarded to the
 * snap_backend object which is expected to process it and reply
 * if required.
 *
 * \param[in] message  The message we just received.
 */
void messenger::process_message(snap::snap_communicator_message const & message)
{
    f_snap_backend->process_message(message);
}


/** \brief The messenger could not connect to snapcommunicator.
 *
 * This function is called whenever the messengers fails to
 * connect to the snapcommunicator server. This could be
 * because snapcommunicator is not running or because the
 * information for the snapbackend is wrong...
 *
 * With snapinit the snapcommunicator should always already
 * be running so this error should not happen once everything
 * is properly setup.
 *
 * \param[in] error_message  An error message.
 */
void messenger::process_connection_failed(std::string const & error_message)
{
    SNAP_LOG_ERROR("connection to snapcommunicator failed (")(error_message)(")");

    // also call the default function, just in case
    snap_tcp_client_permanent_message_connection::process_connection_failed(error_message);

    f_snap_backend->process_connection_failed();
}


/** \brief The connection was established with Snap! Communicator.
 *
 * Whenever the connection is establied with the Snap! Communicator,
 * this callback function is called.
 *
 * The messenger reacts by REGISTERing the snap_backend with the Snap!
 * Communicator. The name of the backend is taken from the action
 * it was called with.
 */
void messenger::process_connected()
{
    snap_tcp_client_permanent_message_connection::process_connected();

    QString action(f_action);
    int const pos(action.indexOf(':'));
    if(pos >= 0)
    {
        action = action.mid(pos + 2);
    }

    snap::snap_communicator_message register_backend;
    register_backend.set_command("REGISTER");
    register_backend.add_parameter("service", action);
    register_backend.add_parameter("version", snap::snap_communicator::VERSION);
    send_message(register_backend);
}



/** \brief A connection between the parent process and child.
 *
 * Whenever we fork() we want to keep a live connection between the
 * parent and the child. We use a Unix pair of socket for the purpose
 * which is implemented with the snap_pipe_message_connection class.
 */
class child_connection
        : public snap_communicator::snap_pipe_message_connection
{
public:
    typedef std::shared_ptr<child_connection>   pointer_t;

                                child_connection(snap_backend * sb, QtCassandra::QCassandraContext::pointer_t & context);
    virtual                     ~child_connection() override {}

    bool                        lock(QString const & uri);
    void                        unlock();

    // snap::snap_communicator::snap_pipe_message_connection implementation
    virtual void                process_message(snap_communicator_message const & message) override;

private:
    snap_backend *                              f_snap_backend = nullptr;
    QtCassandra::QCassandraContext::pointer_t   f_context;
    snap_lock::pointer_t                        f_lock;
};


/** \brief The currently active child connection.
 *
 * Whenever we are ready to fork() a child to run a backend, we create
 * one of these child_connection object and save it in this variable.
 *
 * Once the child process dies, we remove the g_child_connection from
 * the g_communicator. When we create a new child (fork() again), we
 * create a new child connection since the old one will not be valid
 * anymore.
 */
child_connection::pointer_t g_child_connection;


/** \brief Initialize the child_connection object.
 *
 * This function is used to finish creating a child connection.
 *
 * \param[in] sb  The snap backend pointer.
 * \param[in] uri  The URI the child process is going to work on.
 */
child_connection::child_connection(snap_backend * sb, QtCassandra::QCassandraContext::pointer_t & context)
    : f_snap_backend(sb)
    , f_context(context)
    //, f_lock(nullptr)
{
    set_name("child connection");
}


/** \brief Lock the child website.
 *
 * While working on a certain website we want to lock it so
 * only one computer backend can work on that specific website
 * at a time.
 *
 * The URI of the website was specified on the constructor.
 *
 * \todo
 * At this time, because many of the snapcommunicator variables
 * are global variables, the child is affected (i.e. when it
 * calls exit() it wants to clean those global variables and
 * we may get some "weird" side effects--one of which is the
 * f_lock, since it sends the UNLOCK command to the snaplock
 * tool twice as a result.) We may want to look into completely
 * removing the use of global variables. I have done so in a
 * couple of tools (under src/) and it worked nicely.
 *
 * \return true if the lock succeeded.
 */
bool child_connection::lock(QString const & uri)
{
    // if the lock fails, it returns false; note that we want to get a 4h
    // lock, but we wait at most the default (5 sec.) to obtain the lock
    //
    f_lock.reset(new snap_lock(QString()));
    return f_lock->lock(QString("*backend* %1").arg(uri), 4 * 60 * 60);
}


/** \brief This function unlocks the child connection.
 *
 * This function is called whenever the child becomes a zombie.
 * Although the destructor would have a similar effect, we cannot
 * hope to get the destructor in time (i.e. a copy of the connection
 * shared pointer is held by the snapcommunicator and it will be
 * until we return from all the message processing functions.)
 */
void child_connection::unlock()
{
    f_lock.reset();
}


/** \brief The child sent us a message, process it.
 *
 * This callback is called whenever the child sends us a message.
 * In most cases this is to tell us that it is done with a date
 * when it wants to be awaken again.
 *
 * \note
 * At this point I do not foresee any reason for the child to
 * send us any messages. The connection is really for the parent
 * process (snap_backend) to be able to send a STOP message to
 * the child.
 *
 * \param[in] message  The message we just received.
 */
void child_connection::process_message(snap_communicator_message const & message)
{
    f_snap_backend->process_child_message(message);
}




} // no name namespace



snap_backend::snap_backend( server_pointer_t s )
    : snap_child(s)
    , f_parent_pid(getpid())
{
}


/** \brief Clean up the snap backend.
 *
 * This function is used to do some clean up of the snap backend environment.
 */
snap_backend::~snap_backend()
{
    // TBD: should we make sure to delete all the global objects?
    //      as far as I know they should already be cleared up since
    //      we cannot exit the run() loop unless there were...
}


/** \brief Check whether the STOP signal was received.
 *
 * This function checks whether the parent snap_backend process
 * sent us a STOP message. If so the function returns true and
 * you are expected to return from your backend as soon as possible.
 *
 * \todo
 * We may eventually want to receive other messages, not just STOP.
 * However, at this point I do not see the need. If we want other
 * messages, we should transform this function in a pop_message()
 * which returns true if a message is indeed popped. Once the
 * STOP is received, only the STOP can be popped and it will be
 * popped as many times as the function gets called.
 *
 * \return true if the backend thread received the STOP signal.
 */
bool snap_backend::stop_received() const
{
    if(getpid() == f_parent_pid)
    {
        throw snap_logic_exception("snap_backend::get_error(): Function called from the parent process. It can only be used from the child.");
    }

    // make sure to process any pending messages
    //
    // Note: we definitively are in the child process, so the
    //       g_child_connection exists
    //
    g_child_connection->process_read();

    return f_stop_received;
}


/** \brief Add a website URI to process on 'date'.
 *
 * This function is used to add the URI of a website that needs to be
 * processed on a certain date. The URIs are first organized by actions
 * and then by date.
 *
 * \warning
 * The action MUST include the namespace. So if you call from a plugin
 * named "list", for example, the action name must start with "list::"
 * as in "list::pagelist". Otherwise it will not match the f_action
 * parameter used in other places and the data will be ignored.
 *
 * \param[in] action  The action concerned by this.
 * \param[in] date  The date when this action should next be applied.
 * \param[in] website_uri  The URI of the website on which the \p action should
 *                         be applied on \p date.
 */
bool snap_backend::add_uri_for_processing(QString const & action, int64_t date, QString const & website_uri)
{
    try
    {
        QString const action_reference(QString("*%1*").arg(action));
        int64_t const previous_entry(f_backend_table->row(action_reference)->cell(website_uri)->value().safeInt64Value(0, -1));
        if(previous_entry != -1)
        {
            QByteArray column_key;
            QtCassandra::appendInt64Value(column_key, previous_entry);

            // is entry already outdated and thus still effective?
            //
            if(previous_entry <= date)
            {
                // make sure there is indeed an entry though because bugs
                // creep in and the other cell may not be in place anymore
                // and a "return" here would prevent further work on any
                // backend processing
                //
                if(f_backend_table->row(action)->exists(column_key))
                {
                    // we already have that entry at the same date or earlier
                    //
                    return true;
                }
            }

            // make sure we drop the other reference to avoid
            // (generally useless) duplicates
            //
            f_backend_table->row(action)->dropCell(column_key);
        }

        QByteArray date_key;
        QtCassandra::appendInt64Value(date_key, date);
        f_backend_table->row(action)->cell(date_key)->setValue(website_uri);

        // save a reference so we can drop the entry as required
        //
        f_backend_table->row(action_reference)->cell(website_uri)->setValue(date);

        return true;
    }
    catch(std::exception const & e)
    {
        SNAP_LOG_WARNING("Got an exception while adding a URI for processing: ")(e.what());

        // pause for 30 seconds, then we will try again
        //
        request_cassandra_status();

        return false;
    }
}


/** \brief Remove a URI once it was processed.
 *
 * This function removes the specified website URI from the backend table.
 * The function makes use of the reference we save there.
 *
 * The function is called when the child process the specified URI dies.
 * Also, if the website is not ready when we are, we remove the URI from
 * the list since there is no need to have it there. It will be re-added
 * when we get a PING or within five minutes.
 *
 * \param[in] action  The action where a website URI is to be removed.
 * \param[in] key  The key to drop.
 * \param[in] website_uri  The URI to be removed.
 *
 * \return true if the removal worked as expected; false if we lose the
 *         connection to the database in the process.
 */
bool snap_backend::remove_processed_uri(QString const & action, QByteArray const & key, QString const & website_uri)
{
    try
    {
        QString const action_reference(QString("*%1*").arg(action));
        int64_t const previous_entry(f_backend_table->row(action_reference)->cell(website_uri)->value().safeInt64Value(0, -1));
        if(previous_entry != -1)
        {
            // drop the actual entry and the reference
            QByteArray column_key;
            QtCassandra::appendInt64Value(column_key, previous_entry);
            f_backend_table->row(action)->dropCell(column_key);
        }

        // just in case, alway sforce a drop on this cell (it should not
        // exist if previous_entry was -1)
        //
        f_backend_table->row(action_reference)->dropCell(website_uri);

        // also remove the processed entry (which is the one we really use
        // to find what has to be worked on)
        //
        f_backend_table->row(action)->dropCell(key);

        return true;
    }
    catch(std::exception const & e)
    {
        SNAP_LOG_WARNING("Got an exception while adding a URI for processing: ")(e.what());

        // pause for 30 seconds, then we will try again
        //
        request_cassandra_status();

        return false;
    }
}


/** \brief Execute the backend processes after initialization.
 *
 * This function is somewhat similar to the process() function. It is used
 * to ready the server and then run the backend processes by sending a
 * signal.
 */
void snap_backend::run_backend()
{
    // TBD: the calling main() function already has a try/catch, we could
    //      remove this one?
    //
    try
    {
        process_action();

        SNAP_LOG_INFO("snap_backend::run_backend(): exiting normally.");

        // return normally if no exception occurred
        //
        return;
    }
    catch( snap_exception const & e )
    {
        SNAP_LOG_FATAL("snap_backend::run_backend(): snap_exception caught: ")(e.what());
    }
    catch( std::exception const & e )
    {
        SNAP_LOG_FATAL("snap_backend::run_backend(): std::exception caught: ")(e.what());
    }
    catch( ... )
    {
        SNAP_LOG_FATAL("snap_backend::run_backend(): unknown exception caught!");
    }
    exit(1);
    NOTREACHED();
}


void snap_backend::process_action()
{
    {
        auto p_server = f_server.lock();
        if(!p_server)
        {
            throw snap_child_exception_no_server("snap_backend::process_action(): The p_server weak pointer could not be locked");
        }

        init_start_date();

        // somewhat fake being a child (we are not here)
        f_is_child = true;
        f_child_pid = getpid();
        f_client.reset();

        // define a User-Agent for all backends
        //
        // TBD: should that be a parameter in the .conf file?
        //
        f_env[snap::get_name(name_t::SNAP_NAME_CORE_HTTP_USER_AGENT)] = "Snap! Backend (" SNAPWEBSITES_VERSION_STRING ")";

        // define the action and whether it is a CRON action
        //
        f_action = p_server->get_parameter("__BACKEND_ACTION");
        if(f_action.isEmpty())
        {
            f_action = p_server->get_parameter("__BACKEND_CRON_ACTION");
            if(f_action.isEmpty())
            {
                // the default action is "snapbackend", which is not a CRON
                // action and runs the backend_process() signal
                // (see plugins/content/backend.cpp where we do the call)
                // It is part of the content plugin to avoid having to
                // carry a special case all around
                //
                f_action = QString("content::%1").arg(get_name(name_t::SNAP_NAME_CORE_SNAPBACKEND));
            }
            else
            {
                f_cron_action = true;
            }
        }

        // get the URI, since it does not change over time within one
        // run, we save it in a variable member
        //
        f_website = p_server->get_parameter("__BACKEND_URI");

        // check whether this action should use a global lock when running
        // (this is for those actions that cannot be run simultaneously
        // against more than one website at a time; i.e. the sendmail
        // backend is website agnostic, for example.)
        //
        f_global_lock = !p_server->get_parameter("GLOBAL_LOCK").isEmpty();

        // get the snap_communicator singleton
        //
        g_communicator = snap_communicator::instance();

        // create a TCP messenger connected to the Snap! Communicator server
        //
        {
            QString tcp_addr("127.0.0.1");
            int tcp_port(4040);
            snap_config parameters("snapcommunicator");
            tcp_client_server::get_addr_port(QString(parameters["local_listen"]), tcp_addr, tcp_port, "tcp");
            g_messenger.reset(new messenger(this, f_action, tcp_addr.toUtf8().data(), tcp_port));
            g_communicator->add_connection(g_messenger);

            p_server->configure_messenger_logging( g_messenger );
        }
    }

    g_interrupt.reset(new backend_interrupt(this));
    g_communicator->add_connection(g_interrupt);

    // create a Cassandra timer; we use it in the "READY" and snapbackend
    // is not called with a specific action (i.e. a CRON backend); if the
    // timer times out, then we force an exit with a failure status
    //
    {
        g_cassandra_timer.reset(new cassandra_timer(this));
        g_communicator->add_connection(g_cassandra_timer);
    }

    // create a reconnect timer; if we lose the connection to the Cassandra
    // cluster (more precisely, the snapdbproxy local daemon which cuts us
    // off on a throw by the libQtCassandra library), then we want to
    // reconnect after a little while which is handled by this timer;
    // note that the reconnect is actually sending a new CASSANDRASTATUS
    // message and the rest is done as before
    //
    {
        g_reconnect_timer.reset(new reconnect_timer(this));
        g_communicator->add_connection(g_reconnect_timer);
    }

    // create a tick timer; every five minutes we add work to our
    // backend table which is in turn processed whenever the wake up
    // timer happens
    //
    {
        g_tick_timer.reset(new tick_timer(this));
        g_communicator->add_connection(g_tick_timer);
    }

    // create a wake up timer; whenever we have work to do, this timer
    // is used to run the next entry at its specified date and time
    //
    {
        g_wakeup_timer.reset(new wakeup_timer(this));
        g_communicator->add_connection(g_wakeup_timer);
    }

    // we want to immediately be signaled whenever a child process dies
    // so we can move to work on the next one
    //
    {
        g_signal_child_death.reset(new signal_child_death(this));
        g_communicator->add_connection(g_signal_child_death);
    }

    SNAP_LOG_INFO("------------------------------------ CRON backend ")(f_action)(" started.");

    // start our event loop
    //
//SNAP_LOG_WARNING("entering run() loop with action: ")(f_action);
    g_communicator->run();
}


/** \brief Called once on startup and then once every 5 minutes.
 *
 * This function is called once immediately (we set a timeout date
 * of 'now' on initialization) and then once every five minutes.
 * This is used for CRON actions where the backend process needs
 * to be repeated once in a while to ensure proper functioning
 * of the websites as a whole.
 *
 * \note
 * The direct backend processing (snapbackend) and specific website
 * backend processing (snapbackend http://snapwebsites.org/) are
 * also directed here as both of these are also processed in a
 * similar way.
 */
void snap_backend::process_tick()
{
    // STOP received?
    //
    if(f_stop_received)
    {
        return;
    }

    // if the user gave us a specific website to process, we cannot add
    // the URI to the backend table
    //
    if( f_website.isEmpty() )
    {
        // if the "sites" table does not even exists, then either wait
        // or quit immediately
        //
        if(!is_ready(""))
        {
            if(!f_cron_action)
            {
                // one reason for is_ready() to not return true is if snaplock
                // is not up yet
                //
                if(!f_snaplock)
                {
                    SNAP_LOG_FATAL("snap_backend::process_tick(): The Snap! Lock daemon is not available.");
                    exit(1);
                    snap::NOTREACHED();
                }

                // if we are connected to cassandra but are not marked ready
                // that means the "sites" table is not yet defined
                //
                if(f_cassandra)
                {
                    SNAP_LOG_FATAL("snap_backend::process_tick(): The \"sites\" table does not even exist, we cannot yet run a backend action.");
                    exit(1);
                    snap::NOTREACHED();
                }

                // The CRON behavior ends up here all the time because we
                // now wait for the CASSANDRAREADY event before the
                // is_ready() function returns true... so we have to
                // wait a bit before we exit with a fatal error
                //
                ++f_not_ready_counter;
                if(f_not_ready_counter > 3)  // 3 represent a total of 30 seconds of wait at this time (see the 10 second wait below)
                {
                    SNAP_LOG_FATAL("snap_backend::process_tick(): We could not connect to snapdbproxy within 30 seconds.");
                    exit(1);
                    snap::NOTREACHED();
                }
            }

            if(!f_cassandra)
            {
                SNAP_LOG_WARNING("snap_backend::process_tick(): not yet connected to snapdbproxy.");
            }
            else if(f_emit_warning_about_missing_sites)
            {
                f_emit_warning_about_missing_sites = false;

                // the whole table is still missing after 5 minutes!
                // in this case it is an error instead of a fatal error
                SNAP_LOG_WARNING("snap_backend::process_tick(): The \"sites\" table is still empty or nonexistent! Waiting before starting the \"")(f_action)("\" backend processing (a CRON action).");
            }

            // the website is not ready, wait another 10 seconds and try
            // again (with the new snapinit, not yet implemented, we should
            // not need this one, although keeping it is certainly a
            // nice extra security feature...)
            //
            // here we use the timeout date to not have to change the
            // ticking clock
            //
            // TBD: should we instead slide the ticking clock?
            //
            g_tick_timer->set_timeout_date(snap_communicator::get_current_date() + 10LL * 1000000LL);
            return;
        }

        // make sure we reset the "not ready counter" once ready
        //
        f_not_ready_counter = 0;

        try
        {
            // if a site exists then it has a "core::last_updated" entry
            //
            f_sites_table->clearCache(); // just in case, make sure we do not have a query still laying around
            auto column_predicate(std::make_shared<QtCassandra::QCassandraCellKeyPredicate>());
            column_predicate->setCellKey(get_name(name_t::SNAP_NAME_CORE_LAST_UPDATED));
            auto row_predicate(std::make_shared<QtCassandra::QCassandraRowPredicate>());
            row_predicate->setCellPredicate(column_predicate);
            for(;;)
            {
                // WARNING: at this point the f_sites_table may be NULL
                //          because we call add_uri_for_processing()
                //          and that may throw and call
                //          request_cassandra_status() which clears
                //          everything (notice that we have two for loops
                //          and the inner loop breaks on error instead
                //          of "goto exit" or something of the sort...
                //          because the add_uri_for_processing() may
                //          return false for other reasons than a throw.)
                //
                if(f_sites_table == nullptr
                || f_sites_table->readRows(row_predicate) == 0)
                {
                    // no more websites to process
                    break;
                }

                // got some websites
                //
                QtCassandra::QCassandraRows const rows(f_sites_table->rows());
                for(QtCassandra::QCassandraRows::const_iterator it(rows.begin());
                                                                it != rows.end();
                                                                ++it)
                {
                    QString const key(QString::fromUtf8(it.key().data()));
                    if(!add_uri_for_processing(f_action, get_current_date(), key))
                    {
                        // this happens if an error occurs while working with
                        // the database; in that case we cannot go any further
                        //
                        break;
                    }
                }
            }
        }
        catch(std::exception const & e)
        {
            SNAP_LOG_WARNING("Got an exception while adding a URI for processing: ")(e.what());

            // pause for 30 seconds, then we will try again
            //
            request_cassandra_status();
        }
    }

    // if no child is currently running, wake up the messenger ASAP
    //
    if(!g_child_connection)
    {
#ifdef DEBUG
        SNAP_LOG_TRACE("Immediately tick the wakeup_timer from the last tick timeout.");
#endif
        g_wakeup_timer->set_timeout_date(snap_communicator::get_current_date());
    }
}


/** \brief Timeout is called whenever a child process needs to be started.
 *
 * This function is called when the Snap! Communicator main messenger
 * connection times out. This generally means the child process needs
 * to be started with a URI.
 *
 * \return true if a new backend was started on this call.
 */
bool snap_backend::process_timeout()
{
    // STOP received?
    // Child still running? (our timer should never be on when we have
    // a child running, but it is way safer this way)
    //
    if(f_stop_received
    || g_child_connection != nullptr)
    {
        return false;
    }

    if( f_website.isEmpty() )
    {
        // if we reach here f_sites_tables and f_backend_table should
        // both be defined, but just in case since we have rather lose
        // event agreggations...
        //
        if(f_sites_table == nullptr
        || f_backend_table == nullptr)
        {
            return false;
        }

        // the connection to snapdbproxy may be severed while attempting
        // to read more data; here we do a try catch so we can have a
        // pause and attempt to reconnect later (30 seconds later)
        //
        // See SNAP-529 for details
        //
        try
        {
            // if the user did not give us a specific website to work on
            // we want to check for the next entry in our backend table
            //
            QtCassandra::QCassandraRow::pointer_t row(f_backend_table->row(f_action));
            row->clearCache(); // just in case, make sure we do not have a query laying around
            auto column_predicate(std::make_shared<QtCassandra::QCassandraCellRangePredicate>());
            column_predicate->setCount(1); // read only the first row -- WARNING: if you increase that number you MUST add a sub-loop
            column_predicate->setIndex(); // behave like an index
            for(;;)
            {
                row->readCells(column_predicate);
                QtCassandra::QCassandraCells const cells(row->cells());
                if(cells.isEmpty())
                {
                    // it looks like we are done
                    break;
                }

                // check whether the time is past, if it is in more than 10ms
                // then we want to go to sleep again, otherwise we start
                // processing that website now
                //
                QByteArray const key(cells.begin().key());
                int64_t const time_limit(QtCassandra::safeInt64Value(key, 0, 0));
                if(time_limit <= get_current_date() + 10000LL)
                {
                    // note how we remove the URI from the backend table before
                    // we processed it: this is much safer, if that website
                    // (currently) has a problem, then we just end up skipping
                    // it and we will just try again later.
                    //
                    QtCassandra::QCassandraCell::pointer_t cell(*cells.begin());
                    QString const website_uri(cell->value().stringValue());
                    remove_processed_uri(f_action, key, website_uri);
                    if(process_backend_uri(website_uri))
                    {
                        return true;
                    }
                }
                else
                {
                    // we found one that needs to be started in the future
                    // we can exit the loop now after we stamped the timer
                    // for when we want to wake up next
                    //
                    g_wakeup_timer->set_timeout_date(time_limit);
                    break;
                }
            }
        }
        catch(std::exception const & e)
        {
            SNAP_LOG_WARNING("Got an exception while searching for the next website to work on: ")(e.what());

            // pause for 30 seconds, then we will try again
            //
            request_cassandra_status();
        }
    }
    else
    {
        process_backend_uri(f_website);
        return true;
    }

    return false;
}


/** \brief Process a message received from Snap! Communicator.
 *
 * This function gets called whenever the Snap! Communicator sends
 * us a message. This includes the READY and HELP commands, although
 * the most important one is certainly the STOP command used to request
 * this process to STOP as soon as possible.
 *
 * \param[in] message  The message we just received.
 */
void snap_backend::process_message(snap::snap_communicator_message const & message)
{
    QString const command(message.get_command());

    // STATUS is sent too many times, so do not trace them all...
    if(command != "STATUS")
    {
        SNAP_LOG_TRACE("received messenger message [")(message.to_message())("] for ")(f_action);
    }

    if(command == "PING")
    {
        // only CRON actions accept PING messages
        //
        if(!f_cron_action)
        {
            SNAP_LOG_ERROR("PING sent to a backend which is not a CRON action. PING will be ignored.");
            return;
        }

        // someone is asking us to restart the child for a specific URI
        //
        QString uri;
        if(message.has_parameter("uri"))
        {
            uri = message.get_parameter("uri");
        }
        if(uri.isEmpty())
        {
            SNAP_LOG_ERROR("PING sent to \"")(f_action)("\" backend without a URI. PING will be ignored.");
            return;
        }

        if(f_website.isEmpty()
        && is_ready(""))
        {
            if(add_uri_for_processing(f_action, get_current_date(), uri))
            {
                // if no child is currently running, wake up the messenger ASAP
                //
                if(g_child_connection == nullptr)
                {
#ifdef DEBUG
                    SNAP_LOG_TRACE("Run the child now since it was not running.");
#endif
                    g_wakeup_timer->set_timeout_date(snap_communicator::get_current_date());
                }
            }
        }
        else
        {
            f_pinged = true;
        }
        return;
    }

    if(command == "LOG")
    {
        // logrotate just rotated the logs, we have to reconfigure
        //
        SNAP_LOG_INFO("Logging reconfiguration.");
        logging::reconfigure();
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
        // Snap! Communicator received our REGISTER command
        //

        // if the user called snapbackend as the CRON action (i.e. no --action
        // specified on the command line) then we want to be able to time out
        // if snapdbproxy never sends us a CASSANDRAREADY message
        //
        if(!f_cron_action)
        {
            g_cassandra_timer->set_enable(true);
            g_cassandra_timer->set_timeout_date(snap_communicator::get_current_date() + cassandra_timer::MAX_START_INTERVAL);
        }

        // request snapdbproxy to send us a status signal about
        // Cassandra, after that one call, we will receive the
        // statuses just because we understand them.
        //
        process_reconnect(); // simulate a process_reconnect() timeout

        // request snapcommunicator to send us a STATUS message
        // about the current status of the snaplock service
        //
        snap::snap_communicator_message islockready_message;
        islockready_message.set_command("SERVICESTATUS");
        islockready_message.add_parameter("service", "snaplock");
        g_messenger->send_message(islockready_message);

        return;
    }

    if(command == "NOCASSANDRA")
    {
        // we lost Cassandra, disconnect from snapdbproxy until we
        // get CASSANDRAREADY again
        //
        f_auto_retry_cassandra = false;
        disconnect_cassandra();

        return;
    }

    if(command == "CASSANDRAREADY")
    {
        // cancel timeouts
        //
        if(!f_cron_action
        && g_cassandra_timer != nullptr)
        {
            g_cassandra_timer->set_enable(false);
        }
        if(g_reconnect_timer != nullptr)
        {
            // WARNING: this one we do not disable, instead we avoid the
            //          timeout by setting the date to -1
            //
            //          because the CASSANDRAREADY message can happen
            //          back to back, the timer can be started at this
            //          point; this can happens on startup when
            //          snapdbproxy broadcasts its CASSANDRAREADY message
            //          and the snapbackend process already sent a
            //          CASSANDRASTATUS message and the first CASSANDRAREADY
            //          message processing ended up with an error
            //
            g_reconnect_timer->set_timeout_date(-1);
        }

        // connect to Cassandra
        //
        // IMPORTANT NOTE: We are likely to receive two of these events
        //                 in a raw (i.e. the broadcast version and the
        //                 one specifically sent to this or that
        //                 specifically running backend)
        //
        f_auto_retry_cassandra = true;
        if(!connect_cassandra(false))
        {
            SNAP_LOG_WARNING("snapwebsites failed to connect to snapdbproxy (process_message())");

            disconnect_cassandra();
        }
        else if(g_tick_timer != nullptr && !f_stop_received)
        {
            // we are now ready to try running a child process
            //
            g_tick_timer->set_enable(true);
            g_tick_timer->set_timeout_date(snap_communicator::get_current_date());
        }

        return;
    }

    if(command == "STATUS")
    {
        if(message.get_parameter("service") == "snaplock")
        {
            // show the one STATUS that we manage here
            //
            SNAP_LOG_TRACE("received messenger message [")(message.to_message())("] for ")(f_action);

            f_snaplock = message.has_parameter("status")
                      && message.get_parameter("status") == "up";
        }
        // else -- ignore all others

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
        reply.add_parameter("list", "CASSANDRAREADY,HELP,LOG,NOCASSANDRA,PING,QUITTING,READY,STATUS,STOP,UNKNOWN");

        g_messenger->send_message(reply);
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
        g_messenger->send_message(reply);
    }

    return;
}


void snap_backend::disconnect_cassandra()
{
    // we are in control of the backend table
    //
    f_backend_table.reset();

    // we have our own f_sites_table variable
    // (TBD: maybe we could share the snap_child one? right now it is private.)
    //
    f_sites_table.reset();

    // the disconnect_cassandra() in snap_child already takes care of
    //
    //    f_sites_table
    //    f_context
    //    f_cassandra
    //
    snap_child::disconnect_cassandra();
}


void snap_backend::request_cassandra_status()
{
    SNAP_LOG_TRACE("requesting a CASSANDRASTATUS message because we got an error from our connection with snapdbproxy");

    // since we are going to disconnect, there is no need for this timer
    // so we can as well disable it; it will be re-enabled when we
    // receive the CASSANDRAREADY message
    //
    g_tick_timer->set_enable(false);

    // make sure the rest of the class knows that the current state
    // is viewed as "not good"--okay, just kidding, it is viewed as
    // not connected to Cassandra so we need to reset the pointers
    //
    // also we do not want an auto-retry in case the snapdbproxy is
    // really not available
    //
    f_auto_retry_cassandra = false;
    disconnect_cassandra();

    // whether the user wants to request a new CASSANDRASTATUS to be sent
    //
SNAP_LOG_WARNING("get ready for a reconnect in 30 seconds");
    int64_t const now(snap::snap_communicator::get_current_date());
    int64_t const reconnect_date(now + 30LL * 1000000LL);
    g_reconnect_timer->set_timeout_date(reconnect_date);
}


void snap_backend::process_reconnect()
{
    SNAP_LOG_TRACE("sending the CASSANDRASTATUS message");

    snap::snap_communicator_message isdbready_message;
    isdbready_message.set_command("CASSANDRASTATUS");
    isdbready_message.set_service("snapdbproxy");
    g_messenger->send_message(isdbready_message);
}


/** \brief Called whenever we receive the STOP command or equivalent.
 *
 * This function makes sure the snapbackend exits as quickly as
 * possible.
 *
 * \li Marks the messenger as done.
 * \li Disabled wake up and tick timers.
 * \li UNREGISTER from snapcommunicator.
 * \li STOP child if one is currently running.
 * \li Remove timers and child death connections if no child is running.
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
void snap_backend::stop(bool quitting)
{
    f_stop_received = true;

    // stop the timers immediately, although that will not prevent
    // one more call to their callbacks which thus still have to
    // check the f_stop_received flag
    //
    if(g_cassandra_timer != nullptr)
    {
        g_cassandra_timer->set_enable(false);
        g_cassandra_timer->set_timeout_date(-1);
    }
    if(g_reconnect_timer != nullptr)
    {
        g_reconnect_timer->set_enable(false);
        g_reconnect_timer->set_timeout_date(-1);
    }
    if(g_tick_timer != nullptr)
    {
        g_tick_timer->set_enable(false);
        g_tick_timer->set_timeout_delay(-1);
    }
    if(g_wakeup_timer != nullptr)
    {
        g_wakeup_timer->set_enable(false);
        g_wakeup_timer->set_timeout_date(-1);
    }

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

            // unregister if we are still connected to the messenger
            // and Snap! Communicator is not already quitting
            //
            QString action(f_action);
            int const pos(action.indexOf(':'));
            if(pos >= 0)
            {
                action = action.mid(pos + 2);
            }

            snap::snap_communicator_message cmd;
            cmd.set_command("UNREGISTER");
            cmd.add_parameter("service", action);
            g_messenger->send_message(cmd);
        }
    }

    // if we still have a child, ask the child to quit first
    //
    if(g_child_connection != nullptr)
    {
        // propagate the STOP to our current child process
        //
        snap::snap_communicator_message cmd;
        cmd.set_command("STOP");
        g_child_connection->send_message(cmd);
    }
    else
    {
        //g_communicator->remove_connection(g_messenger); -- this one will get an expected HUP shortly or when the child dies
        g_communicator->remove_connection(g_cassandra_timer);
        g_communicator->remove_connection(g_reconnect_timer);
        g_communicator->remove_connection(g_tick_timer);
        g_communicator->remove_connection(g_wakeup_timer);
        g_communicator->remove_connection(g_signal_child_death);
    }

    g_communicator->remove_connection(g_interrupt);
}


void snap_backend::process_connection_failed()
{
    // if this was not called with --action, increase a counter and
    // quit after a few tries
    //
    if(!f_cron_action)
    {
        ++f_error_count;
        if(f_error_count >= 3)
        {
            // too many attempts, just quit
            //
            disconnect();
        }
    }
}



/** \brief Process a "child" message.
 *
 * Whenever we have a child running, we may send and receive messages
 * between the parent and child. Because the parent backend and child
 * processes both use the same g_child_connection object, both end up
 * calling this function to handle their messages.
 *
 * We distinguish the parent and child by their PID.
 *
 * At this time, the parent accepts no messages.
 *
 * The child accepts the STOP, HELP, and UNKNOWN messages. The parent
 * will send a STOP to the child whenever it iself receives a STOP.
 * (i.e. it propagates the STOP message.)
 *
 * \note
 * The f_parent_pid is setup in the parent snap_backend whenever the
 * object is created. It will remain the same once in the child
 * process.
 *
 * \param[in] message  The message received by the parent or the child.
 */
void snap_backend::process_child_message(snap::snap_communicator_message const & message)
{
    SNAP_LOG_TRACE("received child message [")(message.to_message())("] for ")(f_action);

    if(getpid() == f_parent_pid)
    {
        // parent is receiving a message
        //
        // ?
    }
    else
    {
        // child is receiving a message
        //
        QString const command(message.get_command());

        if(command == "STOP")
        {
            f_stop_received = true;
            return;
        }

        if(command == "HELP")
        {
            // return COMMANDS
            //
            snap::snap_communicator_message reply;
            reply.set_command("COMMANDS");
            reply.add_parameter("list", "HELP,STOP,UNKNOWN");
            // we are in the child so g_child_connection exists
            g_child_connection->send_message(reply);
            return;
        }

        if(command == "UNKNOWN")
        {
            // when we send an unknown command we get pinged back with
            // the UNKNOWN message
            // 
            SNAP_LOG_ERROR("we sent an unknown command \"")(message.get_parameter("command"))("\" and probably did not get the expected result.");
            return;
        }

        {
            // return UNKNOWN
            //
            snap::snap_communicator_message reply;
            reply.set_command("UNKNOWN");
            reply.add_parameter("command", command);
            // we are the in the child so g_child_connection exists
            g_child_connection->send_message(reply);
            return;
        }
    }
}


/** \brief This function captures the child process death signal.
 *
 * Whenever the child process dies, we receive this signal immediately.
 * The function processes the exit status with a waitpid() call, removes
 * the child connection from the communicator, and depending on whether
 * it is a backend action, we proceed as follow:
 *
 * \li backend action -- go to sleep for 5 minutes and start the child
 *                       process again then
 * \li normal action -- disconnect from the snap communicator and
 *                      child process and return
 *
 * \param[in] pid  The PID of the child process that just died.
 */
void snap_backend::capture_zombies(pid_t pid)
{
    // first capture the current zombie and save its status upon death
    //
    int status(0);
    pid_t const p(waitpid(pid, &status, 0));
    if(p == -1)
    {
        int const e(errno);
        SNAP_LOG_ERROR("waitpid() returned with an error (errno: ")(e)(" -- ")(strerror(e))(").");
    }
    else
    {
        // save PID and verify it here?
        //if(p != f_child_pid)
        //{
        //}

//SNAP_LOG_WARNING("child process (pid: ")(pid)(") for backend \"")(f_action)("\" returned.");
        if(WIFEXITED(status))
        {
            int const exit_code(WEXITSTATUS(status));
            if(exit_code != 0)
            {
                SNAP_LOG_ERROR("child process (pid: ")(pid)(") for backend \"")(f_action)("\" returned with an error: ")(exit_code)(".");
            }
        }
        else if(WIFSIGNALED(status))
        {
            int const sig(WTERMSIG(status));
            SNAP_LOG_ERROR("child process for backend ")(f_action)(" returned abnormally because of signal \"")(strsignal(sig))("\" (")(sig)(").");
        }
        else
        {
            SNAP_LOG_ERROR("child process for backend ")(f_action)(" returned abnormally.");
        }
    }

    // now we can forget about the child connection
    //
    // TBD: it looks like we could reuse that connection so we
    //      may want to avoid destroying and recreating the
    //      child connections each time, although then we need
    //      a separate flag to know whether a child is currently
    //      running or not (maybe keep its PID?) and it does not
    //      look like the creation is slow at all...
    //
    // WARNING: the g_communicator makes a copy of the connections when
    //          it is processing a set of events; it will be removed on
    //          the next loop, but here we are NOT getting a delete of
    //          the connection so anything we want to do here to make
    //          sure it is gone, we have to call a function for the
    //          purpose! (i.e. we want the UNLOCK to be sent now)
    //
    if(g_child_connection != nullptr)
    {
        g_child_connection->unlock();
    }
    g_communicator->remove_connection(g_child_connection);
    g_child_connection.reset();

    // if we already received a STOP or QUITTING message, then we also
    // want to get rid of the timers and child death signals
    //
    if(f_stop_received)
    {
        g_communicator->remove_connection(g_cassandra_timer);
        g_communicator->remove_connection(g_reconnect_timer);
        g_communicator->remove_connection(g_tick_timer);
        g_communicator->remove_connection(g_wakeup_timer);
        g_communicator->remove_connection(g_signal_child_death);

        // this was the last straw, now we are quitting...
        //
        return;
    }

    // we may have another website to work on right now
    //
    if(f_website.isEmpty() || f_pinged)
    {
        f_pinged = false;
        if(process_timeout())
        {
            return;
        }
    }

    if(!f_cron_action || f_action == "list")
    {
        // this was a "run once and quit", so we want to remove all
        // the connections from the communicator and quit ourselves
        //
        disconnect();
    }
}


/** \brief Check whether the database is ready.
 *
 * This function verifies that the "sites" table exists, if not, then
 * the backends cannot be started safely.
 *
 * Further, if a URI is specified, it verifies that the specified website
 * has a "core::last_updated" field defined.
 *
 * \todo
 * We want to support a background way to upgrade websites. Our current
 * update while accessing the site is okay for small updates, but large
 * upgrades are likely to break everything. So we want to have a way
 * for a backend (snapinit on startup?) to upgrade all websites. Thus,
 * we will want to check a flag to know whether a website was successfully
 * upgraded and if not return false.
 *
 * \param[in] uri  The domain name of a website or an empty string.
 *
 * \return true if the website is considered to be ready.
 */
bool snap_backend::is_ready(QString const & uri)
{
    try
    {
        if(!f_cassandra)
        {
            if(f_auto_retry_cassandra
            && getpid() == f_parent_pid)
            {
                // we received the CASSANDRAREADY message, but did not
                // get a valid connection yet, try again (only in the
                // parent though as the child has one chance and if it
                // fails it just exists)
                //
                if(!connect_cassandra(false))
                {
                    SNAP_LOG_WARNING("snapwebsites failed to connect to snapdbproxy (is_ready())");

                    // note that the connect_cassandra() function should already
                    // do a proper cleanup, but just in case...
                    //
                    disconnect_cassandra();
                    return false;
                }
            }
            else
            {
                // we are in the NOCASSANDRA to CASSANDRAREADY window
                return false;
            }
        }

        if(!f_snaplock)
        {
            // we are waiting on the "snaplock" daemon to be registered as
            // a service to the "snapcommunicator"
            //
            return false;
        }

        if(f_sites_table == nullptr)
        {
            f_context->clearCache();

            // get the "sites" table
            //
            // we do the findTable() here otherwise we would have to try/catch
            // which is slow, not really clean or useful here...
            //
            f_sites_table = f_context->findTable(get_name(name_t::SNAP_NAME_SITES));
            if(f_sites_table == nullptr)
            {
                // sites table does not even exist...
                //
                // we have to reset the connection otherwise we would not get the
                // new context
                //
                request_cassandra_status();
                return false;
            }

            // get the "backend" table
            //
            // we do the findTable() here otherwise we would have to try/catch
            // which is slow, not really clean or useful here...
            //
            f_backend_table = f_context->findTable(get_name(name_t::SNAP_NAME_BACKEND));
            if(f_backend_table == nullptr)
            {
                // backend table does not exist...
                //
                // we have to reset the connection otherwise we would not get the
                // new context
                //
                request_cassandra_status();
                return false;
            }
        }

        if(uri.isEmpty())
        {
            // the mere existance of the sites_table is enough here
            //
            return true;
        }

        // so that specific website must be considered valid
        // which at this time just means having the "core::last_updated"
        // field in the "sites" table
        //
        if(f_sites_table->exists(uri))
        {
            // TODO: to fix SNAP-125 we also want a form of lock, i.e. a parameter
            //       (or just a lock? but our locks are exclusive... see SNAP-470)
            //       that tells us that the website is being updated now...
            //
            //       and conversely we need to know that a backend is running
            //       against a given website so we do not start an update while
            //       that is going on!
            //
            //       with SNAP-470 we can create support for a read-only or
            //       read/write type of semaphore which will resolve that
            //       problem once and for all
            //
            return f_sites_table->row(uri)->exists(get_name(name_t::SNAP_NAME_CORE_LAST_UPDATED))
                && f_sites_table->row(uri)->exists(get_name(name_t::SNAP_NAME_CORE_PLUGIN_THRESHOLD));
        }

        if(!f_cron_action)
        {
            // the regular CRON action did not make it, just quit
            //
            SNAP_LOG_ERROR("website URI \"")(uri)("\" does not reference an existing website.");
            disconnect();
        }
    }
    catch(std::exception const & e)
    {
        // a problem occurred while dealing with the Cassandra cluster
        // through our snapdbproxy daemon
        //
        SNAP_LOG_ERROR("is_ready() received an exception: ")(e.what());

        // pause for 30 seconds, then we will try again
        //
        request_cassandra_status();
    }

    return false;
}


void snap_backend::disconnect()
{
    // remove the connections so we end up quitting
    //
    // TODO: disconnecting these early generates errors we should try to fix:
    //       (see also SNAP-305)
    //
    //       2016-01-20 10:14:03 [15201]:snap_communicator.cpp:2999:halk: error: an error occurred while writing to socket of "snap_tcp_client_permanent_message_connection_impl messenger" (errno: 9 -- Bad file descriptor).
    //       2016-01-20 10:14:03 [15201]:snap_communicator.cpp:1126:halk: error: socket 11 of connection "snap_tcp_client_permanent_message_connection_impl messenger" was marked as erroneous by the kernel.
    //

    // this is an equivalent to a STOP message
    //
    f_stop_received = true;

    if(!f_cron_action && g_messenger != nullptr && g_messenger->is_connected() && f_action != "list")
    {
        g_messenger->mark_done();

        // this was the CRON action
        //
        QString action(f_action);
        int const pos(action.indexOf(':'));
        if(pos >= 0)
        {
            action = action.mid(pos + 2);
        }

        snap::snap_communicator_message cmd;
        cmd.set_command("UNREGISTER");
        cmd.add_parameter("service", action);
        g_messenger->send_message(cmd);

        // g_messenger will very quickly receive a HUP now
    }
    else
    {
        g_communicator->remove_connection(g_messenger);
    }

    // now disconnect so we can quit
    //
    g_communicator->remove_connection(g_interrupt);
    g_communicator->remove_connection(g_cassandra_timer);
    g_communicator->remove_connection(g_reconnect_timer);
    g_communicator->remove_connection(g_tick_timer);
    g_communicator->remove_connection(g_wakeup_timer);
    g_communicator->remove_connection(g_signal_child_death);
}



/** \brief Process a backend request on the specified URI.
 *
 * This function is called with each URI that needs to be processed by
 * the backend processes. It creates a child process that will allow
 * the Cassandra data to not be shared between all instances. Instead
 * each instance reads data and then drops it as the process ends.
 * Since the parent blocks until the child is done, the Cassandra library
 * is still only used by a single process at a time thus we avoid
 * potential conflicts reading/writing on the same network connection
 * (since the child inherits the parents Cassandra connection.)
 *
 * \note
 * Note that the child is created from Cassandra, the plugins, the
 * f_uri and all the resulting keys... so we gain an environment
 * very similar to what we get in the server with Apache.
 *
 * \note
 * If that site has an internal redirect then no processing is
 * performed because otherwise the destination would be processed
 * twice in the end.
 *
 * \todo
 * Add necessary code to break the child if (1) the child is very long
 * and (2) never contact us (i.e. watchdog signals.)
 *
 * \param[in] uri  The URI of the site to be checked.
 */
bool snap_backend::process_backend_uri(QString const & uri)
{
    // first we verify that this very website is indeed ready to accept
    // backend processes, if not return immediately
    //
    if(g_child_connection
    || !is_ready(uri))
    {
        return false;
    }

    // create a child connection so our child and us can communicate
    // (especially, we can send the child a STOP if we ourselves receive
    // a STOP.)
    //
    g_child_connection.reset(new child_connection(this, f_context));

    // We also lock that website while this backend process is running.
    // The lock depends on the URI and the action taken.
    //
    QString lock_uri;
    if(f_global_lock)
    {
        // this action can only run one instance of itself on any
        // one computer in your cluster so the lock does not depend
        // on the URI
        //
        lock_uri = QString("global-backend-lock#%1").arg(f_action);
    }
    else
    {
        // this action can run against multiple websites simultaneous
        //
        lock_uri = QString("%1#%2").arg(uri).arg(f_action);
    }
    if(!g_child_connection->lock(lock_uri))
    {
        g_child_connection.reset();

        // the lock failed, we cannot run against this website at this time
        // (this allows us to have multiple version of the same backend
        // running on various backend computers.)
        //
        SNAP_LOG_INFO("Lock in order to process website \"")(uri)("\" with action \"")(f_action)("\" failed.");

        return false;
    }
    g_communicator->add_connection(g_child_connection);

    // each time we restart a child we obtain a new start date
    // (which is important for CRON actions)
    //
    init_start_date();

    // create a child process so the data between sites does not get
    // shared (also the Cassandra data would remain in memory increasing
    // the foot print each time we run a new website,) but the worst
    // are the plugins; we can request a plugin to be unloaded but
    // frankly the system is not very well written to handle that case.
    pid_t const p(fork_child());
    if(p != 0)
    {
        // parent process
        if(p == -1)
        {
            int const e(errno);

            // fork() failed
            //
            g_communicator->remove_connection(g_child_connection);
            g_child_connection.reset();

            // TODO: now that we have a snap communicator with a timer
            //       we could try to sleep for a while if the error
            //       is ENOMEM or EAGAIN
            //
            SNAP_LOG_FATAL("snap_backend::process_backend_uri() could not create a child process (errno: ")(e)(" -- ")(strerror(e))(").");

            // we do not try again, we just abandon the whole process
            // (i.e. we're in the parent so the backend is quitting 100%)
            //
            exit(1);
            NOTREACHED();
        }
        return true;
    }

    // make it safe in the child process
    //
    try
    {
        SNAP_LOG_INFO("==================================== backend process website \"")(uri)("\" with ")(f_cron_action ? "cron " : "")("action \"")(f_action)("\" started.");

        // make sure that Snap! Communicator is clean in the child,
        // it really cannot be listening on any of these because
        // that would break the parent's streams
        //
        g_communicator->remove_connection(g_messenger);
        g_messenger.reset();
        g_communicator->remove_connection(g_interrupt);
        g_interrupt.reset();
        g_communicator->remove_connection(g_cassandra_timer);
        g_cassandra_timer.reset();
        g_communicator->remove_connection(g_reconnect_timer);
        g_reconnect_timer.reset();
        g_communicator->remove_connection(g_tick_timer);
        g_tick_timer.reset();
        g_communicator->remove_connection(g_wakeup_timer);
        g_wakeup_timer.reset();
        g_communicator->remove_connection(g_signal_child_death);
        g_signal_child_death.reset();

        auto p_server( f_server.lock() );
        if(!p_server)
        {
            throw snap_logic_exception("snap_backend::process_backend_uri(): server pointer is NULL");
        }

        // set the URI; if user supplied it, then it can fail!
        //
        if(!f_uri.set_uri(uri))
        {
            SNAP_LOG_FATAL("snap_backend::process_backend_uri() called with invalid URI: \"")(uri)("\", URI ignored.");
            exit(1);
            NOTREACHED();
        }

        // cassandra re-initialization
        //
        // this is already done in process_action() so we have to reset the
        // pointer before we can call this function again otherwise it throws
        //
        snap_expr::expr::set_cassandra_context(nullptr);
        f_sites_table.reset();
        f_backend_table.reset();
        f_cassandra.reset(); // here all the remaining QCassandra objects should all get deleted
        NOTUSED(connect_cassandra(true)); // since we pass 'true', the function either dies or returns true

        if(!is_ready(uri))
        {
            SNAP_LOG_FATAL("snap_backend::process_backend_uri(): once prepared in the child, URI \"")(uri)("\" is not ready anymore.");
            exit(1);
            NOTREACHED();
        }

        // process the f_uri parameter
        canonicalize_domain();
        canonicalize_website();
        site_redirect();
        if(f_site_key != f_original_site_key)
        {
            SNAP_LOG_FATAL("snap_backend::process_backend_uri() called with incorrect URI: \"")(f_site_key)("\", expected \"")(f_original_site_key)("\".");
            exit(1);
            NOTREACHED();
        }

        init_plugins(true);

        canonicalize_options();

        f_ready = true;

        server::backend_action_set actions;
        if(f_cron_action)
        {
            p_server->register_backend_cron(actions);
#ifdef DEBUG
            // since we are in control of the content plugin, this should
            // never happen...
            //
            if( actions.has_action("content::snapbackend") )
            {
                // the plugin HAS to be content
                throw snap_logic_exception(QString("snap_backend::process_backend_uri(): plugin \"%1\" makes use of a CRON action named \"content::snapbackend\" which is reserved as a special action by the system.")
                                                    .arg(actions.get_plugin_name("content::snapbackend")));
            }
            // XXX: we may want to test that none of the CRON actions are
            //      defined as regular actions
#endif
        }
        else
        {
            p_server->register_backend_action(actions);
        }

        if( actions.has_action(f_action) )
        {
            // this is a valid action, execute the corresponding function!
            //
            actions.execute_action(f_action);
        }
        else if( f_action == "list" )
        {
            std::cout << (f_cron_action ? "CRON " : "") << "Actions available for " << uri << std::endl;
            actions.display();
            std::cout << std::endl;
        }
        else
        {
            if(f_cron_action)
            {
                int const pos(f_action.indexOf(':'));
                QString const namespace_name(f_action.mid(0, pos));
                if(plugins::exists(namespace_name))
                {
                    SNAP_LOG_ERROR("snap_backend::process_backend_uri(): unknown CRON action \"")
                                  (f_action)
                                  ("\" even with plugin \"")
                                  (namespace_name)
                                  ("\" installed.");
                    exit(1);
                    NOTREACHED();
                }
                else
                {
                    // we do not generate an error in case a plugin is not
                    // installed because with many websites installed on
                    // the same system, all may not have all the plugins
                    // installed... so this is just a debug message
                    //
                    SNAP_LOG_DEBUG("snap_backend::process_backend_uri(): unknown CRON action \"")
                                  (f_action)
                                  ("\" for \"")
                                  (uri)
                                  ("\" (although it could be that you need to install plugin \"")
                                  (namespace_name)
                                  ("\" if you wanted to run that backend against this website?)");
                }
            }
            else
            {
                SNAP_LOG_ERROR("snap_backend::process_backend_uri(): unknown action \"")
                              (f_action)
                              ("\".");
                exit(1);
                NOTREACHED();
            }
        }

        // the child process is done successfully
        exit(0);
        NOTREACHED();
    }
    catch( snap_exception const & except )
    {
        SNAP_LOG_FATAL("snap_backend::process_backend_uri(): snap_exception caught: ")(except.what());
    }
    catch( std::exception const & std_except )
    {
        // the snap_logic_exception is not a snap_exception
        // and other libraries may generate other exceptions
        // (i.e. libtld, C++ cassandra driver...)
        //
        SNAP_LOG_FATAL("snap_backend::process_backend_uri(): std::exception caught: ")(std_except.what());
    }
    catch( ... )
    {
        SNAP_LOG_FATAL("snap_backend::process_backend_uri(): unknown exception caught!");
    }

    // the child process is done
    //
    exit(1);
    NOTREACHED();

    // compiler expects a return
    //
    return false;
}


} // namespace snap
// vim: ts=4 sw=4 et
