
# Installing an Instance

Create a droplet, VPS, server, etc. and install the OS (Ubuntu 16.04 at the
time of writing.) If you are to create many such VPS computers, and if
possible, you may want to create one original ("Clean") and make clones from
that one original.

## Install the following sources

### Package for the Cassandra C++ Driver

Edit the file with vim like so:

    sudo vim /etc/apt/sources.list.d/doug.list

Then enter the following

    # Cassandra Driver (2.x)
    deb [trusted=yes] http://apt.dooglio.net/xenial ./

**Note:** `libsnapwebsites` depends on `libqtcassandra`, which depends on
`cassandra-cpp-driver`. So you need this dependency to get started.
We certainly will add this dependency along the other packages.


### Snap! Websites Packages

Edit the file with vim like so:

    sudo vim /etc/apt/sources.list.d/build-m2osw.list

Then enter the following:

    # We include a top secret password, which unfortunately we do not release
    # (we will be looking at getting our sources on PPA again at some point, though.)
    deb [trusted=yes] https://user:password@build.m2osw.com/xenial ./

If you built your own version of packages, setup a website with this
Apache2 option turned on (it makes it easier to see that it works):

    Options +Indexes

The structure is just the name of the release you are on (i.e. trusty,
xenial...) and the complete list of all your .deb. You may also include
all the source files (all the files that `pbuilder` spits out!)

cd in the folder to run `apt-ftparchive` like so:

    cd /var/www/debian/xenial
    apt-ftparchive packages . > Packages
    gzip -f9c Packages > Packages.gz
    # And if you included the sources:
    apt-ftparchive sources . > Sources
    gzip -f9c Sources > Sources.gz

To get the `Packages` and `Sources` files, which apt-get requires to
find all the packages. Then create a "deb ..." entry that matches your
new website. If you may the website local, `[trusted=yes]` is 100% secure
all by itself. If it is a publicly accessible repository outside on the
Internet, you may want to use a secure connection.


## Make sure you are up to date

Once the sources are installed, run an update to make sure they get taken
in account:

    apt-get update


If you have not yet done so, make sure to upgrade your system to the latest,
it will make it easier to see errors if such occurs when installing snap:

    apt-get upgrade
    apt-get dist-upgrade
    apt-get autoremove


## First VPS Computer: Installing snapmanagercgi

Now you are ready to install `snapmanagercgi`, which is required on at least
one computer. If you need to install OpenVPN (a bundle which is part of
`snapmanagercgi`) because, for example, you have access to a private network
between your VPS servers, but that "private" network is shared between many
other users that you cannot trust 100%, then you need a VPN tunnel between
each computer. In that case you want to install snapmanagercgi on each
computer and install the OpenVPN, set it up properly and finally connect
each computer through that VPN (i.e. snapcommunicator neighbors IP addresses
will be those offered by OpenVPN, not the "physical" private network IP
addresses.)

Along with snapmanagercgi comes Apache2 which is used to access the CGI.

WARNING: the `snapmanagercgi` package is expected to be installed on a
pristine computer node. It will handle the Apache2 setup automatically,
but only if it was not already installed and included many setup files
that could contradict our own installation features.

    sudo apt-get install snapmanagercgi


## Following VPS Computers: Installing snapmanagerdaemon

On the other computers, you want to install the `snapmanagerdaemon` package.
This starts `snapinit`, `snapcommunicator`, and `snapmanagerdaemon`. By
default `snapcommunicator` will not be communicating with any neighbors.
However, `snapmanagercgi` will offer you to include neighbors IP addresses.
[TODO: ONE IMPLEMENTED--neighbors editing is not yet in place]
By doing so, you automatically create the cluster.

    sudo apt-get install snapmanagerdaemon

Other bundles can then be installed from the `snapmanager.cgi` frontend
screens without having to access your backend servers at any time.

As mentioned in the previous section, you should install snapmanagercgi
on all computers if you need to install OpenVPN before you can add
the `snapcommunicator` neighbors.





# About snapmanager.cgi

The system functions using plugins in order to allow any number of
features to be added to the manager. This is specifically important
for some plugins may have some special needs that have to be in
the snapmanager.cgi instead of /admin/settings/... (i.e. it may
require a root process to handle the settings.)

The manager is composed of:

  - snapmanagercgi/lib/* -- a library with common functions and classes
  - snapmanagercgi/cgi/* -- the snapmanager.cgi binary
  - snapmanagercgi/daemon/* -- the snapmanagerdaemon binary
  - snapmanagercgi/plugins/*/* -- a set of plugins
  - snapmanagercgi/upgrader/* -- a separate tool to upgrade packages

From the outside the user can access snapmanager.cgi which will run with
the same user/group restrictions as Apache (www-data:www-data). It can
connect to snapmanagerdaemon which accepts messages to act on the system
by adding/removing software and tweaking their settings as required.

The `snapmanager.cgi` and `snapmanagerdaemon` are both linked against the
library and can both load the plugins.

The plugins are useful for three main tasks:

  - gathering data on a computer, this is mainly an installation status
    opposed to the current status as snapwatchdog provides (although
    there may be some overlap)
  - transform the gathered data in HTML for display
  - allow for tweaking the data

The plugin implementation makes use of the libsnapwebsites library to
handle the loading. Note that the snapmanagercgi system always loads
all the plugins available. We think this is important because it always
installs and removes software and as it does so, adds and removes
various plugins. For example, the `snapwatchdog` is an optional (although
recommended) feature and as such its plugin extension will not be
installed by default.

However, to know what is installable... at this point I have not
real clue on how to get that attached to the system (i.e. if something
is not yet installed, how could `snapmanager` know about it?!)

Organization:

    +-----------------------------+            +----------------+
    |                             |            |                |
    | libsnapmanagercgi           |  linked    | Upgrader       |
    | (common code/functions)     |<-----------+                |
    |                             |            |                |
    +-----------------------------+            +----------------+
         ^
         |
         | linked
         |
         +-------------------------------------+------------------------+
         |                                     |                        |
         |                                     |                        |
    +----+------------------------+   +--------+--------------------+   |
    |                             |   |                             |   |
    | snapmanager.cgi             |   | snapmanagerdaemon           |   |
    |                             |   |                             |   |
    +------------------------+----+   +--------+--------------------+   |
                             |                 |                        |
                             |                 |                        |
                             +-----------------+                        |
                             |                                          |
                             | load (dynamic link)                      |
                             v                                          |
                      +-----------------------------+                   |
                      |                             +-------------------+
                      | plugins                     |
                      |                             |
                      +-----------------------------+



vim: ts=4 sw=4 et

_This file is part of the [snapcpp project](http://snapwebsites.org/)._
