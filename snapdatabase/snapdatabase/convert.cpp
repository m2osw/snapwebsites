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
 * \brief Various convertions between data types.
 *
 * At this point, we mainly want to convert a structure data type to a string
 * and vice versa. This is useful to convert values defined in the XML file
 * such as the default value.
 *
 * We also have functions to convert strings to integers of 8, 16, 32, 64,
 * 128, 256, and 512 bits.
 */

// self
//
#include    "snapdatabase/convert.h"


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


buffer_t string_to_uinteger(std::string const & value, size_t max_size)
{
    buffer_t result;
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


std::string uinteger_to_string(buffer_t value, int bytes_for_size, int base)
{
    if(value.size() > static_cast<size_t>(bytes_for_size))
    {
        throw snapdatabase_out_of_range(
                  "Value too large ("
                + std::to_string(value.size() * 8)
                + ") for this field (max: "
                + std::to_string(bytes_for_size * 8)
                + ").");
    }

    uint512_t v;
    std::memcpy(reinterpret_cast<uint8_t *>(v.f_value), reinterpret_cast<uint8_t *>(value.data()), value.size());

    if(v.is_zero())
    {
        return std::string("0");
    }

    char const * intro("");
    std::string result;
    switch(base)
    {
    case 2:
        while(!v.is_zero())
        {
            result += (v.f_value[0] & 1) + '0';
            v.lsr(1);
        }
        intro = "0b";
        break;

    case 8:
        while(!v.is_zero())
        {
            result += (v.f_value[0] & 7) + '0';
            v.lsr(3);
        }
        intro = "0";
        break;

    case 10:
        {
            uint512_t remainder;
            uint512_t ten;
            ten.f_value[0] = 10;
            while(!v.is_zero())
            {
                v.div(ten, remainder);
                result += remainder.f_value[0] + '0';
            }
        }
        break;

    case 16:
        while(!v.is_zero())
        {
            int const digit(v.f_value[0]);
            if(digit >= 10)
            {
                result += digit + 'A';
            }
            else
            {
                result += digit + '0';
            }
            v.lsr(4);
        }
        intro = "0x";
        break;

    }

    std::reverse(result.begin(), result.end());
    return intro + result;
}


buffer_t string_to_integer(std::string const & value, size_t max_size)
{
    buffer_t result;
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


std::string integer_to_string(buffer_t value, int bytes_for_size, int base)
{
    if(static_cast<int8_t>(value.data()[value.size() - 1]) < 0)
    {
        // TODO: this is a tad bit ugly... (i.e. triple memcpy()!!!)
        //
        int512_t v;
        std::memcpy(v.f_value, value.data(), value.size());
        v = -v;
        buffer_t neg(reinterpret_cast<uint8_t const *>(v.f_value), reinterpret_cast<uint8_t const *>(v.f_value + 8));
        return "-" + uinteger_to_string(neg, bytes_for_size, base);
    }
    else
    {
        return uinteger_to_string(value, bytes_for_size, base);
    }
}



template<typename T>
buffer_t string_to_float(std::string const & value, std::function<T(char const *, char **)> f)
{
    buffer_t result;
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


template<typename T>
std::string float_to_string(buffer_t value)
{
    // TBD: we may want to specify the format
    if(value.size() != sizeof(T))
    {
        throw snapdatabase_out_of_range(
                  "Value buffer has an unexpected size ("
                + std::to_string(value.size())
                + ") for this field (expected floating point size: "
                  BOOST_PP_STRINGIZE(sizeof(T))
                  ").");
    }
    std::ostringstream ss;
    ss << *reinterpret_cast<T *>(value.data());
    return ss.str();
}


buffer_t string_to_version(std::string const & value)
{
    buffer_t result;
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


std::string version_to_string(buffer_t value)
{
    if(value.size() != 4)
    {
        throw snapdatabase_out_of_range(
                  "A buffer representing a version must be exactly 4 bytes, not "
                + std::to_string(value.size())
                + ".");
    }
    version_t v(*reinterpret_cast<uint32_t *>(value.data()));
    return v.to_string();
}


buffer_t cstring_to_buffer(std::string const & value)
{
    buffer_t result;
    result.insert(result.end(), value.begin(), value.end());

    // null terminated
    char zero(0);
    result.insert(result.end(), reinterpret_cast<uint8_t *>(&zero),  reinterpret_cast<uint8_t *>(&zero) + sizeof(zero));

    return result;
}


std::string buffer_to_cstring(buffer_t const & value)
{
    if(value.empty())
    {
        throw snapdatabase_out_of_range(
                  "A C-String cannot be saved in an empty buffer ('\\0' missing).");
    }

    if(value[value.size() - 1] != '\0')
    {
        throw snapdatabase_out_of_range(
                  "C-String last byte cannot be anything else than '\\0'.");
    }

    return std::string(value.data(), value.data() + value.size() - 1);
}


buffer_t string_to_buffer(std::string const & value, size_t bytes_for_size)
{
    buffer_t result;
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


std::string buffer_to_string(buffer_t value, size_t bytes_for_size)
{
    if(value.size() < bytes_for_size)
    {
        throw snapdatabase_out_of_range(
                  "Buffer too small to incorporate the P-String size ("
                + std::to_string(value.size())
                + ", expected at least: "
                + std::to_string(bytes_for_size)
                + ").");
    }

    uint32_t size(0);
    memcpy(&size, value.data(), bytes_for_size);

    if(bytes_for_size + size > value.size())
    {
        throw snapdatabase_out_of_range(
                  "Buffer too small for the P-String characters (size: "
                + std::to_string(size)
                + ", character bytes in buffer: "
                + std::to_string(value.size() - bytes_for_size)
                + ").");
    }

    return std::string(value.data() + bytes_for_size, value.data() + bytes_for_size + size);
}


// TODO: add support for getdate()
buffer_t string_to_unix_time(std::string value, int fraction)
{
    struct tm t;
    memset(&t, 0, sizeof(t));
    std::string format("%Y-%m-%dT%T");
    std::string::size_type const pos(value.find('.'));
    int f(0);
    if(pos != std::string::npos)
    {
        std::string date_time(value.substr(0, pos));
        std::string::size_type zone(value.find_first_of("+-"));
        if(zone == std::string::npos)
        {
            zone = value.size();
        }
        else
        {
            format += "%z";
            date_time += value.substr(zone);
        }
        std::string frac(value.substr(pos, zone - pos));
        f = std::atoi(frac.c_str());
        if(f < 0
        || f >= fraction)
        {
            throw snapdatabase_out_of_range(
                      "Time fraction is out of bounds in \""
                    + value
                    + "\".");
        }

        strptime(date_time.c_str(), format.c_str(), &t);
    }
    else
    {
        std::string::size_type zone(value.find_first_of("+-"));
        if(zone != std::string::npos)
        {
            format += "%z";
        }

        strptime(value.c_str(), format.c_str(), &t);
    }

    time_t v(mktime(&t));
    uint64_t with_fraction(v * fraction + f);

    return buffer_t(reinterpret_cast<uint8_t const *>(&with_fraction), reinterpret_cast<uint8_t const *>(&with_fraction + 1));
}


std::string unix_time_to_string(buffer_t value, int fraction)
{
    uint64_t time;
    if(value.size() != sizeof(time))
    {
        throw snapdatabase_out_of_range(
                  "Buffer size is invalid for a time value (size: "
                + std::to_string(value.size())
                + ", expected size: "
                + std::to_string(sizeof(time))
                + ").");
    }
    memcpy(&time, value.data(), sizeof(time));
    time_t const v(time / fraction);
    struct tm t;
    gmtime_r(&v, &t);

    char buf[256];
    strftime(buf, sizeof(buf) - 1, "%FT%T", &t);
    buf[sizeof(buf) - 1] = '\0';

    std::string result(buf);

    if(fraction != 1)
    {
        result += ".";
        std::string frac(std::to_string(time % fraction));
        size_t sz(fraction == 1000 ? 3 : 6);
        while(frac.size() < sz)
        {
            frac = '0' + frac;
        }
        result += frac;
    }

    return result + "+0000";
}


} // no name namespace





buffer_t string_to_typed_buffer(struct_type_t type, std::string const & value)
{
    switch(type)
    {
    case struct_type_t::STRUCT_TYPE_BITS8:
    case struct_type_t::STRUCT_TYPE_UINT8:
        return string_to_uinteger(value, 8);

    case struct_type_t::STRUCT_TYPE_BITS16:
    case struct_type_t::STRUCT_TYPE_UINT16:
        return string_to_uinteger(value, 16);

    case struct_type_t::STRUCT_TYPE_BITS32:
    case struct_type_t::STRUCT_TYPE_UINT32:
        return string_to_uinteger(value, 32);

    case struct_type_t::STRUCT_TYPE_BITS64:
    case struct_type_t::STRUCT_TYPE_UINT64:
    case struct_type_t::STRUCT_TYPE_OID:
    case struct_type_t::STRUCT_TYPE_REFERENCE:
        return string_to_uinteger(value, 64);

    case struct_type_t::STRUCT_TYPE_BITS128:
    case struct_type_t::STRUCT_TYPE_UINT128:
        return string_to_uinteger(value, 128);

    case struct_type_t::STRUCT_TYPE_BITS256:
    case struct_type_t::STRUCT_TYPE_UINT256:
        return string_to_uinteger(value, 256);

    case struct_type_t::STRUCT_TYPE_BITS512:
    case struct_type_t::STRUCT_TYPE_UINT512:
        return string_to_uinteger(value, 512);

    case struct_type_t::STRUCT_TYPE_INT8:
        return string_to_integer(value, 8);

    case struct_type_t::STRUCT_TYPE_INT16:
        return string_to_integer(value, 16);

    case struct_type_t::STRUCT_TYPE_INT32:
        return string_to_integer(value, 32);

    case struct_type_t::STRUCT_TYPE_INT64:
        return string_to_integer(value, 64);

    case struct_type_t::STRUCT_TYPE_INT128:
        return string_to_integer(value, 128);

    case struct_type_t::STRUCT_TYPE_INT256:
        return string_to_integer(value, 256);

    case struct_type_t::STRUCT_TYPE_INT512:
        return string_to_integer(value, 512);

    case struct_type_t::STRUCT_TYPE_FLOAT32:
        return string_to_float<float>(value, std::strtof);

    case struct_type_t::STRUCT_TYPE_FLOAT64:
        return string_to_float<double>(value, std::strtod);

    case struct_type_t::STRUCT_TYPE_FLOAT128:
        return string_to_float<long double>(value, std::strtold);

    case struct_type_t::STRUCT_TYPE_VERSION:
        return string_to_version(value);

    case struct_type_t::STRUCT_TYPE_TIME:
        return string_to_unix_time(value, 1);

    case struct_type_t::STRUCT_TYPE_MSTIME:
        return string_to_unix_time(value, 1000);

    case struct_type_t::STRUCT_TYPE_USTIME:
        return string_to_unix_time(value, 1000000);

    case struct_type_t::STRUCT_TYPE_CSTRING:
        return cstring_to_buffer(value);

    case struct_type_t::STRUCT_TYPE_P8STRING:
        return string_to_buffer(value, 1);

    case struct_type_t::STRUCT_TYPE_P16STRING:
        return string_to_buffer(value, 2);

    case struct_type_t::STRUCT_TYPE_P32STRING:
        return string_to_buffer(value, 4);

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


std::string typed_buffer_to_string(struct_type_t type, buffer_t value, int base)
{
    switch(type)
    {
    case struct_type_t::STRUCT_TYPE_BITS8:
    case struct_type_t::STRUCT_TYPE_UINT8:
        return uinteger_to_string(value, 8, base);

    case struct_type_t::STRUCT_TYPE_BITS16:
    case struct_type_t::STRUCT_TYPE_UINT16:
        return uinteger_to_string(value, 16, base);

    case struct_type_t::STRUCT_TYPE_BITS32:
    case struct_type_t::STRUCT_TYPE_UINT32:
        return uinteger_to_string(value, 32, base);

    case struct_type_t::STRUCT_TYPE_BITS64:
    case struct_type_t::STRUCT_TYPE_UINT64:
    case struct_type_t::STRUCT_TYPE_REFERENCE:
    case struct_type_t::STRUCT_TYPE_OID:
        return uinteger_to_string(value, 64, base);

    case struct_type_t::STRUCT_TYPE_BITS128:
    case struct_type_t::STRUCT_TYPE_UINT128:
        return uinteger_to_string(value, 128, base);

    case struct_type_t::STRUCT_TYPE_BITS256:
    case struct_type_t::STRUCT_TYPE_UINT256:
        return uinteger_to_string(value, 256, base);

    case struct_type_t::STRUCT_TYPE_BITS512:
    case struct_type_t::STRUCT_TYPE_UINT512:
        return uinteger_to_string(value, 512, base);

    case struct_type_t::STRUCT_TYPE_INT8:
        return integer_to_string(value, 8, base);

    case struct_type_t::STRUCT_TYPE_INT16:
        return integer_to_string(value, 16, base);

    case struct_type_t::STRUCT_TYPE_INT32:
        return integer_to_string(value, 32, base);

    case struct_type_t::STRUCT_TYPE_INT64:
        return integer_to_string(value, 64, base);

    case struct_type_t::STRUCT_TYPE_INT128:
        return integer_to_string(value, 128, base);

    case struct_type_t::STRUCT_TYPE_INT256:
        return integer_to_string(value, 256, base);

    case struct_type_t::STRUCT_TYPE_INT512:
        return integer_to_string(value, 512, base);

    case struct_type_t::STRUCT_TYPE_FLOAT32:
        return float_to_string<float>(value);

    case struct_type_t::STRUCT_TYPE_FLOAT64:
        return float_to_string<double>(value);

    case struct_type_t::STRUCT_TYPE_FLOAT128:
        return float_to_string<long double>(value);

    case struct_type_t::STRUCT_TYPE_VERSION:
        return version_to_string(value);

    case struct_type_t::STRUCT_TYPE_TIME:
        return unix_time_to_string(value, 1);

    case struct_type_t::STRUCT_TYPE_MSTIME:
        return unix_time_to_string(value, 1000);

    case struct_type_t::STRUCT_TYPE_USTIME:
        return unix_time_to_string(value, 1000000);

    case struct_type_t::STRUCT_TYPE_CSTRING:
        return buffer_to_cstring(value);

    case struct_type_t::STRUCT_TYPE_P8STRING:
        return buffer_to_string(value, 1);

    case struct_type_t::STRUCT_TYPE_P16STRING:
        return buffer_to_string(value, 2);

    case struct_type_t::STRUCT_TYPE_P32STRING:
        return buffer_to_string(value, 4);

    case struct_type_t::STRUCT_TYPE_BUFFER8:
    case struct_type_t::STRUCT_TYPE_BUFFER16:
    case struct_type_t::STRUCT_TYPE_BUFFER32:
        throw snapdatabase_logic_error("Conversion not yet implemented...");

    default:
        //struct_type_t::STRUCT_TYPE_STRUCTURE:
        //struct_type_t::STRUCT_TYPE_ARRAY8:
        //struct_type_t::STRUCT_TYPE_ARRAY16:
        //struct_type_t::STRUCT_TYPE_ARRAY32:
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
