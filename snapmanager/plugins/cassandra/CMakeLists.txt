#
# File:
#      snapwebsites/snapmanagercgi/plugins/cassandra/CMakeLists.txt
#
# Description:
#      Manage parameters in the default cassandra.yaml from
#      snapamanger.cgi.
#
# Documentation:
#      See the CMake documentation.
#
# License:
#      Copyright (c) 2016-2017 Made to Order Software Corp.
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

project(plugin_cassandra)

find_package( Qt5Core  REQUIRED )
find_package( Qt5Xml   REQUIRED )
find_package( YAML-CPP REQUIRED )

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${YAML_CPP_INCLUDE_DIR}
)

qt5_add_resources(SNAP_MANAGER_RESOURCES_RCC
    cassandra.qrc
)



add_library(cassandra SHARED
    cassandra.cpp
    ${SNAP_MANAGER_MOC_CXX}
    ${SNAP_MANAGER_RESOURCES_RCC}
)

target_link_libraries(cassandra
    snapmanager
    ${QT_LIBRARIES}
    ${SNAPWEBSITES_LIBRARIES}
    ${YAML_CPP_LIBRARIES}
)

install(
    TARGETS cassandra LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR}
)
file(GLOB HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
install(
    FILES ${HEADER_FILES}
    DESTINATION include/snapmanager/plugins/cassandra
)

# vim: ts=4 sw=4 et
