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

project(plugin_cookie_consent_silktide)

find_package( Qt5Core REQUIRED )

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
)

qt5_add_resources(SNAP_MANAGER_RESOURCES_RCC
    cookie_consent_silktide.qrc
)

snap_validate_xml( content.xml                        content.dtd           )
snap_validate_xml( cookie-consent-silktide-page.xml   editor-form.dtd       )
snap_validate_xml( cookie-consent-silktide-parser.xsl simplified-xslt20.dtd )

# One day...
#snap_validate_js( cookie-consent-silktide.js )


add_library(cookie_consent_silktide SHARED
    cookie_consent_silktide.cpp
    cookie_consent_silktide.h
    ${SNAP_MANAGER_MOC_CXX}
    ${SNAP_MANAGER_RESOURCES_RCC}
    content.xml
    cookie-consent-silktide.js
    cookie-consent-silktide-page.xml
    cookie-consent-silktide-parser.xsl
    cookie-consent-silktide-logo-64x64.png
    dark-top.png
    dark-floating.png
    dark-bottom.png
    light-top.png
    light-floating.png
    light-bottom.png
    )

target_link_libraries( cookie_consent_silktide
    Qt5::Core
)

install(
    TARGETS cookie_consent_silktide LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR}
)
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/snapwebsites/plugins/cookie_consent_silktide
    FILES_MATCHING PATTERN "*.h"
)

# vim: ts=4 sw=4 et
