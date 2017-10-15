#
# File:
#      snapwebsites/snapbounce/CMakeLists.txt
#
# Description:
#      The Snap! Websites Server. The server is the main() of the Snap!
#      environment. Most everything is however defined in the
#      libsnapwebsites.so library (see lib/*).
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
cmake_minimum_required(VERSION 3.5)

project( snapbounce_project )

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wold-style-cast -Wnoexcept -DUNISTR_FROM_STRING_EXPLICIT=explicit -DMAGICKCORE_QUANTUM_DEPTH=16 -DMAGICKCORE_HDRI_ENABLE=0" )
set( CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -fwrapv" )

set(CMAKE_INCLUDE_CURRENT_DIR ON)

include_directories(
    ${libsnapwebsites_project_SOURCE_DIR}/src
    ${libsnapwebsites_project_BINARY_DIR}/src
)
include_directories( SYSTEM
    ${Qt5Core_INCLUDE_DIRS}
    ${Qt5Xml_INCLUDE_DIRS}
)
set( QT_LIBRARIES
    Qt5::Core
    Qt5::Xml
)

#SnapGetVersion( SNAPBOUNCE ${CMAKE_CURRENT_SOURCE_DIR} )

find_library( UUID uuid )
if( ${UUID} STREQUAL "UUID-NOTFOUND" )
    message( FATAL_ERROR "Cannot find uuid library!" )
endif()

snap_clear_xml_targets()

add_subdirectory( conf            )
add_subdirectory( src             )
add_subdirectory( manager-plugins )

snap_build_xml_targets()

# vim: ts=4 sw=4 et
