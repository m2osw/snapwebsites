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

#include    "snapdatabase/data/convert.h"


// snapdev lib
//
#include    "snapdev/not_reached.h"


// last include
//
#include    <snapdev/poison.h>



namespace snapdatabase
{



std::uint8_t read_uint8(buffer_t const & buffer, size_t & pos)
{
    pos += sizeof(std::uint8_t);

    return buffer[pos - 1];
}


std::uint16_t read_be_uint16(buffer_t const & buffer, size_t & pos)
{
    pos += sizeof(std::uint16_t);

    return (static_cast<std::uint16_t>(buffer[pos - 2]) << 8)
         | (static_cast<std::uint16_t>(buffer[pos - 1]) << 0);
}


std::uint32_t read_be_uint32(buffer_t const & buffer, size_t & pos)
{
    pos += sizeof(std::uint32_t);

    return (static_cast<std::uint32_t>(buffer[pos - 4]) << 24)
         + (static_cast<std::uint32_t>(buffer[pos - 3]) << 16)
         + (static_cast<std::uint32_t>(buffer[pos - 2]) <<  8)
         + (static_cast<std::uint32_t>(buffer[pos - 1]) <<  0);
}


std::uint64_t read_be_uint64(buffer_t const & buffer, size_t & pos)
{
    pos += sizeof(std::uint64_t);

    return (static_cast<std::uint64_t>(buffer[pos - 8]) << 56)
         + (static_cast<std::uint64_t>(buffer[pos - 7]) << 48)
         + (static_cast<std::uint64_t>(buffer[pos - 6]) << 40)
         + (static_cast<std::uint64_t>(buffer[pos - 5]) << 32)
         + (static_cast<std::uint64_t>(buffer[pos - 4]) << 24)
         + (static_cast<std::uint64_t>(buffer[pos - 3]) << 16)
         + (static_cast<std::uint64_t>(buffer[pos - 2]) <<  8)
         + (static_cast<std::uint64_t>(buffer[pos - 1]) <<  0);
}


void push_uint8(buffer_t & buffer, std::uint8_t value)
{
    buffer.push_back(value);
}


void push_be_uint16(buffer_t & buffer, std::uint16_t value)
{
    buffer.push_back(value >> 8);
    buffer.push_back(value >> 0);
}


void push_be_uint32(buffer_t & buffer, std::uint32_t value)
{
    buffer.push_back(value >> 24);
    buffer.push_back(value >> 16);
    buffer.push_back(value >>  8);
    buffer.push_back(value >>  0);
}


void push_be_uint64(buffer_t & buffer, std::uint64_t value)
{
    buffer.push_back(value >> 56);
    buffer.push_back(value >> 48);
    buffer.push_back(value >> 40);
    buffer.push_back(value >> 32);
    buffer.push_back(value >> 24);
    buffer.push_back(value >> 16);
    buffer.push_back(value >>  8);
    buffer.push_back(value >>  0);
}





cell::cell(schema_column::pointer_t c)
    : f_schema_column(c)
{
}


schema_column::pointer_t cell::schema() const
{
    return f_schema_column;
}


struct_type_t cell::type() const
{
    return f_schema_column->type();
}


bool cell::has_fixed_type() const
{
    return type_with_fixed_size(type());
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


oid_t cell::get_oid() const
{
    verify_cell_type({
              struct_type_t::STRUCT_TYPE_OID
        });

    return f_integer.f_value[0];
}


void cell::set_oid(oid_t oid)
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


void cell::column_id_to_binary(buffer_t & buffer) const
{
    column_id_t const id(f_schema_column->column_id());

    // for the actual data, we use big endian so that way we can use memcmp()
    // to compare different values and get the correct results
    //
    push_be_uint16(buffer, id);
}


column_id_t cell::column_id_from_binary(buffer_t const & buffer, size_t & pos)
{
    return static_cast<column_id_t>(read_be_uint16(buffer, pos));
}


void cell::value_to_binary(buffer_t & buffer) const
{
    switch(f_schema_column->type())
    {
    case struct_type_t::STRUCT_TYPE_VOID:
        // nothing to save for this one
        break;

    case struct_type_t::STRUCT_TYPE_BITS8:
    case struct_type_t::STRUCT_TYPE_UINT8:
    case struct_type_t::STRUCT_TYPE_INT8:
        push_uint8(buffer, f_integer.f_value[0]);
        break;

    case struct_type_t::STRUCT_TYPE_BITS16:
    case struct_type_t::STRUCT_TYPE_UINT16:
    case struct_type_t::STRUCT_TYPE_INT16:
        push_be_uint16(buffer, f_integer.f_value[0]);
        break;

    case struct_type_t::STRUCT_TYPE_BITS32:
    case struct_type_t::STRUCT_TYPE_UINT32:
    case struct_type_t::STRUCT_TYPE_VERSION:
    case struct_type_t::STRUCT_TYPE_INT32:
        push_be_uint32(buffer, f_integer.f_value[0]);
        break;

    case struct_type_t::STRUCT_TYPE_BITS64:
    case struct_type_t::STRUCT_TYPE_UINT64:
    case struct_type_t::STRUCT_TYPE_REFERENCE:
    case struct_type_t::STRUCT_TYPE_OID:
    case struct_type_t::STRUCT_TYPE_TIME:
    case struct_type_t::STRUCT_TYPE_MSTIME:
    case struct_type_t::STRUCT_TYPE_USTIME:
    case struct_type_t::STRUCT_TYPE_INT64:
        push_be_uint64(buffer, f_integer.f_value[0]);
        break;

    case struct_type_t::STRUCT_TYPE_BITS128:
    case struct_type_t::STRUCT_TYPE_UINT128:
    case struct_type_t::STRUCT_TYPE_INT128:
        push_be_uint64(buffer, f_integer.f_value[1]);
        push_be_uint64(buffer, f_integer.f_value[0]);
        break;

    case struct_type_t::STRUCT_TYPE_BITS256:
    case struct_type_t::STRUCT_TYPE_UINT256:
    case struct_type_t::STRUCT_TYPE_INT256:
        push_be_uint64(buffer, f_integer.f_value[3]);
        push_be_uint64(buffer, f_integer.f_value[2]);
        push_be_uint64(buffer, f_integer.f_value[1]);
        push_be_uint64(buffer, f_integer.f_value[0]);
        break;

    case struct_type_t::STRUCT_TYPE_BITS512:
    case struct_type_t::STRUCT_TYPE_UINT512:
    case struct_type_t::STRUCT_TYPE_INT512:
        push_be_uint64(buffer, f_integer.f_value[7]);
        push_be_uint64(buffer, f_integer.f_value[6]);
        push_be_uint64(buffer, f_integer.f_value[5]);
        push_be_uint64(buffer, f_integer.f_value[4]);
        push_be_uint64(buffer, f_integer.f_value[3]);
        push_be_uint64(buffer, f_integer.f_value[2]);
        push_be_uint64(buffer, f_integer.f_value[1]);
        push_be_uint64(buffer, f_integer.f_value[0]);
        break;

    case struct_type_t::STRUCT_TYPE_FLOAT32:
        {
            union fi {
                uint32_t    f_int = 0;
                float       f_float;
            };
            fi value;
            value.f_float = f_float_value;
            push_be_uint32(buffer, value.f_int);
        }
        break;

    case struct_type_t::STRUCT_TYPE_FLOAT64:
        {
            union fi {
                uint64_t    f_int = 0;
                double      f_float;
            };
            fi value;
            value.f_float = f_float_value;
            push_be_uint64(buffer, value.f_int);
        }
        break;

    case struct_type_t::STRUCT_TYPE_FLOAT128:
        {
            union fi {
                uint64_t        f_int[2] = { 0, 0 };
                long double     f_float;
            };
            fi const * value(reinterpret_cast<fi const *>(&f_float_value));
            //value.f_float = f_float_value;
            push_be_uint64(buffer, value->f_int[1]);
            push_be_uint64(buffer, value->f_int[0]);
        }
        break;

    case struct_type_t::STRUCT_TYPE_P8STRING:
        {
            size_t const size(f_string.length());
            if(size > 255)
            {
                throw out_of_bounds(
                          "string to long for a P8STRING (max: 255, actually: "
                        + std::to_string(size)
                        + ").");
            }
            push_uint8(buffer, size);
            if(size > 0)
            {
                uint8_t const * s(reinterpret_cast<uint8_t const *>(f_string.c_str()));
                buffer.insert(buffer.end(), s, s + size);
            }
        }
        break;

    case struct_type_t::STRUCT_TYPE_P16STRING:
        {
            size_t const size(f_string.length());
            if(size > 65535)
            {
                throw out_of_bounds(
                          "string to long for a P16STRING (max: 64Kb, actually: "
                        + std::to_string(size)
                        + ").");
            }
            push_be_uint16(buffer, size);
            if(size > 0)
            {
                uint8_t const * s(reinterpret_cast<uint8_t const *>(f_string.c_str()));
                buffer.insert(buffer.end(), s, s + size);
            }
        }
        break;

    case struct_type_t::STRUCT_TYPE_P32STRING:
        {
            size_t const size(f_string.length());
            if(size > 4294967295)
            {
                throw out_of_bounds(
                          "string to long for a P32STRING (max: 4Gb, actually: "
                        + std::to_string(size)
                        + ").");
            }
            push_be_uint32(buffer, size);
            if(size > 0)
            {
                uint8_t const * s(reinterpret_cast<uint8_t const *>(f_string.c_str()));
                buffer.insert(buffer.end(), s, s + size);
            }
        }
        break;

    case struct_type_t::STRUCT_TYPE_STRUCTURE:
    case struct_type_t::STRUCT_TYPE_ARRAY8:
    case struct_type_t::STRUCT_TYPE_ARRAY16:
    case struct_type_t::STRUCT_TYPE_ARRAY32:
    case struct_type_t::STRUCT_TYPE_BUFFER8:
    case struct_type_t::STRUCT_TYPE_BUFFER16:
    case struct_type_t::STRUCT_TYPE_BUFFER32:
    case struct_type_t::STRUCT_TYPE_END:
    case struct_type_t::STRUCT_TYPE_RENAMED:
        throw type_mismatch(
                  "Unexpected type ("
                + std::to_string(static_cast<int>(f_schema_column->type()))
                + ") to convert a cell to binary.");

    }
}


void cell::value_from_binary(buffer_t const & buffer, size_t & pos)
{
    switch(f_schema_column->type())
    {
    case struct_type_t::STRUCT_TYPE_VOID:
        // nothing to save for this one
        break;

    case struct_type_t::STRUCT_TYPE_BITS8:
    case struct_type_t::STRUCT_TYPE_UINT8:
        set_uinteger(read_uint8(buffer, pos));
        break;

    case struct_type_t::STRUCT_TYPE_INT8:
        set_integer(static_cast<std::int8_t>(read_uint8(buffer, pos)));
        break;

    case struct_type_t::STRUCT_TYPE_BITS16:
    case struct_type_t::STRUCT_TYPE_UINT16:
        set_integer(read_be_uint16(buffer, pos));
        break;

    case struct_type_t::STRUCT_TYPE_INT16:
        set_integer(static_cast<std::int16_t>(read_be_uint16(buffer, pos)));
        break;

    case struct_type_t::STRUCT_TYPE_BITS32:
    case struct_type_t::STRUCT_TYPE_UINT32:
    case struct_type_t::STRUCT_TYPE_VERSION:
        set_integer(read_be_uint32(buffer, pos));
        break;

    case struct_type_t::STRUCT_TYPE_INT32:
        set_integer(static_cast<std::int32_t>(read_be_uint32(buffer, pos)));
        break;

    case struct_type_t::STRUCT_TYPE_BITS64:
    case struct_type_t::STRUCT_TYPE_UINT64:
    case struct_type_t::STRUCT_TYPE_REFERENCE:
    case struct_type_t::STRUCT_TYPE_OID:
        set_integer(read_be_uint64(buffer, pos));
        break;

    case struct_type_t::STRUCT_TYPE_TIME:
    case struct_type_t::STRUCT_TYPE_MSTIME:
    case struct_type_t::STRUCT_TYPE_USTIME:
    case struct_type_t::STRUCT_TYPE_INT64:
        set_integer(static_cast<std::int64_t>(read_be_uint64(buffer, pos)));
        break;

    case struct_type_t::STRUCT_TYPE_BITS128:
    case struct_type_t::STRUCT_TYPE_UINT128:
        f_integer.f_value[7] = 0;
        f_integer.f_value[6] = 0;
        f_integer.f_value[5] = 0;
        f_integer.f_value[4] = 0;
        f_integer.f_value[3] = 0;
        f_integer.f_value[2] = 0;
        f_integer.f_value[1] = read_be_uint64(buffer, pos);
        f_integer.f_value[0] = read_be_uint64(buffer, pos);
        break;

    case struct_type_t::STRUCT_TYPE_INT128:
        f_integer.f_value[1] = read_be_uint64(buffer, pos);
        f_integer.f_value[0] = read_be_uint64(buffer, pos);

        // extend sign
        f_integer.f_value[7] = static_cast<std::int64_t>(f_integer.f_value[1]) < 0 ? -1 : 0;
        f_integer.f_value[6] = f_integer.f_value[7];
        f_integer.f_value[5] = f_integer.f_value[7];
        f_integer.f_value[4] = f_integer.f_value[7];
        f_integer.f_value[3] = f_integer.f_value[7];
        f_integer.f_value[2] = f_integer.f_value[7];
        break;

    case struct_type_t::STRUCT_TYPE_BITS256:
    case struct_type_t::STRUCT_TYPE_UINT256:
        f_integer.f_value[7] = 0;
        f_integer.f_value[6] = 0;
        f_integer.f_value[5] = 0;
        f_integer.f_value[4] = 0;
        f_integer.f_value[3] = read_be_uint64(buffer, pos);
        f_integer.f_value[2] = read_be_uint64(buffer, pos);
        f_integer.f_value[1] = read_be_uint64(buffer, pos);
        f_integer.f_value[0] = read_be_uint64(buffer, pos);
        break;

    case struct_type_t::STRUCT_TYPE_INT256:
        f_integer.f_value[3] = read_be_uint64(buffer, pos);
        f_integer.f_value[2] = read_be_uint64(buffer, pos);
        f_integer.f_value[1] = read_be_uint64(buffer, pos);
        f_integer.f_value[0] = read_be_uint64(buffer, pos);

        // extend sign
        f_integer.f_value[7] = static_cast<std::int64_t>(f_integer.f_value[3]) < 0 ? -1 : 0;
        f_integer.f_value[6] = f_integer.f_value[7];
        f_integer.f_value[5] = f_integer.f_value[7];
        f_integer.f_value[4] = f_integer.f_value[7];
        break;

    case struct_type_t::STRUCT_TYPE_BITS512:
    case struct_type_t::STRUCT_TYPE_UINT512:
    case struct_type_t::STRUCT_TYPE_INT512:
        f_integer.f_value[7] = read_be_uint64(buffer, pos);
        f_integer.f_value[6] = read_be_uint64(buffer, pos);
        f_integer.f_value[5] = read_be_uint64(buffer, pos);
        f_integer.f_value[4] = read_be_uint64(buffer, pos);
        f_integer.f_value[3] = read_be_uint64(buffer, pos);
        f_integer.f_value[2] = read_be_uint64(buffer, pos);
        f_integer.f_value[1] = read_be_uint64(buffer, pos);
        f_integer.f_value[0] = read_be_uint64(buffer, pos);
        break;

    case struct_type_t::STRUCT_TYPE_FLOAT32:
        {
            union fi {
                uint32_t    f_int = 0;
                float       f_float;
            };
            fi value;
            value.f_int = read_be_uint32(buffer, pos);
            f_float_value = value.f_float;
        }
        break;

    case struct_type_t::STRUCT_TYPE_FLOAT64:
        {
            union fi {
                uint64_t    f_int = 0;
                double      f_float;
            };
            fi value;
            value.f_int = read_be_uint64(buffer, pos);
            f_float_value = value.f_float;
        }
        break;

    case struct_type_t::STRUCT_TYPE_FLOAT128:
        {
            union fi {
                uint64_t        f_int[2] = { 0, 0 };
                long double     f_float;
            };
            fi value;
            value.f_int[1] = read_be_uint64(buffer, pos);
            value.f_int[0] = read_be_uint64(buffer, pos);
            f_float_value = value.f_float;
        }
        break;

    case struct_type_t::STRUCT_TYPE_P8STRING:
        {
            size_t const size(read_uint8(buffer, pos));
            f_string.resize(size);
            memcpy(&f_string[0], buffer.data() + pos, size);
            pos += size;
        }
        break;

    case struct_type_t::STRUCT_TYPE_P16STRING:
        {
            size_t const size(read_be_uint16(buffer, pos));
            f_string.resize(size);
            memcpy(&f_string[0], buffer.data() + pos, size);
            pos += size;
        }
        break;

    case struct_type_t::STRUCT_TYPE_P32STRING:
        {
            size_t const size(read_be_uint32(buffer, pos));
            f_string.resize(size);
            memcpy(&f_string[0], buffer.data() + pos, size);
            pos += size;
        }
        break;

    case struct_type_t::STRUCT_TYPE_STRUCTURE:
    case struct_type_t::STRUCT_TYPE_ARRAY8:
    case struct_type_t::STRUCT_TYPE_ARRAY16:
    case struct_type_t::STRUCT_TYPE_ARRAY32:
    case struct_type_t::STRUCT_TYPE_BUFFER8:
    case struct_type_t::STRUCT_TYPE_BUFFER16:
    case struct_type_t::STRUCT_TYPE_BUFFER32:
    case struct_type_t::STRUCT_TYPE_END:
    case struct_type_t::STRUCT_TYPE_RENAMED:
        throw type_mismatch(
                  "Unexpected type ("
                + std::to_string(static_cast<int>(f_schema_column->type()))
                + ") to convert a cell to binary.");

    }
}


void cell::copy_from(cell const & source)
{
    if(f_schema_column->type() == source.f_schema_column->type())
    {
        // no conversion needed, a direct copy will work just fine
        //
        // Note: this happens 99.9% of the time since in most cases you
        //       update your schema by adding and removing columns, but
        //       not the type of existing columns
        //
        switch(source.f_schema_column->type())
        {
        case struct_type_t::STRUCT_TYPE_VOID:
            // void is an empty string
            break;

        case struct_type_t::STRUCT_TYPE_BITS8:
        case struct_type_t::STRUCT_TYPE_BITS16:
        case struct_type_t::STRUCT_TYPE_BITS32:
        case struct_type_t::STRUCT_TYPE_BITS64:
        case struct_type_t::STRUCT_TYPE_BITS128:
        case struct_type_t::STRUCT_TYPE_BITS256:
        case struct_type_t::STRUCT_TYPE_BITS512:
        case struct_type_t::STRUCT_TYPE_INT8:
        case struct_type_t::STRUCT_TYPE_UINT8:
        case struct_type_t::STRUCT_TYPE_INT16:
        case struct_type_t::STRUCT_TYPE_UINT16:
        case struct_type_t::STRUCT_TYPE_INT32:
        case struct_type_t::STRUCT_TYPE_UINT32:
        case struct_type_t::STRUCT_TYPE_INT64:
        case struct_type_t::STRUCT_TYPE_UINT64:
        case struct_type_t::STRUCT_TYPE_INT128:
        case struct_type_t::STRUCT_TYPE_UINT128:
        case struct_type_t::STRUCT_TYPE_INT256:
        case struct_type_t::STRUCT_TYPE_UINT256:
        case struct_type_t::STRUCT_TYPE_INT512:
        case struct_type_t::STRUCT_TYPE_UINT512:
        case struct_type_t::STRUCT_TYPE_REFERENCE:
        case struct_type_t::STRUCT_TYPE_OID:
        case struct_type_t::STRUCT_TYPE_TIME:
        case struct_type_t::STRUCT_TYPE_MSTIME:
        case struct_type_t::STRUCT_TYPE_USTIME:
        case struct_type_t::STRUCT_TYPE_VERSION:
            f_integer = source.f_integer;
            break;

        case struct_type_t::STRUCT_TYPE_FLOAT32:
        case struct_type_t::STRUCT_TYPE_FLOAT64:
        case struct_type_t::STRUCT_TYPE_FLOAT128:
            f_float_value = source.f_float_value;
            break;

        case struct_type_t::STRUCT_TYPE_P8STRING:
        case struct_type_t::STRUCT_TYPE_P16STRING:
        case struct_type_t::STRUCT_TYPE_P32STRING:
            f_string = source.f_string;
            break;

        case struct_type_t::STRUCT_TYPE_STRUCTURE:
        case struct_type_t::STRUCT_TYPE_ARRAY8:
        case struct_type_t::STRUCT_TYPE_ARRAY16:
        case struct_type_t::STRUCT_TYPE_ARRAY32:
        case struct_type_t::STRUCT_TYPE_BUFFER8:
        case struct_type_t::STRUCT_TYPE_BUFFER16:
        case struct_type_t::STRUCT_TYPE_BUFFER32:
        case struct_type_t::STRUCT_TYPE_END:
        case struct_type_t::STRUCT_TYPE_RENAMED:
            throw type_mismatch(
                      "Unexpected type ("
                    + std::to_string(static_cast<int>(f_schema_column->type()))
                    + ") to convert a cell to another.");

        }
    }
    else
    {
        // TODO: we want to specialize some conversions to avoid the double
        //       conversion; at the same time, this only happens when someone
        //       updates their schema
        //
        std::string value;
        switch(source.f_schema_column->type())
        {
        case struct_type_t::STRUCT_TYPE_VOID:
            // void is an empty string
            break;

        case struct_type_t::STRUCT_TYPE_BITS8:
        case struct_type_t::STRUCT_TYPE_BITS16:
        case struct_type_t::STRUCT_TYPE_BITS32:
        case struct_type_t::STRUCT_TYPE_BITS64:
        case struct_type_t::STRUCT_TYPE_BITS128:
        case struct_type_t::STRUCT_TYPE_BITS256:
        case struct_type_t::STRUCT_TYPE_BITS512:
        case struct_type_t::STRUCT_TYPE_INT8:
        case struct_type_t::STRUCT_TYPE_UINT8:
        case struct_type_t::STRUCT_TYPE_INT16:
        case struct_type_t::STRUCT_TYPE_UINT16:
        case struct_type_t::STRUCT_TYPE_INT32:
        case struct_type_t::STRUCT_TYPE_UINT32:
        case struct_type_t::STRUCT_TYPE_INT64:
        case struct_type_t::STRUCT_TYPE_UINT64:
        case struct_type_t::STRUCT_TYPE_INT128:
        case struct_type_t::STRUCT_TYPE_UINT128:
        case struct_type_t::STRUCT_TYPE_INT256:
        case struct_type_t::STRUCT_TYPE_UINT256:
        case struct_type_t::STRUCT_TYPE_INT512:
        case struct_type_t::STRUCT_TYPE_UINT512:
        case struct_type_t::STRUCT_TYPE_REFERENCE:
        case struct_type_t::STRUCT_TYPE_OID:
        case struct_type_t::STRUCT_TYPE_TIME:
        case struct_type_t::STRUCT_TYPE_MSTIME:
        case struct_type_t::STRUCT_TYPE_USTIME:
        case struct_type_t::STRUCT_TYPE_VERSION:
            {
                buffer_t buf(sizeof(source.f_integer.f_value));
                std::memcpy(buf.data(), source.f_integer.f_value, sizeof(source.f_integer.f_value));
                value = typed_buffer_to_string(source.f_schema_column->type(), buf, 16);
            }
            break;

        case struct_type_t::STRUCT_TYPE_FLOAT32:
        case struct_type_t::STRUCT_TYPE_FLOAT64:
        case struct_type_t::STRUCT_TYPE_FLOAT128:
            {
                buffer_t buf(sizeof(source.f_float_value));
                memcpy(buf.data(), &source.f_float_value, sizeof(source.f_float_value));
                value = typed_buffer_to_string(source.f_schema_column->type(), buf, 16);
            }
            break;

        case struct_type_t::STRUCT_TYPE_P8STRING:
        case struct_type_t::STRUCT_TYPE_P16STRING:
        case struct_type_t::STRUCT_TYPE_P32STRING:
            value = source.f_string;
            break;

        case struct_type_t::STRUCT_TYPE_STRUCTURE:
        case struct_type_t::STRUCT_TYPE_ARRAY8:
        case struct_type_t::STRUCT_TYPE_ARRAY16:
        case struct_type_t::STRUCT_TYPE_ARRAY32:
        case struct_type_t::STRUCT_TYPE_BUFFER8:
        case struct_type_t::STRUCT_TYPE_BUFFER16:
        case struct_type_t::STRUCT_TYPE_BUFFER32:
        case struct_type_t::STRUCT_TYPE_END:
        case struct_type_t::STRUCT_TYPE_RENAMED:
            throw type_mismatch(
                      "Unexpected type ("
                    + std::to_string(static_cast<int>(f_schema_column->type()))
                    + ") to convert a cell to another.");

        }

        switch(f_schema_column->type())
        {
        case struct_type_t::STRUCT_TYPE_VOID:
            // void is an empty string
            break;

        case struct_type_t::STRUCT_TYPE_BITS8:
        case struct_type_t::STRUCT_TYPE_BITS16:
        case struct_type_t::STRUCT_TYPE_BITS32:
        case struct_type_t::STRUCT_TYPE_BITS64:
        case struct_type_t::STRUCT_TYPE_BITS128:
        case struct_type_t::STRUCT_TYPE_BITS256:
        case struct_type_t::STRUCT_TYPE_BITS512:
        case struct_type_t::STRUCT_TYPE_UINT8:
        case struct_type_t::STRUCT_TYPE_UINT16:
        case struct_type_t::STRUCT_TYPE_UINT32:
        case struct_type_t::STRUCT_TYPE_UINT64:
        case struct_type_t::STRUCT_TYPE_UINT128:
        case struct_type_t::STRUCT_TYPE_UINT256:
        case struct_type_t::STRUCT_TYPE_UINT512:
        case struct_type_t::STRUCT_TYPE_REFERENCE:
        case struct_type_t::STRUCT_TYPE_OID:
        case struct_type_t::STRUCT_TYPE_VERSION:
            {
                buffer_t buf(string_to_typed_buffer(struct_type_t::STRUCT_TYPE_UINT512, value));
                assert(buf.size() == sizeof(source.f_integer));
                memcpy(f_integer.f_value, buf.data(), sizeof(f_integer.f_value));
            }
            break;

        case struct_type_t::STRUCT_TYPE_INT8:
        case struct_type_t::STRUCT_TYPE_INT16:
        case struct_type_t::STRUCT_TYPE_INT32:
        case struct_type_t::STRUCT_TYPE_INT64:
        case struct_type_t::STRUCT_TYPE_INT128:
        case struct_type_t::STRUCT_TYPE_INT256:
        case struct_type_t::STRUCT_TYPE_INT512:
        case struct_type_t::STRUCT_TYPE_TIME:
        case struct_type_t::STRUCT_TYPE_MSTIME:
        case struct_type_t::STRUCT_TYPE_USTIME:
            {
                buffer_t const buf(string_to_typed_buffer(struct_type_t::STRUCT_TYPE_INT512, value));
                assert(buf.size() == sizeof(source.f_integer));
                memcpy(f_integer.f_value, buf.data(), sizeof(f_integer.f_value));
            }
            break;

        case struct_type_t::STRUCT_TYPE_FLOAT32:
            {
                buffer_t buf(string_to_typed_buffer(struct_type_t::STRUCT_TYPE_FLOAT32, value));
                assert(buf.size() == sizeof(float));
                f_float_value = *reinterpret_cast<float *>(buf.data());
            }
            break;

        case struct_type_t::STRUCT_TYPE_FLOAT64:
            {
                buffer_t buf(string_to_typed_buffer(struct_type_t::STRUCT_TYPE_FLOAT64, value));
                assert(buf.size() == sizeof(double));
                f_float_value = *reinterpret_cast<double *>(buf.data());
            }
            break;

        case struct_type_t::STRUCT_TYPE_FLOAT128:
            {
                buffer_t buf(string_to_typed_buffer(struct_type_t::STRUCT_TYPE_FLOAT128, value));
                assert(buf.size() == sizeof(long double));
                f_float_value = *reinterpret_cast<long double *>(buf.data());
            }
            break;

        case struct_type_t::STRUCT_TYPE_P8STRING:
        case struct_type_t::STRUCT_TYPE_P16STRING:
        case struct_type_t::STRUCT_TYPE_P32STRING:
            f_string = value;
            break;

        case struct_type_t::STRUCT_TYPE_STRUCTURE:
        case struct_type_t::STRUCT_TYPE_ARRAY8:
        case struct_type_t::STRUCT_TYPE_ARRAY16:
        case struct_type_t::STRUCT_TYPE_ARRAY32:
        case struct_type_t::STRUCT_TYPE_BUFFER8:
        case struct_type_t::STRUCT_TYPE_BUFFER16:
        case struct_type_t::STRUCT_TYPE_BUFFER32:
        case struct_type_t::STRUCT_TYPE_END:
        case struct_type_t::STRUCT_TYPE_RENAMED:
            throw type_mismatch(
                      "Unexpected type ("
                    + std::to_string(static_cast<int>(f_schema_column->type()))
                    + ") to convert a cell to another.");

        }
    }
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
