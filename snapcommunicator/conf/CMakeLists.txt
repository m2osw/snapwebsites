#
# File:
#      snapcommunicator/conf/CMakeLists.txt
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


################################################################################
## Logger files
##
install(
    FILES 
        snapcommunicator.properties

    DESTINATION
        share/snapwebsites/logger/
)


################################################################################
## Service files
##
## This is really to create the directory and control its lifetime, the
## snapcommunicator itself is not listed as a service otherwise.
##
install(
    FILES
        README.md

    DESTINATION
        share/snapwebsites/services
)


# vim: ts=4 sw=4 et
