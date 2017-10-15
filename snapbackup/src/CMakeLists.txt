#
# File:
#      snapbackup/src/CMakeLists.txt
#
# Description:
#      The Snap! Websites database tool.
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
## Snap backup utility
##
project(snapbackup)

set( SNAPBACKUP_VERSION_STRING "${SNAPWEBSITES_VERSION_MAJOR}.${SNAPWEBSITES_VERSION_MINOR}.${SNAPWEBSITES_VERSION_PATCH}" )
add_definitions( -DSNAPBACKUP_VERSION_STRING="${SNAPWEBSITES_VERSION_STRING}" )

add_executable(${PROJECT_NAME}
    snap_table_list.cpp
    snap_table_list.h
    snapbackup.cpp
    snapbackup.h
    main.cpp
)

target_link_libraries(${PROJECT_NAME}
    ${ADVGETOPT_LIBRARIES}
    ${QT_LIBRARIES}
    ${CASSWRAPPER_LIBRARIES}
)

install(
    TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)


# vim: ts=4 sw=4 et
