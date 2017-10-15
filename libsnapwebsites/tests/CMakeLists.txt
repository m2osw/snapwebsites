#
# File:
#      snapwebsites/libsnapwebsites/tests/CMakeLists.txt
#
# Description:
#      Includes various types of tests.
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

# The "catch" folder has all sorts of unit tests that do not require scripts,
# multiple binaries, network connections, etc.
#
# Those should have no side effects between each others and thus can all be
# placed together within one large binary.
#
add_subdirectory(catch)

# The other tests require multiple binaries, network connections, things that
# tweak the runtime environment in such a way that it could mess up other
# tests, etc.
#
add_subdirectory(cs)	# Client/Server related
add_subdirectory(ip)	# IP address/ports related


# By default "os" tests are turned off. These are used to test an OS feature
# before writing an actual implementation. Most of these can generally be
# considered throw away. Plus, they do not test the code in the library.
#
#add_subdirectory(os)


