#
# File:
#      snapserver-core-plugins/src/flash/CMakeLists.txt
#
# Description:
#      Allows for flash animations to be played back from a Snap! Website.
#      Also includes JavaScript to detect Flash support. Maybe more later
#      (i.e. add ability to insert Flash animations in editor).
#
# Documentation:
#      See the CMake documentation.
#
# License:
#      Copyright (c) 2013-2019  Made to Order Software Corp.  All Rights Reserved
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

project(plugin_flash)

find_package( Qt5Core REQUIRED )

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
)

qt5_add_resources(SNAP_MANAGER_RESOURCES_RCC
    flash.qrc
)


snap_validate_xml( content.xml               content.dtd           )
snap_validate_xml( flash-settings-page.xml   editor-form.dtd       )
snap_validate_xml( flash-settings-parser.xsl simplified-xslt20.dtd )


add_library(flash SHARED
    flash.cpp
    ${SNAP_MANAGER_MOC_CXX}
    ${SNAP_MANAGER_RESOURCES_RCC}
    content.xml
    crossdomain.xml
    flash-detection.js
    flash-settings-page.xml
    flash-settings-parser.xsl
    flash-player-logo-64x64.png
)

target_link_libraries( flash Qt5::Core )

install(
    TARGETS flash
    LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR}
)
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/snapwebsites/plugins/flash
    FILES_MATCHING PATTERN "*.h"
)


# vim: ts=4 sw=4 et
