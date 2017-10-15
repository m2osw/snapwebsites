#
# File:
#      plugins/robotstxt/CMakeLists.txt
#
# Description:
#      Generate a robots.txt file so robots know what they can and cannot
#      access on your website.
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

project(plugin_robotstxt)

find_package( Qt5Core REQUIRED )

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# This was an attempt to pre-process the .xml file to remove comments
# but at this point it doesn't work right (i.e. the target cannot always
# be found / regenerated).
#add_custom_command(OUTPUT content.xml
#    # The output should be ${MAKE_CURRENT_BINARY_DIR} but unfortunately rcc
#    # cannot be tweak to read files from two different folders (TBD)
#    COMMAND echo \"<?xml version=\\"1.0\\"?>\" >${CMAKE_CURRENT_SOURCE_DIR}/content.xml
#    COMMAND xmlpatterns ${CMAKE_CURRENT_SOURCE_DIR}/../../lib/plugins.xsl ${CMAKE_CURRENT_SOURCE_DIR}/robotstxt.xml >>${CMAKE_CURRENT_SOURCE_DIR}/content.xml
#    MAIN_DEPENDENCY robotstxt.xml
#)
#
#set_directory_properties(PROPERTIES
#    ADDITIONAL_MAKE_CLEAN_FILES ${CMAKE_CURRENT_SOURCE_DIR}/content.xml
#)

qt5_add_resources(SNAP_MANAGER_RESOURCES_RCC
    robotstxt.qrc
)

snap_validate_xml( content.xml ${snapwebsites_plugins_src_SOURCE_DIR}/content/content.dtd )

add_library(robotstxt SHARED
    robotstxt.cpp
    ${SNAP_MANAGER_MOC_CXX}
    ${SNAP_MANAGER_RESOURCES_RCC}
    content.xml
    robotstxt-logo-64x64.png
)

target_link_libraries( robotstxt Qt5::Core )

install(
    TARGETS robotstxt LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR}
)
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/snapwebsites/plugins/robotstxt
    FILES_MATCHING PATTERN "*.h"
)

# vim: ts=4 sw=4 et
