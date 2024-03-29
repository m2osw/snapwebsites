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

project(snapserver_core_plugins_project)

################################################################################
# We need a common place to install the plugins
#
set(PLUGIN_INSTALL_DIR lib/snapwebsites/plugins)

################################################################################
# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

################################################################################
# Automatically run the moc compiler on qt files
set(CMAKE_AUTOMOC TRUE)

set(QT_LIBRARIES
    Qt5::Core
    Qt5::Network
    Qt5::Script
    Qt5::Sql
    Qt5::Xml
    Qt5::XmlPatterns
)

#SnapGetVersion(COREPLUGINS ${CMAKE_CURRENT_SOURCE_DIR})

set(COREPLUGINS_VERSION_MAJOR ${SNAPWEBSITES_VERSION_MAJOR})
set(COREPLUGINS_VERSION_MINOR ${SNAPWEBSITES_VERSION_MINOR})
set(COREPLUGINS_VERSION_PATCH ${SNAPWEBSITES_VERSION_PATCH})

find_library(UUID uuid)
if(${UUID} STREQUAL "UUID-NOTFOUND")
    message(FATAL_ERROR "Cannot find uuid library!")
endif()

find_program(ZIP zip REQUIRED)

if("${XMLLINT_EXECUTABLE}" STREQUAL "XMLLINT_EXECUTABLE-NOTFOUND")
    message( FATAL_ERROR "xmllint was not found, you need to install libxml2-utils" )
endif()

# A few extra warnings:
#
# -Wconversion -- would be nice, a few things are still not cooperating with that one
# -DUNISTR_FROM_STRING_EXPLICIT=explicit is for the Unicode library, to make sure it does not copy strings arounds automatically
# -DMAGICKCORE_QUANTUM_DEPTH=16 is the default depth, newer versions of ImageMagick require that definition from you (no internal default)
# -DMAGICKCORE_HDRI_ENABLE=0 is also the default for ImageMagick; I'm not too sure what HDRI stands for though...
#
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wold-style-cast -Wnoexcept -DUNISTR_FROM_STRING_EXPLICIT=explicit -DMAGICKCORE_QUANTUM_DEPTH=16 -DMAGICKCORE_HDRI_ENABLE=0" )
set( CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -fwrapv" )

include_directories(
    SYSTEM
        ${Qt5Core_INCLUDE_DIRS}
        ${Qt5Network_INCLUDE_DIRS}
        ${Qt5Script_INCLUDE_DIRS}
        ${Qt5Sql_INCLUDE_DIRS}
        ${Qt5Widgets_INCLUDE_DIRS}
        ${Qt5Xml_INCLUDE_DIRS}
        ${Qt5XmlPatterns_INCLUDE_DIRS}
)

include_directories(
    ${libsnapwebsites_project_SOURCE_DIR}
    ${libsnapwebsites_project_BINARY_DIR}
    ${SITTER_INCLUDE_DIRS}
    ${ADVGETOPT_INCLUDE_DIRS}
    ${QTCASSANDRA_INCLUDE_DIRS}
    ${QTSERIALIZATION_INCLUDE_DIRS}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${LIBTLD_INCLUDE_DIRS}
)

##
## Clear these targets for linting
##
snap_clear_css_targets()
snap_clear_js_targets()
snap_clear_xml_targets()

add_subdirectory( conf             )
add_subdirectory( src              )
#add_subdirectory( sitter-plugins   ) -- the list will be fixed, so at this point we don't need this
add_subdirectory( doc              )

##
## Analyse css, xml and javascript
##
snap_build_css_targets()
snap_build_js_targets()
snap_build_xml_targets()

# vim: ts=4 sw=4 et nocindent
