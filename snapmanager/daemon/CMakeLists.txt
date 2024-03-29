# Copyright (c) 2016-2019  Made to Order Software Corp.  All Rights Reserved
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
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

## TODO: once we get rid of the GUI version (if we do so some day) then
##       we can rename this project snapmanager instead of snapmanagerdaemon
project( snapmanagerdaemon )

include_directories(
    SYSTEM
        ${Qt5Core_INCLUDE_DIRS}
        ${Qt5Network_INCLUDE_DIRS}
        ${Qt5Xml_INCLUDE_DIRS}
)

include_directories(
    ${PROJECT_BINARY_DIR}
    ${LIBTLD_INCLUDE_DIRS}
    ${LOG4CPLUS_INCLUDE_DIR}
    ${QT_INCLUDES}
)

add_executable( ${PROJECT_NAME}
    bundle_loader.cpp
    main.cpp
    manager_interrupt.cpp
    manager_messenger.cpp
    manager_status.cpp
    snapmanagerdaemon.cpp
    status.cpp
    status_connection.cpp
)

target_link_libraries( ${PROJECT_NAME}
    snapmanager
    snapwebsites
    libdbproxy
    ${ADVGETOPT_LIBRARIES}
    ${LOG4CPLUS_LIBRARY}
    ${AS2JS_LIBRARIES}
    ${QT_LIBRARIES}
    ${QTSERIALIZATION_LIBRARIES}
    ${LIBTLD_LIBRARIES}
    dl
)

install(
    TARGETS
        ${PROJECT_NAME}

    RUNTIME DESTINATION
        sbin
)


# vim: ts=4 sw=4 et nocindent
