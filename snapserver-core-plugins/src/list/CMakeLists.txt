#
# File:
#      snapserver-core-plugins/src/list/CMakeLists.txt
#
# Description:
#      Manage lists of pages. This plugin is used to generate lists of
#      pages based on different criteria such as the title of the page,
#      the creation date, the author of the page, etc.
#
# Documentation:
#      See the CMake documentation.
#
# License:
#      Copyright (c) 2014-2017  Made to Order Software Corp.
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

project(plugin_list)

find_package( Qt5Core REQUIRED )

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
)

qt5_add_resources(SNAP_MANAGER_RESOURCES_RCC
    list.qrc
)

snap_validate_xml( list/content.xml             ${snapwebsites_plugins_src_SOURCE_DIR}/content/content.dtd )

snap_validate_xml( list/default-item-theme.xsl  simplified-xslt20.dtd )
snap_validate_xml( list/default-item-body.xsl   simplified-xslt20.dtd )
snap_validate_xml( list/default-list-theme.xsl  simplified-xslt20.dtd )
snap_validate_xml( list/default-list-body.xsl   simplified-xslt20.dtd )

snap_validate_js( list/list.js )

add_library(list SHARED
    list.cpp

    #tests.cpp -- the test was exclusively to make sure the "list" and "listref" tables were fine, at this point it's useless

    # data files
    list/content.xml
    list/list.js
    list/default-list-body.xsl
    list/default-list-theme.xsl
    list/default-item-body.xsl
    list/default-item-theme.xsl

    # xml and other content files
    list-tables.xml

    # Qt resources
    ${SNAP_MANAGER_MOC_CXX}
    ${SNAP_MANAGER_RESOURCES_RCC}
)

target_link_libraries(list
    Qt5::Core
    Qt5::Sql
)

install(
    TARGETS list
    LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR}
)
install(
    FILES "list-tables.xml"
    DESTINATION ${SNAP_TABLE_DESCRIPTION_PATH}
)
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/snapwebsites/plugins/list
    FILES_MATCHING PATTERN "*.h"
)

install(
    FILES
        list/list.js
    DESTINATION
        share/javascript/snapwebsites/list
)

# vim: ts=4 sw=4 et
