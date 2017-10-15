#
# File:
#      plugins/output/javascript_unicode/CMakeLists.txt
#
# Description:
#      Generate JavaScript used to help with Unicode handling. We can look
#      at RegExp again once most browsers support version 6, but until then
#      we do not have access to most of that data within JavaScript.
#
#      This project is a separate, standalone project. We only rebuild and
#      generate the final javascript-unicode.js only once in a while when
#      the version of Unicode changes in Qt.
#
# Documentation:
#      See the CMake documentation.
#
# License:
#      Copyright (c) 2016-2017 Made to Order Software Corp.
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

cmake_minimum_required(VERSION 3.5)

project(javascript_unicode)

enable_language( CXX )

find_package( Qt5Core REQUIRED )

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
)


#snap_validate_js( javascript-unicode.js )


add_executable(${PROJECT_NAME}
    javascript_unicode.cpp
)

target_link_libraries(${PROJECT_NAME}
    Qt5::Core
)

#get_property( 3RDPARTY_INCLUDED GLOBAL PROPERTY 3RDPARTY_INCLUDED )


# vim: ts=4 sw=4 et
