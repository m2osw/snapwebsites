/* catch_matrix.cpp
 * Copyright (c) 2018  Made to Order Software Corp.  All Rights Reserved
 *
 * Project: https://snapwebsites.org/project/snapwebsites
 *
 * Permission is hereby granted, free of charge, to any
 * person obtaining a copy of this software and
 * associated documentation files (the "Software"), to
 * deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the
 * following conditions:
 *
 * The above copyright notice and this permission notice
 * shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF
 * ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
 * LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO
 * EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/** \file
 * \brief Verify the various matrix implementation.
 *
 * This file implements tests to verify that the matrix templates
 * work as expected.
 */

// self
//
#include "catch_tests.h"

// libsnapwebsites
//

// ignore the == and != against float warnings
//
#pragma GCC diagnostic ignored "-Wfloat-equal"

#include "snapwebsites/matrix.h"



namespace
{
    auto frand = []()
    {
        return static_cast<double>(rand()) / static_cast<double>(rand());
    };
}
// no name namespace



TEST_CASE("matrix_init", "[matrix]")
{
    // constructor/copy
    // and
    // zero/identity
    //
    GIVEN("constructor")
    {
        SECTION("empty")
        {
            snap::matrix<double> empty;

            REQUIRE(empty.rows() == 0);
            REQUIRE(empty.columns() == 0);

            snap::matrix<double> copy(empty);

            REQUIRE(empty.rows() == 0);
            REQUIRE(empty.columns() == 0);
        }

        SECTION("2x2")
        {
            snap::matrix<double> m(2, 2);

            REQUIRE(m.rows() == 2);
            REQUIRE(m.columns() == 2);

            // by default we get an identity
            //
            REQUIRE(m[0][0] == 1.0);
            REQUIRE(m[0][1] == 0.0);
            REQUIRE(m[1][0] == 0.0);
            REQUIRE(m[1][1] == 1.0);

            double const r00(frand());
            double const r01(frand());
            double const r10(frand());
            double const r11(frand());

            m[0][0] = r00;
            m[0][1] = r01;
            m[1][0] = r10;
            m[1][1] = r11;

            REQUIRE(m[0][0] == r00);
            REQUIRE(m[0][1] == r01);
            REQUIRE(m[1][0] == r10);
            REQUIRE(m[1][1] == r11);

            snap::matrix<double> copy(m);
            snap::matrix<double> c2;

            REQUIRE(copy[0][0] == r00);
            REQUIRE(copy[0][1] == r01);
            REQUIRE(copy[1][0] == r10);
            REQUIRE(copy[1][1] == r11);

            m.clear();

            REQUIRE(m[0][0] == 0.0);
            REQUIRE(m[0][1] == 0.0);
            REQUIRE(m[1][0] == 0.0);
            REQUIRE(m[1][1] == 0.0);

            // copy is still intact
            //
            REQUIRE(copy[0][0] == r00);
            REQUIRE(copy[0][1] == r01);
            REQUIRE(copy[1][0] == r10);
            REQUIRE(copy[1][1] == r11);

            REQUIRE(c2.rows() == 0);
            REQUIRE(c2.columns() == 0);

            c2 = copy;

            REQUIRE(c2[0][0] == r00);
            REQUIRE(c2[0][1] == r01);
            REQUIRE(c2[1][0] == r10);
            REQUIRE(c2[1][1] == r11);

            c2.swap(m);

            REQUIRE(m[0][0] == r00);
            REQUIRE(m[0][1] == r01);
            REQUIRE(m[1][0] == r10);
            REQUIRE(m[1][1] == r11);

            REQUIRE(c2[0][0] == 0.0);
            REQUIRE(c2[0][1] == 0.0);
            REQUIRE(c2[1][0] == 0.0);
            REQUIRE(c2[1][1] == 0.0);
        }

        SECTION("3x3")
        {
            snap::matrix<double> m(3, 3);

            REQUIRE(m.rows() == 3);
            REQUIRE(m.columns() == 3);

            // by default we get an identity
            //
            REQUIRE(m[0][0] == 1.0);
            REQUIRE(m[0][1] == 0.0);
            REQUIRE(m[0][2] == 0.0);
            REQUIRE(m[1][0] == 0.0);
            REQUIRE(m[1][1] == 1.0);
            REQUIRE(m[1][2] == 0.0);
            REQUIRE(m[2][0] == 0.0);
            REQUIRE(m[2][1] == 0.0);
            REQUIRE(m[2][2] == 1.0);

            double const r00(frand());
            double const r01(frand());
            double const r02(frand());
            double const r10(frand());
            double const r11(frand());
            double const r12(frand());
            double const r20(frand());
            double const r21(frand());
            double const r22(frand());

            m[0][0] = r00;
            m[0][1] = r01;
            m[0][2] = r02;
            m[1][0] = r10;
            m[1][1] = r11;
            m[1][2] = r12;
            m[2][0] = r20;
            m[2][1] = r21;
            m[2][2] = r22;

            REQUIRE(m[0][0] == r00);
            REQUIRE(m[0][1] == r01);
            REQUIRE(m[0][2] == r02);
            REQUIRE(m[1][0] == r10);
            REQUIRE(m[1][1] == r11);
            REQUIRE(m[1][2] == r12);
            REQUIRE(m[2][0] == r20);
            REQUIRE(m[2][1] == r21);
            REQUIRE(m[2][2] == r22);

            snap::matrix<double> copy(m);
            snap::matrix<double> c2;

            REQUIRE(copy[0][0] == r00);
            REQUIRE(copy[0][1] == r01);
            REQUIRE(copy[0][2] == r02);
            REQUIRE(copy[1][0] == r10);
            REQUIRE(copy[1][1] == r11);
            REQUIRE(copy[1][2] == r12);
            REQUIRE(copy[2][0] == r20);
            REQUIRE(copy[2][1] == r21);
            REQUIRE(copy[2][2] == r22);

            m.clear();

            REQUIRE(m[0][0] == 0.0);
            REQUIRE(m[0][1] == 0.0);
            REQUIRE(m[0][2] == 0.0);
            REQUIRE(m[1][0] == 0.0);
            REQUIRE(m[1][1] == 0.0);
            REQUIRE(m[1][2] == 0.0);
            REQUIRE(m[2][0] == 0.0);
            REQUIRE(m[2][1] == 0.0);
            REQUIRE(m[2][2] == 0.0);

            // copy is still intact
            //
            REQUIRE(copy[0][0] == r00);
            REQUIRE(copy[0][1] == r01);
            REQUIRE(copy[0][2] == r02);
            REQUIRE(copy[1][0] == r10);
            REQUIRE(copy[1][1] == r11);
            REQUIRE(copy[1][2] == r12);
            REQUIRE(copy[2][0] == r20);
            REQUIRE(copy[2][1] == r21);
            REQUIRE(copy[2][2] == r22);

            REQUIRE(c2.rows() == 0);
            REQUIRE(c2.columns() == 0);

            c2 = copy;

            REQUIRE(c2[0][0] == r00);
            REQUIRE(c2[0][1] == r01);
            REQUIRE(c2[0][2] == r02);
            REQUIRE(c2[1][0] == r10);
            REQUIRE(c2[1][1] == r11);
            REQUIRE(c2[1][2] == r12);
            REQUIRE(c2[2][0] == r20);
            REQUIRE(c2[2][1] == r21);
            REQUIRE(c2[2][2] == r22);

            c2.swap(m);

            REQUIRE(m[0][0] == r00);
            REQUIRE(m[0][1] == r01);
            REQUIRE(m[0][2] == r02);
            REQUIRE(m[1][0] == r10);
            REQUIRE(m[1][1] == r11);
            REQUIRE(m[1][2] == r12);
            REQUIRE(m[2][0] == r20);
            REQUIRE(m[2][1] == r21);
            REQUIRE(m[2][2] == r22);

            REQUIRE(c2[0][0] == 0.0);
            REQUIRE(c2[0][1] == 0.0);
            REQUIRE(c2[0][2] == 0.0);
            REQUIRE(c2[1][0] == 0.0);
            REQUIRE(c2[1][1] == 0.0);
            REQUIRE(c2[1][2] == 0.0);
            REQUIRE(c2[2][0] == 0.0);
            REQUIRE(c2[2][1] == 0.0);
            REQUIRE(c2[2][2] == 0.0);
        }

        SECTION("4x4")
        {
            snap::matrix<double> m(4, 4);

            REQUIRE(m.rows() == 4);
            REQUIRE(m.columns() == 4);

            // by default we get an identity
            //
            REQUIRE(m[0][0] == 1.0);
            REQUIRE(m[0][1] == 0.0);
            REQUIRE(m[0][2] == 0.0);
            REQUIRE(m[0][3] == 0.0);
            REQUIRE(m[1][0] == 0.0);
            REQUIRE(m[1][1] == 1.0);
            REQUIRE(m[1][2] == 0.0);
            REQUIRE(m[1][3] == 0.0);
            REQUIRE(m[2][0] == 0.0);
            REQUIRE(m[2][1] == 0.0);
            REQUIRE(m[2][2] == 1.0);
            REQUIRE(m[2][3] == 0.0);
            REQUIRE(m[3][0] == 0.0);
            REQUIRE(m[3][1] == 0.0);
            REQUIRE(m[3][2] == 0.0);
            REQUIRE(m[3][3] == 1.0);

            double const r00(frand());
            double const r01(frand());
            double const r02(frand());
            double const r03(frand());
            double const r10(frand());
            double const r11(frand());
            double const r12(frand());
            double const r13(frand());
            double const r20(frand());
            double const r21(frand());
            double const r22(frand());
            double const r23(frand());
            double const r30(frand());
            double const r31(frand());
            double const r32(frand());
            double const r33(frand());

            m[0][0] = r00;
            m[0][1] = r01;
            m[0][2] = r02;
            m[0][3] = r03;
            m[1][0] = r10;
            m[1][1] = r11;
            m[1][2] = r12;
            m[1][3] = r13;
            m[2][0] = r20;
            m[2][1] = r21;
            m[2][2] = r22;
            m[2][3] = r23;
            m[3][0] = r30;
            m[3][1] = r31;
            m[3][2] = r32;
            m[3][3] = r33;

            REQUIRE(m[0][0] == r00);
            REQUIRE(m[0][1] == r01);
            REQUIRE(m[0][2] == r02);
            REQUIRE(m[0][3] == r03);
            REQUIRE(m[1][0] == r10);
            REQUIRE(m[1][1] == r11);
            REQUIRE(m[1][2] == r12);
            REQUIRE(m[1][3] == r13);
            REQUIRE(m[2][0] == r20);
            REQUIRE(m[2][1] == r21);
            REQUIRE(m[2][2] == r22);
            REQUIRE(m[2][3] == r23);
            REQUIRE(m[3][0] == r30);
            REQUIRE(m[3][1] == r31);
            REQUIRE(m[3][2] == r32);
            REQUIRE(m[3][3] == r33);

            snap::matrix<double> copy(m);
            snap::matrix<double> c2;

            REQUIRE(copy[0][0] == r00);
            REQUIRE(copy[0][1] == r01);
            REQUIRE(copy[0][2] == r02);
            REQUIRE(copy[0][3] == r03);
            REQUIRE(copy[1][0] == r10);
            REQUIRE(copy[1][1] == r11);
            REQUIRE(copy[1][2] == r12);
            REQUIRE(copy[1][3] == r13);
            REQUIRE(copy[2][0] == r20);
            REQUIRE(copy[2][1] == r21);
            REQUIRE(copy[2][2] == r22);
            REQUIRE(copy[2][3] == r23);
            REQUIRE(copy[3][0] == r30);
            REQUIRE(copy[3][1] == r31);
            REQUIRE(copy[3][2] == r32);
            REQUIRE(copy[3][3] == r33);

            m.clear();

            REQUIRE(m[0][0] == 0.0);
            REQUIRE(m[0][1] == 0.0);
            REQUIRE(m[0][2] == 0.0);
            REQUIRE(m[0][3] == 0.0);
            REQUIRE(m[1][0] == 0.0);
            REQUIRE(m[1][1] == 0.0);
            REQUIRE(m[1][2] == 0.0);
            REQUIRE(m[1][3] == 0.0);
            REQUIRE(m[2][0] == 0.0);
            REQUIRE(m[2][1] == 0.0);
            REQUIRE(m[2][2] == 0.0);
            REQUIRE(m[2][3] == 0.0);

            // copy is still intact
            //
            REQUIRE(copy[0][0] == r00);
            REQUIRE(copy[0][1] == r01);
            REQUIRE(copy[0][2] == r02);
            REQUIRE(copy[0][3] == r03);
            REQUIRE(copy[1][0] == r10);
            REQUIRE(copy[1][1] == r11);
            REQUIRE(copy[1][2] == r12);
            REQUIRE(copy[1][3] == r13);
            REQUIRE(copy[2][0] == r20);
            REQUIRE(copy[2][1] == r21);
            REQUIRE(copy[2][2] == r22);
            REQUIRE(copy[2][3] == r23);
            REQUIRE(copy[3][0] == r30);
            REQUIRE(copy[3][1] == r31);
            REQUIRE(copy[3][2] == r32);
            REQUIRE(copy[3][3] == r33);

            REQUIRE(c2.rows() == 0);
            REQUIRE(c2.columns() == 0);

            c2 = copy;

            REQUIRE(c2[0][0] == r00);
            REQUIRE(c2[0][1] == r01);
            REQUIRE(c2[0][2] == r02);
            REQUIRE(c2[0][3] == r03);
            REQUIRE(c2[1][0] == r10);
            REQUIRE(c2[1][1] == r11);
            REQUIRE(c2[1][2] == r12);
            REQUIRE(c2[1][3] == r13);
            REQUIRE(c2[2][0] == r20);
            REQUIRE(c2[2][1] == r21);
            REQUIRE(c2[2][2] == r22);
            REQUIRE(c2[2][3] == r23);
            REQUIRE(c2[3][0] == r30);
            REQUIRE(c2[3][1] == r31);
            REQUIRE(c2[3][2] == r32);
            REQUIRE(c2[3][3] == r33);

            std::cout << c2 << std::endl;

            c2.swap(m);

            REQUIRE(m[0][0] == r00);
            REQUIRE(m[0][1] == r01);
            REQUIRE(m[0][2] == r02);
            REQUIRE(m[0][3] == r03);
            REQUIRE(m[1][0] == r10);
            REQUIRE(m[1][1] == r11);
            REQUIRE(m[1][2] == r12);
            REQUIRE(m[1][3] == r13);
            REQUIRE(m[2][0] == r20);
            REQUIRE(m[2][1] == r21);
            REQUIRE(m[2][2] == r22);
            REQUIRE(m[2][3] == r23);
            REQUIRE(m[3][0] == r30);
            REQUIRE(m[3][1] == r31);
            REQUIRE(m[3][2] == r32);
            REQUIRE(m[3][3] == r33);

            REQUIRE(c2[0][0] == 0.0);
            REQUIRE(c2[0][1] == 0.0);
            REQUIRE(c2[0][2] == 0.0);
            REQUIRE(c2[0][3] == 0.0);
            REQUIRE(c2[1][0] == 0.0);
            REQUIRE(c2[1][1] == 0.0);
            REQUIRE(c2[1][2] == 0.0);
            REQUIRE(c2[1][3] == 0.0);
            REQUIRE(c2[2][0] == 0.0);
            REQUIRE(c2[2][1] == 0.0);
            REQUIRE(c2[2][2] == 0.0);
            REQUIRE(c2[2][3] == 0.0);
        }
    }
}


TEST_CASE("matrix_additive", "[matrix]")
{
    // create two random 4x4 matrices and make sure the add works
    //
    GIVEN("add")
    {
        SECTION("a+=<scalar>")
        {
            // setup A
            //
            snap::matrix<double> a(4, 4);

            REQUIRE(a.rows() == 4);
            REQUIRE(a.columns() == 4);

            double const a00(frand());
            double const a01(frand());
            double const a02(frand());
            double const a03(frand());
            double const a10(frand());
            double const a11(frand());
            double const a12(frand());
            double const a13(frand());
            double const a20(frand());
            double const a21(frand());
            double const a22(frand());
            double const a23(frand());
            double const a30(frand());
            double const a31(frand());
            double const a32(frand());
            double const a33(frand());

            a[0][0] = a00;
            a[0][1] = a01;
            a[0][2] = a02;
            a[0][3] = a03;
            a[1][0] = a10;
            a[1][1] = a11;
            a[1][2] = a12;
            a[1][3] = a13;
            a[2][0] = a20;
            a[2][1] = a21;
            a[2][2] = a22;
            a[2][3] = a23;
            a[3][0] = a30;
            a[3][1] = a31;
            a[3][2] = a32;
            a[3][3] = a33;

            REQUIRE(a[0][0] == a00);
            REQUIRE(a[0][1] == a01);
            REQUIRE(a[0][2] == a02);
            REQUIRE(a[0][3] == a03);
            REQUIRE(a[1][0] == a10);
            REQUIRE(a[1][1] == a11);
            REQUIRE(a[1][2] == a12);
            REQUIRE(a[1][3] == a13);
            REQUIRE(a[2][0] == a20);
            REQUIRE(a[2][1] == a21);
            REQUIRE(a[2][2] == a22);
            REQUIRE(a[2][3] == a23);
            REQUIRE(a[3][0] == a30);
            REQUIRE(a[3][1] == a31);
            REQUIRE(a[3][2] == a32);
            REQUIRE(a[3][3] == a33);

            // create a scalar for our test
            //
            double scalar(frand());
            while(scalar == 0.0)
            {
                scalar = frand();
            }

            // run operation A += <scalar>
            //
            a += scalar;

            // this can't fail because we ensure scalar != 0
            //
            REQUIRE_FALSE(a[0][0] == a00);
            REQUIRE_FALSE(a[0][1] == a01);
            REQUIRE_FALSE(a[0][2] == a02);
            REQUIRE_FALSE(a[0][3] == a03);
            REQUIRE_FALSE(a[1][0] == a10);
            REQUIRE_FALSE(a[1][1] == a11);
            REQUIRE_FALSE(a[1][2] == a12);
            REQUIRE_FALSE(a[1][3] == a13);
            REQUIRE_FALSE(a[2][0] == a20);
            REQUIRE_FALSE(a[2][1] == a21);
            REQUIRE_FALSE(a[2][2] == a22);
            REQUIRE_FALSE(a[2][3] == a23);
            REQUIRE_FALSE(a[3][0] == a30);
            REQUIRE_FALSE(a[3][1] == a31);
            REQUIRE_FALSE(a[3][2] == a32);
            REQUIRE_FALSE(a[3][3] == a33);

            REQUIRE(a[0][0] == a00 + scalar);
            REQUIRE(a[0][1] == a01 + scalar);
            REQUIRE(a[0][2] == a02 + scalar);
            REQUIRE(a[0][3] == a03 + scalar);
            REQUIRE(a[1][0] == a10 + scalar);
            REQUIRE(a[1][1] == a11 + scalar);
            REQUIRE(a[1][2] == a12 + scalar);
            REQUIRE(a[1][3] == a13 + scalar);
            REQUIRE(a[2][0] == a20 + scalar);
            REQUIRE(a[2][1] == a21 + scalar);
            REQUIRE(a[2][2] == a22 + scalar);
            REQUIRE(a[2][3] == a23 + scalar);
            REQUIRE(a[3][0] == a30 + scalar);
            REQUIRE(a[3][1] == a31 + scalar);
            REQUIRE(a[3][2] == a32 + scalar);
            REQUIRE(a[3][3] == a33 + scalar);
        }

        SECTION("b=a+<scalar>")
        {
            // setup A
            //
            snap::matrix<double> a(4, 4);

            REQUIRE(a.rows() == 4);
            REQUIRE(a.columns() == 4);

            double const a00(frand());
            double const a01(frand());
            double const a02(frand());
            double const a03(frand());
            double const a10(frand());
            double const a11(frand());
            double const a12(frand());
            double const a13(frand());
            double const a20(frand());
            double const a21(frand());
            double const a22(frand());
            double const a23(frand());
            double const a30(frand());
            double const a31(frand());
            double const a32(frand());
            double const a33(frand());

            a[0][0] = a00;
            a[0][1] = a01;
            a[0][2] = a02;
            a[0][3] = a03;
            a[1][0] = a10;
            a[1][1] = a11;
            a[1][2] = a12;
            a[1][3] = a13;
            a[2][0] = a20;
            a[2][1] = a21;
            a[2][2] = a22;
            a[2][3] = a23;
            a[3][0] = a30;
            a[3][1] = a31;
            a[3][2] = a32;
            a[3][3] = a33;

            REQUIRE(a[0][0] == a00);
            REQUIRE(a[0][1] == a01);
            REQUIRE(a[0][2] == a02);
            REQUIRE(a[0][3] == a03);
            REQUIRE(a[1][0] == a10);
            REQUIRE(a[1][1] == a11);
            REQUIRE(a[1][2] == a12);
            REQUIRE(a[1][3] == a13);
            REQUIRE(a[2][0] == a20);
            REQUIRE(a[2][1] == a21);
            REQUIRE(a[2][2] == a22);
            REQUIRE(a[2][3] == a23);
            REQUIRE(a[3][0] == a30);
            REQUIRE(a[3][1] == a31);
            REQUIRE(a[3][2] == a32);
            REQUIRE(a[3][3] == a33);

            // setup B
            //
            snap::matrix<double> b(4, 4);

            REQUIRE(b.rows() == 4);
            REQUIRE(b.columns() == 4);

            double const b00(frand());
            double const b01(frand());
            double const b02(frand());
            double const b03(frand());
            double const b10(frand());
            double const b11(frand());
            double const b12(frand());
            double const b13(frand());
            double const b20(frand());
            double const b21(frand());
            double const b22(frand());
            double const b23(frand());
            double const b30(frand());
            double const b31(frand());
            double const b32(frand());
            double const b33(frand());

            b[0][0] = b00;
            b[0][1] = b01;
            b[0][2] = b02;
            b[0][3] = b03;
            b[1][0] = b10;
            b[1][1] = b11;
            b[1][2] = b12;
            b[1][3] = b13;
            b[2][0] = b20;
            b[2][1] = b21;
            b[2][2] = b22;
            b[2][3] = b23;
            b[3][0] = b30;
            b[3][1] = b31;
            b[3][2] = b32;
            b[3][3] = b33;

            REQUIRE(b[0][0] == b00);
            REQUIRE(b[0][1] == b01);
            REQUIRE(b[0][2] == b02);
            REQUIRE(b[0][3] == b03);
            REQUIRE(b[1][0] == b10);
            REQUIRE(b[1][1] == b11);
            REQUIRE(b[1][2] == b12);
            REQUIRE(b[1][3] == b13);
            REQUIRE(b[2][0] == b20);
            REQUIRE(b[2][1] == b21);
            REQUIRE(b[2][2] == b22);
            REQUIRE(b[2][3] == b23);
            REQUIRE(b[3][0] == b30);
            REQUIRE(b[3][1] == b31);
            REQUIRE(b[3][2] == b32);
            REQUIRE(b[3][3] == b33);

            // create a scalar for our test
            //
            double scalar(frand());
            while(scalar == 0.0)
            {
                scalar = frand();
            }

            // run operation B = A + <scalar>
            //
            b = a + scalar;

            REQUIRE(a[0][0] == a00);
            REQUIRE(a[0][1] == a01);
            REQUIRE(a[0][2] == a02);
            REQUIRE(a[0][3] == a03);
            REQUIRE(a[1][0] == a10);
            REQUIRE(a[1][1] == a11);
            REQUIRE(a[1][2] == a12);
            REQUIRE(a[1][3] == a13);
            REQUIRE(a[2][0] == a20);
            REQUIRE(a[2][1] == a21);
            REQUIRE(a[2][2] == a22);
            REQUIRE(a[2][3] == a23);
            REQUIRE(a[3][0] == a30);
            REQUIRE(a[3][1] == a31);
            REQUIRE(a[3][2] == a32);
            REQUIRE(a[3][3] == a33);

            // this can't fail because we ensure scalar != 0
            //
            REQUIRE_FALSE(b[0][0] == b00);
            REQUIRE_FALSE(b[0][1] == b01);
            REQUIRE_FALSE(b[0][2] == b02);
            REQUIRE_FALSE(b[0][3] == b03);
            REQUIRE_FALSE(b[1][0] == b10);
            REQUIRE_FALSE(b[1][1] == b11);
            REQUIRE_FALSE(b[1][2] == b12);
            REQUIRE_FALSE(b[1][3] == b13);
            REQUIRE_FALSE(b[2][0] == b20);
            REQUIRE_FALSE(b[2][1] == b21);
            REQUIRE_FALSE(b[2][2] == b22);
            REQUIRE_FALSE(b[2][3] == b23);
            REQUIRE_FALSE(b[3][0] == b30);
            REQUIRE_FALSE(b[3][1] == b31);
            REQUIRE_FALSE(b[3][2] == b32);
            REQUIRE_FALSE(b[3][3] == b33);

            REQUIRE(b[0][0] == a00 + scalar);
            REQUIRE(b[0][1] == a01 + scalar);
            REQUIRE(b[0][2] == a02 + scalar);
            REQUIRE(b[0][3] == a03 + scalar);
            REQUIRE(b[1][0] == a10 + scalar);
            REQUIRE(b[1][1] == a11 + scalar);
            REQUIRE(b[1][2] == a12 + scalar);
            REQUIRE(b[1][3] == a13 + scalar);
            REQUIRE(b[2][0] == a20 + scalar);
            REQUIRE(b[2][1] == a21 + scalar);
            REQUIRE(b[2][2] == a22 + scalar);
            REQUIRE(b[2][3] == a23 + scalar);
            REQUIRE(b[3][0] == a30 + scalar);
            REQUIRE(b[3][1] == a31 + scalar);
            REQUIRE(b[3][2] == a32 + scalar);
            REQUIRE(b[3][3] == a33 + scalar);
        }

        SECTION("c=a+b")
        {
            // setup A
            //
            snap::matrix<double> a(4, 4);

            REQUIRE(a.rows() == 4);
            REQUIRE(a.columns() == 4);

            double const a00(frand());
            double const a01(frand());
            double const a02(frand());
            double const a03(frand());
            double const a10(frand());
            double const a11(frand());
            double const a12(frand());
            double const a13(frand());
            double const a20(frand());
            double const a21(frand());
            double const a22(frand());
            double const a23(frand());
            double const a30(frand());
            double const a31(frand());
            double const a32(frand());
            double const a33(frand());

            a[0][0] = a00;
            a[0][1] = a01;
            a[0][2] = a02;
            a[0][3] = a03;
            a[1][0] = a10;
            a[1][1] = a11;
            a[1][2] = a12;
            a[1][3] = a13;
            a[2][0] = a20;
            a[2][1] = a21;
            a[2][2] = a22;
            a[2][3] = a23;
            a[3][0] = a30;
            a[3][1] = a31;
            a[3][2] = a32;
            a[3][3] = a33;

            REQUIRE(a[0][0] == a00);
            REQUIRE(a[0][1] == a01);
            REQUIRE(a[0][2] == a02);
            REQUIRE(a[0][3] == a03);
            REQUIRE(a[1][0] == a10);
            REQUIRE(a[1][1] == a11);
            REQUIRE(a[1][2] == a12);
            REQUIRE(a[1][3] == a13);
            REQUIRE(a[2][0] == a20);
            REQUIRE(a[2][1] == a21);
            REQUIRE(a[2][2] == a22);
            REQUIRE(a[2][3] == a23);
            REQUIRE(a[3][0] == a30);
            REQUIRE(a[3][1] == a31);
            REQUIRE(a[3][2] == a32);
            REQUIRE(a[3][3] == a33);

            // setup B
            //
            snap::matrix<double> b(4, 4);

            REQUIRE(b.rows() == 4);
            REQUIRE(b.columns() == 4);

            double const b00(frand());
            double const b01(frand());
            double const b02(frand());
            double const b03(frand());
            double const b10(frand());
            double const b11(frand());
            double const b12(frand());
            double const b13(frand());
            double const b20(frand());
            double const b21(frand());
            double const b22(frand());
            double const b23(frand());
            double const b30(frand());
            double const b31(frand());
            double const b32(frand());
            double const b33(frand());

            b[0][0] = b00;
            b[0][1] = b01;
            b[0][2] = b02;
            b[0][3] = b03;
            b[1][0] = b10;
            b[1][1] = b11;
            b[1][2] = b12;
            b[1][3] = b13;
            b[2][0] = b20;
            b[2][1] = b21;
            b[2][2] = b22;
            b[2][3] = b23;
            b[3][0] = b30;
            b[3][1] = b31;
            b[3][2] = b32;
            b[3][3] = b33;

            REQUIRE(b[0][0] == b00);
            REQUIRE(b[0][1] == b01);
            REQUIRE(b[0][2] == b02);
            REQUIRE(b[0][3] == b03);
            REQUIRE(b[1][0] == b10);
            REQUIRE(b[1][1] == b11);
            REQUIRE(b[1][2] == b12);
            REQUIRE(b[1][3] == b13);
            REQUIRE(b[2][0] == b20);
            REQUIRE(b[2][1] == b21);
            REQUIRE(b[2][2] == b22);
            REQUIRE(b[2][3] == b23);
            REQUIRE(b[3][0] == b30);
            REQUIRE(b[3][1] == b31);
            REQUIRE(b[3][2] == b32);
            REQUIRE(b[3][3] == b33);

            // setup C
            //
            snap::matrix<double> c(4, 4);

            REQUIRE(c.rows() == 4);
            REQUIRE(c.columns() == 4);

            double const c00(frand());
            double const c01(frand());
            double const c02(frand());
            double const c03(frand());
            double const c10(frand());
            double const c11(frand());
            double const c12(frand());
            double const c13(frand());
            double const c20(frand());
            double const c21(frand());
            double const c22(frand());
            double const c23(frand());
            double const c30(frand());
            double const c31(frand());
            double const c32(frand());
            double const c33(frand());

            c[0][0] = c00;
            c[0][1] = c01;
            c[0][2] = c02;
            c[0][3] = c03;
            c[1][0] = c10;
            c[1][1] = c11;
            c[1][2] = c12;
            c[1][3] = c13;
            c[2][0] = c20;
            c[2][1] = c21;
            c[2][2] = c22;
            c[2][3] = c23;
            c[3][0] = c30;
            c[3][1] = c31;
            c[3][2] = c32;
            c[3][3] = c33;

            REQUIRE(c[0][0] == c00);
            REQUIRE(c[0][1] == c01);
            REQUIRE(c[0][2] == c02);
            REQUIRE(c[0][3] == c03);
            REQUIRE(c[1][0] == c10);
            REQUIRE(c[1][1] == c11);
            REQUIRE(c[1][2] == c12);
            REQUIRE(c[1][3] == c13);
            REQUIRE(c[2][0] == c20);
            REQUIRE(c[2][1] == c21);
            REQUIRE(c[2][2] == c22);
            REQUIRE(c[2][3] == c23);
            REQUIRE(c[3][0] == c30);
            REQUIRE(c[3][1] == c31);
            REQUIRE(c[3][2] == c32);
            REQUIRE(c[3][3] == c33);

            // run operation C = A + B
            //
            c = a + b;

            REQUIRE(a[0][0] == a00);
            REQUIRE(a[0][1] == a01);
            REQUIRE(a[0][2] == a02);
            REQUIRE(a[0][3] == a03);
            REQUIRE(a[1][0] == a10);
            REQUIRE(a[1][1] == a11);
            REQUIRE(a[1][2] == a12);
            REQUIRE(a[1][3] == a13);
            REQUIRE(a[2][0] == a20);
            REQUIRE(a[2][1] == a21);
            REQUIRE(a[2][2] == a22);
            REQUIRE(a[2][3] == a23);
            REQUIRE(a[3][0] == a30);
            REQUIRE(a[3][1] == a31);
            REQUIRE(a[3][2] == a32);
            REQUIRE(a[3][3] == a33);

            REQUIRE(b[0][0] == b00);
            REQUIRE(b[0][1] == b01);
            REQUIRE(b[0][2] == b02);
            REQUIRE(b[0][3] == b03);
            REQUIRE(b[1][0] == b10);
            REQUIRE(b[1][1] == b11);
            REQUIRE(b[1][2] == b12);
            REQUIRE(b[1][3] == b13);
            REQUIRE(b[2][0] == b20);
            REQUIRE(b[2][1] == b21);
            REQUIRE(b[2][2] == b22);
            REQUIRE(b[2][3] == b23);
            REQUIRE(b[3][0] == b30);
            REQUIRE(b[3][1] == b31);
            REQUIRE(b[3][2] == b32);
            REQUIRE(b[3][3] == b33);

            // this could fail
            //REQUIRE_FALSE(c[0][0] == c00);
            //REQUIRE_FALSE(c[0][1] == c01);
            //REQUIRE_FALSE(c[0][2] == c02);
            //REQUIRE_FALSE(c[0][3] == c03);
            //REQUIRE_FALSE(c[1][0] == c10);
            //REQUIRE_FALSE(c[1][1] == c11);
            //REQUIRE_FALSE(c[1][2] == c12);
            //REQUIRE_FALSE(c[1][3] == c13);
            //REQUIRE_FALSE(c[2][0] == c20);
            //REQUIRE_FALSE(c[2][1] == c21);
            //REQUIRE_FALSE(c[2][2] == c22);
            //REQUIRE_FALSE(c[2][3] == c23);
            //REQUIRE_FALSE(c[3][0] == c30);
            //REQUIRE_FALSE(c[3][1] == c31);
            //REQUIRE_FALSE(c[3][2] == c32);
            //REQUIRE_FALSE(c[3][3] == c33);

            REQUIRE(c[0][0] == a00 + b00);
            REQUIRE(c[0][1] == a01 + b01);
            REQUIRE(c[0][2] == a02 + b02);
            REQUIRE(c[0][3] == a03 + b03);
            REQUIRE(c[1][0] == a10 + b10);
            REQUIRE(c[1][1] == a11 + b11);
            REQUIRE(c[1][2] == a12 + b12);
            REQUIRE(c[1][3] == a13 + b13);
            REQUIRE(c[2][0] == a20 + b20);
            REQUIRE(c[2][1] == a21 + b21);
            REQUIRE(c[2][2] == a22 + b22);
            REQUIRE(c[2][3] == a23 + b23);
            REQUIRE(c[3][0] == a30 + b30);
            REQUIRE(c[3][1] == a31 + b31);
            REQUIRE(c[3][2] == a32 + b32);
            REQUIRE(c[3][3] == a33 + b33);
        }

        SECTION("a+=b")
        {
            // setup A
            //
            snap::matrix<double> a(4, 4);

            REQUIRE(a.rows() == 4);
            REQUIRE(a.columns() == 4);

            double const a00(frand());
            double const a01(frand());
            double const a02(frand());
            double const a03(frand());
            double const a10(frand());
            double const a11(frand());
            double const a12(frand());
            double const a13(frand());
            double const a20(frand());
            double const a21(frand());
            double const a22(frand());
            double const a23(frand());
            double const a30(frand());
            double const a31(frand());
            double const a32(frand());
            double const a33(frand());

            a[0][0] = a00;
            a[0][1] = a01;
            a[0][2] = a02;
            a[0][3] = a03;
            a[1][0] = a10;
            a[1][1] = a11;
            a[1][2] = a12;
            a[1][3] = a13;
            a[2][0] = a20;
            a[2][1] = a21;
            a[2][2] = a22;
            a[2][3] = a23;
            a[3][0] = a30;
            a[3][1] = a31;
            a[3][2] = a32;
            a[3][3] = a33;

            REQUIRE(a[0][0] == a00);
            REQUIRE(a[0][1] == a01);
            REQUIRE(a[0][2] == a02);
            REQUIRE(a[0][3] == a03);
            REQUIRE(a[1][0] == a10);
            REQUIRE(a[1][1] == a11);
            REQUIRE(a[1][2] == a12);
            REQUIRE(a[1][3] == a13);
            REQUIRE(a[2][0] == a20);
            REQUIRE(a[2][1] == a21);
            REQUIRE(a[2][2] == a22);
            REQUIRE(a[2][3] == a23);
            REQUIRE(a[3][0] == a30);
            REQUIRE(a[3][1] == a31);
            REQUIRE(a[3][2] == a32);
            REQUIRE(a[3][3] == a33);

            // setup B
            //
            snap::matrix<double> b(4, 4);

            REQUIRE(b.rows() == 4);
            REQUIRE(b.columns() == 4);

            double const b00(frand());
            double const b01(frand());
            double const b02(frand());
            double const b03(frand());
            double const b10(frand());
            double const b11(frand());
            double const b12(frand());
            double const b13(frand());
            double const b20(frand());
            double const b21(frand());
            double const b22(frand());
            double const b23(frand());
            double const b30(frand());
            double const b31(frand());
            double const b32(frand());
            double const b33(frand());

            b[0][0] = b00;
            b[0][1] = b01;
            b[0][2] = b02;
            b[0][3] = b03;
            b[1][0] = b10;
            b[1][1] = b11;
            b[1][2] = b12;
            b[1][3] = b13;
            b[2][0] = b20;
            b[2][1] = b21;
            b[2][2] = b22;
            b[2][3] = b23;
            b[3][0] = b30;
            b[3][1] = b31;
            b[3][2] = b32;
            b[3][3] = b33;

            REQUIRE(b[0][0] == b00);
            REQUIRE(b[0][1] == b01);
            REQUIRE(b[0][2] == b02);
            REQUIRE(b[0][3] == b03);
            REQUIRE(b[1][0] == b10);
            REQUIRE(b[1][1] == b11);
            REQUIRE(b[1][2] == b12);
            REQUIRE(b[1][3] == b13);
            REQUIRE(b[2][0] == b20);
            REQUIRE(b[2][1] == b21);
            REQUIRE(b[2][2] == b22);
            REQUIRE(b[2][3] == b23);
            REQUIRE(b[3][0] == b30);
            REQUIRE(b[3][1] == b31);
            REQUIRE(b[3][2] == b32);
            REQUIRE(b[3][3] == b33);

            // run operation A += B
            //
            a += b;

            // this could fail if any bXX is 0.0
            //
            //REQUIRE_FALSE(a[0][0] == a00);
            //REQUIRE_FALSE(a[0][1] == a01);
            //REQUIRE_FALSE(a[0][2] == a02);
            //REQUIRE_FALSE(a[0][3] == a03);
            //REQUIRE_FALSE(a[1][0] == a10);
            //REQUIRE_FALSE(a[1][1] == a11);
            //REQUIRE_FALSE(a[1][2] == a12);
            //REQUIRE_FALSE(a[1][3] == a13);
            //REQUIRE_FALSE(a[2][0] == a20);
            //REQUIRE_FALSE(a[2][1] == a21);
            //REQUIRE_FALSE(a[2][2] == a22);
            //REQUIRE_FALSE(a[2][3] == a23);
            //REQUIRE_FALSE(a[3][0] == a30);
            //REQUIRE_FALSE(a[3][1] == a31);
            //REQUIRE_FALSE(a[3][2] == a32);
            //REQUIRE_FALSE(a[3][3] == a33);

            REQUIRE(b[0][0] == b00);
            REQUIRE(b[0][1] == b01);
            REQUIRE(b[0][2] == b02);
            REQUIRE(b[0][3] == b03);
            REQUIRE(b[1][0] == b10);
            REQUIRE(b[1][1] == b11);
            REQUIRE(b[1][2] == b12);
            REQUIRE(b[1][3] == b13);
            REQUIRE(b[2][0] == b20);
            REQUIRE(b[2][1] == b21);
            REQUIRE(b[2][2] == b22);
            REQUIRE(b[2][3] == b23);
            REQUIRE(b[3][0] == b30);
            REQUIRE(b[3][1] == b31);
            REQUIRE(b[3][2] == b32);
            REQUIRE(b[3][3] == b33);

            REQUIRE(a[0][0] == a00 + b00);
            REQUIRE(a[0][1] == a01 + b01);
            REQUIRE(a[0][2] == a02 + b02);
            REQUIRE(a[0][3] == a03 + b03);
            REQUIRE(a[1][0] == a10 + b10);
            REQUIRE(a[1][1] == a11 + b11);
            REQUIRE(a[1][2] == a12 + b12);
            REQUIRE(a[1][3] == a13 + b13);
            REQUIRE(a[2][0] == a20 + b20);
            REQUIRE(a[2][1] == a21 + b21);
            REQUIRE(a[2][2] == a22 + b22);
            REQUIRE(a[2][3] == a23 + b23);
            REQUIRE(a[3][0] == a30 + b30);
            REQUIRE(a[3][1] == a31 + b31);
            REQUIRE(a[3][2] == a32 + b32);
            REQUIRE(a[3][3] == a33 + b33);
        }
    }

    // create two random 4x4 matrices and make sure the add works
    //
    GIVEN("sub")
    {
        SECTION("b=a-<scalar>")
        {
            // setup A
            //
            snap::matrix<double> a(4, 4);

            REQUIRE(a.rows() == 4);
            REQUIRE(a.columns() == 4);

            double const a00(frand());
            double const a01(frand());
            double const a02(frand());
            double const a03(frand());
            double const a10(frand());
            double const a11(frand());
            double const a12(frand());
            double const a13(frand());
            double const a20(frand());
            double const a21(frand());
            double const a22(frand());
            double const a23(frand());
            double const a30(frand());
            double const a31(frand());
            double const a32(frand());
            double const a33(frand());

            a[0][0] = a00;
            a[0][1] = a01;
            a[0][2] = a02;
            a[0][3] = a03;
            a[1][0] = a10;
            a[1][1] = a11;
            a[1][2] = a12;
            a[1][3] = a13;
            a[2][0] = a20;
            a[2][1] = a21;
            a[2][2] = a22;
            a[2][3] = a23;
            a[3][0] = a30;
            a[3][1] = a31;
            a[3][2] = a32;
            a[3][3] = a33;

            REQUIRE(a[0][0] == a00);
            REQUIRE(a[0][1] == a01);
            REQUIRE(a[0][2] == a02);
            REQUIRE(a[0][3] == a03);
            REQUIRE(a[1][0] == a10);
            REQUIRE(a[1][1] == a11);
            REQUIRE(a[1][2] == a12);
            REQUIRE(a[1][3] == a13);
            REQUIRE(a[2][0] == a20);
            REQUIRE(a[2][1] == a21);
            REQUIRE(a[2][2] == a22);
            REQUIRE(a[2][3] == a23);
            REQUIRE(a[3][0] == a30);
            REQUIRE(a[3][1] == a31);
            REQUIRE(a[3][2] == a32);
            REQUIRE(a[3][3] == a33);

            // setup B
            //
            snap::matrix<double> b(4, 4);

            REQUIRE(b.rows() == 4);
            REQUIRE(b.columns() == 4);

            double const b00(frand());
            double const b01(frand());
            double const b02(frand());
            double const b03(frand());
            double const b10(frand());
            double const b11(frand());
            double const b12(frand());
            double const b13(frand());
            double const b20(frand());
            double const b21(frand());
            double const b22(frand());
            double const b23(frand());
            double const b30(frand());
            double const b31(frand());
            double const b32(frand());
            double const b33(frand());

            b[0][0] = b00;
            b[0][1] = b01;
            b[0][2] = b02;
            b[0][3] = b03;
            b[1][0] = b10;
            b[1][1] = b11;
            b[1][2] = b12;
            b[1][3] = b13;
            b[2][0] = b20;
            b[2][1] = b21;
            b[2][2] = b22;
            b[2][3] = b23;
            b[3][0] = b30;
            b[3][1] = b31;
            b[3][2] = b32;
            b[3][3] = b33;

            REQUIRE(b[0][0] == b00);
            REQUIRE(b[0][1] == b01);
            REQUIRE(b[0][2] == b02);
            REQUIRE(b[0][3] == b03);
            REQUIRE(b[1][0] == b10);
            REQUIRE(b[1][1] == b11);
            REQUIRE(b[1][2] == b12);
            REQUIRE(b[1][3] == b13);
            REQUIRE(b[2][0] == b20);
            REQUIRE(b[2][1] == b21);
            REQUIRE(b[2][2] == b22);
            REQUIRE(b[2][3] == b23);
            REQUIRE(b[3][0] == b30);
            REQUIRE(b[3][1] == b31);
            REQUIRE(b[3][2] == b32);
            REQUIRE(b[3][3] == b33);

            // create a scalar for our test
            //
            double scalar(frand());
            while(scalar == 0.0)
            {
                scalar = frand();
            }

            // run operation B = A - <scalar>
            //
            b = a - scalar;

            REQUIRE(a[0][0] == a00);
            REQUIRE(a[0][1] == a01);
            REQUIRE(a[0][2] == a02);
            REQUIRE(a[0][3] == a03);
            REQUIRE(a[1][0] == a10);
            REQUIRE(a[1][1] == a11);
            REQUIRE(a[1][2] == a12);
            REQUIRE(a[1][3] == a13);
            REQUIRE(a[2][0] == a20);
            REQUIRE(a[2][1] == a21);
            REQUIRE(a[2][2] == a22);
            REQUIRE(a[2][3] == a23);
            REQUIRE(a[3][0] == a30);
            REQUIRE(a[3][1] == a31);
            REQUIRE(a[3][2] == a32);
            REQUIRE(a[3][3] == a33);

            // this can't fail because we ensure scalar != 0
            //
            REQUIRE_FALSE(b[0][0] == b00);
            REQUIRE_FALSE(b[0][1] == b01);
            REQUIRE_FALSE(b[0][2] == b02);
            REQUIRE_FALSE(b[0][3] == b03);
            REQUIRE_FALSE(b[1][0] == b10);
            REQUIRE_FALSE(b[1][1] == b11);
            REQUIRE_FALSE(b[1][2] == b12);
            REQUIRE_FALSE(b[1][3] == b13);
            REQUIRE_FALSE(b[2][0] == b20);
            REQUIRE_FALSE(b[2][1] == b21);
            REQUIRE_FALSE(b[2][2] == b22);
            REQUIRE_FALSE(b[2][3] == b23);
            REQUIRE_FALSE(b[3][0] == b30);
            REQUIRE_FALSE(b[3][1] == b31);
            REQUIRE_FALSE(b[3][2] == b32);
            REQUIRE_FALSE(b[3][3] == b33);

            REQUIRE(b[0][0] == a00 - scalar);
            REQUIRE(b[0][1] == a01 - scalar);
            REQUIRE(b[0][2] == a02 - scalar);
            REQUIRE(b[0][3] == a03 - scalar);
            REQUIRE(b[1][0] == a10 - scalar);
            REQUIRE(b[1][1] == a11 - scalar);
            REQUIRE(b[1][2] == a12 - scalar);
            REQUIRE(b[1][3] == a13 - scalar);
            REQUIRE(b[2][0] == a20 - scalar);
            REQUIRE(b[2][1] == a21 - scalar);
            REQUIRE(b[2][2] == a22 - scalar);
            REQUIRE(b[2][3] == a23 - scalar);
            REQUIRE(b[3][0] == a30 - scalar);
            REQUIRE(b[3][1] == a31 - scalar);
            REQUIRE(b[3][2] == a32 - scalar);
            REQUIRE(b[3][3] == a33 - scalar);
        }

        SECTION("a-=<scalar>")
        {
            // setup A
            //
            snap::matrix<double> a(4, 4);

            REQUIRE(a.rows() == 4);
            REQUIRE(a.columns() == 4);

            double const a00(frand());
            double const a01(frand());
            double const a02(frand());
            double const a03(frand());
            double const a10(frand());
            double const a11(frand());
            double const a12(frand());
            double const a13(frand());
            double const a20(frand());
            double const a21(frand());
            double const a22(frand());
            double const a23(frand());
            double const a30(frand());
            double const a31(frand());
            double const a32(frand());
            double const a33(frand());

            a[0][0] = a00;
            a[0][1] = a01;
            a[0][2] = a02;
            a[0][3] = a03;
            a[1][0] = a10;
            a[1][1] = a11;
            a[1][2] = a12;
            a[1][3] = a13;
            a[2][0] = a20;
            a[2][1] = a21;
            a[2][2] = a22;
            a[2][3] = a23;
            a[3][0] = a30;
            a[3][1] = a31;
            a[3][2] = a32;
            a[3][3] = a33;

            REQUIRE(a[0][0] == a00);
            REQUIRE(a[0][1] == a01);
            REQUIRE(a[0][2] == a02);
            REQUIRE(a[0][3] == a03);
            REQUIRE(a[1][0] == a10);
            REQUIRE(a[1][1] == a11);
            REQUIRE(a[1][2] == a12);
            REQUIRE(a[1][3] == a13);
            REQUIRE(a[2][0] == a20);
            REQUIRE(a[2][1] == a21);
            REQUIRE(a[2][2] == a22);
            REQUIRE(a[2][3] == a23);
            REQUIRE(a[3][0] == a30);
            REQUIRE(a[3][1] == a31);
            REQUIRE(a[3][2] == a32);
            REQUIRE(a[3][3] == a33);

            // create a scalar for our test
            //
            double scalar(frand());
            while(scalar == 0.0)
            {
                scalar = frand();
            }

            // run operation A -= <scalar>
            //
            a -= scalar;

            // this can't fail because we ensure scalar != 0
            //
            REQUIRE_FALSE(a[0][0] == a00);
            REQUIRE_FALSE(a[0][1] == a01);
            REQUIRE_FALSE(a[0][2] == a02);
            REQUIRE_FALSE(a[0][3] == a03);
            REQUIRE_FALSE(a[1][0] == a10);
            REQUIRE_FALSE(a[1][1] == a11);
            REQUIRE_FALSE(a[1][2] == a12);
            REQUIRE_FALSE(a[1][3] == a13);
            REQUIRE_FALSE(a[2][0] == a20);
            REQUIRE_FALSE(a[2][1] == a21);
            REQUIRE_FALSE(a[2][2] == a22);
            REQUIRE_FALSE(a[2][3] == a23);
            REQUIRE_FALSE(a[3][0] == a30);
            REQUIRE_FALSE(a[3][1] == a31);
            REQUIRE_FALSE(a[3][2] == a32);
            REQUIRE_FALSE(a[3][3] == a33);

            REQUIRE(a[0][0] == a00 - scalar);
            REQUIRE(a[0][1] == a01 - scalar);
            REQUIRE(a[0][2] == a02 - scalar);
            REQUIRE(a[0][3] == a03 - scalar);
            REQUIRE(a[1][0] == a10 - scalar);
            REQUIRE(a[1][1] == a11 - scalar);
            REQUIRE(a[1][2] == a12 - scalar);
            REQUIRE(a[1][3] == a13 - scalar);
            REQUIRE(a[2][0] == a20 - scalar);
            REQUIRE(a[2][1] == a21 - scalar);
            REQUIRE(a[2][2] == a22 - scalar);
            REQUIRE(a[2][3] == a23 - scalar);
            REQUIRE(a[3][0] == a30 - scalar);
            REQUIRE(a[3][1] == a31 - scalar);
            REQUIRE(a[3][2] == a32 - scalar);
            REQUIRE(a[3][3] == a33 - scalar);
        }

        SECTION("c=a-b")
        {
            // setup A
            //
            snap::matrix<double> a(4, 4);

            REQUIRE(a.rows() == 4);
            REQUIRE(a.columns() == 4);

            double const a00(frand());
            double const a01(frand());
            double const a02(frand());
            double const a03(frand());
            double const a10(frand());
            double const a11(frand());
            double const a12(frand());
            double const a13(frand());
            double const a20(frand());
            double const a21(frand());
            double const a22(frand());
            double const a23(frand());
            double const a30(frand());
            double const a31(frand());
            double const a32(frand());
            double const a33(frand());

            a[0][0] = a00;
            a[0][1] = a01;
            a[0][2] = a02;
            a[0][3] = a03;
            a[1][0] = a10;
            a[1][1] = a11;
            a[1][2] = a12;
            a[1][3] = a13;
            a[2][0] = a20;
            a[2][1] = a21;
            a[2][2] = a22;
            a[2][3] = a23;
            a[3][0] = a30;
            a[3][1] = a31;
            a[3][2] = a32;
            a[3][3] = a33;

            REQUIRE(a[0][0] == a00);
            REQUIRE(a[0][1] == a01);
            REQUIRE(a[0][2] == a02);
            REQUIRE(a[0][3] == a03);
            REQUIRE(a[1][0] == a10);
            REQUIRE(a[1][1] == a11);
            REQUIRE(a[1][2] == a12);
            REQUIRE(a[1][3] == a13);
            REQUIRE(a[2][0] == a20);
            REQUIRE(a[2][1] == a21);
            REQUIRE(a[2][2] == a22);
            REQUIRE(a[2][3] == a23);
            REQUIRE(a[3][0] == a30);
            REQUIRE(a[3][1] == a31);
            REQUIRE(a[3][2] == a32);
            REQUIRE(a[3][3] == a33);

            // setup B
            //
            snap::matrix<double> b(4, 4);

            REQUIRE(b.rows() == 4);
            REQUIRE(b.columns() == 4);

            double const b00(frand());
            double const b01(frand());
            double const b02(frand());
            double const b03(frand());
            double const b10(frand());
            double const b11(frand());
            double const b12(frand());
            double const b13(frand());
            double const b20(frand());
            double const b21(frand());
            double const b22(frand());
            double const b23(frand());
            double const b30(frand());
            double const b31(frand());
            double const b32(frand());
            double const b33(frand());

            b[0][0] = b00;
            b[0][1] = b01;
            b[0][2] = b02;
            b[0][3] = b03;
            b[1][0] = b10;
            b[1][1] = b11;
            b[1][2] = b12;
            b[1][3] = b13;
            b[2][0] = b20;
            b[2][1] = b21;
            b[2][2] = b22;
            b[2][3] = b23;
            b[3][0] = b30;
            b[3][1] = b31;
            b[3][2] = b32;
            b[3][3] = b33;

            REQUIRE(b[0][0] == b00);
            REQUIRE(b[0][1] == b01);
            REQUIRE(b[0][2] == b02);
            REQUIRE(b[0][3] == b03);
            REQUIRE(b[1][0] == b10);
            REQUIRE(b[1][1] == b11);
            REQUIRE(b[1][2] == b12);
            REQUIRE(b[1][3] == b13);
            REQUIRE(b[2][0] == b20);
            REQUIRE(b[2][1] == b21);
            REQUIRE(b[2][2] == b22);
            REQUIRE(b[2][3] == b23);
            REQUIRE(b[3][0] == b30);
            REQUIRE(b[3][1] == b31);
            REQUIRE(b[3][2] == b32);
            REQUIRE(b[3][3] == b33);

            // setup C
            //
            snap::matrix<double> c(4, 4);

            REQUIRE(c.rows() == 4);
            REQUIRE(c.columns() == 4);

            double const c00(frand());
            double const c01(frand());
            double const c02(frand());
            double const c03(frand());
            double const c10(frand());
            double const c11(frand());
            double const c12(frand());
            double const c13(frand());
            double const c20(frand());
            double const c21(frand());
            double const c22(frand());
            double const c23(frand());
            double const c30(frand());
            double const c31(frand());
            double const c32(frand());
            double const c33(frand());

            c[0][0] = c00;
            c[0][1] = c01;
            c[0][2] = c02;
            c[0][3] = c03;
            c[1][0] = c10;
            c[1][1] = c11;
            c[1][2] = c12;
            c[1][3] = c13;
            c[2][0] = c20;
            c[2][1] = c21;
            c[2][2] = c22;
            c[2][3] = c23;
            c[3][0] = c30;
            c[3][1] = c31;
            c[3][2] = c32;
            c[3][3] = c33;

            REQUIRE(c[0][0] == c00);
            REQUIRE(c[0][1] == c01);
            REQUIRE(c[0][2] == c02);
            REQUIRE(c[0][3] == c03);
            REQUIRE(c[1][0] == c10);
            REQUIRE(c[1][1] == c11);
            REQUIRE(c[1][2] == c12);
            REQUIRE(c[1][3] == c13);
            REQUIRE(c[2][0] == c20);
            REQUIRE(c[2][1] == c21);
            REQUIRE(c[2][2] == c22);
            REQUIRE(c[2][3] == c23);
            REQUIRE(c[3][0] == c30);
            REQUIRE(c[3][1] == c31);
            REQUIRE(c[3][2] == c32);
            REQUIRE(c[3][3] == c33);

            // run operation C = A - B
            //
            c = a - b;

            REQUIRE(a[0][0] == a00);
            REQUIRE(a[0][1] == a01);
            REQUIRE(a[0][2] == a02);
            REQUIRE(a[0][3] == a03);
            REQUIRE(a[1][0] == a10);
            REQUIRE(a[1][1] == a11);
            REQUIRE(a[1][2] == a12);
            REQUIRE(a[1][3] == a13);
            REQUIRE(a[2][0] == a20);
            REQUIRE(a[2][1] == a21);
            REQUIRE(a[2][2] == a22);
            REQUIRE(a[2][3] == a23);
            REQUIRE(a[3][0] == a30);
            REQUIRE(a[3][1] == a31);
            REQUIRE(a[3][2] == a32);
            REQUIRE(a[3][3] == a33);

            REQUIRE(b[0][0] == b00);
            REQUIRE(b[0][1] == b01);
            REQUIRE(b[0][2] == b02);
            REQUIRE(b[0][3] == b03);
            REQUIRE(b[1][0] == b10);
            REQUIRE(b[1][1] == b11);
            REQUIRE(b[1][2] == b12);
            REQUIRE(b[1][3] == b13);
            REQUIRE(b[2][0] == b20);
            REQUIRE(b[2][1] == b21);
            REQUIRE(b[2][2] == b22);
            REQUIRE(b[2][3] == b23);
            REQUIRE(b[3][0] == b30);
            REQUIRE(b[3][1] == b31);
            REQUIRE(b[3][2] == b32);
            REQUIRE(b[3][3] == b33);

            // this could fail
            //REQUIRE_FALSE(c[0][0] == c00);
            //REQUIRE_FALSE(c[0][1] == c01);
            //REQUIRE_FALSE(c[0][2] == c02);
            //REQUIRE_FALSE(c[0][3] == c03);
            //REQUIRE_FALSE(c[1][0] == c10);
            //REQUIRE_FALSE(c[1][1] == c11);
            //REQUIRE_FALSE(c[1][2] == c12);
            //REQUIRE_FALSE(c[1][3] == c13);
            //REQUIRE_FALSE(c[2][0] == c20);
            //REQUIRE_FALSE(c[2][1] == c21);
            //REQUIRE_FALSE(c[2][2] == c22);
            //REQUIRE_FALSE(c[2][3] == c23);
            //REQUIRE_FALSE(c[3][0] == c30);
            //REQUIRE_FALSE(c[3][1] == c31);
            //REQUIRE_FALSE(c[3][2] == c32);
            //REQUIRE_FALSE(c[3][3] == c33);

            REQUIRE(c[0][0] == a00 - b00);
            REQUIRE(c[0][1] == a01 - b01);
            REQUIRE(c[0][2] == a02 - b02);
            REQUIRE(c[0][3] == a03 - b03);
            REQUIRE(c[1][0] == a10 - b10);
            REQUIRE(c[1][1] == a11 - b11);
            REQUIRE(c[1][2] == a12 - b12);
            REQUIRE(c[1][3] == a13 - b13);
            REQUIRE(c[2][0] == a20 - b20);
            REQUIRE(c[2][1] == a21 - b21);
            REQUIRE(c[2][2] == a22 - b22);
            REQUIRE(c[2][3] == a23 - b23);
            REQUIRE(c[3][0] == a30 - b30);
            REQUIRE(c[3][1] == a31 - b31);
            REQUIRE(c[3][2] == a32 - b32);
            REQUIRE(c[3][3] == a33 - b33);
        }

        SECTION("a-=b")
        {
            // setup A
            //
            snap::matrix<double> a(4, 4);

            REQUIRE(a.rows() == 4);
            REQUIRE(a.columns() == 4);

            double const a00(frand());
            double const a01(frand());
            double const a02(frand());
            double const a03(frand());
            double const a10(frand());
            double const a11(frand());
            double const a12(frand());
            double const a13(frand());
            double const a20(frand());
            double const a21(frand());
            double const a22(frand());
            double const a23(frand());
            double const a30(frand());
            double const a31(frand());
            double const a32(frand());
            double const a33(frand());

            a[0][0] = a00;
            a[0][1] = a01;
            a[0][2] = a02;
            a[0][3] = a03;
            a[1][0] = a10;
            a[1][1] = a11;
            a[1][2] = a12;
            a[1][3] = a13;
            a[2][0] = a20;
            a[2][1] = a21;
            a[2][2] = a22;
            a[2][3] = a23;
            a[3][0] = a30;
            a[3][1] = a31;
            a[3][2] = a32;
            a[3][3] = a33;

            REQUIRE(a[0][0] == a00);
            REQUIRE(a[0][1] == a01);
            REQUIRE(a[0][2] == a02);
            REQUIRE(a[0][3] == a03);
            REQUIRE(a[1][0] == a10);
            REQUIRE(a[1][1] == a11);
            REQUIRE(a[1][2] == a12);
            REQUIRE(a[1][3] == a13);
            REQUIRE(a[2][0] == a20);
            REQUIRE(a[2][1] == a21);
            REQUIRE(a[2][2] == a22);
            REQUIRE(a[2][3] == a23);
            REQUIRE(a[3][0] == a30);
            REQUIRE(a[3][1] == a31);
            REQUIRE(a[3][2] == a32);
            REQUIRE(a[3][3] == a33);

            // setup B
            //
            snap::matrix<double> b(4, 4);

            REQUIRE(b.rows() == 4);
            REQUIRE(b.columns() == 4);

            double const b00(frand());
            double const b01(frand());
            double const b02(frand());
            double const b03(frand());
            double const b10(frand());
            double const b11(frand());
            double const b12(frand());
            double const b13(frand());
            double const b20(frand());
            double const b21(frand());
            double const b22(frand());
            double const b23(frand());
            double const b30(frand());
            double const b31(frand());
            double const b32(frand());
            double const b33(frand());

            b[0][0] = b00;
            b[0][1] = b01;
            b[0][2] = b02;
            b[0][3] = b03;
            b[1][0] = b10;
            b[1][1] = b11;
            b[1][2] = b12;
            b[1][3] = b13;
            b[2][0] = b20;
            b[2][1] = b21;
            b[2][2] = b22;
            b[2][3] = b23;
            b[3][0] = b30;
            b[3][1] = b31;
            b[3][2] = b32;
            b[3][3] = b33;

            REQUIRE(b[0][0] == b00);
            REQUIRE(b[0][1] == b01);
            REQUIRE(b[0][2] == b02);
            REQUIRE(b[0][3] == b03);
            REQUIRE(b[1][0] == b10);
            REQUIRE(b[1][1] == b11);
            REQUIRE(b[1][2] == b12);
            REQUIRE(b[1][3] == b13);
            REQUIRE(b[2][0] == b20);
            REQUIRE(b[2][1] == b21);
            REQUIRE(b[2][2] == b22);
            REQUIRE(b[2][3] == b23);
            REQUIRE(b[3][0] == b30);
            REQUIRE(b[3][1] == b31);
            REQUIRE(b[3][2] == b32);
            REQUIRE(b[3][3] == b33);

            // run operation A -= B
            //
            a -= b;

            // this could fail if one of bXX is 0.0
            //
            //REQUIRE(a[0][0] == a00);
            //REQUIRE(a[0][1] == a01);
            //REQUIRE(a[0][2] == a02);
            //REQUIRE(a[0][3] == a03);
            //REQUIRE(a[1][0] == a10);
            //REQUIRE(a[1][1] == a11);
            //REQUIRE(a[1][2] == a12);
            //REQUIRE(a[1][3] == a13);
            //REQUIRE(a[2][0] == a20);
            //REQUIRE(a[2][1] == a21);
            //REQUIRE(a[2][2] == a22);
            //REQUIRE(a[2][3] == a23);
            //REQUIRE(a[3][0] == a30);
            //REQUIRE(a[3][1] == a31);
            //REQUIRE(a[3][2] == a32);
            //REQUIRE(a[3][3] == a33);

            REQUIRE(b[0][0] == b00);
            REQUIRE(b[0][1] == b01);
            REQUIRE(b[0][2] == b02);
            REQUIRE(b[0][3] == b03);
            REQUIRE(b[1][0] == b10);
            REQUIRE(b[1][1] == b11);
            REQUIRE(b[1][2] == b12);
            REQUIRE(b[1][3] == b13);
            REQUIRE(b[2][0] == b20);
            REQUIRE(b[2][1] == b21);
            REQUIRE(b[2][2] == b22);
            REQUIRE(b[2][3] == b23);
            REQUIRE(b[3][0] == b30);
            REQUIRE(b[3][1] == b31);
            REQUIRE(b[3][2] == b32);
            REQUIRE(b[3][3] == b33);

            REQUIRE(a[0][0] == a00 - b00);
            REQUIRE(a[0][1] == a01 - b01);
            REQUIRE(a[0][2] == a02 - b02);
            REQUIRE(a[0][3] == a03 - b03);
            REQUIRE(a[1][0] == a10 - b10);
            REQUIRE(a[1][1] == a11 - b11);
            REQUIRE(a[1][2] == a12 - b12);
            REQUIRE(a[1][3] == a13 - b13);
            REQUIRE(a[2][0] == a20 - b20);
            REQUIRE(a[2][1] == a21 - b21);
            REQUIRE(a[2][2] == a22 - b22);
            REQUIRE(a[2][3] == a23 - b23);
            REQUIRE(a[3][0] == a30 - b30);
            REQUIRE(a[3][1] == a31 - b31);
            REQUIRE(a[3][2] == a32 - b32);
            REQUIRE(a[3][3] == a33 - b33);
        }
    }
}


TEST_CASE("matrix_multiplicative", "[matrix]")
{
    // create two random 4x4 matrices and make sure the add works
    //
    GIVEN("mul")
    {
        SECTION("b=a*<scalar>")
        {
            // setup A
            //
            snap::matrix<double> a(4, 4);

            REQUIRE(a.rows() == 4);
            REQUIRE(a.columns() == 4);

            double const a00(frand());
            double const a01(frand());
            double const a02(frand());
            double const a03(frand());
            double const a10(frand());
            double const a11(frand());
            double const a12(frand());
            double const a13(frand());
            double const a20(frand());
            double const a21(frand());
            double const a22(frand());
            double const a23(frand());
            double const a30(frand());
            double const a31(frand());
            double const a32(frand());
            double const a33(frand());

            a[0][0] = a00;
            a[0][1] = a01;
            a[0][2] = a02;
            a[0][3] = a03;
            a[1][0] = a10;
            a[1][1] = a11;
            a[1][2] = a12;
            a[1][3] = a13;
            a[2][0] = a20;
            a[2][1] = a21;
            a[2][2] = a22;
            a[2][3] = a23;
            a[3][0] = a30;
            a[3][1] = a31;
            a[3][2] = a32;
            a[3][3] = a33;

            REQUIRE(a[0][0] == a00);
            REQUIRE(a[0][1] == a01);
            REQUIRE(a[0][2] == a02);
            REQUIRE(a[0][3] == a03);
            REQUIRE(a[1][0] == a10);
            REQUIRE(a[1][1] == a11);
            REQUIRE(a[1][2] == a12);
            REQUIRE(a[1][3] == a13);
            REQUIRE(a[2][0] == a20);
            REQUIRE(a[2][1] == a21);
            REQUIRE(a[2][2] == a22);
            REQUIRE(a[2][3] == a23);
            REQUIRE(a[3][0] == a30);
            REQUIRE(a[3][1] == a31);
            REQUIRE(a[3][2] == a32);
            REQUIRE(a[3][3] == a33);

            // setup B
            //
            snap::matrix<double> b(4, 4);

            REQUIRE(b.rows() == 4);
            REQUIRE(b.columns() == 4);

            double const b00(frand());
            double const b01(frand());
            double const b02(frand());
            double const b03(frand());
            double const b10(frand());
            double const b11(frand());
            double const b12(frand());
            double const b13(frand());
            double const b20(frand());
            double const b21(frand());
            double const b22(frand());
            double const b23(frand());
            double const b30(frand());
            double const b31(frand());
            double const b32(frand());
            double const b33(frand());

            b[0][0] = b00;
            b[0][1] = b01;
            b[0][2] = b02;
            b[0][3] = b03;
            b[1][0] = b10;
            b[1][1] = b11;
            b[1][2] = b12;
            b[1][3] = b13;
            b[2][0] = b20;
            b[2][1] = b21;
            b[2][2] = b22;
            b[2][3] = b23;
            b[3][0] = b30;
            b[3][1] = b31;
            b[3][2] = b32;
            b[3][3] = b33;

            REQUIRE(b[0][0] == b00);
            REQUIRE(b[0][1] == b01);
            REQUIRE(b[0][2] == b02);
            REQUIRE(b[0][3] == b03);
            REQUIRE(b[1][0] == b10);
            REQUIRE(b[1][1] == b11);
            REQUIRE(b[1][2] == b12);
            REQUIRE(b[1][3] == b13);
            REQUIRE(b[2][0] == b20);
            REQUIRE(b[2][1] == b21);
            REQUIRE(b[2][2] == b22);
            REQUIRE(b[2][3] == b23);
            REQUIRE(b[3][0] == b30);
            REQUIRE(b[3][1] == b31);
            REQUIRE(b[3][2] == b32);
            REQUIRE(b[3][3] == b33);

            // create a scalar for our test
            //
            double scalar(frand());
            while(scalar == 0.0 || scalar == 1.0)
            {
                scalar = frand();
            }

            // run operation B = A + <scalar>
            //
            b = a * scalar;

            REQUIRE(a[0][0] == a00);
            REQUIRE(a[0][1] == a01);
            REQUIRE(a[0][2] == a02);
            REQUIRE(a[0][3] == a03);
            REQUIRE(a[1][0] == a10);
            REQUIRE(a[1][1] == a11);
            REQUIRE(a[1][2] == a12);
            REQUIRE(a[1][3] == a13);
            REQUIRE(a[2][0] == a20);
            REQUIRE(a[2][1] == a21);
            REQUIRE(a[2][2] == a22);
            REQUIRE(a[2][3] == a23);
            REQUIRE(a[3][0] == a30);
            REQUIRE(a[3][1] == a31);
            REQUIRE(a[3][2] == a32);
            REQUIRE(a[3][3] == a33);

            // this can't fail because we ensure scalar != 0 or 1
            //
            REQUIRE_FALSE(b[0][0] == b00);
            REQUIRE_FALSE(b[0][1] == b01);
            REQUIRE_FALSE(b[0][2] == b02);
            REQUIRE_FALSE(b[0][3] == b03);
            REQUIRE_FALSE(b[1][0] == b10);
            REQUIRE_FALSE(b[1][1] == b11);
            REQUIRE_FALSE(b[1][2] == b12);
            REQUIRE_FALSE(b[1][3] == b13);
            REQUIRE_FALSE(b[2][0] == b20);
            REQUIRE_FALSE(b[2][1] == b21);
            REQUIRE_FALSE(b[2][2] == b22);
            REQUIRE_FALSE(b[2][3] == b23);
            REQUIRE_FALSE(b[3][0] == b30);
            REQUIRE_FALSE(b[3][1] == b31);
            REQUIRE_FALSE(b[3][2] == b32);
            REQUIRE_FALSE(b[3][3] == b33);

            REQUIRE(b[0][0] == a00 * scalar);
            REQUIRE(b[0][1] == a01 * scalar);
            REQUIRE(b[0][2] == a02 * scalar);
            REQUIRE(b[0][3] == a03 * scalar);
            REQUIRE(b[1][0] == a10 * scalar);
            REQUIRE(b[1][1] == a11 * scalar);
            REQUIRE(b[1][2] == a12 * scalar);
            REQUIRE(b[1][3] == a13 * scalar);
            REQUIRE(b[2][0] == a20 * scalar);
            REQUIRE(b[2][1] == a21 * scalar);
            REQUIRE(b[2][2] == a22 * scalar);
            REQUIRE(b[2][3] == a23 * scalar);
            REQUIRE(b[3][0] == a30 * scalar);
            REQUIRE(b[3][1] == a31 * scalar);
            REQUIRE(b[3][2] == a32 * scalar);
            REQUIRE(b[3][3] == a33 * scalar);
        }

        SECTION("a*=<scalar>")
        {
            // setup A
            //
            snap::matrix<double> a(4, 4);

            REQUIRE(a.rows() == 4);
            REQUIRE(a.columns() == 4);

            double const a00(frand());
            double const a01(frand());
            double const a02(frand());
            double const a03(frand());
            double const a10(frand());
            double const a11(frand());
            double const a12(frand());
            double const a13(frand());
            double const a20(frand());
            double const a21(frand());
            double const a22(frand());
            double const a23(frand());
            double const a30(frand());
            double const a31(frand());
            double const a32(frand());
            double const a33(frand());

            a[0][0] = a00;
            a[0][1] = a01;
            a[0][2] = a02;
            a[0][3] = a03;
            a[1][0] = a10;
            a[1][1] = a11;
            a[1][2] = a12;
            a[1][3] = a13;
            a[2][0] = a20;
            a[2][1] = a21;
            a[2][2] = a22;
            a[2][3] = a23;
            a[3][0] = a30;
            a[3][1] = a31;
            a[3][2] = a32;
            a[3][3] = a33;

            REQUIRE(a[0][0] == a00);
            REQUIRE(a[0][1] == a01);
            REQUIRE(a[0][2] == a02);
            REQUIRE(a[0][3] == a03);
            REQUIRE(a[1][0] == a10);
            REQUIRE(a[1][1] == a11);
            REQUIRE(a[1][2] == a12);
            REQUIRE(a[1][3] == a13);
            REQUIRE(a[2][0] == a20);
            REQUIRE(a[2][1] == a21);
            REQUIRE(a[2][2] == a22);
            REQUIRE(a[2][3] == a23);
            REQUIRE(a[3][0] == a30);
            REQUIRE(a[3][1] == a31);
            REQUIRE(a[3][2] == a32);
            REQUIRE(a[3][3] == a33);

            // create a scalar for our test
            //
            double scalar(frand());
            while(scalar == 0.0 || scalar == 1.0)
            {
                scalar = frand();
            }

            // run operation A *= <scalar>
            //
            a *= scalar;

            // this can't fail because we ensure scalar != 0 or 1
            //
            REQUIRE_FALSE(a[0][0] == a00);
            REQUIRE_FALSE(a[0][1] == a01);
            REQUIRE_FALSE(a[0][2] == a02);
            REQUIRE_FALSE(a[0][3] == a03);
            REQUIRE_FALSE(a[1][0] == a10);
            REQUIRE_FALSE(a[1][1] == a11);
            REQUIRE_FALSE(a[1][2] == a12);
            REQUIRE_FALSE(a[1][3] == a13);
            REQUIRE_FALSE(a[2][0] == a20);
            REQUIRE_FALSE(a[2][1] == a21);
            REQUIRE_FALSE(a[2][2] == a22);
            REQUIRE_FALSE(a[2][3] == a23);
            REQUIRE_FALSE(a[3][0] == a30);
            REQUIRE_FALSE(a[3][1] == a31);
            REQUIRE_FALSE(a[3][2] == a32);
            REQUIRE_FALSE(a[3][3] == a33);

            REQUIRE(a[0][0] == a00 * scalar);
            REQUIRE(a[0][1] == a01 * scalar);
            REQUIRE(a[0][2] == a02 * scalar);
            REQUIRE(a[0][3] == a03 * scalar);
            REQUIRE(a[1][0] == a10 * scalar);
            REQUIRE(a[1][1] == a11 * scalar);
            REQUIRE(a[1][2] == a12 * scalar);
            REQUIRE(a[1][3] == a13 * scalar);
            REQUIRE(a[2][0] == a20 * scalar);
            REQUIRE(a[2][1] == a21 * scalar);
            REQUIRE(a[2][2] == a22 * scalar);
            REQUIRE(a[2][3] == a23 * scalar);
            REQUIRE(a[3][0] == a30 * scalar);
            REQUIRE(a[3][1] == a31 * scalar);
            REQUIRE(a[3][2] == a32 * scalar);
            REQUIRE(a[3][3] == a33 * scalar);
        }

        SECTION("c=a*b")
        {
            // setup A
            //
            snap::matrix<double> a(4, 4);

            REQUIRE(a.rows() == 4);
            REQUIRE(a.columns() == 4);

            double const a00(frand());
            double const a01(frand());
            double const a02(frand());
            double const a03(frand());
            double const a10(frand());
            double const a11(frand());
            double const a12(frand());
            double const a13(frand());
            double const a20(frand());
            double const a21(frand());
            double const a22(frand());
            double const a23(frand());
            double const a30(frand());
            double const a31(frand());
            double const a32(frand());
            double const a33(frand());

            a[0][0] = a00;
            a[0][1] = a01;
            a[0][2] = a02;
            a[0][3] = a03;
            a[1][0] = a10;
            a[1][1] = a11;
            a[1][2] = a12;
            a[1][3] = a13;
            a[2][0] = a20;
            a[2][1] = a21;
            a[2][2] = a22;
            a[2][3] = a23;
            a[3][0] = a30;
            a[3][1] = a31;
            a[3][2] = a32;
            a[3][3] = a33;

            REQUIRE(a[0][0] == a00);
            REQUIRE(a[0][1] == a01);
            REQUIRE(a[0][2] == a02);
            REQUIRE(a[0][3] == a03);
            REQUIRE(a[1][0] == a10);
            REQUIRE(a[1][1] == a11);
            REQUIRE(a[1][2] == a12);
            REQUIRE(a[1][3] == a13);
            REQUIRE(a[2][0] == a20);
            REQUIRE(a[2][1] == a21);
            REQUIRE(a[2][2] == a22);
            REQUIRE(a[2][3] == a23);
            REQUIRE(a[3][0] == a30);
            REQUIRE(a[3][1] == a31);
            REQUIRE(a[3][2] == a32);
            REQUIRE(a[3][3] == a33);

            // setup B
            //
            snap::matrix<double> b(4, 4);

            REQUIRE(b.rows() == 4);
            REQUIRE(b.columns() == 4);

            double const b00(frand());
            double const b01(frand());
            double const b02(frand());
            double const b03(frand());
            double const b10(frand());
            double const b11(frand());
            double const b12(frand());
            double const b13(frand());
            double const b20(frand());
            double const b21(frand());
            double const b22(frand());
            double const b23(frand());
            double const b30(frand());
            double const b31(frand());
            double const b32(frand());
            double const b33(frand());

            b[0][0] = b00;
            b[0][1] = b01;
            b[0][2] = b02;
            b[0][3] = b03;
            b[1][0] = b10;
            b[1][1] = b11;
            b[1][2] = b12;
            b[1][3] = b13;
            b[2][0] = b20;
            b[2][1] = b21;
            b[2][2] = b22;
            b[2][3] = b23;
            b[3][0] = b30;
            b[3][1] = b31;
            b[3][2] = b32;
            b[3][3] = b33;

            REQUIRE(b[0][0] == b00);
            REQUIRE(b[0][1] == b01);
            REQUIRE(b[0][2] == b02);
            REQUIRE(b[0][3] == b03);
            REQUIRE(b[1][0] == b10);
            REQUIRE(b[1][1] == b11);
            REQUIRE(b[1][2] == b12);
            REQUIRE(b[1][3] == b13);
            REQUIRE(b[2][0] == b20);
            REQUIRE(b[2][1] == b21);
            REQUIRE(b[2][2] == b22);
            REQUIRE(b[2][3] == b23);
            REQUIRE(b[3][0] == b30);
            REQUIRE(b[3][1] == b31);
            REQUIRE(b[3][2] == b32);
            REQUIRE(b[3][3] == b33);

            // setup C
            //
            snap::matrix<double> c(4, 4);

            REQUIRE(c.rows() == 4);
            REQUIRE(c.columns() == 4);

            double const c00(frand());
            double const c01(frand());
            double const c02(frand());
            double const c03(frand());
            double const c10(frand());
            double const c11(frand());
            double const c12(frand());
            double const c13(frand());
            double const c20(frand());
            double const c21(frand());
            double const c22(frand());
            double const c23(frand());
            double const c30(frand());
            double const c31(frand());
            double const c32(frand());
            double const c33(frand());

            c[0][0] = c00;
            c[0][1] = c01;
            c[0][2] = c02;
            c[0][3] = c03;
            c[1][0] = c10;
            c[1][1] = c11;
            c[1][2] = c12;
            c[1][3] = c13;
            c[2][0] = c20;
            c[2][1] = c21;
            c[2][2] = c22;
            c[2][3] = c23;
            c[3][0] = c30;
            c[3][1] = c31;
            c[3][2] = c32;
            c[3][3] = c33;

            REQUIRE(c[0][0] == c00);
            REQUIRE(c[0][1] == c01);
            REQUIRE(c[0][2] == c02);
            REQUIRE(c[0][3] == c03);
            REQUIRE(c[1][0] == c10);
            REQUIRE(c[1][1] == c11);
            REQUIRE(c[1][2] == c12);
            REQUIRE(c[1][3] == c13);
            REQUIRE(c[2][0] == c20);
            REQUIRE(c[2][1] == c21);
            REQUIRE(c[2][2] == c22);
            REQUIRE(c[2][3] == c23);
            REQUIRE(c[3][0] == c30);
            REQUIRE(c[3][1] == c31);
            REQUIRE(c[3][2] == c32);
            REQUIRE(c[3][3] == c33);

            // run operation C = A * B
            //
            c = a * b;

            REQUIRE(a[0][0] == a00);
            REQUIRE(a[0][1] == a01);
            REQUIRE(a[0][2] == a02);
            REQUIRE(a[0][3] == a03);
            REQUIRE(a[1][0] == a10);
            REQUIRE(a[1][1] == a11);
            REQUIRE(a[1][2] == a12);
            REQUIRE(a[1][3] == a13);
            REQUIRE(a[2][0] == a20);
            REQUIRE(a[2][1] == a21);
            REQUIRE(a[2][2] == a22);
            REQUIRE(a[2][3] == a23);
            REQUIRE(a[3][0] == a30);
            REQUIRE(a[3][1] == a31);
            REQUIRE(a[3][2] == a32);
            REQUIRE(a[3][3] == a33);

            REQUIRE(b[0][0] == b00);
            REQUIRE(b[0][1] == b01);
            REQUIRE(b[0][2] == b02);
            REQUIRE(b[0][3] == b03);
            REQUIRE(b[1][0] == b10);
            REQUIRE(b[1][1] == b11);
            REQUIRE(b[1][2] == b12);
            REQUIRE(b[1][3] == b13);
            REQUIRE(b[2][0] == b20);
            REQUIRE(b[2][1] == b21);
            REQUIRE(b[2][2] == b22);
            REQUIRE(b[2][3] == b23);
            REQUIRE(b[3][0] == b30);
            REQUIRE(b[3][1] == b31);
            REQUIRE(b[3][2] == b32);
            REQUIRE(b[3][3] == b33);

            // this could fail (albeit rather unlikely)
            //REQUIRE_FALSE(c[0][0] == c00);
            //REQUIRE_FALSE(c[0][1] == c01);
            //REQUIRE_FALSE(c[0][2] == c02);
            //REQUIRE_FALSE(c[0][3] == c03);
            //REQUIRE_FALSE(c[1][0] == c10);
            //REQUIRE_FALSE(c[1][1] == c11);
            //REQUIRE_FALSE(c[1][2] == c12);
            //REQUIRE_FALSE(c[1][3] == c13);
            //REQUIRE_FALSE(c[2][0] == c20);
            //REQUIRE_FALSE(c[2][1] == c21);
            //REQUIRE_FALSE(c[2][2] == c22);
            //REQUIRE_FALSE(c[2][3] == c23);
            //REQUIRE_FALSE(c[3][0] == c30);
            //REQUIRE_FALSE(c[3][1] == c31);
            //REQUIRE_FALSE(c[3][2] == c32);
            //REQUIRE_FALSE(c[3][3] == c33);

//std::cout << "A = " << a << std::endl;
//std::cout << "B = " << b << std::endl;
//std::cout << "C = " << c << std::endl;

            // if swapped it would equal this instead
            //REQUIRE(c[0][0] == a00 * b00 + a01 * b10 + a02 * b20 + a03 * b30);

            REQUIRE(c[0][0] == a00 * b00 + a10 * b01 + a20 * b02 + a30 * b03);
            REQUIRE(c[0][1] == a01 * b00 + a11 * b01 + a21 * b02 + a31 * b03);
            REQUIRE(c[0][2] == a02 * b00 + a12 * b01 + a22 * b02 + a32 * b03);
            REQUIRE(c[0][3] == a03 * b00 + a13 * b01 + a23 * b02 + a33 * b03);

            REQUIRE(c[1][0] == a00 * b10 + a10 * b11 + a20 * b12 + a30 * b13);
            REQUIRE(c[1][1] == a01 * b10 + a11 * b11 + a21 * b12 + a31 * b13);
            REQUIRE(c[1][2] == a02 * b10 + a12 * b11 + a22 * b12 + a32 * b13);
            REQUIRE(c[1][3] == a03 * b10 + a13 * b11 + a23 * b12 + a33 * b13);

            REQUIRE(c[2][0] == a00 * b20 + a10 * b21 + a20 * b22 + a30 * b23);
            REQUIRE(c[2][1] == a01 * b20 + a11 * b21 + a21 * b22 + a31 * b23);
            REQUIRE(c[2][2] == a02 * b20 + a12 * b21 + a22 * b22 + a32 * b23);
            REQUIRE(c[2][3] == a03 * b20 + a13 * b21 + a23 * b22 + a33 * b23);

            REQUIRE(c[3][0] == a00 * b30 + a10 * b31 + a20 * b32 + a30 * b33);
            REQUIRE(c[3][1] == a01 * b30 + a11 * b31 + a21 * b32 + a31 * b33);
            REQUIRE(c[3][2] == a02 * b30 + a12 * b31 + a22 * b32 + a32 * b33);
            REQUIRE(c[3][3] == a03 * b30 + a13 * b31 + a23 * b32 + a33 * b33);
        }

        SECTION("a*=b")
        {
            // setup A
            //
            snap::matrix<double> a(4, 4);

            REQUIRE(a.rows() == 4);
            REQUIRE(a.columns() == 4);

            double const a00(frand());
            double const a01(frand());
            double const a02(frand());
            double const a03(frand());
            double const a10(frand());
            double const a11(frand());
            double const a12(frand());
            double const a13(frand());
            double const a20(frand());
            double const a21(frand());
            double const a22(frand());
            double const a23(frand());
            double const a30(frand());
            double const a31(frand());
            double const a32(frand());
            double const a33(frand());

            a[0][0] = a00;
            a[0][1] = a01;
            a[0][2] = a02;
            a[0][3] = a03;
            a[1][0] = a10;
            a[1][1] = a11;
            a[1][2] = a12;
            a[1][3] = a13;
            a[2][0] = a20;
            a[2][1] = a21;
            a[2][2] = a22;
            a[2][3] = a23;
            a[3][0] = a30;
            a[3][1] = a31;
            a[3][2] = a32;
            a[3][3] = a33;

            REQUIRE(a[0][0] == a00);
            REQUIRE(a[0][1] == a01);
            REQUIRE(a[0][2] == a02);
            REQUIRE(a[0][3] == a03);
            REQUIRE(a[1][0] == a10);
            REQUIRE(a[1][1] == a11);
            REQUIRE(a[1][2] == a12);
            REQUIRE(a[1][3] == a13);
            REQUIRE(a[2][0] == a20);
            REQUIRE(a[2][1] == a21);
            REQUIRE(a[2][2] == a22);
            REQUIRE(a[2][3] == a23);
            REQUIRE(a[3][0] == a30);
            REQUIRE(a[3][1] == a31);
            REQUIRE(a[3][2] == a32);
            REQUIRE(a[3][3] == a33);

            // setup B
            //
            snap::matrix<double> b(4, 4);

            REQUIRE(b.rows() == 4);
            REQUIRE(b.columns() == 4);

            double const b00(frand());
            double const b01(frand());
            double const b02(frand());
            double const b03(frand());
            double const b10(frand());
            double const b11(frand());
            double const b12(frand());
            double const b13(frand());
            double const b20(frand());
            double const b21(frand());
            double const b22(frand());
            double const b23(frand());
            double const b30(frand());
            double const b31(frand());
            double const b32(frand());
            double const b33(frand());

            b[0][0] = b00;
            b[0][1] = b01;
            b[0][2] = b02;
            b[0][3] = b03;
            b[1][0] = b10;
            b[1][1] = b11;
            b[1][2] = b12;
            b[1][3] = b13;
            b[2][0] = b20;
            b[2][1] = b21;
            b[2][2] = b22;
            b[2][3] = b23;
            b[3][0] = b30;
            b[3][1] = b31;
            b[3][2] = b32;
            b[3][3] = b33;

            REQUIRE(b[0][0] == b00);
            REQUIRE(b[0][1] == b01);
            REQUIRE(b[0][2] == b02);
            REQUIRE(b[0][3] == b03);
            REQUIRE(b[1][0] == b10);
            REQUIRE(b[1][1] == b11);
            REQUIRE(b[1][2] == b12);
            REQUIRE(b[1][3] == b13);
            REQUIRE(b[2][0] == b20);
            REQUIRE(b[2][1] == b21);
            REQUIRE(b[2][2] == b22);
            REQUIRE(b[2][3] == b23);
            REQUIRE(b[3][0] == b30);
            REQUIRE(b[3][1] == b31);
            REQUIRE(b[3][2] == b32);
            REQUIRE(b[3][3] == b33);

            // run operation A *= B
            //
            a *= b;

            // this could fail
            //
            //REQUIRE_FALSE(a[0][0] == a00);
            //REQUIRE_FALSE(a[0][1] == a01);
            //REQUIRE_FALSE(a[0][2] == a02);
            //REQUIRE_FALSE(a[0][3] == a03);
            //REQUIRE_FALSE(a[1][0] == a10);
            //REQUIRE_FALSE(a[1][1] == a11);
            //REQUIRE_FALSE(a[1][2] == a12);
            //REQUIRE_FALSE(a[1][3] == a13);
            //REQUIRE_FALSE(a[2][0] == a20);
            //REQUIRE_FALSE(a[2][1] == a21);
            //REQUIRE_FALSE(a[2][2] == a22);
            //REQUIRE_FALSE(a[2][3] == a23);
            //REQUIRE_FALSE(a[3][0] == a30);
            //REQUIRE_FALSE(a[3][1] == a31);
            //REQUIRE_FALSE(a[3][2] == a32);
            //REQUIRE_FALSE(a[3][3] == a33);

            REQUIRE(b[0][0] == b00);
            REQUIRE(b[0][1] == b01);
            REQUIRE(b[0][2] == b02);
            REQUIRE(b[0][3] == b03);
            REQUIRE(b[1][0] == b10);
            REQUIRE(b[1][1] == b11);
            REQUIRE(b[1][2] == b12);
            REQUIRE(b[1][3] == b13);
            REQUIRE(b[2][0] == b20);
            REQUIRE(b[2][1] == b21);
            REQUIRE(b[2][2] == b22);
            REQUIRE(b[2][3] == b23);
            REQUIRE(b[3][0] == b30);
            REQUIRE(b[3][1] == b31);
            REQUIRE(b[3][2] == b32);
            REQUIRE(b[3][3] == b33);

            REQUIRE(a[0][0] == a00 * b00 + a10 * b01 + a20 * b02 + a30 * b03);
            REQUIRE(a[0][1] == a01 * b00 + a11 * b01 + a21 * b02 + a31 * b03);
            REQUIRE(a[0][2] == a02 * b00 + a12 * b01 + a22 * b02 + a32 * b03);
            REQUIRE(a[0][3] == a03 * b00 + a13 * b01 + a23 * b02 + a33 * b03);

            REQUIRE(a[1][0] == a00 * b10 + a10 * b11 + a20 * b12 + a30 * b13);
            REQUIRE(a[1][1] == a01 * b10 + a11 * b11 + a21 * b12 + a31 * b13);
            REQUIRE(a[1][2] == a02 * b10 + a12 * b11 + a22 * b12 + a32 * b13);
            REQUIRE(a[1][3] == a03 * b10 + a13 * b11 + a23 * b12 + a33 * b13);

            REQUIRE(a[2][0] == a00 * b20 + a10 * b21 + a20 * b22 + a30 * b23);
            REQUIRE(a[2][1] == a01 * b20 + a11 * b21 + a21 * b22 + a31 * b23);
            REQUIRE(a[2][2] == a02 * b20 + a12 * b21 + a22 * b22 + a32 * b23);
            REQUIRE(a[2][3] == a03 * b20 + a13 * b21 + a23 * b22 + a33 * b23);

            REQUIRE(a[3][0] == a00 * b30 + a10 * b31 + a20 * b32 + a30 * b33);
            REQUIRE(a[3][1] == a01 * b30 + a11 * b31 + a21 * b32 + a31 * b33);
            REQUIRE(a[3][2] == a02 * b30 + a12 * b31 + a22 * b32 + a32 * b33);
            REQUIRE(a[3][3] == a03 * b30 + a13 * b31 + a23 * b32 + a33 * b33);
        }
    }

    // create two random 4x4 matrices and make sure the add works
    //
    GIVEN("div")
    {
        SECTION("b=a/<scalar>")
        {
            // setup A
            //
            snap::matrix<double> a(4, 4);

            REQUIRE(a.rows() == 4);
            REQUIRE(a.columns() == 4);

            double const a00(frand());
            double const a01(frand());
            double const a02(frand());
            double const a03(frand());
            double const a10(frand());
            double const a11(frand());
            double const a12(frand());
            double const a13(frand());
            double const a20(frand());
            double const a21(frand());
            double const a22(frand());
            double const a23(frand());
            double const a30(frand());
            double const a31(frand());
            double const a32(frand());
            double const a33(frand());

            a[0][0] = a00;
            a[0][1] = a01;
            a[0][2] = a02;
            a[0][3] = a03;
            a[1][0] = a10;
            a[1][1] = a11;
            a[1][2] = a12;
            a[1][3] = a13;
            a[2][0] = a20;
            a[2][1] = a21;
            a[2][2] = a22;
            a[2][3] = a23;
            a[3][0] = a30;
            a[3][1] = a31;
            a[3][2] = a32;
            a[3][3] = a33;

            REQUIRE(a[0][0] == a00);
            REQUIRE(a[0][1] == a01);
            REQUIRE(a[0][2] == a02);
            REQUIRE(a[0][3] == a03);
            REQUIRE(a[1][0] == a10);
            REQUIRE(a[1][1] == a11);
            REQUIRE(a[1][2] == a12);
            REQUIRE(a[1][3] == a13);
            REQUIRE(a[2][0] == a20);
            REQUIRE(a[2][1] == a21);
            REQUIRE(a[2][2] == a22);
            REQUIRE(a[2][3] == a23);
            REQUIRE(a[3][0] == a30);
            REQUIRE(a[3][1] == a31);
            REQUIRE(a[3][2] == a32);
            REQUIRE(a[3][3] == a33);

            // setup B
            //
            snap::matrix<double> b(4, 4);

            REQUIRE(b.rows() == 4);
            REQUIRE(b.columns() == 4);

            double const b00(frand());
            double const b01(frand());
            double const b02(frand());
            double const b03(frand());
            double const b10(frand());
            double const b11(frand());
            double const b12(frand());
            double const b13(frand());
            double const b20(frand());
            double const b21(frand());
            double const b22(frand());
            double const b23(frand());
            double const b30(frand());
            double const b31(frand());
            double const b32(frand());
            double const b33(frand());

            b[0][0] = b00;
            b[0][1] = b01;
            b[0][2] = b02;
            b[0][3] = b03;
            b[1][0] = b10;
            b[1][1] = b11;
            b[1][2] = b12;
            b[1][3] = b13;
            b[2][0] = b20;
            b[2][1] = b21;
            b[2][2] = b22;
            b[2][3] = b23;
            b[3][0] = b30;
            b[3][1] = b31;
            b[3][2] = b32;
            b[3][3] = b33;

            REQUIRE(b[0][0] == b00);
            REQUIRE(b[0][1] == b01);
            REQUIRE(b[0][2] == b02);
            REQUIRE(b[0][3] == b03);
            REQUIRE(b[1][0] == b10);
            REQUIRE(b[1][1] == b11);
            REQUIRE(b[1][2] == b12);
            REQUIRE(b[1][3] == b13);
            REQUIRE(b[2][0] == b20);
            REQUIRE(b[2][1] == b21);
            REQUIRE(b[2][2] == b22);
            REQUIRE(b[2][3] == b23);
            REQUIRE(b[3][0] == b30);
            REQUIRE(b[3][1] == b31);
            REQUIRE(b[3][2] == b32);
            REQUIRE(b[3][3] == b33);

            // create a scalar for our test
            //
            double scalar(frand());
            while(scalar == 0.0 || scalar == 1.0)
            {
                scalar = frand();
            }

            // run operation B = A / <scalar>
            //
            b = a / scalar;

            REQUIRE(a[0][0] == a00);
            REQUIRE(a[0][1] == a01);
            REQUIRE(a[0][2] == a02);
            REQUIRE(a[0][3] == a03);
            REQUIRE(a[1][0] == a10);
            REQUIRE(a[1][1] == a11);
            REQUIRE(a[1][2] == a12);
            REQUIRE(a[1][3] == a13);
            REQUIRE(a[2][0] == a20);
            REQUIRE(a[2][1] == a21);
            REQUIRE(a[2][2] == a22);
            REQUIRE(a[2][3] == a23);
            REQUIRE(a[3][0] == a30);
            REQUIRE(a[3][1] == a31);
            REQUIRE(a[3][2] == a32);
            REQUIRE(a[3][3] == a33);

            // this can't fail because we ensure scalar != 0 or 1
            //
            REQUIRE_FALSE(b[0][0] == b00);
            REQUIRE_FALSE(b[0][1] == b01);
            REQUIRE_FALSE(b[0][2] == b02);
            REQUIRE_FALSE(b[0][3] == b03);
            REQUIRE_FALSE(b[1][0] == b10);
            REQUIRE_FALSE(b[1][1] == b11);
            REQUIRE_FALSE(b[1][2] == b12);
            REQUIRE_FALSE(b[1][3] == b13);
            REQUIRE_FALSE(b[2][0] == b20);
            REQUIRE_FALSE(b[2][1] == b21);
            REQUIRE_FALSE(b[2][2] == b22);
            REQUIRE_FALSE(b[2][3] == b23);
            REQUIRE_FALSE(b[3][0] == b30);
            REQUIRE_FALSE(b[3][1] == b31);
            REQUIRE_FALSE(b[3][2] == b32);
            REQUIRE_FALSE(b[3][3] == b33);

            REQUIRE(b[0][0] == a00 / scalar);
            REQUIRE(b[0][1] == a01 / scalar);
            REQUIRE(b[0][2] == a02 / scalar);
            REQUIRE(b[0][3] == a03 / scalar);
            REQUIRE(b[1][0] == a10 / scalar);
            REQUIRE(b[1][1] == a11 / scalar);
            REQUIRE(b[1][2] == a12 / scalar);
            REQUIRE(b[1][3] == a13 / scalar);
            REQUIRE(b[2][0] == a20 / scalar);
            REQUIRE(b[2][1] == a21 / scalar);
            REQUIRE(b[2][2] == a22 / scalar);
            REQUIRE(b[2][3] == a23 / scalar);
            REQUIRE(b[3][0] == a30 / scalar);
            REQUIRE(b[3][1] == a31 / scalar);
            REQUIRE(b[3][2] == a32 / scalar);
            REQUIRE(b[3][3] == a33 / scalar);
        }

        SECTION("a/=<scalar>")
        {
            // setup A
            //
            snap::matrix<double> a(4, 4);

            REQUIRE(a.rows() == 4);
            REQUIRE(a.columns() == 4);

            double const a00(frand());
            double const a01(frand());
            double const a02(frand());
            double const a03(frand());
            double const a10(frand());
            double const a11(frand());
            double const a12(frand());
            double const a13(frand());
            double const a20(frand());
            double const a21(frand());
            double const a22(frand());
            double const a23(frand());
            double const a30(frand());
            double const a31(frand());
            double const a32(frand());
            double const a33(frand());

            a[0][0] = a00;
            a[0][1] = a01;
            a[0][2] = a02;
            a[0][3] = a03;
            a[1][0] = a10;
            a[1][1] = a11;
            a[1][2] = a12;
            a[1][3] = a13;
            a[2][0] = a20;
            a[2][1] = a21;
            a[2][2] = a22;
            a[2][3] = a23;
            a[3][0] = a30;
            a[3][1] = a31;
            a[3][2] = a32;
            a[3][3] = a33;

            REQUIRE(a[0][0] == a00);
            REQUIRE(a[0][1] == a01);
            REQUIRE(a[0][2] == a02);
            REQUIRE(a[0][3] == a03);
            REQUIRE(a[1][0] == a10);
            REQUIRE(a[1][1] == a11);
            REQUIRE(a[1][2] == a12);
            REQUIRE(a[1][3] == a13);
            REQUIRE(a[2][0] == a20);
            REQUIRE(a[2][1] == a21);
            REQUIRE(a[2][2] == a22);
            REQUIRE(a[2][3] == a23);
            REQUIRE(a[3][0] == a30);
            REQUIRE(a[3][1] == a31);
            REQUIRE(a[3][2] == a32);
            REQUIRE(a[3][3] == a33);

            // create a scalar for our test
            //
            double scalar(frand());
            while(scalar == 0.0)
            {
                scalar = frand();
            }

            // run operation A /= <scalar>
            //
            a /= scalar;

            // this can't fail because we ensure scalar != 0 or 1
            //
            REQUIRE_FALSE(a[0][0] == a00);
            REQUIRE_FALSE(a[0][1] == a01);
            REQUIRE_FALSE(a[0][2] == a02);
            REQUIRE_FALSE(a[0][3] == a03);
            REQUIRE_FALSE(a[1][0] == a10);
            REQUIRE_FALSE(a[1][1] == a11);
            REQUIRE_FALSE(a[1][2] == a12);
            REQUIRE_FALSE(a[1][3] == a13);
            REQUIRE_FALSE(a[2][0] == a20);
            REQUIRE_FALSE(a[2][1] == a21);
            REQUIRE_FALSE(a[2][2] == a22);
            REQUIRE_FALSE(a[2][3] == a23);
            REQUIRE_FALSE(a[3][0] == a30);
            REQUIRE_FALSE(a[3][1] == a31);
            REQUIRE_FALSE(a[3][2] == a32);
            REQUIRE_FALSE(a[3][3] == a33);

            REQUIRE(a[0][0] == a00 / scalar);
            REQUIRE(a[0][1] == a01 / scalar);
            REQUIRE(a[0][2] == a02 / scalar);
            REQUIRE(a[0][3] == a03 / scalar);
            REQUIRE(a[1][0] == a10 / scalar);
            REQUIRE(a[1][1] == a11 / scalar);
            REQUIRE(a[1][2] == a12 / scalar);
            REQUIRE(a[1][3] == a13 / scalar);
            REQUIRE(a[2][0] == a20 / scalar);
            REQUIRE(a[2][1] == a21 / scalar);
            REQUIRE(a[2][2] == a22 / scalar);
            REQUIRE(a[2][3] == a23 / scalar);
            REQUIRE(a[3][0] == a30 / scalar);
            REQUIRE(a[3][1] == a31 / scalar);
            REQUIRE(a[3][2] == a32 / scalar);
            REQUIRE(a[3][3] == a33 / scalar);
        }

        SECTION("c=a/b")
        {
            // setup A
            //
            snap::matrix<double> a(4, 4);

            REQUIRE(a.rows() == 4);
            REQUIRE(a.columns() == 4);

            double const a00(frand());
            double const a01(frand());
            double const a02(frand());
            double const a03(frand());
            double const a10(frand());
            double const a11(frand());
            double const a12(frand());
            double const a13(frand());
            double const a20(frand());
            double const a21(frand());
            double const a22(frand());
            double const a23(frand());
            double const a30(frand());
            double const a31(frand());
            double const a32(frand());
            double const a33(frand());

            a[0][0] = a00;
            a[0][1] = a01;
            a[0][2] = a02;
            a[0][3] = a03;
            a[1][0] = a10;
            a[1][1] = a11;
            a[1][2] = a12;
            a[1][3] = a13;
            a[2][0] = a20;
            a[2][1] = a21;
            a[2][2] = a22;
            a[2][3] = a23;
            a[3][0] = a30;
            a[3][1] = a31;
            a[3][2] = a32;
            a[3][3] = a33;

            REQUIRE(a[0][0] == a00);
            REQUIRE(a[0][1] == a01);
            REQUIRE(a[0][2] == a02);
            REQUIRE(a[0][3] == a03);
            REQUIRE(a[1][0] == a10);
            REQUIRE(a[1][1] == a11);
            REQUIRE(a[1][2] == a12);
            REQUIRE(a[1][3] == a13);
            REQUIRE(a[2][0] == a20);
            REQUIRE(a[2][1] == a21);
            REQUIRE(a[2][2] == a22);
            REQUIRE(a[2][3] == a23);
            REQUIRE(a[3][0] == a30);
            REQUIRE(a[3][1] == a31);
            REQUIRE(a[3][2] == a32);
            REQUIRE(a[3][3] == a33);

            // setup B
            //
            snap::matrix<double> b(4, 4);

            REQUIRE(b.rows() == 4);
            REQUIRE(b.columns() == 4);

            double b00;
            double b01;
            double b02;
            double b03;
            double b10;
            double b11;
            double b12;
            double b13;
            double b20;
            double b21;
            double b22;
            double b23;
            double b30;
            double b31;
            double b32;
            double b33;

            // create an inversible matrix (most are with random numbers)
            snap::matrix<double> t(4, 4);
            do
            {
                b00 = frand();
                b01 = frand();
                b02 = frand();
                b03 = frand();
                b10 = frand();
                b11 = frand();
                b12 = frand();
                b13 = frand();
                b20 = frand();
                b21 = frand();
                b22 = frand();
                b23 = frand();
                b30 = frand();
                b31 = frand();
                b32 = frand();
                b33 = frand();

                b[0][0] = b00;
                b[0][1] = b01;
                b[0][2] = b02;
                b[0][3] = b03;
                b[1][0] = b10;
                b[1][1] = b11;
                b[1][2] = b12;
                b[1][3] = b13;
                b[2][0] = b20;
                b[2][1] = b21;
                b[2][2] = b22;
                b[2][3] = b23;
                b[3][0] = b30;
                b[3][1] = b31;
                b[3][2] = b32;
                b[3][3] = b33;

                REQUIRE(b[0][0] == b00);
                REQUIRE(b[0][1] == b01);
                REQUIRE(b[0][2] == b02);
                REQUIRE(b[0][3] == b03);
                REQUIRE(b[1][0] == b10);
                REQUIRE(b[1][1] == b11);
                REQUIRE(b[1][2] == b12);
                REQUIRE(b[1][3] == b13);
                REQUIRE(b[2][0] == b20);
                REQUIRE(b[2][1] == b21);
                REQUIRE(b[2][2] == b22);
                REQUIRE(b[2][3] == b23);
                REQUIRE(b[3][0] == b30);
                REQUIRE(b[3][1] == b31);
                REQUIRE(b[3][2] == b32);
                REQUIRE(b[3][3] == b33);

                t = b;
            }
            while(!t.inverse());

            // setup C
            //
            snap::matrix<double> c(4, 4);

            REQUIRE(c.rows() == 4);
            REQUIRE(c.columns() == 4);

            double const c00(frand());
            double const c01(frand());
            double const c02(frand());
            double const c03(frand());
            double const c10(frand());
            double const c11(frand());
            double const c12(frand());
            double const c13(frand());
            double const c20(frand());
            double const c21(frand());
            double const c22(frand());
            double const c23(frand());
            double const c30(frand());
            double const c31(frand());
            double const c32(frand());
            double const c33(frand());

            c[0][0] = c00;
            c[0][1] = c01;
            c[0][2] = c02;
            c[0][3] = c03;
            c[1][0] = c10;
            c[1][1] = c11;
            c[1][2] = c12;
            c[1][3] = c13;
            c[2][0] = c20;
            c[2][1] = c21;
            c[2][2] = c22;
            c[2][3] = c23;
            c[3][0] = c30;
            c[3][1] = c31;
            c[3][2] = c32;
            c[3][3] = c33;

            REQUIRE(c[0][0] == c00);
            REQUIRE(c[0][1] == c01);
            REQUIRE(c[0][2] == c02);
            REQUIRE(c[0][3] == c03);
            REQUIRE(c[1][0] == c10);
            REQUIRE(c[1][1] == c11);
            REQUIRE(c[1][2] == c12);
            REQUIRE(c[1][3] == c13);
            REQUIRE(c[2][0] == c20);
            REQUIRE(c[2][1] == c21);
            REQUIRE(c[2][2] == c22);
            REQUIRE(c[2][3] == c23);
            REQUIRE(c[3][0] == c30);
            REQUIRE(c[3][1] == c31);
            REQUIRE(c[3][2] == c32);
            REQUIRE(c[3][3] == c33);

            // run operation C = A / B
            //
            c = a / b;

            REQUIRE(a[0][0] == a00);
            REQUIRE(a[0][1] == a01);
            REQUIRE(a[0][2] == a02);
            REQUIRE(a[0][3] == a03);
            REQUIRE(a[1][0] == a10);
            REQUIRE(a[1][1] == a11);
            REQUIRE(a[1][2] == a12);
            REQUIRE(a[1][3] == a13);
            REQUIRE(a[2][0] == a20);
            REQUIRE(a[2][1] == a21);
            REQUIRE(a[2][2] == a22);
            REQUIRE(a[2][3] == a23);
            REQUIRE(a[3][0] == a30);
            REQUIRE(a[3][1] == a31);
            REQUIRE(a[3][2] == a32);
            REQUIRE(a[3][3] == a33);

            REQUIRE(b[0][0] == b00);
            REQUIRE(b[0][1] == b01);
            REQUIRE(b[0][2] == b02);
            REQUIRE(b[0][3] == b03);
            REQUIRE(b[1][0] == b10);
            REQUIRE(b[1][1] == b11);
            REQUIRE(b[1][2] == b12);
            REQUIRE(b[1][3] == b13);
            REQUIRE(b[2][0] == b20);
            REQUIRE(b[2][1] == b21);
            REQUIRE(b[2][2] == b22);
            REQUIRE(b[2][3] == b23);
            REQUIRE(b[3][0] == b30);
            REQUIRE(b[3][1] == b31);
            REQUIRE(b[3][2] == b32);
            REQUIRE(b[3][3] == b33);

            // this could fail
            //REQUIRE_FALSE(c[0][0] == c00);
            //REQUIRE_FALSE(c[0][1] == c01);
            //REQUIRE_FALSE(c[0][2] == c02);
            //REQUIRE_FALSE(c[0][3] == c03);
            //REQUIRE_FALSE(c[1][0] == c10);
            //REQUIRE_FALSE(c[1][1] == c11);
            //REQUIRE_FALSE(c[1][2] == c12);
            //REQUIRE_FALSE(c[1][3] == c13);
            //REQUIRE_FALSE(c[2][0] == c20);
            //REQUIRE_FALSE(c[2][1] == c21);
            //REQUIRE_FALSE(c[2][2] == c22);
            //REQUIRE_FALSE(c[2][3] == c23);
            //REQUIRE_FALSE(c[3][0] == c30);
            //REQUIRE_FALSE(c[3][1] == c31);
            //REQUIRE_FALSE(c[3][2] == c32);
            //REQUIRE_FALSE(c[3][3] == c33);

            // first level verification, which is exactly what the
            // division function does, it does not mean that the
            // division works properly, though
            //
            snap::matrix<double> r(4, 4);
            r = a * t;

            REQUIRE(c[0][0] == r[0][0]);
            REQUIRE(c[0][1] == r[0][1]);
            REQUIRE(c[0][2] == r[0][2]);
            REQUIRE(c[0][3] == r[0][3]);
            REQUIRE(c[1][0] == r[1][0]);
            REQUIRE(c[1][1] == r[1][1]);
            REQUIRE(c[1][2] == r[1][2]);
            REQUIRE(c[1][3] == r[1][3]);
            REQUIRE(c[2][0] == r[2][0]);
            REQUIRE(c[2][1] == r[2][1]);
            REQUIRE(c[2][2] == r[2][2]);
            REQUIRE(c[2][3] == r[2][3]);
            REQUIRE(c[3][0] == r[3][0]);
            REQUIRE(c[3][1] == r[3][1]);
            REQUIRE(c[3][2] == r[3][2]);
            REQUIRE(c[3][3] == r[3][3]);

            REQUIRE(c[0][0] == a00 / b00);
            REQUIRE(c[0][1] == a01 / b01);
            REQUIRE(c[0][2] == a02 / b02);
            REQUIRE(c[0][3] == a03 / b03);
            REQUIRE(c[1][0] == a10 / b10);
            REQUIRE(c[1][1] == a11 / b11);
            REQUIRE(c[1][2] == a12 / b12);
            REQUIRE(c[1][3] == a13 / b13);
            REQUIRE(c[2][0] == a20 / b20);
            REQUIRE(c[2][1] == a21 / b21);
            REQUIRE(c[2][2] == a22 / b22);
            REQUIRE(c[2][3] == a23 / b23);
            REQUIRE(c[3][0] == a30 / b30);
            REQUIRE(c[3][1] == a31 / b31);
            REQUIRE(c[3][2] == a32 / b32);
            REQUIRE(c[3][3] == a33 / b33);
        }

        SECTION("a-=b")
        {
            // setup A
            //
            snap::matrix<double> a(4, 4);

            REQUIRE(a.rows() == 4);
            REQUIRE(a.columns() == 4);

            double const a00(frand());
            double const a01(frand());
            double const a02(frand());
            double const a03(frand());
            double const a10(frand());
            double const a11(frand());
            double const a12(frand());
            double const a13(frand());
            double const a20(frand());
            double const a21(frand());
            double const a22(frand());
            double const a23(frand());
            double const a30(frand());
            double const a31(frand());
            double const a32(frand());
            double const a33(frand());

            a[0][0] = a00;
            a[0][1] = a01;
            a[0][2] = a02;
            a[0][3] = a03;
            a[1][0] = a10;
            a[1][1] = a11;
            a[1][2] = a12;
            a[1][3] = a13;
            a[2][0] = a20;
            a[2][1] = a21;
            a[2][2] = a22;
            a[2][3] = a23;
            a[3][0] = a30;
            a[3][1] = a31;
            a[3][2] = a32;
            a[3][3] = a33;

            REQUIRE(a[0][0] == a00);
            REQUIRE(a[0][1] == a01);
            REQUIRE(a[0][2] == a02);
            REQUIRE(a[0][3] == a03);
            REQUIRE(a[1][0] == a10);
            REQUIRE(a[1][1] == a11);
            REQUIRE(a[1][2] == a12);
            REQUIRE(a[1][3] == a13);
            REQUIRE(a[2][0] == a20);
            REQUIRE(a[2][1] == a21);
            REQUIRE(a[2][2] == a22);
            REQUIRE(a[2][3] == a23);
            REQUIRE(a[3][0] == a30);
            REQUIRE(a[3][1] == a31);
            REQUIRE(a[3][2] == a32);
            REQUIRE(a[3][3] == a33);

            // setup B
            //
            snap::matrix<double> b(4, 4);

            REQUIRE(b.rows() == 4);
            REQUIRE(b.columns() == 4);

            double const b00(frand());
            double const b01(frand());
            double const b02(frand());
            double const b03(frand());
            double const b10(frand());
            double const b11(frand());
            double const b12(frand());
            double const b13(frand());
            double const b20(frand());
            double const b21(frand());
            double const b22(frand());
            double const b23(frand());
            double const b30(frand());
            double const b31(frand());
            double const b32(frand());
            double const b33(frand());

            b[0][0] = b00;
            b[0][1] = b01;
            b[0][2] = b02;
            b[0][3] = b03;
            b[1][0] = b10;
            b[1][1] = b11;
            b[1][2] = b12;
            b[1][3] = b13;
            b[2][0] = b20;
            b[2][1] = b21;
            b[2][2] = b22;
            b[2][3] = b23;
            b[3][0] = b30;
            b[3][1] = b31;
            b[3][2] = b32;
            b[3][3] = b33;

            REQUIRE(b[0][0] == b00);
            REQUIRE(b[0][1] == b01);
            REQUIRE(b[0][2] == b02);
            REQUIRE(b[0][3] == b03);
            REQUIRE(b[1][0] == b10);
            REQUIRE(b[1][1] == b11);
            REQUIRE(b[1][2] == b12);
            REQUIRE(b[1][3] == b13);
            REQUIRE(b[2][0] == b20);
            REQUIRE(b[2][1] == b21);
            REQUIRE(b[2][2] == b22);
            REQUIRE(b[2][3] == b23);
            REQUIRE(b[3][0] == b30);
            REQUIRE(b[3][1] == b31);
            REQUIRE(b[3][2] == b32);
            REQUIRE(b[3][3] == b33);

            // run operation A -= B
            //
            a -= b;

            // this could fail if one of bXX is 0.0
            //
            //REQUIRE(a[0][0] == a00);
            //REQUIRE(a[0][1] == a01);
            //REQUIRE(a[0][2] == a02);
            //REQUIRE(a[0][3] == a03);
            //REQUIRE(a[1][0] == a10);
            //REQUIRE(a[1][1] == a11);
            //REQUIRE(a[1][2] == a12);
            //REQUIRE(a[1][3] == a13);
            //REQUIRE(a[2][0] == a20);
            //REQUIRE(a[2][1] == a21);
            //REQUIRE(a[2][2] == a22);
            //REQUIRE(a[2][3] == a23);
            //REQUIRE(a[3][0] == a30);
            //REQUIRE(a[3][1] == a31);
            //REQUIRE(a[3][2] == a32);
            //REQUIRE(a[3][3] == a33);

            REQUIRE(b[0][0] == b00);
            REQUIRE(b[0][1] == b01);
            REQUIRE(b[0][2] == b02);
            REQUIRE(b[0][3] == b03);
            REQUIRE(b[1][0] == b10);
            REQUIRE(b[1][1] == b11);
            REQUIRE(b[1][2] == b12);
            REQUIRE(b[1][3] == b13);
            REQUIRE(b[2][0] == b20);
            REQUIRE(b[2][1] == b21);
            REQUIRE(b[2][2] == b22);
            REQUIRE(b[2][3] == b23);
            REQUIRE(b[3][0] == b30);
            REQUIRE(b[3][1] == b31);
            REQUIRE(b[3][2] == b32);
            REQUIRE(b[3][3] == b33);

            REQUIRE(a[0][0] == a00 - b00);
            REQUIRE(a[0][1] == a01 - b01);
            REQUIRE(a[0][2] == a02 - b02);
            REQUIRE(a[0][3] == a03 - b03);
            REQUIRE(a[1][0] == a10 - b10);
            REQUIRE(a[1][1] == a11 - b11);
            REQUIRE(a[1][2] == a12 - b12);
            REQUIRE(a[1][3] == a13 - b13);
            REQUIRE(a[2][0] == a20 - b20);
            REQUIRE(a[2][1] == a21 - b21);
            REQUIRE(a[2][2] == a22 - b22);
            REQUIRE(a[2][3] == a23 - b23);
            REQUIRE(a[3][0] == a30 - b30);
            REQUIRE(a[3][1] == a31 - b31);
            REQUIRE(a[3][2] == a32 - b32);
            REQUIRE(a[3][3] == a33 - b33);
        }
    }
}


// vim: ts=4 sw=4 et
