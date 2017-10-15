// Snap Websites Servers -- Fuzzy String Comparisons
// Copyright (C) 2011-2017  Made to Order Software Corp.
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

#include "snapwebsites/snap_exception.h"

#include <string>

namespace snap
{

class fuzzy_string_compare_exception : public snap_exception
{
public:
    explicit fuzzy_string_compare_exception(char const *        whatmsg) : snap_exception("snap_child", whatmsg) {}
    explicit fuzzy_string_compare_exception(std::string const & whatmsg) : snap_exception("snap_child", whatmsg) {}
    explicit fuzzy_string_compare_exception(QString const &     whatmsg) : snap_exception("snap_child", whatmsg) {}
};

class fuzzy_string_compare_exception_invalid_parameters : public fuzzy_string_compare_exception
{
public:
    explicit fuzzy_string_compare_exception_invalid_parameters(char const *        whatmsg) : fuzzy_string_compare_exception(whatmsg) {}
    explicit fuzzy_string_compare_exception_invalid_parameters(std::string const & whatmsg) : fuzzy_string_compare_exception(whatmsg) {}
    explicit fuzzy_string_compare_exception_invalid_parameters(QString const &     whatmsg) : fuzzy_string_compare_exception(whatmsg) {}
};

int levenshtein_distance(std::wstring const & s, std::wstring const & t);
bool strstr_with_levenshtein_distance(std::wstring const & haystack, std::wstring const & needle, int const distance);

} // namespace snap
// vim: ts=4 sw=4 et
