#
# File:
#      plugins/output/CMakeLists.txt
#
# Description:
#      Generate the output of a page. This plugin takes what the content
#      plugin manages and transforms it in HTML or some other format.
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

project(plugin_output)

find_package( Qt5Core REQUIRED )

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
)

qt5_add_resources(SNAP_MANAGER_RESOURCES_RCC
    output.qrc
)

snap_validate_xml( content.xml                     ${snapwebsites_plugins_src_SOURCE_DIR}/content/content.dtd )
snap_validate_xml( layout/notheme-body-parser.xsl  simplified-xslt20.dtd                                      )
snap_validate_xml( layout/notheme-theme-parser.xsl simplified-xslt20.dtd                                      )


# jQuery does not need to be validated
#snap_validate_js( jquery.js                    )
# Externs are used by the compiler and should be auto generated at some point
#snap_validate_js( externs/jquery-extensions.js )
#
snap_validate_css( output.css               )
snap_validate_css( layout/notheme-style.css )
snap_validate_js(  jquery-extensions.js     )
snap_validate_js(  output.js                )
snap_validate_js(  popup.js                 )


add_library(output SHARED
    output.cpp
    output.h
    ${SNAP_MANAGER_MOC_CXX}
    ${SNAP_MANAGER_RESOURCES_RCC}
    content.xml
    output.css
    close-button.png
    plugin-icon-64x64.png
    progress.gif
    content-logo-64x64.png
    snap-logo-64x64.png
    layout-logo-64x64.png
    links-logo-64x64.png
    messages-logo-64x64.png
    wait.gif
    wait-rotate.png
    wait-rotate-64x64.png
    jquery.js
    jquery-extensions.js
    output.js
    popup.js
    rotate-animation.js
    layout/notheme-body-parser.xsl
    layout/notheme-content.xml
    layout/notheme-theme-parser.xsl
    layout/notheme-style.css
)

target_link_libraries( output Qt5::Core )

install(
    TARGETS output LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR}
)
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/snapwebsites/plugins/output
    FILES_MATCHING PATTERN "*.h"
)

install(
    FILES
        jquery-extensions.js
        output.js
        popup.js
        jquery-rotate.js
        rotate-animation.js
        jquery.js
    DESTINATION
        share/javascript/snapwebsites/output
)
install(
    FILES
        externs/jquery-extensions.js
        externs/jquery-rotate.js
    DESTINATION
        share/javascript/snapwebsites/externs
)

# vim: ts=4 sw=4 et
