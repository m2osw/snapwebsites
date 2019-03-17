#
# File:
#      snapwatchdog/tools/CMakeLists.txt
#
# Description:
#      Various shell tools used with the watchscript plugin offered by
#      the snapwatchdog.
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


#######################################
## find_process
#######################################

project(find_process)

add_executable(${PROJECT_NAME}
    find_process.cpp
)

target_link_libraries(${PROJECT_NAME}
    snapwebsites
    libdbproxy
    ${ADVGETOPT_LIBRARIES}
    ${CASSWRAPPER_LIBRARIES}
    ${LOG4CPLUS_LIBRARIES}
    ${QT_LIBRARIES}
    ${LIBTLD_LIBRARIES}
)

install(
    TARGETS
        ${PROJECT_NAME}

    RUNTIME DESTINATION
        bin
)




# vim: ts=4 sw=4 et
