#
# File:
#      snapwebsites/snapfirewall/conf/CMakeLists.txt
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


snap_validate_xml( bundle-firewall.xml bundles.xsd )


#################################################################################
## Config files
##
install(
    FILES 
        # snap configuration files
        snapfirewall.conf

    DESTINATION
        share/snapwebsites/conf
)


#################################################################################
## Logger files
##
install(
    FILES 
        # log4cplus properties
        snapfirewall.properties

    DESTINATION
        share/snapwebsites/logger
)


#################################################################################
## Database tables
##
install(
    FILES "snapfirewall-tables.xml"
    DESTINATION ${SNAP_TABLE_DESCRIPTION_PATH}
)


#################################################################################
## Bundles are also saved under a bundle directory for reference
##
install(
    FILES
        bundle-firewall.xml

    DESTINATION
        share/snapwebsites/bundles
)


#################################################################################
## fail2ban files
##
install(
    FILES 
        01-iptables.conf
        iptables
        snap-apache-auth-filter.conf
        snap-apache-badbots-filter.conf
        snap-apache-fakegooglebot-filter.conf
        snap-apache-jail.conf
        snap-apache-proxy-filter.conf
        snap-apache-timeout-filter.conf
        snap-apache-unwanted-filter.conf
        snap-firewall-action.conf
        snap-iptables-filter.conf
        snap-jail.conf
        snap-recidive-jail.conf

    DESTINATION
        share/snapwebsites/fail2ban
)


# vim: ts=4 sw=4 et
