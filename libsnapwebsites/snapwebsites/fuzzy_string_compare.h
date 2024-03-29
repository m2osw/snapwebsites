// Snap Websites Servers -- Fuzzy String Comparisons
// Copyright (c) 2011-2019  Made to Order Software Corp.  All Rights Reserved
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

// libexcept
//
#include    <libexcept/exception.h>


// C++
//
#include    <string>



namespace snap
{

DECLARE_MAIN_EXCEPTION(fuzzy_string_compare_exception);

DECLARE_EXCEPTION(fuzzy_string_compare_exception, fuzzy_string_compare_exception_invalid_parameters);

int levenshtein_distance(std::wstring const & s, std::wstring const & t);
bool strstr_with_levenshtein_distance(std::wstring const & haystack, std::wstring const & needle, int const distance);

} // namespace snap
// vim: ts=4 sw=4 et
