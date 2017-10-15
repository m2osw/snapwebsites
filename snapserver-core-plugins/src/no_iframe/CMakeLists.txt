#
# File:
#      plugins/no_iframe/CMakeLists.txt
#
# Description:
#      Prevent other websites from adding us inside an iframe.
#
# Documentation:
#      See the CMake documentation.
#
# License:
#      Copyright (c) 2017 Made to Order Software Corp.
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

project(plugin_no_iframe)

find_package( Qt5Core REQUIRED )

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
)

qt5_add_resources(SNAP_MANAGER_RESOURCES_RCC
    no_iframe.qrc
)


## Validate content
##
snap_validate_xml( content.xml                          ${snapwebsites_plugins_src_SOURCE_DIR}/content/content.dtd    )
snap_validate_xml( editor/no_iframe-settings-page.xml   ${snapwebsites_plugins_src_SOURCE_DIR}/editor/editor-form.dtd )
snap_validate_xml( layout/no_iframe-settings-parser.xsl simplified-xslt20.dtd                                         )
snap_validate_js( no-iframe/no-iframe.js )


add_library(no_iframe SHARED
    no_iframe.cpp
    ${SNAP_MANAGER_MOC_CXX}
    ${SNAP_MANAGER_RESOURCES_RCC}
    content.xml
    editor/no_iframe-settings-page.xml
    layout/no_iframe-settings-parser.xsl
    no-iframe/no-iframe.js
    no-iframe-logo-64x64.png
)

target_link_libraries( no_iframe Qt5::Core )

install(
    TARGETS no_iframe LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR}
)
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/snapwebsites/plugins/no_iframe
    FILES_MATCHING PATTERN "*.h"
)

install(
    FILES
        no-iframe/no-iframe.js
    DESTINATION
        share/javascript/snapwebsites/no-iframe
)

# vim: ts=4 sw=4 et
