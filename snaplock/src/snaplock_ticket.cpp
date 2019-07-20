/*
 * Text:
 *      snaplock/src/snaplock_ticket.cpp
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


// ourselves
//
#include "snaplock.h"


// our lib
//
#include <snapwebsites/log.h>


// last include
//
#include <snapdev/poison.h>



namespace snaplock
{

/** \class snaplock_ticket
 * \brief Handle the ticket messages.
 *
 * \section introduction Introduction
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
 * \section bakery_algorithm The Bakery Algorithm Explained
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
 *   https://snapwebsites.org/project/libqtcassandra
 *
 *
 * \section implementation Our implementation in snaplock
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
 * \subsection message_sequence Message Sequence Chart
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
 * \section drawback Any drawback?
 *
 * \subsection timeouts Timeouts
 *
 * Note that our locks have a timeout, by default it is very small
 * (5 seconds, which for a front end hit to a website is very
 * long already!) If that timeout is too short (i.e. a backend
 * does heavy lifting work on the data,) then you can make it
 * larger. Our backends are given 4h by default.
 *
 * \subsection deadlock Deadlock
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
 * \subsection one_lock One lock at a time.
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
 * instances (i.e. one other computer when we allow exactly 3 leaders,)
 * we can then create the ticket.
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
 * \param[in] sl  A pointer to the snaplock object.
 * \param[in] messenger  A pointer to the messenger.
 * \param[in] object_name  The name of the object getting locked.
 * \param[in] entering_key  The key (ticket) used to entery the bakery.
 * \param[in] obtension_timeout  The time when the attempt to get the lock
 *                               times out in seconds.
 * \param[in] lock_duration  The amount of time the lock lasts once obtained.
 * \param[in] server_name  The name of the server generating the locked.
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
    , f_owner(f_snaplock->get_server_name())
    , f_entering_key(entering_key)
{
    // clamp the lock duration
    //
    if(f_lock_duration < snap::snap_lock::SNAP_UNLOCK_MINIMUM_TIMEOUT)
    {
        f_lock_duration = snap::snap_lock::SNAP_UNLOCK_MINIMUM_TIMEOUT;
    }
    else if(f_lock_duration > snap::snap_lock::SNAP_MAXIMUM_TIMEOUT)
    {
        f_lock_duration = snap::snap_lock::SNAP_MAXIMUM_TIMEOUT;
    }

    set_unlock_duration(f_lock_duration);

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


/** \brief Send a message to the other two leaders.
 *
 * The send_message() is "broadcast" to the other two leaders.
 *
 * This is a safe guard so if one of our three leaders fails, we have
 * a backup of the lock status.
 *
 * The locking system also works if there are only two or even just one
 * computer. In those cases, special care has to be taken to get things
 * to work as expected.
 *
 * \param[in] message  The message to send to the other two leaders.
 *
 * \return true if the message was forwarded at least once, false otherwise.
 */
bool snaplock_ticket::send_message_to_leaders(snap::snap_communicator_message & message)
{
    // finish the message initialization
    //
    message.set_service("snaplock");
    message.add_parameter("object_name", f_object_name);

    snaplock::computer_t::pointer_t leader(f_snaplock->get_leader_a());
    if(leader != nullptr)
    {
        // there are at least two leaders
        //
        message.set_server(leader->get_name());
        f_messenger->send_message(message);

        // check for a third leader
        //
        leader = f_snaplock->get_leader_b();
        if(leader != nullptr)
        {
            message.set_server(leader->get_name());
            f_messenger->send_message(message);
        }

        // we have to wait for at least one reply
        //
        return true;
    }

    // there is only one leader (ourselves)
    //
    // verify that this is correct otherwise we would mess up the algorithm
    //
    return f_snaplock->get_computer_count() != 1;
}


/** \brief Enter the mode that lets us retrieve our ticket number.
 *
 * In order to make sure we can get the current largest ticket number
 * in a unique enough way, snaplock has to enter the lock loop. This
 * process starts by sending a `LOCKENTERING` message to all the
 * other snaplock leaders.
 */
void snaplock_ticket::entering()
{
    // TODO implement the special case when there is only 1 leader
    //      (on the other hand, that should be rather rare)
    //snaplock::computer_t::pointer_t leader(f_snaplock->get_leader_a());
    //if(leader == nullptr)
    //{
    //    -- do the necessary to obtain the lock --
    //    return;
    //}

    snap::snap_communicator_message entering_message;
    entering_message.set_command("LOCKENTERING");
    entering_message.add_parameter("key", f_entering_key);
    entering_message.add_parameter("timeout", f_obtention_timeout);
    entering_message.add_parameter("duration", f_lock_duration);
    if(f_lock_duration != f_unlock_duration)
    {
        entering_message.add_parameter("unlock_duration", f_unlock_duration);
    }
    entering_message.add_parameter("source", f_server_name + "/" + f_service_name);
    entering_message.add_parameter("serial", f_serial);
    if(!send_message_to_leaders(entering_message))
    {
        // there are no other leaders, make sure the algorithm progresses
        //
        entered();
    }
}


/** \brief Tell this entering that we received a LOCKENTERED message.
 *
 * This function gets called each time we receive a `LOCKENTERED`
 * message with this ticket entering key.
 *
 * Since we have 1 to 3 leaders, the quorum and thus consensus is reached
 * as soon as we receive one `LOCKENTERED` message. So as a result this
 * function sends `GETMAXTICKET` the first time it gets called. The
 * `GETMAXTICKET` message allows us to determine the ticket number for
 * the concerned object.
 *
 * \note
 * The msg_lockentered() function first checked whether the
 * `LOCKENTERED` message had anything to do with this ticket.
 * If not, the message was just ignored.
 */
void snaplock_ticket::entered()
{
    // is this ticket concerned?
    //
    if(!f_get_max_ticket)
    {
        // with 2 or 3 leaders, quorum is obtain with one
        // single acknowledgement
        //
        f_get_max_ticket = true;

        // calculate this instance max. ticket number
        //
        f_our_ticket = f_snaplock->get_last_ticket(f_object_name);

        snap::snap_communicator_message get_max_ticket_message;
        get_max_ticket_message.set_command("GETMAXTICKET");
        get_max_ticket_message.add_parameter("key", f_entering_key);
        if(!send_message_to_leaders(get_max_ticket_message))
        {
            // there are no other leaders, make sure the algorithm progresses
            //
            max_ticket(f_our_ticket);
        }
    }
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

        ++f_our_ticket;

        add_ticket();
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

    f_snaplock->set_ticket(f_object_name, f_ticket_key, shared_from_this());

    snap::snap_communicator_message add_ticket_message;
    add_ticket_message.set_command("ADDTICKET");
    add_ticket_message.add_parameter("key", f_ticket_key);
    add_ticket_message.add_parameter("timeout", f_obtention_timeout);
    if(!send_message_to_leaders(add_ticket_message))
    {
        ticket_added(f_snaplock->get_entering_tickets(f_object_name));
    }
}


/** \brief Called whenever a TICKETADDED is received.
 *
 * This function sends a LOCKEXITING if the ticket reached the total number
 * of TICKETADDED required to get a quorum (which is just one with 1 to 3
 * leaders.)
 *
 * The \p still_entering paramater defines the list of tickets that are
 * still trying to enter the same object. This is very important. It needs
 * to be completely drained before we can proceed and mark the ticket as
 * assigned.
 *
 * \param[in] still_entering  The list of still entering processes
 */
void snaplock_ticket::ticket_added(snaplock_ticket::key_map_t const & still_entering)
{
    if(!f_added_ticket_quorum)
    {
        // when we have 2 or 3 leaders, quorum is obtain with one
        // single acknowledgement
        //
        f_added_ticket_quorum = true;

        f_still_entering = still_entering;

        // okay, the ticket was added on all snaplock
        // now we can forget about the entering flag
        // (equivalent to setting it to false)
        //
        snap::snap_communicator_message exiting_message;
        exiting_message.set_command("LOCKEXITING");
        exiting_message.add_parameter("key", f_entering_key);
        snap::NOTUSED(send_message_to_leaders(exiting_message));

        f_snaplock->lock_exiting(exiting_message);
    }
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

                // let the other two leaders know that the ticket is ready
                //
                snap::snap_communicator_message ticket_ready_message;
                ticket_ready_message.set_command("TICKETREADY");
                ticket_ready_message.add_parameter("key", f_ticket_key);
                snap::NOTUSED(send_message_to_leaders(ticket_ready_message));
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
    && !f_lock_failed)
    {
        snap::snap_communicator_message activate_lock_message;
        activate_lock_message.set_command("ACTIVATELOCK");
        activate_lock_message.add_parameter("key", f_ticket_key);
        if(!send_message_to_leaders(activate_lock_message))
        {
            lock_activated();
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
void snaplock_ticket::lock_activated()
{
    if(f_ticket_ready
    && !f_locked
    && !f_lock_failed)
    {
        f_locked = true;
        f_lock_timeout = f_lock_duration + time(nullptr);

        if(f_owner == f_snaplock->get_server_name())
        {
            snap::snap_communicator_message locked_message;
            locked_message.set_command("LOCKED");
            locked_message.set_server(f_server_name);
            locked_message.set_service(f_service_name);
            locked_message.add_parameter("object_name", f_object_name);
            locked_message.add_parameter("timeout_date", f_lock_timeout);
            f_messenger->send_message(locked_message);
        }
    }
}


/** \brief We are done with the ticket.
 *
 * This function sends the DROPTICKET message to get read of a ticket
 * from another leader's list of tickets.
 *
 * Another leader has a list of tickets as it receives LOCK and ADDTICKET
 * messages.
 */
void snaplock_ticket::drop_ticket()
{
    SNAP_LOG_TRACE("Unlock on \"")(f_object_name)("\" with key \"")(f_entering_key)("\".");

    snap::snap_communicator_message drop_ticket_message;
    drop_ticket_message.set_command("DROPTICKET");
    drop_ticket_message.add_parameter("key", f_ticket_key.isEmpty() ? f_entering_key : f_ticket_key);
    send_message_to_leaders(drop_ticket_message);

    if(!f_lock_failed)
    {
        f_lock_failed = true;

        //if(f_owner == f_snaplock->get_server_name()) -- this can happen with any leader so we have to send the UNLOCKED
        //                                                the other leaders won't call this function they receive DROPTICKET
        //                                                instead and as mentioned in the TODO below, we should get a QUORUM
        //                                                instead...
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
}


/** \brief Let the service that wanted this lock know that it failed.
 *
 * This function sends a reply to the server that requested the lock to
 * let it know that it somehow failed.
 *
 * The function replies with a LOCKFAILED when the lock was never
 * obtained. In this case the origin server cannot access the resources.
 *
 * The function rep[ies with UNLOCKED when the lock timed out. The
 * server is expected to send an UNLOCK reply to acknowledge the
 * failure and fully release the lock. The lock will remain in place
 * until that acknowledgement is received or an amount of time
 * equal to the lock duration by default with a minimum of 1 minute.
 *
 * By default, the UNLOCKED acknowledgement timeout is set to the same
 * amount as the LOCK duration with a minimum of 60 seconds. It can
 * also be specified with the unlock_duration parameter in the LOCK
 * message.
 *
 * \note
 * The function may get called multiple times. The failure message
 * is sent only on the first call.
 *
 * \note
 * If the ticket was created on another snaplock (not the one that
 * received the LOCK event in the first place) then this ticket is
 * not marked as being owned by this snaplock and as a result this
 * function only marks the ticket as failed.
 */
void snaplock_ticket::lock_failed()
{
    if(!f_lock_failed)
    {
        // send that message at most once
        //
        f_lock_failed = true;

        if(f_locked)
        {
            // now we have to extend the lock timeout to make sure that
            // the UNLOCKED has a chance to be acknowledged
            //
            f_lock_timeout += f_unlock_duration;
        }

        if(f_owner == f_snaplock->get_server_name())
        {
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
}


/** \brief Define whether this ticket is the owner of that lock.
 *
 * Whenever comes time to send the LOCK, UNLOCK, or LOCKFAILED messages,
 * only the owner is expected to send it. This flag tells us who the
 * owner is and thus who is responsible for sending that message.
 *
 * \todo
 * The ownership has to travel to others whenever a leader disappears.
 *
 * \param[in] owner  The name of this ticket owner.
 */
void snaplock_ticket::set_owner(QString const & owner)
{
    f_owner = owner;
}


/** \brief Return the name of this ticket's owner.
 *
 * This function returns the name of the owner of this ticket. When a
 * leader dies out, its name stick around until a new leader gets
 * assigned to it.
 *
 * \return  The name of this ticket owner.
 */
QString const & snaplock_ticket::get_owner() const
{
    return f_owner;
}


/** \brief Retrieve the client process identifier.
 *
 * This function splits the entering key and return the process identifier.
 * This is primarily used to resend a LOCK message since in most cases
 * this information should not be required.
 *
 * \note
 * This is not really information that the ticket is supposed to know about
 * but well... there is now a case where we need to know this.
 *
 * \return The name of this ticket owner.
 */
pid_t snaplock_ticket::get_client_pid() const
{
    snap::snap_string_list const segments(f_entering_key.split('/'));
    if(segments.size() != 2)
    {
        throw snaplock_exception_content_invalid_usage(
                  "snaplock_ticket::get_owner() split f_entering_key "
                + f_entering_key
                + " and did not get exactly two segments.");
    }
    bool ok(false);
    return segments[1].toInt(&ok, 10);
}


/** \brief Give the lock a serial number for some form of unicity.
 *
 * When we lose a leader, the unicity of the ticket may be required as we
 * start sharing the tickets between the surviving leaders. This is done
 * for the RELOCK message which attempts to restart the an old LOCK. In
 * that case, two leaders end up attempt a RELOCK on the same ticket.
 * To make sure that we can easily ignore the second attempt, we use
 * the serial number to see that the exact same message is getting there
 * twice.
 *
 * The snaplock daemon uses the leader number as part of the serial
 * number (bits 24 and 25) so it is unique among all the instances,
 * at least until a snaplock deamon dies and its unique numbers get
 * mingled (and the old leaders may change their own number too...)
 *
 * \param[in] serial  The serial number of the ticket.
 */
void snaplock_ticket::set_serial(serial_t serial)
{
    f_serial = serial;
}


/** \brief Return the serial number of this ticket.
 *
 * This function returns the serial number of this ticket. See the
 * set_serial() function for additional information about this number.
 *
 * \return  The serial number of the ticket.
 */
snaplock_ticket::serial_t snaplock_ticket::get_serial() const
{
    return f_serial;
}


/** \brief Change the lock duration to the specified value.
 *
 * If the service requesting a lock fails to acknowledge an unlock, then
 * the lock still gets unlocked after this number of seconds.
 *
 * By default, this parameter gets set to the same value as duration with
 * a minimum of 60. When the message includes an `unlock_duration` parameter
 * then that value is used instead.
 *
 * \note
 * If \p duration is less than SNAP_UNLOCK_MINIMUM_TIMEOUT,
 * then SNAP_UNLOCK_MINIMUM_TIMEOUT is used. At time of writing
 * SNAP_UNLOCK_MINIMUM_TIMEOUT is 60 seconds or one minute.
 *
 * \warning
 * It is important to understand that as soon as an UNLOCKED event arrives,
 * you should acknowledge it if it includes an "error" parameter. Not
 * doing so increases the risk that two or more processes access the same
 * resource simultaneously.
 *
 * \param[in] duration  The number of seconds to acknowledge an UNLOCKED
 *            event; after that the lock is released no matter what.
 */
void snaplock_ticket::set_unlock_duration(int32_t duration)
{
    if(duration == snap::snap_lock::SNAP_UNLOCK_USES_LOCK_TIMEOUT)
    {
        duration = f_lock_duration;
    }

    if(duration < snap::snap_lock::SNAP_UNLOCK_MINIMUM_TIMEOUT)
    {
        f_unlock_duration = snap::snap_lock::SNAP_UNLOCK_MINIMUM_TIMEOUT;
    }
    else if(duration > snap::snap_lock::SNAP_MAXIMUM_TIMEOUT)
    {
        f_unlock_duration = snap::snap_lock::SNAP_MAXIMUM_TIMEOUT;
    }
    else
    {
        f_unlock_duration = duration;
    }
}


/** \brief Get unlock duration.
 *
 * The unlock duration is used in case the lock times out. It extends
 * the lock duration for that much longer until the client acknowledge
 * the locks or the lock really times out.
 *
 * \return The unlock acknowledgement timeout duration.
 */
snap::snap_lock::timeout_t snaplock_ticket::get_unlock_duration() const
{
    return f_unlock_duration;
}


/** \brief Set the ticket number.
 *
 * The other two leaders receive the ticket number in the ADDTICKET
 * message. That number must be saved in the ticket, somehow. This
 * is the function we use to do that.
 *
 * It is very important to have the correct number (by default it is
 * zero) since the algorithm asks for the maximum ticket number
 * currently available and without that information that request
 * cannot be answered properly.
 *
 * \param[in] number  The ticket number to save in f_our_ticket.
 */
void snaplock_ticket::set_ticket_number(ticket_id_t number)
{
    if(f_our_ticket != 0)
    {
        throw std::logic_error("snaplock_ticket::set_ticket_number() called with "
                + std::to_string(number)
                + " when f_our_ticket is already set to "
                + std::to_string(f_our_ticket)
                + ".");
    }
    if(f_added_ticket)
    {
        throw std::logic_error("snaplock_ticket::set_ticket_number() called when f_added_ticket is already true.");
    }
    f_added_ticket = true;

    f_our_ticket = number;
    f_ticket_key = QString("%1/%2").arg(f_our_ticket, 8, 16, QChar('0')).arg(f_entering_key);
}


/** \brief Mark the ticket as being ready.
 *
 * This ticket is marked as being ready.
 *
 * A ticket is ready when all the entering tickets were removed from it
 * on the owning leader. On the other two leaders, the ticket gets marked
 * as being ready once they receive the LOCKEXITING message.
 */
void snaplock_ticket::set_ready()
{
    f_ticket_ready = true;
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


/** \brief Check whether this ticket is locked or not.
 *
 * This function returns true if the ticket is currently locked.
 *
 * \return true when the ticket was successfully locked at some point.
 */
bool snaplock_ticket::is_locked() const
{
    return f_locked;
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


/** \brief Define a time when the ticket times out while waiting.
 *
 * This function defines the time threshold when to timeout this
 * ticket in case a service does not reply to an ALIVE message.
 *
 * Whenever a leader dies, a ticket which is not locked yet may be
 * transferred to another leader. To not attempt to lock a ticket
 * for nothing, the new leader first checks that the service
 * which requested that lock is indeed still alive by send an
 * ALIVE message to it. In return it expects an ABSOLUTELY
 * reply.
 *
 * If the ABSOLUTELY reply does not make it in time (at this time
 * we limit this to 5 seconds) then we consider that this service
 * is not responsive and we cancel the lock altogether.
 *
 * To cancel this timeout, call the function with 0 in \p timeout.
 *
 * \note
 * Since that message should happen while the snap_lock object
 * is wait for the LOCK event, the reply should be close to
 * instantaneous. So 5 seconds is plenty until somehow your
 * network is really busy or really large and the time for
 * the message to travel is too long.
 *
 * \param[in] timeout  The time when the ALIVE message times out.
 */
void snaplock_ticket::set_alive_timeout(time_t timeout)
{
    if(timeout < 0)
    {
        timeout = 0;
    }

    if(timeout < f_obtention_timeout)
    {
        f_alive_timeout = timeout;
    }
    else
    {
        // use the obtension timeout if smaller because that was the
        // first premise that the client asked about
        //
        f_alive_timeout = f_obtention_timeout;
    }
}


/** \brief Retrieve the lock duration.
 *
 * This function returns the lock duration in seconds as defined with
 * the constructor.
 *
 * \return The lock duration in seconds.
 */
snap::snap_lock::timeout_t snaplock_ticket::get_lock_duration() const
{
    return f_lock_duration;
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


/** \brief Get the current lock timeout date.
 *
 * This function returns the current lock timeout.
 *
 * If the lock is being re-requested (after the loss of a leader) then
 * the ALIVE timeout may be returned for a short period of time.
 *
 * If the lock was not yet obtained, this function returns the obtension
 * timeout timestamp. Once the lock was obtained, the lock timeout gets
 * defined and that one is returned instead.
 *
 * \note
 * This is the date used in the timed_out() function.
 *
 * \return The date when the ticket will timeout or zero.
 */
time_t snaplock_ticket::get_current_timeout() const
{
    if(f_alive_timeout > 0)
    {
        return f_alive_timeout;
    }

    if(f_locked)
    {
        return f_lock_timeout;
    }

    return f_obtention_timeout;
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
    return get_current_timeout() <= time(nullptr);
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


/** \brief Retrieve the server name of this ticket.
 *
 * This function returns the name of the server associated with this
 * lock, i.e. the server to which the LOCKED and UNLOCKED commands are to
 * be sent back to.
 *
 * This name is also used in case of an error to send the LOCKFAILED back
 * to the service that requested the lock.
 *
 * \return The server name of the ticket.
 */
QString const & snaplock_ticket::get_server_name() const
{
    return f_server_name;
}


/** \brief Retrieve the service name of this ticket.
 *
 * This function returns the name of the service associated with this
 * lock. This is the service to which the LOCKED and UNLOCKED messages
 * are sent.
 *
 * This name is also used in case of an error to send the LOCKFAILED back
 * to the service that requested the lock.
 *
 * \return The service name of the ticket.
 */
QString const & snaplock_ticket::get_service_name() const
{
    return f_service_name;
}


/** \brief Retrieve a reference to the entering key of this ticket.
 *
 * This function returns the entering key of this ticket. The
 * entering key is defined on instantiation so it is always available.
 *
 * \note
 * By contrast, the ticket key is not available up until the time the
 * ticket number is marked as valid.
 *
 * \return The entering key of this ticket.
 */
QString const & snaplock_ticket::get_entering_key() const
{
    return f_entering_key;
}


/** \brief Retrieve a reference to the ticket key.
 *
 * This function returns the ticket key of this ticket. The
 * ticket key is only defined at a later time when the ticket has
 * properly entered the bakery. It includes three parameters:
 *
 * \li Ticket number as an hexadecimal number of 8 digits,
 * \li Server name of the server asking for the lock,
 * \li Process Identifier (PID) of the service daemon asking for the lock.
 *
 * \note
 * This function returns an empty string until the ticket key is available.
 *
 * \return The ticket key.
 */
QString const & snaplock_ticket::get_ticket_key() const
{
    return f_ticket_key;
}


/** \brief Serialize a ticket to send it over to another leader.
 *
 * This function serialize a ticket to share it with the other 
 * leaders. This is important when a new leader gets elected
 * as it would not otherwise have any idea of what the existing
 * tickets are, although it is not 100% important, if another
 * of the two snaplock was to go down, it becomes primordial
 * for the tickets to be known in the other leaders.
 *
 * This is used at the start before a leader starts accepting new
 * lock requests.
 *
 * \return This ticket as a serialized string.
 *
 * \sa unserialize()
 */
QString snaplock_ticket::serialize() const
{
    std::map<QString, QString> data;

    data["object_name"]         = f_object_name;
    data["obtention_timeout"]   = QString("%1").arg(f_obtention_timeout);
    //data["alive_timeout"]       = QString("%1").arg(f_alive_timeout); -- we do not want to transfer this one
    data["lock_duration"]       = QString("%1").arg(f_lock_duration);
    data["unlock_duration"]     = QString("%1").arg(f_unlock_duration);
    data["server_name"]         = f_server_name;
    data["service_name"]        = f_service_name;
    data["owner"]               = f_owner;
    if(f_serial != NO_SERIAL)
    {
        data["serial"]          = QString("%1").arg(f_serial);
    }
    data["entering_key"]        = f_entering_key;
    data["get_max_ticket"]      = f_get_max_ticket ? "true" : "false";
    data["our_ticket"]          = QString("%1").arg(f_our_ticket);
    data["added_ticket"]        = f_added_ticket ? "true" : "false";
    data["ticket_key"]          = f_ticket_key;
    data["added_ticket_quorum"] = f_added_ticket_quorum ? "true" : "false";

    // this is a map
    //names["still_entering"]   = f_still_entering;
    //snaplock_ticket::key_map_t      f_still_entering = snaplock_ticket::key_map_t();

    data["ticket_ready"]        = f_ticket_ready ? "true" : "false";
    data["locked"]              = f_locked ? "true" : "false";
    data["lock_timeout"]        = QString("%1").arg(f_lock_timeout);
    data["lock_failed"]         = f_lock_failed ? "true" : "false";

    QString result;
    for(auto & it : data)
    {
        result += it.first;
        result += QChar('=');
        it.second.replace("|", "%7C");  // make sure the value does not include any ';'
        result += it.second;
        result += QChar('|');
    }

    return result;
}


/** \brief Unserialize a ticket string back to a ticket object.
 *
 * This function unserialize a string that was generated using the
 * serialize() function.
 *
 * Note that unknown fields are ignored and none of the fields are
 * considered mandatory. Actually the function generates no errors.
 * This means it should be forward compatible.
 *
 * The data gets unserialized in `this` object.
 *
 * \param[in] data  The serialized data.
 */
void snaplock_ticket::unserialize(QString const & data)
{
    bool ok(false);
    snap::snap_string_list const vars(data.split('|'));
    for(auto const & d : vars)
    {
        int const pos(d.indexOf('='));
        QString const name(d.mid(0, pos));
        QString const value(d.mid(pos + 1));
        if(name == "object_name")
        {
#ifdef _DEBUG
            if(f_object_name != value)
            {
                throw std::logic_error(
                            "snaplock_ticket::unserialize() not unserializing object name \""
                            + std::string(value.toUtf8().data())
                            + "\" over itself \""
                            + std::string(f_object_name.toUtf8().data())
                            + "\" (object name mismatch)."
                        );
            }
#endif
            f_object_name = value;
        }
        else if(name == "obtention_timeout")
        {
            f_obtention_timeout = value.toLong(&ok, 10);
        }
        //else if(name == "alive_timeout") -- we do not transfer this one (not required, and could actually cause problems)
        //{
        //    f_alive_timeout = value.toLong(&ok, 10);
        //}
        else if(name == "lock_duration")
        {
            f_lock_duration = value.toLong(&ok, 10);
        }
        else if(name == "unlock_duration")
        {
            f_unlock_duration = value.toLong(&ok, 10);
        }
        else if(name == "server_name")
        {
            f_server_name = value;
        }
        else if(name == "service_name")
        {
            f_service_name = value;
        }
        else if(name == "owner")
        {
            f_owner = value;
        }
        else if(name == "serial")
        {
            f_serial = value.toLong(&ok, 10);
        }
        else if(name == "entering_key")
        {
#ifdef _DEBUG
            if(f_entering_key != value)
            {
                throw std::logic_error(
                            "snaplock_ticket::unserialize() not unserializing entering key \""
                            + std::string(value.toUtf8().data())
                            + "\" over itself \""
                            + std::string(f_entering_key.toUtf8().data())
                            + "\" (entering key mismatch)."
                        );
            }
#endif
            f_entering_key = value;
        }
        else if(name == "get_max_ticket")
        {
            f_get_max_ticket = f_get_max_ticket || value == "true";
        }
        else if(name == "our_ticket")
        {
            f_our_ticket = value.toLong(&ok, 10);
        }
        else if(name == "added_ticket")
        {
            f_added_ticket = f_added_ticket || value == "true";
        }
        else if(name == "ticket_key")
        {
            f_ticket_key = value;
        }
        else if(name == "added_ticket_quorum")
        {
            f_added_ticket_quorum = f_added_ticket_quorum || value == "true";
        }

        // this is a map
        //names["still_entering"]   = f_still_entering;
        //snaplock_ticket::key_map_t      f_still_entering = snaplock_ticket::key_map_t();

        else if(name == "ticket_ready")
        {
            f_ticket_ready = f_ticket_ready || value == "true";
        }
        else if(name == "locked")
        {
            f_locked = f_locked || value == "true";
        }
        else if(name == "lock_timeout")
        {
            // the time may be larger because of an UNLOCK so we keep
            // the largest value
            //
            time_t const timeout(value.toLong(&ok, 10));
            if(timeout > f_lock_timeout)
            {
                f_lock_timeout = timeout;
            }
        }
        else if(name == "lock_failed")
        {
            f_lock_failed = f_lock_failed || value == "true";
        }
    }
}


}
// snaplock namespace
// vim: ts=4 sw=4 et
