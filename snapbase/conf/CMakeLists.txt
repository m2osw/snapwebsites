# Copyright (c) 2011-2019  Made to Order Software Corp.  All Rights Reserved
#
# https://snapwebsites.org/
# contact@m2osw.com
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA


#################################################################################
## Apache2 files
##
install(
    FILES
        # snap configuration files (shared between snapbackend and snapserver)
        snap-acceptfilter.conf
        snap-apache2-extra-options.conf
        snap-apache2-security.conf
        snap-apache2-ssl.conf
        snap-apache2-ssl-log.conf
        snap-httpoxy.conf

    DESTINATION
        share/snapwebsites/apache2
)


#################################################################################
## Logger files
##
install(
    FILES
        # log4cplus properties for snapserver, snapbase, and a few others
        #
        log.properties

    DESTINATION
        share/snapwebsites/logger
)


# vim: ts=4 sw=4 et
