#
# File:
#      snapwebsites/snapmanagercgi/conf/CMakeLists.txt
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

project(snapmanagercgi_conf)

snap_validate_xml( snapmanagercgi-parser.xsl simplified-xslt20.dtd )

snap_validate_xml( bundle-cassandra.xml  bundles.xsd )
snap_validate_xml( bundle-dns.xml        bundles.xsd )
snap_validate_xml( bundle-rkhunter.xml   bundles.xsd )
snap_validate_xml( bundle-ssh.xml        bundles.xsd )
snap_validate_xml( bundle-swap.xml       bundles.xsd )
snap_validate_xml( bundle-tools.xml      bundles.xsd )
snap_validate_xml( bundle-tripwire.xml   bundles.xsd )
snap_validate_xml( bundle-vpn_client.xml bundles.xsd )
snap_validate_xml( bundle-vpn_server.xml bundles.xsd )
snap_validate_js ( snapmanagercgi.js                 )
snap_validate_css( snapmanagercgi.css                )


#################################################################################
## Config files
##
install(
    FILES
        # snap configuration files
        snapmanager.conf

        # misc files
        snapmanagercgi-parser.xsl
        snapmanagercgi.js
        snapmanagercgi.css

    DESTINATION
        share/snapwebsites/conf
)
#
file(
    INSTALL
        bundles.xsd
    DESTINATION
        ${CMAKE_BINARY_DIR}/dist/share/snapwebsites/xsd
)
#
install(
    FILES
        jquery/jquery.js
        jquery/jquery-ui.js
        jquery/jquery-ui.css

    DESTINATION
        share/snapwebsites/conf/jquery
)


#################################################################################
## Apache2 files
##
install(
    FILES
        # Sites
        snapmanager-apache2.conf

        # User Configuration Files
        user-snap-apache2-snapmanager-common.conf
        user-snap-apache2-snapmanager-443.conf

    DESTINATION
        share/snapwebsites/apache2
)


#################################################################################
## Logger files
##
install(
    FILES
        # log4cplus properties
        snapmanagercgi.properties
        snapmanagerdaemon.properties

    DESTINATION
        share/snapwebsites/logger
)


#################################################################################
## Save HTML files in their own directory
##
install(
    FILES
        # HTML files
        index.html
        favicon.ico
        globe.gif
        globe_still.png
        snapmanagercgi-login.html

    DESTINATION
        share/snapwebsites/html/snapmanager
)

#################################################################################
## Save XSD files in their own directory
##
install(
    FILES
        bundles.xsd

    DESTINATION
        share/snapwebsites/xsd
)


#################################################################################
## Bundles are also saved under a bundle directory for reference
##
install(
    FILES
        bundle-cassandra.xml
        bundle-dns.xml
        bundle-mysql.xml
        bundle-rkhunter.xml
        bundle-ssh.xml
        bundle-swap.xml
        bundle-tools.xml
        bundle-tripwire.xml
        bundle-vpn_client.xml
        bundle-vpn_server.xml

    DESTINATION
        share/snapwebsites/bundles
)


#################################################################################
## fail2ban files
##
install(
    FILES
        snap-named-jail.conf

    DESTINATION
        share/snapwebsites/fail2ban
)


# vim: ts=4 sw=4 et
