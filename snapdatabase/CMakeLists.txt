# Copyright (c) 2019  Made to Order Software Corp.  All Rights Reserved
#
# https://snapwebsites.org/project/snapdatabase
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

cmake_minimum_required(VERSION 3.10.2)

###############################################################################
## The Snap! Versatile Database Support
##
project(snapdatabase_project)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(SNAP_TABLE_DESCRIPTION_PATH lib/snapwebsites/tables)

#SnapGetVersion(SNAPDATABASE ${CMAKE_CURRENT_SOURCE_DIR})

set(SNAPDATABASE_VERSION_MAJOR ${SNAPWEBSITES_VERSION_MAJOR})
set(SNAPDATABASE_VERSION_MINOR ${SNAPWEBSITES_VERSION_MINOR})
set(SNAPDATABASE_VERSION_PATCH ${SNAPWEBSITES_VERSION_PATCH})

include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}

    ${libsnapwebsites_project_SOURCE_DIR}
    ${libsnapwebsites_project_BINARY_DIR}
    ${snapmanagercgi_project_BINARY_DIR}/lib
    ${snapmanagercgi_project_SOURCE_DIR}/lib

    ${LIBEXCEPT_INCLUDE_DIRS}
    ${SNAPDEV_INCLUDE_DIRS}
    ${SNAPLOGGER_INCLUDE_DIRS}
)

snap_clear_xml_targets()

add_subdirectory( cmake           )
add_subdirectory( conf            )
add_subdirectory( snapdatabase    )     # library first
add_subdirectory( proxy           )
add_subdirectory( daemon          )
add_subdirectory( manager-plugins )
add_subdirectory( tests           )
add_subdirectory( doc             )

snap_build_xml_targets()

# vim: ts=4 sw=4 et
