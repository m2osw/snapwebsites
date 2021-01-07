// Copyright (c) 2019  Made to Order Software Corp.  All Rights Reserved
//
// https://snapwebsites.org/project/snapdatabase
// contact@m2osw.com
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
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.


/** \file
 * \brief Database file implementation.
 *
 * Each table uses one or more files. Each file is handled by a dbfile
 * object and a corresponding set of blocks.
 */

// self
//
#include    "snapdatabase/bigint/bigint.h"

#include    "snapdatabase/exception.h"
#include    "snapdatabase/arch_support.h"


// C++ lib
//
#include    <cstring>
#include    <iostream>
#include    <string>


// last include
//
#include    <snapdev/poison.h>



namespace snapdatabase
{





int512_t::int512_t()
{
}


int512_t::int512_t(int512_t const & rhs)
{
    for(int idx(0); idx < 7; ++idx)
    {
        f_value[idx] = rhs.f_value[idx];
    }
    f_high_value = rhs.f_high_value;
}


int512_t::int512_t(uint512_t const & rhs)
{
    for(int idx(0); idx < 7; ++idx)
    {
        f_value[idx] = rhs.f_value[idx];
    }
    f_high_value = rhs.f_value[7];
}


int512_t::int512_t(std::initializer_list<std::uint64_t> rhs)
{
    if(rhs.size() > std::size(f_value))
    {
        throw snapdatabase_out_of_range(
                  "rhs array too large for int512_t constructor ("
                + std::to_string(rhs.size())
                + " > "
                + std::to_string(std::size(f_value))
                + ").");
    }

    std::copy(rhs.begin(), rhs.end(), f_value);
    if(rhs.size() < std::size(f_value))
    {
        std::memset(reinterpret_cast<uint8_t *>(f_value) + rhs.size(), 0, sizeof(f_value) - rhs.size());
    }
}


std::size_t int512_t::bit_size() const
{
    int512_t p;
    if(is_negative())
    {
        p -= *this;
        if(p.is_negative())
        {
            return 512;
        }
    }
    else
    {
        p = *this;
    }

    if(p.f_high_value != 0)
    {
        return (__builtin_clzll(p.f_high_value) ^ 0x3F) + 7 * 64 + 1;
    }

    std::size_t result(512 - 63);
    for(int idx(6); idx >= 0; --idx)
    {
        result -= 64;
        if(p.f_value[idx] != 0)
        {
            return (__builtin_clzll(p.f_value[idx]) ^ 0x3F) + result;
        }
    }

    return 0;
}


int512_t int512_t::operator - () const
{
    int512_t neg;
    neg -= *this;
    return neg;
}


int512_t & int512_t::operator += (int512_t const & rhs)
{
    add512(f_value, rhs.f_value);       // the add includes the high value
    return *this;
}


int512_t & int512_t::operator -= (int512_t const & rhs)
{
    sub512(f_value, rhs.f_value);       // the sub includes the high value
    return *this;
}




uint512_t::uint512_t()
{
}


uint512_t::uint512_t(uint512_t const & rhs)
{
    f_value[0] = rhs.f_value[0];
    f_value[1] = rhs.f_value[1];
    f_value[2] = rhs.f_value[2];
    f_value[3] = rhs.f_value[3];
    f_value[4] = rhs.f_value[4];
    f_value[5] = rhs.f_value[5];
    f_value[6] = rhs.f_value[6];
    f_value[7] = rhs.f_value[7];
}


uint512_t::uint512_t(int512_t const & rhs)
{
    f_value[0] = rhs.f_value[0];
    f_value[1] = rhs.f_value[1];
    f_value[2] = rhs.f_value[2];
    f_value[3] = rhs.f_value[3];
    f_value[4] = rhs.f_value[4];
    f_value[5] = rhs.f_value[5];
    f_value[6] = rhs.f_value[6];
    f_value[7] = rhs.f_high_value;
}


uint512_t::uint512_t(std::initializer_list<std::uint64_t> rhs)
{
    if(rhs.size() > std::size(f_value))
    {
        throw snapdatabase_out_of_range(
                  "rhs array too large for int512_t constructor ("
                + std::to_string(rhs.size())
                + " > "
                + std::to_string(std::size(f_value))
                + ").");
    }

    std::copy(rhs.begin(), rhs.end(), f_value);
    if(rhs.size() < std::size(f_value))
    {
        std::memset(reinterpret_cast<uint8_t *>(f_value) + rhs.size(), 0, sizeof(f_value) - rhs.size());
    }
}


uint512_t & uint512_t::operator = (uint512_t const & rhs)
{
    f_value[0] = rhs.f_value[0];
    f_value[1] = rhs.f_value[1];
    f_value[2] = rhs.f_value[2];
    f_value[3] = rhs.f_value[3];
    f_value[4] = rhs.f_value[4];
    f_value[5] = rhs.f_value[5];
    f_value[6] = rhs.f_value[6];
    f_value[7] = rhs.f_value[7];

    return *this;
}


uint512_t & uint512_t::operator = (int512_t const & rhs)
{
    f_value[0] = rhs.f_value[0];
    f_value[1] = rhs.f_value[1];
    f_value[2] = rhs.f_value[2];
    f_value[3] = rhs.f_value[3];
    f_value[4] = rhs.f_value[4];
    f_value[5] = rhs.f_value[5];
    f_value[6] = rhs.f_value[6];
    f_value[7] = rhs.f_high_value;

    return *this;
}


size_t uint512_t::bit_size() const
{
    std::size_t result(512);
    for(int idx(7); idx >= 0; --idx)
    {
        result -= 64;
        if(f_value[idx] != 0)
        {
            return (__builtin_clzll(f_value[idx]) ^ 0x3F) + result;
        }
    }

    return 0;
}


void uint512_t::lsl(int count)
{
    if(count < 0)
    {
        throw snapdatabase_out_of_range(
                  "lsl() cannot be called with a negative value ("
                + std::to_string(count)
                + ").");
    }

    if(count > 512)
    {
        count = 512;
    }

    int pos(8);
    int move(count / 64);
    int const shift(count % 64);
    if(move > 0)
    {
        pos = 8 - move;
        if(move < 8)
        {
            memmove(f_value + move, f_value, pos * 8);
        }
        memset(f_value, 0, move * 8);
    }
    if(shift != 0)
    {
        int remainder(64 - shift);

        std::uint64_t extra(0);
        while(move < 8)
        {
            std::uint64_t const next(f_value[move] >> remainder);
            f_value[move] = (f_value[move] << shift) | extra;
            extra = next;
            ++move;
        }
    }
}


void uint512_t::lsr(int count)
{
    if(count < 0)
    {
        throw snapdatabase_out_of_range(
                  "lsr() cannot be called with a negative value ("
                + std::to_string(count)
                + ").");
    }

    if(count > 512)
    {
        count = 512;
    }

    int pos(8);
    int const move(count / 64);
    int const shift(count % 64);
    if(move > 0)
    {
        pos = 8 - move;
        if(move < 8)
        {
            memmove(f_value, f_value + move, pos * 8);
        }
        memset(f_value + pos * 8, 0, move * 8);
    }
    if(shift != 0)
    {
        int remainder(64 - shift);

        std::uint64_t extra(0);
        while(pos > 0)
        {
            --pos;
            std::uint64_t const next(f_value[pos] << remainder);
            f_value[pos] = (f_value[pos] >> shift) | extra;
            extra = next;
        }
    }
}


bool uint512_t::is_zero() const
{
    return f_value[0] == 0
        && f_value[1] == 0
        && f_value[2] == 0
        && f_value[3] == 0
        && f_value[4] == 0
        && f_value[5] == 0
        && f_value[6] == 0
        && f_value[7] == 0;
}


int uint512_t::compare(uint512_t const & rhs) const
{
    int idx(8);
    while(idx > 0)
    {
        --idx;
        if(f_value[idx] != rhs.f_value[idx])
        {
            return f_value[idx] > rhs.f_value[idx] ? 1 : -1;
        }
    }

    return 0;
}


// we have this one because we need it to convert back to a string
//
uint512_t & uint512_t::div(uint512_t const & rhs, uint512_t & remainder)
{
    bool zero(true);
    for(int idx(0); idx < 8; ++idx)
    {
        if(f_value[idx] != 0)
        {
            zero = false;
            break;
        }
    }
    if(zero)
    {
        throw snapdatabase_logic_error("Division by zero not allowed in uint512_t.");
    }

    int c(compare(rhs));
    if(c == -1)
    {
        // a / (a + n) = 0 (remainder = a)   where n > 0
        //
        remainder = *this;
        memset(f_value, 0, sizeof(f_value));
        return *this;
    }

    if(c == 0)
    {
        // a / a = 1 (remainder = 0)
        //
        memset(remainder.f_value, 0, sizeof(remainder.f_value));
        f_value[0] = 1;
        memset(f_value + 1, 0, sizeof(f_value) - sizeof(f_value[0]));
        return *this;
    }

    // in this case we have to do the division
    //
    // TBD: we could also use these size to handle the two previous
    //      special cases; we'd have to determine which of the compare()
    //      and the bit_size() is faster...
    //
    std::size_t lhs_size(bit_size());
    std::size_t rhs_size(rhs.bit_size());

    remainder = *this;
    memset(f_value, 0, sizeof(f_value));

    uint512_t divisor(rhs);
    divisor.lsl(lhs_size - rhs_size);

    uint512_t one;
    one.f_value[0] = 1;

    // this is it! this loop calculates the division the very slow way
    // (TODO we need to use (a / b) and (a % b) and do all the math and
    // it will be much faster... the bit by bit operations are slow even
    // if they work magic)
    do
    {
        lsl(1);
        c = remainder.compare(divisor);
        if(c >= 0)
        {
            remainder -= divisor;
            operator += (one);
        }
        divisor.lsr(1);
    }
    while(!divisor.is_zero());

    return *this;
}


uint512_t uint512_t::operator - () const
{
    uint512_t neg;
    neg -= *this;
    return neg;
}


uint512_t & uint512_t::operator += (uint512_t const & rhs)
{
    add512(f_value, rhs.f_value);
    return *this;
}


uint512_t & uint512_t::operator -= (uint512_t const & rhs)
{
    sub512(f_value, rhs.f_value);
    return *this;
}


uint512_t & uint512_t::operator *= (uint512_t const & rhs)
{
    // this is a very slow way to do a multiplication, but very easy,
    // we do not use it much so we're fine for now...
    //
    uint512_t lhs(*this);

    uint512_t factor(rhs);
    if((factor.f_value[0] & 1) == 0)
    {
        lhs.f_value[0] = 0;
        lhs.f_value[1] = 0;
        lhs.f_value[2] = 0;
        lhs.f_value[3] = 0;
        lhs.f_value[4] = 0;
        lhs.f_value[5] = 0;
        lhs.f_value[6] = 0;
        lhs.f_value[7] = 0;
    }

    for(;;)
    {
        factor.lsr(1);
        if(factor == 0)
        {
            break;
        }

        lhs.lsl(1);
        if((factor.f_value[0] & 1) != 0)
        {
            *this += lhs;
        }
    }

    return *this;
}


bool uint512_t::operator == (uint64_t rhs) const
{
    return f_value[0] == rhs
        && f_value[1] == 0
        && f_value[2] == 0
        && f_value[3] == 0
        && f_value[4] == 0
        && f_value[5] == 0
        && f_value[6] == 0
        && f_value[7] == 0;
}


bool uint512_t::operator != (uint64_t rhs) const
{
    return f_value[0] != rhs
        || f_value[1] != 0
        || f_value[2] != 0
        || f_value[3] != 0
        || f_value[4] != 0
        || f_value[5] != 0
        || f_value[6] != 0
        || f_value[7] != 0;
}



} // namespace snapdatabase
// vim: ts=4 sw=4 et
