#
# File:
#      snapbackend/conf/CMakeLists.txt
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


snap_validate_xml( bundle-snapbackend.xml bundles.xsd )


#################################################################################
## Bundles are also saved under a bundle directory for reference
##
install(
    FILES
        bundle-snapbackend.xml

    DESTINATION
        share/snapwebsites/bundles
)


###############################################################################
## A snapwatchdog script to check out a blooper
##
## This script checks on the CRON script wrongly named snapbackend
## and installed by snapbackend. The CRON script that was actually for
## the snapbackup package to run a daily backup of the Cassandra database.
##
## This is here because it was installed by snapbackend and unless you purge
## snapbackend and reinstall it, it will stick around. Since snapmanager.cgi
## automatically does a purge when it removes a bundle, it either isn't there
## already or this script will detect it and let you know there is a problem.)
##
install(
    PROGRAMS
        watch_snapbackup_old_cron_script

    DESTINATION
        share/snapwebsites/snapwatchdog/scripts
)


# vim: ts=4 sw=4 et
