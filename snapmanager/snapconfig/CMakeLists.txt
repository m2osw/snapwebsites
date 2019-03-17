#
# File:
#      snapmanager/snapconfig/CMakeLists.txt
#
# Description:
#      Build the snapconfig command line tool used to lookup and edit snap
#      configuration files from shell scripts.
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



################################################################################
## Create the snapconfig tool
##
project( snapconfig )

include_directories(
    ${libsnapwebsites_project_SOURCE_DIR}/src
    ${libsnapwebsites_project_BINARY_DIR}/src
    ${ADVGETOPT_INCLUDE_DIRS}
)

add_executable(${PROJECT_NAME}
    snapconfig.cpp
)

target_link_libraries(${PROJECT_NAME}
    snapmanager
    snapwebsites
    libdbproxy
    ${ADVGETOPT_LIBRARIES}
    ${AS2JS_LIBRARIES}
    ${CASSVALUE_LIBRARIES}
    ${LOG4CPLUS_LIBRARIES}
    ${QTSERIALIZATION_LIBRARIES}
    ${QT_LIBRARIES}
    ${LIBTLD_LIBRARIES}
    dl
)

install(
    TARGETS
        ${PROJECT_NAME}

    RUNTIME DESTINATION
        bin
)


# vim: ts=4 sw=4 et
