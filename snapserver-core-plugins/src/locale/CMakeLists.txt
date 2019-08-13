#
# File:
#      snapserver-core-plugins/src/locale/CMakeLists.txt
#
# Description:
#      Offer support for various locale function used to format strings
#      depending on a locale selection. The locale also handles the
#      timezone that a website owner and a user may select.
#
# Documentation:
#      See the CMake documentation.
#
# License:
#      Copyright (c) 2011-2019  Made to Order Software Corp.  All Rights Reserved
#
#      https://snapwebsites.org/
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

project(plugin_locale)

# The ICU Find function changed and was added to the OS
string(REGEX MATCH "Linux-4.4" OS_NAME_AND_VERSION ${CMAKE_SYSTEM})
if("${OS_NAME_AND_VERSION}" STREQUAL "Linux-4.4")
    find_package( ICU REQUIRED )
else()
    find_package( ICU REQUIRED COMPONENTS uc i18n )
endif()

find_package( Qt5Core REQUIRED )

include_directories(
    SYSTEM
        ${ICU_INCLUDE_DIRS}
)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
)

qt5_add_resources(SNAP_MANAGER_RESOURCES_RCC
    locale.qrc
)


# We use the name snap_locale.h for the header because there is a locale.h
# file in the OS under Linux

add_library(locale SHARED
    locale.cpp
    snap_locale.h
    ${SNAP_MANAGER_MOC_CXX}
    ${SNAP_MANAGER_RESOURCES_RCC}
)

target_link_libraries( locale
    Qt5::Core
    ${ICU_LIBRARIES}
    ${ICU_I18N_LIBRARIES}
)

install(
    TARGETS
        locale

    LIBRARY DESTINATION
        ${PLUGIN_INSTALL_DIR}
)

install(
    DIRECTORY
        ${CMAKE_CURRENT_SOURCE_DIR}/

    DESTINATION
        include/snapwebsites/plugins/locale

    FILES_MATCHING
        PATTERN "*.h"
)

# vim: ts=4 sw=4 et
