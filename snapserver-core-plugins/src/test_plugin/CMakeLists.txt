# Copyright (c) 2013-2019  Made to Order Software Corp.  All Rights Reserved
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

project(plugin_test_plugin)

find_package( Qt5Core REQUIRED )

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
)

qt5_add_resources(SNAP_MANAGER_RESOURCES_RCC
    test_plugin.qrc
)

snap_validate_xml( content.xml            content.dtd           )
snap_validate_xml( test_plugin-tables.xml tables.xsd            )
snap_validate_xml( test-plugin-parser.xsl simplified-xslt20.dtd )
snap_validate_css( test-plugin.css                              )


add_library(test_plugin SHARED
    test_plugin.cpp
    ${SNAP_MANAGER_MOC_CXX}
    ${SNAP_MANAGER_RESOURCES_RCC}
    test-plugin.css
    content.xml
    test-plugin-parser.xsl
    test-plugin-logo-64x64.jpg
)

target_link_libraries( test_plugin Qt5::Core )

install(
    TARGETS test_plugin LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR}
)
install(
    FILES "test_plugin-tables.xml"
    DESTINATION ${SNAP_TABLE_DESCRIPTION_PATH}
)
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/snapwebsites/plugins/test_plugin
    FILES_MATCHING PATTERN "*.h"
)

install(
    FILES
        test-plugin.js
    DESTINATION
        share/javascript/snapwebsites/test_plugin
)

# vim: ts=4 sw=4 et
