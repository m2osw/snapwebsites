# Copyright (c) 2016-2019  Made to Order Software Corp.  All Rights Reserved
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

project(snapmanager)

set( CMAKE_AUTOMOC TRUE )

# Put the version in the header file
#
file( MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/snapmanager )
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/snapmanager/version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/snapmanager/version.h
)

#find_package( log4cplus REQUIRED )
find_package( Qt5Core        REQUIRED )
find_package( Qt5Xml         REQUIRED )
find_package( Qt5XmlPatterns REQUIRED )

set( HEADERS
    snapmanager/bundle.h                        # Helper class to load bundle XML files
    snapmanager/form.h                          # Helper classes to generate forms
    snapmanager/manager.h                       # Implements the core of the watchdog server
    snapmanager/plugin_base.h                   # Base for all plugins to add a few direct callbacks
    snapmanager/server_status.h                 # Handle status data
    snapmanager/status.h                        # Hanele one specific status
)

add_library( ${PROJECT_NAME} SHARED
    ${HEADERS}
    bundle.cpp                                  # Helper class to load bundle XML files
    form.cpp                                    # Helper classes to generate forms
    installer.cpp                               # Code used to install bundles on a server
    manager.cpp                                 # Implements the core of the watchdog server
    plugin_base.cpp                             # Base for all plugins to add a few direct callbacks
    server_status.cpp                           # Handle status data
    status.cpp                                  # Hanele one specific status
)


target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${LIBEXCEPT_INCLUDE_DIRS}
        ${CMAKE_CURRENT_BINARY_DIR}
        ${QT_INCLUDES}
)


target_link_libraries( ${PROJECT_NAME}
    ${ADVGETOPT_LIBRARIES}
    ${LIBEXCEPT_LIBRARIES}
    ${SNAPLOGGER_LIBRARY}
    ${QT_LIBRARIES}
    ${SNAPWEBSITES_LIBRARIES}
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
    DIRECTORY
        ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION
        include
    FILES_MATCHING PATTERN
        "snapmanager/*.h"
)
install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/snapmanager/version.h
    DESTINATION
        include/snapmanager
)

# vim: ts=4 sw=4 et nocindent
