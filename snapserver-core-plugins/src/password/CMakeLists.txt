#
# File:
#      plugins/password/CMakeLists.txt
#
# Description:
#      Handle various checks on passwords when registering users.
#      It may also be used when changing passwords.
#
# Documentation:
#      See the CMake documentation.
#
# License:
#      Copyright (c) 2011-2017 Made to Order Software Corp.
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

project(plugin_password)

find_package( Qt5Core REQUIRED )

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
)

qt5_add_resources(SNAP_MANAGER_RESOURCES_RCC
    password.qrc
)


## Validate content
##
snap_validate_css( password.css                         )
snap_validate_xml( content.xml                          ${snapwebsites_plugins_src_SOURCE_DIR}/content/content.dtd    )
snap_validate_xml( editor/password-blacklist-page.xml   ${snapwebsites_plugins_src_SOURCE_DIR}/editor/editor-form.dtd )
snap_validate_xml( editor/password-policy-page.xml      ${snapwebsites_plugins_src_SOURCE_DIR}/editor/editor-form.dtd )
snap_validate_xml( layout/password-blacklist-parser.xsl simplified-xslt20.dtd                                         )
snap_validate_xml( layout/password-policy-parser.xsl    simplified-xslt20.dtd                                         )
snap_validate_xml( password_widgets/password-form.xsl   simplified-xslt20.dtd                                         )
snap_validate_js ( password.js                          )


add_library(password SHARED
    password.cpp
    password.h
    blacklist.cpp
    policy.cpp
    ${SNAP_MANAGER_MOC_CXX}
    ${SNAP_MANAGER_RESOURCES_RCC}
    content.xml
    editor/password-blacklist-page.xml
    editor/password-policy-page.xml
    layout/password-blacklist-parser.xsl
    layout/password-policy-parser.xsl
    password.css
    password.js
    password_widgets/password-form.xsl
    password-logo-64x64.png
)

target_link_libraries( password Qt5::Core )

install(
    TARGETS password LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR}
)
install(
    FILES "password-tables.xml"
    DESTINATION ${SNAP_TABLE_DESCRIPTION_PATH}
)
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/snapwebsites/plugins/password
    FILES_MATCHING PATTERN "*.h"
)

# vim: ts=4 sw=4 et
