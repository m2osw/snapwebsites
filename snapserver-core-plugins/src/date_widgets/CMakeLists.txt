#
# File:
#      plugins/date_widgets/CMakeLists.txt
#
# Description:
#      Date widget extensions to the Snap! editor.
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

project(plugin_date_widgets)

find_package( Qt5Core REQUIRED )

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${LIBTLD_INCLUDE_DIRS}
)


qt5_add_resources(SNAP_MANAGER_RESOURCES_RCC
    date_widgets.qrc
)

## Validate content
##
snap_validate_css( date_widgets/date-widgets.css )
snap_validate_xml( content.xml                  ${snapwebsites_plugins_src_SOURCE_DIR}/content/content.dtd    )
snap_validate_xml( date-widgets-test-page.xml   ${snapwebsites_plugins_src_SOURCE_DIR}/editor/editor-form.dtd )
snap_validate_xml( date_widgets/date-form.xsl   simplified-xslt20.dtd                                         )
snap_validate_xml( date-widgets-test-parser.xsl simplified-xslt20.dtd                                         )
snap_validate_js(  date_widgets/date-widgets.js  )


add_library(date_widgets SHARED
    date_widgets.cpp
    ${SNAP_MANAGER_MOC_CXX}
    ${SNAP_MANAGER_RESOURCES_RCC}
    content.xml
    date_widgets/date-widgets.js
    date_widgets/date-widgets.css
    date_widgets/date-widgets-logo-64x64.png
    date_widgets/date-form.xsl
    date-widgets-test-page.xml
    date-widgets-test-parser.xsl
)

target_link_libraries( date_widgets Qt5::Core )

install(
    TARGETS date_widgets LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR}
)
file(GLOB HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
install(
    FILES ${HEADER_FILES}
    DESTINATION include/snapwebsites/plugins/date_widgets
)

install(
    FILES
        date_widgets/date-widgets.js
    DESTINATION
        share/javascript/snapwebsites/date_widgets
)

# vim: ts=4 sw=4 et
