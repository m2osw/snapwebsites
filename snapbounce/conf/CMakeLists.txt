#
# File:
#      snapbounce/conf/CMakeLists.txt
#
# Description:
#      Install configuration files and samples.
#
# Documentation:
#      See the CMake documentation.
#
# License:
#      Copyright (c) 2011-2019  Made to Order Software Corp.  All Rights Reserved
#
#      https://snapwebsites.org/
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

snap_validate_xml( bundle-mailserver.xml bundles.xsd )
snap_validate_xml( bundle-snapmta.xml    bundles.xsd )


#################################################################################
## Bundles are also saved under a bundle directory for reference
##
install(
    FILES 
        bundle-mailserver.xml
        bundle-snapmta.xml

    DESTINATION
        share/snapwebsites/bundles
)


#################################################################################
## fail2ban files
##
install(
    FILES 
        snap-postfix-jail.conf

    DESTINATION
        share/snapwebsites/fail2ban
)


# vim: ts=4 sw=4 et
