################################################################################
# File:
#      snapwebsites/snapbackup/manager-plugins/CMakeLists.txt
#
# Description:
#      Set of extensions used by the Snap! Manager CGI and daemon.
#
# Documentation:
#      See the CMake documentation.
#
# License:
#      Copyright (c) 2016-2017 Made to Order Software Corp.
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
################################################################################

project( manager_plugins )

# Automatically run the moc compiler on qt files
set( CMAKE_AUTOMOC TRUE )

################################################################################
# We need a common place to install the plugins
#
set( PLUGIN_INSTALL_DIR lib/snapwebsites/manager_plugins )


################################################################################
# Allow for #include of other header files available to plugins
#
#include_directories(
#    #${snapmanager_SOURCE_DIR}
#)

################################################################################
# Keep the plugins in alphabetical order for ease of management
#
add_subdirectory(backup)


# vim: ts=4 sw=4 nocindent et
