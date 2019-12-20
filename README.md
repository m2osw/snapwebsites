
# Introduction

Snap! C++ is an Open Source CMS mainly written in C++ and JavaScript.

This repository includes a large library and large set of core projects
so one can run a website. The following describes a few things about
each project. Each project also has its own `README.md`.


# Contrib

Most of the Snap! Website projects make use of one or more of the
contrib libraries and tools.

For example, the `snapfirewall` uses the `iplock` tool in order to add
and remove IP addresses on the Linux firewall (`iptables`). Most of
our tools make use of the `advgetopt` library.

Many of the classes in our `libsnapwebsites` will one day become
projects on their own.


# Projects

## cmake

Various files to properly support cmake and be able to use our
projects with and from third party extensions.


## Library

The `libsnapwebsites` library includes a large number of helper functions
and classes that are used by our Core project and are also directly
available to third party extensions.

Especially, the library includes the `snap_communicator` class which
handles the network in all the other tools that require access to
the network. This class can parse and process messages as they arrive
at a client or a server. It is very much like an RPC system.


## snapbase

A few scripts and configuration files that are required by many of the
other packages.


## snapserver

The `snapserver` is the main binary that makes Snap! function. It handles
requests that are passed to it from Apache (through `snapcgi`).

It knows about the `snapserver-core-plugins` and handles many of the basics
of the `snapbackend` tool.


## snapserver-core-plugins

This is a long list of plugins that make the system work in a very
modular way. For example, the password plugin is used to handle user
passwords.

At this point, the core plugins are those that are part of Snap! by
default and not the bare minimum list of plugins.

The minimum listis defined in the
`libsnapmanager/src/snapwebsites/snap_child.cpp` file.
That list, at time of writing, looks as follow:

    // list of plugins that we cannot do without
    char const * g_minimum_plugins[] =
    {
        "attachment",
        "content",
        "editor",
        "filter",
        "form", // this one will be removed completely (phased out)
        "info",
        "javascript",
        "layout",
        "links",
        "list",
        "listener",
        "locale",
        "menu",
        "messages",
        "mimetype", // this should not be required
        "output",
        "password",
        "path",
        "permissions",
        "sendmail",
        "server_access",
        "sessions",
        "taxonomy",
        "users",
        "users_ui"
    };

At some point, we want to break up the list of plugins in multiple
packages, some that would be themed. For example, we could have an
accounting package with the bookkeeper and timetracker plugins.

We already have thrid party plugins so we know extending the core
plugins work. At this time it is just a lot easier to keep all of
our plugins in one package.


## snapcgi

This project is a very small binary used to link Apache to `snapserver`.

On a Snap! system, `snapserver` is started once and runs forever. Whenever
Apache2 receives a request, it runs the `snap.cgi` binary and that binary
connects to `snapserver` and sends the request there. `snapserver` starts
a new `snap_child` and gives the child the request data.

At that point, the `snapserver` loads the plugins used by that website and
executes the code that the URL warrants to execute.

The CGI code includes some anti-hacker code so that way we can prevent
wasting time (it checks those things even before it connects to
`snapserver`.)

Some of the reasons for this break up are:

* The Apache2/snap.cgi pair can reside on a front end computer which
  does not run any `snapserver` daemon.
* The `snapserver` can be ready (fully initialized) when it receives
  requests. The server has to connect to Cassandra through the `snapdbproxy`
  and to some other servers and all those network connections would be too
  slow if we had to restart the server each time we receive a new connection,
* `snap.cgi` is capable of detecting that a `snapserver` is not available
  and select another one when required (this is not yet fully implemented,
  but the idea is for `snap.cgi` to be capable of selecting another
  `snapserver` if the default one is down or too busy--in effect, this
  gives you a free software based load balancer.)


## snapcommunicator

The Snap! Communicator is the center piece that allows all our servers
(and some clients) to all communicate together. It is capable of sending
and receive messages from any number of machines. It will even be able
to discover additional machines as they broadcast themselves (with the
infamous `GOSSIP` message.)

The server is able to automatically send messages to the right machine.
For example, certain backends (see `snapbackend`) only run on a few
backend computers (computers that do not face the Internet.) Whenever
a modification to one of your websites happens, it quite often sends
a `PING` to `snapcommunicator` which in turn forwards  that `PING` to
the corresponding backend. That wakes up the backend immediately and
gives it a chance of working on the new data very quickly.


## snapdbproxy

The `snapdbproxy` is a utility that sends the CQL orders to the
Cassandra cluster. We are also planning to have other database
drivers so we can use MySQL with Snap! instead of only Cassandra
(which is more practical for smaller websites.)

The utility is run as a server which is particularly useful because
we want to connect once and stay connected to the Cassandra cluster.
It saves a large amount of time to use this that way. A local
connection to `snapdbproxy` is really fast. In contrast, two to five
remote connections to a Cassandra cluster is very slow.

Also, this way the `snapdbproxy` uses the full power of the Cassandra
driver which allows for switching between connections when one
goes down, making that capability transparently available to the
developer.

Finally, the Cassandra driver is multi-threaded and that causes many
problems in some of our daemons which are not multi-thread safe (on
purposes because it makes things way more complicated for usually very
little benefits.)


## snapfirewall

The Snap! system comes with a way to add and remove IP addresses
to the Linux firewall in a completely automated way. This uses
the `iplock` tool and the Cassandra database.

There are several problems with handling the firewall:

1. you want to make sure you do it right, so we thought that having
a set of modules that do it all for you would resolve that issue.
2. you want to run servers that are not `root`, so having a secure
tool such as `iplock` allows you to run as a non-root user and still
be able to add and remove rules from your firewall.
3. on a (re)boot you want your firewall setup before your servers
are started, otherwise there would be a (albeit small) window of
time when a hacker could access one or more of your services.
4. when a hacker is detected and blocked on computer A, it is going
to still be able to hack computers B, C, D, ... Z. Only blocking the
hacker on computer A is not as proactive than blocking that hacker on
every single computer all at once (okay, there is a tiny delay, but
still, you block on computer A, it gets blocked on computer B to Z
within less than a second!)

The firewall is also responsible for managing some other protection
tools such as fail2ban and it blocks many things automatically once
installed (email spammers, IoT hackers, Apache hackers, etc.)


## [snaprfs](https://snapwebsites.org/project/snap-replication-file-system)

Various parts of Snap! need to have files shared between many of
the computers available in your cluster. To satisfy that need,
we created the Snap Replication File System (RFS). This means it runs
on all computers and replicate files between computers without
you having to do more than ask for a certain file to get duplicated
automatically. This works in all directions and can be made conditional
(i.e. some computers may not need a copy of that file.)

This is used to replace code that was specialized in various parts
of the system. For example, the snapmanagerdaemon would copy some of
the files it generates on other computers to allow the visualization
of the status of any computer.

We also use it to share all the tables that snapdbproxy uses to setup
the Snap websites environment.

Ultimately, it would be great to make it available via mount so we
could see a directory and files that may reside on any computer with
standard Unix commands such as ls, cat, grep, etc.


## [snapwatchdog](https://snapwebsites.org/implementation/feature-requirements/watchdog-snap-cluster-core)

The `snapwatchdog` service runs in the background of all your servers
and gathers statistic data every minute.

It will also verify that your computer is healthy by making sure
that certain tools are running or not running.

For example, a computer could be down because `snapserver` did not
start. If you setup the computer properly then `snapwatchdog`
knows what to expect on that system. If something is missing or
something is there when it shouldn't be, then the watchdog sends
the administrators a notification via email.

The tool is a service. Once a minute it wakes up and creates a
child. The child loads the `snapwatchdog` plugins which allow
for various checks to be run. One of the plugins actually
runs shell scripts that other projects install. Plugins can
also be offered by other projects (except the snapmanager project
which is a snapwatchdog dependency.)

For example, the `snapfirewall` has a script to check whether the
`fail2ban-client` python script is running. Once in a while it
wakes up and does part of its work and then gets stuck using
100% of the CPU. If that process is detected running for 3 minutes
or more, then the script generates an error which is then converted
to an email.

The Snap! Watchdog also detects high memory usage, high CPU usage,
and high disk usage. Again, problems discovered get emailed to the
cluster administrator.


## snapdb

The `snapdb` utility is used to check out the database from the
command line. It is also capable of modifying it (drop a value,
drop a row, add a new value, update an existing value.)

This is particularly useful in our scripts. As a developer, I also
often use it. The problem with `cqlsh` is that it can't handle blobs
properly. We save all our data as blobs because that way we did avoid
all sorts of data conversions (when we used thrift). Now that we are
using CQL, this is not so true anymore, unfortunately.


## snaplayout

The Snap! Layout utility installs themes in your Snap! database. At
this time it is not very advanced. For example there is no function
to remove a theme.

The tool allows you to make theme selections for a page and its children
or an entire website.


## snaplock

The Snap! environment runs in a cluster and we often need to get a lock.
We implemented our own because all the other implementations (At least
at the time) would not be capable of using multiple computers for a
lock. All those we checked would have one central computer that would
do the lock/unlock work. If that computer failed, you would lose the
existing locks and for a moment you could not use locks, possibly
creating quite a problem (blocking all accesses until the lock computer
gets repaired.)

Our implementation, instead, is capable of doing a distributed lock.
This means that if any one computer goes down, `snaplock` continues to
work seemlessly. At some point we will write the necessary code to
limit the number of computers used in a lock to just 3 to 5 instead
of the entire cluster which makes it somewhat slow (we don't lock
often, though.)


## snaplog

Since Snap! runs on many computers, it can be daunting to find the
correct log. We instead offer a way to send all the logs to one
backend which saves the data in a MySQL database. That way you
can search the database for your log and also check what happened
before and after, possibly on another computer... which could have
been the trigger of the bug you're looking into.

This is somewhat similar to what people call Remote Log Services,
except that you have it for free on your cluster (and we don't
yet offer any kind of statistics or reports of your logs).


## snapbackend

The library offers a server and children which the `snapbackend` reuses to
handle backend tasks.

The same binary is used to run many backends because backends are in most
cases developed as plugins. This is done that way so the backends can
run in an environment very similar to what you otherwise encounter
when running your plugin as your snapserver received a hit from a client.

The core backend at time of writing are:

1. **list::listjournal** -- a backend used to track changes to pages so we
can effectively update lists that may contain said page; this backend runs
on all the computer that run `snapserver` and a `snapbackend` that may
generate a list (i.e. all `snapbackend` is probably the safest, it will
be sleeping all the time if not really required.)
2. **list::pagelist** -- a backend used to generate lists, it uses the data
gathered by the list::listjournal backend to know which page to check
against
3. **images::images** -- a backend used to tweak images for your website,
for example you may want to support a large image (4096x1024) as a
header in your posts, but for smart phones, a 1024x256 would be more
than enough (possibly even smaller) so you would write a script that
generates many different sizes of images and then offer all those
different sizes each time the image gets referenced
4. **sendmail::sendmail** -- a backend used to actually send an email,
whenever a front end generates an email, it doesn't send it for two
main reasons: that computer may not have an MTA and it could take
time to generate the email time we don't want the client to wait
for; instead the email gets saved in the database and this process
wakes up and sends the email at its leisure; note that gives the
front end the possibility to create an email that will be sent some
time later (i.e. you could schedule the email for the next day.)


## snaplistd

Whenever a page gets modified, it needs to be checked against all your
lists. This is how we make your lists efficient in the end (i.e. the
building of the lists is _slow_, however, accessing the lists is very
fast as we build indexes for them.)

This backend is used to accept `snapcommunicator` messages of page
changes as send by the `list::listjournal` backend and save those
changes to MySQL. Later the `list::pagelist` reads the data from
MySQL and works on it generating new or removing old index entries.

This service has to run on all the computers that run `list::pagelist`
and have access to MySQL.


## snapmanager

This tool makes use of an HTML interface and it can be used to setup
your Snap! cluster. It would be really complicated without it because
of the large number of services you need to install, start, stop, etc.

At this time there is a conceptual problem which prevents one instance
on one computer from making changes on another computer. Some of the
changes will work, others won't. This is because certain things will
be installed on your current computer and others won't be. Except for
that _small_ glitch, this is what we use to manage our clusters:
run updates, install/remove a service, etc.

You will have to visit each one of your server until that _small_
glitch gets fixed. It is still extremely useful because the installation
and management is all done from a nice HTML interface and this makes
it very easy to manage your servers. The bundles that come along each
part of the software will always correctly install and uninstall whatever
they are managing, leaving you with just a few lines of data to enter
in the interface.

<p align="center">
<img alt="Snap! Watchdog Screenshot"
     src="https://snapwebsites.org/sites/snapwebsites.org/files/images/screenshot-watchdog-tab.png"
     width="1386"
     height="683"/>
</p>


## snapbackup

For smaller productions, you probably want to backup your Cassandra
cluster. Once you have a large number of Cassandra node, this won't
be possible, however, you will be able to be more confident that
Cassandra is properly replicating and that the loss of data is minimal.

This process is strongly recommended on any installations that use
less than 12 Cassandra nodes. Just make sure to have a computer with
enough disk space for the backup. At the same time the disk space
requirements of Cassandra are much higher than the requirements of
the `snapbackup` as the data ends up compressed and linear in the
backup. The Cassandra files include a large number of meta data
in comparison and they don't normally get compressed as well as
the backups.


## snapbounce

You install `snapbounce` on the computer that handles the Postfix mail
server. It captures all the bounces and reports them to Snap! That way
the `sendmail::sendmail` backend and plugin are capable of marking users
as not reachable (at least through email.)

The installation of Postfix and `snapbounce` is automatic when you use
the `snapmanager` browser interface (because, frankly, who knows how
to capture bounced email from Postfix?)


## cassview

This is a developer utility that gives you access to your Cassandra
cluster in a GUI. It lets you access the fields as Snap! manages
them, making it easy to edit some values for test purposes.


## [snapdatabase](https://snapwebsites.org/project/snapdatabase)

This project is ultimately expected to replace Cassandra. At the moment
we only use it for journals such as when a change that requires work from
the backends is made. Cassandra is totally awful with such journaling so
we instead created our tables.

There is quite a bit of documentation. The main idea of this project is
to create a database system that is at the same time allowing horizontal
growth (a la Cassandra, by distributing the data between computers) and
allowing for the database to handle all the nitty gritty which Cassandra
doesn't handle such as complex secondary indexes and full on transactions
(i.e equivalent to a `BEGIN; INSERT/UPDATE/DEETE; COMMIT/ROLLBACK;` block).

In regard to indexes, we have documentation for four different types:

* OID indexes, each row is given an ID and sorted in the database in that
  way; most tables (all for now) make use of it; it allows for an indirection
  so other indexes can reference the OID and not the data directly allowing
  for the data to move about when being updated;

* Primary Index, all rows also have a primary index; this is the same as we
  have in Cassandra, except that our system is capable to automatically
  concatenate multiple columns to for the primary index (the latest Cassandra
  does so too);

* Secondary Indexes, a set of rows (filtered with an equivalent to a `WHERE`
  clause) are sorted using a different set of columns (`ORDER BY ...`);

* Tree Index, a table which has a Primary Key representing a path (`/a/b/c/...`)
  can be sorted using a tree index; the path can be anything which forms a
  tree and the separator does not have to be the slash character;

Having those indexes directly defined in our database will improve the speed
by a large magnitude. The latest method I use is to build a table with a
specific primary key which I have to be build using multiple columns. The
equivaluent of a pre-calculated `WHERE` clause. This works and is really fast,
however, it has one drawback: it's not all manage with a single transaction
mechanism. In other words, I may save data in the content table, then try
to create the index for that new content and that part fails. This can mean
the database gets out of sync. and we don't really know it. Another point
which this database implementation will look into preventing.


# Tracking Delay of loading a binary under Linux

A very interesting trick is to learn how to get statistics when loading
a binary under Linux.

    LD_DEBBUG=statistics <path to binary>

The results of the statistics are written in your console so you have
to capture that output.

First an example of a binary started **cold** (when it is expected that
many of the libraries were not yet loaded):

    $ LD_DEBUG=statistics ../../BUILD/snapwebsites/snapcgi/src/snap.cgi
        24351:
        24351:  runtime linker statistics:
        24351:    total startup time in dynamic loader: 1970618235 cycles
        24351:              time needed for relocation: 1961465023 cycles (99.5%)
        24351:                   number of relocations: 12024
        24351:        number of relocations from cache: 16995
        24351:          number of relative relocations: 63446
        24351:             time needed to load objects: 8439663 cycles (.4%)
        24351:
        24351:  runtime linker statistics:
        24351:             final number of relocations: 14731
        24351:  final number of relocations from cache: 16995

Second is the same binary, but this time as a **warm** start. We can see
that it starts about 43 times faster (45.6M cycles instead of 1.97B.)

    $ LD_DEBUG=statistics ../../BUILD/snapwebsites/snapcgi/src/snap.cgi
        24428:
        24428:  runtime linker statistics:
        24428:    total startup time in dynamic loader: 45679278 cycles
        24428:              time needed for relocation: 35356048 cycles (77.4%)
        24428:                   number of relocations: 12024
        24428:        number of relocations from cache: 16995
        24428:          number of relative relocations: 63446
        24428:             time needed to load objects: 9565275 cycles (20.9%)
        24428:
        24428:  runtime linker statistics:
        24428:             final number of relocations: 14731
        24428:  final number of relocations from cache: 16995

As we increase the number of libraries, these numbers may increase. What
we want to look into in some cases is to either switch to a static link
(only for low level contribs, though) or to place the item in a separate
plugin which does not always get loaded.


# Bugs

Submit bug reports and patches on
[github](https://github.com/m2osw/snapcpp/issues).


_This file is part of the [snapcpp project](https://snapwebsites.org/)._
