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
 * \brief Script handling implementation.
 *
 * In various places we allow scripts to be run. Scripts are used to filter
 * the data and generate keys of secondary indexes.
 */

// self
//
#include    "snapdatabase/data/script.h"


// snapwebsites lib (TODO: remove dependency--especially because we do not want Qt in snapdatabase!)
//
#include    <snapwebsites/snap_expr.h>


// snaplogger lib
//
#include    <snaplogger/message.h>


// last include
//
#include    <snapdev/poison.h>



namespace snapdatabase
{



buffer_t compile_script(std::string const & script)
{
    snap::snap_expr::expr e;
    if(!e.compile(QString::fromUtf8(script.c_str())))
    {
        SNAP_LOG_WARNING
            << "Invalid script \""
            << script
            << "\". We were not able to compile it."
            << SNAP_LOG_SEND;
        return buffer_t();
    }
    QByteArray buf(e.serialize());

    // convert the QByteArray to a buffer_t
    //
    uint32_t size(buf.size());
    buffer_t result;
    result.reserve(size + 8);       // header is "SSX1" + uint32_t size in little endian
    char const * magic = "SSX1";
    result.insert(result.end(), magic, magic + 4);
    result.insert(result.end(), reinterpret_cast<uint8_t *>(&size), reinterpret_cast<uint8_t *>(&size + 1));
    result.insert(result.end(), reinterpret_cast<uint8_t *>(buf.data()), reinterpret_cast<uint8_t *>(buf.data()) + size);

    return result;
}


buffer_t execute_script(buffer_t compiled_script, row::pointer_t row)
{
    buffer_t result;

    int size(compiled_script.size());
    if(size < 8)
    {
        SNAP_LOG_WARNING
            << "A script buffer has to be at least 8 bytes."
            << SNAP_LOG_SEND;
        return result;
    }
    size -= 8;

    char const * input(reinterpret_cast<char const *>(compiled_script.data()));
    if(input[0] != 'S'
    || input[1] != 'S'
    || input[2] != 'X'
    || input[3] != '1')
    {
        SNAP_LOG_WARNING
            << "Script type ('"
            << input[0]
            << input[1]
            << input[2]
            << input[3]
            << "') not currently supported."
            << SNAP_LOG_SEND;
        return result;
    }
    input += 4;

    int const expected_size((input[0] <<  0)
                          | (input[1] <<  8)
                          | (input[2] << 16)
                          | (input[3] << 24));
    if(expected_size != size)
    {
        SNAP_LOG_WARNING
            << "Unexpected script size (got "
            << size
            << ", expected "
            << expected_size
            << "')."
            << SNAP_LOG_SEND;
        return result;
    }
    input += 4;

    try
    {
        QByteArray buf(input, size);

        snap::snap_expr::expr e;
        e.unserialize(buf);

        snap::snap_expr::variable_t return_value;
        snap::snap_expr::functions_t functions;

        // TODO: transform the entire row in script variables
        snap::snap_expr::variable_t::variable_map_t variables;
        cell::map_t cells(row->cells());
        for(auto const & c : cells)
        {
            schema_column::pointer_t schema(c.second->schema());
            snap::snap_expr::variable_t v(QString::fromUtf8(schema->name().c_str()));
            switch(schema->type())
            {
            case struct_type_t::STRUCT_TYPE_VOID:
                v.set_value();
                break;

            case struct_type_t::STRUCT_TYPE_BITS8:
            case struct_type_t::STRUCT_TYPE_UINT8:
                v.set_value(c.second->get_uint8());
                break;

            case struct_type_t::STRUCT_TYPE_INT8:
                v.set_value(c.second->get_int8());
                break;

            case struct_type_t::STRUCT_TYPE_BITS16:
            case struct_type_t::STRUCT_TYPE_UINT16:
                v.set_value(c.second->get_uint16());
                break;

            case struct_type_t::STRUCT_TYPE_INT16:
                v.set_value(c.second->get_int16());
                break;

            case struct_type_t::STRUCT_TYPE_BITS32:
            case struct_type_t::STRUCT_TYPE_UINT32:
            case struct_type_t::STRUCT_TYPE_VERSION:
                v.set_value(c.second->get_uint32());
                break;

            case struct_type_t::STRUCT_TYPE_INT32:
                v.set_value(c.second->get_int32());
                break;

            case struct_type_t::STRUCT_TYPE_BITS64:
            case struct_type_t::STRUCT_TYPE_UINT64:
            case struct_type_t::STRUCT_TYPE_REFERENCE:
            case struct_type_t::STRUCT_TYPE_OID:
            case struct_type_t::STRUCT_TYPE_TIME:
            case struct_type_t::STRUCT_TYPE_MSTIME:
            case struct_type_t::STRUCT_TYPE_USTIME:
                v.set_value(c.second->get_uint64());
                break;

            case struct_type_t::STRUCT_TYPE_INT64:
                v.set_value(c.second->get_int64());
                break;

            case struct_type_t::STRUCT_TYPE_BITS128:
            case struct_type_t::STRUCT_TYPE_UINT128:
                {
                    uint512_t value(c.second->get_uint128());
                    QByteArray data(reinterpret_cast<char const *>(value.f_value), sizeof(uint64_t) * 2);
                    v.set_value(data);
                }
                break;

            case struct_type_t::STRUCT_TYPE_INT128:
                {
                    uint512_t value(c.second->get_int128());
                    QByteArray data(reinterpret_cast<char const *>(value.f_value), sizeof(uint64_t) * 2);
                    v.set_value(data);
                }
                break;

            case struct_type_t::STRUCT_TYPE_BITS256:
            case struct_type_t::STRUCT_TYPE_UINT256:
                {
                    uint512_t value(c.second->get_uint256());
                    QByteArray data(reinterpret_cast<char const *>(value.f_value), sizeof(uint64_t) * 4);
                    v.set_value(data);
                }
                break;

            case struct_type_t::STRUCT_TYPE_INT256:
                {
                    uint512_t value(c.second->get_int256());
                    QByteArray data(reinterpret_cast<char const *>(value.f_value), sizeof(uint64_t) * 4);
                    v.set_value(data);
                }
                break;

            case struct_type_t::STRUCT_TYPE_BITS512:
            case struct_type_t::STRUCT_TYPE_UINT512:
                {
                    uint512_t value(c.second->get_uint512());
                    QByteArray data(reinterpret_cast<char const *>(value.f_value), sizeof(uint64_t) * 8);
                    v.set_value(data);
                }
                break;

            case struct_type_t::STRUCT_TYPE_INT512:
                {
                    uint512_t value(c.second->get_int512());
                    QByteArray data(reinterpret_cast<char const *>(value.f_value), sizeof(uint64_t) * 8);
                    v.set_value(data);
                }
                break;

            case struct_type_t::STRUCT_TYPE_FLOAT32:
                v.set_value(c.second->get_float32());
                break;

            case struct_type_t::STRUCT_TYPE_FLOAT64:
                v.set_value(c.second->get_float64());
                break;

            case struct_type_t::STRUCT_TYPE_FLOAT128:
                // TODO: we have to add support for long double in the
                //       expression, for now use a double
                //
                v.set_value(static_cast<double>(c.second->get_float128()));
                break;

            case struct_type_t::STRUCT_TYPE_P8STRING:
            case struct_type_t::STRUCT_TYPE_P16STRING:
            case struct_type_t::STRUCT_TYPE_P32STRING:
                v.set_value(QString::fromUtf8(c.second->get_string().c_str()));
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
            variables[QString::fromUtf8(schema->name().c_str())] = v;
        }

        e.execute(return_value, variables, functions);

        libdbproxy::value const & value(return_value.get_value());
        switch(return_value.get_type())
        {
        case snap::snap_expr::variable_t::variable_type_t::EXPR_VARIABLE_TYPE_NULL:
            // done
            break;

        case snap::snap_expr::variable_t::variable_type_t::EXPR_VARIABLE_TYPE_BOOL:
            {
                auto const v(value.safeBoolValue());
                result.insert(result.end(), reinterpret_cast<uint8_t const *>(&v), reinterpret_cast<uint8_t const *>(&v + 1));
            }
            break;

        case snap::snap_expr::variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT8:
            {
                auto const v(value.safeSignedCharValue());
                result.insert(result.end(), reinterpret_cast<uint8_t const *>(&v), reinterpret_cast<uint8_t const *>(&v + 1));
            }
            break;

        case snap::snap_expr::variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT8:
            {
                auto const v(value.safeUnsignedCharValue());
                result.insert(result.end(), reinterpret_cast<uint8_t const *>(&v), reinterpret_cast<uint8_t const *>(&v + 1));
            }
            break;

        case snap::snap_expr::variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT16:
            {
                auto const v(value.safeInt16Value());
                result.insert(result.end(), reinterpret_cast<uint8_t const *>(&v), reinterpret_cast<uint8_t const *>(&v + 1));
            }
            break;

        case snap::snap_expr::variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT16:
            {
                auto const v(value.safeUInt16Value());
                result.insert(result.end(), reinterpret_cast<uint8_t const *>(&v), reinterpret_cast<uint8_t const *>(&v + 1));
            }
            break;

        case snap::snap_expr::variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT32:
            {
                auto const v(value.safeInt32Value());
                result.insert(result.end(), reinterpret_cast<uint8_t const *>(&v), reinterpret_cast<uint8_t const *>(&v + 1));
            }
            break;

        case snap::snap_expr::variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT32:
            {
                auto const v(value.safeUInt32Value());
                result.insert(result.end(), reinterpret_cast<uint8_t const *>(&v), reinterpret_cast<uint8_t const *>(&v + 1));
            }
            break;

        case snap::snap_expr::variable_t::variable_type_t::EXPR_VARIABLE_TYPE_INT64:
            {
                auto const v(value.safeInt64Value());
                result.insert(result.end(), reinterpret_cast<uint8_t const *>(&v), reinterpret_cast<uint8_t const *>(&v + 1));
            }
            break;

        case snap::snap_expr::variable_t::variable_type_t::EXPR_VARIABLE_TYPE_UINT64:
            {
                auto const v(value.safeUInt64Value());
                result.insert(result.end(), reinterpret_cast<uint8_t const *>(&v), reinterpret_cast<uint8_t const *>(&v + 1));
            }
            break;

        case snap::snap_expr::variable_t::variable_type_t::EXPR_VARIABLE_TYPE_FLOAT:
            {
                auto const v(value.safeFloatValue());
                result.insert(result.end(), reinterpret_cast<uint8_t const *>(&v), reinterpret_cast<uint8_t const *>(&v + 1));
            }
            break;

        case snap::snap_expr::variable_t::variable_type_t::EXPR_VARIABLE_TYPE_DOUBLE:
            {
                auto const v(value.safeDoubleValue());
                result.insert(result.end(), reinterpret_cast<uint8_t const *>(&v), reinterpret_cast<uint8_t const *>(&v + 1));
            }
            break;

        case snap::snap_expr::variable_t::variable_type_t::EXPR_VARIABLE_TYPE_STRING:
            {
                auto const v(value.stringValue());
                QByteArray data(v.toUtf8());
                result.insert(result.end(), reinterpret_cast<uint8_t const *>(data.data()), reinterpret_cast<uint8_t const *>(data.data()) + data.size());
            }
            break;

        case snap::snap_expr::variable_t::variable_type_t::EXPR_VARIABLE_TYPE_BINARY:
            {
                auto const & v(value.binaryValue());
                result.insert(result.end(), reinterpret_cast<uint8_t const *>(v.data()), reinterpret_cast<uint8_t const *>(v.data()) + v.size());
            }
            break;

        }

    }
    catch(snap::snap_expr::snap_expr_exception const & e)
    {
        // ignore all execution exceptions, but log a warning at least
        //
        SNAP_LOG_WARNING
            << "An error occurred while exceuting a script: "
            << e.what()
            << SNAP_LOG_SEND;
        return buffer_t();
    }

    return result;
}



} // namespace snapdatabase
// vim: ts=4 sw=4 et
