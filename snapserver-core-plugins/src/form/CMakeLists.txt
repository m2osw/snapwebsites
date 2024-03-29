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

project(plugin_form)

find_package( Qt5Core REQUIRED )

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
)

qt5_add_resources(SNAP_MANAGER_RESOURCES_RCC
    form.qrc
)

snap_validate_xml( content.xml    content.dtd           )
snap_validate_xml( core-form.xsl  simplified-xslt20.dtd )
snap_validate_js ( form.js                              )

# Unfortunate, but at this time this doesn't work too well...
# (i.e. xmllint [as in libxml2] does not support this complex XSD schema)
#snap_validate_xml( core-form.xsl schema-for-xslt20.xsd )

add_library(form SHARED
    form.cpp
    ${SNAP_MANAGER_MOC_CXX}
    ${SNAP_MANAGER_RESOURCES_RCC}
    content.xml
    core-form.xsl
    form.js
)

target_link_libraries( form Qt5::Core )

install(
    TARGETS
        form

    LIBRARY DESTINATION
        ${PLUGIN_INSTALL_DIR}
)

install(
    DIRECTORY
        ${CMAKE_CURRENT_SOURCE_DIR}/

    DESTINATION
        include/snapwebsites/plugins/form

    FILES_MATCHING PATTERN
        "*.h"
)

###############################################################################
## Install DTD file.
##
install(
    FILES
        snap-form.dtd

    DESTINATION
        share/snapwebsites/dtd
)

#
# WARNING: we use a configure_file(...) on top of an install(FILES ...)
#          so we can access the file in our SnapXmlLint.cmake without
#          any additional magic
#
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/snap-form.dtd
    ${SNAP_COMMON_DIR}/dtd/snap-form.dtd
    COPYONLY
)

install(
    FILES
        form.js

    DESTINATION
        share/javascript/snapwebsites/form
)

# vim: ts=4 sw=4 et
