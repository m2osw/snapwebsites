// Snap Websites Server -- some basic UTF-8 handling
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

#include "snapwebsites/snap_utf8.h"

#include "snapwebsites/poison.h"


namespace snap
{

/** \brief Validate a string as ASCII characters.
 *
 * This function checks that all the characters in a string are comprised
 * only of ACSII characters (code bytes 0x00 to 0x7F).
 *
 * We may later constrain this range more to also prevent control
 * characters.
 *
 * \note
 * This function is used to validate headers from a POST because those
 * just cannot include characters other than ASCII. Actually, most
 * controls are also forbidden.
 *
 * \param[in] string  The string to be validated.
 *
 * \return true if the string is empty, NULL, or only includes ASCII characters
 */
bool is_valid_ascii(char const *string)
{
    if(string != nullptr)
    {
        for(unsigned char const *s(reinterpret_cast<unsigned char const *>(string)); *s != '\0'; ++s)
        {
            if(*s >= 0x80)
            {
                return false;
            }
        }
    }

    return true;
}


/** \brief Check whether a string is valid UTF-8 or not.
 *
 * This function can be used to verify that an input string is valid
 * UTF-8. The function checks each byte and if the byte is valid in
 * a UTF-8 stream it returns true, otherwise it returns false.
 *
 * \note
 * This test is done on data received from clients to make sure that
 * the form data encoding was respected. We only support UTF-8 forms
 * so any client that does not is pretty much limited to sending
 * ASCII characters...
 *
 * Source: http://stackoverflow.com/questions/1031645/how-to-detect-utf-8-in-plain-c
 * Source: http://www.w3.org/International/questions/qa-forms-utf-8
 *
 * \note
 * The test ensures proper encoding of UTF-8 in the range 0 to
 * 0x10FFFF and also that UTF-16 surrogate aren't used as characters
 * (i.e. code points 0xD800 to 0xDFFF). No other code points are considered
 * invalid (i.e. 0xFFFE is not a valid character, but this function does
 * not return false when it finds such.)
 *
 * The Perl expression:
 *
 * \code
 * $field =~
 *   m/\A(
 *      [\x09\x0A\x0D\x20-\x7E]            # ASCII
 *    | [\xC2-\xDF][\x80-\xBF]             # non-overlong 2-byte
 *    |  \xE0[\xA0-\xBF][\x80-\xBF]        # excluding overlongs
 *    | [\xE1-\xEC\xEE\xEF][\x80-\xBF]{2}  # straight 3-byte
 *    |  \xED[\x80-\x9F][\x80-\xBF]        # excluding surrogates
 *    |  \xF0[\x90-\xBF][\x80-\xBF]{2}     # planes 1-3
 *    | [\xF1-\xF3][\x80-\xBF]{3}          # planes 4-15
 *    |  \xF4[\x80-\x8F][\x80-\xBF]{2}     # plane 16
 *   )*\z/x;
 * \endcode
 *
 * \warning
 * Remember that QString already handles UTF-8. However, it keeps the
 * characters as UTF-16 characters in its buffers. This means asking
 * for the UTF-8 representation of a QString should always be considered
 * valid UTF-8 (although some surrogates, etc. may be wrong!)
 *
 * \param[in] string  The NUL terminated string to scan.
 *
 * \return true if the string is valid UTF-8
 */
bool is_valid_utf8(char const *string)
{
    if(string == nullptr)
    {
        // empty strings are considered valid
        return true;
    }

    // use unsigned characters so it works even if char is signed
    unsigned char const *s(reinterpret_cast<unsigned char const *>(string));
    while(*s != '\0')
    {
        if(s[0] <= 0x7F)
        {
            ++s;
        }
        else if(s[0] >= 0xC2 && s[0] <= 0xDF // non-overlong 2-byte
             && s[1] >= 0x80 && s[1] <= 0xBF)
        {
            s += 2;
        }
        else if(s[0] == 0xE0 // excluding overlongs
             && s[1] >= 0xA0 && s[1] <= 0xBF
             && s[2] >= 0x80 && s[2] <= 0xBF)
        {
            s += 3;
        }
        else if(((0xE1 <= s[0] && s[0] <= 0xEC) || s[0] == 0xEE || s[0] == 0xEF) // straight 3-byte
             && s[1] >= 0x80 && s[1] <= 0xBF
             && s[2] >= 0x80 && s[2] <= 0xBF)
        {
            s += 3;
        }
        else if(s[0] == 0xED // excluding surrogates
             && s[1] >= 0x80 && s[1] <= 0x9F
             && s[2] >= 0x80 && s[2] <= 0xBF)
        {
            s += 3;
        }
        else if(s[0] == 0xF0 // planes 1-3
             && s[1] >= 0x90 && s[1] <= 0xBF
             && s[2] >= 0x80 && s[2] <= 0xBF
             && s[3] >= 0x80 && s[3] <= 0xBF)
        {
            s += 4;
        }
        else if(s[0] >= 0xF1 && s[0] <= 0xF3 // planes 4-15
             && s[1] >= 0x80 && s[1] <= 0xBF
             && s[2] >= 0x80 && s[2] <= 0xBF
             && s[3] >= 0x80 && s[3] <= 0xBF)
        {
            s += 4;
        }
        else if(s[0] == 0xF4 // plane 16
             && s[1] >= 0x80 && s[1] <= 0x8F
             && s[2] >= 0x80 && s[2] <= 0xBF
             && s[3] >= 0x80 && s[3] <= 0xBF)
        {
            s += 4;
        }
        else
        {
            // not a supported character
            return false;
        }
    }

    return true;
}


} // namespace snap

// vim: ts=4 sw=4 et
