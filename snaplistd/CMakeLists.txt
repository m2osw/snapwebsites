#
# File:
#      snapwebsites/snaplistd/CMakeLists.txt
#
# Description:
#      A daemon to save list journals on the computer(s) running the
#      "pagelist" backend.
#
# Documentation:
#      See the CMake documentation.
#
# License:
#      Copyright (c) 2016-2017 Made to Order Software Corp.
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
cmake_minimum_required(VERSION 3.5)

project( snaplistd_project )

# Find includes in corresponding build directories
set( CMAKE_INCLUDE_CURRENT_DIR ON )

include_directories( SYSTEM
    ${Qt5Core_INCLUDE_DIRS}
    ${Qt5Xml_INCLUDE_DIRS}
)
set( QT_LIBRARIES
    Qt5::Core
    Qt5::Sql
    Qt5::Xml
)
include_directories(
    ${libsnapwebsites_project_SOURCE_DIR}/src
    ${libsnapwebsites_project_BINARY_DIR}/src
    ${QTSERIALIZATION_INCLUDE_DIRS}
)

option( RUN_TESTS "Enable tests to run via CTest" OFF )
if( ${RUN_TESTS} )
    enable_testing()
endif()

add_subdirectory( conf  )
add_subdirectory( src   )
add_subdirectory( doc   )
#add_subdirectory( tests ) TODO

# vim: ts=4 sw=4 et
