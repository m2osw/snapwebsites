# Copyright (c) 2013-2019  Made to Order Software Corp.  All Rights Reserved
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

project(plugin_editor)

find_package( Qt5Core REQUIRED )

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${LIBTLD_INCLUDE_DIRS}
)


qt5_add_resources(SNAP_MANAGER_RESOURCES_RCC
    editor.qrc
)

snap_validate_css( editor.css )
# Form dependency to be removed.
snap_validate_xml( new-draft.xml          snap-form.dtd          )
snap_validate_xml( content.xml            content.dtd            )
snap_validate_xml( default-page.xml       editor-form.dtd        )
snap_validate_xml( editor-test-page.xml   editor-form.dtd        )
snap_validate_xml( editor-test-parser.xsl simplified-xslt20.dtd  )
snap_validate_xml( editor-form.xsl        simplified-xslt20.dtd  )

# Unfortunate, but at this time this doesn't work too well...
# (i.e. xmllint [as in libxml2] does not support this complex XSD schema)
#snap_validate_xml( editor-form.xsl schema-for-xslt20.xsd )

snap_validate_js( editor.js )


add_library(editor SHARED
    editor.cpp

    create.cpp
    save_info.cpp

    ${SNAP_MANAGER_MOC_CXX}
    ${SNAP_MANAGER_RESOURCES_RCC}

    buttons.png
    checkmark-15x11.png
    dropdown-arrow-15x15.png
    editor.js
    editor.css
    editor-logo-64x64.png
    content.xml
    editor-form.xsl
    editor-test-page.xml
    editor-test-parser.xsl
    new-draft.xml
    radio-selected.png
    radio-unselected.png
)

target_link_libraries(editor
    Qt5::Core
)

install(
    TARGETS
        editor

    LIBRARY DESTINATION
        ${PLUGIN_INSTALL_DIR}
)

file(GLOB HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

install(
    FILES
        ${HEADER_FILES}

    DESTINATION
        include/snapwebsites/plugins/editor
)

###############################################################################
## Install DTD file.
##
install(
    FILES
        editor-form.dtd

    DESTINATION
        share/snapwebsites/dtd
)

#
# WARNING: we use a configure_file(...) on top of an install(FILES ...)
#          so we can access the file in our SnapXmlLint.cmake without
#          any additional magic
#
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/editor-form.dtd
    ${SNAP_COMMON_DIR}/dtd/editor-form.dtd
    COPYONLY
)

install(
    FILES
        editor.js

    DESTINATION
        share/javascript/snapwebsites/editor
)

# vim: ts=4 sw=4 et
