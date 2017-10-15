#
# File:
#      plugins/listener/CMakeLists.txt
#
# Description:
#      List to the client's polls. This plugin is capable of accepting
#      AJAX requests that include a set of URIs that are to be pushed
#      to other plugins that offer a listen capability. For example,
#      a user who drags and drops a PDF file can be given a preview
#      of the file. However, the preview is "calculated" by a backend
#      and as such takes "a while" (many seconds). Because of that we
#      like to not have a connection that last that long (it could
#      prevents other connections from being added to the service as
#      that would use up an Apache child process for a long time.)
#      So instead the client can check about once a second whether
#      the item is available. The different processes can also give
#      a long period of time for the test (i.e. if we know that an
#      item will take at least 10 seconds, then we do not need to
#      test 10 times before it has a chance to be there.)
#
# Documentation:
#      See the CMake documentation.
#
# License:
#      Copyright (c) 2014-2017  Made to Order Software Corp.
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

project(plugin_listener)

find_package( Qt5Core REQUIRED )

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
)

qt5_add_resources(SNAP_MANAGER_RESOURCES_RCC
    listener.qrc
)


snap_validate_xml( content.xml ${snapwebsites_plugins_src_SOURCE_DIR}/content/content.dtd )
snap_validate_js ( listener.js )


add_library(listener SHARED
    listener.cpp
    listener.h
    ${SNAP_MANAGER_MOC_CXX}
    ${SNAP_MANAGER_RESOURCES_RCC}
    content.xml
    listener.js
    listener-logo-64x64.png
)

target_link_libraries( listener Qt5::Core )

install(
    TARGETS listener LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR}
)
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/snapwebsites/plugins/listener
    FILES_MATCHING PATTERN "*.h"
)

install(
    FILES
        listener.js
    DESTINATION
        share/javascript/snapwebsites/listener
)

# vim: ts=4 sw=4 et
