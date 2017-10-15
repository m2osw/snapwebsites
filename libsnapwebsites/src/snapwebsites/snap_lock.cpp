// Snap Lock -- class to handle inter-process and inter-computer locks
// Copyright (C) 2016-2017  Made to Order Software Corp.
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

#include "snapwebsites/snap_lock.h"

#include "snapwebsites/log.h"
#include "snapwebsites/not_reached.h"
#include "snapwebsites/not_used.h"
#include "snapwebsites/qstring_stream.h"

#include <iostream>

#include <unistd.h>
#include <sys/syscall.h>

#include "snapwebsites/poison.h"


/** \file
 * \brief Implementation of the Snap Lock class.
 *
 * This class is used to create an inter-process lock within a entire
 * Snap cluster.
 *
 * The class uses a blocking socket to communicate to a snaplock
 * instance and wait for the LOCKED event. Once received, it
 * let you run your code.
 *
 * If instead we receive a LOCKFAILED or UNLOCKED (on a timeout we may
 * get an UNLOCKED event) message as a response, then the class throws
 * since the lock was not obtained.
 */

namespace snap
{

namespace
{


/** \brief The default time to live of a lock.
 *
 * By default the inter-process locks are kept for only five seconds.
 * You may change the default using the
 * snaplock::initialize_lock_duration_timeout() function.
 *
 * You can specify how long a lock should be kept around by setting its
 * duration at the time you create it (see the snap_lock constructor.)
 */
snap_lock::timeout_t g_lock_duration_timeout = snap_lock::SNAP_LOCK_DEFAULT_TIMEOUT;


/** \brief The default time to wait in order to obtain a lock.
 *
 * By default the snaplock waits five seconds to realize an
 * inter-process lock. If the locak cannot be obtained within
 * that short period of time, the locking fails.
 *
 * You may change the default using the
 * snaplock::initialize_lock_obtention_timeout() function.
 *
 * You can specify how long to wait for a lock to take by setting
 * its obtention duration at the time you create it (see the snap_lock
 * constructor.)
 *
 * \note
 * By default we use the same amount of the g_lock_duration_timeout
 * variable, which at this point is fortuitious.
 */
snap_lock::timeout_t g_lock_obtention_timeout = snap_lock::SNAP_LOCK_DEFAULT_TIMEOUT;


/** \brief The default snapcommunicator address.
 *
 * This variable holds the snapcommunicator IP address used to create
 * locks by connecting (Sending messages) to snaplock processes.
 *
 * It can be changed using the snaplock::initialize_snapcommunicator()
 * function.
 */
std::string g_snapcommunicator_address = "127.0.0.1";


/** \brief The default snapcommunicator port.
 *
 * This variable holds the snapcommunicator port used to create
 * locks by connecting (Sending messages) to snaplock processes.
 *
 * It can be changed using the snaplock::initialize_snapcommunicator()
 * function.
 */
int g_snapcommunicator_port = 4040;


/** \brief The default snapcommunicator mode.
 *
 * This variable holds the snapcommunicator mode used to create
 * locks by connecting (Sending messages) to snaplock processes.
 *
 * It can be changed using the snaplock::initialize_snapcommunicator()
 * function.
 */
tcp_client_server::bio_client::mode_t g_snapcommunicator_mode = tcp_client_server::bio_client::mode_t::MODE_PLAIN;


/** \brief A unique number used to name the lock service.
 *
 * Each time we create a new lock service we need to have a new unique name
 * because otherwise we could receive replies from a previous lock and
 * there is not other way for us to distinguish them. This is important
 * only if the user is trying to lock the same object several times in
 * a row.
 */
int g_unique_number = 0;


/** \brief Retrieve the current thread identifier.
 *
 * This function retrieves the current thread identifier.
 *
 * \return The thread identifier, which is a pid_t specific to each thread
 *         of a process.
 */
pid_t gettid()
{
    return syscall(SYS_gettid);
}

}
// no name namespace



/** \brief The actual implementation of snap_lock.
 *
 * This class is the actual implementation of snap_lock which is completely
 * hidden from the users of snap_lock. (i.e. implementation details.)
 */
class lock_connection
        : public snap_communicator::snap_tcp_blocking_client_message_connection
{
public:
                            lock_connection(QString const & object_name, snap_lock::timeout_t lock_duration, snap_lock::timeout_t lock_obtention_timeout);
                            ~lock_connection();

    void                    unlock();

    bool                    is_locked() const;
    time_t                  get_lock_timeout_date() const;
    time_t                  get_obtention_timeout_date() const;

    // implementation of snap_communicator::snap_tcp_blocking_client_message_connection
    void                    process_timeout();
    void                    process_message(snap_communicator_message const & message);

private:
    pid_t                       f_owner;
    QString const               f_service_name;
    QString const               f_object_name;
    snap_lock::timeout_t const  f_lock_duration = 0;
    time_t                      f_lock_timeout_date = 0;
    time_t const                f_obtention_timeout_date = 0;
};





/** \brief Initiate an inter-process lock.
 *
 * This constructor is used to obtain an inter-process lock.
 *
 * The lock will be effective on all the computers that have access to
 * the running snaplock instances you can connect to via snapcommunicator.
 *
 * The LOCK and other messages are sent to the snaplock daemon using
 * snapcommunicator.
 *
 * Note that the constructor creates a "lock service" which is given a
 * name which is "lock" and the current thread identifier and a unique
 * number. We use an additional unique number to make sure messages
 * sent to our old instance(s) do not make it to a newer instance, which
 * could be confusing and break the lock mechanism in case the user
 * was trying to get a lock multiple times on the same object.
 *
 * \warning
 * The g_unique_number and the other global variables are not handle
 * safely if you attempt locks in a multithread application. The
 * unicity of the lock name will still be correct because the name
 * includes the thread identifier (see gettid() function.) In a
 * multithread application, the initialize_...() function should be
 * called once by the main thread before the other threads get created.
 *
 * \param[in] object_name  The name of the object being locked.
 * \param[in] lock_duration  The amount of time the lock will last if obtained.
 * \param[in] lock_obtention_timeout  The amount of time to wait for the lock.
 */
lock_connection::lock_connection(QString const & object_name, snap_lock::timeout_t lock_duration, snap_lock::timeout_t lock_obtention_timeout)
    : snap_tcp_blocking_client_message_connection(g_snapcommunicator_address, g_snapcommunicator_port, g_snapcommunicator_mode)
    , f_owner(gettid())
    , f_service_name(QString("lock_%1_%2").arg(gettid()).arg(++g_unique_number))
    , f_object_name(object_name)
    , f_lock_duration(lock_duration == -1 ? g_lock_duration_timeout : lock_duration)
    //, f_lock_timeout_date(0) -- only determined if we obtain the lock
    , f_obtention_timeout_date((lock_obtention_timeout == -1 ? g_lock_obtention_timeout : lock_obtention_timeout) + time(nullptr))
{
    // tell the lower level when the lock obtention times out;
    // that one is in microseconds; if we do obtain the lock,
    // the timeout is then changed to the lock timeout date
    // (which is computed from the time at which we get the lock)
    //
    set_timeout_date(f_obtention_timeout_date * 1000000L);

    // need to register with snap communicator
    //
    snap::snap_communicator_message register_message;
    register_message.set_command("REGISTER");
    register_message.add_parameter("service", f_service_name);
    register_message.add_parameter("version", snap::snap_communicator::VERSION);
    send_message(register_message);

    // now wait for the READY and HELP replies, send LOCK, and
    // either timeout or get the LOCKED message
    //
    run();
}


/** \brief Send the UNLOCK message to snaplock to terminate the lock.
 *
 * The destructor makes sure that the lock is released.
 */
lock_connection::~lock_connection()
{
    try
    {
        unlock();
    }
    catch(snap_communicator_invalid_message const &)
    {
    }
}


/** \brief Send the UNLOCK early (before the destructor is called).
 *
 * This function releases the lock obtained by the constructor.
 *
 * It is safe to call the function multiple times. It will send the
 * UNLOCK only the first time.
 *
 * Note that it is not possible to re-obtain the lock once unlocked.
 * You will have to create a new lock_conenction object to do so.
 *
 * \note
 * If you fork or attempt to unlock from another thread, the unlock()
 * function will do nothing. Only the exact thread that created the
 * lock can call unlock(). This does happen in snap_backend children
 * which attempt to remove the lock their parent setup because the
 * f_lock variable is part of the connection which is defined in a
 * global variable.
 */
void lock_connection::unlock()
{
    if(f_lock_timeout_date != 0
    && f_owner == gettid())
    {
        f_lock_timeout_date = 0;

        // done
        //
        // explicitly send the UNLOCK message and then make sure to unregister
        // from snapcommunicator; note that we do not wait for a reply to the
        // UNLOCK message, since to us it does not matter much as long as the
        // message was sent...
        //
        snap_communicator_message unlock_message;
        unlock_message.set_command("UNLOCK");
        unlock_message.set_service("snaplock");
        unlock_message.add_parameter("object_name", f_object_name);
        unlock_message.add_parameter("pid", gettid());
        send_message(unlock_message);

        snap_communicator_message unregister_message;
        unregister_message.set_command("UNREGISTER");
        unregister_message.add_parameter("service", f_service_name);
        send_message(unregister_message);
    }
}


/** \brief Check whether the lock worked (true) or not (false).
 *
 * This function returns true if the lock succeeded and is still
 * active. This function detects whether the lock timed out and
 * returns false if so.
 *
 * You may check how long the lock has left using the
 * get_lock_timeout_date() which is the date when the lock
 * times out.
 *
 * Note that the get_lock_timeout_date() function will return zero
 * if the lock was not obtained and a threshold date in case the
 * lock was obtained, then zero again after a call to the unlock()
 * function.
 *
 * \return true if the lock is currently active.
 *
 * \sa get_lock_timeout_date()
 * \sa unlock()
 */
bool lock_connection::is_locked() const
{
    // if the lock timeout date was not yet defined, it is not yet
    // locked (we may still be trying to obtain the lock, though);
    // one not zero, the lock is valid as long as that date is
    // after 'now'
    //
    return f_lock_timeout_date != 0 && f_lock_timeout_date > time(nullptr);
}


/** \brief Retrieve the cutoff time for this lock.
 *
 * This lock will time out when this date is reached.
 *
 * If the value is zero, then the lock was not yet obtained.
 *
 * \return The lock timeout date
 */
time_t lock_connection::get_lock_timeout_date() const
{
    return f_lock_timeout_date;
}


/** \brief Retrieve the cutoff time for this lock.
 *
 * This lock will time out when this date is reached.
 *
 * If the value is zero, then the lock was not yet obtained.
 *
 * \return The lock timeout date
 */
time_t lock_connection::get_obtention_timeout_date() const
{
    return f_obtention_timeout_date;
}


/** \brief The lock was not obtained in time.
 *
 * This function gets called whenever the lock does not take with
 * the 'obtention_timeout' amount.
 *
 * Here we tell the system we are done with the lock so that way the
 * run() function returns silently (instead of throwing an error.)
 *
 * The lock will not be active so a call to is_locked() will say
 * so (i.e. return false.)
 */
void lock_connection::process_timeout()
{
    mark_done();
}


/** \brief Process messages as we receive them.
 *
 * This function is called whenever a complete message is read from
 * the snapcommunicator.
 *
 * In a perfect world, the following shows what happens, message wise,
 * as far as the client is concerned. The snaplock sends many more
 * messages in order to obtain the lock (See src/snaplock/snaplock_ticket.cpp
 * for details about those events.)
 *
 * \note
 * The REGISTER message is sent from the constructor to initiate the
 * process. This function starts by receiving the READY message.
 *
 * \msc
 *    client,snapcommunicator,snaplock;
 *
 *    client->snapcommunicator [label="REGISTER"];
 *    snapcommunicator->client [label="READY"];
 *    snapcommunicator->client [label="HELP"];
 *    client->snapcommunicator [label="COMMANDS"];
 *    client->snapcommunicator [label="LOCK"];
 *    snapcommunicator->snaplock [label="LOCK"];
 *    snaplock->snapcommunicator [label="LOCKED"];
 *    snapcommunicator->client [label="LOCKED"];
 *    ...;
 *    client->snapcommunicator [label="UNLOCK"];
 *    snapcommunicator->snaplock [label="UNLOCK"];
 *    client->snapcommunicator [label="UNREGISTER"];
 * \endmsc
 *
 * If somehow the lock fails, we may also receive LOCKFAILED or UNLOCKED.
 *
 * \param[in] message  The message we just received.
 */
void lock_connection::process_message(snap_communicator_message const & message)
{
    // This adds way too many messages! Use only to debug if required.
    //SNAP_LOG_TRACE("received messenger message [")(message.to_message())("] for ")(f_server_name);

    QString const command(message.get_command());

    switch(command[0].unicode())
    {
    case 'H':
        if(command == "HELP")
        {
            // snapcommunicator wants us to tell it what commands
            // we accept
            snap_communicator_message commands_message;
            commands_message.set_command("COMMANDS");
            commands_message.add_parameter("list", "HELP,LOCKED,LOCKFAILED,QUITTING,READY,STOP,UNKNOWN,UNLOCKED");
            send_message(commands_message);

            // no reply expected from the COMMANDS message,
            // so send the LOCK now which really initiate the locking
            snap_communicator_message lock_message;
            lock_message.set_command("LOCK");
            lock_message.set_service("snaplock");
            lock_message.add_parameter("object_name", f_object_name);
            lock_message.add_parameter("pid", gettid());
            lock_message.add_parameter("timeout", f_obtention_timeout_date);
            lock_message.add_parameter("duration", f_lock_duration);
            send_message(lock_message);

            return;
        }
        break;

    case 'L':
        if(command == "LOCKED")
        {
            // the lock worked!
            if(message.get_parameter("object_name") != f_object_name)
            {
                // somehow we received the LOCKED message with the wrong object name
                //
                throw snap_lock_failed_exception(QString("received lock confirmation for object \"%1\" instead of \"%2\" (LOCKED).")
                                .arg(message.get_parameter("object_name"))
                                .arg(f_object_name));
            }
            f_lock_timeout_date = message.get_integer_parameter("timeout_date");

            // change the timeout to the new date, since we are about to
            // quit the run() loop anyway, it should not be necessary
            //
            set_timeout_date(f_lock_timeout_date * 1000000L);

            // release hand back to the user while lock is still active
            //
            mark_done();
            return;
        }
        else if(command == "LOCKFAILED")
        {
            if(message.get_parameter("object_name") == f_object_name)
            {
                SNAP_LOG_WARNING("lock for object \"")
                                (f_object_name)
                                ("\" failed (LOCKEDFAILED).");
            }
            else
            {
                // this should not happen
                //
                SNAP_LOG_WARNING("object \"")
                                (message.get_parameter("object_name"))
                                ("\" just reported a lock failure and we received its message while trying to lock \"")
                                (f_object_name)
                                ("\" (LOCKEDFAILED).");
            }

            mark_done();
            return;
        }
        break;

    case 'Q':
        if(command == "QUITTING")
        {
            SNAP_LOG_WARNING("we received the QUITTING command while waiting for a lock.");

            mark_done();
            return;
        }
        break;

    case 'R':
        if(command == "READY")
        {
            // the REGISTER worked, wait for the HELP message
            return;
        }
        break;

    case 'S':
        if(command == "STOP")
        {
            SNAP_LOG_WARNING("we received the STOP command while waiting for a lock.");

            mark_done();
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
        else if(command == "UNLOCKED")
        {
            // we should not receive the UNLOCKED before the LOCKED
            // and thus this should never be accessed; however, if
            // the LOCKED message got lost, we could very well receive
            // this one instead (it should not because we use a different
            // name each time we request a lock too, but who knows...)
            //
            f_lock_timeout_date = 0;
            if(message.get_parameter("object_name") == f_object_name)
            {
                SNAP_LOG_FATAL("lock for object \"")(f_object_name)("\" failed (UNLOCKED).");
                throw snap_lock_failed_exception(QString("lock for object \"%1\" failed (UNLOCKED).")
                                .arg(f_object_name));
            }
            SNAP_LOG_FATAL("object \"")
                          (message.get_parameter("object_name"))
                          ("\" just got unlocked and we received its message while trying to lock \"")
                          (f_object_name)
                          ("\" (UNLOCKED).");
            throw snap_lock_failed_exception(QString("object \"%1\" just got unlocked and we received its message while trying to lock \"%2\" (UNLOCKED).")
                                .arg(message.get_parameter("object_name"))
                                .arg(f_object_name));
        }
        break;

    }

    // unknown command is reported and process goes on
    //
    {
        SNAP_LOG_ERROR("unsupported command \"")(command)("\" was received by snap_lock on the connection with Snap! Communicator.");

        snap::snap_communicator_message unknown_message;
        unknown_message.set_command("UNKNOWN");
        unknown_message.add_parameter("command", command);
        send_message(unknown_message);
    }
}








/** \brief Create an inter-process lock.
 *
 * This constructor blocks until you obtained an inter-process lock
 * named \p object_name when you specify such a name. If you pass
 * an empty string, then the lock object is in the "unlocked" state.
 * You may call the lock() function once you are ready to lock the
 * system.
 *
 * By default you do not have to specify the lock_duration and
 * lock_obtention_timeout. When set to -1 (the default in the
 * constructor declaration,) these values are replaced by their
 * defaults that are set using the initialize_lock_duration_timeout()
 * and initialize_lock_obtention_timeout() functions.
 *
 * \warning
 * If the object name is specified in the constructor, then the system
 * attempts to lock the object immediately meaning that the only way
 * we can let you know that the lock failed is by throwing an exception.
 * If you want to avoid the exception, instead of doing a try/catch,
 * use the contructor without any parameters and then call the lock()
 * function and check whether you get true or false.
 *
 * \param[in] object_name  The name of the lock.
 * \param[in] lock_duration  The amount of time the lock will last if obtained.
 * \param[in] lock_obtention_timeout  The amount of time to wait for the lock.
 *
 * \sa snap_lock::initialize_lock_duration_timeout()
 */
snap_lock::snap_lock(QString const & object_name, timeout_t lock_duration, timeout_t lock_obtention_timeout)
    //: f_lock_connection(nullptr) -- auto-init
{
    if(!object_name.isEmpty())
    {
        if(!lock(object_name, lock_duration, lock_obtention_timeout))
        {
            throw snap_lock_failed_exception(QString("locking \"%1\" failed.").arg(object_name));
        }
    }
}


/** \brief Set how long inter-process locks last.
 *
 * This function lets you change the default amount of time the
 * inter-process locks last (i.e. their "Time To Live") in seconds.
 *
 * For example, to keep locks for 1 hour, use 3600.
 *
 * This value is used whenever a lock is created with its lock
 * duration set to -1.
 *
 * \warning
 * This function is not thread safe.
 *
 * \param[in] timeout  The total number of seconds locks will last for by default.
 */
void snap_lock::initialize_lock_duration_timeout(timeout_t timeout)
{
    // ensure a minimum of 3 seconds
    if(timeout < SNAP_LOCK_MINIMUM_TIMEOUT)
    {
        timeout = SNAP_LOCK_MINIMUM_TIMEOUT;
    }
    g_lock_duration_timeout = timeout;
}


/** \brief Retrieve the current lock duration.
 *
 * This function returns the current lock timeout. It can be useful if
 * you want to use a lock with a different timeout and then restore
 * the previous value afterward.
 *
 * Although if you have access/control of the lock itself, you may instead
 * want to specify the timeout in the snap_lock constructor directly.
 *
 * \return Current lock TTL in seconds.
 */
snap_lock::timeout_t snap_lock::current_lock_duration_timeout()
{
    return g_lock_duration_timeout;
}


/** \brief Set how long to wait for an inter-process lock to take.
 *
 * This function lets you change the default amount of time the
 * inter-process locks can wait before forfeiting the obtention
 * of a new lock.
 *
 * This amount can generally remain pretty small. For example,
 * you could say that you want to wait just 1 minute even though
 * the lock you want to get will last 24 hours. This means, within
 * one minute your process will be told that the lock cannot be
 * realized. In other words, you cannot do the work you intended
 * to do. If the lock is released within the 1 minute and you
 * are next on the list, you will get the lock and can proceed
 * with the work you intended to do.
 *
 * The default is five seconds which for a front end is already
 * quite enormous.
 *
 * This value is used whenever a lock is created with its lock
 * obtention timeout value set to -1.
 *
 * \warning
 * This function is not thread safe.
 *
 * \param[in] timeout  The total number of seconds to wait to obtain future locks.
 */
void snap_lock::initialize_lock_obtention_timeout(timeout_t timeout)
{
    // ensure a minimum of 3 seconds
    if(timeout < SNAP_LOCK_MINIMUM_TIMEOUT)
    {
        timeout = SNAP_LOCK_MINIMUM_TIMEOUT;
    }
    g_lock_obtention_timeout = timeout;
}


/** \brief Retrieve the current lock obtention duration.
 *
 * This function returns the current timeout for the obtention of a lock.
 * It can be useful if you want to use a lock with a different obtention
 * timeout and then restore the previous value afterward.
 *
 * Although if you have access/control of the lock itself, you may instead
 * want to specify the timeout in the snap_lock constructor directly.
 *
 * \return Current lock obtention maximum wait period in seconds.
 */
snap_lock::timeout_t snap_lock::current_lock_obtention_timeout()
{
    return g_lock_obtention_timeout;
}


/** \brief Initialize the snapcommunicator details.
 *
 * This function must be called before any lock can be obtained in order
 * to define the address and port to use to connect to the snapcommunicator
 * process.
 *
 * \warning
 * This function is not thread safe.
 *
 * \param[in] addr  The address of snapcommunicator.
 * \param[in] port  The port used to connect to snapcommunicator.
 * \param[in] mode  The mode used to open the connection (i.e. plain or secure.)
 */
void snap_lock::initialize_snapcommunicator(std::string const & addr, int port, tcp_client_server::bio_client::mode_t mode)
{
    g_snapcommunicator_address = addr;
    g_snapcommunicator_port = port;
    g_snapcommunicator_mode = mode;
}


/** \brief Attempt to lock the specified object.
 *
 * This function attempts to lock the specified object. If a lock
 * was already in place, the function always sends an UNLOCK first.
 *
 * On a time scale, the lock_duration and lock_obtention_timeout
 * parameters are used as follow:
 *
 * \li Get current time ('now')
 * \li Calculate when it will be too late for this process to get the
 *     lock, this is 'now + lock_obtention_timeout', if we get the
 *     lock before then, all good, if not, we return with false
 * \li Assuming we obtained the lock, get the new current time ('locked_time')
 *     and add the lock_duration to know when the lock ends
 *     ('lock_time + lock_duration'). Your process can run up until that
 *     date is reached.
 *
 * \param[in] object_name  The name of the object to lock.
 * \param[in] lock_duration  The amount of time the lock will last if obtained.
 * \param[in] lock_obtention_timeout  The amount of time to wait for the lock.
 *
 * \return Whether the lock was obtained (true) or not (false).
 */
bool snap_lock::lock(QString const & object_name, timeout_t lock_duration, timeout_t lock_obtention_timeout)
{
    // although we have a shared pointer, the order in which the lock
    // and unlock work would be inverted if we were to just call
    // the reset() function (i.e. it would try to obtain the new lock
    // first, then release the old lock second; which could cause a
    // dead-lock,) therefore it is better if we explicitly call the
    // unlock() function first
    //
    unlock();

    f_lock_connection = std::make_shared<lock_connection>(object_name, lock_duration, lock_obtention_timeout);

    return f_lock_connection->is_locked();
}


/** \brief Release the inter-process lock early.
 *
 * This function releases this inter-process lock early.
 *
 * If the lock was not active (i.e. lock() was never called or returned
 * false last time you called it), then nothing happens.
 */
void snap_lock::unlock()
{
    if(f_lock_connection)
    {
        f_lock_connection->unlock();
        f_lock_connection.reset();
    }
}


/** \brief Get the exact time when the lock times out.
 *
 * This function can be used to check when the lock will be considerd
 * out of date and thus when you should stop doing whatever requires
 * said lock.
 *
 * The time is in second and you can compare it against time(nullptr) to
 * know whether it timed out already or how long you still have:
 *
 * \code
 *      int64_t const diff(lock.get_timeout_date() - time(nullptr));
 *      if(diff <= 0)
 *      {
 *          // locked already timed out
 *          ...
 *      }
 *      else
 *      {
 *          // you have another 'diff' seconds to work on your stuff
 *          ...
 *      }
 * \endcode
 *
 * Remember that this exact date was sent to the snaplock system but you may
 * have a clock with a one second or so difference between various computers
 * so if the amount is really small (1 or 2) you should probably already
 * considered that the locked has timed out.
 *
 * \return The date when this lock will be over, or zero if the lock is
 *         not currently active.
 */
time_t snap_lock::get_timeout_date() const
{
    if(f_lock_connection)
    {
        return f_lock_connection->get_lock_timeout_date();
    }

    return 0;
}


} // namespace snap
// vim: ts=4 sw=4 et
