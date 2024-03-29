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
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

project(snapwebsites)

# Put the version in the header file
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/version.h
)


option( SNAP_NO_FORK OFF "Set to suppress snapserver's inate forking [for debugging purposes only--not for release!]" )
if( ${SNAP_NO_FORK} )
    message( WARNING "SNAP_NO_FORK is turned on! This is not a production-ready build!" )
    add_definitions( -DSNAP_NO_FORK )
endif()


qt5_add_resources(SNAP_LIBRARY_RESOURCES_RCC
    snap.qrc
)

snap_validate_xml( core-tables.xml tables.xsd )



add_library(${PROJECT_NAME} SHARED
    compression.cpp                             # compress/decompress data
    #dbutils.cpp                                 # utilities to help convert coded table and row names and column data. (see snap_tables.cpp too!)
    floats.cpp                                  # Floats helper functions
    fuzzy_string_compare.cpp                    # Compare strings in a fuzzy manner
    loadavg.cpp                                 # handle the loadavg file
    mail_exchanger.cpp                          # retrieve the MX record for a given domain
    meminfo.cpp                                 # read /proc/meminfo
    mkgmtime.c                                  # mktime() with a UTC time
    mounts.cpp                                  # read /proc/mounts or some other mount point file
    password.cpp                                # generate hashed password of various types
    qdomhelpers.cpp                             # Qt extensions to help with the DOM
    qdomnodemodel.cpp                           # Qt extension to support XSLT
    qdomreceiver.cpp                            # Qt extension to output XSLT as XML
    qdomxpath.cpp                               # Qt extension to query a XML DOM with X-Path
    qhtmlserializer.cpp                         # Qt extension to output HTML as a string
    quoted_printable.cpp                        # to encode / decode data using the quoted printable format
    qxmlmessagehandler.cpp                      # Qt extension for us to capture the QXmlQuery messages
    snap_backend.cpp                            # type of snap_child for backends
    snap_cassandra.cpp                          # encapuslate initiating a connection to the database.
    snap_console.cpp                            # a CUI helper with two panels: Input and Output so the output can be asynchronous
    snap_child.cpp                              # requests processing
    snap_child_cache_control.cpp                # manage cache controls in the snap_child class
    snap_image.cpp                              # image handling
    snap_initialize_website.cpp                 # initialize website by sending an Apache-like query to the server
    snap_lock.cpp                               # support for inter-process / inter-computer locks using network and memory only
    snap_magic.cpp                              # support for MIME type
    snap_pid.cpp                                # RAII PID file management
    snap_tables.cpp                             # table schema/definitions
    snap_version.cpp                            # extract name, version, browser info from a filename
    snapwebsites.cpp                            # server
    xslt.cpp                                    # code to do XSLT transformations
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${LIBADDR_INCLUDE_DIRS}
        ${LIBEXCEPT_INCLUDE_DIRS}
        ${LIBPROCPS_INCLUDE_DIRS}
        ${MAGIC_INCLUDE_DIRS}
        ${OPENSSL_INCLUDE_DIR}
        ${SNAPLOGGER_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}
    ${ADVGETOPT_LIBRARIES}
    ${CASSVALUE_LIBRARIES}
    ${LIBADDR_LIBRARIES}
    ${LIBEXCEPT_LIBRARIES}
    ${LIBTLD_LIBRARIES}
    ${SNAPLOGGER_LIBRARIES}
    ${QT_LIBRARIES}
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
    TARGETS
        ${PROJECT_NAME}

    LIBRARY DESTINATION
        lib
)

install(
    FILES
        tables.xsd

    DESTINATION
        share/snapwebsites/xsd
)

install(
    FILES
        core-tables.xml

    DESTINATION
        ${SNAP_TABLE_DESCRIPTION_PATH}
)

install(
    DIRECTORY
        ${CMAKE_CURRENT_SOURCE_DIR}/

    DESTINATION
        include/snapwebsites

    FILES_MATCHING PATTERN
        "*.h"
)

install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/version.h

    DESTINATION
        include/snapwebsites
)

#
# WARNING: we use a configure_file(...) on top of an install(FILES ...)
#          so we can access the file in our SnapXmlLint.cmake without
#          any additional magic
#
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/tables.xsd
    ${SNAP_COMMON_DIR}/xsd/tables.xsd
    COPYONLY
)


# vim: ts=4 sw=4 et nocindent
