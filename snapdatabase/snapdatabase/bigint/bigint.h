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
#pragma once


/** \file
 * \brief Handle a block structure.
 *
 * Each block contains a structure. The very first four bytes are always the
 * magic characters which define the type of the block. The remained of the
 * block is a _lose_ structure which very often changes in size because it
 * includes parameters such as a string or an array.
 *
 * Also in most cases arrays are also themselvess _lose_ structures (a few
 * are just numbers such as column ids or block references.)
 *
 * IMPORTANT: The types defined here are also the types that we accept in
 * a user table. Here we define structures and later tables.
 */

// C++ lib
//
#include    <cstdint>
#include    <initializer_list>



namespace snapdatabase
{



constexpr std::uint64_t round_down(std::uint64_t value, std::uint64_t multiple)
{
    return value - value % multiple;
}


constexpr std::uint64_t round_up(std::uint64_t value, std::uint64_t multiple)
{
    std::uint64_t adjusted(value + multiple - 1);
    return round_down(adjusted, multiple);
}


constexpr std::uint64_t divide_rounded_up(std::uint64_t value, std::uint64_t multiple)
{
    return (value + multiple - 1) / multiple;
}


void                                add128(std::uint64_t * dst, std::uint64_t const * src);
void                                add256(std::uint64_t * dst, std::uint64_t const * src);
void                                add512(std::uint64_t * dst, std::uint64_t const * src);

void                                sub128(std::uint64_t * dst, std::uint64_t const * src);
void                                sub256(std::uint64_t * dst, std::uint64_t const * src);
void                                sub512(std::uint64_t * dst, std::uint64_t const * src);


struct uint512_t;

struct int512_t
{
                                    int512_t();
                                    int512_t(int512_t const & rhs);
                                    int512_t(uint512_t const & rhs);
                                    int512_t(std::initializer_list<std::uint64_t> rhs);

    int512_t &                      operator = (int512_t const & rhs) = default;

    bool                            is_positive() const { return f_high_value >= 0; }
    bool                            is_negative() const { return f_high_value < 0; }

    std::size_t                     bit_size() const;

    int512_t                        operator - () const;
    int512_t &                      operator += (int512_t const & rhs);
    int512_t &                      operator -= (int512_t const & rhs);

    std::uint64_t                   f_value[7] = { 0 };
    std::int64_t                    f_high_value = 0;
};


struct uint512_t
{
                                    uint512_t();
                                    uint512_t(uint512_t const & rhs);
                                    uint512_t(int512_t const & rhs);
                                    uint512_t(std::initializer_list<std::uint64_t> rhs);

    uint512_t &                     operator = (uint512_t const & rhs);
    uint512_t &                     operator = (int512_t const & rhs);

    bool                            is_positive() const { return true; }
    bool                            is_negative() const { return false; }

    std::size_t                     bit_size() const;
    void                            lsl(int count);
    void                            lsr(int count);
    bool                            is_zero() const;
    int                             compare(uint512_t const & rhs) const;
    uint512_t &                     div(uint512_t const & rhs, uint512_t & remainder);

    uint512_t                       operator - () const;
    uint512_t &                     operator += (uint512_t const & rhs);
    uint512_t &                     operator -= (uint512_t const & rhs);
    uint512_t &                     operator *= (uint512_t const & rhs);

    bool                            operator == (uint64_t rhs) const;
    bool                            operator != (uint64_t rhs) const;

    std::uint64_t                   f_value[8] = { 0 };
};




} // namespace snapdatabase
// vim: ts=4 sw=4 et
