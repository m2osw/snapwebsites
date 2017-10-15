#
# File:
#      plugins/sendmail/CMakeLists.txt
#
# Description:
#      Send emails to specified addresses using the system sendmail command
#      line. This plugin makes use of a backend process to actually process
#      the emails on a mail server. The front end saves the email in your
#      Cassandra database.
#
# Documentation:
#      See the CMake documentation.
#
# License:
#      Copyright (c) 2013-2017 Made to Order Software Corp.
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

project(plugin_sendmail)

find_package( Qt5Core REQUIRED )

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${LIBTLD_INCLUDE_DIRS}
)


qt5_add_resources(SNAP_MANAGER_RESOURCES_RCC
    sendmail.qrc
)

snap_validate_xml( content.xml                      ${snapwebsites_plugins_src_SOURCE_DIR}/content/content.dtd )
snap_validate_xml( layout/sendmail-content.xml      ${snapwebsites_plugins_src_SOURCE_DIR}/content/content.dtd )
snap_validate_xml( layout/sendmail-theme-parser.xsl simplified-xslt20.dtd                                      )
snap_validate_xml( layout/sendmail-body-parser.xsl  simplified-xslt20.dtd                                      )

add_library(sendmail SHARED
    sendmail.cpp
    tests.cpp
    ${SNAP_MANAGER_MOC_CXX}
    ${SNAP_MANAGER_RESOURCES_RCC}
    content.xml
    layout/sendmail-content.xml
    layout/sendmail-theme-parser.xsl
    layout/sendmail-body-parser.xsl
    sendmail-logo-64x64.png
)

target_link_libraries( sendmail Qt5::Core )

install(
    TARGETS sendmail LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR}
)
install(
    FILES "sendmail-tables.xml"
    DESTINATION ${SNAP_TABLE_DESCRIPTION_PATH}
)
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/snapwebsites/plugins/sendmail
    FILES_MATCHING PATTERN "*.h"
)

# vim: ts=4 sw=4 et
