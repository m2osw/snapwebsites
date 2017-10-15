#
# File:
#      snapwebsites/snapcgi/conf/CMakeLists.txt
#
# Description:
#      Install configuration files and samples.
#
# Documentation:
#      See the CMake documentation.
#
# License:
#      Copyright (c) 2011-2017 Made to Order Software Corp.
#
#      http://snapwebsites.org/
#      contact@m2osw.com
#
#      This program is free software; you can redistribute it and/or modify
#      it under the terms of the GNU General Public License as published by
#      the Free Software Foundation; either version 2 of the License, or
#      (at your option) any later version.
#     
#      This program is distributed in the hope that it will be useful,
#      but WITHOUT ANY WARRANTY; without even the implied warranty of
#      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#      GNU General Public License for more details.
#     
#      You should have received a copy of the GNU General Public License
#      along with this program; if not, write to the Free Software
#      Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
#

snap_validate_xml( bundle-frontend.xml bundles.xsd )


#################################################################################
## Config files
##
install(
    FILES 
        snapcgi.conf

    DESTINATION
        share/snapwebsites/conf
)


#################################################################################
## Apache Config files
##
install(
    FILES 
        # Sites
        000-snap-apache2-default-80.conf
        000-snap-apache2-default-443.conf
        000-snap-apache2-default-any.conf

        # User Configuration Templates
        user-snap-apache2-template-common.conf
        user-snap-apache2-template-80.conf
        user-snap-apache2-template-443.conf

        # User Configuration Files
        snap-apache2-maintenance.conf
        snap-apache2-vhost-security.conf

    DESTINATION
        share/snapwebsites/apache2
)


#################################################################################
## Maintenance files
##
install(
    FILES 
        # Index file
        favicon.ico
        snap-logo.png
        index.html

    DESTINATION
        share/snapwebsites/html/snap
)

#################################################################################
## Maintenance files
##
install(
    FILES 
        favicon.ico
        maintenance.html
        snap-logo.png

    DESTINATION
        share/snapwebsites/html/maintenance
)


#################################################################################
## Logger files
##
install(
    FILES 
        # log4cplus properties
        snapcgi.properties

    DESTINATION
        share/snapwebsites/logger
)


#################################################################################
## Bundles are also saved under a bundle directory for reference
##
install(
    FILES 
        bundle-frontend.xml

    DESTINATION
        share/snapwebsites/bundles
)


# vim: ts=4 sw=4 et
