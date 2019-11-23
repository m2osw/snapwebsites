
# Client Service

## Brief Description

The _Client Service_ is a daemon running on your computer. It knows how to
communicate with the backend and does all of that work for you.

You only communicate with this _Client Service_ and never actually send
the data directly to the server. It includes support for all the available
features.

## Features

### Connecting to All Servers

The _Client Service_ does several things when it starts: it connects to
the backend servers (all of them unless that number is prohibitive). This
gives the client a way to send data to any one server directly instead
of sending it to one server and rely on that backend server to forward
the data to the correct N computers.

In case of a really large network (thousands of computers), we may have
to device a system where you only connect to N/F computers where F is
the replication factor. This means we would not benefit from the UDP
replication mechanism unless the servers help us with the necessary
inter-node communication to make sure that our UDP packets make it as
expected.

### Replication Using UDP

The first step in the replication process is to send the data once from
the _Client Service_ to the servers expecting it (i.e. the clients
calculates the CRC and automatically knows to where the data needs to
be sent).

### Compression

We have a setting defining a threshold over which data is compressed before
it gets sent. This allows us to (1) reduce the bandwidth which on a busy
system can be really useful and (2) reduce the amount of space necessary
on the server.

Note that compression is not going to be truly available on the key and
indexed columns.

### CRC

To select the set of computers to which we want to send the data we need
to have a CRC calculated for the key of the data. This allows us to
directly send the data to the right server.

### Not Connected

If we can't get a connection to that right computer, we still want to
send the data to a backend server which will then be in charge of sending
that data once one of the computers that is expected to get the data is
back online.

### Local Journal

The _Client Service_ first saves the data it receives from your computer
to a local journal. This is a file where we save the data we receive from
your local services.

This journal has several uses: it allows us to keep track of the data
for a given key so on a `GET` we can return that data (we are likely to
still have it in our memory cache, but if not, it may be in that journal).
This means no trip to the backends are necessary.

This is where we need the `LISTEN` feature because when a backend received
a new row, it has to let the clients that had a copy of the old data that
it was replaced.

So in other words, we have a _heavy_ local cache of the rows locally until
too old, outdated, too much data is cached and we need to rescue space for
new data to be saved locally.

Note also that very large buffers may have special consideration both
ways: i.e. even if not accessed often, we may still want to keep it around
because, well, it's so big. For example, having a full copy of the database
schema is probably an excellent idea. We can `LISTEN` for changes to the
schema and ask for a copy in case we are told it changed.

So, we have two local journals: one fast one in memory and one slow one
in a file. Just that will make the database system lightning fast. The _only_
drawback I can think of is that all computers, even clients, need to have
a properly synchronized clock otherwise you'd get invalid data.

Note: for security reason certain tables should never be cached on the
filesystem of a front end computer. Especially anything to do with the
user (user data, e-commerce related account information, etc.) Make it
a per table setting where when set it eliminates the possibility to
locally cache the data (at least in a file and possibly even in memory).

HOWEVER: with such a system we could even look at making all the computers
database computers! (no separation needed, after all, we can use resources
on the front ends to run the database too because in many cases the front
ends won't be that busy all the time).

## Client's Library

We want to have a client's library which allows any C++ software to
very easily connect and work with the _Client Service_. In other words,
all the work of connecting to that local service, setting up the cache,
getting information about tables, etc. all will be done under the hood.

### C++ Functions

At this level, all the commands we can send are C++ functions. We also
support callbacks (what is called Triggers in SQL) through the Event
Dispatcher library and by using the `LISTEN` command.

The following commands are specific to a table. We handle one row at
a time. We do not give access to a per cell basis like in Cassandra.

Note: to handle really large data, use a "file" table (or maybe call
it "blob") and save the large cells there and a reference in your
rows.

**Note:** This is similar to an SQL `CREATE TRIGGER`, only our events
only occur when data changes and on a per row basis. We'll still offer
a `BEFORE` and `AFTER` capability. The `BEFORE` will have the ability
to modify the data before it gets inserted or updated and the `AFTER`
can change the data before it gets sent to the client.

#### The `blob` Value

The value in a row is a `blob`. This is actually one block of data
which the library manipulates for you. It allows you to have _any_
kind of data in a row. You can think about it as a map of key/values.
Each key in this map represents a column.

See the BLOB.md doc for details.

#### `get_table(std::string name)`

This function allows you to get a table object. From a table object
you can access the data in the database.

A table also holds settings which can be tweaked.

#### `get(std::string key)`

Retrieve the data from row referenced by `key`.

If the key does not exist, the call fails.

#### `set(std::string key, blob value)`

Save `value` in the database. They `key` can later be used to retrieve
the data back out with a `get()`.

Note that the value gets saved whether the key already exists or not.

#### `insert(std::string key, std::string value)`

Save `value` in the database if `key` does not exist yet. This is going
to be using the TOD assigned to this data to determine whether the data
already existed.

So if two computers use `insert()` nearly at the same time, the one with
the smaller TOD wins.

The test of the availability of `key` is considered atomic.

#### `update(std::string key, std::string value)`

Save `value` in the datbase if `key` already exists. if the key does not
exist, then the `update()` fails.

The test of the availability of `key` is considered atomic.

### Application Cache

The Client's library is in charge of part of the local cache so that way
it's in your application directly. Just like the QtCassandra has been
doing (only we had problems for total lack of signals from the
cluster in case something changes under our feet; and also cursors did
not work correctly at all in that regard; we need to resolve both problems
for our own systems).

Anyway, the idea in the end is for any data that we read and/or write to
be cached in the application so future reads can re-use the data we have
cached. With a `LISTEN` we can be sure that if the data changes we'll
know about it and discard or update our caches accordingly.

Note: application cache should be limited to memory caches. This way we
can more easily make things secure. It's harder for a hacker to access
data of a process than just reading a file on disk.



