
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
projects from third party extensions.


## Library

The `libsnapwebsites` library includes a large number of helper functions
and classes that are used by our Core project and are also directly
available to third party extensions.

Especially, the library includes the `snap_communicator` class which
handles the network in all the other tools that require access to
the network. This class can parse and process messages as they arrive
at a client or a server.


## snapbase

A few scripts that are required by many of the other packages.


## snapserver

The `snapserver` is the main binary that makes Snap! function. It handles
requires that are passed to it from Apache (through `snapcgi`).

It knows about the `snapserver-core-plugins` and handles the `snapbackend`.


## snapserver-core-plugins

This is a long list of plugins that make the system work in a very
module way. For example, the password plugin is used to handle user
passwords.

At this point, the core plugins are those that are part of Snap! by
default and not the bare minimum list of plugins.

The minimum listis defined in the
`libsnapmanager/src/snapwebsites/snap_child.cpp` file.
That list, at the time of writing, looks as follow:

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
(and comse clients) to all communicate together. It is capable of sending
and receive messages from any number of machines. It will even be able
to discover additional machines as they broadcast themselves.

The server is able to automatically send messages to the right machine.
For example, certain backends (see `snapbackend`) only run on a few
backend computers (computers that do not face the Internet.) WHenever
a modification to one of your websites happens, it quite often sends
a `PING` to `snapcommunicator` which in turn forwards  that `PING` to
the corresponding backend. That wakes up the backend immediately and
gives it a chance of working on the new data.


## snapdbproxy

The `snapdbproxy` is a utility that sends the CQL orders to the
Cassandra cluster. We are also planning to have other database
drivers so we can use MySQL with Snap! instead of only Cassandra
(which is more practical for smaller websites.)

The utility is run as a server which is particularly useful because
we want to connect once and stay connected to the Cassandra cluster.
It saves a large amount of time to use this that way. A local
connection is really fast. 2 to 5 remote connections, as is done
with Cassandra, is very slow.

Also, this way the `snapdbproxy` uses the full power of the Cassandra
driver which allows for switching between connections when one
goes down, making that capability transparently available to the
developer.


## snapfirewall

The Snap! system comes with a way to add and remove IP addresses
to the Linux firewall in a completely automated way. This uses
the `iplock` tool and the Cassandra database.

There are several problems with handling the firewall:

1. you want to make sure you do it right, so we thought that having
a set of modules that do it all for you would resolve that issue.
2. you want to run servers that are not `root`, so having a secure
tool such as `iplock` allows you to run as someone else and still
be able to add and remove rules from your website.
3. on a (re)boot you want your firewall setup before your servers
are started, otherwise there would be a (albeit small) window of
time when a hacker could access one or more of your servers.
4. when a hacker is detected on computer A, it is going to still be
a hacker on computer B, C, D, ... Z. Only blocking the hacker on
computer A is not as proactive than blocking that hacker on every
single computer all at once (okay, there is a tiny delay, but
still, you block on computer A, it gets blocked on computer B to Z
within less than a second!)

The firewall is also responsible for managing some other protection
tools such as fail2ban and it blocks many things automatically once
installed (email spammers, IoT hackers, Apache hackers, etc.)


## snapwatchdog

The `snapwatchdog` service runs in the background of all your computer
and gathers statistic data one a minute.

It will also verify that your computer is healthy by making sure
that certain tools are running or not running.

For example, a computer could be down because `snapserver` did not
start. If you label the computer properly then `snapwatchdog`
knows what to expect on that system. If something is missing or
something is there when it shouldn't be, the the watchdog sends
the administrators a notification via email.

The tool is a service. Once a minute it wakes up and creates a
child. The child loads the `snapwatchdog` plugins which allow
for various checks to be run. One of the plugins actually
runs shell scripts that other projects install.

For example, the `snapfirewall` has a script to check whether the
`fail2ban-client` python script is running. Once in a while it
wakes up and does part of its work and then gets stuck using
100% of the CPU. If that process is detected running  for 3 minutes
or more, then we send an email.


## snapdb

The `snapdb` utility is used  to check out the database from the
command line. It is also capable of modifying it (drop a value,
drop a row, add a new value, update an existing value.)

This is particularly useful in our scripts. As a developer, I also
often use it. The problem with `cqlsh` is that it can't handle blobs
properly. We save all our data as blobs because that way we did avoid
all sorts of conversions (when we used thrift). Now that we are using
CQL, this is not so true anymore, unfortunately.


## snaplayout

The Snap! Layout utility installs themes in your Snap! database. At
this time it is not very advanced. For example there is no function
to remove a theme.

The tool allows you to make theme selections for a page and its children
or the entire website.


## snaplock

The Snap! environment runs in a cluster and we often need to get a lock.
We implemented our own because all the other implementations (At least
at the time) would not be capable of using multiple computers for a
lock. All those we checked would have one central computer that would
do the lock/unlock work. If that computer failed, you would lose the
existing locks and for a moment you could not use locks, possibly
creating quite a problem (blocking all accesses until the lock gets
repaired.)

Our implementation, instead, is capable of doing a distributed lock.
This means thatif any one computer goes down, `snaplock` continues to
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
when running your plugin because your server received a hit from a client.


## snaplistd

Whenever a page gets modified, it needs to be checked against all your
lists. This is how we make your lists efficient in the end. This
backend is used to read such changes and send them to the list backend.

This service has to run on all the computers that run `snapserver` and
on most of not all backends.


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


## snapbackup

For smaller productions, you probably want to backup your Cassandra
cluster. Once you have a large number of Cassandra node, this won't
be possible, howeever, you will be able to be more confident that
Cassandra is properly replicating and that the loss of data is minimal.

This process is strongly recommended on any installations that use
less 12 Cassandra nodes. Just make sure to have a computer with enough
disk space for the backup. At the same time the disk space requirements
of Cassandra are much higher than the requirements of the `snapbackup`
as the data ends up compressed and linear in the backup. The Cassandra
files include a large number of meta data, in comparison and they don't
normally get compressed as well as the backups.


## snapbounce

You install `snapbounce` On the computer that handles the Postfix mail
server. It captures all the bounces and reports them to Snap! That way
the `sendmail` plugin is capable of marking users as not reachable
(at least through email).

The installation is automatic when you use the `snapmanager` browser
interface (because, frankly, who knows how to capture bounced email
from Postfix?)


## cassview

This is a developer utility that gives you access to your Cassandra
cluster in a GUI format. It lets you access the fields as Snap! manages
them, making it easy to tweak some values for test purposes.


## snapdatabase

This is just documentation at the moment. The idea would be to create
our own database system specifically tailored to our needs instead of
using a more broad system like Cassandra. Especially now that Cassandra
imposses the use of CQL. Maybe one day.


# Bugs

Submit bug reports and patches on
[github](https://github.com/m2osw/snapcpp/issues).


_This file is part of the [snapcpp project](https://snapwebsites.org/)._
