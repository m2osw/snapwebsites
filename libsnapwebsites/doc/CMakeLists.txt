#
# File:
#      CMakeLists.txt
#
# Description:
#      Create the documentation of the entire Snap! Websites environment.
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


##
## Documentation
##
AddDoxygenTarget(
    libsnapwebsites
    ${SNAPWEBSITES_VERSION_MAJOR}
    ${SNAPWEBSITES_VERSION_MINOR}
    ${SNAPWEBSITES_VERSION_PATCH}
)

install(
    FILES
        Cassandra-node-distribution.odt
        content-data-details.png
        content-data.odg
        content-data.png
        Ecma-262.pdf
        footer.html
        install-snap.sh
        js-editor-650x650.png
        js-editor.odg
        js-editor.png
        layout-determination.odg
        layout-determination.png
        permission-formula.odf
        permission-hierarchy-example.odg
        permission-hierarchy-example.png
        Permissions-Computation.odg
        Permissions-Computation.pdf
        Permissions-Computation.png
        plan_interference_e_language.pdf
        Registration-Login-Process.odg
        Registration-Login-Process.svg
        Session-Random-Number-Hacked.png
        Session-Random-Number.odg
        Session-Random-Number.png
        snapwebsites-cluster-setup.odg
        snapwebsites-cluster-setup.png
    DESTINATION
        share/doc/snapwebsites/libsnapwebsites
)

# vim: ts=4 sw=4 et nocindent
