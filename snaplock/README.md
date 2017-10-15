
Inter Computer Exclusive Lock
=============================

The `snaplock` tool is used to quickly obtain locks between computers.

The implementation makes uses of two technologies for very high availability:

1. The [Lamport's bakery algorithm](https://en.wikipedia.org/wiki/Lamport's_bakery_algorithm)
which allows us to obtain the exclusive lock itself.

2. The QUORUM voting system which allows us to lose any number of computers
participating to the bakery algorithm and still get locks.


Lamport's bakery algorithm
==========================

This algorithm is shown on Wikipedia. However, we have two major differences:

1. We use the algorithm to lock between any number of computers, not just
one computer with multiple threads. Our algorithm is used to lock processes
and make sure that we do not spinlock or poll at any point (i.e. we make
a request for a lock, then wait by falling asleep, then get awaken either
because the lock was obtained or because the lock could not be obtained
in time--or in some cases because something else failed.)

2. The algorithm is programmed to be elastic. This means the number of
computers that can be locked can dynamically grow and shrink. (i.e. The
default algorithm has an assigned number of processes it can lock and
unlock.)

The one important part for the algorithm to function properly is to make
sure that each computer has a different name. The index (`j` in the
Wikipedia example) must be unique and we use the computer name for that
purpose.

In the Snap! environment, the `snapcommunicator` daemon is responsible
for connecting computers together. If a `snapcommunicator` receives
two requests from two different computers to `CONNECT` themselves to
it and both have the same name, the second connection is always refused.


The QUORUM Agreement
====================

Whenever someone wants to obtain a lock, various steps have to be taken
as shown in the Lamport's bakery algorithm. Those steps change the value
of a Boolean (`Entering`) or an integer (`Number`).

Since our implementation has one constraint: continue to function without
interruption even when a computer goes down, we use a voting system
between the `snaplock` daemons that are currently running.

This works in a way which is very similar to the Cassandra consistency
level named QUORUM. We expect at least `N / 2 + 1` computers to accept
a message before we can go on. By accepting the message, a computer
writes the value we sent to it and replies to the message with an
acknoledgement. Once we get at least `N / 2 + 1` acknowledgement replies,
we can move on to the next step.

For example, the `ENTERING` message is sent by the `snaplock` daemon
requiring a lock. This marks the specified computer and lock (each lock
is given a "name" which most often is a URI) as `ENTERING`. So `snaplock`
broadcast the message to all the other `snaplock` it knows about and waits
for them to reply with `ENTERED`. When they do so, they also save an
object saying that this specific computer and lock are being entered.
Once the first `snaplock` received enough `ENTERED` messages, it views
that one lock request as entered and it can start the next process:
retrieve the largest ticket number. This works in a similar fashion,
by sending another event and waiting for its acknowledgement.

Note that the current implementation (As of Oct 4, 2016) is not 100%
safe because it does not record who answered each messages. We cannot
hope that if A acknowledges message 1 and then B acknowledge message 2,
then everything is good. We need A to continue to acknowledge our
messages and we must ignore B if it never acknowledge message 1. This
will be fixed in a later version.


Semaphore Support
=================

At some point we want to add support for semaphores. This will allow
us to cleanly drain all requests to a website by creating a semaphore
that can either be locked in `READ-ONLY` mode (normal client access) or
in `READ/WRITE` mode.

Each client accessing a website through the `snap_child` class will
have to obtain a `READ-ONLY` semaphore. If that fails, then someone
else is running a process in `READ/WRITE` mode and we have to refuse
the session altogether (i.e. switch to showing a static page saying
something like "site is under maintenance").

Backend processes that change the database in ways that are not
compatible to inter-process processing, namely, the `snapinstallwebsite`
process, will request a `READ/WRITE` access to the semaphore. This
will prevent any further `READ-ONLY` requests from being accepted
until the `READ/WRITE` process is done.

We want to look into offering several ways to enter the `READ/WRITE`
mode in order to make it nicer:

1. hard locking--in this mode a `READ/WRITE` request is given a spot
in the list of incoming locks. It blocks any further `READ-ONLY`
locks so it gets its turn as fast as possible (as soon as all the
existing `READ-ONLY` are done.) This is the default.

2. soft locking--in this mode, the `READ/WRITE` lock does not prevent
further clients from obtaining the `READ-ONLY` lock. This means the
`READ/WRITE` may not happen for some time (it does not lock its position
in the lock FIFO.)

3. timed soft locking--like the _soft locking_ but further waits for a
little while after the lock is obtainable before trying to obtain the
lock. The idea behind this one is to allow a client to finish browsing,
because one we get the `READ/WRITE` lock, the client won't be able to
access the site for a while. The wait is calculated from the last
`READ-ONLY` lock to the new `READ/WRITE` lock. It could be as long
as 10 minutes. If no other `READ-ONLY` happened within 10 minutes,
then go ahead and try to obtain the `READ/WRITE` lock and do your work.

Any of these locks will have timeouts. If the timeout is passed before
the lock can be obtained, the lock fails.

The semaphore locks will be used to drain users from a website so
we can run `snapinstallwebsite` (i.e. upgrade) without end user
interferences.

Note that this is different from draining a computer from client
accesses.


_This file is part of the [snapcpp project](http://snapwebsites.org/)._
