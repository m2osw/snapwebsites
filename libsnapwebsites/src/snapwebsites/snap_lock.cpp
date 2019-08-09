// Snap Lock -- class to handle inter-process and inter-computer locks
// Copyright (c) 2016-2019  Made to Order Software Corp.  All Rights Reserved
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


// self
//
#include "snapwebsites/snap_lock.h"


// snapwebsites lib
//
#include "snapwebsites/log.h"
#include "snapwebsites/qstring_stream.h"
#include "snapwebsites/snap_communicator_dispatcher.h"


// snapdev lib
//
#include <snapdev/not_reached.h>
#include <snapdev/not_used.h>


// C++ lib
//
#include <iostream>


// C lib
//
#include <unistd.h>
#include <sys/syscall.h>


// last include
//
#include <snapdev/poison.h>



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


/** \brief The default time to wait for a timed out lock acknowledgement.
 *
 * Whenever a lock is obtained, it can time out if the owner does not send
 * an UNLOCK message on time.
 *
 * When a lock times out, the snaplock daemon forcibly sends an UNLOCKED
 * message with an error message set to "timedout". When the service that
 * owns that lock receives that message, it is expected to acknowledge it.
 * If no acknowledgement happens before this duration elapses, then the
 * lock is released no matter what. This, indeed, means that another
 * process may obtain the same lock and access the same resources in
 * parallel...
 *
 * To acknowledge an UNLOCKED message, reply with UNLOCK. No other UNLOCKED
 * message gets sent after that.
 *
 * By default the unlock duraction is set to
 * snap_lock::SNAP_UNLOCK_MINIMUM_TIMEOUT which is one minute.
 */
snap_lock::timeout_t g_unlock_duration_timeout = snap_lock::SNAP_UNLOCK_MINIMUM_TIMEOUT;


#ifdef _DEBUG
/** \brief Whether the snapcommunicator parameters were initialized.
 *
 * This variable is only used in debug mode. This allows us to know whether
 * the initialize_snapcommunicator() was called before we make use of the
 * snap_lock() interface. Without that initialization, we may run in various
 * problems if the administrator changed his snapcommunicator parameters
 * such as the port to which we need to connect. This would be a bug in
 * the code.
 *
 * In release mode we ignore that flag.
 */
bool g_snapcommunicator_initialized = false;
#endif


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
 * there is no other way for us to distinguish them. This is important
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





namespace details
{

/** \brief The actual implementation of snap_lock.
 *
 * This class is the actual implementation of snap_lock which is completely
 * hidden from the users of snap_lock. (i.e. implementation details.)
 *
 * The class connects to the snapcommunicator daemon, sends a LOCK event
 * and waits for the LOCKED message or a failure at which point the
 * daemon returns.
 *
 * The connection remains _awake_ even once the lock was obtained in
 * case the snaplock daemon wants to send us another message and so
 * we can send it an UNLOCK message. It is also important to check
 * whether the lock timed out after a while. The snaplock daemon
 * sends us an UNLOCKED message which we need to acknowledge with
 * an UNLOCK. This is important to allow the next user in line to
 * quickly obtain his own lock.
 */
class lock_connection
    : public snap_communicator::snap_tcp_blocking_client_message_connection
    , public snap::dispatcher<lock_connection>
{
public:
    typedef std::shared_ptr<lock_connection>        pointer_t;

                            lock_connection(QString const & object_name, snap_lock::timeout_t lock_duration, snap_lock::timeout_t lock_obtention_timeout, snap_lock::timeout_t unlock_duration);
    virtual                 ~lock_connection() override;

    void                    run();
    void                    unlock();

    bool                    is_locked() const;
    bool                    lock_timedout() const;
    time_t                  get_lock_timeout_date() const;

    // implementation of snap_communicator::snap_tcp_blocking_client_message_connection
    virtual void            process_timeout() override;
    //virtual void            process_message(snap_communicator_message const & message) override;

    // implementation of snap_communicator::connection_with_send_message
    virtual void            ready(snap_communicator_message & message) override;
    virtual void            stop(bool quitting) override;

private:
    void                    msg_locked(snap::snap_communicator_message & message);
    void                    msg_lockfailed(snap::snap_communicator_message & message);
    void                    msg_unlocked(snap::snap_communicator_message & message);

    static snap::dispatcher<lock_connection>::dispatcher_match::vector_t const    g_lock_connection_messages;

    pid_t const                 f_owner;
    QString const               f_service_name;
    QString const               f_object_name;
    snap_lock::timeout_t const  f_lock_duration;
    time_t                      f_lock_timeout_date = 0;
    time_t const                f_obtention_timeout_date;
    snap_lock::timeout_t const  f_unlock_duration = snap_lock::SNAP_UNLOCK_USES_LOCK_TIMEOUT;
    bool                        f_lock_timedout = false;
};



snap::dispatcher<lock_connection>::dispatcher_match::vector_t const lock_connection::g_lock_connection_messages =
{
    {
        "LOCKED"
      , &lock_connection::msg_locked
    },
    {
        "LOCKFAILED"
      , &lock_connection::msg_lockfailed
    },
    {
        "UNLOCKED"
      , &lock_connection::msg_unlocked
    }
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
 * If somehow the lock fails, we may receive LOCKFAILED or UNLOCKED instead
 * of the LOCK message.
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
 * \param[in] unlock_duration  The amount of time we have to acknowledge a
 *                             timed out lock.
 */
lock_connection::lock_connection(QString const & object_name, snap_lock::timeout_t lock_duration, snap_lock::timeout_t lock_obtention_timeout, snap_lock::timeout_t unlock_duration)
    : snap_tcp_blocking_client_message_connection(g_snapcommunicator_address, g_snapcommunicator_port, g_snapcommunicator_mode)
    , dispatcher(this, g_lock_connection_messages)
    , f_owner(gettid())
    , f_service_name(QString("lock_%1_%2").arg(gettid()).arg(++g_unique_number))
    , f_object_name(object_name)
    , f_lock_duration(lock_duration == -1 ? g_lock_duration_timeout : lock_duration)
    //, f_lock_timeout_date(0) -- only determined if we obtain the lock
    , f_obtention_timeout_date((lock_obtention_timeout == -1 ? g_lock_obtention_timeout : lock_obtention_timeout) + time(nullptr))
    , f_unlock_duration(unlock_duration)
{
#ifdef _DEBUG
    if(!g_snapcommunicator_initialized)
    {
        throw snap_lock_not_initialized("your process must call snap::snap_lock::initialize_snapcommunicator() at least once before you can create locks.");
    }
#endif
    add_snap_communicator_commands();

    // tell the lower level when the lock obtention times out;
    // that one is in microseconds; if we do obtain the lock,
    // the timeout is then changed to the lock duration
    // (which is computed from the time at which we get the lock)
    //
    set_timeout_date(f_obtention_timeout_date * 1000000L);
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


/** \brief Listen for messages.
 *
 * This function overloads the blocking connection run() function so we
 * can properly initialize the lock_connection object (some things just
 * cannot be done in the construtor.)
 *
 * The function makes sure we have the dispatcher setup and sends the
 * REGISTER message so we get registered with snapcommunicator.
 */
void lock_connection::run()
{
    set_dispatcher(std::dynamic_pointer_cast<lock_connection>(shared_from_this()));

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
    snap_tcp_blocking_client_message_connection::run();
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
 * lock can actually unlock. This does happen in snap_backend children
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
 * The following is a simple usage example. Note that the UNLOCK
 * message will be sent to the snaplock daemon whenever the
 * snap_lock get destroyed and a lock was obtained. There is
 * no need for you to call the unlock() function. However, it can
 * be useful to perform the very important tasks on the resource
 * being locked first and if it times out, forfeit further less
 * important work.
 *
 * \code
 *      {
 *          snap_lock l(...);
 *
 *          ...do work with resource...
 *          if(l.is_locked())
 *          {
 *              ...do more work with resource...
 *          }
 *          // l.unlock() is implicit
 *      }
 * \endcode
 *
 * You may check how long the lock has left using the
 * get_lock_timeout_date() which is the date when the lock
 * times out.
 *
 * Note that the get_lock_timeout_date() function returns zero
 * if the lock was not obtained and a threshold date in case the
 * lock was obtained, then zero again after a call to the unlock()
 * function or when the UNLOCKED message was received and processed
 * (i.e. to get the UNLOCKED message processed you need to call the
 * lock_timedout() function).
 *
 * \return true if the lock is currently active.
 *
 * \sa get_lock_timeout_date()
 * \sa unlock()
 * \sa lock_timedout()
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


/** \brief Check whether the lock timed out.
 *
 * This function checks whether an UNLOCKED was received while you hold
 * the lock. You cannot call the function if you did not yet obtain the
 * lock.
 *
 * If you are going to destroy the lock right away after determining that
 * it timed out, call the is_locked() function instead. The lock object
 * will automatically send an UNLOCK message when it gets destroyed so
 * just that is enough.
 *
 * Now, you want to use this lock_timedout() function in case you want
 * to test whether a lock is about to be released by the snaplock daemon
 * which took ownship of the lock. It will not send the UNLOCK event
 * back (acknowlegement). You are responsible to call the unlock()
 * function once this function returns true.
 *
 * This function is used on a snap_lock object that is kept around
 * for a while. If you are going to destroy the lock anyway, you
 * can just do that as soon as possible and you will be just fine.
 *
 * The following more or less shows a case where the lock_timedout()
 * and unlock() can be used. The lock_timedout() function can be
 * called any number of time, so if you do work in a loop, you can
 * safely call it once or twice per iteration:
 *
 * \code
 *  snap_lock l;
 *
 *  for(;;)
 *  {
 *      l.lock(...);
 *
 *      do
 *      {
 *          ...start synchronized work here...
 *          if(l.lock_timedout())
 *          {
 *              // make sure we unlock
 *              l.unlock();
 *              break;
 *          }
 *          ...more synchronized work
 *      }
 *      while(false);
 *  }
 * \endcode
 *
 * This example shows an explicit unlock() call. If you immediately
 * try a new lock() call, then the unlock() is call implicitely.
 *
 * \note
 * As mentioned above, you still got a little bit of time after this
 * function returns true. However, the sooner you call the unlock()
 * function after this function returns true, the safer.
 *
 * \return true if the lock timed out, false otherwise.
 *
 * \sa is_locked()
 * \sa unlock()
 */
bool lock_connection::lock_timedout() const
{
    if(f_lock_timeout_date != 0)
    {
        // if the timeout date has not yet elapsed then the lock cannot
        // have dropped yet (unless a big problem happened and checking
        // with snaplock would probably fail anyway.)
        //
        if(f_lock_timeout_date > time(nullptr))
        {
            return false;
        }

        // it looks like we timed out, check for an UNLOCKED event
        //
        // just do a peek(), that is enough since the msg_unlocked()
        // will set the f_lock_timedout flag as required; if we did
        // not yet receive a full event, we return false (i.e. not
        // yet timed out); it will also set f_lock_timeout_date back
        // to zero
        //
        const_cast<lock_connection *>(this)->peek();
    }

    return f_lock_timedout;
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


/** \brief The snapcommunicator is ready to talk to us.
 *
 * This function gets called once the connection between the snapcommunicator
 * and us is up and running.
 *
 * We immediately send the LOCK message to the snaplock running on this
 * system.
 */
void lock_connection::ready(snap_communicator_message & message)
{
    NOTUSED(message);

    // no reply expected from the COMMANDS message,
    // so send the LOCK now which initiates the locking
    //
    snap_communicator_message lock_message;
    lock_message.set_command("LOCK");
    lock_message.set_service("snaplock");
    lock_message.add_parameter("object_name", f_object_name);
    lock_message.add_parameter("pid", gettid());
    lock_message.add_parameter("timeout", f_obtention_timeout_date);
    lock_message.add_parameter("duration", f_lock_duration);
    if(f_unlock_duration != snap_lock::SNAP_UNLOCK_USES_LOCK_TIMEOUT)
    {
        lock_message.add_parameter("unlock_duration", f_unlock_duration);
    }
    send_message(lock_message);
}


/** \brief Stop the lock connections.
 *
 * Whenever a STOP or QUITTING is received this function gets called.
 *
 * We should never receive these messages in a lock_connection, although
 * it is definitely possible and part of the protocol. What is more likely
 * to happen is that the function
 *
 * \param[in] quitting  Whether the sending is telling us that we are quitting
 *            (i.e. rebooting) or just stopping.
 */
void lock_connection::stop(bool quitting)
{
    SNAP_LOG_WARNING("we received the ")
                    (quitting ? "QUITTING" : "STOP")
                    (" command while waiting for a lock.");

    mark_done();
}


/** \brief Process the LOCKED message.
 *
 * This function processes the LOCKED message meaning that it saves the
 * timeout_date as determined by the snaplock daemon and mark the message
 * loop as done which means it will return to the caller.
 *
 * Note however that the TCP/IP connection to snapcommunicator is kept
 * alive since we want to keep the lock until we are done with it.
 *
 * \param[in] message  The message we just received.
 */
void lock_connection::msg_locked(snap::snap_communicator_message & message)
{
    // the lock worked?
    //
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
}


/** \brief Process the LOCKFAILED message.
 *
 * This function processes the LOCKFAILED message meaning that it saves
 * the lock was never obtained. In most cases this is due to a timeout.
 * The timeout can be due to the fact that the snaplock too is not yet
 * handling locks (LOCKREADY was not yet sent.)
 *
 * Since the lock failed, you must not make use of the resource(s) you
 * were trying to lock.
 *
 * \param[in] message  The message we just received.
 */
void lock_connection::msg_lockfailed(snap::snap_communicator_message & message)
{
    if(message.get_parameter("object_name") == f_object_name)
    {
        SNAP_LOG_WARNING("lock for object \"")
                        (f_object_name)
                        ("\" failed (LOCKEDFAILED) with reason: ")
                        (message.get_parameter("error"))
                        (".");
    }
    else
    {
        // this should not happen
        //
        SNAP_LOG_ERROR("object \"")
                      (message.get_parameter("object_name"))
                      ("\" just reported a lock failure and we received its message while trying to lock \"")
                      (f_object_name)
                      ("\" (LOCKEDFAILED) with reason: ")
                      (message.get_parameter("error"))
                      (".");
    }

    mark_done();
}


/** \brief Process the UNLOCKLED message.
 *
 * This function processes the UNLOCKED message.
 *
 * There are three potential cases when we can receive an UNLOCKED:
 *
 * \li a spurious message -- i.e. the message was not meant for this lock;
 *     this is not ever expected, we throw when this happens
 * \li lock timed out -- if your process takes too long before releasing
 *     the lock, you get this type of UNLOCKED with an error message
 *     saying "timedout"
 * \li acknowledgement -- when we send the UNLOCK message to a snaplock, it
 *     acknowledges with UNLOCKED (unless we already received the UNLOCKED
 *     because of a timeout, then it does not get sent twice)
 *
 * The first case (object name mismatched) is just not likely to ever
 * occur. It is more of a sanity test.
 *
 * The second case (lock timed out) can happen if your task takes longer
 * then expected. In that case, you should end your task, at least the
 * part that required the locked resources. You task must then acknowledge
 * that it is done by sending an UNLOCK message to snaplock.
 *
 * The third case (acknowledgement) is sent when you initiate the
 * release of your lock by sending the UNLOCK message (see unlock()
 * for more details.)
 *
 * \attention
 * In the second case (lock timed out) the lock will still be in place
 * until your send the UNLOCK message or the amount of time specified
 * in the unlock duration parameter. By default (which is also the minimum,)
 * this is 60 seconds. You can still be accessing your resources, but should
 * consider stopping as soon as possible and re-obtain a new lock to
 * continue your work.
 *
 * \exception snap_lock_failed_exception
 * This exception is raised when the UNLOCKED is received unexpectendly
 * (i.e. the object name does not match or the UNLOCKED is received before
 * the LOCKED was ever received.)
 *
 * \param[in] message  The message we just received.
 */
void lock_connection::msg_unlocked(snap::snap_communicator_message & message)
{
    // we should never receive an UNLOCKED from the wrong object
    // because each lock attempt is given a new name
    //
    if(message.get_parameter("object_name") != f_object_name)
    {
        f_lock_timeout_date = 0;
        SNAP_LOG_FATAL("object \"")
                      (message.get_parameter("object_name"))
                      ("\" just got unlocked and we received its message while trying to lock \"")
                      (f_object_name)
                      ("\" (UNLOCKED).");
        throw snap_lock_failed_exception(QString("object \"%1\" just got unlocked and we received its message while trying to lock \"%2\" (UNLOCKED).")
                            .arg(message.get_parameter("object_name"))
                            .arg(f_object_name));
    }

    // we should not receive the UNLOCKED before the LOCKED
    // also snaplock prevents the sending of more than one
    // UNLOCKED event so the following should never be true
    //
    if(f_lock_timeout_date == 0)
    {
        SNAP_LOG_FATAL("lock for object \"")
                      (f_object_name)
                      ("\" failed (UNLOCKED received before LOCKED).");
        throw snap_lock_failed_exception(QString("lock for object \"%1\" failed (UNLOCKED received before LOCKED).")
                        .arg(f_object_name));
    }

    f_lock_timeout_date = 0;

    if(message.has_parameter("error"))
    {
        QString const error(message.get_parameter("error"));
        if(error == "timedout")
        {
            SNAP_LOG_INFO("lock for object \"")
                            (f_object_name)
                            ("\" timed out.");

            // we are expected to send an acknowledgement in this case
            // but we do not do so here, the caller is expected to take
            // care of that by calling the unlock() function explicitly
            //
            f_lock_timedout = true;
        }
        else
        {
            SNAP_LOG_WARNING("lock for object \"")
                            (f_object_name)
                            ("\" failed with error: ")
                            (error)
                            (".");
        }
    }
}







}
// details namespace



// once in a while these definitions may be required (especially in debug mode)
//
// TODO: remove once we use C++17
//
constexpr snap_lock::timeout_t    snap_lock::SNAP_LOCK_DEFAULT_TIMEOUT;
constexpr snap_lock::timeout_t    snap_lock::SNAP_LOCK_MINIMUM_TIMEOUT;
constexpr snap_lock::timeout_t    snap_lock::SNAP_UNLOCK_MINIMUM_TIMEOUT;
constexpr snap_lock::timeout_t    snap_lock::SNAP_UNLOCK_USES_LOCK_TIMEOUT;
constexpr snap_lock::timeout_t    snap_lock::SNAP_MAXIMUM_OBTENTION_TIMEOUT;
constexpr snap_lock::timeout_t    snap_lock::SNAP_MAXIMUM_TIMEOUT;












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
 * \param[in] unlock_duration  The amount of time we have to acknowledge a
 *                             timed out lock.
 *
 * \sa snap_lock::initialize_lock_duration_timeout()
 */
snap_lock::snap_lock(QString const & object_name, timeout_t lock_duration, timeout_t lock_obtention_timeout, timeout_t unlock_duration)
{
    if(!object_name.isEmpty())
    {
        if(!lock(object_name, lock_duration, lock_obtention_timeout, unlock_duration))
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


/** \brief Set how long we wait on an inter-process unlock acknowledgement.
 *
 * This function lets you change the default amount of time the
 * inter-process unlock last (i.e. their "Time To Survive" after
 * a lock time out) in seconds.
 *
 * For example, to allow your application to take up to 5 minuts to
 * acknowldege a timed out lock, set this value to 300.
 *
 * This value is used whenever a lock is created with its unlock
 * duration set to -1. Note that by default this value is itself
 * -1 meaning that the unlock duration will use the lock duration.
 *
 * \warning
 * This function is not thread safe.
 *
 * \param[in] timeout  The total number of seconds timed out locks wait for
 *                     an acknowledgement.
 */
void snap_lock::initialize_unlock_duration_timeout(timeout_t timeout)
{
    // ensure a minimum of 60 seconds
    // but allow -1 as a valid value
    //
    if(timeout != SNAP_UNLOCK_USES_LOCK_TIMEOUT
    && timeout < SNAP_UNLOCK_MINIMUM_TIMEOUT)
    {
        timeout = snap_lock::SNAP_UNLOCK_MINIMUM_TIMEOUT;
    }
    g_unlock_duration_timeout = timeout;
}


/** \brief Retrieve the current unlock duration.
 *
 * This function returns the current unlock duration. It can be useful
 * if you want to use a lock with a different timeout and then restore
 * the previous value afterward.
 *
 * Although if you have access/control of the lock itself, you may instead
 * want to specify the unlock duration in the snap_lock constructor directly.
 *
 * \return Current unlock TTL in seconds.
 */
snap_lock::timeout_t snap_lock::current_unlock_duration_timeout()
{
    return g_unlock_duration_timeout;
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

#ifdef _DEBUG
    g_snapcommunicator_initialized = true;
#endif
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
 * \note
 * The existing lock, if there is one, gets unlock()'ed before the systems
 * attempts to gain a new lock. This is particularly important especially
 * since we may be trying to obtain the exact same lock again.
 *
 * \param[in] object_name  The name of the object to lock.
 * \param[in] lock_duration  The amount of time the lock will last if obtained.
 * \param[in] lock_obtention_timeout  The amount of time to wait for the lock.
 *
 * \return Whether the lock was obtained (true) or not (false).
 */
bool snap_lock::lock(QString const & object_name, timeout_t lock_duration, timeout_t lock_obtention_timeout, timeout_t unlock_duration)
{
    // although we have a shared pointer, the order in which the lock
    // and unlock work would be inverted if we were to just call
    // the reset() function (i.e. it would try to obtain the new lock
    // first, then release the old lock second; which could cause a
    // dead-lock,) therefore it is better if we explicitly call the
    // unlock() function first
    //
    unlock();

    f_lock_connection = std::make_shared<details::lock_connection>(object_name, lock_duration, lock_obtention_timeout, unlock_duration);
    f_lock_connection->run();

    return f_lock_connection->is_locked();
}


/** \brief Release the inter-process lock early.
 *
 * This function releases this inter-process lock early.
 *
 * If the lock was not active (i.e. lock() was never called or returned
 * false last time you called it), then nothing happens.
 *
 * This is useful if you keep the same lock object around and want to
 * lock/unlock it at various time. It is actually very important to
 * unlock your locks so other processes can then gain access to the
 * resources that they protect.
 */
void snap_lock::unlock()
{
    if(f_lock_connection != nullptr)
    {
        // be explicit although the destructor of the lock connection
        // calls unlock() on its own
        //
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
 *      snap::snap_lock::timeout_t const diff(lock.get_timeout_date() - time(nullptr));
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
 *
 * \sa lock_timedout()
 * \sa is_locked()
 */
time_t snap_lock::get_timeout_date() const
{
    if(f_lock_connection != nullptr)
    {
        return f_lock_connection->get_lock_timeout_date();
    }

    return 0;
}


/** \brief This function checks whether the lock is considered locked.
 *
 * This function checks whether the lock worked and is still considered
 * locked, as in, it did not yet time out.
 *
 * This function does not access the network at all. It checks whether
 * the lock is still valid using the current time and the time at which
 * the LOCKED message said the lock would time out.
 *
 * If you want to know whether snaplock decided that the lock timed out
 * then you need to consider calling the lock_timedout() function instead.
 *
 * If you want to know how my time you have left on this lock, use the
 * get_timeout_date() instead and subtract time(nullptr). If positive,
 * that's the number of seconds you have left.
 *
 * \note
 * The function returns false if there is no lock connection, which
 * means that there is no a lock in effect at this time.
 *
 * \return true if the lock is still in effect.
 *
 * \sa lock_timedout()
 * \sa get_timeout_date()
 */
bool snap_lock::is_locked() const
{
    if(f_lock_connection != nullptr)
    {
        return f_lock_connection->is_locked();
    }

    return false;
}


/** \brief This function checks whether the lock timed out.
 *
 * This function checks whether the lock went passed the deadline and
 * we were sent an UNLOCKED message with the error set to "timedout".
 * If so, this function returns true and that means we have to stop
 * the work we are doing as soon as possible or the resource may be
 * released to another process and thus a mess may happen.
 *
 * Note that the snaplock daemon holds a lock after it send an UNLOCKED
 * with a "timedout" error for an amount of time equal to the duration
 * of the lock or one minute, whichever is longer (most often, it will
 * be one minute, although some locks for backends are often held for
 * a much longer period of time.)
 *
 * \note
 * The function checks whether a message was received, so it does not
 * really spend much time at all with the network. It just looks at
 * the socket buffer and if data is present, it reads only. However,
 * it reads those bytes one by one, so that can be a bit slow. Only
 * the UNLOCKED message should be sent and it will be rather short
 * so I would not worry about it since it will work with a buffer
 * which is going to be really fast anyway.
 *
 * \note
 * The function returns true if there is no lock connection, which
 * means that there is no a lock in effect at this time.
 *
 * \return true if the lock timed out and you need to stop work on
 *         the locked resource as soon as possible.
 *
 * \sa is_locked()
 * \sa get_timeout_date()
 */
bool snap_lock::lock_timedout() const
{
    if(f_lock_connection != nullptr)
    {
        return f_lock_connection->lock_timedout();
    }

    return true;
}


/** \brief Initialize an RAII lock duration timeout.
 *
 * You may change the lock duration timeout for a part of your code by
 * using this RAII lock object. Create it on the stack and it will
 * automatically restore the old timeout once you exit your block.
 *
 * \code
 *    {
 *        // change duration to at least 5 min.
 *        //
 *        raii_lock_duration_timeout raii_lock_duration_timeout_instance(300);
 *
 *        // create the lock
 *        //
 *        snap_lock lock(object_we_are_to_work_on);
 *
 *        [...do work...]
 *    }
 *    // here the pervious timeout was restored
 * \endcode
 *
 * \attention
 * The new timeout is ignored if smaller than the current timeout.
 *
 * \todo
 * We probably want to offer a way to define the comparison function so
 * that way we can force the change, or make it only if smaller instead
 * of larger.
 *
 * \param[in] temporary_lock_timeout  The new timeout.
 */
raii_lock_duration_timeout::raii_lock_duration_timeout(snap_lock::timeout_t temporary_lock_timeout)
    : f_save_timeout(snap::snap_lock::current_lock_obtention_timeout())
{
    if(temporary_lock_timeout > f_save_timeout)
    {
        snap::snap_lock::initialize_lock_duration_timeout(temporary_lock_timeout);
    }
}


/** \brief The destructor restores the previous timeout value.
 *
 * This destructor restores the lock duration timeout to the value it was
 * set to before you instantiated this object.
 */
raii_lock_duration_timeout::~raii_lock_duration_timeout()
{
    snap::snap_lock::initialize_lock_duration_timeout(f_save_timeout);
}



/** \brief Initialize an RAII lock obtention timeout.
 *
 * You may change the lock obtention timeout for a part of your code by
 * using this RAII lock object. Create it on the stack and it will
 * automatically restore the old timeout once you exit your block.
 *
 * \code
 *    {
 *        // change obtention to at least 5 min.
 *        //
 *        raii_lock_obtention_timeout raii_lock_obtention_timeout_instance(300);
 *
 *        // create the lock
 *        //
 *        snap_lock lock(object_we_are_to_work_on);
 *
 *        [...do work...]
 *    }
 *    // here the pervious timeout was restored
 * \endcode
 *
 * \attention
 * The new timeout is ignored if smaller than the current timeout.
 *
 * \todo
 * We probably want to offer a way to define the comparison function so
 * that way we can force the change, or make it only if smaller instead
 * of larger.
 *
 * \param[in] temporary_lock_timeout  The new timeout.
 */
raii_lock_obtention_timeout::raii_lock_obtention_timeout(snap_lock::timeout_t temporary_lock_timeout)
    : f_save_timeout(snap::snap_lock::current_lock_obtention_timeout())
{
    if(temporary_lock_timeout > f_save_timeout)
    {
        snap::snap_lock::initialize_lock_obtention_timeout(temporary_lock_timeout);
    }
}


/** \brief The destructor restores the previous timeout value.
 *
 * This destructor restores the lock obtention timeout to the value it was
 * set to before you instantiated this object.
 */
raii_lock_obtention_timeout::~raii_lock_obtention_timeout()
{
    snap::snap_lock::initialize_lock_obtention_timeout(f_save_timeout);
}


} // namespace snap
// vim: ts=4 sw=4 et
