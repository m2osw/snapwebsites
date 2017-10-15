#
# File:
#      plugins/layout/CMakeLists.txt
#
# Description:
#      Generate a page content. This includes many different type of data such
#      as a regular page or a tag part of a taxonomy.
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

project(plugin_layout)

find_package( Qt5Core REQUIRED )

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
)

qt5_add_resources(SNAP_MANAGER_RESOURCES_RCC
    layout.qrc
)


## Validate content
##
snap_validate_css( layout/style.css                )
snap_validate_xml( layout/content.xml              ${snapwebsites_plugins_src_SOURCE_DIR}/content/content.dtd )
snap_validate_xml( layout/html-header.xsl          simplified-xslt20.dtd                                      )
snap_validate_xml( layout/default-theme-parser.xsl simplified-xslt20.dtd                                      )
snap_validate_xml( layout/functions.xsl            simplified-xslt20.dtd                                      )
snap_validate_xml( layout/user-messages.xsl        simplified-xslt20.dtd                                      )
snap_validate_xml( layout/default-body-parser.xsl  simplified-xslt20.dtd                                      )


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUTC_YEAR=\"`date -u +%Y`\" -DUTC_MONTH=\"`date -u +%-m`\" -DUTC_DAY=\"`date -u +%-d`\" -DUTC_HOUR=\"`date -u +%-H`\" -DUTC_MINUTE=\"`date -u +%-M`\" -DUTC_SECOND=\"`date -u +%-S`\"")

add_library(layout SHARED
    layout.cpp
    ${SNAP_MANAGER_MOC_CXX}
    ${SNAP_MANAGER_RESOURCES_RCC}
    layout/style.css
    layout/content.xml
    layout/functions.xsl
    layout/html-header.xsl
    layout/user-messages.xsl
    layout/default-body-parser.xsl
    layout/default-theme-parser.xsl
)

target_link_libraries( layout Qt5::Core )

install(
    TARGETS layout LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR}
)
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/snapwebsites/plugins/layout
    FILES_MATCHING PATTERN "*.h"
)
install(
    FILES "layout-tables.xml"
    DESTINATION ${SNAP_TABLE_DESCRIPTION_PATH}
)


# vim: ts=4 sw=4 et wrap
