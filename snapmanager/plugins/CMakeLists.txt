################################################################################
# File:
#      snapwebsites/snapmanagercgi/plugins/CMakeLists.txt
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

project( snapmanagercgi_plugins )

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Automatically run the moc compiler on qt files
set( CMAKE_AUTOMOC TRUE )

################################################################################
# We need a common place to install the plugins
#
set( PLUGIN_INSTALL_DIR lib/snapwebsites/manager_plugins )

################################################################################
# Keep the plugins in alphabetical order for ease of management
#
add_subdirectory(cassandra)
add_subdirectory(apache2)
add_subdirectory(apt)
add_subdirectory(dns)
add_subdirectory(self)
add_subdirectory(ssh)
#add_subdirectory(system)
add_subdirectory(vpn)


# vim: ts=4 sw=4 nocindent et
