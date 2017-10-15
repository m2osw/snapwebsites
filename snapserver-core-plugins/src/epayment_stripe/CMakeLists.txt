#
# File:
#      plugins/epayment_stripe/CMakeLists.txt
#
# Description:
#      Offer support for Stripe payments. This is a credit card payment
#      facility.
#
# Documentation:
#      See the CMake documentation.
#
# License:
#      Copyright (c) 2016-2017 Made to Order Software Corp.
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

project(plugin_epayment_stripe)

# Get as2js for the JSON parser
find_package( As2Js 0.1.0 REQUIRED )
find_package( Qt5Core REQUIRED )

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${AS2JS_INCLUDE_DIRS}
)

qt5_add_resources(SNAP_MANAGER_RESOURCES_RCC
    epayment_stripe.qrc
)


snap_validate_css( epayment-stripe.css                 )
snap_validate_xml( content.xml                         ${snapwebsites_plugins_src_SOURCE_DIR}/content/content.dtd    )
snap_validate_xml( epayment-stripe-settings-page.xml   ${snapwebsites_plugins_src_SOURCE_DIR}/editor/editor-form.dtd )
snap_validate_xml( epayment-stripe-settings-parser.xsl simplified-xslt20.dtd                                         )
snap_validate_js ( epayment-stripe.js )


add_library(epayment_stripe SHARED
    epayment_stripe.cpp
    ${SNAP_MANAGER_MOC_CXX}
    ${SNAP_MANAGER_RESOURCES_RCC}
    epayment-stripe.css
    epayment-stripe.js
    stripe-logo-64x64.png
    stripe-large.png
    epayment-stripe-settings-page.xml
    epayment-stripe-settings-parser.xsl
)

target_link_libraries( epayment_stripe
    Qt5::Core
    ${AS2JS_LIBRARIES}
)

install(
    TARGETS epayment_stripe LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR}
)
install(
    FILES "epayment_stripe-tables.xml"
    DESTINATION ${SNAP_TABLE_DESCRIPTION_PATH}
)
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/snapwebsites/plugins/epayment_stripe
    FILES_MATCHING PATTERN "*.h"
)

install(
    FILES
        epayment-stripe.js
    DESTINATION
        share/javascript/snapwebsites/epayment_stripe
)

# vim: ts=4 sw=4 et
