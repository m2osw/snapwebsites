
INTRODUCTION
============

The `snaplog` daemon listens to `LOG` messages that are sent by the
`libsnapwebsites/src/log.cpp` implementation whenever the `MessengerAppender`
is setup in your log4cplus properties.


DEFAULT LEVEL
=============

By default the level used to send logs to the `snaplog` daemon is `INFO`.
We strongly suggest that you do not decrease that level any further on
a live cluster as it would adds a lot of traffic on the network, traffic
that could imper your database accesses, which also uses your network.


SETUP
=====

We make use of the bundle installation mechanism to get everything
installed as required, including the MySQL database system. Everything
else should be automatic.


_This file is part of the [snapcpp project](http://snapwebsites.org/)._
