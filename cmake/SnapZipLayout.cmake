################################################################################
# Snap Websites Server -- zip up layout content for distribution
# Copyright (C) 2013-2017  Made to Order Software Corp.
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
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
################################################################################

if( NOT DEFINED SNAP_ZIP_LAYOUT )
set( SNAP_ZIP_LAYOUT TRUE )

find_program( ZIP zip REQUIRED )
if( ${ZIP} STREQUAL "ZIP-NOTFOUND" )
    message( FATAL_ERROR "zip archiver required!" )
endif()

################################################################################
# Zip up each of the layout files.
#
function( snap_zip_layout )
    set( options        )
    set( oneValueArgs   LAYOUT_NAME )
    set( multiValueArgs LAYOUT_FILES )
    cmake_parse_arguments( ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )
    #
    if( NOT ARG_LAYOUT_NAME )
        message( FATAL_ERROR "You must specify LAYOUT_NAME!" )
    endif()
    #
    if( NOT ARG_LAYOUT_FILES )
        message( FATAL_ERROR "You must provide LAYOUT_FILES to zip!" )
    endif()

    set( STAGING_DIR ${CMAKE_CURRENT_BINARY_DIR}/${ARG_LAYOUT_NAME} )

    unset( STAGING_COMMANDS )
    unset( ABSOLUTE_LAYOUT_FILES )
    foreach( file ${ARG_LAYOUT_FILES} )
        get_filename_component( fullfile ${file} ABSOLUTE )
        list( APPEND ABSOLUTE_LAYOUT_FILES ${fullfile} )
        list( APPEND STAGING_COMMANDS
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${fullfile} ${STAGING_DIR}
        )
    endforeach()

    set( ZIPFILE ${CMAKE_CURRENT_BINARY_DIR}/${ARG_LAYOUT_NAME}_layout.zip )
    add_custom_command( OUTPUT ${ZIPFILE}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${STAGING_DIR}
        ${STAGING_COMMANDS}
        COMMAND ${ZIP} ${ZIPFILE} -r ${ARG_LAYOUT_NAME}
        DEPENDS ${ABSOLUTE_LAYOUT_FILES}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )

    add_custom_target( ${ARG_LAYOUT_NAME}_zipfile ALL DEPENDS ${ZIPFILE} )

    install(
        FILES
            ${ZIPFILE}
        DESTINATION
            share/snapwebsites/layouts/${ARG_LAYOUT_NAME}
    )
endfunction()

endif()

# vim: ts=4 sw=4 et nocindent
