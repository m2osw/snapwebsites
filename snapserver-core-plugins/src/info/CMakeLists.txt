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

project(plugin_info)

find_package( Qt5Core REQUIRED )

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
)

qt5_add_resources(SNAP_MANAGER_RESOURCES_RCC
    info.qrc
)


## Validate content
##
snap_validate_css( info.css                                                   )
snap_validate_xml( content.xml                          content.dtd           )
snap_validate_xml( editor/info-settings-page.xml        editor-form.dtd       )
snap_validate_xml( editor/plugin-settings-page.xml      editor-form.dtd       )
snap_validate_xml( editor/unsubscribe-page.xml          editor-form.dtd       )
snap_validate_xml( layout/info-settings-parser.xsl      simplified-xslt20.dtd )
snap_validate_xml( layout/plugin-settings-parser.xsl    simplified-xslt20.dtd )
snap_validate_xml( layout/unsubscribe-parser.xsl        simplified-xslt20.dtd )
snap_validate_xml( layout/unsubscribe-theme-parser.xsl  simplified-xslt20.dtd )
snap_validate_js ( plugin-selection/plugin-selection.js                       )
snap_validate_js ( sendmail/unsubscribe.js                                    )


add_library(info SHARED
    info.cpp
    plugin_selection.cpp
    unsubscribe.cpp
    tests.cpp
    ${SNAP_MANAGER_MOC_CXX}
    ${SNAP_MANAGER_RESOURCES_RCC}
    info.css
    content.xml
    editor/info-settings-page.xml
    editor/plugin-settings-page.xml
    editor/unsubscribe-page.xml
    layout/info-settings-parser.xsl
    layout/plugin-settings-parser.xsl
    layout/unsubscribe-parser.xsl
    layout/unsubscribe-theme-parser.xsl
    plugin-selection/plugin-selection.js
    sendmail/unsubscribe.js
    info-logo-64x64.png
)

target_link_libraries( info Qt5::Core )

install(
    TARGETS info LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR}
)
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/snapwebsites/plugins/info
    FILES_MATCHING PATTERN "*.h"
)

install(
    FILES
        plugin-selection/plugin-selection.js
    DESTINATION
        share/javascript/snapwebsites/info
)
install(
    FILES
        sendmail/unsubscribe.js
    DESTINATION
        share/javascript/snapwebsites/sendmail
)

# vim: ts=4 sw=4 et
