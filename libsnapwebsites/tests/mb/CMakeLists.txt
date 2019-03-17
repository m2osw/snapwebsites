#
# File:
#      libsnapwebsites/tests/mb/CMakeLists.txt
#
# Description:
#      Multi-binary and/or tests with side effects.
#      These tests do not include any network related test.
#
# Documentation:
#      See the CMake documentation.
#
# License:
#      Copyright (c) 2011-2019  Made to Order Software Corp.  All Rights Reserved
#
#      https://snapwebsites.org/
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

#include_directories(
#    ${CMAKE_CURRENT_SOURCE_DIR}
#)


##
## NOTE: Some of these tests may not have the add_test() cmake command because
##       running the test may not be that simple.
##

set( LIBRARIES
    ${CASSWRAPPER_LIBRARIES}
    dl
    ${LIBEXCEPT_LIBRARIES}
    ${LOG4CPLUS_LIBRARIES}
    ${LIBTLD_LIBRARIES}

    ${QT_LIBRARIES}
    # This one is not general, but we can have it in all tests, that's fine.
    Qt5::X11Extras

    snapwebsites
    libdbproxy
)

##
## Test the email class
##
project(test_email)

add_compile_options(-DSENDMAIL_PATH=\"${CMAKE_CURRENT_SOURCE_DIR}\")
add_executable( ${PROJECT_NAME}
    test_email.cpp
)
target_link_libraries( ${PROJECT_NAME} ${LIBRARIES} )

add_test( test_email ${PROJECT_NAME} )


##
## Test that the color matrices work as expected
##
project(test_color_matrix)

add_executable( ${PROJECT_NAME}
    test_color_matrix.cpp
)
target_link_libraries( ${PROJECT_NAME} ${LIBRARIES} )

add_test( test_color_matrix ${PROJECT_NAME} )



# vim: ts=4 sw=4 et
