#
# File:
#      snapwebsites/libsnapwebsites/tests/ip/CMakeLists.txt
#
# Description:
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

project(ip_test)

include_directories(
    SYSTEM
        ${MAGIC_INCLUDE_DIRS}
        ${LOG4CPLUS_INCLUDE_DIRS}
        ${LIBPROCPS_INCLUDE_DIRS}
        ${OPENSSL_INCLUDE_DIR}
)

include_directories(
    ${LIBTLD_INCLUDE_DIRS}
    ${snapwebsites_SOURCE_DIR}
)

add_executable( ${PROJECT_NAME} 
    ip_test.cpp
)

target_link_libraries( ${PROJECT_NAME}
    ${ADVGETOPT_LIBRARIES}
    ${LIBEXCEPT_LIBRARIES}
    ${LIBTLD_LIBRARIES}
    ${LOG4CPLUS_LIBRARIES}
    ${QT_LIBRARIES}
    ${AS2JS_LIBRARIES}
    ${QTSERIALIZATION_LIBRARIES}
    ${MAGIC_LIBRARIES}
    ${LIBPROCPS_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    ${UUID}
    pthread
    dl
    snapwebsites
    libdbproxy
)

# vim: ts=4 sw=4 et nocindent
