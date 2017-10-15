#
# File:
#      plugins/epayment/CMakeLists.txt
#
# Description:
#      Offer support for various types of electronic and not so electronic
#      payments (credit cards, systems such as paypal, bit coins, etc.)
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

project(plugin_epayment)

find_package( Qt5Core REQUIRED )

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
)

qt5_add_resources(SNAP_MANAGER_RESOURCES_RCC
    epayment.qrc
)


snap_validate_css( epayment.css )

snap_validate_xml( content.xml                      content/content.dtd )

snap_validate_xml( epayment-settings-page.xml       editor/editor-form.dtd )

snap_validate_xml( epayment-settings-parser.xsl     simplified-xslt20.dtd )
snap_validate_xml( epayment-product-parser.xsl      simplified-xslt20.dtd )

snap_validate_js( epayment.js )


add_library(epayment SHARED
    epayment.cpp
    ${SNAP_MANAGER_MOC_CXX}
    ${SNAP_MANAGER_RESOURCES_RCC}
    content.xml
    epayment.css
    epayment.js
    epayment-settings-page.xml
    epayment-settings-parser.xsl
    epayment-product-parser.xsl
    epayment-logo-64x64.png
)

target_link_libraries( epayment
    Qt5::Core
)

install(
    TARGETS epayment LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR}
)
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/snapwebsites/plugins/epayment
    FILES_MATCHING PATTERN "*.h"
)

install(
    FILES
        epayment.js
    DESTINATION
        share/javascript/snapwebsites/epayment
)

# vim: ts=4 sw=4 et
