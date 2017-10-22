#
# File:
#      src/CMakeLists.txt
#
# Description:
#      The Snap! Watchdog Server and tools.
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

#################################################################################
## Dependencies
##


#################################################################################
## The Snap! Watchdog
##
project(snapwatchdogserver)

include_directories(
    SYSTEM
        ${QT_INCLUDES}
)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${snapwatchdog_SOURCE_DIR}
)

add_executable(${PROJECT_NAME}
    snapwatchdogserver.cpp
)

# while developing you may need to set LD_LIBRARY_PATH
target_link_libraries(${PROJECT_NAME}
    snapwatchdog
    snapwebsites
    libdbproxy
    ${ADVGETOPT_LIBRARIES}
    ${LOG4CPLUS_LIBRARY}
    ${AS2JS_LIBRARIES}
    ${QTSERIALIZATION_LIBRARIES}
    ${LIBTLD_LIBRARIES}
    #dl
)

install(
    TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin
)




# vim: ts=4 sw=4 et
