# Copyright (c) 2011-2019  Made to Order Software Corp.  All Rights Reserved
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

set( CASSANDRA_HOST_IP "127.0.0.1" CACHE STRING "Host IP address for the cassandra server to connect to." )

##
## The unified unit tests in one large test
##

find_package(SnapCatch2)

if(SnapCatch2_FOUND)

    ##
    ## Send orders to the Snap Server via the snap.cgi tool
    ##
    project(test_snap_cgi)
    add_executable(test_snap_cgi
        test_snap_cgi.cpp
    )
    # while developing you may need to set LD_LIBRARY_PATH
    target_include_directories(${PROJECT_NAME}
        PUBLIC
            ${SNAPDEV_INCLUDE_DIRS}
    )
    target_link_libraries(test_snap_cgi
        ${LIBEXCEPT_LIBRARIES}
    )
    add_test(
        NAME
            ${PROJECT_NAME}

        COMMAND
            ${PROJECT_NAME}
    )

else(SnapCatch2_FOUND)

    # You may use 'apt-get install catch' under Ubuntu
    message("Catch based tests will not be created because you do not seem to have catch.hpp installed...")

endif(SnapCatch2_FOUND)



# vim: ts=4 sw=4 et
