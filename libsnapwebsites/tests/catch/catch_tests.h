#ifndef LIBSNAPWEBSITES_HTML_TESTS_H
#define LIBSNAPWEBSITES_HTML_TESTS_H
// libsnapwebsites html -- Test Suite
// Copyright (C) 2015-2017  Made to Order Software Corp.
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

/** \file
 * \brief Common header for all our catch tests.
 *
 * libsnapwebsites comes with this unit test suite to verify various XML,
 * HTML, and HTTP classes and functions, mainly make sure the strings are
 * properly handled.
 */

#include <catch.hpp>

// include that most tests are likely to use
//
#include <snapwebsites/qstring_stream.h>
#include <iostream>


namespace libsnapwebsites_test
{

extern char * g_progname;

/** \brief The default epsilon for the nearly_equal() test.
 *
 * This function returns a default epsilon (1e-5) for the nearly_equal()
 * function.
 *
 * \note
 * You should not change the default to make your test pass. Instead,
 * specific epsilon in your nearly_equal() funtion call as the third
 * parameter.
 *
 * \return A default epsilon.
 */
template<typename T>
T default_epsilon()
{
    return static_cast<T>(0.00001);
}

/** \brief Check that two floating point values are nearly equal.
 *
 * This function returns true if two floating points are considered
 * nearly equal.
 *
 * \param[in] lhs  The left hand side float.
 * \param[in] rhs  The right hand side float.
 * \param[in] epsilon  The allowed error margin.
 *
 * \return true if both numbers are considered equal.
 */
template<typename T>
bool nearly_equal(T const & lhs, T const & rhs, T epsilon = default_epsilon<T>())
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
    // already equal?
    //
    if(lhs == rhs)
    {
        return true;
    }

    T const diff(fabs(lhs - rhs));
    if(lhs == 0.0
    || rhs == 0.0
    || lhs + rhs == 0.0
    || diff < std::numeric_limits<T>::min())
    {
        return diff < (epsilon * std::numeric_limits<T>::min());
    }
#pragma GCC diagnostic pop

    return diff / (fabs(lhs) + fabs(rhs)) < epsilon;
}


} // libsnapwebsites_test namespace
#endif
// #ifndef LIBSNAPWEBSITES_HTML_TESTS_H

// vim: ts=4 sw=4 et
