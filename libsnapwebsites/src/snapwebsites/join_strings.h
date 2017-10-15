// Snap Websites Servers -- join an array of strings with a separator
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

#include <string>


namespace snap
{

/** \brief Transform a vector string in a string.
 *
 * This function concatenate all the strings from a vector adding a separator
 * in between each. In effect, it does:
 *
 * \code
 *      s1 + sep + s2 + sep + s3...
 * \endcode
 *
 * If you do not need a separator, you can use the std::accumulate() function
 * although it is going to be slower (i.e. it will do a lot of realloc() since
 * it does not know how long the final string is going to be.)
 *
 * \param[in] tokens  The container of strings.
 * \param[in] separator  The separator to add between each string.
 *
 * \return the number of items in the resulting container.
 */
template < class ContainerT >
typename ContainerT::value_type join_strings(ContainerT & tokens
                                            , typename ContainerT::value_type const & separator)
{
    typename ContainerT::value_type result;

    // we have a special case because we want to access the first
    // item (see tokens[0] below) to make the for_each() simpler.
    //
    if(!tokens.empty())
    {
        // calculate the final size, which is way faster than reallocating
        // over and over again in the 'result += string' below
        //
        size_t const total_size(std::accumulate(tokens.begin(), tokens.end(), separator.length() * (tokens.size() - 1),
                                [](size_t & sum, typename ContainerT::value_type const & str)
                                {
                                    return sum + str.length();
                                }));

        result.reserve(total_size);

        // avoid special case in the loop
        // (i.e. no separator before the first token)
        //
        result += tokens[0];

        std::for_each(
                  tokens.begin() + 1
                , tokens.end()
                , [&separator, &result](auto const & s)
                        {
                            result += separator + s;
                        });
    }

    return result;
}

} // namespace snap
// vim: ts=4 sw=4 et
