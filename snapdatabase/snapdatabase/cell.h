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
#include    "snapdatabase/schema.h"
#include    "snapdatabase/bigint.h"



namespace snapdatabase
{



class cell
{
public:
    typedef std::shared_ptr<cell>               pointer_t;
    typedef std::map<std::string, pointer_t>    map_t;

                                                cell(schema_column::pointer_t t);

    schema_column::pointer_t                    schema() const;

    bool                                        is_void() const;
    void                                        set_void();

    int8_t                                      get_int8() const;
    void                                        set_int8(int8_t value);

    uint8_t                                     get_uint8() const;
    void                                        set_uint8(uint8_t value);

    int16_t                                     get_int16() const;
    void                                        set_int16(int16_t value);

    uint16_t                                    get_uint16() const;
    void                                        set_uint16(uint16_t value);

    int32_t                                     get_int32() const;
    void                                        set_int32(int32_t value);

    uint32_t                                    get_uint32() const;
    void                                        set_uint32(uint32_t value);

    int64_t                                     get_int64() const;
    void                                        set_int64(int64_t value);

    uint64_t                                    get_uint64() const;
    void                                        set_uint64(uint64_t value);

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

private:
    schema_column::pointer_t                    f_schema_column = schema_column::pointer_t();
    uint512_t                                   f_integer = uint512_t();
    long double                                 f_float_value = 0.0L;
    std::string                                 f_string = std::string();
};



} // namespace snapdatabase
// vim: ts=4 sw=4 et
