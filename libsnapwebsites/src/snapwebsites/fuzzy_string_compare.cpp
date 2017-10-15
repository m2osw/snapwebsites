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

#include "snapwebsites/fuzzy_string_compare.h"

#include <vector>

#include "snapwebsites/poison.h"


namespace snap
{

/** \brief Computes the Levenshtein distance between two strings.
 *
 * This function calculates the Levenshtein distance between two strings
 * using the fastest algorithm, assuming that allocating memory is fast.
 *
 * The strings are expected to be UTF-32, although under a system like
 * MS-Windows a wstring uses UTF-16 instead...
 *
 * \note
 * This algorithm comes from Wikipedia:
 * https://en.wikipedia.org/wiki/Levenshtein_distance
 *
 * \important
 * The function does not change the case of the string. If you want
 * to compare case insensitive, make sure to convert the string to
 * lowercase first. You may also want to simplify the string (i.e.
 * remove additional white spaces, or even remove all white spaces.)
 *
 * \param[in] s  The left hand side string.
 * \param[in] t  The right hand side string.
 *
 * \return The Levenshtein distance between \p s and \p t.
 */
int levenshtein_distance(std::wstring const & s, std::wstring const & t)
{
    // degenerate cases
    if(s == t)
    {
        return 0; // exactly equal distance is zero
    }
    if(s.empty())
    {
        return t.length();
    }
    if(t.empty())
    {
        return s.length();
    }
 
    // create two work vectors of integer distances
    std::vector<int> v0(t.length() + 1);
    std::vector<int> v1(v0.size());
 
    // initialize v0 (the previous row of distances)
    // this row is A[0][i]: edit distance for an empty s
    // the distance is just the number of characters to delete from t
    for(size_t i(0); i < v0.size(); ++i)
    {
        v0[i] = i;
    }
 
    for(size_t i(0); i < s.length(); ++i)
    {
        // calculate v1 (current row distances) from the previous row v0
 
        // first element of v1 is A[i+1][0]
        //   edit distance is delete (i+1) chars from s to match empty t
        v1[0] = i + 1;
 
        // use formula to fill in the rest of the row
        for(size_t j(0); j < t.length(); j++)
        {
            int const cost(s[i] == t[j] ? 0 : 1);
            v1[j + 1] = std::min(v1[j] + 1, std::min(v0[j + 1] + 1, v0[j] + cost));
        }
 
        // copy v1 (current row) to v0 (previous row) for next iteration
        //v0 = v1; -- swap is a lot faster!
        v0.swap(v1);
    }
 
    return v0[t.length()];
}

/** \brief Search a string in another with a given Levenshtein distance.
 *
 * This function searches string \p needle in \p haystack using the
 * specified Levenshtein distance.
 *
 * In other words, the function shortens \p haystack to a length
 * equal to \p needle, then \p needle + 1, \p needle + 2, ...,
 * \p needle + \p distance and check each such shorter version
 * of \p haystack against \p needle. If any of these sub-haystack
 * return a distance smaller or equal to the specified parameter
 * named \p distance, then the function returns the position
 * at which the match was found. The function stops as soon as
 * a match is found, this means it may not find the smallest
 * possible match.
 *
 * \exception
 *
 * \param[in] haystack  The haystack where the \p needle is searched.
 * \param[in] needle  The needle to search in \p haystack.
 * \param[in] distance  The distance which represents a match.
 *
 * \return The position where a match was found, -1 if no matches
 *         were found.
 */
bool strstr_with_levenshtein_distance(std::wstring const & haystack, std::wstring const & needle, int const distance)
{
    if(distance <= 0)
    {
        throw fuzzy_string_compare_exception_invalid_parameters("Levenshtein distance in strstr_with_levenshtein_distance() needs to be > 0");
    }

    size_t const needle_length(needle.length());
    if(needle_length >= haystack.length())
    {
        return levenshtein_distance(haystack, needle) <= distance;
    }

    // haystack is larger than needle, apply our loops
    //
    size_t const haystack_length(haystack.length());
    for(size_t i(needle_length); i < haystack_length; ++i)
    {
        size_t const max_distance(std::min(i + distance, haystack_length) - i);
        for(size_t j(0); j <= max_distance; ++j)
        {
            std::wstring const sub_haystack(haystack.substr(i - needle_length, i + j));
            if(levenshtein_distance(sub_haystack, needle) <= distance)
            {
                return true;
            }
        }
    }

    return false;
}

} // namespace snap
// vim: ts=4 sw=4 et
