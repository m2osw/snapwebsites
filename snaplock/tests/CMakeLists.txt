#
# File:
#      snaplock/tests/CMakeLists.txt
#
# Description:
#      A daemon to synchronize processes between any number of computers
#      by blocking all of them but one.
#
# Documentation:
#      See the CMake documentation.
#
# License:
#      Copyright (c) 2016-2019  Made to Order Software Corp.  All Rights Reserved
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
find_package( Catch )

if(CATCH_FOUND)
    set( CASSANDRA_HOST_IP "127.0.0.1" CACHE STRING "Host IP address for the cassandra server to connect to." )
    ##
    ## Test the snap_lock from one or multiple computers
    ##
    project(test_snap_lock)
    include_directories( ${CASSVALUE_INCLUDE_DIRS} )
    add_executable( ${PROJECT_NAME}
        test_snap_lock.cpp
    )
    target_link_libraries(${PROJECT_NAME}
        snapwebsites
        libdbproxy
        ${CASSVALUE_LIBRARIES}
        ${CASSWRAPPER_LIBRARIES}
        ${LIBEXCEPT_LIBRARIES}
        ${LIBTLD_LIBRARIES}
        ${LOG4CPLUS_LIBRARIES}
        ${QT_LIBRARIES}
        dl
    )
    add_test(
        NAME
            ${PROJECT_NAME}

        COMMAND
            ${PROJECT_NAME} --cassandra ${CASSANDRA_HOST_IP}
    )


    ##
    ## Test the snap_lock on a single computer with a snapcommunicator
    ## simulator so we can get things test thoroughly.
    ##
    project(test_multi_snaplock)
    add_executable(${PROJECT_NAME}
        test_multi_snaplocks.cpp
    )
    target_link_libraries(${PROJECT_NAME}
        snapwebsites
        libdbproxy
        #${CASSVALUE_LIBRARIES}
        #${CASSWRAPPER_LIBRARIES}
        ${LIBEXCEPT_LIBRARIES}
        ${LIBTLD_LIBRARIES}
        ${LOG4CPLUS_LIBRARIES}
        ${QT_LIBRARIES}
        dl
    )
    include_directories( ${CMAKE_CURRENT_BINARY_DIR}/../src ) # for version.h
    add_test(
        NAME
            ${PROJECT_NAME}

        COMMAND
            ${PROJECT_NAME} --count 10
    )
endif(CATCH_FOUND)


# vim: ts=4 sw=4 et
