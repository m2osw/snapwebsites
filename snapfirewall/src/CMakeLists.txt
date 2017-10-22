#
# File:
#      snapwebsites/snapfirewall/src/CMakeLists.txt
#
# Description:
#      The Snap! Websites Server. The server is the main() of the Snap!
#      environment. Most everything is however defined in the
#      libsnapwebsites.so library (see lib/*).
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
## Snap firewall manager
##
project(snapfirewall)

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/version.h.in ${CMAKE_CURRENT_BINARY_DIR}/version.h )

include_directories( ${CMAKE_CURRENT_BINARY_DIR} )

add_executable(${PROJECT_NAME}
    snapfirewall.cpp
)

target_link_libraries(${PROJECT_NAME}
    snapwebsites
    libdbproxy
    ${ADVGETOPT_LIBRARIES}
    ${CASSVALUE_LIBRARIES}
    ${LIBEXCEPT_LIBRARIES}
    ${LOG4CPLUS_LIBRARIES}
    ${LIBTLD_LIBRARIES}
    ${AS2JS_LIBRARIES}
    ${QT_LIBRARIES}
    ${QTSERIALIZATION_LIBRARIES}
)

install(
    TARGETS
        ${PROJECT_NAME}

    RUNTIME DESTINATION
        bin
)

install(
    PROGRAMS
        count-unwanted
        showfw
        unblock-all

    DESTINATION
        bin
)


#################################################################################
## Snap firewall manager
##
project(snapresetfail2ban)

include_directories( ${CMAKE_CURRENT_BINARY_DIR} )

add_executable(${PROJECT_NAME}
    snapresetfail2ban.cpp
)

target_link_libraries(${PROJECT_NAME}
    snapwebsites
    libdbproxy
    ${ADVGETOPT_LIBRARIES}
    ${LOG4CPLUS_LIBRARIES}
    ${LIBTLD_LIBRARIES}
    ${AS2JS_LIBRARIES}
    ${QT_LIBRARIES}
    ${QTSERIALIZATION_LIBRARIES}
)

install(
    TARGETS
        ${PROJECT_NAME}

    RUNTIME DESTINATION
        bin
)




# vim: ts=4 sw=4 et
