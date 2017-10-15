// Snap Websites Server -- handle the basic display of the website content
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

#include "snapwebsites/floats.h"

#include <cstdint>

#include "snapwebsites/poison.h"


namespace snap
{

/** \brief Compare two floating points together.
 *
 * Comparing two floating point numbers requires some work if you want to
 * make sure that you do get true when the floats are (nearly) equal, then
 * you want to compare the mantissa and not the floats as is.
 *
 * The function compares the signs, unless a and b are +0.0 and -0.0 if the
 * signs are different the floating points are considered different.
 *
 * Next the function extracts the mantissa and shifts it before comparing
 * the absolute value of the difference against the value defined in
 * epsilon.
 *
 * \note
 * At this time the compare ignores whether one of the input is a NaN.
 *
 * \param[in] a  The left hand side floating point value.
 * \param[in] b  The right hand side floating point value.
 * \param[in] epsilon  The maximum delta between a and b.
 *
 * \return true if a and b are considered equal.
 */
template<typename T, typename I, int mantissa, int exponent, I epsilon>
bool almost_equal(T a, T b)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
    // quick test as is first (works great for 0.0 == 0.0)
    // (so the warning is okay, we're under control)
    if(a == b)
#pragma GCC diagnostic pop
    {
        return true;
    }
 
    // different signs means they do not match.
    bool const sa(a < static_cast<T>(0.0));
    bool const sb(b < static_cast<T>(0.0));
    if(sa ^ sb)
    {
        // we already checked for +0.0 == -0.0
        return false;
    }

    union float_t
    {
        T   f_float;
        I   f_integer;
    };
    float_t fa, fb;
    fa.f_float = a;
    fb.f_float = b;

    // retrieve the mantissa
    I ia(fa.f_integer & (static_cast<I>(-1) >> (sizeof(I) * 8 - mantissa)));
    I ib(fb.f_integer & (static_cast<I>(-1) >> (sizeof(I) * 8 - mantissa)));

    // retrieve the exponent
    I ea((fa.f_integer >> mantissa) & (static_cast<I>(-1) >> (sizeof(I) * 8 - exponent)));
    I eb((fb.f_integer >> mantissa) & (static_cast<I>(-1) >> (sizeof(I) * 8 - exponent)));

    // adjust the mantissa with the exponent
    // TBD: ameliorate to try to keep as many bits as possible?
    if(ea < eb)
    {
        I d(eb - ea);
        ib >>= d;
    }
    else if(ea > eb)
    {
        I d(eb - ea);
        ia >>= d;
    }

    // Compare the mantissa
    I p(ia > ib ? ia - ib : ib - ia);
    return p < epsilon;
}
 

/** \brief Compare two float numbers against each others.
 *
 * This function compares two float numbers against each others and return
 * true if they are considered equal.
 *
 * \param[in] a  The left hand side floating point value.
 * \param[in] b  The right hand side floating point value.
 *
 * \return true if the two floats are close.
 */
bool compare_floats(float a, float b)
{
    return almost_equal<float, int32_t, 23, 8, 0x20>(a, b);
}


/** \brief Compare two double numbers against each others.
 *
 * This function compares two double numbers against each others and return
 * true if they are considered equal.
 *
 * \param[in] a  The left hand side floating point value.
 * \param[in] b  The right hand side floating point value.
 *
 * \return true if the two floats are close.
 */
bool compare_floats(double a, double b)
{
    return almost_equal<double, int64_t, 52, 11, 0x80>(a, b);
}


/** \fn compare_floats(double a, float b)
 * \brief Compare two floating point numbers against each others.
 *
 * This function compares a float with a double number after converting
 * the float to a double. The function returns true if they are
 * considered equal.
 *
 * \param[in] a  The left hand side floating point value.
 * \param[in] b  The right hand side floating point value.
 *
 * \return true if the two floats are close.
 */
bool compare_floats(double a, float b)
{
    return compare_floats(a, static_cast<double>(b));
}


/** \fn compare_floats(float a, double b)
 * \brief Compare two floating point numbers against each others.
 *
 * This function compares a float with a double number after converting
 * the float to a double. The function returns true if they are
 * considered equal.
 *
 * \param[in] a  The left hand side floating point value.
 * \param[in] b  The right hand side floating point value.
 *
 * \return true if the two floats are close.
 */
bool compare_floats(float a, double b)
{
    return compare_floats(static_cast<double>(a), b);
}


} // namespace snap
// vim: ts=4 sw=4 et
