# Copyright (c) 2011-2019  Made to Order Software Corp.  All Rights Reserved
#
# https://snapwebsites.org/
# contact@m2osw.com
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
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
)

set( CASSANDRA_HOST_IP "127.0.0.1" CACHE STRING "Host IP address for the cassandra server to connect to." )

##
## The unified unit tests in one large test
##

find_package(SnapCatch2)

if(SnapCatch2_FOUND)

    # TODO: turn this into a static library, because it takes a lot
    # longer to compile each test.
    set( SNAP_TESTS
        ${snapbase_project_SOURCE_DIR}/src/snap_tests.h
        ${snapbase_project_SOURCE_DIR}/src/snap_tests.cpp
    )
    set( TEST_LIBRARIES
        snapwebsites
        libdbproxy
        ${LOG4CPLUS_LIBRARIES}
        ${LIBTLD_LIBRARIES}
        ${QT_LIBRARIES}
        dl
    )

    project( test_unix_timestamp )
    add_executable(${PROJECT_NAME}
        ${SNAP_TESTS}
        test_unix_timestamp.cpp
    )
    target_link_libraries(${PROJECT_NAME}
        ${QT_LIBRARIES}
    )
    add_test( NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME} )


    project( test_entity_filter )
    add_executable(${PROJECT_NAME}
        ${SNAP_TESTS}
        test_entity_filter.cpp
    )
    target_link_libraries(${PROJECT_NAME}
        ${TEST_LIBRARIES}
    )
    add_test( NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME} )


    project( test_ip_strings )
    add_executable(${PROJECT_NAME}
        ${SNAP_TESTS}
        test_ip_strings.cpp
    )
    target_link_libraries(${PROJECT_NAME}
        ${TEST_LIBRARIES}
    )
    add_test( NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME} )

    project(test_quoted_printable)
    add_executable(${PROJECT_NAME}
        ${SNAP_TESTS}
        test_quoted_printable.cpp
    )
    target_link_libraries(${PROJECT_NAME}
        ${TEST_LIBRARIES}
    )
    add_test( NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME} )


    project(test_snap_child_date_parser)
    add_executable(${PROJECT_NAME}
        ${SNAP_TESTS}
        test_snap_child_date_parser.cpp
    )
    target_link_libraries(${PROJECT_NAME}
        ${TEST_LIBRARIES}
    )
    add_test( NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME} )


    project(test_snap_version)
    add_executable( ${PROJECT_NAME}
        ${SNAP_TESTS}
        test_snap_version.cpp
    )
    target_link_libraries(${PROJECT_NAME}
        ${TEST_LIBRARIES}
    )
    add_test( NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME} )



    ##
    ## Check catching and displaying of snap exceptions
    ## (because we render the stack)
    ##
    project(test_snap_exception)
    add_executable( ${PROJECT_NAME}
        ${SNAP_TESTS}
        test_snap_exception.cpp
    )
    target_link_libraries(${PROJECT_NAME}
        ${TEST_LIBRARIES}
    )
    add_test( NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME} )


else(SnapCatch2_FOUND)

    # You may use 'apt-get install catch' under Ubuntu
    message("Catch based tests will not be created because you do not seem to have catch.hpp installed...")

endif(SnapCatch2_FOUND)



# vim: ts=4 sw=4 et
