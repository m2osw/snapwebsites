// Snap Websites Servers -- tokenize a string to a container
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
#include <algorithm>

namespace snap
{

/** \brief Transform a string in a vector of strings.
 *
 * This function transforms a string to a vector a strings
 * as separated by the specified delimiters.
 *
 * The trim_empty parameter can be used to avoid empty entries,
 * either at the start, middle, or end.
 *
 * \note
 * If the tokens vector is not empty, the items of the string
 * being tokenized will be appended to the existing vector.
 *
 * \param[in,out] tokens  The container receiving the resulting strings.
 * \param[in] str  The string to tokenize.
 * \param[in] delimiters  The list of character delimiters.
 * \param[in] trim_empty  Whether to keep empty entries or not.
 * \param[in] trim_string  Trim those characters from the start/end before saving.
 *
 * \return the number of items in the resulting container.
 */
template < class ContainerT >
size_t tokenize_string(ContainerT & tokens
                     , typename ContainerT::value_type const & str
                     , typename ContainerT::value_type const & delimiters
                     , bool const trim_empty = false
                     , typename ContainerT::value_type const & trim_string = typename ContainerT::value_type())
{
    for(typename ContainerT::value_type::size_type pos(0),
                                                   last_pos(0);
        pos != ContainerT::value_type::npos;
        last_pos = pos + 1)
    {
        pos = str.find_first_of(delimiters, last_pos);

        char const * start(str.data() + last_pos);
        char const * end(start + ((pos == ContainerT::value_type::npos ? str.length() : pos) - last_pos));

        if(start != end                 // if not (already) empty
        && !trim_string.empty())        // and there are characters to trim
        {
            // find first character not in trim_string
            //
            start = std::find_if_not(
                  start
                , end
                , [&trim_string](auto const c)
                  {
                      return trim_string.find(c) != ContainerT::value_type::npos;
                  });

            // find last character not in trim_string
            //
            if(start < end)
            {
                reverse_cstring<typename ContainerT::value_type::value_type const> const rstr(start, end);
                auto const p(std::find_if_not(
                      rstr.begin()
                    , rstr.end()
                    , [&trim_string](auto const c)
                      {
                          return trim_string.find(c) != ContainerT::value_type::npos;
                      }));
                end = p.get();
            }
        }

        if(start != end     // if not empty
        || !trim_empty)     // or user accepts empty
        {
            tokens.push_back(typename ContainerT::value_type(start, end - start));
        }
    }

    return tokens.size();
}

} // namespace snap
// vim: ts=4 sw=4 et
