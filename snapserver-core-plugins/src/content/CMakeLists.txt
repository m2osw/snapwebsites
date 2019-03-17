#
# File:
#      snapserver-core-plugins/src/content/CMakeLists.txt
#
# Description:
#      Generate the content of a page. This includes many different type of
#      data such as a regular page or a tag part of a taxonomy.
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

project(plugin_content)

find_package( Qt5Core REQUIRED )

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
)

qt5_add_resources(SNAP_MANAGER_RESOURCES_RCC
    content.qrc
)

snap_validate_xml( content-tables.xml tables.xsd )


add_library(content SHARED
    content.cpp

    # various parts of the content plugin
    backend.cpp
    cache_control.cpp
    clone.cpp
    destroy.cpp
    expr_functions.cpp
    journal.cpp
    revision.cpp

    # sub-classes offered by the content plugin
    attachment_file.cpp
    field_search.cpp
    path_info.cpp
    permission_flag.cpp
    status.cpp

    # tests
    tests.cpp

    # xml and other content files
    content-tables.xml

    ${SNAP_MANAGER_MOC_CXX}
    ${SNAP_MANAGER_RESOURCES_RCC}
)

target_link_libraries(content
    Qt5::Core
    ${CSSPP_LIBRARIES}
)

install(
    TARGETS
        content

    LIBRARY DESTINATION
        ${PLUGIN_INSTALL_DIR}
)

install(
    FILES
        "content-tables.xml"

    DESTINATION
        ${SNAP_TABLE_DESCRIPTION_PATH}
)

install(
    DIRECTORY
        ${CMAKE_CURRENT_SOURCE_DIR}/

    DESTINATION
        include/snapwebsites/plugins/content

    FILES_MATCHING PATTERN
        "*.h"
)

###############################################################################
## Install DTD files.
##
install(
    FILES
        content.dtd

    DESTINATION
        share/snapwebsites/dtd
)


#
# WARNING: we use a configure_file(...) on top of an install(FILES ...)
#          so we can access the file in our SnapXmlLint.cmake without
#          any additional magic
#
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/content.dtd
    ${SNAP_COMMON_DIR}/dtd/content.dtd
    COPYONLY
)


# vim: ts=4 sw=4 et
