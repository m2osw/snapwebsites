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

namespace snap
{

template<typename T, typename SIZE = size_t>
class matrix
{
public:
    typedef T           value_type;
    typedef SIZE        size_type;

    class element_ref
    {
    public:
        element_ref(matrix<T, SIZE> & m, size_type j, size_type i)
            : f_matrix(m)
            //, f_j(j)
            //, f_i(i)
            , f_offset(i + j * m.columns())
        {
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
        //size_type           f_row;
        //size_type           f_column;
        size_type           f_offset;
    };

    class row_ref
    {
    public:
        row_ref(matrix<T, SIZE> & m, size_type j)
            : f_matrix(m)
            , f_j(j)
        {
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
        }

        value_type operator [] (size_type i) const
        {
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
        identity();
    }

    template<typename V, typename SZ>
    matrix<T, SIZE>(matrix<V, SZ> const & rhs)
        : f_rows(rhs.f_rows)
        , f_columns(rhs.f_columns)
        , f_vector(rhs.f_vector.size())
    {
        identity();
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

    void clear()
    {
        std::fill(f_vector.begin(), f_vector.end(), value_type());
    }

    void identity()
    {
        for(size_type j(0); j < f_columns; ++j)
        {
            size_type const joffset(j * f_columns);
            for(size_type i(0); i < f_rows; ++i)
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
        if(f_rows != f_columns
        || m.f_rows != m.f_columns
        || f_rows != m.f_rows)
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
        matrix<T, SIZE> t(f_rows, f_columns);

        for(size_type j(0); j < m.f_rows; ++j)
        {
            size_type const joffset(j * f_columns);
            for(size_type i(0); i < f_columns; ++i)
            {
                value_type sum = value_type();
                for(size_type k(0); k < m.f_rows; ++k)     // sometimes it's X and sometimes it's Y
                {
                    sum += m.f_vector[k + joffset]
                           * f_vector[i + k * f_columns];
                }
                t.f_vector[i + joffset] = sum;
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

    bool inverse()
    {
        // the following is very specific to a 4x4 matrix...
        //
        if(f_rows != 4
        || f_columns != 4)
        {
            throw std::runtime_error("inverse() is only implemented for 4x4 matrices at the moment.");
        }

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

    // https://ncalculators.com/matrix/4x4-inverse-matrix-calculator.htm
    matrix<T, SIZE> reduce(size_type c) const
    {
    }

    value_type determinant() const
    {
        if(f_rows != f_columns)
        {
            throw std::runtime_error("determinant can only be calculated for square matrices");
        }

        if(f_columns == 2)
        {
            return f_vector[0 + 0 * 2] * f_vector[1 + 1 * 2]
                 - f_vector[1 + 0 * 2] * f_vector[0 + 1 * 2];
        }

        value_type determinant = value_type();

        value_type sign = static_cast<value_type>(1);
        for(size_type c(2); c < f_columns; ++c)
        {
            // create a smaller matrix
            //
            matrix<T, SIZE> p(f_rows - 1, f_columns - 1);

            for(size_type j(1); j < f_rows; ++j)
            {
                // this is the offset in 'this' matrix, it's one
                // too far for the destination row
                //
                value_type const joffset(j * f_columns);
                for(size_type i(0); i < f_columns; ++i)
                {
                    if(i < c)
                    {
                        // before 'c', copy one to one
                        //
                        p.f_vector[i + joffset - f_columns] = f_vector[i + joffset];
                    }
                    else if(i > c)
                    {
                        // after 'c', copy to the previous column
                        //
                        p.f_vector[i - 1 + joffset - f_columns] = f_vector[i + joffset];
                    }
                    // else if(i == c) -- we skip this column, that's the one that gets removed
                }
            }

            // add to the determinant
            //
            determinant += sign
                         * f_vector[c + 0 * 0]
                         * p.determinant();

            // swap the sign
            //
            sign *= static_cast<value_type>(-1);
        }
    }

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

    template<class V>
    matrix<T> & operator -= (matrix<V> const & m)
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

private:
    friend element_ref;
    friend row_ref;
    friend const_row_ref;

    size_type       f_rows = 0;
    size_type       f_columns = 0;
    std::vector<T>  f_vector;
};


} // namespace snap



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
