#
# File:
#      snapwebsites/snapbounce/manager-plugins/mailserver/CMakeLists.txt
#
# Description:
#      Manage parameters from snapmanager.cgi and snapmanagerdaemon
#      and their environment.
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

project(plugin_mailserver)

find_package( Qt5Core REQUIRED )
find_package( Qt5Xml  REQUIRED )

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${snapmanagercgi_project_BINARY_DIR}/lib
    ${snapmanagercgi_project_SOURCE_DIR}/lib
    ${QTSERIALIZATION_INCLUDE_DIRS}
)

qt5_add_resources(SNAP_MANAGER_RESOURCES_RCC
    mailserver.qrc
)

add_library(mailserver SHARED
    mailserver.cpp
    ${SNAP_MANAGER_MOC_CXX}
    ${SNAP_MANAGER_RESOURCES_RCC}
)

#message("AS2JS_LIBRARIES = " ${AS2JS_LIBRARIES})

target_link_libraries(mailserver
    ${QT_LIBRARIES}
)

install(
    TARGETS
        mailserver

    LIBRARY DESTINATION
        ${PLUGIN_INSTALL_DIR}
)

file(GLOB HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
install(
    FILES
        ${HEADER_FILES}

    DESTINATION
        ${PLUGIN_HEADERS_DIR}/mailserver
)

# vim: ts=4 sw=4 et
