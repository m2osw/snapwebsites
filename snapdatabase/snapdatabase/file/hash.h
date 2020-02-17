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
 * \brief Incremental hash function.
 *
 * We have our own simple hash function we use to compute a hash of
 * our keys in order to check a Bloom Filter. According to many tests
 * available out there, we can save a good 50% of processing by using
 * a bloom filter, especially on a big data table. Also in our case,
 * we can _promote_ the Bloom Filter data to the client and not even
 * bother the server if we get a negative on the Bloom Filter check.
 */

// C++ lib
//
#include    <cstdint>



namespace snapdatabase
{



typedef std::uint32_t       hash_t;


class hash
{
public:
                                    hash(hash_t seed);
                                    hash(hash const & rhs) = delete;

    hash &                          operator = (hash const & rhs) = delete;

    void                            add(std::uint8_t const * v, std::size_t size);
    hash_t                          get() const;

private:
    hash_t                          get_byte() const;
    hash_t                          peek_byte(int pos) const;
    std::size_t                     size() const;
    void                            get_64bits(hash_t & v1, hash_t & v2);
    void                            peek_64bits(hash_t & v1, hash_t & v2) const;

    hash_t                          f_hash = 0;
    mutable std::uint8_t const *    f_buffer = nullptr;
    mutable std::size_t             f_size = 0;
    std::uint8_t                    f_temp[8] = { 0 };
    mutable std::size_t             f_temp_size = 0;
};



} // namespace snapdatabase
// vim: ts=4 sw=4 et
