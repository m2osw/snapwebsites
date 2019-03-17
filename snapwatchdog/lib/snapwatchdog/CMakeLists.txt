#
# File:
#      snapwatchdog/lib/snapwatchdog/CMakeLists.txt
#
# Description:
#      Library used by the Snap! Watchdog server and plugins.
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

project(snapwatchdog)

snap_validate_xml( watchdog-log-definitions.xml watchdog-log-definitions.xsd )
snap_validate_xml( snapwatchdog-tables.xml      tables.xsd                   )

set( CMAKE_AUTOMOC TRUE )

# Put the version in the header file
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/version.h
)


#find_package( log4cplus REQUIRED )
find_package( Qt5Core        REQUIRED )
find_package( Qt5Xml         REQUIRED )
find_package( Qt5XmlPatterns REQUIRED )

option( SNAP_NO_FORK OFF "Set to suppress watchdog server's inate forking [for debugging purposes only--not for release!]" )
if( ${SNAP_NO_FORK} )
    message( WARNING "SNAP_NO_FORK is turned on! This is not a production-ready build!" )
    add_definitions( -DSNAP_NO_FORK )
endif()

include_directories(
    #SYSTEM
        #${LOG4CPLUS_INCLUDE_DIR}
        ${QT_INCLUDES}
)

qt5_add_resources(SNAPWATCHDOG_RESOURCES_RCC
    snapwatchdog.qrc
)

add_library( ${PROJECT_NAME} SHARED
    snapwatchdog.cpp                            # Implements the core of the watchdog server
    log_definitions.cpp
    ${SNAPWATCHDOG_MOC_CXX}
    ${SNAPWATCHDOG_RESOURCES_RCC}
)

target_link_libraries( ${PROJECT_NAME}
    ${LOG4CPLUS_LIBRARY}
    Qt5::Core
    Qt5::Xml
    Qt5::XmlPatterns
)

set_target_properties( ${PROJECT_NAME} PROPERTIES
    VERSION ${SNAPWEBSITES_VERSION_MAJOR}.${SNAPWEBSITES_VERSION_MINOR}
    SOVERSION ${SNAPWEBSITES_VERSION_MAJOR}
)

install(
    TARGETS
        ${PROJECT_NAME}

    LIBRARY DESTINATION
        lib
)

install(
    FILES
        "snapwatchdog-tables.xml"

    DESTINATION
        ${SNAP_TABLE_DESCRIPTION_PATH}
)

install(
    DIRECTORY
        ${CMAKE_CURRENT_SOURCE_DIR}/

    DESTINATION
        include/snapwatchdog

    FILES_MATCHING PATTERN
        "*.h"
)

install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/version.h

    DESTINATION
        include/snapwatchdog
)

install(
    FILES
        watchdog-log-definitions.xml

    DESTINATION
        share/snapwebsites/snapwatchdog/log-definitions
)

install(
    FILES
        watchdog-log-definitions.xsd

    DESTINATION
        share/snapwebsites/xsd
)


# vim: ts=4 sw=4 et nocindent
