
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
acknowledgement. Once we get at least `N / 2 + 1` acknowledgement replies,
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


Algorithm
=========

Our snaplock system makes use of two algorithm to handle locks.

### Cluster Up

Before we can get valid elections (and thus valid locks) we must have a
valid cluster. This means the cluster an instance of snaplock runs on must
be properly connected to at least QUORUM nodes.

The snaplock knows that the cluster is up when it receives the CLUSTERUP
message. In the event too many connections are lost, the snaplock may
receive a CLUSTERDOWN event. This may have the effect of losing the
elected leaders depending on where these leaders were defined.


### Elections

#### Network Complete Problem

[Leader Election Algorithms](https://en.wikipedia.org/wiki/Leader_election)
require that the network be complete. In other words, if you have n nodes in
your cluster, you need all n nodes to be part of the network before the
leader election can even start.

This is problematic in our environment because we may be working on a
computer for a long period of time (remember that even minutes is a long
time on the Internet) and therefore prevent any locking mechanism from
happening if a network complete system is required.

It could also be that a computer crashes and is not restarted in a while.
The snaplock system needs to continue to run when a crash happens.


#### There can be only one!

The main problem in a leader election is that in the end there can be only
one leader. This constraint also applies to snaplock. Therefore, to avoid
potential problems the algorithm needs sufficient means to ensure the
unicity of the leader.

To achieve this feat, snaplock waits until at least a quorum of snaplock
services are ready (a.k.a. interconnected) to start the election.
The snapcommunicator knows the total number of nodes in the network,
whether they are currently running or not. A quorum is calculated as that
total number of nodes divided by two plus one:

        n
    q = - + 1
        2

Note that the cluster may be considered up before we have a quorum of
snaplock services. That is, snapcommunicators may be interconnected
before all snaplock services are available to communicate with each others.
This is why we need to have a quorum specific to the snaplock service and
not just a cluster up (remember that the snapcommunicator sends a CLUSTERUP
message whenever a quorum of snapcommunicators are interconnected.)

The election can start as soon as a quorum is reached. The election is
performed between the computers that are part of that first quorum. Any
other node that attempts to vote themselves in will be turned down.


#### Should we wait?

For more snaplock services to get onboard, we can wait a little once the
quorum is reach. Any additional snaplock that register themselves during
that wait period can be included in the election.

One special event can happen while waiting: the network becomes complete.
If that special event does happen, then the wait can be stopped at that
moment.


#### Quorum or no quorum?

_Sadly_ one event that can happen just after we reached the quorum state
is to lose it. If that happens and there is still an election, it has to
be stopped. If the election already elected snaplock leaders, losing the
quorum means that we cannot trust the lock status anymore so in that case
snaplock must mark all the existing locks as failed.


#### Why only one?

The snaplock system, in order to be failsafe, requires three nodes to
work in collusion. As a result, snaplock really elects three nodes as
leaders, not just one.


#### Who's quorum?

When the snaplock quorum is reached for one snaplock, it may not be
exactly the same as the quorum of another snaplock. This happens because
messages are sent asynchronously.

As a result, the list of nodes to participate in the election is not
unlikely to different between each node that thought it was part of
the quorum.

The algorithm presented below takes care of this problem by removing
any node which does not appear in all the candidates participating in
the election. This reduces the number of nodes that may become leaders,
but it does not prevent a valid election on a snaplock quorum network.
However, it is possible (especially in smaller networks) that the final
number of candidates drops below the minimum required (three in our
case or the maximum number of nodes if less than three, although remember
that in case there are less than three nodes total we need a complete
cluster first anyway.)


#### Who starts the election?

Any node that thinks it's time to start the election sends its message
to the nodes it currently knows about (those that it sees as allowed to
be candidates.)

Sending one message is enough to calculate the get the election results.
However, snaplock has to acknowledge the results because of the
dynamism of the network and the possibility that the results have changed
before the main leaders were all elected.


#### Node Priority

In order to increase the chance that the elected leaders are on a certain
set of nodes, each node can be assigned a priority from 1 to 15.

We elect the nodes with the lowest identifier, so if you set their priority
to 1, they are likely to be elected first.

Once elected, a node changes its priority to 0. This way it remains a
leader up until it's decommissioned for some reason (i.e. crashes, needs
to reboot, etc.) Having a priority of 0 may cause problems only if the
quorum of the network is often lost. Since we use a mesh to connect our
nodes, this possibility should remain rare.

Note: on a network with a total number of nodes of 3 or less, the priority,
ID and IP address as describe here are not useful. All the nodes will be
leaders anyway. Also, on a network with 4 nodes, _the 4th node_ will
automatically be elected when one of the other nodes go down.

Example:

Say you have 6 front end nodes, 3 backends, and 6 Cassandra nodes on your
network.

3 of the 6 front ends should be the ones that have the three snaplock
leaders. Those 6 nodes should be assigned a priority of 1.

Two of the backends could become the snaplock leaders, but these two are
not too thrilled about it. We assign a priority of 8 to them. The third
backend, though, is fine, it doesn't do that much most of the time. We
assign a priority of 2.

On the other hand, the 6 Cassandra nodes should never run as a snaplock
leader. All 6 are given a priority of 15. Although they may still be
selected when some _strange_ circumstances happen, it's very unlikely
to ever happen.


#### Node Identifier

Along the priority, snaplock randomly assignes each node a 28 bit number,
which complements the 4 bits of priority.

With that many bits, it's very unlikely that two computers would ever have
the same identifier, although if that happens we also make use of the IP
address of the computer as a mean to order them together.

The three computers that are valid candidates and with the lower priority
and the lower 28 bit number become the three snaplock leaders.


#### IP Address

The full number snaplock generates includes the IPv6 address at the end.
Since each computer must have a distinct IP address to function properly
on a network, this is a great unique number.

_Why not just use the IP address then and not bother with a priority and
random node identifier?_

Our idea is that this way we can make sure that different computers become
snaplock leaders over time, making it less likely that they start disturbing
the load of work on your cluster one way or the other. Especially, if the
algorithm was to always select the three computers with the smallest IP
address, it could cause problems because you generally do not have much
of a choice when it comes to IP address assignment. In other words, just
using the IP address would prevent you from choosing which computer would
generally end up as leaders.

That being said, by giving a priority of 1 to just three computers and 15
to all the others and assuming your computers are turned on in the
correct sequence, chances are those three computers with a priority of 1
are _always_ going to be the leaders (this will be true only until they
need to be rebooted, at which point they lose their leadership to other
computers. However, they'll be the first in line to regain their leadership
as soon as those other computers lose theirs.) In other words, you can still
get a very similar effect. Wisely using the priority gives you a much
better solution to push certain nodes in getting elected.


#### Who are the Leaders?

When ready (snaplock quorum + wait time), the snaplocks that know are
candidates send one `VOTE` message to all the other candidates it knows
of. The `VOTE` message includes the priority, node identifier, and IP
address as one large number (160 bits, called `my_id` in the algorithm.)

If one candidate sends v `VOTE` messages, then it expects to receive
v `VOTE` messages as well. From these and its own vote, each node can
sort the messages by `my_id` and the first three nodes are the leaders.
In our implementation we have one large number of 160 bits, the sort
uses `memcmp()`.


#### Late Comers

The algorithm has to take in account the fact that some nodes will
appear after the election took place. This happens when a computer
which is not a leader gets rebooted or just because some computers
were a little slow when the cluster first started.

For this reason, we need an algorithm which detect the current
status of snaplock. There are the status which are important to the
election:

* booting -- the cluster of snaplock has not yet reached a quorum
* pause -- once quorum is reached, we define an election date and then wait
* voting -- all known candidates send a `VOTE` message
* elected -- the voting period is over, leaders are known

While booting, the state is clear to all snaplock instances as the quorum is
not yet in place, although some instances may not yet have reached the quorum
when others have.

When we reach the quorum, we may reach a network which is in the 'pause',
'voting', or 'elected' state. We need to determine the state to know whether
we are a potential candidate or not. To do so we send an `ELECTION_DATE`
message with a proposed date. Our proposed date is `now()` plus a little
bit. The other candidates to which you send the `ELECTION_DATE` reply with
the same message and the smallest of their own election date or yours unless
the election is already over in which case they reply with a `LEADERS`
message. If the returned election date is in the past, then voting is
already in process or done. This node can't participate.


#### Multiple locations potential failures

The current implementation does not support a cluster which spans
between multiple locations (say one in the US and one in Europe). If the
cluster as a whole is cut off for long periods of time, locks may stop
functioning on one side or even both sides.


#### Complete Algorithm

The algorithm includes several stages as describe below.

Wait for the snaplock quorum:

    -- gather 'n'
    n := <total number of nodes in cluster>;

    -- compute quorum
    if n <= 3 then
      q := n;
    else
      q := n / 2 + 1;
    end if;

    -- number of known snaplock (start at 1 since there is ourselves)
    c := 1;
    while c < q
    do
      wait for new snaplock to connect;
      c := c + 1;
    done

    -- we've got the quorum, determine election date
    election_date := now() + '10 sec.';

    -- inform the other candidates of our election date
    foreach w in candidates
    do
      send_message(w, 'ELECTION_DATE', election_date);
    done;

Wait for the election date and then cast our `VOTE`:

    -- wait for the election date
    while election_date > now()
    do
      sleep for a bit;

      check for messages;
      case message of
        "ELECTION_DATE":
          -- another candidate replied or is informing us of his election date
          -- (in this loop `election_date > now()` is true so we won't get
          -- rejected with a `LEADERS` message)
          if message.election_date < election_date then
            election_date = message.election_date;
          else
            send_message(message.sender, "ELECTION_DATE", election_date);
          end if;

        "LEADERS":
          -- we arrive too late in the game, the elections already took place
          LeaderA = message.A;
          LeaderB = message.B;
          LeaderC = message.C;
          -- at this point the algorithm "ends" (it will be re-awaken by
          -- the loss of a leader, though)

      end case;
    done

    -- now cast our vote (this is the vote() procedure)
    foreach w in candidates
    do
      send_message(w, 'VOTE', my_id);
    done;

We expect "replies" from each candidate (more precisely, we expect each
candidate to also cast their vode within the same loop, however, if a
candidate receives a `VOTE` message but it doesn't know of the sender,
it has to send the sender a `DESIST` message, it will have to get out of
the race and send a `RETIRE` message to the other candidates to make sure
it is not taken in account.

The following is the handling of the `VOTE` messages:

    -- wait for messages
    do
      wait for message;
      case message of
        "VOTE":
          -- if we allow for varying election dates across the network
          -- then when we receive the first 'VOTE' message, we may not
          -- yet have cast our own vote, so we want to do that here
          if i_did_not_vote_yet then
            i_did_not_vote_yet := false;
            vote();
          end if;

          if search_sender_in_list_of_candidates(message.sender) then
            -- record valid senders information
            add_sender_info(candidates, message);
            check_for_vote_completion();
          else
            send_message(message.sender, 'DESIST');
          end if;

        "DESIST":
          -- we must let all know that we've desist the race
          foreach w in candidates
          do
            if w <> message.sender then
              send_message(w, 'RETIRE', my_id);
            end if;
          done;

        "RETIRE":
          -- the sender was asked to desist, so remove it from our list
          remove_sender_from_candidates(message.sender);
          if candidates.count < max(3, n) then
            -- if too many candidates get removed, ask for a rerun
            for w in all_known_snaplocks
              send_message(w, "RERUN");
            end for;
          else
            check_for_vote_completion();
          end if;

      end case;
    done;

The `check_for_vote_completion()` checks to see whether all the `VOTE`
messages were received. This is simple, each time we receive a `VOTE`
message we call `add_sender_info()`. Once we called that function `n`
times and the total number of candidates is `n`, then the vote is
complete.

With a complete vote, we can move forward with the leader determination:

    sort candiates by id;

    LeaderA = candidate[0];
    if n >= 2 then
      LeaderB = candidate[1];
      if n >= 3 then
        LeaderC = candidate[2];
      else
        LeaderC = null;
      end if;
    else
      LeaderB = null;
      LeaderC = null;
    end if;

_Note: `LeaderB` and `LeaderC` are only defined on clusters with 3 or
more nodes._

Since all the candidates are insured to have a unique `id`, the sort always
sorts in the exact same order for all candidates which in effect all have
the exact same list in the end.

Now we can mark the three leaders as such by changing their priority to 0
and finally send a message to confirm the leader identity to any other
snaplock, especially those that were not candidates.

    if my_id = LeaderA.id then
      my_id.priority = 0;
    elsif my_id = LeaderB.id then
      my_id.priority = 0;
    elsif my_id = LeaderC.id then
      my_id.priority = 0;
    end if;

    for w in all_known_snaplock
    do
      -- send the LEADERS message with the id of each leader
      -- a leader that receives this message must verify that
      -- it has the id of one of A, B, or C, if not the election
      -- must be canceled
      send_message(w, "LEADERS", "A=" + LeaderA.id, "B=" + LeaderB.id, "C=" + LeaderC.id);
    done;

    -- wait for messages
    do
      wait for message;
      case message of
        "LEADERS":
          -- note: assuming the algorithm is properly implemented this test
          -- is really not likely to ever be true, but there is slim
          -- possibility when one candidate is _not properly_ connected yet
          if i_am_a_leader and my_id not in (message.A, message.B, message.C) then
            -- we ARE a leader but someone else thinks we are not?
            for w in all_known_snaplocks
              send_message(w, "RERUN");
            end for;
          else
            -- record the information; saving all three gives us a way
            -- to send new lock requests any one of the three leaders;
            -- to which of the three LOCK requests should be send can be
            -- determined randomly or by round robin
            LeaderA = message.A;
            LeaderB = message.B;
            LeaderC = message.C;
          end if;

        "RERUN":
          -- elections have been canceled as we found out there was a cheater
          -- (we may want the message to include the new election_date?)
          election_date = now() + "10 sec.";

	  -- here we need to reset the leader information too
	  LeaderA = null;
	  LeaderB = null;
	  LeaderC = null;
	  my_id.priority = default_priority;

        "ELECTION_DATE":
          -- when we are here, the election is past and a wannabe candidate
          -- has to be told who the leaders are
          send_message(message.sender, "LEADERS", "A=" + LeaderA.id, "B=" + LeaderB.id, "C=" + LeaderC.id);

      end case;
    done;







### Lock

Our lock makes use of the Lamport's Bakery Algorithm. This was first designed
to handle locks on a computer with multiple processors running in parallel,
processors which do not offer any kind of locking mechanism.

The algorithm is shown in more details in `src/snaplock_ticket.cpp`. With
the leaders being the only computers that actually participate in the
Lamport's Bekery Algorithm.

The basics go like this:

Say we have three leaders, A, B, and C. A computer L receives a LOCK message
to obtain a lock for a process running on that computer. L forwards the
request to all of A, B, and C. The three processes then send a LOCKENTERED
message. This results in a query about the GETMAXTICKET. A reply to that
request sends a MAXTICKET message with the answer from each of A, B, and C.
With that information, A, B, and C can do an ADDTICKET. Once that message
is received, it replies with TICKETADDED. Finally, the process ends with
a LOCKEXITING. Once all of the are received, we have a locked process on
L where we send the LOCKED message.

The system only uses 3 to 5 nodes to handle the lock to make sure that it
doesn't take too long to process such requests. Locks with 3 computers
are really fast. More is generally not required unless you have a really
large cluster and want to make sure that the locks happen properly.
That being said, many systems still make use of lock systems that elect
a single computer which means that they do not have to make use of any
kind of advanced algorithm. They can just accept locks in the order they
receive them (a.k.a. FIFO.) This is much faster as long as you do not
have a fault on that computer in which case you cmopletely lose the
ability to obtain locks until you elect a new computer as the new
lock master. However, you have another problem in this case: the lock
that was current and those that were waiting are all lost without any
kind of feedback. Our mechanism gives us a chance to resolve the
loss of a leader by selecting another one and giving that other computer
the list of the existing locks. That means we do not lose any of the
existing locks and will continue as if nothing had happended (as long
as the lost computer is not one which had a lock, otherwise there
will be some side effects to the lock's table.)

All the locks are only kept in memory.


#### Potential Optimizations

A future optimization would be to look into having only the elected
snaplock daemons running. If one fails, another computer can then
automatically start snaplock. The _only_ problem in this case is: _How
do we handle the elections if snaplock isn't running on all concerned
computers?_



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


Bugs
====

Submit bug reports and patches on
[github](https://github.com/m2osw/snapwebsites/issues).


_This file is part of the [snapcpp project](https://snapwebsites.org/)._
