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

cmake_minimum_required(VERSION 3.10.2)

project(snapbase_project)

##
## Version
##
#SnapGetVersion( SNAPBASE ${CMAKE_CURRENT_SOURCE_DIR} )

set( SNAPBASE_VERSION_MAJOR ${SNAPWEBSITES_VERSION_MAJOR} )
set( SNAPBASE_VERSION_MINOR ${SNAPWEBSITES_VERSION_MINOR} )
set( SNAPBASE_VERSION_PATCH ${SNAPWEBSITES_VERSION_PATCH} )

add_subdirectory( conf    )
add_subdirectory( doc     )
add_subdirectory( dtd     )
add_subdirectory( src     )
add_subdirectory( scripts )

# vim: ts=4 sw=4 et
