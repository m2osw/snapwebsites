#
# File:
#      snapwebsites/libsnapwebsites/tests/CMakeLists.txt
#
# Description:
#      Various very basic OS tests to verify that we can use such and
#      such OS feature before implementing it in the libsnapwebsites
#      library.
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

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
)

##
## One test per OS feature tested
##


##
## Check gethostbyname()
##
project(gethostips)
add_executable( ${PROJECT_NAME}
    gethostips.cpp
)
add_test( NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME} )


##
## String Speed (testing a Qt feature)
##
project(string_speed)
add_executable( ${PROJECT_NAME}
    string_speed.cpp
)
target_link_libraries(${PROJECT_NAME}
    ${QT_LIBRARIES}
)
add_test( NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME} )


##
## NetLink to track processes (forks())
##
project(pmon)
add_executable( ${PROJECT_NAME}
    pmon.c
)


# vim: ts=4 sw=4 et
