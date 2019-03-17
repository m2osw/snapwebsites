################################################################################
# Snap Websites Server -- prepare XML content for checking against DTD
# Copyright (c) 2013-2019  Made to Order Software Corp.  All Rights Reserved
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

if( NOT DEFINED SNAP_XML_LINT )
set( SNAP_XML_LINT TRUE )

################################################################################
# Handle linting the xml files...
#
find_program( BASH    bash    /bin     )
if( ${BASH} STREQUAL "BASH-NOTFOUND" )
    message( FATAL_ERROR "xmllint is used with bash which has to be installed. Run 'sudo apt-get install bash' and try again." )
endif()

find_program( XMLLINT xmllint /usr/bin )
if( ${XMLLINT} STREQUAL "XMLLINT-NOTFOUND" )
    message( FATAL_ERROR "xmllint is not installed. Run 'sudo apt-get install libxml2-utils' and try again." )
endif()

###############################################################################
# Create a bash script that runs the actual xmllint tool against your files.
#
set( lint_script ${CMAKE_BINARY_DIR}/do_xml_lint.sh CACHE INTERNAL "XML lint script" FORCE )
file( WRITE  ${lint_script} "#!${BASH}\n" )
file( APPEND ${lint_script} "if test \${3##*.} = 'dtd'; then\n" )
file( APPEND ${lint_script} "  ${XMLLINT} --dtdvalid $3 --output $2 $1 && exit 0 || (rm $2; exit 1)\n" )
file( APPEND ${lint_script} "else\n" )
file( APPEND ${lint_script} "  ${XMLLINT} --schema $3 --output $2 $1 && exit 0 || (rm $2; exit 1)\n" )
file( APPEND ${lint_script} "fi\n" )

###############################################################################
# Add a DTD/XSD file that should be validated by the cmake process
#
#    snap_validate_xml( <name-of-xml-file-to-validate> <name-of-xsd-or-dtd> )
#
# The search happens in this order:
#
#    . check whether the DTD/XSD file exists as is (local or fullpath)
#    . check as is within CMAKE_CURRENT_SOURCE_DIR
#    . check extension:
#      - if extension is ""
#        + check ${CMAKE_CURRENT_SOURCE_DIR}/xsd/<name>.xsd
#        + check ${CMAKE_CURRENT_SOURCE_DIR}/<name>.xsd
#        + check ${CMAKE_CURRENT_SOURCE_DIR}/dtd/<name>.dtd
#        + check ${CMAKE_CURRENT_SOURCE_DIR}/<name>.dtd
#        + check ${SNAP_COMMON_DIR}/xsd/<name>.xsd
#        + check ${SNAP_COMMON_DIR}/<name>.xsd
#        + check ${SNAP_COMMON_DIR}/dtd/<name>.dtd
#        + check ${SNAP_COMMON_DIR}/<name>.dtd
#        + check ${CMAKE_INSTALL_PREFIX}/share/snapwebsites/xsd/<name>.xsd
#        + check ${CMAKE_INSTALL_PREFIX}/share/snapwebsites/dtd/<name>.dtd (fallback)
#      - if extension is ".xsd"
#        + check ${CMAKE_CURRENT_SOURCE_DIR}/xsd/<name>
#        + check ${CMAKE_CURRENT_SOURCE_DIR}/<name>
#        + check ${SNAP_COMMON_DIR}/xsd/<name>
#        + check ${SNAP_COMMON_DIR}/<name>
#        + check ${CMAKE_INSTALL_PREFIX}/share/snapwebsites/xsd/<name> (fallback)
#      - if extension is ".dtd"
#        + check ${CMAKE_CURRENT_SOURCE_DIR}/dtd/<name>
#        + check ${CMAKE_CURRENT_SOURCE_DIR}/<name>
#        + check ${SNAP_COMMON_DIR}/dtd/<name>
#        + check ${SNAP_COMMON_DIR}/<name>
#        + check ${CMAKE_INSTALL_PREFIX}/share/snapwebsites/dtd/<name> (fallback)
#      - if extension is something else
#        + error
#    . make sure the final result is a file that exists
#
# ${SNAP_COMMON_DIR} is assigned a default in SnapWebsitesConfig.cmake
# so if you include that configuration file, you will get a valid default.
# You may also set your own value to that variable. When not defined, any
# line checking for DTD/XSD files under ${SNAP_COMMON_DIR}/... are ignored.
# See the snapwebsites/CMakeLists.txt for an example of how this directory
# is used (and why it is searched BEFORE the ${CMAKE_INSTALL_PREFIX}/...
# directories, which is very important!)
#
function( snap_validate_xml XML_FILE DTD_FILE )
    # search various paths in the hope of finding the ${DTD_FILE}
    # the result is saved in ${DTD_PATH}
    #
    set( DTD_PATH "" )

    if( EXISTS "${DTD_FILE}" )
        # accept a valid full path as is
        #
        set( DTD_PATH "${DTD_FILE}" )
    elseif( EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${DTD_FILE}" )
        # accept a direct or sub-directory path as is
        #
        set( DTD_PATH "${CMAKE_CURRENT_SOURCE_DIR}/${DTD_FILE}" )
    else()
        # direct (full/sub-directrory) path is not valid,
        # do a search with just the basename
        #
        get_filename_component( DTD_EXTENSION ${DTD_FILE} EXT )
        get_filename_component( DTD_BASEFILE  ${DTD_FILE} NAME )

        if( "${DTD_EXTENSION}" STREQUAL "" )
            # in this case, we can test for both, XSD and DTD files
            # we always want to force an extension, though
            #
            # first define a default in this case
            # (whether it exists will be tested later)
            #
            set( DTD_PATH "${CMAKE_INSTALL_PREFIX}/share/snapwebsites/dtd/${DTD_BASEFILE}.dtd" )

            if( EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/xsd/${DTD_BASEFILE}.xsd" )
                set( DTD_PATH "${CMAKE_CURRENT_SOURCE_DIR}/xsd/${DTD_BASEFILE}.xsd" )
            elseif( EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/dtd/${DTD_BASEFILE}.dtd" )
                set( DTD_PATH "${CMAKE_CURRENT_SOURCE_DIR}/dtd/${DTD_BASEFILE}.dtd" )
            elseif( EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${DTD_BASEFILE}.xsd" )
                set( DTD_PATH "${CMAKE_CURRENT_SOURCE_DIR}/${DTD_BASEFILE}.xsd" )
            elseif( EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${DTD_BASEFILE}.dtd" )
                set( DTD_PATH "${CMAKE_CURRENT_SOURCE_DIR}/${DTD_BASEFILE}.dtd" )
            else()
                if( DEFINED SNAP_COMMON_DIR )
                    # when defined, check in the SNAP_COMMON_DIR
                    #
                    # this is necessary when we have XSD/DTD files installed
                    # within the same project, see the configure_file(...)
                    # constructs for more details.
                    #
                    if( EXISTS "${SNAP_COMMON_DIR}/xsd/${DTD_BASEFILE}.xsd" )
                        set( DTD_PATH "${SNAP_COMMON_DIR}/xsd/${DTD_BASEFILE}.xsd" )
                    elseif( EXISTS "${SNAP_COMMON_DIR}/dtd/${DTD_BASEFILE}.dtd" )
                        set( DTD_PATH "${SNAP_COMMON_DIR}/dtd/${DTD_BASEFILE}.dtd" )
                    elseif( EXISTS "${SNAP_COMMON_DIR}/${DTD_BASEFILE}.xsd" )
                        set( DTD_PATH "${SNAP_COMMON_DIR}/${DTD_BASEFILE}.xsd" )
                    elseif( EXISTS "${SNAP_COMMON_DIR}/${DTD_BASEFILE}.dtd" )
                        set( DTD_PATH "${SNAP_COMMON_DIR}/${DTD_BASEFILE}.dtd" )
                    endif()
                endif()
                if( "${DTD_PATH}" STREQUAL "" OR NOT EXISTS ${DTD_PATH} )
                    # try the distribution (installation) folders last
                    #
                    # if using the XSD or DTD file from another project,
                    # which was built earlier, the file will be there
                    #
                    if( EXISTS "${CMAKE_INSTALL_PREFIX}/share/snapwebsites/xsd/${DTD_BASEFILE}.xsd" )
                        set( DTD_PATH "${CMAKE_INSTALL_PREFIX}/share/snapwebsites/xsd/${DTD_BASEFILE}.xsd" )
                    endif()
                endif()
            endif()
        elseif( "${DTD_EXTENSION}" STREQUAL ".xsd")
            # only check for an XSD file
            #
            # first define a default in this case
            # (whether it exists will be tested later)
            #
            set( DTD_PATH "${CMAKE_INSTALL_PREFIX}/share/snapwebsites/xsd/${DTD_BASEFILE}" )

            if( EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/xsd/${DTD_BASEFILE}" )
                set( DTD_PATH "${CMAKE_CURRENT_SOURCE_DIR}/xsd/${DTD_BASEFILE}" )
            elseif( EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${DTD_BASEFILE}" )
                set( DTD_PATH "${CMAKE_CURRENT_SOURCE_DIR}/${DTD_BASEFILE}" )
            elseif( DEFINED SNAP_COMMON_DIR )
                # when defined, check in the SNAP_COMMON_DIR
                #
                # this is necessary when we have XSD files installed
                # within the same project, see the configure_file(...)
                # constructs for more details.
                #
                if( EXISTS "${SNAP_COMMON_DIR}/xsd/${DTD_BASEFILE}" )
                    set( DTD_PATH "${SNAP_COMMON_DIR}/xsd/${DTD_BASEFILE}" )
                elseif( EXISTS "${SNAP_COMMON_DIR}/${DTD_BASEFILE}" )
                    set( DTD_PATH "${SNAP_COMMON_DIR}/${DTD_BASEFILE}" )
                endif()
            endif()
        elseif( "${DTD_EXTENSION}" STREQUAL ".dtd")
            # only check for an DTD file
            #
            # first define a default in this case
            # (whether it exists will be tested later)
            #
            set( DTD_PATH "${CMAKE_INSTALL_PREFIX}/share/snapwebsites/dtd/${DTD_BASEFILE}" )

            if( EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/dtd/${DTD_BASEFILE}" )
                set( DTD_PATH "${CMAKE_CURRENT_SOURCE_DIR}/dtd/${DTD_BASEFILE}" )
            elseif( EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${DTD_BASEFILE}" )
                set( DTD_PATH "${CMAKE_CURRENT_SOURCE_DIR}/${DTD_BASEFILE}" )
            elseif( DEFINED SNAP_COMMON_DIR )
                # when defined, check in the SNAP_COMMON_DIR
                #
                # this is necessary when we have XSD/DTD files installed
                # within the same project, see the configure_file(...)
                # constructs for more details.
                #
                if( EXISTS "${SNAP_COMMON_DIR}/dtd/${DTD_BASEFILE}" )
                    set( DTD_PATH "${SNAP_COMMON_DIR}/dtd/${DTD_BASEFILE}" )
                elseif( EXISTS "${SNAP_COMMON_DIR}/${DTD_BASEFILE}" )
                    set( DTD_PATH "${SNAP_COMMON_DIR}/${DTD_BASEFILE}" )
                endif()
            endif()
        else()
            # unknown extension
            #
            message( FATAL_ERROR "DTD_PATH: '${DTD_FILE}' has extension '${DTD_EXTENSION}' which is not supported. Expected .xsd or .dtd or no extension." )
        endif()
    endif()

    # verify that we indeed found the ${DTD_FILE}
    #
    if( NOT EXISTS ${DTD_PATH} )
        message( FATAL_ERROR "DTD_PATH: '${DTD_FILE}' tested as '${DTD_PATH}' was not found on the system! Please install or use configure_file(...) if from the same project and try again." )
    endif()

    get_filename_component( FULL_XML_PATH ${XML_FILE} ABSOLUTE )

    set_property( GLOBAL APPEND PROPERTY XML_FILE_LIST "${FULL_XML_PATH}" "${DTD_PATH}" "${CMAKE_CURRENT_BINARY_DIR}" )
endfunction()

###############################################################################
# Clear the XML_FILE_LIST so a large project can maintained several smaller
# lists in various subdirectory()
#
function( snap_clear_xml_targets )
    set_property( GLOBAL PROPERTY XML_FILE_LIST "" )
endfunction()


###############################################################################
# Make sure all lint targets get built properly...
# Call snap_build_xml_targets() after calling snap_validate_xml() above.
#
macro( snap_build_xml_targets )
	set( arg_count 3 )
    unset( lint_file_list )
    get_property( XML_FILE_LIST GLOBAL PROPERTY XML_FILE_LIST )
    list( LENGTH XML_FILE_LIST range )
    math( EXPR mod_test "${range} % ${arg_count}" )
    if( NOT ${mod_test} EQUAL 0 )
        message( FATAL_ERROR "The list of files must be a multiple of ${arg_count}. Each XML file must have accompanying DTD file and path!" )
    endif()
    #
    # Create a lint file for each pair
    #
    math( EXPR whole_range "${range} - 1" )
    foreach( var_idx RANGE 0 ${whole_range} ${arg_count} )
        list( GET XML_FILE_LIST ${var_idx} xml_file )
        math( EXPR next_idx "${var_idx} + 1" )
        list( GET XML_FILE_LIST ${next_idx} dtd_file )
        math( EXPR next_idx "${next_idx} + 1" )
        list( GET XML_FILE_LIST ${next_idx} binary_dir )
        #
        get_filename_component( base_xml_file ${xml_file} NAME )
        set( lint_file "${binary_dir}/${base_xml_file}.lint" )
        #
        # Command runs the lint_script above...
        #
        add_custom_command(
            OUTPUT ${lint_file}
            COMMAND ${BASH} ${lint_script} ${xml_file} ${lint_file} ${dtd_file}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            DEPENDS ${xml_file} ${dtd_file}
            COMMENT "running xmllint on ${xml_file}"
        )
        list( APPEND lint_file_list ${lint_file} )
    endforeach()
    #
    # Make each lint file.
    #
    add_custom_target(
        ${PROJECT_NAME}_build_lint ALL
        DEPENDS ${lint_file_list}
    )
    #
    # Handy target for wiping out all lint files and forcing a recheck!
    #
    add_custom_target(
        ${PROJECT_NAME}_clean_lint
        COMMAND rm -rf ${lint_file_list}
        DEPENDS ${lint_file_list}
    )
endmacro()

endif()

# vim: ts=4 sw=4 et nocindent
