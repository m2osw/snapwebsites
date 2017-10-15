#
# File:
#      plugins/detectadblocker/CMakeLists.txt
#
# Description:
#      Detect whether an ad blocker is active on the client's side.
#
# Documentation:
#      See the CMake documentation.
#
# License:
#      Copyright (c) 2016-2017  Made to Order Software Corp.
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

project(plugin_detectadblocker)

find_package( Qt5Core REQUIRED )

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
)

qt5_add_resources(SNAP_MANAGER_RESOURCES_RCC
    detectadblocker.qrc
)

snap_validate_xml( detectadblocker/content.xml                ${snapwebsites_plugins_src_SOURCE_DIR}/content/content.dtd    )
snap_validate_xml( editor/detectadblocker-settings-page.xml   ${snapwebsites_plugins_src_SOURCE_DIR}/editor/editor-form.dtd )
snap_validate_xml( layout/detectadblocker-settings-parser.xsl simplified-xslt20.dtd                                         )
snap_validate_js ( detectadblocker/detectadblocker.js )
snap_validate_js ( detectadblocker/adframe.js         )


add_library(detectadblocker SHARED
    detectadblocker.cpp
    ${SNAP_MANAGER_MOC_CXX}
    ${SNAP_MANAGER_RESOURCES_RCC}
    editor/detectadblocker-settings-page.xml
    layout/detectadblocker-settings-parser.xsl
    detectadblocker/content.xml
    detectadblocker/detectadblocker.js
    detectadblocker/detectadblocker-logo-64x64.png
    detectadblocker/adframe.js
)

target_link_libraries( detectadblocker Qt5::Core )

install(
    TARGETS detectadblocker LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR}
)
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/snapwebsites/plugins/detectadblocker
    FILES_MATCHING PATTERN "*.h"
)

install(
    FILES
        detectadblocker/detectadblocker.js
        detectadblocker/adframe.js
    DESTINATION
        share/javascript/snapwebsites/detectadblocker
)

# vim: ts=4 sw=4 et
