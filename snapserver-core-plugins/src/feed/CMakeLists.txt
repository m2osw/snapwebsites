#
# File:
#      plugins/feed/CMakeLists.txt
#
# Description:
#      Generate a set of items (a list) that are to be sent as a feed
#      of a website. This feature uses the list capability to generate
#      the feed data and then a function that extracts the data as XML
#      data.
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

project(plugin_feed)

find_package( Qt5Core REQUIRED )

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
)

qt5_add_resources(SNAP_MANAGER_RESOURCES_RCC
    feed.qrc
)


snap_validate_xml( content.xml              ${snapwebsites_plugins_src_SOURCE_DIR}/content/content.dtd    )
snap_validate_xml( feed-settings-page.xml   ${snapwebsites_plugins_src_SOURCE_DIR}/editor/editor-form.dtd )
snap_validate_xml( feed-parser.xsl          simplified-xslt20.dtd                                         )
snap_validate_xml( feed-settings-parser.xsl simplified-xslt20.dtd                                         )
snap_validate_xml( atom.xsl                 atom-xslt20.dtd                                               )
snap_validate_xml( rss.xsl                  rss-xslt20.dtd                                                )


add_library(feed SHARED
    feed.cpp
    ${SNAP_MANAGER_MOC_CXX}
    ${SNAP_MANAGER_RESOURCES_RCC}
    content.xml
    atom.xsl
    atom-xslt20.dtd
    feed-parser.xsl
    feed-settings-page.xml
    feed-settings-parser.xsl
    rss.xsl
    rss-xslt20.dtd
    feed-logo-64x64.png
)

target_link_libraries( feed Qt5::Core )

install(
    TARGETS feed LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR}
)
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/snapwebsites/plugins/feed
    FILES_MATCHING PATTERN "*.h"
)

################################################################################
# Install DTD files.
#
set( DTD_FILES
		atom-xslt20.dtd
		rss-xslt20.dtd
)
install(
	FILES
        ${DTD_FILES}
	DESTINATION
		share/snapwebsites/dtd
)
#
file(
    INSTALL
        ${DTD_FILES}
    DESTINATION
        ${SNAP_DIST_DIR}/share/snapwebsites/dtd
)

# vim: ts=4 sw=4 et
