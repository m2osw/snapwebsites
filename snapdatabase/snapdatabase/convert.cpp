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
#include    "snapdatabase/schema.h"


// C++ lib
//
#include    <type_traits>
#include    <cstdlib>


// snaplogger lib
//
#include    <snaplogger/message.h>


// last include
//
#include    <snapdev/poison.h>



namespace snapdatabase
{



namespace
{

enum class number_type_t
{
    NUMBER_TYPE_BINARY,
    NUMBER_TYPE_OCTAL,
    NUMBER_TYPE_DECIMAL,
    NUMBER_TYPE_HEXADECIMAL
};


uint512_t string_to_int(std::string const & number, bool accept_negative_values)
{
    bool negative(false);
    char const * n(number.c_str());
    while(isspace(*n))
    {
        ++n;
    }
    if(*n == '+')
    {
        ++n;
    }
    else if(*n == '-')
    {
        if(!accept_negative_values)
        {
            throw invalid_number(
                      "Negative values are not accepted, \""
                    + number
                    + "\" is not valid.");
        }

        ++n;
        negative = true;
    }
    uint512_t result;
    bool expect_quote(false);
    number_type_t t(number_type_t::NUMBER_TYPE_DECIMAL);
    if(*n == '0')
    {
        if(n[1] == 'x'
        || n[1] == 'X')
        {
            n += 2;
            t = number_type_t::NUMBER_TYPE_HEXADECIMAL;
        }
        else if(n[1] == 'b'
             || n[1] == 'B')
        {
            n += 2;
            t = number_type_t::NUMBER_TYPE_BINARY;
        }
        else
        {
            ++n;
            t = number_type_t::NUMBER_TYPE_OCTAL;
        }
    }
    else if(*n == 'x'
         || *n == 'X')
    {
        if(n[1] == '\'')
        {
            n += 2;
            t = number_type_t::NUMBER_TYPE_HEXADECIMAL;
            expect_quote = true;
        }
    }

    switch(t)
    {
    case number_type_t::NUMBER_TYPE_BINARY:
        while(*n >= '0' && *n <= '1')
        {
            uint512_t digit;
            digit.f_value[0] = *n - '0';

            // do result * 2 with one add
            result += result;       // x2

            result += digit;
            ++n;
        }
        break;

    case number_type_t::NUMBER_TYPE_OCTAL:
        while(*n >= '0' && *n <= '7')
        {
            uint512_t digit;
            digit.f_value[0] = *n - '0';

            // do result * 8 with a few adds
            result += result;       // x2
            result += result;       // x4
            result += result;       // x8

            result += digit;
            ++n;
        }
        break;

    case number_type_t::NUMBER_TYPE_DECIMAL:
        while(*n >= '0' && *n <= '9')
        {
            uint512_t digit;
            digit.f_value[0] = *n - '0';

            // do result * 10 with a few adds
            result += result;       // x2
            uint512_t eight(result);
            eight += eight;         // x4
            eight += eight;         // x8
            result += eight;        // x2 + x8 = x10

            result += digit;
            ++n;
        }
        break;

    case number_type_t::NUMBER_TYPE_HEXADECIMAL:
        for(;;)
        {
            uint512_t digit;
            if(*n >= '0' && *n <= '9')
            {
                digit.f_value[0] = *n - '0';
            }
            else if((*n >= 'a' && *n <= 'f') || (*n >= 'A' && *n <= 'F'))
            {
                digit.f_value[0] = (*n & 0xDF) - ('A' + 10);
            }
            else
            {
                break;
            }

            // do result * 16 with a few adds
            result += result;       // x2
            result += result;       // x4
            result += result;       // x8
            result += result;       // x16

            result += digit;
            ++n;
        }
        break;

    }

    if(expect_quote)
    {
        if(*n != '\'')
        {
            throw invalid_number(
                      "Closing quote missing in \""
                    + number
                    + "\".");
        }
        ++n;
    }

    while(isspace(*n))
    {
        ++n;
    }

    if(*n != '\0')
    {
        throw invalid_number(
                  "Could not convert number \""
                + number
                + "\" to a valid uint512_t value.");
    }

    return negative ? -result : result;
}


buffer_t & string_to_uinteger(buffer_t & result, std::string const & value, size_t max_size)
{
    uint512_t const n(string_to_int(value, false));

    if(max_size != 512 && n.bit_size() > max_size)
    {
        throw snapdatabase_out_of_range(
                  "Number \""
                + value
                + "\" too large for an "
                + std::to_string(max_size)
                + " bit value.");
    }

    result.insert(result.end()
                , reinterpret_cast<uint8_t const *>(&n.f_value)
                , reinterpret_cast<uint8_t const *>(&n.f_value) + max_size / 8);

    return result;
}


buffer_t & string_to_integer(buffer_t & result, std::string const & value, size_t max_size)
{
    int512_t const n(string_to_int(value, true));

    if(max_size != 512 && n.bit_size() > max_size)
    {
        throw snapdatabase_out_of_range(
                  "Number \""
                + value
                + "\" too large for a signed "
                + std::to_string(max_size)
                + " bit value.");
    }

    result.insert(result.end()
                , reinterpret_cast<uint8_t const *>(&n.f_value)
                ,  reinterpret_cast<uint8_t const *>(&n.f_value) + max_size / 8);

    return result;
}


template<typename T>
buffer_t & string_to_float(buffer_t & result, std::string const & value, std::function<T(char const *, char **)> f)
{
    char * e(nullptr);
    errno = 0;
    T r(f(value.c_str(), &e));
    if(errno == ERANGE)
    {
        throw snapdatabase_out_of_range(
                  "Floating point number \""
                + value
                + "\" out of range.");
    }

    // ignore ending spaces
    //
    while(isspace(*e))
    {
        ++e;
    }

    if(*e != '\0')
    {
        throw invalid_number(
                  "Floating point number \""
                + value
                + "\" includes invalid numbers.");
    }

    result.insert(result.end()
                , reinterpret_cast<uint8_t *>(&r)
                , reinterpret_cast<uint8_t *>(&r) + sizeof(r));

    return result;
}


//buffer_t & string_to_float(buffer_t & result, std::string const & value)
//{
//    char * e(nullptr);
//    errno = 0;
//    double r(strtod(value.c_str(), &e));
//    if(errno == ERANGE)
//    {
//        throw snapdatabase_out_of_range(
//                  "Floating point number \""
//                + value
//                + "\" out of range.");
//    }
//
//    // ignore ending spaces
//    //
//    while(isspace(*e))
//    {
//        ++e;
//    }
//
//    if(*e != '\0')
//    {
//        throw invalid_number(
//                  "Floating point number \""
//                + value
//                + "\" includes invalid numbers.");
//    }
//
//    result.insert(result.end()
//                , reinterpret_cast<uint8_t *>(&r)
//                , reinterpret_cast<uint8_t *>(&r) + sizeof(r));
//
//    return result;
//}


//buffer_t & string_to_float128(buffer_t & result, std::string const & value)
//{
//    char * e(nullptr);
//    errno = 0;
//    long double r(strtold(value.c_str(), &e));
//    if(errno == ERANGE)
//    {
//        throw snapdatabase_out_of_range(
//                  "Floating point number \""
//                + value
//                + "\" out of range.");
//    }
//
//    // ignore ending spaces
//    //
//    while(isspace(*e))
//    {
//        ++e;
//    }
//
//    if(*e != '\0')
//    {
//        throw invalid_number(
//                  "Floating point number \""
//                + value
//                + "\" includes invalid numbers.");
//    }
//
//    result.insert(result.end()
//                , reinterpret_cast<uint8_t *>(&r)
//                , reinterpret_cast<uint8_t *>(&r) + sizeof(r));
//
//    return result;
//}


buffer_t & string_to_version(buffer_t & result, std::string const & value)
{
    std::string::size_type const pos(value.find('.'));
    if(pos == std::string::npos)
    {
        throw snapdatabase_out_of_range(
                  "Version \""
                + value
                + "\" must include a period (.) between the major and minor numbers.");
    }

    std::string const version_major(value.substr(0, pos));
    std::string const version_minor(value.substr(pos + 1));

    uint512_t const a(string_to_int(version_major, false));
    uint512_t const b(string_to_int(version_minor, false));

    if(a.bit_size() > 16
    || b.bit_size() > 16)
    {
        throw snapdatabase_out_of_range(
                  "One or both of the major or minor numbers from version \""
                + value
                + "\" are too large for a version number (max. is 65535).");
    }

    version_t const v(a.f_value[0], b.f_value[0]);
    uint32_t const binary(v.to_binary());

    result.insert(result.end()
                , reinterpret_cast<uint8_t const *>(&binary)
                , reinterpret_cast<uint8_t const *>(&binary) + sizeof(binary));

    return result;
}


buffer_t & cstring_to_buffer(buffer_t & result, std::string const & value)
{
    result.insert(result.end(), value.begin(), value.end());

    // null terminated
    char zero(0);
    result.insert(result.end(), reinterpret_cast<uint8_t *>(&zero),  reinterpret_cast<uint8_t *>(&zero) + sizeof(zero));

    return result;
}


buffer_t & string_to_buffer(buffer_t & result, std::string const & value, size_t bytes_for_size)
{
    uint32_t size(value.length());

    uint64_t max_size(1ULL << bytes_for_size * 8);

    if(size >= max_size)
    {
        throw snapdatabase_out_of_range(
                  "String too long ("
                + std::to_string(size)
                + ") for this field (max: "
                + std::to_string(max_size)
                + ").");
    }

    // WARNING: this copy works in Little Endian only
    //
    result.insert(result.end(), reinterpret_cast<uint8_t *>(&size),  reinterpret_cast<uint8_t *>(&size) + bytes_for_size);

    result.insert(result.end(), value.begin(), value.end());

    return result;
}



} // no name namespace





buffer_t string_to_typed_buffer(struct_type_t type, std::string const & value)
{
    buffer_t result;
    switch(type)
    {
    case struct_type_t::STRUCT_TYPE_BITS8:
    case struct_type_t::STRUCT_TYPE_UINT8:
        return string_to_uinteger(result, value, 8);

    case struct_type_t::STRUCT_TYPE_BITS16:
    case struct_type_t::STRUCT_TYPE_UINT16:
        return string_to_uinteger(result, value, 16);

    case struct_type_t::STRUCT_TYPE_BITS32:
    case struct_type_t::STRUCT_TYPE_UINT32:
        return string_to_uinteger(result, value, 32);

    case struct_type_t::STRUCT_TYPE_BITS64:
    case struct_type_t::STRUCT_TYPE_UINT64:
    case struct_type_t::STRUCT_TYPE_OID:
    case struct_type_t::STRUCT_TYPE_REFERENCE:
        return string_to_uinteger(result, value, 64);

    case struct_type_t::STRUCT_TYPE_BITS128:
    case struct_type_t::STRUCT_TYPE_UINT128:
        return string_to_uinteger(result, value, 128);

    case struct_type_t::STRUCT_TYPE_BITS256:
    case struct_type_t::STRUCT_TYPE_UINT256:
        return string_to_uinteger(result, value, 256);

    case struct_type_t::STRUCT_TYPE_BITS512:
    case struct_type_t::STRUCT_TYPE_UINT512:
        return string_to_uinteger(result, value, 512);

    case struct_type_t::STRUCT_TYPE_INT8:
        return string_to_integer(result, value, 8);

    case struct_type_t::STRUCT_TYPE_INT16:
        return string_to_integer(result, value, 16);

    case struct_type_t::STRUCT_TYPE_INT32:
        return string_to_integer(result, value, 32);

    case struct_type_t::STRUCT_TYPE_INT64:
        return string_to_integer(result, value, 64);

    case struct_type_t::STRUCT_TYPE_INT128:
        return string_to_integer(result, value, 128);

    case struct_type_t::STRUCT_TYPE_INT256:
        return string_to_integer(result, value, 256);

    case struct_type_t::STRUCT_TYPE_INT512:
        return string_to_integer(result, value, 512);

    case struct_type_t::STRUCT_TYPE_FLOAT32:
        return string_to_float<float>(result, value, std::strtof);

    case struct_type_t::STRUCT_TYPE_FLOAT64:
        return string_to_float<double>(result, value, std::strtod);

    case struct_type_t::STRUCT_TYPE_FLOAT128:
        return string_to_float<long double>(result, value, std::strtold);

    case struct_type_t::STRUCT_TYPE_VERSION:
        return string_to_version(result, value);

    case struct_type_t::STRUCT_TYPE_TIME:
    case struct_type_t::STRUCT_TYPE_MSTIME:
    case struct_type_t::STRUCT_TYPE_USTIME:
        throw snapdatabase_logic_error("Conversion not yet implemented...");

    case struct_type_t::STRUCT_TYPE_CSTRING:
        return cstring_to_buffer(result, value);

    case struct_type_t::STRUCT_TYPE_P8STRING:
        return string_to_buffer(result, value, 1);

    case struct_type_t::STRUCT_TYPE_P16STRING:
        return string_to_buffer(result, value, 2);

    case struct_type_t::STRUCT_TYPE_P32STRING:
        return string_to_buffer(result, value, 4);

    case struct_type_t::STRUCT_TYPE_BUFFER8:
    case struct_type_t::STRUCT_TYPE_BUFFER16:
    case struct_type_t::STRUCT_TYPE_BUFFER32:
        throw snapdatabase_logic_error("Conversion not yet implemented...");

    default:
        //struct_type_t::STRUCT_TYPE_ARRAY8:
        //struct_type_t::STRUCT_TYPE_ARRAY16:
        //struct_type_t::STRUCT_TYPE_ARRAY32:
        //struct_type_t::STRUCT_TYPE_STRUCTURE:
        //struct_type_t::STRUCT_TYPE_END
        //struct_type_t::STRUCT_TYPE_VOID
        //struct_type_t::STRUCT_TYPE_RENAMED
        throw snapdatabase_logic_error(
              "Unexpected structure type ("
            + std::to_string(static_cast<int>(type))
            + ") to convert a string to a buffer");

    }
}


std::string typed_buffer_to_string(struct_type_t type, buffer_t value)
{
    switch(type)
    {
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
    case struct_type_t::STRUCT_TYPE_FLOAT32:
    case struct_type_t::STRUCT_TYPE_FLOAT64:
    case struct_type_t::STRUCT_TYPE_VERSION:
    case struct_type_t::STRUCT_TYPE_TIME:
    case struct_type_t::STRUCT_TYPE_MSTIME:
    case struct_type_t::STRUCT_TYPE_USTIME:
    case struct_type_t::STRUCT_TYPE_CSTRING:
    case struct_type_t::STRUCT_TYPE_P8STRING:
    case struct_type_t::STRUCT_TYPE_P16STRING:
    case struct_type_t::STRUCT_TYPE_P32STRING:
    case struct_type_t::STRUCT_TYPE_STRUCTURE:
    case struct_type_t::STRUCT_TYPE_ARRAY8:
    case struct_type_t::STRUCT_TYPE_ARRAY16:
    case struct_type_t::STRUCT_TYPE_ARRAY32:
    case struct_type_t::STRUCT_TYPE_BUFFER8:
    case struct_type_t::STRUCT_TYPE_BUFFER16:
    case struct_type_t::STRUCT_TYPE_BUFFER32:
    case struct_type_t::STRUCT_TYPE_REFERENCE:
    case struct_type_t::STRUCT_TYPE_OID:

    default:
        //struct_type_t::STRUCT_TYPE_END
        //struct_type_t::STRUCT_TYPE_VOID
        //struct_type_t::STRUCT_TYPE_RENAMED
        throw snapdatabase_logic_error(
              "Unexpected structure type ("
            + std::to_string(static_cast<int>(type))
            + ") to convert a string to a buffer");

    }
}


int64_t convert_to_int(std::string const & value, size_t max_size)
{
    int512_t n(string_to_int(value, true));

    if(n.bit_size() > max_size)
    {
        throw snapdatabase_out_of_range(
                  "Number \""
                + value
                + "\" too large for a signed "
                + std::to_string(max_size)
                + " bit value.");
    }

    return n.f_value[0];
}


uint64_t convert_to_uint(std::string const & value, size_t max_size)
{
    uint512_t n(string_to_int(value, false));

    if(n.bit_size() > max_size)
    {
        throw snapdatabase_out_of_range(
                  "Number \""
                + value
                + "\" too large for a signed "
                + std::to_string(max_size)
                + " bit value.");
    }

    return n.f_value[0];
}



} // namespace snapdatabase
// vim: ts=4 sw=4 et
