
# Replication

## Duplication for Safety

The replication features means that we want copies of the data on multiple
computers in order to avoid one single point of failure.

Data is duplicated on a per row basis. The selection of the computers on
which the data gets copied is determined by the row identifier and a CRC
calculation. Cassandra uses Murmur now. It was using an MD5 sum before.
Either way should work for us.

### Settings

Cassandra offers a replication factor which can differ per context (a context
is the equivalent to a database). So all the rows of all the tables within
one context will be replicated on the same N computers where N is the
replication factor.

### Computer Selection

The selection is done automatically. We assign "key ranges" (the CRC is what
we really assign).

So with 12 computers and a replication factor of 3, we want something like
3/12th of the data per computer.

The concept is that the distribution of keys when using a CRC is much better
(i.e. you are much more likely to have a similarly total number of keys
assigned to each computer.) In our Snap! implementation, though, we have
many tables used with one key but many columns. This is used as a poor's man
index. The disavantage is that this index is going to sit on just one set of
computers. If large, it can have an impact. However, with a CRC there is no
way to handle an index on more than one computer. The result is a limit to
the size of an index to the maximum of one row.

#### Changes to the Cluster

We want to be able to add and remove computers at will (not constantly, but
at any time we should be able to grow and shrink the size of the database
cluster).

This means the data needs to be attached to a specific computer
by key, but when we add/remove computers those computers would change...
Where we do get key A if it was assigned to computers M, N, O and now
it is expected to be on computers X, Y, Z? From what I've gathered, the
client asks X, Y, or Z which in turn, if they do not find the data, check
with M, N, or O. If the data is not there either, then it did not exist.

When growing, with time, the keys which are on the _wrong computers_ get
moved around as expected so at some point the extra step described above
becomes unnecessary

However, shrinking the number of computers in your database is much more
complicated. You can't remove too many nodes at once and not lose some
data. So you have to do that in a very controlled manner. Just like when
adding new computers, we can then redistribute the keys so they match
the computers as expected.

### Basic Implementation

On Cassandra, they send the data on their ring.

In our system, we want to follow the ScyllaDB concept and use UDP to send
the data on all N computers at once. We can use an TCP connection as a
control system, but UDP can be used that way too. (i.e. send a UDP packet,
wait for an acknowledgement, if the ack does not occur, resend).

The UDP allows us to send the data to N computer at once so those computers
do not have to spend time to replicate that data. The replication will have
happened in the network device (switch) between the computers. It is of
course assumed that you'll have a good piece of equipment which allows for
such fast replication in your switch.

The TCP connection can be used to send a small message with info about
what file is being sent and the number of packets that will be sent. Then
the client's sends the data through UDP. Once done sending all the data,
each server acknowledge about what they received and did not receive.
If the N computers together received at least one full copy of all the
data then we're done as the client.

Data that did not make it on a certain computer will be sent to it from
another computer which has a copy. Since we work with a transaction
system, other clients will see the new data only once all N computers
are ready to present the new row/cells. Until then, the old data gets
sent to the clients.

The very client which sent the row, however, will be given a copy of
that data if they do a `GET` immediately after, even if the data did
not yet stabilized on the servers.


