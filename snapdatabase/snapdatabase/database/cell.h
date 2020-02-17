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
 * \brief Cell header.
 *
 * A row is composed of cells. Whenever you access the database, you create
 * rows and add cells to it.
 *
 * Note that a cell must be defned in the schema of a table to be allowed
 * in a row.
 */

// self
//
#include    "snapdatabase/bigint/bigint.h"
#include    "snapdatabase/data/schema.h"



namespace snapdatabase
{


// all of the following columns are recognized by the system
// you are free to read any one of them, you can write to some of them
//
constexpr char const *                          g_oid_column                = "oid";
constexpr char const *                          g_expiration_date_column    = "expiration_date";


std::uint8_t  read_uint8(buffer_t const & buffer, size_t & pos);
std::uint16_t read_be_uint16(buffer_t const & buffer, size_t & pos);
std::uint32_t read_be_uint32(buffer_t const & buffer, size_t & pos);
std::uint64_t read_be_uint64(buffer_t const & buffer, size_t & pos);

void push_uint8(buffer_t & buffer, uint8_t value);
void push_be_uint16(buffer_t & buffer, uint16_t value);
void push_be_uint32(buffer_t & buffer, uint32_t value);
void push_be_uint64(buffer_t & buffer, uint64_t value);



class cell
{
public:
    typedef std::shared_ptr<cell>               pointer_t;
    typedef std::map<column_id_t, pointer_t>    map_t;

                                                cell(schema_column::pointer_t t);

    schema_column::pointer_t                    schema() const;
    struct_type_t                               type() const;
    bool                                        has_fixed_type() const;

    bool                                        is_void() const;
    void                                        set_void();

    std::uint64_t                               get_oid() const;
    void                                        set_oid(std::uint64_t value);

    std::int8_t                                 get_int8() const;
    void                                        set_int8(std::int8_t value);

    std::uint8_t                                get_uint8() const;
    void                                        set_uint8(std::uint8_t value);

    std::int16_t                                get_int16() const;
    void                                        set_int16(std::int16_t value);

    std::uint16_t                               get_uint16() const;
    void                                        set_uint16(std::uint16_t value);

    std::int32_t                                get_int32() const;
    void                                        set_int32(std::int32_t value);

    std::uint32_t                               get_uint32() const;
    void                                        set_uint32(std::uint32_t value);

    std::int64_t                                get_int64() const;
    void                                        set_int64(std::int64_t value);

    std::uint64_t                               get_uint64() const;
    void                                        set_uint64(std::uint64_t value);

    int512_t                                    get_int128() const;
    void                                        set_int128(int512_t value);

    uint512_t                                   get_uint128() const;
    void                                        set_uint128(uint512_t value);

    int512_t                                    get_int256() const;
    void                                        set_int256(int512_t value);

    uint512_t                                   get_uint256() const;
    void                                        set_uint256(uint512_t value);

    int512_t                                    get_int512() const;
    void                                        set_int512(int512_t value);

    uint512_t                                   get_uint512() const;
    void                                        set_uint512(uint512_t value);

    std::uint64_t                               get_time() const;
    void                                        set_time(std::uint64_t value);

    std::uint64_t                               get_time_ms() const;
    void                                        set_time_ms(std::uint64_t value);

    std::uint64_t                               get_time_us() const;
    void                                        set_time_us(std::uint64_t value);

    float                                       get_float32() const;
    void                                        set_float32(float value);

    double                                      get_float64() const;
    void                                        set_float64(double value);

    long double                                 get_float128() const;
    void                                        set_float128(long double value);

    version_t                                   get_version() const;
    void                                        set_version(version_t value);

    std::string                                 get_string() const;
    void                                        set_string(std::string const & value);

    void                                        column_id_to_binary(buffer_t & buffer) const;
    static column_id_t                          column_id_from_binary(buffer_t const & buffer, size_t & pos);

    void                                        value_to_binary(buffer_t & buffer) const;
    void                                        value_from_binary(buffer_t const & buffer, size_t & pos);

    void                                        copy_from(cell const & source);

private:
    void                                        set_integer(std::int64_t value);
    void                                        set_uinteger(std::uint64_t value);
    void                                        verify_cell_type(std::vector<struct_type_t> const & expected) const;

    schema_column::pointer_t                    f_schema_column = schema_column::pointer_t();
    uint512_t                                   f_integer = uint512_t();
    long double                                 f_float_value = 0.0L;
    std::string                                 f_string = std::string();
};



} // namespace snapdatabase
// vim: ts=4 sw=4 et
