# Copyright (c) 2011-2019  Made to Order Software Corp.  All Rights Reserved
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

project(plugin_epayment_paypal)

# Get as2js for the JSON parser
find_package( As2Js 0.1.0 REQUIRED )
find_package( Qt5Core REQUIRED )

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${AS2JS_INCLUDE_DIRS}
)

qt5_add_resources(SNAP_MANAGER_RESOURCES_RCC
    epayment_paypal.qrc
)


## Validate content
##
snap_validate_xml( content.xml                         content.dtd           )
snap_validate_xml( epayment-paypal-settings-page.xml   editor-form.dtd       )
snap_validate_xml( epayment-paypal-settings-parser.xsl simplified-xslt20.dtd )
snap_validate_xml( epayment_paypal-tables.xml          tables.xsd            )
snap_validate_css( epayment-paypal.css                                       )
snap_validate_js ( epayment-paypal.js                                        )


add_library(epayment_paypal SHARED
    epayment_paypal.cpp
    ${SNAP_MANAGER_MOC_CXX}
    ${SNAP_MANAGER_RESOURCES_RCC}
    epayment-paypal.css
    epayment-paypal.js
    paypal-logo-64x64.png
    paypal.png
    paypal-medium.png
    paypal-large.png
    epayment-paypal-settings-page.xml
    epayment-paypal-settings-parser.xsl
)

target_link_libraries( epayment_paypal
    Qt5::Core
    ${AS2JS_LIBRARIES}
)

install(
    TARGETS epayment_paypal LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR}
)
install(
    FILES "epayment_paypal-tables.xml"
    DESTINATION ${SNAP_TABLE_DESCRIPTION_PATH}
)
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/snapwebsites/plugins/epayment_paypal
    FILES_MATCHING PATTERN "*.h"
)

install(
    FILES
        epayment-paypal.js
    DESTINATION
        share/javascript/snapwebsites/epayment_paypal
)

# vim: ts=4 sw=4 et
