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

project(snapupgrader)

include_directories(
    ${PROJECT_BINARY_DIR}
    ${LIBTLD_INCLUDE_DIRS}
    ${LOG4CPLUS_INCLUDE_DIR}
    ${QT_INCLUDES}
    ${snapmanager_SOURCE_DIR}
)


add_executable(${PROJECT_NAME}
    snapupgrader.cpp
)

target_link_libraries(${PROJECT_NAME}
    snapmanager
    snapwebsites
    libdbproxy
    ${ADVGETOPT_LIBRARIES}
    ${AS2JS_LIBRARIES}
    ${LIBTLD_LIBRARIES}
    ${LOG4CPLUS_LIBRARIES}
    ${QT_LIBRARIES}
    ${QTSERIALIZATION_LIBRARIES}
)

install(
    TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin
)

# vim: ts=4 sw=4 et
