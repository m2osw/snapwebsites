# Copyright (c) 2012-2019  Made to Order Software Corp.  All Rights Reserved
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

AtomicNames("names.an")

project(plugin_links)

find_package(Qt5Core REQUIRED)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
)

qt5_add_resources(SNAP_MANAGER_RESOURCES_RCC
    links.qrc
)

snap_validate_xml(links-tables.xml tables.xsd)

add_library(links SHARED
    links.cpp
    backend.cpp
    tests.cpp
    ${SNAP_MANAGER_MOC_CXX}
    ${SNAP_MANAGER_RESOURCES_RCC}
)

target_link_libraries(links Qt5::Core)

install(
    TARGETS
        links

    LIBRARY DESTINATION
        ${PLUGIN_INSTALL_DIR}
)

install(
    PROGRAMS
        snapcleanuplinks

    DESTINATION
        bin
)

install(
    FILES
        "links-tables.xml"

    DESTINATION
        ${SNAP_TABLE_DESCRIPTION_PATH}
)

install(
    DIRECTORY
        ${CMAKE_CURRENT_SOURCE_DIR}/

    DESTINATION
        include/snapwebsites/plugins/links

    FILES_MATCHING PATTERN
        "*.h"
)

# vim: ts=4 sw=4 et
