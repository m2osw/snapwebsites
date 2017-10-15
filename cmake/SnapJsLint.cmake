################################################################################
# Snap Websites Server -- check javascript content for lint issues.
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

if( NOT DEFINED SNAP_JS_LINT )
set( SNAP_JS_LINT TRUE )

################################################################################
# Handle linting the javascript files...
#
find_program( BASH   bash    /bin     )
find_program( JSLINT gjslint /usr/bin )
#
# Disable:
#   0002 -- Missing space before "("
#   0110 -- Line too long (over 80 characters)
#   0120 -- Binary operator should go on previous line "&&" 
#   0131 -- Single-quoted string preferred over double-quoted string.
#
set( OPTIONS "--disable 0002,0110,0120,0131 --jslint_error=blank_lines_at_top_level --jslint_error=unused_private_members" )

set( js_lint_script ${CMAKE_BINARY_DIR}/do_js_lint.sh CACHE INTERNAL "JS lint script" FORCE )
file( WRITE  ${js_lint_script} "#!${BASH}\n"                                                         )
file( APPEND ${js_lint_script} "${JSLINT} ${OPTIONS} $1 > $2 && exit 0 || (cat $2; rm $2; exit 1)\n" )

function( snap_validate_js JS_FILE )
    get_filename_component( FULL_JS_PATH ${JS_FILE} ABSOLUTE )
    set_property( GLOBAL APPEND PROPERTY JS_FILE_LIST "${FULL_JS_PATH}" "${CMAKE_CURRENT_BINARY_DIR}" )
endfunction()

function( snap_clear_js_targets )
    set_property( GLOBAL PROPERTY JS_FILE_LIST "" )
endfunction()


################################################################################
# Make sure all lint targets get built properly...
# Call snap_build_js_targets() after calling snap_validate_js() above.
#
macro( snap_build_js_targets )
    set( arg_count 2 )
    unset( lint_file_list )
    get_property( JS_FILE_LIST GLOBAL PROPERTY JS_FILE_LIST )
    list( LENGTH JS_FILE_LIST range )
    math( EXPR mod_test "${range} % ${arg_count}" )
    if( NOT ${mod_test} EQUAL 0 )
        message( FATAL_ERROR "The list of files must have an even count. Each JS file must have an accompanying binary path file!" )
    endif()
    #
    # Create a lint file for each pair
    #
    math( EXPR whole_range "${range} - 1" )
    foreach( var_idx RANGE 0 ${whole_range} ${arg_count} )
        list( GET JS_FILE_LIST ${var_idx} js_file )
        math( EXPR next_idx "${var_idx} + 1" )
        list( GET JS_FILE_LIST ${next_idx} binary_dir )
        #
        get_filename_component( base_js_file ${js_file} NAME )
        set( lint_file "${binary_dir}/${base_js_file}.lint" )
        #
        # Command runs the js_lint_script above...
        #
        add_custom_command(
            OUTPUT ${lint_file}
            COMMAND ${BASH} ${js_lint_script} "${js_file}" "${lint_file}"
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            DEPENDS ${js_file}
            COMMENT "running gjslint on ${js_file}"
        )
        list( APPEND lint_file_list ${lint_file} )
    endforeach()
    #
    # Make each lint file.
    #
    add_custom_target(
        build_js_lint ALL
        DEPENDS ${lint_file_list}
    )
    #
    # Handy target for wiping out all lint files and forcing a recheck!
    #
    add_custom_target(
        clean_jslint
        COMMAND rm -rf ${lint_file_list}
        DEPENDS ${lint_file_list}
    )
endmacro()

endif()

# vim: ts=4 sw=4 et nocindent
