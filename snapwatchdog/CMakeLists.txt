#
# File:
#      snapwebsites/snapwatchdog/CMakeLists.txt
#
# Description:
#      Definitions to create the build environment with cmake
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
## Initialization
##
cmake_minimum_required(VERSION 3.5)


project( snapwatchdog_project )

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Automatically run the moc compiler on qt files
set( CMAKE_AUTOMOC TRUE )

## Include support modules
##
if( NOT CMAKE_PREFIX_PATH )
    set( CMAKE_PREFIX_PATH "/usr" )
endif()

set( CMAKE_MODULE_PATH
    ${CMAKE_PREFIX_PATH}/share/cmake/Modules
    ${PROJECT_SOURCE_DIR}/cmake
    ${CMAKE_MODULE_PATH}
)

# A few extra warnings specifically for snapwatchdog libraries and tools
# You can also play with -Weffc++ although we are definitively not compliant
# (especially we do not define all the auto-initialized variables!)
# -Wconversion -- would be nice, a few things are still not cooperating with that one
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wold-style-cast -Wnoexcept" )
set( CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -fwrapv" )

include_directories( SYSTEM
    ${Qt5Core_INCLUDE_DIRS}
    ${Qt5Network_INCLUDE_DIRS}
    ${Qt5Xml_INCLUDE_DIRS}
    ${Qt5XmlPatterns_INCLUDE_DIRS}
)

include_directories(
    ${PROJECT_SOURCE_DIR}/lib
    ${PROJECT_BINARY_DIR}
    ${PROJECT_BINARY_DIR}/lib
    ${libsnapwebsites_project_SOURCE_DIR}/src
    ${libsnapwebsites_project_BINARY_DIR}/src
    ${snapmanagercgi_project_BINARY_DIR}/lib
    ${snapmanagercgi_project_SOURCE_DIR}/lib
    ${QTSERIALIZATION_INCLUDE_DIRS}
)

set( SNAP_TABLE_DESCRIPTION_PATH lib/snapwebsites/tables )

snap_clear_xml_targets()

##
## Compiling
##
add_subdirectory(conf)              # Configuration files and samples
add_subdirectory(lib)               # Common code which is too specialized to create a separate library
add_subdirectory(plugins)           # Plug-ins (i.e. modular functionality)
add_subdirectory(src)               # Actual Snap! Websites server
add_subdirectory(manager-plugins)   # Actual Snap! Websites server

##
## Analyse xml and javascript
##
snap_build_xml_targets()

# vim: ts=4 sw=4 et nocindent
