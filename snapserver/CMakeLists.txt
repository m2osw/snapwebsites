#
# File:
#      snapwebsites/snapserver/CMakeLists.txt
#
# Description:
#      Definitions to create the build environment with cmake
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

##
## Initialization
##
cmake_minimum_required(VERSION 3.5)

# Reasons for warnings that are turned off:
# * We use variadic macros for the signals, very useful to avoid otherwise
#   quite complicated statements or many macro definitions (one per # of params).
# * We use | in our parser grammar and we get many parenthesis warnings for those.
# Final flags:
#set(CMAKE_CXX_FLAGS "-Werror -Wall -Wextra -pedantic -Wcast-align -Wcast-qual -Wctor-dtor-privacy -Wdisabled-optimization -Wformat=2 -Winit-self -Wlogical-op -Wmissing-include-dirs -Wnoexcept -Wold-style-cast -Woverloaded-virtual -Wredundant-decls -Wshadow -Wsign-promo -Wstrict-null-sentinel -Wstrict-overflow=5 -Wundef -Wno-unused -Wno-variadic-macros -Wno-parentheses -fdiagnostics-show-option")
# Development flags:
#set(CMAKE_CXX_FLAGS "-Werror -Wall -Wextra -pedantic -Wcast-align -Wcast-qual -Wctor-dtor-privacy -Wdisabled-optimization -Wformat=2 -Winit-self -Wlogical-op -Wmissing-include-dirs -Wnoexcept -Woverloaded-virtual -Wredundant-decls -Wshadow -Wsign-promo -Wstrict-null-sentinel -Wstrict-overflow=5 -Wundef -Wno-unused -Wno-variadic-macros -Wno-parentheses -fdiagnostics-show-option")

# Other interesting warnings but that appear all over the place in other libraries
#   -Wsign-conversion  -- unsigned int a; int b; a = b; or b = a;
#   -Wmissing-declarations  -- we often create local functions without first declaring them

project( snapwebsites_project )

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Automatically run the moc compiler on qt files
set( CMAKE_AUTOMOC TRUE )

option( RUN_TESTS "Enable tests to run via CTest" OFF )
if( ${RUN_TESTS} )
    enable_testing()
endif()

## Include support modules
##
set( CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH} )

include_directories( SYSTEM
    ${Qt5Core_INCLUDE_DIRS}
    ${Qt5Network_INCLUDE_DIRS}
    ${Qt5Script_INCLUDE_DIRS}
    ${Qt5Sql_INCLUDE_DIRS}
    ${Qt5Widgets_INCLUDE_DIRS}
    ${Qt5Xml_INCLUDE_DIRS}
    ${Qt5XmlPatterns_INCLUDE_DIRS}
)
set( QT_LIBRARIES
    Qt5::Core
    Qt5::Network
    Qt5::Script
    Qt5::Sql
    Qt5::Xml
    Qt5::XmlPatterns
)

include_directories(
    ${libsnapwebsites_project_SOURCE_DIR}/src
    ${libsnapwebsites_project_BINARY_DIR}/src
    ${snapmanagercgi_project_BINARY_DIR}/lib
    ${snapmanagercgi_project_SOURCE_DIR}/lib
    ${QTSERIALIZATION_INCLUDE_DIRS}
)

find_library( UUID uuid )
if( ${UUID} STREQUAL "UUID-NOTFOUND" )
    message( FATAL_ERROR "Cannot find uuid library!" )
endif()

find_program( ZIP zip               REQUIRED )
find_program( CLOSURELINTER gjslint REQUIRED DOC "verify that Google JavaScript linter is installed: sudo apt-get install closure-linter" )

if( "${XMLLINT_EXECUTABLE}" STREQUAL "XMLLINT_EXECUTABLE-NOTFOUND" )
    message( FATAL_ERROR "xmllint was not found, you need to install libxml2-utils" )
endif()

# A few extra warnings specifically for snapwebsites libraries and tools
# You can also play with -Weffc++ although we are definitively not compliant
# (especially we do not define all the auto-initialized variables!)
# -Wconversion -- would be nice, a few things are still not cooperating with that one
# -DUNISTR_FROM_STRING_EXPLICIT=explicit is for the Unicode library, to make sure it does not copy strings arounds automatically
# -DMAGICKCORE_QUANTUM_DEPTH=16 is the default depth, newer versions of ImageMagick require that definition from you (no internal default)
# -DMAGICKCORE_HDRI_ENABLE=0 is also the default for ImageMagick; I'm not too sure what HDRI stands for though...
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wold-style-cast -Wnoexcept -DUNISTR_FROM_STRING_EXPLICIT=explicit -DMAGICKCORE_QUANTUM_DEPTH=16 -DMAGICKCORE_HDRI_ENABLE=0" )
set( CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -fwrapv" )

include_directories(
    ${libsnapwebsites_project_SOURCE_DIR}/src
    ${libsnapwebsites_project_BINARY_DIR}/src
    ${ADVGETOPT_INCLUDE_DIRS}
    ${QTCASSANDRA_INCLUDE_DIRS}
    ${QTSERIALIZATION_INCLUDE_DIRS}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${LIBTLD_INCLUDE_DIRS}
)

snap_clear_xml_targets()

##
## Compiling
##
add_subdirectory( conf            ) # Configuration files and samples
add_subdirectory( src             ) # Actual Snap! Websites server
add_subdirectory( manager-plugins ) # Actual Snap! Websites server
add_subdirectory( doc             ) # Generate the documentation
add_subdirectory( tests           ) # Tests to run against the server

snap_build_xml_targets()

##
## Install run scripts
##
install(
    PROGRAMS
        scripts/reset-db.sh
        scripts/run-server.sh
    DESTINATION
        ./
)

##
## Code Analysis
##
## TODO:
## Look into a way to compile everything using the -Weffc++ flag because that
## gives us many interesting warnings;
##
add_custom_target(snapwebsites_snap_code_analysis
    COMMAND mkdir -p ${PROJECT_BINARY_DIR}/analysis
    # Count things and report potential problems with different classes
    COMMAND echo "cccc -- output directory ${PROJECT_BINARY_DIR}/analysis/cccc -- and errors saved in ${PROJECT_BINARY_DIR}/analysis/cccc.output"
    COMMAND cccc --lang=c++ --html_outfile=${PROJECT_BINARY_DIR}/analysis/cccc/index.html --outdir=${PROJECT_BINARY_DIR}/analysis/cccc src/*.cpp plugins/*/*.cpp plugins/*/*.h >${PROJECT_BINARY_DIR}/analysis/cccc.output 2>&1
    # Boost inspect tool that reports various problems
    COMMAND echo "inspect -- output ${PROJECT_BINARY_DIR}/analysis/inspect.html"
    COMMAND inspect -tab -crlf -end -path_name -ascii -minmax -assert_macro -deprecated_macro -unnamed -link -copyright >"${PROJECT_BINARY_DIR}/analysis/inspect.html"
    # Static analysis of the code
    COMMAND echo "cppcheck -- output ${PROJECT_BINARY_DIR}/analysis/cppcheck.out"
    COMMAND cppcheck plugins src tests >"${PROJECT_BINARY_DIR}/analysis/cppcheck.out" 2>&1
    # Count # of TODO, XXX, TBD, FIXME, and \todo: (documented in doxygen)
    COMMAND sh tests/todo.sh "${PROJECT_BINARY_DIR}/analysis"

    # All of these are expected to work on source code so make sure we're
    # in the source code top directory
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

add_custom_target(snap_javascript_analysis
    # Compiler need to be scripts that get called on each compile
    # because otherwise errors will pile up! This will require us
    # to move to 14.10 so we get the latest working version of
    # javascript closure compiler.

    # Lint the JavaScript code to find additional "errors"
    COMMAND sh tests/js-lint.sh "${PROJECT_BINARY_DIR}/analysis"
    # Compile the JavaScript code to verify most everything (strong typing, etc.)
    COMMAND sh tests/js-compile.sh "${PROJECT_BINARY_DIR}/analysis"

    # All of these are expected to work on source code so make sure we're
    # in the source code top directory
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

# vim: ts=4 sw=4 et nocindent
