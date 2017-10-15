/*
 * Text:
 *      snaplock_ticket.cpp
 *
 * Description:
 *      A daemon to synchronize processes between any number of computers
 *      by blocking all of them but one.
 *
 *      The ticket class is used to know which computer is next. All
 *      computers are given a number. The computer with the smallest
 *      number has priority in case multiple computers are assigned
 *      the same ticket number.
 *
 *      This algorithm is called the Lamport's Bakery Algorithm.
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
#include "snaplock.h"

// our lib
//
#include <snapwebsites/log.h>


/** \class snaplock_ticket
 * \brief Handle the ticket messages.
 *
 * \section Introduction
 *
 * This class manages the Leslie Lamport's Bakery Algorithm (1974) lock
 * mechanism (a critical section that we can get between any number
 * of threads, processes, computers.) Details of this algorithm can
 * be found here:
 *
 *   http://en.wikipedia.org/wiki/Lamport's_bakery_algorithm
 *
 * The algorithm requires:
 *
 * \li A unique name for each computer (server_name)
 * \li A unique number for the process attempting the lock
 *     (see gettid(2) manual)
 * \li A user supplied object name (the name of the lock)
 * \li A ticket number (use the largest existing ticket number + 1)
 *
 * We also include a timeout on any one lock so we can forfeit the
 * lock from happening if it cannot be obtained in a minimal amount
 * of time. The timeout is specified as an absolute time in the
 * future (now + X seconds.) The timeout is given in seconds (a
 * standard time_t value).
 *
 * This class sends various messages to manage the locks.
 *
 *
 * \section The Bakery Algorithm Explained
 *
 * The bakery algorithm is based on the basic idea that a large number
 * of customers go to one bakery to buy bread. In order to make sure
 * they all are served in the order they come in, they are given a ticket
 * with a number. The ticket numbers increase by one for each new customer.
 * The person still in line with the smallest ticket number is served next.
 * Once served, the ticket is destroyed.
 *
 * \note
 * The ticket numbers can restart at one whenever the queue of customers
 * goes empty. Otherwise it only increases. From our usage in Snap, it is
 * really rare that the ticket numbers would not quickly be reset,
 * especially because we have such numbers on a per object_name basis
 * and thus many times the number will actually be one.
 *
 * On a computer without any synchronization mechanism available (our case)
 * two customers may enter the bakery simultaneously (especially since we
 * are working with processes that may run on different computers.) This
 * means two customers may end up with the exact same ticket number and
 * there are no real means to avoid that problem. However, each customer
 * is also assigned two unique numbers on creation: its "host number"
 * (its server name, we use a string to simplify things) and its process
 * number (we actually use gettid() so each thread gets a unique number
 * which is an equivalent to a pid_t number for every single thread.)
 * These two numbers are used to further order processes and make sure
 * we can tell who will get the lock first.
 *
 * So, the basic bakery algorithm looks like this in C++. This algorithm
 * expects memory to be guarded (shared or "volatile"; always visible by
 * all threads.) In our case, we send the data over the network to
 * all the snaplock processes. This is definitely guarded.
 *
 * \code
 *     // declaration and initial values of global variables
 *     namespace {
 *         int                   num_threads = 100;
 *         std::vector<bool>     entering;
 *         std::vector<uint32_t> tickets;
 *     }
 *
 *     // initialize the vectors
 *     void init()
 *     {
 *         entering.reserve(num_threads);
 *         tickets.reserve(num_threads);
 *     }
 *
 *     // i is a thread "number" (0 to 99)
 *     void lock(int i)
 *     {
 *         // get the next ticket
 *         entering[i] = true;
 *         int my_ticket(0);
 *         for(int j(0); j < num_threads; ++j)
 *         {
 *             if(ticket[k] > my_ticket)
 *             {
 *                 my_ticket = ticket[k];
 *             }
 *         }
 *         ++my_ticket; // add 1, we want the next ticket
 *         entering[i] = false;
 *
 *         for(int j(0); j < num_threads; ++j)
 *         {
 *             // wait until thread j receives its ticket number
 *             while(entering[j])
 *             {
 *                 sleep();
 *             }
 *
 *             // there are several cases:
 *             //
 *             // (1) tickets that are 0 are not assigned so we can just go
 *             //     through
 *             //
 *             // (2) smaller tickets win over us (have a higher priority,)
 *             //     so if there is another thread with a smaller ticket
 *             //     sleep a little and try again; that ticket must go to
 *             //     zero to let us through that guard
 *             //
 *             // (3) if tickets are equal, compare the thread numbers and
 *             //     like the tickets, the smallest thread wins
 *             //
 *             while(ticket[j] != 0 && (ticket[j] < ticket[i] || (ticket[j] == ticket[i] && j < i))
 *             {
 *                 sleep();
 *             }
 *         }
 *     }
 *     
 *     // i is the thread number
 *     void unlock(int i)
 *     {
 *         // release our ticket
 *         ticket[i] = 0;
 *     }
 *   
 *     void SomeThread(int i)
 *     {
 *         while(true)
 *         {
 *             [...]
 *             // non-critical section...
 *             lock(i);
 *             // The critical section code goes here...
 *             unlock(i);
 *             // non-critical section...
 *             [...]
 *         }
 *     }
 * \endcode
 *
 * Note that there are two possible optimizations when actually
 * implementing the algorithm:
 *
 * \li You can enter (entering[i] = true), get your ticket,
 *     exit (entering[i] = false) and then get the list of
 *     still existing 'entering' processes. Once that list
 *     goes empty, we do not need to test the entering[j]
 *     anymore because any further entering[j] will be about
 *     processes with a larger ticket number and thus
 *     processes that will appear later in the list of tickets.
 *
 * \li By sorting (and they are) our ticket requests by ticket,
 *     server name, and process pid, we do not have to search
 *     for the smallest ticket. The smallest ticket is automatically
 *     first in that list! So all we have to do is: if not first,
 *     sleep() some more.
 *
 * \note
 * A Cassandra version is proposed on the following page. However,
 * because Cassandra always manages its data with tombstones, you
 * get a very large number of tombstones quickly in your database
 * (at least the CF that manages the lock.) Hence, we have our
 * own deamon which is much faster anyway because it only does work
 * in memory and through the network.
 *
 *   http://wiki.apache.org/cassandra/Locking
 *
 * \note
 * We also have our own Cassandra implementation in our
 * libQtCassandra library which is fully functional (look
 * at version 0.5.22).
 *
 *   http://snapwebsites.org/project/libqtcassandra
 *
 *
 * \section Our implementation in snaplock
 *
 * Locks are given a name by our users. This is used to lock just
 * one small thing for any amount of time as required by your
 * implementation.
 *
 * That name is used as an index to the f_tickets object in the
 * snaplock class. Within such a ticket, you have one entry per
 * process trying to obtain that lock.
 *
 * For example, the users plugin generates a unique user identifier
 * which is a number starting at 1. When a process needs to do this,
 * we need a lock to prevent any other processes to do it at the
 * same time. We also use a QUORUM consistency in Cassandra to
 * load/increment/save the user number.
 *
 * In this example, all we need to lock is an object named something
 * like "user number". Actually, if the number is specific to a
 * website, we can use the website URI. In this case, we can use a
 * name like this: "http://www.example.com/user#number". This says
 * we are managing an atomic "#number" at address
 * "http://www.example.com/user". This also means we do not need
 * to block anyone if the other people need to lock a completely
 * different field (so process A can lock the user unique number
 * while process B could lock an invoice unique number.)
 *
 * As a result, the locking mechanism manages the locks on a per
 * lock name basis. In other words, if only two processes request
 * a lock simultaneously and the object_name parameter are not equal,
 * they both get their lock instantaneously (at least very quickly.)
 *
 * \subsection Message Sequence Chart
 *
 * \msc
 *  Client,SnapLockA,SnapLockB,SnapLockC;
 *
 *  Client->SnapLockA [label="LOCK"];
 *
 *  SnapLockA->SnapLockA [label="LOCKENTERING"];
 *  SnapLockA->SnapLockB [label="LOCKENTERING"];
 *  SnapLockA->SnapLockC [label="LOCKENTERING"];
 *
 *  SnapLockA->SnapLockA [label="LOCKENTERED"];
 *  SnapLockB->SnapLockA [label="LOCKENTERED"];
 *  SnapLockC->SnapLockA [label="LOCKENTERED"];
 *
 *  SnapLockA->SnapLockA [label="GETMAXTICKET"];
 *  SnapLockA->SnapLockB [label="GETMAXTICKET"];
 *  SnapLockA->SnapLockC [label="GETMAXTICKET"];
 *
 *  SnapLockA->SnapLockA [label="MAXTICKET"];
 *  SnapLockB->SnapLockA [label="MAXTICKET"];
 *  SnapLockC->SnapLockA [label="MAXTICKET"];
 *
 *  SnapLockA->SnapLockA [label="ADDTICKET"];
 *  SnapLockA->SnapLockB [label="ADDTICKET"];
 *  SnapLockA->SnapLockC [label="ADDTICKET"];
 *
 *  SnapLockA->SnapLockA [label="TICKETADDED"];
 *  SnapLockB->SnapLockA [label="TICKETADDED"];
 *  SnapLockC->SnapLockA [label="TICKETADDED"];
 *
 *  SnapLockA->SnapLockA [label="LOCKEXITING"];
 *  SnapLockA->SnapLockB [label="LOCKEXITING"];
 *  SnapLockA->SnapLockC [label="LOCKEXITING"];
 *
 *  SnapLockA->Client [label="LOCKED"];
 * \endmsc
 *
 *
 * \section Any drawback?
 *
 * \subsection Timeouts
 *
 * Note that our locks have a timeout, by default it is very small
 * (5 seconds, which for a front end hit to a website is very
 * long already!) If that timeout is too short (i.e. a backend
 * does heavy lifting work on the data,) then you can make it
 * larger. Our backends are given 4h by default.
 *
 * \subsection Deadlock
 *
 * Like with any lock, if you have two processes that both try
 * two distinct locks each in the other order, you get a deadlock:
 *
 * P1 tries to get L1, and gets it;
 *
 * P2 tries to get L2, and gets it;
 *
 * P1 tries to get L2, and has to wait on P2;
 *
 * P2 tries to get L1, and creates a deadlock.
 *
 * The deadlock itself will be resolved once the lock times out,
 * but P2 will "never" have a chance to work on L1.
 *
 * \subsection One lock at a time.
 *
 * The process of obtaining a lock assumes that the process requesting
 * a lock gets blocked between the time it sends the request and the
 * time it receives the confirmation for that lock.
 *
 * This is very important because we manage objects coming from a
 * specific process as unique by using theid PID. If the same process
 * could send more than one lock request, the PID would be the same
 * and if trying to lock the same object twice, you would have a bug
 * because this system does not have any way to distinguish two
 * such requests if received simlutaneously.
 *
 * The lock should look as follow, although we have two implementations
 * one of which does no work in a local place like this because it
 * will be asynchronous.
 *
 * \code
 * {
 *   SnapLock lock("some name");
 *
 *   // do protected work here...
 * }
 * \endcode
 */




/** \brief Initialize a ticket object.
 *
 * The constructor initializes a ticket object by creating a ticket
 * key and allocating an entering object.
 *
 * Once the entering object was acknowledged by QUORUM snaplock
 * instances, we can then create the ticket.
 *
 * \note
 * We create a key from the server name, client PID, and object
 * name for the entering process to run. This key will be unique
 * among all computers assuming (1) your client PID is unique and
 * (2) your servers all have unique names.
 *
 * \note
 * If you use threads, or are likely to use threads, make sure to
 * use the gettid() function instead of getpid() to define a
 * unique client PID.
 *
 * \param[in] server_name  The name of the server generating the locked.
 * \param[in] client_pid  The pid of the client locking the system.
 * \param[in] object_name  The name of the object getting locked.
 * \param[in] timeout  The time when the lock is automatically released
 *                     or times out in seconds.
 * \param[in] service_name  The server waiting for the LOCKED message.
 */
snaplock_ticket::snaplock_ticket(
              snaplock * sl
            , snaplock_messenger::pointer_t messenger
            , QString const & object_name
            , QString const & entering_key
            , time_t obtention_timeout
            , int32_t lock_duration
            , QString const & server_name
            , QString const & service_name)
    : f_snaplock(sl)
    , f_messenger(messenger)
    , f_object_name(object_name)
    , f_obtention_timeout(obtention_timeout)
    , f_lock_duration(lock_duration)
    , f_server_name(server_name)
    , f_service_name(service_name)
    , f_entering_key(entering_key)
{
    SNAP_LOG_TRACE("Attempting to lock \"")
                  (f_object_name)
                  ("\" on \"")
                  (f_entering_key)
                  ("\" for \"")
                  (f_server_name)
                  ("/")
                  (f_service_name)
                  ("\" (timeout: ")
                  (f_obtention_timeout)
                  (").");
}


/** \brief Enter the mode that let us retrieve out ticket number.
 *
 * In order to make sure we can get the current largest ticket number
 * in a unique enough way, we have to enter the lock loop. This is
 * done by sending a LOCKENTERING message to all the snaplock
 * processes currently running.
 */
void snaplock_ticket::entering()
{
    snap::snap_communicator_message entering_message;
    entering_message.set_command("LOCKENTERING");
    entering_message.set_service("*");
    entering_message.add_parameter("object_name", f_object_name);
    entering_message.add_parameter("key", f_entering_key);
    entering_message.add_parameter("timeout", f_obtention_timeout);
    entering_message.add_parameter("duration", f_lock_duration);
    entering_message.add_parameter("source", f_server_name + "/" + f_service_name);
    f_messenger->send_message(entering_message);
}


/** \brief Tell this entering that we received a LOCKENTERED message.
 *
 * This function gets called each time we receive an LOCKENTERED message
 * with this entering key.
 * The function first checks whether the LOCKENTERED message has anything
 * to do with this ticket. If not, the call is just ignored.
 *
 * If the ticket is concerned (same key), then we increase the number of
 * time the object was called and when we reach quorum, we send a
 * GETMAXTICKET event to get the next ticket number.
 */
void snaplock_ticket::entered()
{
    // is this ticket concerned?
    //
    if(!f_get_max_ticket)               // quorum already reached?
    {
        ++f_entered_count;

        if(f_entered_count >= f_snaplock->quorum())
        {
            f_get_max_ticket = true;

            snap::snap_communicator_message add_ticket_message;
            add_ticket_message.set_command("GETMAXTICKET");
            add_ticket_message.set_service("*");
            add_ticket_message.add_parameter("object_name", f_object_name);
            add_ticket_message.add_parameter("key", f_entering_key);
            //add_ticket_message.add_parameter("timeout", f_timeout);
            f_messenger->send_message(add_ticket_message);
        }
    }
}


/** \brief Return the ticket number of this ticket.
 *
 * This function returns the ticket number of this ticket. This
 * is generally used to determine the largest ticket number
 * currently in use in order to assign a new ticket number
 * to a process.
 *
 * By default the value is 0 meaning that no ticket number was
 * yet assigned to that ticket object.
 *
 * \return The current ticket number.
 */
snaplock_ticket::ticket_id_t snaplock_ticket::get_ticket_number() const
{
    return f_our_ticket;
}


/** \brief Get the obtention timeout date.
 *
 * This function returns the obtention timeout. Note that if the lock
 * was already obtained, then this date may be in the past. You can test
 * that by checking the get_lock_timeout() function first.
 *
 * \return The date when the obtention of the ticket timeouts.
 */
time_t snaplock_ticket::get_obtention_timeout() const
{
    return f_obtention_timeout;
}


/** \brief Get the lock timeout date.
 *
 * This function returns the lock timeout. If not yet defined, the
 * function will return zero.
 *
 * \note
 * The ticket will immediately be assigned a timeout date when it
 * gets activated.
 *
 * \return The date when the ticket will timeout or zero.
 */
time_t snaplock_ticket::get_lock_timeout() const
{
    return f_lock_timeout;
}


/** \brief Called whenever a MAXTICKET is received.
 *
 * This function registers the largest ticket number. Once we reach
 * QUORUM, then we have the largest number and we can move on to the
 * next stage, which is to add the ticket.
 *
 * \param[in] new_max_ticket  Another possibly larger ticket.
 */
void snaplock_ticket::max_ticket(int64_t new_max_ticket)
{
    if(!f_added_ticket)
    {
        if(new_max_ticket > f_our_ticket)
        {
            f_our_ticket = new_max_ticket;
        }

        ++f_max_ticket_count;

        if(f_max_ticket_count >= f_snaplock->quorum())
        {
            ++f_our_ticket;

            add_ticket();
        }
    }
}


/** \brief Send the ADDTICKET message.
 *
 * This function sends the ADDTICKET message to all the snaplock
 * instances currently known.
 */
void snaplock_ticket::add_ticket()
{
    // we expect exactly one call to this function
    //
    if(f_added_ticket)
    {
        throw std::logic_error("snaplock_ticket::add_ticket() called more than once.");
    }
    f_added_ticket = true;

    //
    // WARNING: the ticket key MUST be properly sorted by:
    //
    //              ticket number
    //              server name
    //              client pid
    //
    // The client PID does not need to be sorted numerically, just be sorted
    // so one client is before the other.
    //
    // However, the ticket number MUST be numerically sorted. For this reason,
    // since the key is a string, we must add introducing zeroes.
    //
    f_ticket_key = QString("%1/%2").arg(f_our_ticket, 8, 16, QChar('0')).arg(f_entering_key);

    snap::snap_communicator_message add_ticket_message;
    add_ticket_message.set_command("ADDTICKET");
    add_ticket_message.set_service("*");
    add_ticket_message.add_parameter("object_name", f_object_name);
    add_ticket_message.add_parameter("key", f_ticket_key);
    add_ticket_message.add_parameter("timeout", f_obtention_timeout);
    //add_ticket_message.add_parameter("source", f_server_name + "/" + f_service_name); -- done in LOCKENTERING already
    f_messenger->send_message(add_ticket_message);
}


/** \brief Called whenever a MAXTICKET is received.
 *
 * This function registers the largest ticket number. Once we reach
 * QUORUM, then we have the largest number and we can move on to the
 * next stage, which is to add the ticket.
 *
 * \param[in] still_entering  The list of still entering processes
 */
void snaplock_ticket::ticket_added(snaplock_ticket::key_map_t const & still_entering)
{
    if(!f_added_ticket_quorum)
    {
        ++f_ticket_added_count;

        if(f_ticket_added_count >= f_snaplock->quorum())
        {
            f_added_ticket_quorum = true;

            // okay, the ticket was added on all snaplock
            // now we can forget about the entering flag
            // (equivalent to setting it to false)
            //
            exiting();

            f_still_entering = still_entering;
        }
    }
}


/** \brief Exit the ticket.
 *
 * This function sends the LOCKEXITING message asking all to drop
 * the ENTERING that we sent earlier.
 */
void snaplock_ticket::exiting()
{
    snap::snap_communicator_message exiting_message;
    exiting_message.set_command("LOCKEXITING");
    exiting_message.set_service("*");
    exiting_message.add_parameter("object_name", f_object_name);
    exiting_message.add_parameter("key", f_entering_key);
    f_messenger->send_message(exiting_message);
}


/** \brief Call any time time an entering flag is reset.
 *
 * This function gets called whenever an entering flag gets set
 * back to false (i.e. removed in our implementation.)
 *
 * This function knows whether this ticket received its number
 * and is not yet ready. In both of these circumstances, we
 * are waiting for all entering flags that got created while
 * we determined the largest ticket number to be removed.
 */
void snaplock_ticket::remove_entering(QString const & key)
{
    if(f_added_ticket_quorum
    && !f_ticket_ready)
    {
        auto it(f_still_entering.find(key));
        if(it != f_still_entering.end())
        {
            f_still_entering.erase(it);

            // just like the quorum computation, we compute the
            // remaining list of entering tickets dynamically at
            // the time we check the value
            //
            for(auto key_entering(f_still_entering.begin()); key_entering != f_still_entering.end(); )
            {
                if(key_entering->second->timed_out())
                {
                    key_entering = f_still_entering.erase(key_entering);
                }
                else
                {
                    ++key_entering;
                }
            }

            // once all removed, our ticket is ready!
            //
            if(f_still_entering.empty())
            {
                f_ticket_ready = true;
            }
        }
    }
}


/** \brief Check whether this ticket can be activated and do so if so.
 *
 * This function checks whether the ticket is ready to be activated.
 * This means it got a ticket and the ticket is ready. If so, then
 * it sends the LOCKED message back to the system that required it.
 *
 * This function can be called multiple times. It will send
 * the LOCKED message only once.
 */
void snaplock_ticket::activate_lock()
{
    if(f_ticket_ready
    && !f_locked
    && !f_lock_failed
    && !f_service_name.isEmpty()
    && f_server_name == f_snaplock->get_server_name())
    {
        f_locked = true;
        f_lock_timeout = f_lock_duration + time(nullptr);

        snap::snap_communicator_message locked_message;
        locked_message.set_command("LOCKED");
        locked_message.set_server(f_server_name);
        locked_message.set_service(f_service_name);
        locked_message.add_parameter("object_name", f_object_name);
        locked_message.add_parameter("timeout_date", f_lock_timeout);
        locked_message.add_parameter("quorum", f_snaplock->quorum()); // mainly for debug/info so one can know how many computers replied before we said LOCKED
        f_messenger->send_message(locked_message);
    }
}


/** \brief We are done with the ticket.
 *
 * This function sends the DROPTICKET message if the object sent the
 * ADDTICKET at some point.
 */
void snaplock_ticket::drop_ticket()
{
    SNAP_LOG_TRACE("Unlock on \"")(f_object_name)("\" with key \"")(f_entering_key)("\".");

    snap::snap_communicator_message drop_ticket_message;
    drop_ticket_message.set_command("DROPTICKET");
    drop_ticket_message.set_service("*");
    drop_ticket_message.add_parameter("object_name", f_object_name);
    drop_ticket_message.add_parameter("key", f_ticket_key.isEmpty() ? f_entering_key : f_ticket_key);
    f_messenger->send_message(drop_ticket_message);

    if(!f_service_name.isEmpty()
    && f_server_name == f_snaplock->get_server_name())
    {
        // we can immediately say it got unlocked...
        //
        // TODO: this is true ONLY if you lock the same object no more than
        //       once within a session, which is not unlikely false (it is
        //       true for what I can remember of Snap!, but long term this
        //       is not safe.) Like the LOCK, we need a quorum and then
        //       send the UNLOCK... At this point, I'm not too sure how
        //       we implement such because the drop_ticket function ends
        //       up deleting the ticket from memory and thus no counting
        //       can happen after that... (i.e. we need a special case
        //       of the receiver for the UNLOCK, argh!)
        //
        snap::snap_communicator_message unlocked_message;
        unlocked_message.set_command("UNLOCKED");
        unlocked_message.set_server(f_server_name);
        unlocked_message.set_service(f_service_name);
        unlocked_message.add_parameter("object_name", f_object_name);
        f_messenger->send_message(unlocked_message);
    }
}


/** \brief Check whether this ticket timed out.
 *
 * This function returns true if the ticket timed out and should be
 * removed from the various lists where it is kept.
 *
 * The function select the date to check the timeout depending on
 * the current status of the lock. If the lock was successfully
 * activated, the lock timeout date is used. If the lock was not
 * yet activate, the obtention timeout date is used.
 *
 * \return true if the ticket timed out.
 */
bool snaplock_ticket::timed_out() const
{
    // Note: as long as f_locked is false, the f_lock_timeout value is zero
    //
    return (f_locked ? f_lock_timeout : f_obtention_timeout) < time(nullptr);
}


/** \brief Let the service that wanted this lock know that it failed.
 *
 * This function sends a reply to the server that required the lock to
 * let it know that it failed.
 *
 * \note
 * The function may get called multiple times. The failure message
 * is sent only on the first call.
 *
 * \note
 * If the ticket was created on another snaplock (not the one that
 * received the LOCK event in the first place) then the name of
 * the server will be empty and this function does nothing.
 */
void snaplock_ticket::lock_failed()
{
    if(!f_lock_failed
    && !f_service_name.isEmpty()
    && f_server_name == f_snaplock->get_server_name())
    {
        // send that message at most once
        f_lock_failed = true;

        if(f_locked)
        {
            // if we were locked and reach here, then the lock
            // timed out while locked
            //
            SNAP_LOG_INFO("Lock on \"")(f_object_name)("\" with key \"")(f_entering_key)("\" timed out.");

            snap::snap_communicator_message lock_failed_message;
            lock_failed_message.set_command("UNLOCKED");
            lock_failed_message.set_server(f_server_name);
            lock_failed_message.set_service(f_service_name);
            lock_failed_message.add_parameter("object_name", f_object_name);
            lock_failed_message.add_parameter("error", "timedout");
            f_messenger->send_message(lock_failed_message);
        }
        else
        {
            SNAP_LOG_INFO("Lock on \"")(f_object_name)("\" with key \"")(f_entering_key)("\" failed.");

            snap::snap_communicator_message lock_failed_message;
            lock_failed_message.set_command("LOCKFAILED");
            lock_failed_message.set_server(f_server_name);
            lock_failed_message.set_service(f_service_name);
            lock_failed_message.add_parameter("object_name", f_object_name);
            lock_failed_message.add_parameter("error", "failed");
            f_messenger->send_message(lock_failed_message);
        }
    }
}


/** \brief Retrieve the object name of this ticket.
 *
 * This function returns the name of the object associated with this
 * lock (i.e. what is being locked.)
 *
 * \return The object name of the ticket.
 */
QString const & snaplock_ticket::get_object_name() const
{
    return f_object_name;
}


/** \brief Retrieve a reference to the entering key of this ticket.
 *
 * This function returns the entering key of this ticket. The
 * entering key is defined on instantiation so it is always available.
 *
 * \note
 * By contrast, the ticket key is not available up until the time the
 * ticket number is marked as valid. Hence, at this time we do not
 * offer a function to retrieve that other key.
 *
 * \return The entering key of this ticket.
 */
QString const & snaplock_ticket::get_entering_key() const
{
    return f_entering_key;
}


// vim: ts=4 sw=4 et
