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
 * \brief Cell file implementation.
 *
 * When handling a row, it has a set of cells. The set may change between
 * calls. To the minimum, though, a row should at least have one cell.
 */

// self
//
#include    "snapdatabase/database/cell.h"


// snapdev lib
//
#include    "snapdev/not_reached.h"


// last include
//
#include    <snapdev/poison.h>



namespace snapdatabase
{




cell::cell(schema_column::pointer_t c)
    : f_schema_column(c)
{
}


schema_column::pointer_t cell::schema() const
{
    return f_schema_column;
}


bool cell::is_void() const
{
    return f_schema_column->type() == struct_type_t::STRUCT_TYPE_VOID;
}


void cell::set_void()
{
    verify_cell_type({
              struct_type_t::STRUCT_TYPE_VOID
        });
}


std::uint64_t cell::get_oid() const
{
    verify_cell_type({
              struct_type_t::STRUCT_TYPE_OID
        });

    return f_integer.f_value[0];
}


void cell::set_oid(std::uint64_t oid)
{
    verify_cell_type({
              struct_type_t::STRUCT_TYPE_OID
        });

    set_uinteger(oid);
}


std::int8_t cell::get_int8() const
{
    verify_cell_type({
              struct_type_t::STRUCT_TYPE_INT8
        });

    return f_integer.f_value[0];
}


void cell::set_int8(std::int8_t value)
{
    verify_cell_type({
              struct_type_t::STRUCT_TYPE_INT8
        });

    set_integer(value);
}


std::uint8_t cell::get_uint8() const
{
    verify_cell_type({
              struct_type_t::STRUCT_TYPE_BITS8
            , struct_type_t::STRUCT_TYPE_UINT8
        });

    return f_integer.f_value[0];
}


void cell::set_uint8(std::uint8_t value)
{
    verify_cell_type({
              struct_type_t::STRUCT_TYPE_BITS8
            , struct_type_t::STRUCT_TYPE_UINT8
        });

    set_uinteger(value);
}


std::int16_t cell::get_int16() const
{
    verify_cell_type({
              struct_type_t::STRUCT_TYPE_INT16
        });

    return f_integer.f_value[0];
}


void cell::set_int16(std::int16_t value)
{
    verify_cell_type({
              struct_type_t::STRUCT_TYPE_INT16
        });

    set_integer(value);
}


std::uint16_t cell::get_uint16() const
{
    verify_cell_type({
              struct_type_t::STRUCT_TYPE_BITS16
            , struct_type_t::STRUCT_TYPE_UINT16
        });

    return f_integer.f_value[0];
}


void cell::set_uint16(std::uint16_t value)
{
    verify_cell_type({
              struct_type_t::STRUCT_TYPE_BITS16
            , struct_type_t::STRUCT_TYPE_UINT16
        });

    set_uinteger(value);
}


std::int32_t cell::get_int32() const
{
    verify_cell_type({
              struct_type_t::STRUCT_TYPE_INT32
        });

    return f_integer.f_value[0];
}


void cell::set_int32(std::int32_t value)
{
    verify_cell_type({
              struct_type_t::STRUCT_TYPE_INT32
        });

    set_integer(value);
}


std::uint32_t cell::get_uint32() const
{
    verify_cell_type({
              struct_type_t::STRUCT_TYPE_BITS32
            , struct_type_t::STRUCT_TYPE_UINT32
        });

    return f_integer.f_value[0];
}


void cell::set_uint32(std::uint32_t value)
{
    verify_cell_type({
              struct_type_t::STRUCT_TYPE_BITS32
            , struct_type_t::STRUCT_TYPE_UINT32
        });

    set_uinteger(value);
}


std::int64_t cell::get_int64() const
{
    verify_cell_type({
              struct_type_t::STRUCT_TYPE_INT64
        });

    return f_integer.f_value[0];
}


void cell::set_int64(std::int64_t value)
{
    verify_cell_type({
              struct_type_t::STRUCT_TYPE_INT64
        });

    set_integer(value);
}


std::uint64_t cell::get_uint64() const
{
    verify_cell_type({
              struct_type_t::STRUCT_TYPE_BITS64
            , struct_type_t::STRUCT_TYPE_UINT64
        });

    return f_integer.f_value[0];
}


void cell::set_uint64(std::uint64_t value)
{
    verify_cell_type({
              struct_type_t::STRUCT_TYPE_BITS64
            , struct_type_t::STRUCT_TYPE_UINT64
        });

    set_uinteger(value);
}


int512_t cell::get_int128() const
{
    verify_cell_type({
              struct_type_t::STRUCT_TYPE_INT128
        });

    return f_integer;
}


void cell::set_int128(int512_t value)
{
    verify_cell_type({
              struct_type_t::STRUCT_TYPE_INT128
        });

    f_integer = value;
}


uint512_t cell::get_uint128() const
{
    verify_cell_type({
              struct_type_t::STRUCT_TYPE_UINT128
        });

    return f_integer;
}


void cell::set_uint128(uint512_t value)
{
    verify_cell_type({
              struct_type_t::STRUCT_TYPE_UINT128
        });

    f_integer = value;
}


int512_t cell::get_int256() const
{
    verify_cell_type({
              struct_type_t::STRUCT_TYPE_INT256
        });

    return f_integer;
}


void cell::set_int256(int512_t value)
{
    verify_cell_type({
              struct_type_t::STRUCT_TYPE_INT256
        });

    f_integer = value;
}


uint512_t cell::get_uint256() const
{
    verify_cell_type({
              struct_type_t::STRUCT_TYPE_UINT256
        });

    return f_integer;
}


void cell::set_uint256(uint512_t value)
{
    verify_cell_type({
              struct_type_t::STRUCT_TYPE_UINT256
        });

    f_integer = value;
}


int512_t cell::get_int512() const
{
    verify_cell_type({
              struct_type_t::STRUCT_TYPE_INT512
        });

    return f_integer;
}


void cell::set_int512(int512_t value)
{
    verify_cell_type({
              struct_type_t::STRUCT_TYPE_INT512
        });

    f_integer = value;
}


uint512_t cell::get_uint512() const
{
    verify_cell_type({
              struct_type_t::STRUCT_TYPE_UINT512
        });

    return f_integer;
}


void cell::set_uint512(uint512_t value)
{
    verify_cell_type({
              struct_type_t::STRUCT_TYPE_UINT512
        });

    f_integer = value;
}


std::uint64_t cell::get_time() const
{
    verify_cell_type({
              struct_type_t::STRUCT_TYPE_TIME
        });

    return f_integer.f_value[0];
}


void cell::set_time(std::uint64_t t)
{
    verify_cell_type({
              struct_type_t::STRUCT_TYPE_TIME
        });

    set_uinteger(t);
}


std::uint64_t cell::get_time_ms() const
{
    verify_cell_type({
              struct_type_t::STRUCT_TYPE_MSTIME
        });

    return f_integer.f_value[0];
}


void cell::set_time_ms(std::uint64_t t)
{
    verify_cell_type({
              struct_type_t::STRUCT_TYPE_MSTIME
        });

    set_uinteger(t);
}


std::uint64_t cell::get_time_us() const
{
    verify_cell_type({
              struct_type_t::STRUCT_TYPE_USTIME
        });

    return f_integer.f_value[0];
}


void cell::set_time_us(std::uint64_t t)
{
    verify_cell_type({
              struct_type_t::STRUCT_TYPE_USTIME
        });

    set_uinteger(t);
}


float cell::get_float32() const
{
    verify_cell_type({
              struct_type_t::STRUCT_TYPE_FLOAT32
        });

    return f_float_value;
}


void cell::set_float32(float value)
{
    verify_cell_type({
              struct_type_t::STRUCT_TYPE_FLOAT32
        });

    f_float_value = value;
}


double cell::get_float64() const
{
    verify_cell_type({
              struct_type_t::STRUCT_TYPE_FLOAT64
        });

    return f_float_value;
}


void cell::set_float64(double value)
{
    verify_cell_type({
              struct_type_t::STRUCT_TYPE_FLOAT64
        });

    f_float_value = value;
}


long double cell::get_float128() const
{
    verify_cell_type({
              struct_type_t::STRUCT_TYPE_FLOAT128
        });

    return f_float_value;
}


void cell::set_float128(long double value)
{
    verify_cell_type({
              struct_type_t::STRUCT_TYPE_FLOAT128
        });

    f_float_value = value;
}


version_t cell::get_version() const
{
    verify_cell_type({
              struct_type_t::STRUCT_TYPE_VERSION
        });

    return version_t(f_integer.f_value[0]);
}


void cell::set_version(version_t value)
{
    verify_cell_type({
              struct_type_t::STRUCT_TYPE_VERSION
        });

    set_uinteger(value.to_binary());
}


std::string cell::get_string() const
{
    verify_cell_type({
              struct_type_t::STRUCT_TYPE_P8STRING
            , struct_type_t::STRUCT_TYPE_P16STRING
            , struct_type_t::STRUCT_TYPE_P32STRING
        });

    return f_string;
}


void cell::set_string(std::string const & value)
{
    verify_cell_type({
              struct_type_t::STRUCT_TYPE_P8STRING
            , struct_type_t::STRUCT_TYPE_P16STRING
            , struct_type_t::STRUCT_TYPE_P32STRING
        });

    f_string = value;
}


void cell::set_integer(int64_t value)
{
    f_integer.f_value[0] = value;
    f_integer.f_value[1] = value < 0 ? -1 : 0;
    f_integer.f_value[2] = f_integer.f_value[1];
    f_integer.f_value[3] = f_integer.f_value[1];
    f_integer.f_value[4] = f_integer.f_value[1];
    f_integer.f_value[5] = f_integer.f_value[1];
    f_integer.f_value[6] = f_integer.f_value[1];
    f_integer.f_value[7] = f_integer.f_value[1];
}


void cell::set_uinteger(uint64_t value)
{
    f_integer.f_value[0] = value;
    f_integer.f_value[1] = 0;
    f_integer.f_value[2] = 0;
    f_integer.f_value[3] = 0;
    f_integer.f_value[4] = 0;
    f_integer.f_value[5] = 0;
    f_integer.f_value[6] = 0;
    f_integer.f_value[7] = 0;
}


void cell::verify_cell_type(std::vector<struct_type_t> const & expected) const
{
    if(std::find(expected.begin(), expected.end(), f_schema_column->type()) == expected.end())
    {
        std::string names;
        for(auto const & e : expected)
        {
            if(!names.empty())
            {
                names += " or ";
            }
            names += to_string(e);
        }
        throw type_mismatch(
              "The call you made to this cell expected "
            + names
            + " type"
            + (expected.size() == 1 ? "" : "s")
            + ", but the schema says this cell is of type "
            + to_string(f_schema_column->type())
            + ".");
    }
}



} // namespace snapdatabase
// vim: ts=4 sw=4 et
