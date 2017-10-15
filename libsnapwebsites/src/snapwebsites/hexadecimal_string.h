// Snap Websites Servers -- convert string to hexadecimal
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

// snapwebsites lib
//
#include "snapwebsites/snap_exception.h"

// C++ lib
//
#include <algorithm>



namespace snap
{


class string_exception_invalid_parameter : public snap::snap_exception
{
public:
    string_exception_invalid_parameter(char const *        what_msg) : snap_exception(what_msg) {}
    string_exception_invalid_parameter(std::string const & what_msg) : snap_exception(what_msg) {}
    string_exception_invalid_parameter(QString const &     what_msg) : snap_exception(what_msg) {}
};


/** \brief Transform a binary string to hexadecimal.
 *
 * This function transforms a string of binary bytes (any value from 0x00
 * to 0xFF) to a string of hexadecimal digits.
 *
 * The output string will be exactly 2x the size of the input string.
 *
 * \param[in] binary  The input binary string to convert.
 *
 * \return The hexademical representation of the input string.
 */
inline std::string bin_to_hex(std::string const & binary)
{
    if(binary.empty())
    {
        return std::string();
    }

    std::string result;

    result.reserve(binary.length() * 2);

    std::for_each(
              binary.begin()
            , binary.end()
            , [&result](char const & c)
            {
                auto to_hex([](char d)
                {
                    return d < 10 ? d + '0' : d + ('a' - 10);
                });

                result.push_back(to_hex((c >> 4) & 15));
                result.push_back(to_hex(c & 15));
            });

    return result;
}


/** \brief Convert an hexadecimal string to a binary string.
 *
 * This function is the inverse of the bin_to_hex() function. It
 * converts a text string of hexadecimal numbers (exactly 2 digits
 * each) into a binary string (a string of any bytes from 0x00 to
 * 0xFF.)
 *
 * The output will be exactly half the size of the input.
 *
 * \exception string_exception_invalid_parameter
 * If the input string is not considered valid, then this exception is
 * raised. To be valid every single character must be an hexadecimal
 * digit (0-9, a-f, A-F) and the length of the string must be even.
 *
 * \param[in] hex  The salt as an hexadecimal string of characters.
 *
 * \return The converted value in a binary string.
 */
inline std::string hex_to_bin(std::string const & hex)
{
    std::string result;

    if((hex.length() & 1) != 0)
    {
        throw string_exception_invalid_parameter("the hex parameter must have an even size");
    }

    for(char const * s(hex.c_str()); *s != '\0'; s += 2)
    {
        int value(0);

        // first digit
        //
        if(s[0] >= '0' && s[0] <= '9')
        {
            value = (s[0] - '0') * 16;
        }
        else if(s[0] >= 'a' && s[0] <= 'f')
        {
            value = (s[0] - 'a' + 10) * 16;
        }
        else if(s[0] >= 'A' && s[0] <= 'F')
        {
            value = (s[0] - 'A' + 10) * 16;
        }
        else
        {
            throw string_exception_invalid_parameter("the hex parameter must only contain valid hexadecimal digits");
        }

        // second digit
        //
        if(s[1] >= '0' && s[1] <= '9')
        {
            value += s[1] - '0';
        }
        else if(s[1] >= 'a' && s[1] <= 'f')
        {
            value += s[1] - 'a' + 10;
        }
        else if(s[1] >= 'A' && s[1] <= 'F')
        {
            value += s[1] - 'A' + 10;
        }
        else
        {
            throw string_exception_invalid_parameter("the hex parameter must only contain valid hexadecimal digits");
        }

        result.push_back(value);
    }

    return result;
}

} // namespace snap
// vim: ts=4 sw=4 et
