#
# File:
#      snapmanagercgi/CMakeLists.txt
#
# Description:
#      The Snap! Manager CGI to manage a Snap! cluster from your
#      browser.
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

##
## The Snap! Manager CGI
##
project( snapmanagercgi_project )

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Automatically run the moc compiler on qt files
set( CMAKE_AUTOMOC TRUE )

##
## CMake module path
##
if( NOT CMAKE_PREFIX_PATH )
    set( CMAKE_PREFIX_PATH "/usr" )
endif()

set( CMAKE_MODULE_PATH
    ${CMAKE_PREFIX_PATH}/share/cmake/Modules
    ${CMAKE_MODULE_PATH}
)

set( QT_LIBRARIES
    Qt5::Core
    Qt5::Network
    Qt5::Xml
)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/lib
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/lib
    ${libsnapwebsites_project_SOURCE_DIR}/src
    ${libsnapwebsites_project_BINARY_DIR}/src
    ${QTSERIALIZATION_INCLUDE_DIRS}
)

snap_clear_xml_targets()

##
## Add sub-projects
##
add_subdirectory( conf     )          # Configuration files and samples
add_subdirectory( lib      )          # Common code to CGI, daemon, plugins
add_subdirectory( cgi      )          # The snapmanager.cgi application
add_subdirectory( daemon   )          # The snapmanagerdaemon to apply commands on any computer
add_subdirectory( plugins  )          # "Core" plugins used to gather and update the status
add_subdirectory( upgrader )          # Tool to run "apt-get upgrade" without getting smashed
add_subdirectory( password )          # Tool to manager password files
add_subdirectory( doc      )          # Documentation generation

option( BUILD_GUI "Build the snapmanager gui (requires Qt5 Widgets and Webkit)." ON )
if( ${BUILD_GUI} )
    add_subdirectory( gui )
endif()

snap_build_xml_targets()

# vim: ts=4 sw=4 et nocindent
