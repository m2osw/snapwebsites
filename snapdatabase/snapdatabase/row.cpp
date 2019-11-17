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
#include    "snapdatabase/row.h"


// last include
//
#include    <snapdev/poison.h>



namespace snapdatabase
{



row::row(table::pointer_t t)
    : f_table(t)
{
}


buffer_t row::to_binary() const
{
    buffer_t result;

    auto push_uint8 = [&](uint8_t value)
        {
            result.push_back(value);
        };

    auto push_uint16 = [&](uint16_t value)
        {
            result.push_back(value >> 0);
            result.push_back(value >> 8);
        };

    auto push_uint32 = [&](uint32_t value)
        {
            result.push_back(value);
        };

    auto push_uint64 = [&](uint64_t value)
        {
            result.push_back(value);
        };

    table::pointer_t t(f_table.lock());
    schema_column::map_by_id_t columns(t->columns_by_id());

    for(auto c : f_cells)
    {
        schema_column::pointer_t schema(c.second->schema());

        column_id_t const id(schema->column_id());
        push_uint16(id);

        switch(schema->type())
        {
        case struct_type_t::STRUCT_TYPE_VOID:
            snap::NOTUSED(c.second->is_void());
            break;

        case struct_type_t::STRUCT_TYPE_BITS8:
        case struct_type_t::STRUCT_TYPE_UINT8:
            push_uint8(c.second->get_uint8());
            break;

        case struct_type_t::STRUCT_TYPE_INT8:
            push_uint8(c.second->get_int8());
            break;

        case struct_type_t::STRUCT_TYPE_BITS16:
        case struct_type_t::STRUCT_TYPE_UINT16:
            push_uint16(c.second->get_uint16());
            break;

        case struct_type_t::STRUCT_TYPE_INT16:
            push_uint16(c.second->get_int16());
            break;

        case struct_type_t::STRUCT_TYPE_BITS32:
        case struct_type_t::STRUCT_TYPE_UINT32:
        case struct_type_t::STRUCT_TYPE_VERSION:
            push_uint32(c.second->get_uint32());
            break;

        case struct_type_t::STRUCT_TYPE_INT32:
            push_uint32(c.second->get_int32());
            break;

        case struct_type_t::STRUCT_TYPE_BITS64:
        case struct_type_t::STRUCT_TYPE_UINT64:
        case struct_type_t::STRUCT_TYPE_REFERENCE:
        case struct_type_t::STRUCT_TYPE_OID:
        case struct_type_t::STRUCT_TYPE_TIME:
        case struct_type_t::STRUCT_TYPE_MSTIME:
        case struct_type_t::STRUCT_TYPE_USTIME:
            push_uint64(c.second->get_uint64());
            break;

        case struct_type_t::STRUCT_TYPE_INT64:
            push_uint64(c.second->get_int64());
            break;

        case struct_type_t::STRUCT_TYPE_BITS128:
        case struct_type_t::STRUCT_TYPE_UINT128:
            {
                uint512_t const value(c.second->get_uint128());
                push_uint64(value.f_value[0]);
                push_uint64(value.f_value[1]);
            }
            break;

        case struct_type_t::STRUCT_TYPE_INT128:
            {
                int512_t const value(c.second->get_int128());
                push_uint64(value.f_value[0]);
                push_uint64(value.f_value[1]);
            }
            break;

        case struct_type_t::STRUCT_TYPE_BITS256:
        case struct_type_t::STRUCT_TYPE_UINT256:
            {
                uint512_t const value(c.second->get_uint256());
                push_uint64(value.f_value[0]);
                push_uint64(value.f_value[1]);
                push_uint64(value.f_value[2]);
                push_uint64(value.f_value[3]);
            }
            break;

        case struct_type_t::STRUCT_TYPE_INT256:
            {
                int512_t const value(c.second->get_int256());
                push_uint64(value.f_value[0]);
                push_uint64(value.f_value[1]);
                push_uint64(value.f_value[2]);
                push_uint64(value.f_value[3]);
            }
            break;

        case struct_type_t::STRUCT_TYPE_BITS512:
        case struct_type_t::STRUCT_TYPE_UINT512:
            {
                uint512_t const value(c.second->get_uint512());
                push_uint64(value.f_value[0]);
                push_uint64(value.f_value[1]);
                push_uint64(value.f_value[2]);
                push_uint64(value.f_value[3]);
                push_uint64(value.f_value[4]);
                push_uint64(value.f_value[5]);
                push_uint64(value.f_value[6]);
                push_uint64(value.f_value[7]);
            }
            break;

        case struct_type_t::STRUCT_TYPE_INT512:
            {
                int512_t const value(c.second->get_int512());
                push_uint64(value.f_value[0]);
                push_uint64(value.f_value[1]);
                push_uint64(value.f_value[2]);
                push_uint64(value.f_value[3]);
                push_uint64(value.f_value[4]);
                push_uint64(value.f_value[5]);
                push_uint64(value.f_value[6]);
                push_uint64(value.f_value[7]);
            }
            break;

        case struct_type_t::STRUCT_TYPE_FLOAT32:
            {
                union fi {
                    uint32_t    f_int = 0;
                    float       f_float;
                };
                fi value;
                value.f_float = c.second->get_float32();
                push_uint32(value.f_int);
            }
            break;

        case struct_type_t::STRUCT_TYPE_FLOAT64:
            {
                union fi {
                    uint64_t    f_int = 0;
                    double      f_float;
                };
                fi value;
                value.f_float = c.second->get_float64();
                push_uint64(value.f_int);
            }
            break;

        case struct_type_t::STRUCT_TYPE_FLOAT128:
            {
                union fi {
                    uint64_t        f_int[2] = { 0, 0 };
                    long double     f_float;
                };
                fi value;
                value.f_float = c.second->get_float128();
                push_uint64(value.f_int[0]);
                push_uint64(value.f_int[1]);
            }
            break;

        case struct_type_t::STRUCT_TYPE_CSTRING:
            {
                std::string const value(c.second->get_string());
                uint8_t const * s(reinterpret_cast<uint8_t const *>(value.c_str()));
                result.insert(result.end(), s, s + value.length() + 1);
            }
            break;

        case struct_type_t::STRUCT_TYPE_P8STRING:
            {
                std::string const value(c.second->get_string());
                if(value.length() > 255)
                {
                    throw out_of_bounds(
                              "string to long for a P8STRING (max: 255, actually: "
                            + std::to_string(value.length())
                            + ").");
                }
                push_uint8(value.length());
                if(!value.empty())
                {
                    uint8_t const * s(reinterpret_cast<uint8_t const *>(value.c_str()));
                    result.insert(result.end(), s, s + value.length());
                }
            }
            break;

        case struct_type_t::STRUCT_TYPE_P16STRING:
            {
                std::string const value(c.second->get_string());
                if(value.length() > 65535)
                {
                    throw out_of_bounds(
                              "string to long for a P16STRING (max: 64Kb, actually: "
                            + std::to_string(value.length())
                            + ").");
                }
                uint16_t const size(static_cast<uint16_t>(value.length()));
                push_uint16(size);
                if(size > 0)
                {
                    uint8_t const * s(reinterpret_cast<uint8_t const *>(value.c_str()));
                    result.insert(result.end(), s, s + size);
                }
            }
            break;

        case struct_type_t::STRUCT_TYPE_P32STRING:
            {
                std::string const value(c.second->get_string());
                if(value.length() > 65535)
                {
                    throw out_of_bounds(
                              "string to long for a P32STRING (max: 4Gb, actually: "
                            + std::to_string(value.length())
                            + ").");
                }
                uint32_t const size(static_cast<uint32_t>(value.length()));
                push_uint32(size);
                if(size > 0)
                {
                    uint8_t const * s(reinterpret_cast<uint8_t const *>(value.c_str()));
                    result.insert(result.end(), s, s + size);
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
                    + std::to_string(static_cast<int>(schema->type()))
                    + ") to convert a cell from binary.");

        }
    }

    return result;
}


void row::from_binary(buffer_t const & blob)
{
    table::pointer_t t(f_table.lock());
    schema_column::map_by_id_t columns(t->columns_by_id());
    size_t pos(0);

    auto get_uint8 = [&]()
        {
            if(pos + 1 > blob.size())
            {
                throw unexpected_eof("blob too small for a [u]int8_t.");
            }

            uint8_t const value(blob[pos]);
            pos += 1;
            return value;
        };

    auto get_uint16 = [&]()
        {
            if(pos + 2 > blob.size())
            {
                throw unexpected_eof("blob too small for a [u]int16_t.");
            }

            uint16_t const value((static_cast<uint16_t>(blob[pos + 0]) <<  0)
                               + (static_cast<uint16_t>(blob[pos + 1]) <<  8));
            pos += 2;
            return value;
        };

    auto get_uint32 = [&]()
        {
            if(pos + 4 > blob.size())
            {
                throw unexpected_eof("blob too small for a [u]int32_t.");
            }

            uint32_t const value((static_cast<uint32_t>(blob[pos + 0]) <<  0)
                               + (static_cast<uint32_t>(blob[pos + 1]) <<  8)
                               + (static_cast<uint32_t>(blob[pos + 2]) << 16)
                               + (static_cast<uint32_t>(blob[pos + 3]) << 24));
            pos += 4;
            return value;
        };

    auto get_uint64 = [&]()
        {
            if(pos + 8 > blob.size())
            {
                throw unexpected_eof("blob too small for a [u]int64_t.");
            }

            uint64_t const value((static_cast<uint64_t>(blob[pos + 0]) <<  0)
                               + (static_cast<uint64_t>(blob[pos + 1]) <<  8)
                               + (static_cast<uint64_t>(blob[pos + 2]) << 16)
                               + (static_cast<uint64_t>(blob[pos + 3]) << 24)
                               + (static_cast<uint64_t>(blob[pos + 4]) << 32)
                               + (static_cast<uint64_t>(blob[pos + 5]) << 40)
                               + (static_cast<uint64_t>(blob[pos + 6]) << 48)
                               + (static_cast<uint64_t>(blob[pos + 7]) << 56));
            pos += 8;
            return value;
        };

    while(pos < blob.size())
    {
        column_id_t const id(get_uint16());
        auto it(columns.find(id));
        if(it == columns.end())
        {
            throw column_not_found(
                    "column with identifier "
                    + std::to_string(static_cast<int>(id))
                    + " not found.");
        }

        cell::pointer_t v(std::make_shared<cell>(it->second));
        switch(it->second->type())
        {
        case struct_type_t::STRUCT_TYPE_VOID:
            v->set_void();
            break;

        case struct_type_t::STRUCT_TYPE_BITS8:
        case struct_type_t::STRUCT_TYPE_UINT8:
            v->set_uint8(get_uint8());
            break;

        case struct_type_t::STRUCT_TYPE_INT8:
            v->set_int8(get_uint8());
            break;

        case struct_type_t::STRUCT_TYPE_BITS16:
        case struct_type_t::STRUCT_TYPE_UINT16:
            v->set_uint16(get_uint16());
            break;

        case struct_type_t::STRUCT_TYPE_INT16:
            v->set_int16(get_uint16());
            break;

        case struct_type_t::STRUCT_TYPE_BITS32:
        case struct_type_t::STRUCT_TYPE_UINT32:
        case struct_type_t::STRUCT_TYPE_VERSION:
            v->set_uint32(get_uint32());
            break;

        case struct_type_t::STRUCT_TYPE_INT32:
            v->set_int32(get_uint32());
            break;

        case struct_type_t::STRUCT_TYPE_BITS64:
        case struct_type_t::STRUCT_TYPE_UINT64:
        case struct_type_t::STRUCT_TYPE_REFERENCE:
        case struct_type_t::STRUCT_TYPE_OID:
        case struct_type_t::STRUCT_TYPE_TIME:
        case struct_type_t::STRUCT_TYPE_MSTIME:
        case struct_type_t::STRUCT_TYPE_USTIME:
            v->set_uint64(get_uint64());
            break;

        case struct_type_t::STRUCT_TYPE_INT64:
            v->set_int64(get_uint64());
            break;

        case struct_type_t::STRUCT_TYPE_BITS128:
        case struct_type_t::STRUCT_TYPE_UINT128:
            {
                uint512_t value;
                value.f_value[0] = get_uint64();
                value.f_value[1] = get_uint64();
                v->set_uint128(value);
            }
            break;

        case struct_type_t::STRUCT_TYPE_INT128:
            {
                int512_t value;
                value.f_value[0] = get_uint64();
                value.f_value[1] = get_uint64();
                value.f_value[2] = static_cast<int64_t>(value.f_value[1]) < 0 ? -1 : 0;
                value.f_value[3] = value.f_value[2];
                value.f_value[4] = value.f_value[2];
                value.f_value[5] = value.f_value[2];
                value.f_value[6] = value.f_value[2];
                value.f_high_value = value.f_value[2];
                v->set_int128(value);
            }
            break;

        case struct_type_t::STRUCT_TYPE_BITS256:
        case struct_type_t::STRUCT_TYPE_UINT256:
            {
                uint512_t value;
                value.f_value[0] = get_uint64();
                value.f_value[1] = get_uint64();
                value.f_value[2] = get_uint64();
                value.f_value[3] = get_uint64();
                v->set_uint256(value);
            }
            break;

        case struct_type_t::STRUCT_TYPE_INT256:
            {
                int512_t value;
                value.f_value[0] = get_uint64();
                value.f_value[1] = get_uint64();
                value.f_value[2] = get_uint64();
                value.f_value[3] = get_uint64();
                value.f_value[4] = static_cast<int64_t>(value.f_value[3]) < 0 ? -1 : 0;
                value.f_value[5] = value.f_value[4];
                value.f_value[6] = value.f_value[4];
                value.f_high_value = value.f_value[4];
                v->set_int256(value);
            }
            break;

        case struct_type_t::STRUCT_TYPE_BITS512:
        case struct_type_t::STRUCT_TYPE_UINT512:
            {
                uint512_t value;
                value.f_value[0] = get_uint64();
                value.f_value[1] = get_uint64();
                value.f_value[2] = get_uint64();
                value.f_value[3] = get_uint64();
                value.f_value[4] = get_uint64();
                value.f_value[5] = get_uint64();
                value.f_value[6] = get_uint64();
                value.f_value[7] = get_uint64();
                v->set_uint512(value);
            }
            break;

        case struct_type_t::STRUCT_TYPE_INT512:
            {
                int512_t value;
                value.f_value[0] = get_uint64();
                value.f_value[1] = get_uint64();
                value.f_value[2] = get_uint64();
                value.f_value[3] = get_uint64();
                value.f_value[4] = get_uint64();
                value.f_value[5] = get_uint64();
                value.f_value[6] = get_uint64();
                value.f_value[7] = get_uint64();
                v->set_int512(value);
            }
            break;

        case struct_type_t::STRUCT_TYPE_FLOAT32:
            {
                uint32_t const value(get_uint32());
                v->set_float32(*reinterpret_cast<float const *>(&value));
            }
            break;

        case struct_type_t::STRUCT_TYPE_FLOAT64:
            {
                uint64_t const value(get_uint64());
                v->set_float64(*reinterpret_cast<double const *>(&value));
            }
            break;

        case struct_type_t::STRUCT_TYPE_FLOAT128:
            {
                uint64_t const value[2] = { get_uint64(), get_uint64() };
                v->set_float128(*reinterpret_cast<long double const *>(value));
            }
            break;

        case struct_type_t::STRUCT_TYPE_CSTRING:
            {
                size_t const start(pos);
                uint8_t const * e(blob.data() + start);
                while(*e != '\0')
                {
                    ++pos;
                    if(pos >= blob.size())
                    {
                        throw unexpected_eof("blob too small for this string.");
                    }
                    ++e;
                }
                v->set_string(std::string(blob.data() + start, e));
                ++pos;  // skip the '\0'
            }
            break;

        case struct_type_t::STRUCT_TYPE_P8STRING:
            {
                if(pos >= blob.size())
                {
                    throw unexpected_eof("blob too small for this string.");
                }

                uint8_t const size(get_uint8());

                if(pos + size > blob.size())
                {
                    throw unexpected_eof("blob too small for this string.");
                }

                v->set_string(std::string(blob.data() + pos, blob.data() + pos + size));
                pos += size;
            }
            break;

        case struct_type_t::STRUCT_TYPE_P16STRING:
            {
                if(pos >= blob.size())
                {
                    throw unexpected_eof("blob too small for this string.");
                }

                uint16_t const size(get_uint16());

                if(pos + size > blob.size())
                {
                    throw unexpected_eof("blob too small for this string.");
                }

                v->set_string(std::string(blob.data() + pos, blob.data() + pos + size));
                pos += size;
            }
            break;

        case struct_type_t::STRUCT_TYPE_P32STRING:
            {
                if(pos >= blob.size())
                {
                    throw unexpected_eof("blob too small for this string.");
                }

                uint32_t const size(get_uint32());

                if(pos + size > blob.size())
                {
                    throw unexpected_eof("blob too small for this string.");
                }

                v->set_string(std::string(blob.data() + pos, blob.data() + pos + size));
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
                    + std::to_string(static_cast<int>(it->second->type()))
                    + ") to convert a cell from binary.");

        }

        f_cells[it->second->name()] = v;
    }
}



} // namespace snapdatabase
// vim: ts=4 sw=4 et
