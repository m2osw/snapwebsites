
User Template Configuration Files
=================================

The following three files are defined under:

   /usr/share/snapwebsites/apache2

We reuse these templates each time a new apache2 configuration file is
created for a different domain (which is useful only if the domain
has specific settings, in most cases this means an SSL certificate of
its own.)

They are also used by the default Snap! environment.


user-snap-apache2-template-common.conf
======================================

This file includes parameters that are common to the settings used by
port 80 and port 443. This includes the main domain name, the aliases,
the webmaster email address.


user-snap-apache2-template-80.conf
==================================

This file includes parameters specific to port 80 settings.


user-snap-apache2-template-443.conf
===================================

This file includes parameters specific to port 443 settings, such
as the SSL files (keys, certificates...) to load for that one virtual
host.


_This file is part of the [snapcpp project](http://snapwebsites.org/)._
