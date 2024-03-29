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

##
## Initialization
##

cmake_minimum_required(VERSION 3.10.2)

project(snapbackup_project)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Automatically run the moc compiler on qt files
set( CMAKE_AUTOMOC TRUE )

## Include support modules
##
set( CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH} )

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wold-style-cast -Wnoexcept" )
set( CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -fwrapv"                       )

include_directories( SYSTEM
    ${Qt5Core_INCLUDE_DIRS}
    ${Qt5Network_INCLUDE_DIRS}
    ${Qt5Sql_INCLUDE_DIRS}
)
set( QT_LIBRARIES
    Qt5::Core
    Qt5::Network
    Qt5::Sql
)

include_directories(
    ${libsnapwebsites_project_SOURCE_DIR}
    ${libsnapwebsites_project_BINARY_DIR}
    ${snapmanagercgi_project_BINARY_DIR}/lib
    ${snapmanagercgi_project_SOURCE_DIR}/lib
    ${ADVGETOPT_INCLUDE_DIRS}
    ${CASSWRAPPER_INCLUDE_DIRS}
    ${CMAKE_CURRENT_BINARY_DIR}
)

snap_clear_xml_targets()

add_subdirectory( conf            )
add_subdirectory( src             )
add_subdirectory( manager-plugins )

snap_build_xml_targets()

# vim: ts=4 sw=4 et nocindent
