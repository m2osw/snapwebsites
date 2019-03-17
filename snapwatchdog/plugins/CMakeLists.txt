################################################################################
# File:
#      snapwatchdog/plugins/CMakeLists.txt
#
# Description:
#      Set of extensions used by the Snap! Watchdog daemon.
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
################################################################################

project( snapwatchdog_plugins )

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Automatically run the moc compiler on qt files
set( CMAKE_AUTOMOC TRUE )

################################################################################
# We need a common place to install the plugins
#
set( PLUGIN_INSTALL_DIR lib/snapwebsites/watchdog_plugins )


################################################################################
# Keep the plugins in alphabetical order for ease of management
#
add_subdirectory(watchdog_apt)
add_subdirectory(watchdog_cassandra)
add_subdirectory(watchdog_cpu)
add_subdirectory(watchdog_disk)
add_subdirectory(watchdog_firewall)
add_subdirectory(watchdog_flags)
add_subdirectory(watchdog_log)
add_subdirectory(watchdog_memory)
add_subdirectory(watchdog_network)
add_subdirectory(watchdog_packages)
add_subdirectory(watchdog_processes)
add_subdirectory(watchdog_watchscripts)


# vim: ts=4 sw=4 nocindent
