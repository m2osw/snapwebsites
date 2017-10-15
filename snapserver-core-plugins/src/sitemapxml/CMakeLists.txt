#
# File:
#      plugins/sitemapxml/CMakeLists.txt
#
# Description:
#      Generate a sitemap.xml file which robots use to find all your website
#      pages. The plug-in may do a lot of the work using a backend process
#      so as to avoid wasting precious time on the frontend systems.
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

project(plugin_sitemapxml)

find_package( Qt5Core REQUIRED )

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
)

qt5_add_resources(SNAP_MANAGER_RESOURCES_RCC
    sitemapxml.qrc
)

snap_validate_xml( content.xml            ${snapwebsites_plugins_src_SOURCE_DIR}/content/content.dtd )
snap_validate_xml( sitemapxml-to-text.xsl simplified-xslt20.dtd                                      )
snap_validate_xml( sitemapxml-to-html.xsl simplified-xslt20.dtd                                      )

add_library(sitemapxml SHARED
    sitemapxml.cpp
    ${SNAP_MANAGER_MOC_CXX}
    ${SNAP_MANAGER_RESOURCES_RCC}
    content.xml
)

target_link_libraries( sitemapxml Qt5::Core )

install(
    TARGETS sitemapxml LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR}
)
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/snapwebsites/plugins/sitemapxml
    FILES_MATCHING PATTERN "*.h"
)

################################################################################
# Install XSD files.
#
set( DIST_FILES
		siteindex.xsd
		sitemap.xsd
)
#
install(
	FILES
        ${DIST_FILES}
	DESTINATION
		share/snapwebsites/xsd
)
#
file(
    INSTALL
        ${DIST_FILES}
    DESTINATION
        ${SNAP_DIST_DIR}/share/snapwebsites/xsd
)

# vim: ts=4 sw=4 et
