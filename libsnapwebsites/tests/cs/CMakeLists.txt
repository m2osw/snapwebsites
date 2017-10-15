#
# File:
#      snapwebsites/libsnapwebsites/tests/cs/CMakeLists.txt
#
# Description:
#      Client/Server tests.
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

#include_directories(
#    ${CMAKE_CURRENT_SOURCE_DIR}
#)


##
## WARNING: These tests do not have the add_test() cmake command because
##          you cannot just run the server or the client on its own. They
##          have to run together and in most cases the server has to be
##          ready before the client gets started.
##

set( LIBRARIES
    ${CASSWRAPPER_LIBRARIES}
    dl
    ${LIBEXCEPT_LIBRARIES}
    ${LOG4CPLUS_LIBRARIES}
    ${LIBTLD_LIBRARIES}
    ${QT_LIBRARIES}
    snapwebsites
    QtCassandra
)

##
## Test a client/server via snap_communicator to make sure that the
## bio_client and the bio_server do indeed work.
##
project(test_ssl_server)
add_executable( ${PROJECT_NAME}
    test_ssl_server.cpp
)
target_link_libraries( ${PROJECT_NAME} ${LIBRARIES} )


project(test_ssl_client)
add_executable( ${PROJECT_NAME}
    test_ssl_client.cpp
)
target_link_libraries( ${PROJECT_NAME} ${LIBRARIES} )





##
## Test a client/server via snap_communicator to see whether shutdown()
## has the same effect in the parent as it has in the client
##
## (It was determined that YES it is incorrect so we actually have the
## shutdown turned off--see the code if you want to test with it turned ON)
##
project(test_shutdown_server)
add_executable( ${PROJECT_NAME}
    test_shutdown_server.cpp
)
target_link_libraries( ${PROJECT_NAME} ${LIBRARIES} )


project(test_shutdown_client)
add_executable( ${PROJECT_NAME}
    test_shutdown_client.cpp
)
target_link_libraries( ${PROJECT_NAME} ${LIBRARIES} )




# vim: ts=4 sw=4 et
