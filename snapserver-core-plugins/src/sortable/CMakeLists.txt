#
# File:
#      plugins/sortable/CMakeLists.txt
#
# Description:
#      Allow for a special widget that gives the end user a way to sort
#      items in a list with any number of levels (i.e. a list of lists
#      can also be viewed as a set of items that can be dragged and dropped.)
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

project(plugin_sortable)

find_package( Qt5Core REQUIRED )

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
)

qt5_add_resources(SNAP_MANAGER_RESOURCES_RCC
    sortable.qrc
)


snap_validate_xml( content.xml ${snapwebsites_plugins_src_SOURCE_DIR}/content/content.dtd )

# Sortable.js is a 3rd party library, we are not directly responsible for the
# non-closure compiler compatibility
#snap_validate_js( sortable.js )

snap_validate_css( sortable.css )
snap_validate_js( sortable-widgets.js )


add_library(sortable SHARED
    sortable.cpp
    sortable.h
    ${SNAP_MANAGER_MOC_CXX}
    ${SNAP_MANAGER_RESOURCES_RCC}
    content.xml
    sortable.css
    sortable.js
    sortable-widgets.js
    sortable_widgets/sortable-form.xsl
    sortable-logo-64x64.png
)

target_link_libraries( sortable Qt5::Core )

install(
    TARGETS sortable LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR}
)
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/snapwebsites/plugins/sortable
    FILES_MATCHING PATTERN "*.h"
)

# vim: ts=4 sw=4 et
