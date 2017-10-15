#
# File:
#      plugins/epayment_creditcard/CMakeLists.txt
#
# Description:
#      Add one form to request the end user to enter their credit card
#      information. This plugin is used by any plugin capable of processing
#      a credit card.
#
# Documentation:
#      See the CMake documentation.
#
# License:
#      Copyright (c) 2014-2017 Made to Order Software Corp.
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

project(plugin_epayment_creditcard)

find_package( Qt5Core REQUIRED )

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
)

qt5_add_resources(SNAP_MANAGER_RESOURCES_RCC
    epayment_creditcard.qrc
)


snap_validate_xml( content.xml                                     ${snapwebsites_plugins_src_SOURCE_DIR}/content/content.dtd    )
snap_validate_xml( editor/epayment-credit-card-settings-page.xml   ${snapwebsites_plugins_src_SOURCE_DIR}/editor/editor-form.dtd )
snap_validate_xml( editor/epayment-credit-card-page.xml            ${snapwebsites_plugins_src_SOURCE_DIR}/editor/editor-form.dtd )
snap_validate_xml( layout/epayment-credit-card-settings-parser.xsl simplified-xslt20.dtd                                         )
snap_validate_xml( layout/epayment-credit-card-parser.xsl          simplified-xslt20.dtd                                         )
snap_validate_js ( epayment-credit-card.js      )
snap_validate_js ( epayment-credit-card-form.js )


add_library(epayment_creditcard SHARED
    epayment_creditcard.cpp
    epayment_creditcard.h
    epayment_creditcard_info.cpp
    epayment_gateway_features.cpp
    ${SNAP_MANAGER_MOC_CXX}
    ${SNAP_MANAGER_RESOURCES_RCC}
    content.xml
    epayment-credit-card.js
    epayment-credit-card-form.js
    epayment-credit-card-logo-64x64.png
    editor/epayment-credit-card-settings-page.xml
    editor/epayment-credit-card-page.xml
    layout/epayment-credit-card-settings-parser.xsl
    layout/epayment-credit-card-parser.xsl
)

target_link_libraries( epayment_creditcard Qt5::Core )

install(
    TARGETS epayment_creditcard LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR}
)
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/snapwebsites/plugins/epayment_creditcard
    FILES_MATCHING PATTERN "*.h"
)

#install(
#    FILES
#        epayment-creditcard.js
#    DESTINATION
#        share/javascript/snapwebsites/epayment_creditcard
#)

# vim: ts=4 sw=4 et
