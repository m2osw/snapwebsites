#
# File:
#      plugins/filter/CMakeLists.txt
#
# Description:
#      Plug-in used to filter XML data before sending it to a client. This
#      ensures that the data does not contain any XSS code.
#
# Documentation:
#      See the CMake documentation.
#
# License:
#      Copyright (c) 2011-2017  Made to Order Software Corp.
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

project(plugin_filter)

find_package( Qt5Core REQUIRED )

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
)

qt5_add_resources(SNAP_MANAGER_RESOURCES_RCC
    filter.qrc
)

snap_validate_xml( content.xml ${snapwebsites_plugins_src_SOURCE_DIR}/content/content.dtd )

add_library(filter SHARED
    filter.cpp
    token_help.cpp
    ${SNAP_MANAGER_MOC_CXX}
    ${SNAP_MANAGER_RESOURCES_RCC}
    content.xml
    filter-logo-64x64.png
    token-help.xsl
)

target_link_libraries( filter Qt5::Core )

install(
    TARGETS filter LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR}
)
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/snapwebsites/plugins/filter
    FILES_MATCHING PATTERN "*.h"
)

# vim: ts=4 sw=4 et
