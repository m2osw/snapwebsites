
Introduction
============

The `snapwatchdog` daemon runs in the background and check the health
of your system once every second. The results are saved in your Cassandra
cluster for later review.

The data includes basic computer status such as CPU and memory usage as
well as other data such as how much CPU and memory a `snap_child` process
used on its run.


Statistics
==========

The watchdog processes will gather statistics about the following
elements, once a minute:

* Cassandra
* CPU
* Disk
* Firewall
* Memory
* Network
* Various Processes

In general, the statistics about processes is to check to know whether
they are running or not running.

The Cassandra plugin is specialized in testing various things about
Cassandra to make sure that it runs smoothly.

The CPU, Disk, Firewall, Memory, Network plugins just gather statistics
(how much of each is being used.) The statistics are saved in the data
directory. 7 days worth of data is kept then it gets overwritten. You
can find this data under:

    /var/lib/snapwebsites/snapwatchdog/data/...


Internal and External Scripts
=============================

Every time the snapwatchdog daemon wakes up, it runs a set of scripts,
which by default are saved under:

    /usr/share/snapwebsites/snapwatchdog/scripts/...

The scripts are expected to test whether things are running as expected.
This includes tested whether certain daemon are running or not. For example,
if snapcommunicator is not running, it will be reported.

The scripts are also used to detect bad things that are at times happening
on a server. For example, `fail2ban` version 0.9.3-1 has a process named
`fail2ban-client` which at times does not exit. Somehow it runs in a tight
loop using 100% of the CPU. We have a script in the snapfirewall that
detects whether the `fail2ban-client` runs for 3 minutes in a row. If that
happens, then it generates an error and sends an email.

The scripts will create files under the following directory:

    /var/lib/snapwebsites/snapwatchdog/scripts-output/...

This includes files that the scripts use to remember things (i.e. to know
how long a process has been running, for example) and we also save their
stdout and stderr streams to a "log" file there. Those logs is what we
send to the administrator's mailbox.


## Script: `watch_firewall_fail2ban_client` (`fail2ban-client`)

This script is insalled by the `snapfirewall` package and it is used to
make sure that `fail2ban-client` gets stop once it starts running and
using 100% of the CPU for too long. At this time it just sends an eamil
to the administrator who is expected to go on the server and make sure
that there is indeed a problem.

This is an issue in fail2ban version 0.9.3-1 (Ubuntu 16.04). Newer version
may not be affected.

It seems that this happens if the `logrotate` tool renames a file that the
`fail2ban-client` tool is working on. However, it is nearly impossible
to reproduce in a consistent way so there is no real resolution at this
point (i.e. according to the author, this version already has a fixed
`fail2ban-client` script.)


Todo
====

At this point many things are still very much missing: checking whether
the network is working as expected, checking various high level things
(i.e. whether Apache2 is not setup in Maintenance mode), and actually
reporting the info to the administrator in a front end within your
Snap! website installation.


Bugs
====

Submit bug reports and patches on
[github](https://github.com/m2osw/snapwebsites/issues).


_This file is part of the [snapcpp project](https://snapwebsites.org/)._
