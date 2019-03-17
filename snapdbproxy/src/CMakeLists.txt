#
# File:
#      snapdbproxy/src/CMakeLists.txt
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


#################################################################################
## Snap DB Proxy daemon
##
project(snapdbproxy)

# Use just here to check for memory leaks that snapdbproxy may generate
# See SNAP-663 -- however, this did not help me, there are no leaks that SAN
#                 detects so I had to look in another way (i.e. when leaving
#                 the application cleanly, all memory is cleanly freed)
#set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fsanitize=enum -fsanitize=unreachable -fsanitize=leak")

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/version.h.in ${CMAKE_CURRENT_BINARY_DIR}/version.h )
include_directories( ${CMAKE_CURRENT_BINARY_DIR} )

add_executable(${PROJECT_NAME}
    snapdbproxy.cpp
    main.cpp
    snapdbproxy_connection.cpp
    snapdbproxy_initializer.cpp
    snapdbproxy_initializer_thread.cpp
    snapdbproxy_interrupt.cpp
    snapdbproxy_listener.cpp
    snapdbproxy_messenger.cpp
    snapdbproxy_nocassandra.cpp
    snapdbproxy_statuschanged.cpp
    snapdbproxy_thread.cpp
)

target_link_libraries(${PROJECT_NAME}
    snapwebsites
    libdbproxy
    ${ADVGETOPT_LIBRARIES}
    ${CASSWRAPPER_LIBRARIES}
    ${LIBEXCEPT_LIBRARIES}
    ${LOG4CPLUS_LIBRARIES}
    ${QT_LIBRARIES}
    ${LIBTLD_LIBRARIES}
)

install(
    TARGETS
        ${PROJECT_NAME}

    RUNTIME DESTINATION
        sbin
)


# vim: ts=4 sw=4 et
