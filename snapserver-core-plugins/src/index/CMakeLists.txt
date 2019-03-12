#
# File:
#      snapserver-core-plugins/src/index/CMakeLists.txt
#
# Description:
#      Manage indexes of pages. This plugin is used to generate indexes of
#      pages based on different criteria such as the title of the page,
#      the creation date, the author of the page, etc.
#
# Documentation:
#      See the CMake documentation.
#
# License:
#      Copyright (c) 2019  Made to Order Software Corp.  All Rights Reserved
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

project(plugin_index)

find_package( Qt5Core REQUIRED )

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
)

qt5_add_resources(SNAP_MANAGER_RESOURCES_RCC
    index.qrc
)


snap_validate_xml( index/content.xml               content.dtd           )

snap_validate_xml( index/default-record-theme.xsl  simplified-xslt20.dtd )
snap_validate_xml( index/default-record-body.xsl   simplified-xslt20.dtd )
snap_validate_xml( index/default-index-theme.xsl   simplified-xslt20.dtd )
snap_validate_xml( index/default-index-body.xsl    simplified-xslt20.dtd )

snap_validate_xml( index-tables.xml                tables.xsd            )


snap_validate_js( index/index.js )


add_library(index SHARED
    index.cpp

    # data files
    index/content.xml
    index/index.js
    index/default-index-body.xsl
    index/default-index-theme.xsl
    index/default-index-body.xsl
    index/default-index-theme.xsl

    # xml and other content files
    index-tables.xml

    # Qt resources
    ${SNAP_MANAGER_MOC_CXX}
    ${SNAP_MANAGER_RESOURCES_RCC}
)

target_link_libraries(index
    Qt5::Core
    Qt5::Sql
)

install(
    TARGETS index
    LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR}
)
install(
    FILES "index-tables.xml"
    DESTINATION ${SNAP_TABLE_DESCRIPTION_PATH}
)
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/snapwebsites/plugins/index
    FILES_MATCHING PATTERN "*.h"
)

install(
    FILES
        index/index.js
    DESTINATION
        share/javascript/snapwebsites/index
)

# vim: ts=4 sw=4 et
