#
# File:
#      plugins/versions/CMakeLists.txt
#
# Description:
#      Generate the "versions" page. This plugin lists the version of the
#      various elements used by Snap Websites.
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

project(plugin_versions)

find_package( Qt5Core REQUIRED )

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
)

qt5_add_resources(SNAP_MANAGER_RESOURCES_RCC
    versions.qrc
)

snap_validate_xml( content.xml ${snapwebsites_plugins_src_SOURCE_DIR}/content/content.dtd )


# TODO: Find a way to get the GIT branch and pass it to the compiler...
#       This won't work because (1) we are not always going to have
#       git (i.e. pbuilder environment); and (2) this command gets
#       executed in the destination directory which generally is not
#       where the git repository is.
#
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGIT_BRANCH=\"s`git rev-parse --abbrev-ref HEAD`e\"")

add_library(versions SHARED
    versions.cpp
    versions.h
    ${SNAP_MANAGER_MOC_CXX}
    ${SNAP_MANAGER_RESOURCES_RCC}
    content.xml
    versions-logo-64x64.png
)

target_link_libraries( versions Qt5::Core )

install(
    TARGETS versions LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR}
)
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/snapwebsites/plugins/versions
    FILES_MATCHING PATTERN "*.h"
)

# vim: ts=4 sw=4 et
