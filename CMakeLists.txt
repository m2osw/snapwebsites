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
#      Permission is hereby granted, free of charge, to any person obtaining a
#      copy of this software and associated documentation files (the
#      "Software"), to deal in the Software without restriction, including
#      without limitation the rights to use, copy, modify, merge, publish,
#      distribute, sublicense, and/or sell copies of the Software, and to
#      permit persons to whom the Software is furnished to do so, subject to
#      the following conditions:
#
#      The above copyright notice and this permission notice shall be included
#      in all copies or substantial portions of the Software.
#
#      THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
#      OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
#      MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
#      IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
#      CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
#      TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
#      SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#

cmake_minimum_required( VERSION 3.5 )

project( snapwebsites_top_dir )

if( "${CMAKE_BUILD_TYPE}" STREQUAL "Debug" )
    message( STATUS "Debug is in effect for snapwebsites!" )
endif()

## Include support modules
##
set( CMAKE_MODULE_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake
    ${CMAKE_MODULE_PATH}
)

## Include 3rdparty and contrib
##
find_package( AdvGetOpt        REQUIRED )
find_package( As2Js            0.1.0    REQUIRED )
find_package( Boost            REQUIRED )
find_package( CassValue        REQUIRED )
find_package( CassWrapper      REQUIRED )
find_package( CSSPP            REQUIRED )
find_package( LibExcept        REQUIRED )
find_package( LibTLD           REQUIRED )
find_package( LibXml2          REQUIRED )
find_package( log4cplus        REQUIRED )
find_package( Magic            REQUIRED )
find_package( OpenSSL          REQUIRED )
find_package( ProcPS           REQUIRED )
find_package( Qt5Core          REQUIRED )
find_package( Qt5Network       REQUIRED )
find_package( Qt5Script        REQUIRED )
find_package( Qt5Sql           REQUIRED )
find_package( Qt5Widgets       REQUIRED )
find_package( Qt5Xml           REQUIRED )
find_package( Qt5XmlPatterns   REQUIRED )
find_package( QtSerialization  REQUIRED )
find_package( SnapCMakeModules REQUIRED )
find_package( ZLIB             REQUIRED )
find_package( ZipIos           REQUIRED 2.1      )
find_package( SnapDoxygen      )

SnapGetVersion( SNAPWEBSITES ${CMAKE_CURRENT_SOURCE_DIR} )

################################################################################
# Override the DTD_SOURCE_PATH that is provided by the libsnapwebsites library
set( DTD_SOURCE_PATH "${CMAKE_BINARY_DIR}/dist/share/snapwebsites/dtd" CACHE PATH "Location of the default DTD files" )
set( XSD_SOURCE_PATH "${CMAKE_BINARY_DIR}/dist/share/snapwebsites/xsd" CACHE PATH "Default XSD source files."         )
set( SNAP_DIST_DIR   "${CMAKE_CURRENT_BINARY_DIR}/dist"                CACHE PATH "Local install dir."                )

include( SnapCssLint   )
include( SnapJsLint    )
include( SnapXmlLint   )
include( SnapZipLayout )

add_subdirectory( cmake                   )
add_subdirectory( libsnapwebsites         )
add_subdirectory( cassview                )
add_subdirectory( snapbase                )
add_subdirectory( snapmanager             )
add_subdirectory( snapserver              )
add_subdirectory( snapserver-core-plugins )
add_subdirectory( snapbackend             )
add_subdirectory( snapbackup              )
add_subdirectory( snapbounce              )
add_subdirectory( snapcgi                 )
add_subdirectory( snapcommunicator        )
add_subdirectory( snapdb                  )
add_subdirectory( snapdbproxy             )
add_subdirectory( snapfirewall            )
add_subdirectory( snaplayout              )
add_subdirectory( snaplistd               )
add_subdirectory( snaplock                )
add_subdirectory( snaplog                 )
add_subdirectory( snapwatchdog            )

# vim: ts=4 sw=4 et nocindent
