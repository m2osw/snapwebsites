#pragma once
// Snap! Websites -- Test Suite Main Header
// Copyright (C) 2015-2017  Made to Order Software Corp.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

/** \file
 * \brief Common header for all our catch tests.
 *
 * Snap! Websites comes with a set of unit tests. All are not yet
 * converted to catch.cpp and pretty much none are full coverage
 * at this point.
 */

// include the version and a few other things
#include <snapwebsites/snapwebsites.h>

#include <catch.hpp>

#include <string>

class snap_test
{
public:
    static std::string progname() { return f_progname; }
    static std::string progdir()  { return f_progdir;  }
    static bool        verbose()  { return f_verbose;  }
    static std::string host()     { return f_host;     }

    static void progname ( const std::string& val ) { f_progname = val; }
    static void progdir  ( const std::string& val ) { f_progdir  = val; }
    static void verbose  ( const bool         val ) { f_host     = val; }
    static void host     ( const std::string& val ) { f_host     = val; }

private:
    static std::string  f_progname;
    static std::string  f_progdir;
    static bool         f_verbose;
    static std::string  f_host;
};

// Local Variables:
// mode: cpp
// indent-tabs-mode: nil
// c-basic-offset: 4
// tab-width: 4
// End:

// vim: ts=4 sw=4 et
