
Introduction
============

The `snapdbproxy` daemon is used to communicate with the Cassandra
cluster. It allows us to very quickly get a connection to Cassandra
since this process runs locally and relays the Cassandra orders
between a Snap front end and a Cassandra cluster.


Multi-Threading
===============

The C++ `Cassandra` driver makes use of threads and we do not have a
choice on whether threads will be used or not. Many of our daemons
do not support threads because they make use of the `fork()` function
which is incompatible with threads.

The `snapdbproxy` resolves that issue by itself being _thread safe_
in the sense that it does not use `fork()` at all.


Connection Speed
================

Since `snapdbproxy` runs on the same computer as your other Snap!
services, the connection between those services and `snapdbproxy`
is really fast as it uses `lo` (we may even change that for a
Unix socket at some point.)

The `snapdbproxy` makes use of `snapcommunicator`, but it has
its own listening server to allow for database related
connections. The implementation of the protocol for that connection
is found in the `libQtCassandra` library.


Availability
============

Using `snapcommunicator`, the `snapdbproxy` sends two messages to
listeners:

* `NOCASSANDRA`

If `snapdbproxy` is not yet connected to Cassandra or somehow is not
able to connect, it will send the `NOCASSANDRA` message to other
services. When other services receive that message, they should
disconnect from `snapdbproxy` if they are still connected.

* `CASSANDRAREADY`

When `snapdbproxy` connects to Cassandra it broadcasts this message
to all the other services. If a service connects to `snapcommunicator`
after that message was broadcast, it will miss it. This is why
`snapdbproxy` also sends the `NOCASSANDRA` or `CASSANDRAREADY` to
a service that sends it a `CASSANDRASTATUS` message.


Bugs
====

Submit bug reports and patches on
[github](https://github.com/m2osw/snapwebsites/issues).


_This file is part of the [snapcpp project](https://snapwebsites.org/)._
