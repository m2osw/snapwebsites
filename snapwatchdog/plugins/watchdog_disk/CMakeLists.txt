#
# File:
#      snapwatchdog/plugins/watchdog_disk/CMakeLists.txt
#
# Description:
#      Disk watchdog: report disk usage over time.
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

project(watchdog_disk)

find_package( Qt5Core REQUIRED )
find_package( Qt5Xml  REQUIRED )

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${snapwatchdog_SOURCE_DIR}
)


qt5_add_resources(SNAP_MANAGER_RESOURCES_RCC
    disk.qrc
)


add_library(${PROJECT_NAME} SHARED
    disk.cpp
    ${SNAP_MANAGER_MOC_CXX}
    ${SNAP_MANAGER_RESOURCES_RCC}
)

target_link_libraries(${PROJECT_NAME}
    Qt5::Core
    Qt5::Xml
)

install(
    TARGETS
        ${PROJECT_NAME}
    LIBRARY DESTINATION
        ${PLUGIN_INSTALL_DIR}
)
file(GLOB HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
install(
    FILES
        ${HEADER_FILES}
    DESTINATION
        include/snapwatchdog/plugins/disk
)

# vim: ts=4 sw=4 et
