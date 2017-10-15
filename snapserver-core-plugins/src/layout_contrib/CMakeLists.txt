#
# File:
#      plugins/layout_contrib/CMakeLists.txt
#
# Description:
#      Various CSS, JS, Fonts, etc. to help build modern layouts.
#
# Documentation:
#      See the CMake documentation.
#
# License:
#      Copyright (c) 2017  Made to Order Software Corp.
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

project(plugin_layout_contrib)

find_package( Qt5Core REQUIRED )

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
)

qt5_add_resources(SNAP_MANAGER_RESOURCES_RCC
    layout_contrib.qrc
)


## Validate content
##
snap_validate_xml( content.xml ${snapwebsites_plugins_src_SOURCE_DIR}/content/content.dtd )

snap_validate_css( layout_contrib/agency.css           )
snap_validate_css( layout_contrib/bootstrap.css        )
snap_validate_css( layout_contrib/bootstrap-social.css )
snap_validate_css( layout_contrib/bootstrap-theme.css  )
snap_validate_css( layout_contrib/font-awesome.css     )
snap_validate_css( layout_contrib/metis-menu.css       )
snap_validate_css( layout_contrib/sb-admin-2.css       )

snap_validate_js( layout_contrib/agency.js     )
#snap_validate_js( layout_contrib/bootstrap.js  ) -- apparently the author does not believe in having ';' at the end of lines
snap_validate_js( layout_contrib/metis-menu.js )
snap_validate_js( layout_contrib/sb-admin-2.js )



add_library(layout_contrib SHARED
    layout_contrib.cpp
    ${SNAP_MANAGER_MOC_CXX}
    ${SNAP_MANAGER_RESOURCES_RCC}
    content.xml
    layout_contrib/agency.css
    layout_contrib/bootstrap.css
    layout_contrib/bootstrap-social.css
    layout_contrib/bootstrap-theme.css
    layout_contrib/font-awesome.css
    layout_contrib/metis-menu.css
    layout_contrib/agency.js
    layout_contrib/bootstrap.js
    layout_contrib/metis-menu.js
    layout_contrib/FontAwesome.otf
    layout_contrib/fontawesome-webfont.eot
    layout_contrib/fontawesome-webfont.svg
    layout_contrib/fontawesome-webfont.ttf
    layout_contrib/fontawesome-webfont.woff
    layout_contrib/fontawesome-webfont.woff2
    layout_contrib/glyphicons-halflings-regular.eot
    layout_contrib/glyphicons-halflings-regular.svg
    layout_contrib/glyphicons-halflings-regular.ttf
    layout_contrib/glyphicons-halflings-regular.woff
    layout_contrib/glyphicons-halflings-regular.woff2
    layout_contrib/header-background.jpg
    layout_contrib/world-map.png
)

target_link_libraries( layout_contrib Qt5::Core )

install(
    TARGETS
        layout_contrib
    LIBRARY DESTINATION
        ${PLUGIN_INSTALL_DIR}
)
install(
    DIRECTORY
        ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION
        include/snapwebsites/plugins/layout_contrib
    FILES_MATCHING PATTERN
        "*.h"
)


# vim: ts=4 sw=4 et wrap
