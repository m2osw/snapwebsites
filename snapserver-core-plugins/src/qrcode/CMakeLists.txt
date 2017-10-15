#
# File:
#      plugins/qrcode/CMakeLists.txt
#
# Description:
#      Generate the QR code of pages. Note that the plugin makes use of
#      the shorturl when a page has a very long URI.
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

project(plugin_qrcode)

find_package(ImageMagick
    COMPONENTS Magick++
)
if( NOT ImageMagick_FOUND )
    message( FATAL_ERROR "ImageMagic is required to build the image plugin" )
endif()

find_package( QREncode QUIET REQUIRED )
find_package( Qt5Core REQUIRED )

include_directories(
    SYSTEM
        ${LIBQRENCODE_INCLUDES}
        ${ImageMagick_INCLUDE_DIRS}
)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
)

qt5_add_resources(SNAP_MANAGER_RESOURCES_RCC
    qrcode.qrc
)

snap_validate_xml( content.xml                ${snapwebsites_plugins_src_SOURCE_DIR}/content/content.dtd    )
snap_validate_xml( qrcode-settings-page.xml   ${snapwebsites_plugins_src_SOURCE_DIR}/editor/editor-form.dtd )
snap_validate_xml( qrcode-settings-parser.xsl simplified-xslt20.dtd                                         )

add_library(qrcode SHARED
    qrcode.cpp
    ${SNAP_MANAGER_MOC_CXX}
    ${SNAP_MANAGER_RESOURCES_RCC}
    content.xml
    qrcode-logo-64x64.png
    qrcode-not-available.png
    qrcode-settings-page.xml
    qrcode-settings-parser.xsl
)

target_link_libraries( qrcode
    Qt5::Core
    ${LIBQRENCODE_LIBRARIES}
    ${ImageMagick_LIBRARIES}
)

install(
    TARGETS qrcode LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR}
)
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/snapwebsites/plugins/qrcode
    FILES_MATCHING PATTERN "*.h"
)

# vim: ts=4 sw=4 et
