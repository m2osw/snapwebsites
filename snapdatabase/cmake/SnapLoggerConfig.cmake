# Copyright (c) 2013-2019  Made to Order Software Corp.  All Rights Reserved
#
# https://snapwebsites.org/project/snaplogger
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
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#
# - Try to find SnapLogger
#
# Once done this will define
#
# SNAPLOGGER_FOUND        - System has SnapLogger
# SNAPLOGGER_INCLUDE_DIRS - The SnapLogger include directories
# SNAPLOGGER_LIBRARIES    - The libraries needed to use SnapLogger (none)
# SNAPLOGGER_DEFINITIONS  - Compiler switches required for using SnapLogger (none)
#

find_path(
    SNAPLOGGER_INCLUDE_DIR
        snaplogger/logger.h

    PATHS
        $ENV{SNAPLOGGER_INCLUDE_DIR}
)

find_library(
    SNAPLOGGER_LIBRARY
        snaplogger

    PATHS
        $ENV{SNAPLOGGER_LIBRARY}
)

mark_as_advanced(
    SNAPLOGGER_INCLUDE_DIR
    SNAPLOGGER_LIBRARY
)

set(SNAPLOGGER_INCLUDE_DIRS ${SNAPLOGGER_INCLUDE_DIR})
set(SNAPLOGGER_LIBRARIES    ${SNAPLOGGER_LIBRARY}    )

include(FindPackageHandleStandardArgs)

# handle the QUIETLY and REQUIRED arguments and set SNAPLOGGER_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(
    SnapLogger
    DEFAULT_MSG
    SNAPLOGGER_INCLUDE_DIR
    SNAPLOGGER_LIBRARY
)

# vim: ts=4 sw=4 et
