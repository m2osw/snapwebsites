// Snap Websites Servers -- matrix class
// Copyright (c) 2018  Made to Order Software Corp.  All Rights Reserved
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
#pragma once

// C++ lib
//
#include <cctype>
#include <iostream>
#include <stdexcept>
#include <vector>

// QImage
//
#include <QImage>

#include <sstream>


namespace snap
{

template<typename T, typename SIZE = std::size_t>
class matrix
{
public:
    typedef T           value_type;
    typedef SIZE        size_type;

    // The color weights are used to convert RGB to Luminance
    // With a factor, it's possible to change the color toward or
    // away from the perfect Luminance if the color is not already
    // a gray color (see the saturation() function.)
    //
    // see https://www.opengl.org/archives/resources/code/samples/advanced/advanced97/notes/node140.html
    //
    static value_type constexpr     RED_WEIGHT   = 0.3086;
    static value_type constexpr     GREEN_WEIGHT = 0.6094;
    static value_type constexpr     BLUE_WEIGHT  = 0.0820;

    class element_ref
    {
    public:
        element_ref(matrix<T, SIZE> & m, size_type j, size_type i)
            : f_matrix(m)
            //, f_j(j)
            //, f_i(i)
            , f_offset(i + j * m.columns())
        {
            if(i >= m.f_columns)
            {
                throw std::out_of_range("used [] operator with too large a row number");
            }
        }

        element_ref & operator = (value_type const v)
        {
            f_matrix.f_vector[f_offset] = v;

            return *this;
        }

        bool operator == (value_type const v) const
        {
            return f_matrix.f_vector[f_offset] == v;
        }

        bool operator != (value_type const v) const
        {
            return f_matrix.f_vector[f_offset] != v;
        }

        bool operator < (value_type const v) const
        {
            return f_matrix.f_vector[f_offset] < v;
        }

        bool operator <= (value_type const v) const
        {
            return f_matrix.f_vector[f_offset] <= v;
        }

        bool operator > (value_type const v) const
        {
            return f_matrix.f_vector[f_offset] > v;
        }

        bool operator >= (value_type const v) const
        {
            return f_matrix.f_vector[f_offset] >= v;
        }

        operator T () const
        {
            return f_matrix.f_vector[f_offset];
        }

    private:
        matrix<T, SIZE> &   f_matrix;
        size_type           f_offset;
    };

    class row_ref
    {
    public:
        row_ref(matrix<T, SIZE> & m, size_type j)
            : f_matrix(m)
            , f_j(j)
        {
            if(j >= m.f_rows)
            {
                throw std::out_of_range("used [] operator with too large a row number");
            }
        }

        element_ref operator [] (size_type i)
        {
            element_ref r(f_matrix, f_j, i);
            return r;
        }

        value_type operator [] (size_type i) const
        {
            return f_matrix.f_vector[i + f_j * f_matrix.columns()];
        }

    private:
        matrix<T, SIZE> &   f_matrix;
        size_type           f_j;        // row
    };

    class const_row_ref
    {
    public:
        const_row_ref(matrix<T, SIZE> const & m, size_type j)
            : f_matrix(m)
            , f_j(j)
        {
            if(j >= m.f_rows)
            {
                throw std::out_of_range("used [] operator with too large a row number");
            }
        }

        value_type operator [] (size_type i) const
        {
            if(i >= f_matrix.f_columns)
            {
                throw std::out_of_range("used [] operator with too large a row number");
            }
            return f_matrix.f_vector[i + f_j * f_matrix.columns()];
        }

    private:
        matrix<T, SIZE> const & f_matrix;
        size_type               f_j;        // row
    };

    matrix<T, SIZE>()
    {
    }

    matrix<T, SIZE>(size_type rows, size_type columns)
        : f_rows(rows)
        , f_columns(columns)
        , f_vector(rows * columns)
    {
        initialize();
    }

    template<typename V, typename SZ>
    matrix<T, SIZE>(matrix<V, SZ> const & rhs)
        : f_rows(rhs.f_rows)
        , f_columns(rhs.f_columns)
        , f_vector(rhs.f_vector.size())
    {
        initialize();
    }

    template<typename V, typename SZ>
    matrix<T, SIZE> & operator = (matrix<V, SZ> const & rhs)
    {
        if(this != &rhs)
        {
            f_rows = rhs.f_rows;
            f_columns = rhs.f_columns;
            f_vector = rhs.f_vector;
        }
    }

    template<typename V, typename SZ>
    matrix<T, SIZE> & operator = (matrix<V, SZ> const && rhs)
    {
        if(this != &rhs)
        {
            f_rows = rhs.f_rows;
            f_columns = rhs.f_columns;
            f_vector = rhs.f_vector;
        }
    }

    size_type rows() const
    {
        return f_rows;
    }

    size_type columns() const
    {
        return f_columns;
    }

    void swap(matrix<T, SIZE> & rhs)
    {
        std::swap(f_rows, rhs.f_rows);
        std::swap(f_columns, rhs.f_columns);
        f_vector.swap(rhs.f_vector);
    }

    void initialize()
    {
        if(f_rows == f_columns)
        {
            identity();
        }
        else
        {
            clear();
        }
    }

    void clear()
    {
        std::fill(f_vector.begin(), f_vector.end(), value_type());
    }

    void identity()
    {
        for(size_type j(0); j < f_rows; ++j)
        {
            size_type const joffset(j * f_columns);
            for(size_type i(0); i < f_columns; ++i)
            {
                f_vector[i + joffset] =
                    i == j
                        ? static_cast<value_type>(1)
                        : value_type();
            }
        }
    }

    row_ref operator [] (size_type row)
    {
        row_ref r(*this, row);
        return r;
    }

    const_row_ref operator [] (size_type row) const
    {
        const_row_ref r(*this, row);
        return r;
    }

    template<class S>
    matrix<T, SIZE> operator * (S const & scalar) const
    {
        matrix<T, SIZE> t(*this);
        return t *= scalar;
    }

//    At this point I don't know how to make that work...
//    template<class S, typename V, typename SZ> friend
//    matrix<V, SZ> operator * (S const & scalar, matrix<V, SZ> const & m)
//    {
//        return m * scalar;
//    }

    template<class S>
    matrix<T, SIZE> & operator *= (S const & scalar)
    {
        for(size_type j(0); j < f_rows; ++j)
        {
            size_type const joffset(j * f_columns);
            for(size_type i(0); i < f_columns; ++i)
            {
                f_vector[i + joffset] *= scalar;
            }
        }

        return *this;
    }

    template<typename V, typename SZ>
    matrix<T, SIZE> operator * (matrix<V, SZ> const & m) const
    {
        if(f_columns != m.f_rows)
        {
            // enhance that support with time
            // (i.e. the number of columns of one matrix has to match the
            // number of rows of the other, they don't need to be square
            // matrices of the same size)
            //
            throw std::runtime_error("matrices of incompatible sizes for a multiplication");
        }

        // temporary buffer
        //
        matrix<T, SIZE> t(f_rows, m.f_columns);

        for(size_type j(0); j < f_rows; ++j)
        {
            size_type const joffset(j * f_columns);
            for(size_type i(0); i < m.f_columns; ++i)
            {
                value_type sum = value_type();

                // k goes from 0 to (f_columns == m.f_rows)
                //
                for(size_type k(0); k < m.f_rows; ++k)     // sometimes it's X and sometimes it's Y
                {
                    sum += f_vector[k + joffset]
                       * m.f_vector[i + k * m.f_columns];
                }
                t.f_vector[i + j * t.f_columns] = sum;
            }
        }

        return t;
    }

    template<class V>
    matrix<T, SIZE> & operator *= (matrix<V> const & m)
    {
        return *this = *this * m;
    }

    template<class S>
    matrix<T, SIZE> operator / (S const & scalar) const
    {
        matrix<T, SIZE> t(*this);
        return t /= scalar;
    }

    template<class S>
    matrix<T, SIZE> & operator /= (S const & scalar)
    {
        for(size_type y(0); y < f_rows; ++y)
        {
            for(size_type x(0); x < f_columns; ++x)
            {
                size_type const idx(x + y * f_columns);
                f_vector[idx] /= scalar;
            }
        }

        return *this;
    }

    template<typename V, typename SZ>
    matrix<T, SIZE> operator / (matrix<V, SZ> const & m) const
    {
        // temporary buffer
        //
        matrix<T, SIZE> t(m);
        t.inverse();
        return *this * t;
    }

    template<typename V, typename SZ>
    matrix<T, SIZE> & operator /= (matrix<V, SZ> const & m)
    {
        matrix<T, SIZE> t(m);
        t.inverse();
        return *this *= t;
    }

    /** \brief Compute the inverse of `this` matrix if possible.
     *
     * This function computes the matrix determinant to see whether
     * it can be inverted. If so, it computes the inverse and becomes
     * that inverse.
     *
     * The function returns false if the inverse can't be calculated
     * and the matrix remains unchanged.
     *
     * $$A^{-1} = {1 \over det(A)} adj(A)$$
     *
     * \return true if the inverse was successful.
     */
    bool inverse()
    {
        if(f_rows != 4
        || f_columns != 4)
        {
            double const det(determinant());
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
            if(det == static_cast<value_type>(0))
            {
                return false;
            }
#pragma GCC diagnostic pop
            snap::matrix<double> adj(adjugate());

            *this = adj * (static_cast<value_type>(1) / det);
            return true;
        }

        // the following is very specific to a 4x4 matrix...
        //
        value_type temp[4][8], *r[4], m[5];

        r[0] = temp[0];
        r[1] = temp[1];
        r[2] = temp[2];
        r[3] = temp[3];

        temp[0][0] = f_vector[0 + 0 * 4];
        temp[0][1] = f_vector[0 + 1 * 4];
        temp[0][2] = f_vector[0 + 2 * 4];
        temp[0][3] = f_vector[0 + 3 * 4];

        temp[1][0] = f_vector[1 + 0 * 4];
        temp[1][1] = f_vector[1 + 1 * 4];
        temp[1][2] = f_vector[1 + 2 * 4];
        temp[1][3] = f_vector[1 + 3 * 4];

        temp[2][0] = f_vector[2 + 0 * 4];
        temp[2][1] = f_vector[2 + 1 * 4];
        temp[2][2] = f_vector[2 + 2 * 4];
        temp[2][3] = f_vector[2 + 3 * 4];

        temp[3][0] = f_vector[3 + 0 * 4];
        temp[3][1] = f_vector[3 + 1 * 4];
        temp[3][2] = f_vector[3 + 2 * 4];
        temp[3][3] = f_vector[3 + 3 * 4];

        // can we do it?!
        value_type abs_a = fabsf(r[3][0]);
        value_type abs_b = fabsf(r[2][0]);
        if(abs_a > abs_b) {
            value_type *swap = r[3];
            r[3] = r[2];
            r[2] = swap;
            abs_b = abs_a;
        }

        abs_a = fabsf(r[1][0]);
        if(abs_b > abs_a)
        {
            value_type *swap = r[2];
            r[2] = r[1];
            r[1] = swap;
            abs_a = abs_b;
        }

        abs_b = fabsf(r[0][0]);
        if(abs_a > abs_b)
        {
            value_type *swap = r[1];
            r[1] = r[0];
            r[0] = swap;
            abs_b = abs_a;
        }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
        if(abs_b == 0.0f)
        {
            return false;
        }

        temp[0][4] = temp[1][5] = temp[2][6] = temp[3][7] = 1.0f;
        temp[0][5] =
        temp[0][6] =
        temp[0][7] =
        temp[1][4] =
        temp[1][6] =
        temp[1][7] =
        temp[2][4] =
        temp[2][5] =
        temp[2][7] =
        temp[3][4] =
        temp[3][5] =
        temp[3][6] = 0.0f;

        // first elimination
        m[4] = 1.0f / r[0][0];
        m[1] = r[1][0] * m[4];
        m[2] = r[2][0] * m[4];
        m[3] = r[3][0] * m[4];

        m[4] = r[0][1];
        if(m[4] != 0.0)
        {
            r[1][1] -= m[1] * m[4];
            r[2][1] -= m[2] * m[4];
            r[3][1] -= m[3] * m[4];
        }

        m[4] = r[0][2];
        if(m[4] != 0.0)
        {
            r[1][2] -= m[1] * m[4];
            r[2][2] -= m[2] * m[4];
            r[3][2] -= m[3] * m[4];
        }

        m[4] = r[0][3];
        if(m[4] != 0.0)
        {
            r[1][3] -= m[1] * m[4];
            r[2][3] -= m[2] * m[4];
            r[3][3] -= m[3] * m[4];
        }

        m[4] = r[0][4];
        if(m[4] != 0.0)
        {
            r[1][4] -= m[1] * m[4];
            r[2][4] -= m[2] * m[4];
            r[3][4] -= m[3] * m[4];
        }

        m[4] = r[0][5];
        if(m[4] != 0.0)
        {
            r[1][5] -= m[1] * m[4];
            r[2][5] -= m[2] * m[4];
            r[3][5] -= m[3] * m[4];
        }

        m[4] = r[0][6];
        if(m[4] != 0.0)
        {
            r[1][6] -= m[1] * m[4];
            r[2][6] -= m[2] * m[4];
            r[3][6] -= m[3] * m[4];
        }

        m[4] = r[0][7];
        if(m[4] != 0.0)
        {
            r[1][7] -= m[1] * m[4];
            r[2][7] -= m[2] * m[4];
            r[3][7] -= m[3] * m[4];
        }

        // can we do it?!
        abs_a = fabsf(r[3][1]);
        abs_b = fabsf(r[2][1]);
        if(abs_a > abs_b)
        {
            value_type *swap = r[3];
            r[3] = r[2];
            r[2] = swap;
            abs_b = abs_a;
        }
        abs_a = fabsf(r[1][1]);
        if(abs_b > abs_a)
        {
            value_type *swap = r[2];
            r[2] = r[1];
            r[1] = swap;
            abs_a = abs_b;
        }

        if(abs_a == 0.0f)
        {
            return false;
        }

        // second elimination
        m[4] = 1.0f / r[1][1];

        m[2] = r[2][1] * m[4];
        m[3] = r[3][1] * m[4];

        if(m[2] != 0.0f)
        {
            r[2][2] -= m[2] * r[1][2];
            r[2][3] -= m[2] * r[1][3];
        }

        if(m[3] != 0.0f)
        {
            r[3][2] -= m[3] * r[1][2];
            r[3][3] -= m[3] * r[1][3];
        }

        m[4] = r[1][4];
        if(m[4] != 0.0f)
        {
            r[2][4] -= m[2] * m[4];
            r[3][4] -= m[3] * m[4];
        }

        m[4] = r[1][5];
        if(m[4] != 0.0f)
        {
            r[2][5] -= m[2] * m[4];
            r[3][5] -= m[3] * m[4];
        }

        m[4] = r[1][6];
        if(m[4] != 0.0f)
        {
            r[2][6] -= m[2] * m[4];
            r[3][6] -= m[3] * m[4];
        }

        m[4] = r[1][7];
        if(m[4] != 0.0f)
        {
            r[2][7] -= m[2] * m[4];
            r[3][7] -= m[3] * m[4];
        }

        // can we do it?!
        abs_a = fabsf(r[3][2]);
        abs_b = fabsf(r[2][2]);
        if(abs_a > abs_b)
        {
            value_type *swap = r[3];
            r[3] = r[2];
            r[2] = swap;
            abs_b = abs_a;
        }

        if(abs_b == 0.0f)
        {
            return false;
        }

        // third elimination
        m[3] = r[3][2] / r[2][2];

        r[3][3] -= m[3] * r[2][3];
        r[3][4] -= m[3] * r[2][4];
        r[3][5] -= m[3] * r[2][5];
        r[3][6] -= m[3] * r[2][6];
        r[3][7] -= m[3] * r[2][7];

        // can we do it?!
        if(r[3][3] == 0.0f) {
            return false;
        }
#pragma GCC diagnostic pop

        // back substitute
        /* 3 */
        m[4] = 1.0f / r[3][3];
        r[3][4] *= m[4];
        r[3][5] *= m[4];
        r[3][6] *= m[4];
        r[3][7] *= m[4];

        /* 2 */
        m[2] = r[2][3];
        m[4] = 1.0f / r[2][2];
        r[2][4] = m[4] * (r[2][4] - r[3][4] * m[2]);
        r[2][5] = m[4] * (r[2][5] - r[3][5] * m[2]);
        r[2][6] = m[4] * (r[2][6] - r[3][6] * m[2]);
        r[2][7] = m[4] * (r[2][7] - r[3][7] * m[2]);

        m[1] = r[1][3];
        r[1][4] -= r[3][4] * m[1];
        r[1][5] -= r[3][5] * m[1];
        r[1][6] -= r[3][6] * m[1];
        r[1][7] -= r[3][7] * m[1];

        m[0] = r[0][3];
        r[0][4] -= r[3][4] * m[0];
        r[0][5] -= r[3][5] * m[0];
        r[0][6] -= r[3][6] * m[0];
        r[0][7] -= r[3][7] * m[0];

        /* 1 */
        m[1] = r[1][2];
        m[4] = 1.0f / r[1][1];
        r[1][4] = m[4] * (r[1][4] - r[2][4] * m[1]);
        r[1][5] = m[4] * (r[1][5] - r[2][5] * m[1]);
        r[1][6] = m[4] * (r[1][6] - r[2][6] * m[1]);
        r[1][7] = m[4] * (r[1][7] - r[2][7] * m[1]);

        m[0] = r[0][2];
        r[0][4] -= r[2][4] * m[0];
        r[0][5] -= r[2][5] * m[0];
        r[0][6] -= r[2][6] * m[0];
        r[0][7] -= r[2][7] * m[0];

        /* 0 */
        m[0] = r[0][1];
        m[4] = 1.0f / r[0][0];
        r[0][4] = m[4] * (r[0][4] - r[1][4] * m[0]);
        r[0][5] = m[4] * (r[0][5] - r[1][5] * m[0]);
        r[0][6] = m[4] * (r[0][6] - r[1][6] * m[0]);
        r[0][7] = m[4] * (r[0][7] - r[1][7] * m[0]);

        // save in destination (we need to put that in the back substitution)
        f_vector[0 + 0 * 4] = r[0][4];
        f_vector[0 + 1 * 4] = r[0][5];
        f_vector[0 + 2 * 4] = r[0][6];
        f_vector[0 + 3 * 4] = r[0][7];

        f_vector[1 + 0 * 4] = r[1][4];
        f_vector[1 + 1 * 4] = r[1][5];
        f_vector[1 + 2 * 4] = r[1][6];
        f_vector[1 + 3 * 4] = r[1][7];

        f_vector[2 + 0 * 4] = r[2][4];
        f_vector[2 + 1 * 4] = r[2][5];
        f_vector[2 + 2 * 4] = r[2][6];
        f_vector[2 + 3 * 4] = r[2][7];

        f_vector[3 + 0 * 4] = r[3][4];
        f_vector[3 + 1 * 4] = r[3][5];
        f_vector[3 + 2 * 4] = r[3][6];
        f_vector[3 + 3 * 4] = r[3][7];

        return true;
    }

    /** \brief Reduce a matrix by removing one row and one column.
     *
     * This function creates a minor duplicate of this matrix with column i
     * and row j removed.
     *
     * The minor is denoted:
     *
     * $$M_{ij}$$
     *
     * It is a matrix built from $A$ without column `i` and row `j`.
     *
     * \note
     * The matrix must at least be a 2x2 matrix.
     *
     * \note
     * There is a "minor" macro so I named this function minor_matrix().
     *
     * \param[in] row  The row to remove.
     * \param[in] column  The column to remove.
     *
     * \return The requested minor matrix.
     */
    matrix<T, SIZE> minor_matrix(size_type row, size_type column) const
    {
        if(f_rows < 2
        || f_columns < 2)
        {
            throw std::runtime_error("minor_matrix() must be called with a matrix which is at least 2x2, although it does not need to be a square matrix");
        }

        matrix<T, SIZE> p(f_rows - 1, f_columns - 1);

        // we loop using p sizes
        // the code below ensures the correct input is retrieved
        //
        // di -- destination column
        // dj -- destination row
        // si -- source column
        // sj -- source row
        //
        for(size_type dj(0); dj < p.f_rows; ++dj)
        {
            for(size_type di(0); di < p.f_columns; ++di)
            {
                // here we have 4 cases:
                //
                //     a11 a12 | a13 | a14 a15
                //     a21 a22 | a23 | a24 a25
                //     --------+-----+--------
                //     a31 a32 | a33 | a34 a35
                //     --------+-----+--------
                //     a41 a42 | a43 | a44 a45
                //     a51 a52 | a53 | a54 a55
                //
                // assuming 'row' and 'column' are set to 3 and 3, then
                // the graph shows the 4 cases as the 4 corners, the
                // center lines will be removed so they are ignored in
                // the source
                //
                size_type const si(di < column ? di : di + 1);
                size_type const sj(dj < row    ? dj : dj + 1);

                p.f_vector[di + dj * p.f_columns] = f_vector[si + sj * f_columns];
            }
        }

        return p;
    }

    /** \brief Calculate the determinant of this matrix.
     *
     * This function calculates the determinant of this matrix:
     *
     * $$det(A) = \sum_{\sigma \in S_n} \Big( sgn(\sigma) \prod_{i=1}^{n} a_{i,\sigma_i} \Big)$$
     *
     * The function is implemented using a recursive set of calls. It
     * knows how to calculate a 2x2 matrix. All the ohers use recursive
     * calls to calculate the final result.
     *
     * Let's say you have a 3x3 matrix like this:
     *
     * \code
     *     | a11 a12 a13 |
     *     | a21 a22 a23 |
     *     | a31 a32 a33 |
     * \endcode
     *
     * If first calculates the determinant of the 2x2 matrix:
     *
     * \code
     *     | a22 a23 | = a22 x a33 - a23 x a32
     *     | a32 a33 |
     * \endcode
     *
     * Then multiply that determinant by `a11`.
     *
     * Next it calculates the determinant of the 2x2 matrix:
     *
     * \code
     *     | a21 a23 | = a21 x a33 - a23 x a31
     *     | a31 a33 |
     * \endcode
     *
     * Then multiply that determinant by `a12` and multiply by -1.
     *
     * Finally, it calculates the last 2x2 matrix at the bottom left corner.
     *
     * \code
     *     | a21 a22 | = a21 x a32 - a22 x a31
     *     | a31 a32 |
     * \endcode
     *
     * Then multiply that last determinant by `a13`.
     *
     * Finally we sum all three results and that's our determinant for a
     * 3x3 matrix.
     *
     * The determinant of a 4x4 matrix will be calculated in a similar
     * way by also calculating the determin of all the 3x3 matrices
     * defined under the first row.
     *
     * Source: https://en.wikipedia.org/wiki/Determinant
     *
     * \exception runtime_error
     * If the matrix is not a square matrix, raise a runtime_error exception.
     *
     * \return The determinant value.
     */
    value_type determinant() const
    {
        if(f_rows != f_columns)
        {
            throw std::runtime_error("determinant can only be calculated for square matrices");
        }

        if(f_columns == 1)
        {
            return f_vector[0];
        }

        if(f_columns == 2)
        {
            return f_vector[0 + 0 * 2] * f_vector[1 + 1 * 2]
                 - f_vector[1 + 0 * 2] * f_vector[0 + 1 * 2];
        }

        value_type determinant = value_type();

        value_type sign = static_cast<value_type>(1);
        for(size_type c(0); c < f_columns; ++c)
        {
            // create a minor matrix
            //
            matrix<T, SIZE> p(minor_matrix(0, c));

            // add to the determinant for that column
            // (number of row 0 column 'c')
            //
            determinant += sign
                         * f_vector[c + 0 * f_columns]
                         * p.determinant();

            // swap the sign
            //
            sign *= static_cast<value_type>(-1);
        }

        return determinant;
    }

    /** \brief Swap the rows and columns of a matrix.
     *
     * This function returns the transpose of this matrix.
     *
     * Generally noted as:
     *
     * $$A^T$$
     *
     * The definition of the transpose is:
     *
     * $$[A^T]_{ij} = [A]_{ji}$$
     *
     * The resulting matrix has a number of columns equal to 'this' matrix
     * rows and vice versa.
     *
     * \return A new matrix representing the transpose of 'this' matrix.
     */
    matrix<T, SIZE> transpose() const
    {
        // 'm' has its number of rows and columns swapped compared
        // to 'this'
        matrix<T, SIZE> m(f_columns, f_rows);

        for(size_type j(0); j < f_rows; ++j)
        {
            for(size_type i(0); i < f_columns; ++i)
            {
                // we could also have used "j + i * f_rows" on the left
                // but I think it's more confusing
                //
                m.f_vector[j + i * m.f_columns] = f_vector[i + j * f_columns];
            }
        }

        return m;
    }

    /** \brief This function calculates the adjugate of this matrix.
     *
     * \return The adjugate of this matrix.
     */
    matrix<T, SIZE> adjugate() const
    {
        if(f_rows != f_columns)
        {
            // is that true?
            //
            throw std::runtime_error("adjugate can only be calculated for square matrices");
        }

        matrix<T, SIZE> r(f_rows, f_columns);

        // det(A) when A is 1x1 equals | 1 |
        // which is the default 'r'
        //
        if(f_columns != 1)
        {
            //if(f_columns == 2)
            //{
            //    // the 2x2 matrix is handled as a special case just so it goes
            //    // faster but not so much more than that
            //    //
            //    //   adj | a b | = |  d -b |
            //    //       | c d |   | -c  a |
            //    //
            //    r.f_vector[0 + 0 * 2] =  f_vector[1 + 1 * 2];
            //    r.f_vector[1 + 0 * 2] = -f_vector[1 + 0 * 2];
            //    r.f_vector[0 + 1 * 2] = -f_vector[0 + 1 * 2];
            //    r.f_vector[1 + 1 * 2] =  f_vector[0 + 0 * 2];
            //}
            //else
            {
                // for larger matrices we use a loop and calculate the determinant
                // for each new value with the "rest" of the matrix at that point
                //
                for(size_type j(0); j < f_rows; ++j)
                {
                    for(size_type i(0); i < f_columns; ++i)
                    {
                        matrix<T, SIZE> p(minor_matrix(j, i));
                        r.f_vector[i + j * r.f_columns] = static_cast<value_type>(((i + j) & 1) == 0 ? 1 : -1) * p.determinant();
                    }
                }

                return r.transpose();
            }
        }

        return r;
    }

    /** \brief Add a scale to 'this' matrix.
     *
     * This function adds the specified scalar to the matrix. This adds
     * the specified amount to all the elements of the matrix.
     *
     * $$[A]_{ij} \leftarrow [A]_{ij} + scalar$$
     *
     * \param[in] scalar  The scalar to add to this matrix.
     */
    template<class S>
    matrix<T, SIZE> operator + (S const & scalar) const
    {
        matrix<T, SIZE> t(*this);
        return t += scalar;
    }

    template<class S>
    matrix<T, SIZE> & operator += (S const & scalar)
    {
        for(size_type y(0); y < f_rows; ++y)
        {
            for(size_type x(0); x < f_columns; ++x)
            {
                size_type const idx(x + y * f_columns);
                f_vector[idx] += scalar;
            }
        }

        return *this;
    }

    template<typename V, typename SZ>
    matrix<T, SIZE> operator + (matrix<V, SZ> const & m) const
    {
        matrix<T, SIZE> t(*this);
        return t += m;
    }

    template<typename V, typename SZ>
    matrix<T, SIZE> & operator += (matrix<V, SZ> const & m)
    {
        if(f_rows    != m.f_rows
        || f_columns != m.f_columns)
        {
            throw std::runtime_error("matrices of incompatible sizes for an addition");
        }

        for(size_type y(0); y < f_rows; ++y)
        {
            for(size_type x(0); x < f_columns; ++x)
            {
                size_type const idx(x + y * f_columns);
                f_vector[idx] += m.f_vector[idx];
            }
        }

        return *this;
    }

    template<class S>
    matrix<T, SIZE> operator - (S const & scalar) const
    {
        matrix<T, SIZE> t(*this);
        return t -= scalar;
    }

    template<class S>
    matrix<T, SIZE> & operator -= (S const & scalar)
    {
        matrix<T, SIZE> t(f_rows, f_columns);

        for(size_type y(0); y < f_rows; ++y)
        {
            for(size_type x(0); x < f_columns; ++x)
            {
                size_type const idx(x + y * f_columns);
                f_vector[idx] -= scalar;
            }
        }

        return *this;
    }

    template<typename V, typename SZ>
    matrix<T, SIZE> operator - (matrix<V, SZ> const & m) const
    {
        matrix<T, SIZE> t(*this);
        return t -= m;
    }

    template<class V, typename SZ>
    matrix<T, SIZE> & operator -= (matrix<V, SZ> const & m)
    {
        if(f_rows    != m.f_rows
        || f_columns != m.f_columns)
        {
            throw std::runtime_error("matrices of incompatible sizes for a subtraction");
        }

        for(size_type y(0); y < f_rows; ++y)
        {
            for(size_type x(0); x < f_columns; ++x)
            {
                size_type const idx(x + y * f_columns);
                f_vector[idx] -= m.f_vector[idx];
            }
        }

        return *this;
    }


    /** \brief Apply a scaling factor to this matrix.
     *
     * This function multiplies 'this' matrix by a scaling factor and
     * returns the result. 'this' does not get changed.
     *
     * The scale matrix looks like:
     *
     * $$
     * \begin{bmatrix}
     *       brightness_r & 0 & 0 & 0
     *    \\ 0 & brightness_g & 0 & 0
     *    \\ 0 & 0 & brightness_b & 0
     *    \\ 0 & 0 & 0 & 1
     * \end{bmatrix}
     * $$
     *
     * The 'r', 'g', 'b' indices represent the RGB colors if one wants to
     * scale just one color at a time although this function only offers
     * to set all of the fields to the same value \p s.
     *
     * See:
     * http://www.graficaobscura.com/matrix/index.html
     *
     * \param[in] b  The scaling factor to apply to this matrix.
     *
     * \return 'this' x 'brightness'
     */
    template<typename S>
    matrix<T, SIZE> brightness(S const b) const
    {
        if(f_rows    != 4
        || f_columns != 4)
        {
            throw std::runtime_error("scale() is only for 4x4 matrices at this time");
        }

        matrix<T, SIZE> m(4, 4);
        m[0][0] = b;
        m[1][1] = b;
        m[2][2] = b;

        return *this * m;
    }


    /** \brief Apply an RGB color saturation to this matrix.
     *
     * This function applies the saturation matrix defined below to
     * 'this' matrix. When the saturation parameter is set to zero (0)
     * the function transforms the all colors to gray. When the saturation
     * is set to one (1), the color does not get changed.
     *
     * The saturation matrix:
     *
     * $$
     * \begin{bmatrix}
     *     0.3086 \times ( 1 - saturation) + saturation  & 0.3086 \times ( 1 - saturation)               & 0.3086 \times ( 1 - saturation)                & 0
     *  \\ 0.6094 \times ( 1 - saturation)               & 0.6094 \times ( 1 - saturation) + saturation  & 0.6094 \times ( 1 - saturation)                & 0
     *  \\ 0.0820 \times ( 1 - saturation)               & 0.0820 \times ( 1 - saturation)               & 0.0820 \times ( 1 - saturation) + saturation   & 0
     *  \\ 0                                             & 0                                             & 0                                              & 1
     * \end{bmatrix}
     * $$
     *
     * See:
     * http://www.graficaobscura.com/matrix/index.html
     *
     * \param[in] s  How close or far to correct the color toward saturation.
     *
     * \return 'this' x 'scale'
     */
    template<typename S>
    matrix<T, SIZE> saturation(S const s) const
    {
        if(f_rows    != 4
        || f_columns != 4)
        {
            throw std::runtime_error("scale() is only for 4x4 matrices at this time");
        }

        matrix<T, SIZE> m(4, 4);

        value_type const ns(static_cast<value_type>(s));
        value_type const os(static_cast<value_type>(1) - static_cast<value_type>(s));

        m[0][0] = RED_WEIGHT * os + ns;
        m[0][1] = RED_WEIGHT * os;
        m[0][2] = RED_WEIGHT * os;

        m[1][0] = GREEN_WEIGHT * os;
        m[1][1] = GREEN_WEIGHT * os + ns;
        m[1][2] = GREEN_WEIGHT * os;

        m[2][0] = BLUE_WEIGHT * os;
        m[2][1] = BLUE_WEIGHT * os;
        m[2][2] = BLUE_WEIGHT * os + ns;

        return *this * m;
    }


    /** \brief Apply a hue correction.
     *
     * This function applies a hue correction to 'this' matrix.
     *
     * The hue correction makes use of the rotation around the red
     * and green axis, then a skew to take luminace in account.
     * At that point it rotates the color. Finally, the function
     * reverses the skew and rotate back around the green and red
     * axis.
     *
     * The following is the list of matrices used to rotate the hue:
     *
     * Rotation around the Red axis (Rr):
     *
     * $$
     * R_r =
     * \begin{bmatrix}
     *       1 &  0                  & 0                  & 0
     *    \\ 0 &  {1 \over \sqrt 2 } & {1 \over \sqrt 2 } & 0
     *    \\ 0 & -{1 \over \sqrt 2 } & {1 \over \sqrt 2 } & 0
     *    \\ 0 &  0                  & 0                  & 1
     * \end{bmatrix}
     * $$
     *
     * \note
     * The $1 \over \sqrt 2$ is $sin ( \pi \over 4 )$ and
     * $cos ( \pi \over 4 )$.
     *
     * Rotation around the Green axis (Rg):
     *
     * $$
     * R_g =
     * \begin{bmatrix}
     *       1 & 0 & 0 & 0
     *    \\ 0 & {\sqrt 2 \over \sqrt 3} & -{1 \over \sqrt 3} & 0
     *    \\ 0 & {1 \over \sqrt 3} & {\sqrt 2 \over \sqrt 3} & 0
     *    \\ 0 & 0 & 0 & 1
     * \end{bmatrix}
     * $$
     *
     * \note
     * The $\sqrt 2 \over \sqrt 3$ and $1 \over \sqrt 3$ are sin and
     * cos as well. These take the first rotation in account (i.e.
     * so it is again a 45° angle but applied after the 45° angle
     * applied to the red axis.)
     *
     * Combine both rotations:
     *
     * $$
     * R_{rg} = R_r R_g
     * $$
     *
     * Since colors are linear but not at a similar angle, we want to
     * unskew them for which we need to use the luminance. So here we
     * compute the luminance of the color.
     *
     * $$
     * \begin{bmatrix}
     *        L_r
     *     \\ L_g
     *     \\ L_b
     *     \\ 0
     * \end{bmatrix}
     * =
     * R_{rg}
     * \begin{bmatrix}
     *        W_r
     *     \\ W_g
     *     \\ W_b
     *     \\ 0
     * \end{bmatrix}
     * $$
     *
     * Now we define a rotation matrix for the blue axis. This one uses
     * a variable as the angle. This is the actual rotation angle which
     * can go from 0 to $2 \pi$:
     *
     * $$
     * R_b =
     * \begin{bmatrix}
     *       cos( \theta )  & sin( \theta ) & 0 & 0
     *    \\ -sin( \theta ) & cos( \theta ) & 0 & 0
     *    \\ 0              & 0             & 1 & 0
     *    \\ 0              & 0             & 0 & 1
     * \end{bmatrix}
     * $$
     *
     * Now we have all the matrices to caculate the hue rotation
     * of all the components of an image:
     *
     * $$
     * H = R_{rg} S R_b S^{-1} R_{rg}^{-1}
     * $$
     *
     * $H$ can then be used as in:
     *
     * $$
     * \begin{bmatrix}
     *      R'
     *   \\ G'
     *   \\ B'
     * \end{bmatrix}
     * =
     * H
     * \begin{bmatrix}
     *      R
     *   \\ G
     *   \\ B
     * \end{bmatrix}
     * $$
     *
     * See:
     * http://www.graficaobscura.com/matrix/index.html
     *
     * \param[in] h  The amount of rotation, an angle in radian.
     *
     * \return 'this' rotated around the hue axis by \p h
     */
    template<typename S>
    matrix<T, SIZE> hue(S const h) const
    {
        if(f_rows    != 4
        || f_columns != 4)
        {
            throw std::runtime_error("scale() is only for 4x4 matrices at this time");
        }

        // $R_r$ -- rotation around red axis
        //
        matrix<T, SIZE> r_r(4, 4);
        value_type const inv_sqrt_2 = static_cast<value_type>(1) / sqrt(static_cast<value_type>(2));
        r_r[1][1] =  inv_sqrt_2;
        r_r[1][2] =  inv_sqrt_2;
        r_r[2][1] = -inv_sqrt_2;
        r_r[2][2] =  inv_sqrt_2;

        // $R_g$ -- rotation around green axis
        //
        matrix<T, SIZE> r_g(4, 4);
        value_type const sqrt_2_over_sqrt_3 = sqrt(static_cast<value_type>(2)) / sqrt(static_cast<value_type>(3));
        value_type const minus_inv_sqrt_3 = -static_cast<value_type>(1) / sqrt(static_cast<value_type>(3));
        r_r[0][0] =  sqrt_2_over_sqrt_3;
        r_r[0][2] =  minus_inv_sqrt_3;
        r_r[2][1] = -minus_inv_sqrt_3;
        r_r[2][2] =  sqrt_2_over_sqrt_3;

        // $R_{rg}$ -- the product or $R_r$ and $R_g$
        //
        matrix<T, SIZE> r_rg(r_r * r_g);

        // Luminance Vector
        //
        matrix<T, SIZE> l(4, 1);
        l[0][0] = RED_WEIGHT;
        l[1][0] = GREEN_WEIGHT;
        l[2][0] = BLUE_WEIGHT;
        l[3][0] = 0;

        matrix<T, SIZE> s(r_rg * l);

        // Rotate blue
        //
        matrix<T, SIZE> r_b(4, 4);
        value_type const rot_cos = cos(static_cast<value_type>(h));
        value_type const rot_sin = sin(static_cast<value_type>(h));
        r_b[0][0] =  rot_cos;
        r_b[0][1] =  rot_sin;
        r_b[1][0] = -rot_sin;
        r_b[1][1] =  rot_cos;

        // $H = R_{rg} S R_b S^{-1} R_{rg}^{-1}$
        //
        return r_rg * s * r_b / s / r_rg;
    }

private:
    friend element_ref;
    friend row_ref;
    friend const_row_ref;

    size_type       f_rows = 0;
    size_type       f_columns = 0;
    std::vector<T>  f_vector;
};


} // namespace snap



/** \brief Output a matrix to a basic_ostream.
 *
 * This function allows one to print out a matrix. The function attempts
 * to properly format the matrix inorder o mak readable.
 *
 * \param[in] out  The output stream where the matrix gets written.
 * \param[in] matrix  The actual matrix that is to be printed.
 *
 * \return A reference to the basic_ostream object.
 */
template<class E, class S, class T, class SIZE>
std::basic_ostream<E, S> & operator << (std::basic_ostream<E, S> & out, snap::matrix<T, SIZE> const & m)
{
    // write to a string buffer first
    //
    std::basic_ostringstream<E, S, std::allocator<E> > s;

    // setup the string output like the out stream
    //
    s.flags(out.flags());
    s.imbue(out.getloc());
    s.precision(out.precision());

    // output the matrix
    //
    s << "[";
    for(typename snap::matrix<T, SIZE>::size_type j(0); j < m.rows(); ++j)
    {
        s << std::endl << "  [";
        if(m.columns() > 0)
        {
            s << static_cast<T>(m[j][0]);
            for(typename snap::matrix<T, SIZE>::size_type i(1); i < m.columns(); ++i)
            {
                s << ", " << static_cast<T>(m[j][i]);
            }
        }
        s << "]";
    }
    s << std::endl << "]" << std::endl;

    // buffer is ready, display in output in one go
    //
    return out << s.str().c_str();
}


// file in ve folder (matrix.c)
// http://www.graficaobscura.com/matrix/index.html
// https://ncalculators.com/matrix/3x3-matrix-multiplication-calculator.htm
// /home/alexis/m2osw/sources/freeware/libx3d/utilities/src/fast_matrix.c++

// vim: ts=4 sw=4 et
