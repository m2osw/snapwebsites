#
# File:
#      snapwebsites/CMakeLists.txt
#
# Description:
#      Library used by the Snap! Websites server and plugins. All the functions
#      and classes that can be used in common are defined here
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

project(snapwebsites)

# Put the version in the header file
configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/version.h.in ${CMAKE_CURRENT_BINARY_DIR}/version.h )


option( SNAP_NO_FORK OFF "Set to suppress snapserver's inate forking [for debugging purposes only--not for release!]" )
if( ${SNAP_NO_FORK} )
    message( WARNING "SNAP_NO_FORK is turned on! This is not a production-ready build!" )
    add_definitions( -DSNAP_NO_FORK )
endif()


include_directories(
    SYSTEM
        ${MAGIC_INCLUDE_DIRS}
        ${LIBPROCPS_INCLUDE_DIRS}
        ${OPENSSL_INCLUDE_DIR}
)

include_directories(
    ${CASSVALUE_INCLUDE_DIRS}
    ${LIBTLD_INCLUDE_DIRS}
    ${LOG4CPLUS_INCLUDE_DIRS}
)

qt5_add_resources(SNAP_LIBRARY_RESOURCES_RCC
    snap.qrc
)

set( HEADERS
    addr.h
    cache_control.h
    chownnm.h
    compression.h
    dbutils.h
    file_content.h
    floats.h
    fuzzy_string_compare.h
    glob_dir.h
    http_client_server.h
    http_cookie.h
    http_link.h
    http_strings.h
    inet_helpers.h
    join_strings.h
    loadavg.h
    lockfile.h
    log.h
    mail_exchanger.h
    minmax.h
    mkdir_p.h
    mkgmtime.h
    mounts.h
    not_reached.h
    not_used.h
    plugins.h
    poison.h
    process.h
    qcaseinsensitivestring.h
    qdomhelpers.h
    qdomnodemodel.h
    qdomreceiver.h
    qdomxpath.h
    qhtmlserializer.h
    qlockfile.h
    qstring_stream.h
    quoted_printable.h
    qxmlmessagehandler.h
    reverse_cstring.h
    snap_backend.h
    snap_cassandra.h
    snap_child.h
    snap_communicator.h
    snap_config.h
    snap_exception.h
    snap_expr.h
    snap_image.h
    snap_initialize_website.h
    snap_lock.h
    snap_magic.h
    snap_parser.h
    snap_pipe.h
    snap_signals.h
    snap_string_list.h
    snap_tables.h
    snap_thread.h
    snap_uri.h
    snap_utf8.h
    snap_version.h
    snapwebsites.h
    string_pathinfo.h
    string_replace.h
    tcp_client_server.h
    tokenize_string.h
    udp_client_server.h
    version.h.in
    xslt.h
)

add_library( ${PROJECT_NAME} SHARED
    ${HEADERS}
    addr.cpp                                    # handling of host IP addresses (IPv4 and IPv6)
    cache_control.cpp                           # parse / handle Cache-Control settings
    chownnm.cpp                                 # chown using user and group names instead of uid/gid
    compression.cpp                             # compress/decompress data
    dbutils.cpp                                 # utilities to help convert coded table and row names and column data. (see snap_tables.cpp too!)
    file_content.cpp                            # load / save whole files in one go
    floats.cpp                                  # Floats helper functions
    fuzzy_string_compare.cpp                    # Compare strings in a fuzzy manner
    glob_dir.cpp                                # Enumerate the contents of a directory using a wildcard
    http_client_server.cpp                      # implementation of the HTTP protocol
    http_cookie.cpp                             # HTTP cookie, server side
    http_link.cpp                               # HTTP link, server side (HTTP header)
    http_strings.cpp                            # parse HTTP strings (Accept strings in particular)
    inet_helpers.cpp                            # helper functions in link with the network
    loadavg.cpp                                 # handle the loadavg file
    log.cpp                                     # log4cplus support
    mail_exchanger.cpp                          # retrieve the MX record for a given domain
    mkdir_p.cpp                                 # make directories so path is valid
    mkgmtime.c                                  # mktime() with a UTC time
    mounts.cpp                                  # read /proc/mounts or some other mount point file
    password.cpp                                # generate hashed password of various types
    plugins.cpp                                 # plugins loader and list
    process.cpp                                 # run a process optionally with pipes
    qdomhelpers.cpp                             # Qt extensions to help with the DOM
    qdomnodemodel.cpp                           # Qt extension to support XSLT
    qdomreceiver.cpp                            # Qt extension to output XSLT as XML
    qdomxpath.cpp                               # Qt extension to query a XML DOM with X-Path
    qhtmlserializer.cpp                         # Qt extension to output HTML as a string
    quoted_printable.cpp                        # to encode / decode data using the quoted printable format
    qxmlmessagehandler.cpp                      # Qt extension for us to capture the QXmlQuery messages
    snap_backend.cpp                            # type of snap_child for backends
    snap_cassandra.cpp                          # encapuslate initiating a connection to the database.
    snap_communicator.cpp                       # handle any number of sockets with a single process.
    snap_config.cpp                             # class to read in configuration
    snap_child.cpp                              # requests processing
    snap_child_cache_control.cpp                # manage cache controls in the snap_child class
    snap_exception.cpp                          # exception stack backtrack
    snap_expr.cpp                               # snap C-like expression compiler and execution environment
    snap_image.cpp                              # image handling
    snap_initialize_website.cpp                 # initialize website by sending an Apache-like query to the server
    snap_lock.cpp                               # support for inter-process / inter-computer locks using network and memory only
    snap_magic.cpp                              # support for MIME type
    snap_parser.cpp                             # lexer/parser for all our script like entries
    snap_pipe.cpp                               # RAII popen()/pclose() class
    snap_tables.cpp                             # table schema/definitions
    snap_thread.cpp                             # a C++ thread implementation
    snap_uri.cpp                                # URI handling
    snap_utf8.cpp                               # UTF-8 functions
    snap_version.cpp                            # extract name, version, browser info from a filename
    snapwebsites.cpp                            # server
    tcp_client_server.cpp                       # low level TCP/IP that works
    udp_client_server.cpp                       # low level UDP/IP that works
    xslt.cpp                                    # code to do XSLT transformations
)

target_link_libraries( ${PROJECT_NAME}
    ${ADVGETOPT_LIBRARIES}
    ${CASSVALUE_LIBRARIES}
    ${LIBEXCEPT_LIBRARIES}
    ${LOG4CPLUS_LIBRARIES}
    ${LIBTLD_LIBRARIES}
    ${QT_LIBRARIES}
    ${AS2JS_LIBRARIES}
    ${QTCASSANDRA_LIBRARIES}
    ${QTSERIALIZATION_LIBRARIES}
    ${MAGIC_LIBRARIES}
    ${LIBPROCPS_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    ${UUID}
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
install(
    FILES "core-tables.xml"
    DESTINATION ${SNAP_TABLE_DESCRIPTION_PATH}
)
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/snapwebsites
    FILES_MATCHING PATTERN "*.h"
)
install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/version.h
    DESTINATION include/snapwebsites
)

# vim: ts=4 sw=4 et nocindent
