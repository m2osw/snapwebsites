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
    if(f_schema_column->type() != struct_type_t::STRUCT_TYPE_VOID)
    {
        throw type_mismatch(
              "expected void type, received "
            + std::to_string(static_cast<int>(f_schema_column->type())));
    }
}


int8_t cell::get_int8() const
{
    if(f_schema_column->type() != struct_type_t::STRUCT_TYPE_INT8)
    {
        throw type_mismatch(
              "expected int8 type, received "
            + std::to_string(static_cast<int>(f_schema_column->type())));
    }

    return f_integer.f_value[0];
}


void cell::set_int8(int8_t value)
{
    if(f_schema_column->type() != struct_type_t::STRUCT_TYPE_INT8)
    {
        throw type_mismatch(
              "expected int8 type, received "
            + std::to_string(static_cast<int>(f_schema_column->type())));
    }

    f_integer.f_value[0] = value;
    f_integer.f_value[1] = value < 0 ? -1 : 0;
    f_integer.f_value[2] = f_integer.f_value[1];
    f_integer.f_value[3] = f_integer.f_value[1];
    f_integer.f_value[4] = f_integer.f_value[1];
    f_integer.f_value[5] = f_integer.f_value[1];
    f_integer.f_value[6] = f_integer.f_value[1];
    f_integer.f_value[7] = f_integer.f_value[1];
}


uint8_t cell::get_uint8() const
{
    if(f_schema_column->type() != struct_type_t::STRUCT_TYPE_BITS8
    && f_schema_column->type() != struct_type_t::STRUCT_TYPE_UINT8)
    {
        throw type_mismatch(
              "expected bits8 or uint8 type, received "
            + std::to_string(static_cast<int>(f_schema_column->type())));
    }

    return f_integer.f_value[0];
}


void cell::set_uint8(uint8_t value)
{
    if(f_schema_column->type() != struct_type_t::STRUCT_TYPE_BITS8
    && f_schema_column->type() != struct_type_t::STRUCT_TYPE_UINT8)
    {
        throw type_mismatch(
              "expected bits8 or uint8 type, received "
            + std::to_string(static_cast<int>(f_schema_column->type())));
    }

    f_integer.f_value[0] = value;
    f_integer.f_value[1] = 0;
    f_integer.f_value[2] = 0;
    f_integer.f_value[3] = 0;
    f_integer.f_value[4] = 0;
    f_integer.f_value[5] = 0;
    f_integer.f_value[6] = 0;
    f_integer.f_value[7] = 0;
}


int16_t cell::get_int16() const
{
    if(f_schema_column->type() != struct_type_t::STRUCT_TYPE_INT16)
    {
        throw type_mismatch(
              "expected int16 type, received "
            + std::to_string(static_cast<int>(f_schema_column->type())));
    }

    return f_integer.f_value[0];
}


void cell::set_int16(int16_t value)
{
    if(f_schema_column->type() != struct_type_t::STRUCT_TYPE_INT16)
    {
        throw type_mismatch(
              "expected int16 type, received "
            + std::to_string(static_cast<int>(f_schema_column->type())));
    }

    f_integer.f_value[0] = value;
    f_integer.f_value[1] = static_cast<int64_t>(value) < 0 ? -1 : 0;
    f_integer.f_value[2] = f_integer.f_value[1];
    f_integer.f_value[3] = f_integer.f_value[1];
    f_integer.f_value[4] = f_integer.f_value[1];
    f_integer.f_value[5] = f_integer.f_value[1];
    f_integer.f_value[6] = f_integer.f_value[1];
    f_integer.f_value[7] = f_integer.f_value[1];
}


uint16_t cell::get_uint16() const
{
    if(f_schema_column->type() != struct_type_t::STRUCT_TYPE_BITS16
    && f_schema_column->type() != struct_type_t::STRUCT_TYPE_UINT16)
    {
        throw type_mismatch(
              "expected uint16 type, received "
            + std::to_string(static_cast<int>(f_schema_column->type())));
    }

    return f_integer.f_value[0];
}


void cell::set_uint16(uint16_t value)
{
    if(f_schema_column->type() != struct_type_t::STRUCT_TYPE_BITS16
    && f_schema_column->type() != struct_type_t::STRUCT_TYPE_UINT16)
    {
        throw type_mismatch(
              "expected uint16 type, received "
            + std::to_string(static_cast<int>(f_schema_column->type())));
    }

    f_integer.f_value[0] = value;
    f_integer.f_value[1] = 0;
    f_integer.f_value[2] = 0;
    f_integer.f_value[3] = 0;
    f_integer.f_value[4] = 0;
    f_integer.f_value[5] = 0;
    f_integer.f_value[6] = 0;
    f_integer.f_value[7] = 0;
}


int32_t cell::get_int32() const
{
    if(f_schema_column->type() != struct_type_t::STRUCT_TYPE_INT32)
    {
        throw type_mismatch(
              "expected int32 type, received "
            + std::to_string(static_cast<int>(f_schema_column->type())));
    }

    return f_integer.f_value[0];
}


void cell::set_int32(int32_t value)
{
    if(f_schema_column->type() != struct_type_t::STRUCT_TYPE_INT32)
    {
        throw type_mismatch(
              "expected int32 type, received "
            + std::to_string(static_cast<int>(f_schema_column->type())));
    }

    f_integer.f_value[0] = value;
    f_integer.f_value[1] = value < 0 ? -1 : 0;
    f_integer.f_value[2] = f_integer.f_value[1];
    f_integer.f_value[3] = f_integer.f_value[1];
    f_integer.f_value[4] = f_integer.f_value[1];
    f_integer.f_value[5] = f_integer.f_value[1];
    f_integer.f_value[6] = f_integer.f_value[1];
    f_integer.f_value[7] = f_integer.f_value[1];
}


uint32_t cell::get_uint32() const
{
    if(f_schema_column->type() != struct_type_t::STRUCT_TYPE_BITS32
    && f_schema_column->type() != struct_type_t::STRUCT_TYPE_UINT32)
    {
        throw type_mismatch(
              "expected bits32 or uint32 type, received "
            + std::to_string(static_cast<int>(f_schema_column->type())));
    }

    return f_integer.f_value[0];
}


void cell::set_uint32(uint32_t value)
{
    if(f_schema_column->type() != struct_type_t::STRUCT_TYPE_BITS32
    && f_schema_column->type() != struct_type_t::STRUCT_TYPE_UINT32)
    {
        throw type_mismatch(
              "expected bits32 or uint32 type, received "
            + std::to_string(static_cast<int>(f_schema_column->type())));
    }

    f_integer.f_value[0] = value;
    f_integer.f_value[1] = 0;
    f_integer.f_value[2] = 0;
    f_integer.f_value[3] = 0;
    f_integer.f_value[4] = 0;
    f_integer.f_value[5] = 0;
    f_integer.f_value[6] = 0;
    f_integer.f_value[7] = 0;
}


int64_t cell::get_int64() const
{
    if(f_schema_column->type() != struct_type_t::STRUCT_TYPE_INT64)
    {
        throw type_mismatch(
              "expected int64 type, received "
            + std::to_string(static_cast<int>(f_schema_column->type())));
    }

    return f_integer.f_value[0];
}


void cell::set_int64(int64_t value)
{
    if(f_schema_column->type() != struct_type_t::STRUCT_TYPE_INT64)
    {
        throw type_mismatch(
              "expected int64 type, received "
            + std::to_string(static_cast<int>(f_schema_column->type())));
    }

    f_integer.f_value[0] = value;
    f_integer.f_value[1] = static_cast<int64_t>(value) < 0 ? -1 : 0;
    f_integer.f_value[2] = f_integer.f_value[1];
    f_integer.f_value[3] = f_integer.f_value[1];
    f_integer.f_value[4] = f_integer.f_value[1];
    f_integer.f_value[5] = f_integer.f_value[1];
    f_integer.f_value[6] = f_integer.f_value[1];
    f_integer.f_value[7] = f_integer.f_value[1];
}


uint64_t cell::get_uint64() const
{
    if(f_schema_column->type() != struct_type_t::STRUCT_TYPE_BITS64
    && f_schema_column->type() != struct_type_t::STRUCT_TYPE_UINT64
    && f_schema_column->type() != struct_type_t::STRUCT_TYPE_TIME
    && f_schema_column->type() != struct_type_t::STRUCT_TYPE_MSTIME
    && f_schema_column->type() != struct_type_t::STRUCT_TYPE_USTIME
    && f_schema_column->type() != struct_type_t::STRUCT_TYPE_REFERENCE
    && f_schema_column->type() != struct_type_t::STRUCT_TYPE_OID)
    {
        throw type_mismatch(
              "expected int64 type, received "
            + std::to_string(static_cast<int>(f_schema_column->type())));
    }

    return f_integer.f_value[0];
}


void cell::set_uint64(uint64_t value)
{
    if(f_schema_column->type() != struct_type_t::STRUCT_TYPE_BITS64
    && f_schema_column->type() != struct_type_t::STRUCT_TYPE_UINT64
    && f_schema_column->type() != struct_type_t::STRUCT_TYPE_TIME
    && f_schema_column->type() != struct_type_t::STRUCT_TYPE_MSTIME
    && f_schema_column->type() != struct_type_t::STRUCT_TYPE_USTIME
    && f_schema_column->type() != struct_type_t::STRUCT_TYPE_REFERENCE
    && f_schema_column->type() != struct_type_t::STRUCT_TYPE_OID)
    {
        throw type_mismatch(
              "expected int64 type, received "
            + std::to_string(static_cast<int>(f_schema_column->type())));
    }

    f_integer.f_value[0] = value;
    f_integer.f_value[1] = 0;
    f_integer.f_value[2] = 0;
    f_integer.f_value[3] = 0;
    f_integer.f_value[4] = 0;
    f_integer.f_value[5] = 0;
    f_integer.f_value[6] = 0;
    f_integer.f_value[7] = 0;
}


int512_t cell::get_int128() const
{
    if(f_schema_column->type() != struct_type_t::STRUCT_TYPE_INT128)
    {
        throw type_mismatch(
              "expected int128 type, received "
            + std::to_string(static_cast<int>(f_schema_column->type())));
    }

    return f_integer;
}


void cell::set_int128(int512_t value)
{
    if(f_schema_column->type() != struct_type_t::STRUCT_TYPE_INT128)
    {
        throw type_mismatch(
              "expected int128 type, received "
            + std::to_string(static_cast<int>(f_schema_column->type())));
    }

    f_integer = value;
}


uint512_t cell::get_uint128() const
{
    if(f_schema_column->type() != struct_type_t::STRUCT_TYPE_UINT128)
    {
        throw type_mismatch(
              "expected uint128 type, received "
            + std::to_string(static_cast<int>(f_schema_column->type())));
    }

    return f_integer;
}


void cell::set_uint128(uint512_t value)
{
    if(f_schema_column->type() != struct_type_t::STRUCT_TYPE_UINT128)
    {
        throw type_mismatch(
              "expected uint128 type, received "
            + std::to_string(static_cast<int>(f_schema_column->type())));
    }

    f_integer = value;
}


int512_t cell::get_int256() const
{
    if(f_schema_column->type() != struct_type_t::STRUCT_TYPE_INT256)
    {
        throw type_mismatch(
              "expected int256 type, received "
            + std::to_string(static_cast<int>(f_schema_column->type())));
    }

    return f_integer;
}


void cell::set_int256(int512_t value)
{
    if(f_schema_column->type() != struct_type_t::STRUCT_TYPE_INT256)
    {
        throw type_mismatch(
              "expected int256 type, received "
            + std::to_string(static_cast<int>(f_schema_column->type())));
    }

    f_integer = value;
}


uint512_t cell::get_uint256() const
{
    if(f_schema_column->type() != struct_type_t::STRUCT_TYPE_UINT256)
    {
        throw type_mismatch(
              "expected uint256 type, received "
            + std::to_string(static_cast<int>(f_schema_column->type())));
    }

    return f_integer;
}


void cell::set_uint256(uint512_t value)
{
    if(f_schema_column->type() != struct_type_t::STRUCT_TYPE_UINT256)
    {
        throw type_mismatch(
              "expected uint256 type, received "
            + std::to_string(static_cast<int>(f_schema_column->type())));
    }

    f_integer = value;
}


int512_t cell::get_int512() const
{
    if(f_schema_column->type() != struct_type_t::STRUCT_TYPE_INT512)
    {
        throw type_mismatch(
              "expected int512 type, received "
            + std::to_string(static_cast<int>(f_schema_column->type())));
    }

    return f_integer;
}


void cell::set_int512(int512_t value)
{
    if(f_schema_column->type() != struct_type_t::STRUCT_TYPE_INT512)
    {
        throw type_mismatch(
              "expected int512 type, received "
            + std::to_string(static_cast<int>(f_schema_column->type())));
    }

    f_integer = value;
}


uint512_t cell::get_uint512() const
{
    if(f_schema_column->type() != struct_type_t::STRUCT_TYPE_UINT512)
    {
        throw type_mismatch(
              "expected uint512 type, received "
            + std::to_string(static_cast<int>(f_schema_column->type())));
    }

    return f_integer;
}


void cell::set_uint512(uint512_t value)
{
    if(f_schema_column->type() != struct_type_t::STRUCT_TYPE_UINT512)
    {
        throw type_mismatch(
              "expected uint512 type, received "
            + std::to_string(static_cast<int>(f_schema_column->type())));
    }

    f_integer = value;
}


float cell::get_float32() const
{
    if(f_schema_column->type() != struct_type_t::STRUCT_TYPE_FLOAT32)
    {
        throw type_mismatch(
              "expected float32 type, received "
            + std::to_string(static_cast<int>(f_schema_column->type())));
    }

    return f_float_value;
}


void cell::set_float32(float value)
{
    if(f_schema_column->type() != struct_type_t::STRUCT_TYPE_FLOAT32)
    {
        throw type_mismatch(
              "expected float32 type, received "
            + std::to_string(static_cast<int>(f_schema_column->type())));
    }

    f_float_value = value;
}


double cell::get_float64() const
{
    if(f_schema_column->type() != struct_type_t::STRUCT_TYPE_FLOAT64)
    {
        throw type_mismatch(
              "expected float64 type, received "
            + std::to_string(static_cast<int>(f_schema_column->type())));
    }

    return f_float_value;
}


void cell::set_float64(double value)
{
    if(f_schema_column->type() != struct_type_t::STRUCT_TYPE_FLOAT64)
    {
        throw type_mismatch(
              "expected float64 type, received "
            + std::to_string(static_cast<int>(f_schema_column->type())));
    }

    f_float_value = value;
}


long double cell::get_float128() const
{
    if(f_schema_column->type() != struct_type_t::STRUCT_TYPE_FLOAT128)
    {
        throw type_mismatch(
              "expected float128 type, received "
            + std::to_string(static_cast<int>(f_schema_column->type())));
    }

    return f_float_value;
}


void cell::set_float128(long double value)
{
    if(f_schema_column->type() != struct_type_t::STRUCT_TYPE_FLOAT128)
    {
        throw type_mismatch(
              "expected float128 type, received "
            + std::to_string(static_cast<int>(f_schema_column->type())));
    }

    f_float_value = value;
}


version_t cell::get_version() const
{
    if(f_schema_column->type() != struct_type_t::STRUCT_TYPE_VERSION)
    {
        throw type_mismatch(
              "expected version type, received "
            + std::to_string(static_cast<int>(f_schema_column->type())));
    }

    return version_t(f_integer.f_value[0]);
}


void cell::set_version(version_t value)
{
    if(f_schema_column->type() != struct_type_t::STRUCT_TYPE_VERSION)
    {
        throw type_mismatch(
              "expected version type, received "
            + std::to_string(static_cast<int>(f_schema_column->type())));
    }

    f_integer.f_value[0] = value.to_binary();
    f_integer.f_value[1] = 0;
    f_integer.f_value[2] = 0;
    f_integer.f_value[3] = 0;
    f_integer.f_value[4] = 0;
    f_integer.f_value[5] = 0;
    f_integer.f_value[6] = 0;
    f_integer.f_value[7] = 0;
}


std::string cell::get_string() const
{
    if(f_schema_column->type() != struct_type_t::STRUCT_TYPE_CSTRING
    || f_schema_column->type() != struct_type_t::STRUCT_TYPE_P8STRING
    || f_schema_column->type() != struct_type_t::STRUCT_TYPE_P16STRING
    || f_schema_column->type() != struct_type_t::STRUCT_TYPE_P32STRING)
    {
        throw type_mismatch(
              "expected cstring, p8/16/32string type, received "
            + std::to_string(static_cast<int>(f_schema_column->type())));
    }

    return f_string;
}


void cell::set_string(std::string const & value)
{
    if(f_schema_column->type() != struct_type_t::STRUCT_TYPE_CSTRING
    || f_schema_column->type() != struct_type_t::STRUCT_TYPE_P8STRING
    || f_schema_column->type() != struct_type_t::STRUCT_TYPE_P16STRING
    || f_schema_column->type() != struct_type_t::STRUCT_TYPE_P32STRING)
    {
        throw type_mismatch(
              "expected cstring, p8/16/32string type, received "
            + std::to_string(static_cast<int>(f_schema_column->type())));
    }

    f_string = value;
}





} // namespace snapdatabase
// vim: ts=4 sw=4 et
