#
# File:
#      plugins/server_access/CMakeLists.txt
#
# Description:
#      Process the output of a query when the client sent an AJAX request.
#      This plugin intercepts all normal responses and transforms them to
#      AJAX and handles the expected AJAX responses with one specialized
#      function and response object.
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

project(plugin_server_access)

find_package( Qt5Core REQUIRED )

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
)

qt5_add_resources(SNAP_MANAGER_RESOURCES_RCC
    server_access.qrc
)



snap_validate_js( server-access.js )


add_library(server_access SHARED
    server_access.cpp
    server_access.h
    tests.cpp
    ${SNAP_MANAGER_MOC_CXX}
    ${SNAP_MANAGER_RESOURCES_RCC}
    content.xml
    server-access.js
    server-access-logo-64x64.png
)

target_link_libraries( server_access Qt5::Core )

install(
    TARGETS server_access LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR}
)
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/snapwebsites/plugins/server_access
    FILES_MATCHING PATTERN "*.h"
)

install(
    FILES
        server-access.js
    DESTINATION
        share/javascript/snapwebsites/server_access
)

# vim: ts=4 sw=4 et
