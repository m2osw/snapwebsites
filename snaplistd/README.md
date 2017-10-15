
Inter Computer List Management
==============================

The `snaplistd` daemon is used to manage lists in Snap!

The daemon is the part that is sent data from middle end computers (those
computers running `snap_child`.) It then saves that data to a MySQL
database which later the `pagelist` backend processes.


Processing of Requests that generate list journal entries
=========================================================

The following is a simplified diagram that shows everything from the client
to the pagelist backend processes showing how things are handled.

    +----------------+
    |                |
    |     Client     |    HTTP request
    |                |
    +--------++------+
             ||   send through public network (usually Internet)
             vv
    +----------------+
    |                |
    |    Apache2     |    Receive HTTP request
    |                |
    +--------+-------+
             |    fork()/exec()
             v
    +----------------+
    |                |
    |   Snap! CGI    |    Manage HTTP Request for Snap!
    |                |
    +--------++------+
             ||   send through private network
             vV
    +----------------+
    |                |
    |  Snap! Server  |    Accept request
    |                |
    +--------+-------+
             |    fork() (no exec(), it is just a sub-process)
             v
    +----------------+
    |                |
    |  Snap! Child   |    Process and generate result of GET/POST request
    |                |
    +--------+-------+
             |    code execution
             v 
    +------------------------+
    |                        |
    |  List Plugin           |   If a page is created or modified, save
    |  On Page Changed       |   info to local journal
    |  Save info to journal  |
    |                        |
    +--------++--------------+
             ||   UDP sending PING to "journallist" backend
             vv
    +------------------------+
    |                        |
    |  Journal List          |    Send journal requests to backend
    |  Process journal by    |<---------------------------------+
    |  sending info to listd |<--------------------------------+|
    |                        |                                 ||
    +--------++--------------+                                 ||
             ||   TCP sending LISTDATA to "snaplistd"          ||
             vv                                                ||
    +----------------------+                                   ||
    |                      |                                   ||
    |  snaplistd           +-----------------------------------+|
    |  save data to MySQL  +------------------------------------+
    |  reply to snaplistd  |    TCP sending GOTLISTDATA
    |  ping pagelist       |
    |                      |
    +--------++------------+
             ||  UDP sending PING to "pagelist" backend
             vv
    +----------------------+
    |                      |
    |  pagelist backend    |    Process entries as found in MySQL
    |                      |
    +----------------------+


Algorithms
==========

1. Snap Child -- List Plugin

The list plugin receives a call whenever a page gets created or modified.

The list plugin gets the current date and time and current date offset,
the current list priority, and the URI of the page that just changed
and appends that data at the end of the current journal file.

The filename for the current journal file is defined using the current
hour (a number from 00 and 23.)

The message is build as a string ending by a newline character:

    uri=<URI>;priority=<123>;key_start_date=<unix_timestamp_in_microseconds>\n

This is extensible if we later wanted to add other parameters in these
messages but these are enough to encompass all the data we have been using
to generate lists so far.


2. `journallist` backend

The `journallist` backend is expected to run on every single middle
computer (i.e. computers running List Plugin) and gather the journal
data that needs to be sent to the `snaplistd` deamons.

This process constantly runs and expects to receive a UDP PING event once
in a while to be awaken about having work to do. The `snap_child` process
writes data to the journal as described in point (1). The `journallist`
backend reads that data and sends it to `snaplistd` using the `LISTDATA`
message of `snapcommunicator`.

Because we want to make sure that everything happens as expected, the
`LISTDATA` message must be acknowledged by a `GOTLISTDATA` response. One
`LISTDATA` message sends exactly one message from the journal. Once that
message was acknowledged, the data is removed from the journal.

Older journal (other than the current hour or previous hour) get deleted
once all of their messages were successfully sent to `snaplistd`.


3. `snaplistd` daemon

This daemon listens for `LISTDATA` messages and saves them in a
MySQL table. Once the save to MySQL worked, we send the `GOTLISTDATA`
to the sender who can then remove the data from the journal it is
managing.

The MySQL database is used because we want the entries to be sorted
properly. This means sorted by:

* domain name
* priority
* date and time

Note that for the date and time we keep it as an `int64_t` (a `BIGINT`
as far as MySQL is concerned) so as to avoid the conversion to and
from a string.

The sorting by domain name allows us to manage multiple domains and
not reall entries from another domain for nothing.




vim: ts=4 sw=4 et

_This file is part of the [snapcpp project](http://snapwebsites.org/)._
