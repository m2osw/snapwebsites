
User Directory
==============

This directory is scanned for user versions of configuration files.
The files here are expected to have the exact same name as those found
one directory up and their content has priority (i.e. if the same field
is defined in the .conf one directory up, it will be ignored.)

So to override a parameter just redefine it in your version here.

For example, the `snapcommunicator.conf` file includes a parameter
named `my_address` which by default is set to `192.168.0.1`. To replace
it to `10.0.0.1`, do this:

    prompt $ sudo vim /etc/snapwebsites/snapwebsites.d/snapcommunicator.conf

Enter:

    my_address=10.0.0.1
    listen=10.0.0.1:4040

Then save and restart `snapcommunicator` with:

    systemctl restart snapcommunicator

Now it uses `10.0.0.1` instead of `192.168.0.1`. (Note that you most
probably want to change the `listen` parameter at the same time as
`my_address` since both addresses must be equal for things to work.)


snapmanager.cgi and snapmanagerdaemon
=====================================

Note that the `snapmanager.cgi` interface offers you to edit certain fields
(such as the `my_address` field shown above.)

The `snapmanagerdaemon` tool actually edits the files under the
`/etc/snapwebsites/snapwebsites.d` directory. If the file does not exist
yet, then it gets created there. In other words, changes through the
`snapmanager.cgi` interface are also viewed as user changes.


Reason Behind the User Directory
================================

The strong point of using this sub-directory is that this way you avoid
having to do a diff against the original settings any time you do an
upgrade to make sure you are where you want.


_This file is part of the [snapcpp project](http://snapwebsites.org/)._
