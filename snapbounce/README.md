

INTRODUCTION
============

The `snapbounce` tool is used to capture bounced emails from a postfix
mail server. The captured emails are saved in Cassandra which gives
the `snapserver` plugins a chance to know what bounced and inform
the corresponding administrator.


COMPILATION
===========

Please see the `INSTALL.txt` file for additional information. At this
time `snapbounce` is part of the Snap! set of projects and it will
compile automatically within that environment.

The project comes with two scripts that you may find useful:

* .../bin/snap-ubuntu-packages

This script installs all the packages that are required to compile
the entire environment as a developer. These packages are listed
in the various control files of each project, but these are not
useful for programmers who want to compile a version of Snap! on
their system. Thus we created this script.

* .../bin/build-snap

Build a DEBUG version of Snap! in a directory named `BUILD`.

The script also creates a `RELEASE` directory so one can also
compile in RELEASE mode, which once in a while is useful.


INSTALLATION
============

You may install the software with `make install`. However,
that won't be sufficient in order to get snapbounce to work.
You also need to setup the postfix environment to recognize
the tool as follow:


## /etc/postfix/transport.maps

In order to specify what tool to use on a bounced email, you
need to have a `transport.maps` file. This file will include
the following:

    # After changes, run postmap
    #   postmap /etc/postfix/transport.maps
    #
    bounces@snapwebsites.com snapbounce:


## /etc/postfix/main.cf

For the transport.maps to be taken in account, you have to
add / edit a few declarations in your main.cf file. There are
the three variables we set for `snapbounce` to function:

    notify_classes = bounce
    bounce_notice_recipient = bounces@snapwebsites.net
    transport_maps = hash:/etc/postfix/transport.maps

We also add the following variable, although it is not directly
in link with `snapbounce`, it seems to generally help postfix
do a better job:

    inet_protocols = all


## /etc/postfix/master.cf

Finally, we need to tell postfix how to invoke `snapbounce` whenever
a bounced email comes back to us. This is done by editing the
`master.cf` file and adding the following entry in it:

    # Service to save bounced emails in Cassandra
    snapbounce unix -       n       n       -       -       pipe
      flags=FRq user=snapwebsites argv=/usr/bin/snapbounce
      --sender ${sender} --recipient ${recipient}

You may want to adjust the name of the user(:group) depending on your
mail system. If the user does not exist, the command still gets
executed, but it gets executed as root.

The other parameters should not be changed unless you really know what
you are doing.


## Restart Postfix

To make sure postfix takes the changes in account, make sure to restart.

    sudo systemctl restart postfix


_This file is part of the [snapcpp project](http://snapwebsites.org/)._
