#
# File:
#      src/CMakeLists.txt
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
#      Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#

##
project(cassview)

include_directories(
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_BINARY_DIR}"
)

qt5_wrap_ui(UI_FILES
    AboutDialog.ui
    InputDialog.ui
    MainWindow.ui
    SettingsDialog.ui
)

qt5_add_resources(RES_FILES
    resources.qrc
)

add_executable(${PROJECT_NAME}
    resources.qrc
    AboutDialog.cpp
    CassandraModel.cpp
    KeyspaceModel.cpp
    DisplayException.h
    DisplayException.cpp
    InputDialog.cpp
    MainWindow.cpp
    main.cpp
    RowModel.cpp
    SettingsDialog.cpp
    TableModel.cpp
    ${UI_FILES}
    ${RES_FILES}
)

# while developing you may need to set LD_LIBRARY_PATH
target_link_libraries(${PROJECT_NAME}
    snapwebsites
    QtCassandra
    ${ADVGETOPT_LIBRARIES}
    ${CASSWRAPPER_LIBRARIES}
    ${LIBEXCEPT_LIBRARY}
    ${LIBTLD_LIBRARIES}
    ${LOG4CPLUS_LIBRARY}
    ${QT_LIBRARIES}
    ${QTSERIALIZATION_LIBRARIES}
)

install(
    TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin
)

install(
    FILES snap-medium-icon.png
    DESTINATION share/pixmaps
    RENAME cassview.png
)

# vim: ts=4 sw=4 et
