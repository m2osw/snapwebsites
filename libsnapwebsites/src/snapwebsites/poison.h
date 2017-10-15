// Snap Websites Server -- advanced handling of Unix processes
// Copyright (C) 2013-2017  Made to Order Software Corp.
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
#pragma once

// list of functions we 100% forbid in the Snap! C++ environment

// strcat() is really bad, use QString or std::string
// also forget about strcpy()
#pragma GCC poison strcat strncat wcscat wcsncat strcpy

// printf() is generally okay, but std::cerr/cout, sstream are safer
#pragma GCC poison printf   fprintf   sprintf   snprintf \
                   vprintf  vfprintf  vsprintf  vsnprintf \
                   wprintf  fwprintf  swprintf  snwprintf \
                   vwprintf vfwprintf vswprintf vswnprintf

// vim: ts=4 sw=4 et
