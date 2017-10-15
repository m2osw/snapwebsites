#
# File:
#      plugins/locale_widgets/CMakeLists.txt
#
# Description:
#      Offer support for various locale widgets taht extend the editor
#      capabilities with widgets that respect the current website or
#      user localization.
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

project(plugin_locale_widgets)

find_package( Qt5Core REQUIRED )

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
)

qt5_add_resources(SNAP_MANAGER_RESOURCES_RCC
    locale_widgets.qrc
)


## Validate content
##
snap_validate_css( locale-timezone.css )
snap_validate_xml( content.xml         ${snapwebsites_plugins_src_SOURCE_DIR}/content/content.dtd )
snap_validate_xml( locale-form.xsl     simplified-xslt20.dtd                                      )


add_library( locale_widgets SHARED
    locale_widgets.cpp
    locale_widgets.h
    ${SNAP_MANAGER_MOC_CXX}
    ${SNAP_MANAGER_RESOURCES_RCC}
    content.xml
    locale-form.xsl
    locale-timezone.js
    locale-timezone.css
)

target_link_libraries( locale_widgets
    Qt5::Core
)

install(
    TARGETS locale_widgets
    LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR}
)
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/snapwebsites/plugins/locale_widgets
    FILES_MATCHING PATTERN "*.h"
)

install(
    FILES
        locale-timezone.js
    DESTINATION
        share/javascript/snapwebsites/locale_widgets
)

# vim: ts=4 sw=4 et
