
User Directory
==============

This directory is scanned for user versions of configuration files.
The files here are expected to start with two digits followed by one
dash and then the exact same name as those found one directory up.

The files are sorted from 00 to 99. The file with the highest number
(99) is loaded last. Parameters get overwritten so a parameter set in
99 hides all the values set to that parameter in any other files before
then. The file in the parent directory is read before the file named
`00-<name>.conf` it is has the lowest priority.

So to override parameters redefine them in your configuration file in
this folder. By default, the user definitions appear in the file using
50 (local to this computer) and 80 (global to your cluster).

For example, the `snapcommunicator.conf` file includes a parameter
named `my_address` which by default is set to `192.168.0.1`. To replace
it to `10.0.0.1`, do this:

    prompt $ sudo vim /etc/snapwebsites/snapwebsites.d/50-snapcommunicator.conf

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

By default, it uses a filename starting with `50-<name>.conf`. When defining
global settings, it uses `80-<name>.conf`. These files are made global by
copying the files on your entire cluster by using `snaprfs`.

Snap! Projects also create configuration files for other projects. In that
case, they most often use a number below 50 (i.e. to change the project's
defaults but not prevent the user from changing those values).


Reason Behind the User Directory
================================

The strong point of using this sub-directory is that this way you avoid
having to do a diff against the original settings any time you do an
upgrade to make sure you are where you want. This would really not be
practical in the `snapmanagerdaemon`!

The other point is the fact that with the two digits introducer, it
allows us to define project settings, user local settings, user global
settings, and group settings (like global but only copied on a limited
number of machines in your cluster).


Bugs
====

Submit bug reports and patches on
[github](https://github.com/m2osw/snapwebsites/issues).


_This file is part of the [snapcpp project](https://snapwebsites.org/)._
