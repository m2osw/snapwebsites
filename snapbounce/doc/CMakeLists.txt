#
# File:
#      CMakeLists.txt
#
# Description:
#      Create the documentation of this package.
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


##
## Documentation
##
AddDoxygenTarget(
    snapbounce
    ${SNAPWEBSITES_VERSION_MAJOR}
    ${SNAPWEBSITES_VERSION_MINOR}
    ${SNAPWEBSITES_VERSION_PATCH}
)

# TBD: move this file to the conf directory?
#      At this time I am thinking to not use this file but instead have
#      XML files that I load and then generate the firewall
install(
    FILES snapbounce-postfix.txt
    DESTINATION share/snapwebsites/conf
)

# vim: ts=4 sw=4 et nocindent
