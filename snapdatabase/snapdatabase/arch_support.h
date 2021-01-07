// Copyright (c) 2019-2020  Made to Order Software Corp.  All Rights Reserved
//
// https://snapwebsites.org/project/snapdatabase
// contact@m2osw.com
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
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#pragma once


/** \file
 * \brief Functions/variables that are not available in all distros.
 *
 * We use some newer functions and/or variables that would have worked
 * in older distros, only they were not yet available. This is just to
 * avoid having to redefine these everywhere.
 */


// C++ lib
//
#include    <cstdint>


// various functions we need to support all distros
#if __GNUC__ <= 5
namespace std
{

template <class T, size_t N>
constexpr size_t size(const T (&)[N]) noexcept
{
    return N;
}

}
// std namespace to get a size() that works on Ubuntu 16.04
#endif


// vim: ts=4 sw=4 et
