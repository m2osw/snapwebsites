
INTRODUCTION
============

The `snapwatchdog` daemon runs in the background and check the health
of your system once every second. The results are saved in your Cassandra
cluster for later review.

The data includes basic computer status such as CPU and memory usage as
well as other data such as how much CPU and memory a `snap_child` process
used on its run.

TODO
====

At this point many things are still very much missing: checking whether
the network is working as expected, checking various high level things
(i.e. whether Apache2 is not setup in Maintenance mode), and actually
reporting the info to the administrator in a front end within your
Snap! website installation.

