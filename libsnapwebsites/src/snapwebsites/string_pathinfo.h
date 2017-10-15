// Snap Websites Servers -- break up a string according to Unix path definitions
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
//
// Based on: http://stackoverflow.com/questions/236129/split-a-string-in-c#1493195
//
#pragma once

#include "snapwebsites/reverse_cstring.h"

#include <string>

namespace snap
{

/** \brief Retrieve the basename of a path.
 *
 * This function retrieves the basename of a path.
 *
 * \param[in] path  The path from which basename gets retrieved.
 * \param[in] extension  If the path ends with that extension, remove it.
 *
 * \return The basename of \p path.
 */
template < class StringT >
StringT string_pathinfo_basename(StringT const & path
                               , typename std::decay<StringT>::type const & suffix = ""
                               , typename std::decay<StringT>::type const & prefix = "")
{
    // ignore path if present
    //
    typename StringT::size_type pos(path.find_last_of('/'));
    if(pos == StringT::npos)
    {
        // if no '/' in string, the entire name is a basename
        //
        pos = 0;
    }
    else
    {
        ++pos;      // skip the actual '/'
    }

    // ignore prefix if present
    //
    if(prefix.length() <= path.length() - pos
    && path.compare(pos, prefix.length(), prefix) == 0)
    {
        pos += prefix.length();
    }

    // if the path ends with suffix, then return the path without it
    //
    if(suffix.length() <= path.length() - pos
    && path.compare(path.length() - suffix.length(), suffix.length(), suffix) == 0)
    {
        return path.substr(pos, path.length() - pos - suffix.length());
    }

    // no suffix in this case
    //
    return path.substr(pos);
}

} // namespace snap
// vim: ts=4 sw=4 et
