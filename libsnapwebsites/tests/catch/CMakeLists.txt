#
# File:
#      snapcpp/libsnapwebsites/tests/catch/CMakeLists.txt
#
# Description:
#      Build libsnapwebsites catch tests.
#
# Documentation:
#      See the CMake documentation.
#
# License:
#      Copyright (C) 2015-2017  Made to Order Software Corporation
#
#      This library is free software; you can redistribute it and/or
#      modify it under the terms of the GNU Lesser General Public
#      License as published by the Free Software Foundation; either
#      version 2 of the License, or (at your option) any later version.
#
#      This library is distributed in the hope that it will be useful,
#      but WITHOUT ANY WARRANTY; without even the implied warranty of
#      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#      Lesser General Public License for more details.
#
#      You should have received a copy of the GNU Lesser General Public
#      License along with this library; if not, write to the Free Software
#      Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
#

find_package( Catch )

if(CATCH_FOUND)

    project( libsnapwebsites_tests )

    include_directories(
        ${CATCH_INCLUDE_DIR}
    )

    add_executable( ${PROJECT_NAME}
        # main()
        catch_tests.cpp
        catch_tests.h

        # actual tests
        catch_snapuri.cpp
        catch_weightedhttpstrings.cpp
    )

    target_link_libraries( ${PROJECT_NAME}
        ${ADVGETOPT_LIBRARIES}
        ${LIBEXCEPT_LIBRARIES}
        ${LIBTLD_LIBRARIES}
        ${LOG4CPLUS_LIBRARIES}
        ${QT_LIBRARIES}
        ${AS2JS_LIBRARIES}
        ${QTSERIALIZATION_LIBRARIES}
        ${MAGIC_LIBRARIES}
        ${LIBPROCPS_LIBRARIES}
        ${OPENSSL_LIBRARIES}
        ${UUID}
        pthread
        dl
        snapwebsites
        libdbproxy
    )

    # You can use the --success command line option to see all the tests
    # as they run; it is a LOT of output though, thus by default we don't
    # use it
    add_test( libsnapwebsites_tests ${PROJECT_NAME} )

else(CATCH_FOUND)

    # You may use 'apt-get install catch' under Ubuntu
    message("No catch test will be created because you do not seem to have catch.hpp installed...")

endif(CATCH_FOUND)

# vim: ts=4 sw=4 et
