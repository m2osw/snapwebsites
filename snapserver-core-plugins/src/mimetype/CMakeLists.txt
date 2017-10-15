#
# File:
#      plugins/mimetype/CMakeLists.txt
#
# Description:
#      Generate the MIME type of a file. This plugin expects a set of
#      bytes from a file and checks the bytes to determine the MIME
#      type when known. For better transfer speed, since this has to
#      be sent to the client, we offer 3 levels: basics, normal, and
#      advanced. It is really rare that you'd ever even remotely
#      consider using advanced (it will be slow and probably won't
#      add much anything that's useful.)
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

project(plugin_mimetype)

find_package( Qt5Core REQUIRED )

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
)

qt5_add_resources(SNAP_MANAGER_RESOURCES_RCC
    mimetype.qrc
)

snap_validate_xml( content.xml ${snapwebsites_plugins_src_SOURCE_DIR}/content/content.dtd )


# Those are generated so we probably do not really need to validate
# them... also we have a dependency problem at this point
#snap_validate_js( mimetype-basics.js    )
#snap_validate_js( mimetype-standard.js  )
#snap_validate_js( mimetype-advanced.js  )


add_library(mimetype SHARED
    mimetype.cpp
    mimetype.h
    ${SNAP_MANAGER_MOC_CXX}
    ${SNAP_MANAGER_RESOURCES_RCC}
    content.xml
    mimetype-basics.js
)

target_link_libraries( mimetype
    Qt5::Core
)

install(
    TARGETS mimetype LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR}
)
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/snapwebsites/plugins/mimetype
    FILES_MATCHING PATTERN "*.h"
)




##
## Create the mimetype-basics.js file
##
# Ugly, but we have to create it in the source directory because rcc
# is not able to get files but in the current directory and sub-directories.
# Thus generated files HAVE to litter the source directory. (Although we
# could copy all the necessary files in the binary directory?)

function( AddMagicFiles TARGET_NAME MAGIC_FILES )
    project( MimeType_${TARGET_NAME} )

    string(TOLOWER "${TARGET_NAME}" TARGET_lname)

    add_custom_command(
        # Generate a .js from a set of magic files
        OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/mimetype-${TARGET_lname}.js
        COMMAND ${CMAKE_CURRENT_BINARY_DIR}/magic_to_js --name ${TARGET_NAME}
                ${MAGIC_FILES}
                1> mimetype-${TARGET_lname}.js
                || (rm mimetype-${TARGET_lname}.js && exit 1)
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/magic_to_js ${MAGIC_FILES}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Generating mimetype-${TARGET_lname}.js"
    )
endfunction(AddMagicFiles)


set( MIMETYPE_BASICS
    Magdir/pdf
    Magdir/snap-basics
)
AddMagicFiles( Basics "${MIMETYPE_BASICS}" )





# TODO: support more formats at different levels
#
#set( MIMETYPE_STANDARD
#    Magdir/...
#)
#AddMagicFiles( Standard ${MIMETYPE_STANDARD} )
#
#
#set( MIMETYPE_ADVANCED
#    Magdir/...
#)
#AddMagicFiles( Advanced ${MIMETYPE_ADVANCED} )
#
#


project(magic_to_js)

# Version used in the .js files
set(MIMETYPE_VERSION_MAJOR  0)
set(MIMETYPE_VERSION_MINOR  0)
set(MIMETYPE_VERSION_PATCH 12)

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/magic-to-js.h.in ${CMAKE_CURRENT_BINARY_DIR}/magic-to-js.h )

add_executable( ${PROJECT_NAME}
    magic-to-js.cpp
    magic-to-js.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/magic-to-js.h
)
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)
target_link_libraries( ${PROJECT_NAME}
    ${SNAPWEBSITES_LIBRARIES}
    Qt5::Core
)
# TODO: only install in the development version? (do we have such a component?)
install(
    TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin
)


install(
    FILES
        mimetype-basics.js
    DESTINATION
        share/javascript/snapwebsites/mimetype
)

# vim: ts=4 sw=4 et
