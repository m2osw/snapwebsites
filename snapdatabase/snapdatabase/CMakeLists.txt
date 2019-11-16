# Copyright (c) 2013-2019 Made to Order Software Corp.
#
# https://snapwebsites.org/project/snaplogger
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
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA


##
## snaplogger library
##
project(snaplogger)

# Put the version in the header file
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/version.h
)

add_library(${PROJECT_NAME} SHARED
    add128.s
    add256.s
    add512.s
    sub128.s
    sub256.s
    sub512.s

    bigint.cpp
    block_blob.cpp
    block.cpp
    block_data.cpp
    block_entry_index.cpp
    block_free_block.cpp
    block_free_space.cpp
    block_index_pointers.cpp
    block_indirect_index.cpp
    block_schema.cpp
    block_secondary_index.cpp
    block_top_index.cpp
    cell.cpp
    context.cpp
    convert.cpp
    dbfile.cpp
    error.cpp
    file_bloom_filter.cpp
    file_external_index.cpp
    file_snap_database_table.cpp
    hash.cpp
    row.cpp
    schema.cpp
    script.cpp
    structure.cpp
    table.cpp
    version.cpp
    virtual_buffer.cpp
    xml.cpp
)

include_directories(
    ${ADVGETOPT_INCLUDE_DIRS}
    ${CPPTHREAD_INCLUDE_DIRS}
    ${LIBEXCEPT_INCLUDE_DIRS}
    ${LIBUFTF8_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}
    ${ADVGETOPT_LIBRARIES}
    ${CPPTHREAD_LIBRARIES}
    ${LIBEXCEPT_LIBRARIES}
    ${LIBUFTF8_LIBRARIES}
    ${LOG4CPLUS_LIBRARIES}
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    VERSION
        ${SNAPDATABASE_VERSION_MAJOR}.${SNAPDATABASE_VERSION_MINOR}

    SOVERSION
        ${SNAPDATABASE_VERSION_MAJOR}
)


set(QT_INCLUDE_PROPERTIES "")
foreach(DIR ${Qt5Core_INCLUDE_DIRS})
    set(QT_INCLUDE_PROPERTIES "${QT_INCLUDE_PROPERTIES} -isystem ${DIR}")
endforeach()
set_source_files_properties(script.cpp PROPERTIES
    COMPILE_FLAGS
        ${QT_INCLUDE_PROPERTIES}
)

#${Qt5Network_INCLUDE_DIRS}
#${Qt5Script_INCLUDE_DIRS}
#${Qt5Sql_INCLUDE_DIRS}
#${Qt5Widgets_INCLUDE_DIRS}
#${Qt5Xml_INCLUDE_DIRS}
#${Qt5XmlPatterns_INCLUDE_DIRS}

install(
    TARGETS
        ${PROJECT_NAME}

    RUNTIME DESTINATION
        bin

    LIBRARY DESTINATION
        lib

    ARCHIVE DESTINATION
        lib
)

install(
    FILES
        bigint.h
        block_blob.h
        block_data.h
        block_entry_index.h
        block_free_block.h
        block_free_space.h
        block.h
        block_index_pointers.h
        block_indirect_index.h
        block_schema.h
        block_secondary_index.h
        block_top_index.h
        cell.h
        consistency.h
        context.h
        convert.h
        dbfile.h
        dbtype.h
        error.h
        exception.h
        file_bloom_filter.h
        file_external_index.h
        file_snap_database_table.h
        hash.h
        row.h
        schema.h
        structure.h
        table.h
        ${CMAKE_CURRENT_BINARY_DIR}/version.h
        virtual_buffer.h
        xml.h

    DESTINATION
        include/snapdatabase
)


# vim: ts=4 sw=4 et
