
Snap Manager User Configuration Files
=====================================

The following two files are defined under:

   /usr/share/snapwebsites/apache2

These two files can be edited by the administrator.


user-snap-apache2-snapmanager-common.conf
=========================================

This file includes parameters that are common to the settings used by
port 80 and port 443. This includes the main domain name, the aliases,
the webmaster email address.

This also may include some security entry, although for snapmanager.cgi
this is generally not viewed as required.


user-snap-apache2-snapmanager-443.conf
======================================

This file includes parameters specific to port 443 settings, such
as the SSL files (keys, certificates...) to load for that one virtual
host.

By default it loads the ssl-cert-snakeoil files which are auto-generated
when installing Apache2. These are safe, only it is a self signed
certificate, so you get a warning in your browser when accessing the
page.


_This file is part of the [snapcpp project](http://snapwebsites.org/)._

