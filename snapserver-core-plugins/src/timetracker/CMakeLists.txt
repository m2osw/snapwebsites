#
# File:
#      plugins/timetracker/CMakeLists.txt
#
# Description:
#      Time tracker.
#
# Documentation:
#      See the CMake documentation.
#
# License:
#      Copyright (c) 2013-2017 Made to Order Software Corp.
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

project(plugin_timetracker)

find_package( Qt5Core REQUIRED )

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
)

qt5_add_resources(SNAP_MANAGER_RESOURCES_RCC
    timetracker.qrc
)

snap_validate_css( timetracker/timetracker.css            )
snap_validate_xml( timetracker/content.xml                ${snapwebsites_plugins_src_SOURCE_DIR}/content/content.dtd    )
snap_validate_xml( editor/timetracker-page.xml            ${snapwebsites_plugins_src_SOURCE_DIR}/editor/editor-form.dtd )
snap_validate_xml( editor/timetracker-add-user-page.xml   ${snapwebsites_plugins_src_SOURCE_DIR}/editor/editor-form.dtd )
snap_validate_xml( editor/timetracker-settings-page.xml   ${snapwebsites_plugins_src_SOURCE_DIR}/editor/editor-form.dtd )
snap_validate_xml( layout/calendar-parser.xsl             simplified-xslt20.dtd                                         )
snap_validate_xml( layout/timetracker-parser.xsl          simplified-xslt20.dtd                                         )
snap_validate_xml( layout/timetracker-add-user-parser.xsl simplified-xslt20.dtd                                         )
snap_validate_xml( layout/timetracker-settings-parser.xsl simplified-xslt20.dtd                                         )
snap_validate_js ( timetracker/timetracker.js )


add_library(timetracker SHARED
    timetracker.cpp
    ${SNAP_MANAGER_MOC_CXX}
    ${SNAP_MANAGER_RESOURCES_RCC}
    editor/timetracker-page.xml
    editor/timetracker-add-user-page.xml
    editor/timetracker-settings-page.xml
    layout/calendar-parser.xsl
    layout/timetracker-parser.xsl
    layout/timetracker-add-user-parser.xsl
    layout/timetracker-settings-parser.xsl
    timetracker/content.xml
    timetracker/timetracker.css
    timetracker/timetracker.js
    timetracker/timetracker-logo-64x64.png
    timetracker/black-bicycle.png
    timetracker/black-car.png
    timetracker/black-client.png
    timetracker/black-commute.png
    timetracker/black-foot.png
    timetracker/black-telecommute.png
)

target_link_libraries( timetracker Qt5::Core )

install(
    TARGETS timetracker LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR}
)
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/snapwebsites/plugins/timetracker
    FILES_MATCHING PATTERN "*.h"
)

install(
    FILES
        timetracker/timetracker.js
    DESTINATION
        share/javascript/snapwebsites/timetracker
)

# vim: ts=4 sw=4 et
