#
# File:
#      snapcommunicator/tests/CMakeLists.txt
#
# Description:
#      The Snap! Websites Server. This CMake compiles tests that can be used
#      against the server. All C++ tests should be converted to a catch_*
#      test instead of its own brew. Now that we have a test bed ready for
#      such the conversion should be easy enough. We will still have separate
#      tests that are external processes we need to run against the main test.
#      (i.e. a test for client/server network connections.)
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

set( CASSANDRA_HOST_IP "127.0.0.1" CACHE STRING "Host IP address for the cassandra server to connect to." )

##
## The unified unit tests in one large test
##

find_package(SnapCatch2)

if(SnapCatch2_FOUND)
    ##
    ## Code from snapbase
    ##
    set( SNAP_TESTS
        ${snapbase_project_SOURCE_DIR}/src/snap_tests.h
        ${snapbase_project_SOURCE_DIR}/src/snap_tests.cpp
    )
    include_directories( ${snapbase_project_SOURCE_DIR}/src )

    project(test_snap_communicator)
    add_executable(${PROJECT_NAME}
        # Main
        ${SNAP_TESTS}
        catch_tests.cpp
        # Test suites
        catch_snap_communicator.cpp
    )
    target_link_libraries(${PROJECT_NAME}
        snapwebsites
        libdbproxy
        ${LOG4CPLUS_LIBRARIES}
        ${LIBTLD_LIBRARIES}
        ${QT_LIBRARIES}
        dl
    )
    add_test( NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME} --host ${CASSANDRA_HOST_IP} )

    ##
    ## A client used to test our Snap! Communicator
    ##
    project(test_snap_communicator_client)
    add_executable(${PROJECT_NAME}
        # Main
        ${SNAP_TESTS}
        test_snap_communicator_client.cpp
    )
    target_link_libraries(${PROJECT_NAME}
        snapwebsites
        libdbproxy
        ${LOG4CPLUS_LIBRARIES}
        ${LIBTLD_LIBRARIES}
        ${QT_LIBRARIES}
        dl
    )
    add_test( NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME} --host ${CASSANDRA_HOST_IP} )

else(SnapCatch2_FOUND)

    # You may use 'apt-get install catch' under Ubuntu
    message("Catch based tests will not be created because you do not seem to have catch.hpp installed...")

endif(SnapCatch2_FOUND)



# vim: ts=4 sw=4 et
