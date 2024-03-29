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

cmake_minimum_required(VERSION 3.10.2)

project( snapbounce_project )

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wold-style-cast -Wnoexcept -DUNISTR_FROM_STRING_EXPLICIT=explicit -DMAGICKCORE_QUANTUM_DEPTH=16 -DMAGICKCORE_HDRI_ENABLE=0" )
set( CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -fwrapv" )

set(CMAKE_INCLUDE_CURRENT_DIR ON)

include_directories(
    SYSTEM
        ${Qt5Core_INCLUDE_DIRS}
        ${Qt5Xml_INCLUDE_DIRS}
)
set( QT_LIBRARIES
    Qt5::Core
    Qt5::Xml
)
include_directories(
    ${libsnapwebsites_project_SOURCE_DIR}
    ${libsnapwebsites_project_BINARY_DIR}
)

# This can't use the debian version one directory up
# but the version is defined as SNAPWEBSITES_VERSION_...
#SnapGetVersion( SNAPBOUNCE ${CMAKE_CURRENT_SOURCE_DIR} )

set( SNAPBOUNCE_VERSION_MAJOR ${SNAPWEBSITES_VERSION_MAJOR} )
set( SNAPBOUNCE_VERSION_MINOR ${SNAPWEBSITES_VERSION_MINOR} )
set( SNAPBOUNCE_VERSION_PATCH ${SNAPWEBSITES_VERSION_PATCH} )


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
