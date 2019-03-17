#
# File:
#      libsnapwebsites/src/snapwebsites_qt/CMakeLists.txt
#
# Description:
#      Library used by the Snap! Websites front ends which make use of Qt,
#      X-Windows and other equivalents (i.e. you are likely to need a
#      Desktop.) Because of that, it can't be merged with the basic
#      libsnapwebsites.
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

project(snapwebsites_qt)

include_directories(
    ${MAGIC_INCLUDE_DIRS}
    ${LIBPROCPS_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
    ${CASSVALUE_INCLUDE_DIRS}
    ${LIBADDR_INCLUDE_DIRS}
    ${LIBTLD_INCLUDE_DIRS}
    ${LOG4CPLUS_INCLUDE_DIRS}
)

qt5_add_resources(SNAP_LIBRARY_RESOURCES_RCC
    snap.qrc
)

set( HEADERS
    snap_communicator_qt.h
)

add_library( ${PROJECT_NAME} SHARED
    ${HEADERS}
    snap_communicator_qt.cpp                    # qt snap_connection extension
)

target_link_libraries( ${PROJECT_NAME}
    ${ADVGETOPT_LIBRARIES}
    ${CASSVALUE_LIBRARIES}
    ${LIBADDR_LIBRARIES}
    ${LIBEXCEPT_LIBRARIES}
    ${LIBTLD_LIBRARIES}
    ${LOG4CPLUS_LIBRARIES}
    ${QT_LIBRARIES}
    # And the "offensive" library (as far as libsnapwebsites is concerned)
    Qt5::X11Extras
    ${AS2JS_LIBRARIES}
    ${QTCASSANDRA_LIBRARIES}
    ${QTSERIALIZATION_LIBRARIES}
    ${MAGIC_LIBRARIES}
    ${LIBPROCPS_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    ${UUID}
    ncurses
    readline
    pthread
    dl
)

set_target_properties( ${PROJECT_NAME} PROPERTIES
    VERSION ${SNAPWEBSITES_VERSION_MAJOR}.${SNAPWEBSITES_VERSION_MINOR}
    SOVERSION ${SNAPWEBSITES_VERSION_MAJOR}
)

install(
    TARGETS ${PROJECT_NAME} LIBRARY DESTINATION lib
)

# TODO:
# See whether we could use the same folder as "libsnapwebsites"
# but I don't think that's doable...
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/snapwebsites_qt
    FILES_MATCHING PATTERN "*.h"
)

# vim: ts=4 sw=4 et nocindent
