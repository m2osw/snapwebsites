# Copyright (c) 2015-2019  Made to Order Software Corp.  All Rights Reserved
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

project(plugin_avatar)

find_package( Qt5Core REQUIRED )

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
)

qt5_add_resources(SNAP_MANAGER_RESOURCES_RCC
    avatar.qrc
)


snap_validate_xml( content.xml                content.dtd           )
snap_validate_xml( avatar-settings-page.xml   editor-form.dtd       )
snap_validate_xml( avatar-settings-parser.xsl simplified-xslt20.dtd )


add_library(avatar SHARED
    avatar.cpp
    ${SNAP_MANAGER_MOC_CXX}
    ${SNAP_MANAGER_RESOURCES_RCC}
    content.xml
    avatar-settings-page.xml
    avatar-settings-parser.xsl
    avatar-logo-64x64.png
)

target_link_libraries( avatar Qt5::Core )

install(
    TARGETS avatar LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR}
)
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/snapwebsites/plugins/avatar
    FILES_MATCHING PATTERN "*.h"
)

# vim: ts=4 sw=4 et
