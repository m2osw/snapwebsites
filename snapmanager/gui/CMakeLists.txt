#
# File:
#      snapmanager/CMakeLists.txt
#
# Description:
#      The Snap! Manager to edit data in the Cassandra cluster
#      using a simple Qt graphical interface.
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

# Required packages to build GUI
#
find_package( CassWrapper      REQUIRED )
find_package( Qt5Widgets       REQUIRED )
find_package( Qt5WebKit        REQUIRED )
find_package( Qt5WebKitWidgets REQUIRED )

add_definitions( -DSNAPMANAGER_VERSION=\"${SNAPMANAGERCGI_VERSION_MAJOR}.${SNAPMANAGERCGI_VERSION_MINOR}.${SNAPMANAGERCGI_VERSION_PATCH}\" )

include_directories( SYSTEM
    ${Qt5Widgets_INCLUDE_DIRS}
    ${Qt5WebKit_INCLUDE_DIRS}
    ${Qt5WebKitWidgets_INCLUDE_DIRS}
)

list( APPEND QT_LIBRARIES
    Qt5::Widgets
    Qt5::WebKit
    Qt5::WebKitWidgets
)

##
## The Snap! Manager
##
project( snapmanagergui )

qt5_wrap_ui(SNAP_MANAGER_UIS_H
    snap-manager-mainwindow.ui
    snap-manager-aboutbox.ui
    snap-manager-createcontextbox.ui
    snap-manager-helpbox.ui
    snap-manager-initialize-websitebox.ui
    snap-manager-decode-utf8-dialog.ui
    snap-manager-connectform.ui
    snap-manager-domainform.ui
)
qt5_add_resources(SNAP_MANAGER_RESOURCES_RCC
    snap-manager-resources.qrc
)

add_executable( ${PROJECT_NAME}
    DomainModel.cpp
    RowModel.cpp
    TableModel.cpp
    WebsiteModel.cpp
    snap-manager.cpp
    snap-manager-about.cpp
    snap-manager-createcontext.cpp
    snap-manager-help.cpp
    snap-manager-initialize-website.cpp
    snap-manager-decode-utf8.cpp
    get_child.h
    ${SNAP_MANAGER_UIS_H}
    ${SNAP_MANAGER_RESOURCES_RCC}
)

# while developing you may need to set LD_LIBRARY_PATH
target_link_libraries( ${PROJECT_NAME}
    snapwebsites
    QtCassandra
    ${ADVGETOPT_LIBRARIES}
    ${CASSWRAPPER_LIBRARIES}
    ${LOG4CPLUS_LIBRARY}
    ${QT_LIBRARIES}
    ${QTSERIALIZATION_LIBRARIES}
    ${LIBTLD_LIBRARIES}
    dl
)

install( TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin )
install( FILES snapmanagergui.desktop
    DESTINATION share/applications
)
install( FILES images/snap-medium-icon.png
    DESTINATION share/pixmaps
    RENAME snapmanagergui.png
)

## TODO: add code coverage, doxygen and unittests

# vim: ts=4 sw=4 et nocindent
