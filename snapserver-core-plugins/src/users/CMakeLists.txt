#
# File:
#      plugins/users/CMakeLists.txt
#
# Description:
#      Plug-in used to handle users on Snap! C++ and within specific websites.
#
# Documentation:
#      See the CMake documentation.
#
# License:
#      Copyright (c) 2012-2017  Made to Order Software Corp.
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

project(plugin_users)

find_package( Qt5Core REQUIRED )

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
)

qt5_add_resources(SNAP_MANAGER_RESOURCES_RCC
    users.qrc
)


snap_validate_xml( content.xml ${snapwebsites_plugins_src_SOURCE_DIR}/content/content.dtd )
snap_validate_js ( users.js                                                               )

add_library(users SHARED
    users.cpp
    user_info.cpp
    user_logged_info.cpp
    user_security.cpp
    ${SNAP_MANAGER_MOC_CXX}
    ${SNAP_MANAGER_RESOURCES_RCC}
    content.xml
    users.js
    users-logo-64x64.png
)

target_link_libraries( users Qt5::Core )

install(
    TARGETS users LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR}
)
install(
    FILES "users-tables.xml"
    DESTINATION ${SNAP_TABLE_DESCRIPTION_PATH}
)
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/snapwebsites/plugins/users
    FILES_MATCHING PATTERN "*.h"
)

install(
    FILES
        users.js
    DESTINATION
        share/javascript/snapwebsites/users
)

# vim: ts=4 sw=4 et
