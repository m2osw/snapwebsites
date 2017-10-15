
User Configuration Files
========================

The files defined in this directory are editable by end users.

These files are installed once on the very first installation and edited
by `snapmanagerdaemon` or manually by system administrators.

They will never get updated and should be secure as it is because the
real security happens in files that we consider as being READ-ONLY.


Which Site Should I Enable By Default
=====================================

By default, we setup the configuration file to make your Snap! website
work on port 80. This is the default because without the necessary files,
we cannot setup an SSL based website on port 443.

If you have a signed certificate or can obtain one, you may want to change
the default Snap! site with one of the following set of commands:

1. Port 80 only

    sudo a2dissite 000-snap-apache2-default-443
    sudo a2dissite 000-snap-apache2-default-any
    sudo a2ensite 000-snap-apache2-default-80

2. Port 443, hits to port 80 get redirected

    sudo a2dissite 000-snap-apache2-default-80
    sudo a2dissite 000-snap-apache2-default-any
    sudo a2ensite 000-snap-apache2-default-443

3. Either Port 80 or 443, no automatic redirection in Apache2

    sudo a2dissite 000-snap-apache2-default-80
    sudo a2dissite 000-snap-apache2-default-443
    sudo a2ensite 000-snap-apache2-default-any


snap-apache2-maintenance.conf
=============================

The default maintenance settings are defined in this file.

Although it works for us, you may want to tweak the parameters of this
file so change the way the maintenance screen is send to the client.


snap-apache2-security.conf
==========================

The default User defined security options. Remember that these will
be included by all the `<VirtualHost ...>` definitions pertaining to
Snap! (unless you change that scheme.)

By default, there are no additional option for security.

Also, if you want specific security for just one website, you will want
to edit that website settings and not this common security file.


_This file is part of the [snapcpp project](http://snapwebsites.org/)._
