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

project(oauth2)

find_package( Qt5Core REQUIRED )

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
)

qt5_add_resources(SNAP_MANAGER_RESOURCES_RCC
    oauth2.qrc
)


snap_validate_xml( content.xml                content.dtd           )
snap_validate_xml( oauth2-settings-page.xml   editor-form.dtd       )
snap_validate_xml( oauth2-settings-parser.xsl simplified-xslt20.dtd )


add_library(${PROJECT_NAME} SHARED
    oauth2.cpp
    ${SNAP_MANAGER_MOC_CXX}
    ${SNAP_MANAGER_RESOURCES_RCC}
    content.xml
    oauth2-settings-page.xml
    oauth2-settings-parser.xsl
    oauth2-logo-64x64.png
)

target_link_libraries( ${PROJECT_NAME} Qt5::Core )

install(
    TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR}
)
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/snapwebsites/plugins/oauth2
    FILES_MATCHING PATTERN "*.h"
)


#################################################################################
## Encode a username and password for basic auth tool.
##
project(snap_uuencode_basic_auth)

add_executable(${PROJECT_NAME}
    snap_uuencode_basic_auth.cpp
)

target_link_libraries(${PROJECT_NAME}
    ${QTCASSANDRA_LIBRARIES}
    ${QTSERIALIZATION_LIBRARIES}
    ${QT_LIBRARIES}
)

install(
    TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin
)

# vim: ts=4 sw=4 et
