// Snap Websites Servers -- replace needles in string
// Copyright (C) 2016-2017  Made to Order Software Corp.
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

#include <string>
#include <type_traits>

namespace snap
{

/** \brief Search needles in input string and replace with replacement strings.
 *
 * This function takes two parameters: a string and a vector of string pairs
 * representing a needle (first) and a replacement (second).
 *
 * The algorithm checks each needle at the current position, starting at
 * position 0. The first match get replaced. What was replaced is not
 * checked any further (it is part of the output.)
 *
 * Therefore, if you have two needles one after another such as: "car" and
 * then "carpool", the second one will never match since whatever get
 * replaced does not participate in the next match and whenever a word
 * starts with "car" it matches the first pair and never has a chance
 * to hit the second pair. In other words, make sure you needles are in
 * the correct order (i.e. probably longest first.)
 *
 * \todo
 * Look into whether we can find a way to find all the possible replacement
 * in order to compute the output string without having to do many
 * reallocations.
 *
 * \todo
 * Add another version which compares case insensitively.
 *
 * \param[in] input  The input string where replacements will occur.
 * \param[in] search  The search parameters, the vector of needles/replacement strings.
 *
 * \return A new string with the search applied to the input string.
 */
template < class StringT >
StringT string_replace_many(StringT const & input
                   , std::vector<std::pair<typename std::decay<StringT>::type, typename std::decay<StringT>::type> > search)
{
    typename StringT::size_type pos(0);
    typename StringT::size_type len(input.length());
    StringT result;

    while(pos < len)
    {
        auto const & match(std::find_if(
                search.begin(),
                search.end(),
                [input, pos](auto const & nr)
                {
                    // check whether we still have enough characters first
                    // and if so, compare against the input string for equality
                    //
                    if(input.length() - pos >= nr.first.length()
                    && input.compare(pos, nr.first.length(), nr.first) == 0)
                    {
                        // we found a match so return true
                        //
                        return true;
                    }
                    return false;
                }));

        if(match == search.end())
        {
            // no match found, copy the character as is
            //
            result += input[pos];
            ++pos;
        }
        else
        {
            // got a replacement, use it and then skip the matched
            // characters in the input string
            //
            result += match->second;
            pos += match->first.length();
        }
    }

    return result;
}

} // namespace snap
// vim: ts=4 sw=4 et
