#
# File:
#      snapbounce/src/CMakeLists.txt
#
# Description:
#      The Snap! Websites mail tools.
#
#        * snapbounce is used to gracefully handle emails that bounce
#          by saving them in our database and warning the sender
#
#        * sendmail is a thin SMTP or mail queuing tool (i.e. when emails
#          can't be forwarded to an SMTP server, enqueue them locally.)
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

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/version.h
)


#################################################################################
## Snap! Email Bounce Manager
##
project(snapbounce)

include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CASSVALUE_INCLUDE_DIRS}
    ${QTSERIALIZATION_INCLUDE_DIRS}
)

snap_validate_xml( snapbounce-processes.xml watchdog-processes.xsd )

add_executable(${PROJECT_NAME}
    snapbounce.cpp
)

target_link_libraries(${PROJECT_NAME}
    snapwebsites
    libdbproxy
    ${ADVGETOPT_LIBRARIES}
    ${CASSVALUE_LIBRARIES}
    ${LIBEXCEPT_LIBRARIES}
    ${LIBTLD_LIBRARIES}
    ${LOG4CPLUS_LIBRARIES}
    ${QT_LIBRARIES}
    ${QTSERIALIZATION_LIBRARIES}
    ${UUID}
)

install(
    TARGETS
        ${PROJECT_NAME}
    RUNTIME DESTINATION
        bin
)


#################################################################################
## Snap! sendmail tool to have a form of MTA on all machines
##
## Note: the project is named snapsendmail because we already have a sendmail
##       plugin in our snapserver-core-plugins
##
project(snapsendmail)

include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CASSVALUE_INCLUDE_DIRS}
    ${QTSERIALIZATION_INCLUDE_DIRS}
)

add_executable(${PROJECT_NAME}
    sendmail.cpp
)

set_target_properties(${PROJECT_NAME}
    PROPERTIES 
        OUTPUT_NAME sendmail
)

target_link_libraries(${PROJECT_NAME}
    snapwebsites
    libdbproxy
    ${ADVGETOPT_LIBRARIES}
    ${CASSVALUE_LIBRARIES}
    ${LIBEXCEPT_LIBRARIES}
    ${LIBTLD_LIBRARIES}
    ${LOG4CPLUS_LIBRARIES}
    ${QT_LIBRARIES}
    ${QTSERIALIZATION_LIBRARIES}
    ${UUID}
)

# IMPORTANT: this tool HAS to go in /usr/sbin to be found by CRON and other
#            old tools that checks that directory for sendmail
install(
    TARGETS
        ${PROJECT_NAME}
    RUNTIME DESTINATION
        sbin
)


# vim: ts=4 sw=4 et
