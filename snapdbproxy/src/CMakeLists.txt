#
# File:
#      snapwebsites/snapdbproxy/src/CMakeLists.txt
#
# Description:
#      A proxy to access the Cassandra database cluster without having to
#      reconnect to Cassandra on each attempt and also in order to hide
#      the problematic threads of the C/C++ Cassandra driver.
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


#################################################################################
## Snap DB Proxy daemon
##
project(snapdbproxy)

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/version.h.in ${CMAKE_CURRENT_BINARY_DIR}/version.h )
include_directories( ${CMAKE_CURRENT_BINARY_DIR} )

add_executable(${PROJECT_NAME}
    snapdbproxy.cpp
    main.cpp
    snapdbproxy_connection.cpp
    snapdbproxy_interrupt.cpp
    snapdbproxy_listener.cpp
    snapdbproxy_messenger.cpp
    snapdbproxy_nocassandra.cpp
    snapdbproxy_thread.cpp
)

target_link_libraries(${PROJECT_NAME}
    snapwebsites
    QtCassandra
    ${ADVGETOPT_LIBRARIES}
    ${CASSWRAPPER_LIBRARIES}
    ${LIBEXCEPT_LIBRARIES}
    ${LOG4CPLUS_LIBRARIES}
    ${QT_LIBRARIES}
    ${LIBTLD_LIBRARIES}
)

install(
    TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin
)


# vim: ts=4 sw=4 et
