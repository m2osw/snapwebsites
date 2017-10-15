################################################################################
# Snap Websites Server -- check css content for lint issues.
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

if( NOT DEFINED SNAP_CSS_LINT )
set( SNAP_CSS_LINT TRUE )

################################################################################
# Handle linting the javascript files...
#
find_program( BASH   bash    /bin     )
find_program( CSSPP  csspp   /usr/bin )
#
#set( OPTIONS "--Werror" )

set( css_lint_script ${CMAKE_BINARY_DIR}/do_css_lint.sh CACHE INTERNAL "CSS lint script" FORCE )
file( WRITE  ${css_lint_script} "#!${BASH}\n"                                                         )
file( APPEND ${css_lint_script} "export LD_LIBRARY_PATH=${CMAKE_INSTALL_PREFIX}/lib\n"                       )
file( APPEND ${css_lint_script} "${CSSPP} ${OPTIONS} $1 -I ${CMAKE_INSTALL_PREFIX}/lib/csspp/scripts -o $2 && exit 0 || (cat $2; rm $2; exit 1)\n" )

function( snap_validate_css CSS_FILE )
    get_filename_component( FULL_CSS_PATH ${CSS_FILE} ABSOLUTE )
    set_property( GLOBAL APPEND PROPERTY CSS_FILE_LIST "${FULL_CSS_PATH}" "${CMAKE_CURRENT_BINARY_DIR}" )
endfunction()

function( snap_clear_css_targets )
    set_property( GLOBAL PROPERTY CSS_FILE_LIST "" )
endfunction()


################################################################################
# Make sure all lint targets get built properly...
# Call snap_build_css_targets() after calling snap_validate_css() above.
#
macro( snap_build_css_targets )
    set( arg_count 2 )
    unset( lint_file_list )
    get_property( CSS_FILE_LIST GLOBAL PROPERTY CSS_FILE_LIST )
    list( LENGTH CSS_FILE_LIST range )
    math( EXPR mod_test "${range} % ${arg_count}" )
    if( NOT ${mod_test} EQUAL 0 )
        message( FATAL_ERROR "The list of files must have an even count. Each CSS file must have an accompanying binary path file!" )
    endif()
    #
    # Create a lint file for each pair
    #
    math( EXPR whole_range "${range} - 1" )
    foreach( var_idx RANGE 0 ${whole_range} ${arg_count} )
        list( GET CSS_FILE_LIST ${var_idx} css_file )
        math( EXPR next_idx "${var_idx} + 1" )
        list( GET CSS_FILE_LIST ${next_idx} binary_dir )
        #
        get_filename_component( base_css_file ${css_file} NAME )
        set( lint_file "${binary_dir}/${base_css_file}.lint" )
        #
        # Command runs the css_lint_script above...
        #
        add_custom_command(
            OUTPUT ${lint_file}
            COMMAND ${BASH} ${css_lint_script} "${css_file}" "${lint_file}"
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            DEPENDS ${css_file}
            COMMENT "running csspp on ${css_file}"
        )
        list( APPEND lint_file_list ${lint_file} )
    endforeach()
    #
    # Make each lint file.
    #
    add_custom_target(
        build_css_lint ALL
        DEPENDS ${lint_file_list}
    )
    #
    # Handy target for wiping out all lint files and forcing a recheck!
    #
    add_custom_target(
        clean_css_lint
        COMMAND rm -rf ${lint_file_list}
        DEPENDS ${lint_file_list}
    )
endmacro()

endif()

# vim: ts=4 sw=4 et nocindent
