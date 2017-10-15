#
# File:
#      CMakeLists.txt
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
#set(CMAKE_CXX_FLAGS "-Werror -Wall -Wextra -pedantic -std=c++0x -Wcast-align -Wcast-qual -Wctor-dtor-privacy -Wdisabled-optimization -Wformat=2 -Winit-self -Wlogical-op -Wmissing-include-dirs -Wnoexcept -Wold-style-cast -Woverloaded-virtual -Wredundant-decls -Wshadow -Wsign-promo -Wstrict-null-sentinel -Wstrict-overflow=5 -Wundef -Wno-unused -Wno-variadic-macros -Wno-parentheses -fdiagnostics-show-option")
# Development flags:
#set(CMAKE_CXX_FLAGS "-Werror -Wall -Wextra -pedantic -std=c++0x -Wcast-align -Wcast-qual -Wctor-dtor-privacy -Wdisabled-optimization -Wformat=2 -Winit-self -Wlogical-op -Wmissing-include-dirs -Wnoexcept -Woverloaded-virtual -Wredundant-decls -Wshadow -Wsign-promo -Wstrict-null-sentinel -Wstrict-overflow=5 -Wundef -Wno-unused -Wno-variadic-macros -Wno-parentheses -fdiagnostics-show-option")

# Other interesting warnings but that appear all over the place in other libraries
#   -Wsign-conversion  -- unsigned int a; int b; a = b; or b = a;
#   -Wmissing-declarations  -- we often create local functions without first declaring them

project( libsnapwebsites_project )

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Automatically run the moc compiler on qt files
set( CMAKE_AUTOMOC TRUE )

enable_language( CXX )
enable_language( C )

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

#SnapGetVersion( LIBSNAPWEBSITES ${CMAKE_CURRENT_SOURCE_DIR} )

find_library( UUID uuid )
if( ${UUID} STREQUAL "UUID-NOTFOUND" )
    message( FATAL_ERROR "Cannot find uuid library!" )
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
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_BINARY_DIR}/src
    ${ADVGETOPT_INCLUDE_DIRS}
    ${QTCASSANDRA_INCLUDE_DIRS}
    ${QTSERIALIZATION_INCLUDE_DIRS}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${LIBTLD_INCLUDE_DIRS}
)

set( SNAP_TABLE_DESCRIPTION_PATH lib/snapwebsites/tables )

##
## Compiling
##
add_subdirectory(src)           # The snapwebsites libraries
add_subdirectory(doc)           # API Documentation
add_subdirectory(tests)         # Unit tests

##
## Code Analysis
##
## TODO:
## Look into a way to compile everything using the -Weffc++ flag because that
## gives us many interesting warnings;
##
add_custom_target(libwebsites_snap_code_analysis
    COMMAND mkdir -p ${PROJECT_BINARY_DIR}/analysis
    # Count things and report potential problems with different classes
    COMMAND echo "cccc -- output directory ${PROJECT_BINARY_DIR}/analysis/cccc -- and errors saved in ${PROJECT_BINARY_DIR}/analysis/cccc.output"
    COMMAND cccc --lang=c++ --html_outfile=${PROJECT_BINARY_DIR}/analysis/cccc/index.html --outdir=${PROJECT_BINARY_DIR}/analysis/cccc src/*.cpp src/*.h src/*.h.in >${PROJECT_BINARY_DIR}/analysis/cccc.output 2>&1
    # Boost inspect tool that reports various problems
    COMMAND echo "inspect -- output ${PROJECT_BINARY_DIR}/analysis/inspect.html"
    COMMAND inspect -tab -crlf -end -path_name -ascii -minmax -assert_macro -deprecated_macro -unnamed -link -copyright >"${PROJECT_BINARY_DIR}/analysis/inspect.html"
    # Static analysis of the code
    COMMAND echo "cppcheck -- output ${PROJECT_BINARY_DIR}/analysis/cppcheck.out"
    COMMAND cppcheck src >"${PROJECT_BINARY_DIR}/analysis/cppcheck.out" 2>&1

    # All of these are expected to work on source code so make sure we're
    # in the source code top directory
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

# vim: ts=4 sw=4 et nocindent
