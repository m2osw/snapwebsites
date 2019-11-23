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
#include    "snapdatabase/structure.h"

#include    "snapdatabase/convert.h"


// snaplogger lib
//
#include    <snaplogger/message.h>


// boost lib
//
#include    <boost/algorithm/string.hpp>


// snapdev lib
//
#include    <snapdev/not_used.h>


// C++ lib
//
#include    <iostream>


// last include
//
#include    <snapdev/poison.h>



namespace snapdatabase
{



namespace
{


struct name_to_struct_type_t
{
    char const *        f_name = nullptr;
    struct_type_t       f_type = struct_type_t::STRUCT_TYPE_END;
};

#define NAME_TO_STRUCT_TYPE(name)   { #name, struct_type_t::STRUCT_TYPE_##name }

name_to_struct_type_t g_name_to_struct_type[] =
{
    // WARNING: Keep in alphabetical order
    //
    NAME_TO_STRUCT_TYPE(ARRAY16),
    NAME_TO_STRUCT_TYPE(ARRAY32),
    NAME_TO_STRUCT_TYPE(ARRAY8),
    NAME_TO_STRUCT_TYPE(BITS128),
    NAME_TO_STRUCT_TYPE(BITS16),
    NAME_TO_STRUCT_TYPE(BITS256),
    NAME_TO_STRUCT_TYPE(BITS32),
    NAME_TO_STRUCT_TYPE(BITS512),
    NAME_TO_STRUCT_TYPE(BITS64),
    NAME_TO_STRUCT_TYPE(BITS8),
    NAME_TO_STRUCT_TYPE(BUFFER16),
    NAME_TO_STRUCT_TYPE(BUFFER32),
    NAME_TO_STRUCT_TYPE(BUFFER8),
    NAME_TO_STRUCT_TYPE(CSTRING),
    NAME_TO_STRUCT_TYPE(END),       // to end a list
    NAME_TO_STRUCT_TYPE(FLOAT32),
    NAME_TO_STRUCT_TYPE(FLOAT64),
    NAME_TO_STRUCT_TYPE(INT128),
    NAME_TO_STRUCT_TYPE(INT16),
    NAME_TO_STRUCT_TYPE(INT256),
    NAME_TO_STRUCT_TYPE(INT32),
    NAME_TO_STRUCT_TYPE(INT512),
    NAME_TO_STRUCT_TYPE(INT64),
    NAME_TO_STRUCT_TYPE(INT8),
    NAME_TO_STRUCT_TYPE(MSTIME),
    NAME_TO_STRUCT_TYPE(OID),
    NAME_TO_STRUCT_TYPE(P16STRING),
    NAME_TO_STRUCT_TYPE(P32STRING),
    NAME_TO_STRUCT_TYPE(P8STRING),
    NAME_TO_STRUCT_TYPE(REFERENCE),
    NAME_TO_STRUCT_TYPE(RENAMED),
    NAME_TO_STRUCT_TYPE(STRUCTURE),
    NAME_TO_STRUCT_TYPE(TIME),
    NAME_TO_STRUCT_TYPE(UINT128),
    NAME_TO_STRUCT_TYPE(UINT16),
    NAME_TO_STRUCT_TYPE(UINT256),
    NAME_TO_STRUCT_TYPE(UINT32),
    NAME_TO_STRUCT_TYPE(UINT512),
    NAME_TO_STRUCT_TYPE(UINT64),
    NAME_TO_STRUCT_TYPE(UINT8),
    NAME_TO_STRUCT_TYPE(USTIME),
    NAME_TO_STRUCT_TYPE(VERSION),
    NAME_TO_STRUCT_TYPE(VOID)
};








#pragma GCC diagnostic push
//#pragma GCC diagnostic ignored "-Wpedantic"
constexpr ssize_t const g_struct_type_sizes[] =
{
    [static_cast<int>(struct_type_t::STRUCT_TYPE_END)]          = INVALID_SIZE,
    [static_cast<int>(struct_type_t::STRUCT_TYPE_VOID)]         = 0,
    [static_cast<int>(struct_type_t::STRUCT_TYPE_BITS8)]        = sizeof(uint8_t),
    [static_cast<int>(struct_type_t::STRUCT_TYPE_BITS16)]       = sizeof(uint16_t),
    [static_cast<int>(struct_type_t::STRUCT_TYPE_BITS32)]       = sizeof(uint32_t),
    [static_cast<int>(struct_type_t::STRUCT_TYPE_BITS64)]       = sizeof(uint64_t),
    [static_cast<int>(struct_type_t::STRUCT_TYPE_BITS128)]      = sizeof(uint64_t) * 2,
    [static_cast<int>(struct_type_t::STRUCT_TYPE_BITS256)]      = sizeof(uint64_t) * 4,
    [static_cast<int>(struct_type_t::STRUCT_TYPE_BITS512)]      = sizeof(uint64_t) * 8,
    [static_cast<int>(struct_type_t::STRUCT_TYPE_INT8)]         = sizeof(int8_t),
    [static_cast<int>(struct_type_t::STRUCT_TYPE_UINT8)]        = sizeof(uint8_t),
    [static_cast<int>(struct_type_t::STRUCT_TYPE_INT16)]        = sizeof(int16_t),
    [static_cast<int>(struct_type_t::STRUCT_TYPE_UINT16)]       = sizeof(uint16_t),
    [static_cast<int>(struct_type_t::STRUCT_TYPE_INT32)]        = sizeof(int32_t),
    [static_cast<int>(struct_type_t::STRUCT_TYPE_UINT32)]       = sizeof(uint32_t),
    [static_cast<int>(struct_type_t::STRUCT_TYPE_INT64)]        = sizeof(int64_t),
    [static_cast<int>(struct_type_t::STRUCT_TYPE_UINT64)]       = sizeof(uint64_t),
    [static_cast<int>(struct_type_t::STRUCT_TYPE_INT128)]       = sizeof(int64_t) * 2,
    [static_cast<int>(struct_type_t::STRUCT_TYPE_UINT128)]      = sizeof(uint64_t) * 2,
    [static_cast<int>(struct_type_t::STRUCT_TYPE_INT256)]       = sizeof(int64_t) * 4,
    [static_cast<int>(struct_type_t::STRUCT_TYPE_UINT256)]      = sizeof(uint64_t) * 4,
    [static_cast<int>(struct_type_t::STRUCT_TYPE_INT512)]       = sizeof(int64_t) * 8,
    [static_cast<int>(struct_type_t::STRUCT_TYPE_UINT512)]      = sizeof(uint64_t) * 8,
    [static_cast<int>(struct_type_t::STRUCT_TYPE_FLOAT32)]      = sizeof(float),
    [static_cast<int>(struct_type_t::STRUCT_TYPE_FLOAT64)]      = sizeof(double),
    [static_cast<int>(struct_type_t::STRUCT_TYPE_FLOAT128)]     = sizeof(long double),
    [static_cast<int>(struct_type_t::STRUCT_TYPE_VERSION)]      = sizeof(uint32_t),
    [static_cast<int>(struct_type_t::STRUCT_TYPE_TIME)]         = sizeof(time_t),
    [static_cast<int>(struct_type_t::STRUCT_TYPE_MSTIME)]       = sizeof(uint64_t),
    [static_cast<int>(struct_type_t::STRUCT_TYPE_USTIME)]       = sizeof(uint64_t),
    [static_cast<int>(struct_type_t::STRUCT_TYPE_CSTRING)]      = VARIABLE_SIZE,
    [static_cast<int>(struct_type_t::STRUCT_TYPE_P8STRING)]     = VARIABLE_SIZE,
    [static_cast<int>(struct_type_t::STRUCT_TYPE_P16STRING)]    = VARIABLE_SIZE,
    [static_cast<int>(struct_type_t::STRUCT_TYPE_P32STRING)]    = VARIABLE_SIZE,
    [static_cast<int>(struct_type_t::STRUCT_TYPE_STRUCTURE)]    = VARIABLE_SIZE,
    [static_cast<int>(struct_type_t::STRUCT_TYPE_ARRAY8)]       = VARIABLE_SIZE,
    [static_cast<int>(struct_type_t::STRUCT_TYPE_ARRAY16)]      = VARIABLE_SIZE,
    [static_cast<int>(struct_type_t::STRUCT_TYPE_ARRAY32)]      = VARIABLE_SIZE,
    [static_cast<int>(struct_type_t::STRUCT_TYPE_BUFFER8)]      = VARIABLE_SIZE,
    [static_cast<int>(struct_type_t::STRUCT_TYPE_BUFFER16)]     = VARIABLE_SIZE,
    [static_cast<int>(struct_type_t::STRUCT_TYPE_BUFFER32)]     = VARIABLE_SIZE,
    [static_cast<int>(struct_type_t::STRUCT_TYPE_REFERENCE)]    = sizeof(uint64_t),
    [static_cast<int>(struct_type_t::STRUCT_TYPE_OID)]          = sizeof(uint64_t),
    [static_cast<int>(struct_type_t::STRUCT_TYPE_RENAMED)]      = INVALID_SIZE
};
#pragma GCC diagnostic pop


void verify_size(struct_type_t type, size_t size)
{
    if(static_cast<size_t>(type) >= sizeof(g_struct_type_sizes) / sizeof(g_struct_type_sizes[0]))
    {
        throw snapdatabase_out_of_range(
                  "type out of range for converting it to a size ("
                + std::to_string(static_cast<int>(type))
                + ", max: "
                + std::to_string(sizeof(g_struct_type_sizes) / sizeof(g_struct_type_sizes[0]))
                + ").");
    }

    if(g_struct_type_sizes[static_cast<int>(type)] != static_cast<ssize_t>(size))
    {
        throw snapdatabase_out_of_range(
                  "value and type sizes do not correspond ("
                + std::to_string(size)
                + ", max: "
                + std::to_string(g_struct_type_sizes[static_cast<int>(type)])
                + ").");
    }
}




}
// no name namespace






struct_type_t name_to_struct_type(std::string const & type_name)
{
#ifdef _DEBUG
    // verify in debug because if not in order we can't do a binary search
    for(size_t idx(1);
        idx < sizeof(g_name_to_struct_type) / sizeof(g_name_to_struct_type[0]);
        ++idx)
    {
        if(strcmp(g_name_to_struct_type[idx - 1].f_name
                , g_name_to_struct_type[idx].f_name) >= 0)
        {
            throw snapdatabase_logic_error(
                      "names in g_name_to_struct_type area not in alphabetical order: "
                    + std::string(g_name_to_struct_type[idx - 1].f_name)
                    + " >= "
                    + g_name_to_struct_type[idx].f_name
                    + " (position: "
                    + std::to_string(idx)
                    + ").");
        }
    }
#endif

    std::string const uc(boost::algorithm::to_upper_copy(type_name));

    int j(sizeof(g_name_to_struct_type) / sizeof(g_name_to_struct_type[0]));
    int i(0);
    while(i < j)
    {
        int const p((j - i) / 2 + i);
        int const r(uc.compare(g_name_to_struct_type[p].f_name));
        if(r > 0)
        {
            i = p + 1;
        }
        else if(r < 0)
        {
            j = p;
        }
        else
        {
            return g_name_to_struct_type[p].f_type;
        }
    }

    return INVALID_STRUCT_TYPE;
}



flag_definition::flag_definition()
{
}


flag_definition::flag_definition(
          std::string const & field_name
        , std::string const & flag_name
        , size_t pos
        , size_t size)
    : f_field_name(field_name)
    , f_flag_name(flag_name)
    , f_pos(pos)
    , f_size(size)
    , f_mask(((1 << size) - 1) << pos)  // fails if size == 64...
{
    if(size == 0)
    {
        throw invalid_parameter(
                  "Bit field named \""
                + field_name
                + "."
                + flag_name
                + "\" can't have a size of 0.");
    }
    if(size >= 64)
    {
        throw invalid_parameter(
                  "Bit field named \""
                + field_name
                + "."
                + flag_name
                + "\" is too large ("
                + std::to_string(size)
                + " >= 64).");
    }
    if(pos + size > 64)
    {
        throw invalid_parameter(
                  "The mask of the bit field named \""
                + field_name
                + "."
                + flag_name
                + "\" does not fit in a uint64_t.");
    }
}


std::string flag_definition::full_name() const
{
    return f_field_name + "." + f_flag_name;
}


std::string flag_definition::field_name() const
{
    return f_field_name;
}


std::string flag_definition::flag_name() const
{
    return f_flag_name;
}


size_t flag_definition::pos() const
{
    return f_pos;
}


size_t flag_definition::size() const
{
    return f_size;
}


flags_t flag_definition::mask() const
{
    return f_mask;
}





uint32_t field_t::size() const
{
    return f_size;
}


structure::pointer_t field_t::operator [] (int idx)
{
    if(static_cast<uint32_t>(idx) >= f_sub_structures.size())
    {
        throw out_of_bounds(
              "index ("
            + std::to_string(idx)
            + ") is out of bounds (0.."
            + std::to_string(f_size - 1)
            + ")");
    }
    return f_sub_structures[idx];
}





structure::structure(struct_description_t const * descriptions)
    : f_descriptions(descriptions)
{
}


void structure::set_block(block::pointer_t b, std::uint64_t size)
{
    f_buffer = std::make_shared<virtual_buffer>(b, 0, size);
}


void structure::set_virtual_buffer(virtual_buffer::pointer_t buffer, uint64_t start_offset)
{
    f_buffer = buffer;
    f_start_offset = start_offset;
}


virtual_buffer::pointer_t structure::get_virtual_buffer(uint64_t & start_offset) const
{
    start_offset = f_start_offset;
    return f_buffer;
}




/** \brief Get the static size or get 0.
 *
 * This function returns the size of the structure if the size is static.
 *
 * Most structures are no static, though, they will have variable fields
 * such as a string or a buffer. This function returns 0 for those
 * structures. You can still get a size using the get_current_size()
 * function, just keep in mind that the size may change as the data
 * varies in the structure.
 *
 * \note
 * A sub-structure is considered static as long as all of its fields are
 * static fields.
 *
 * \return The size of the structure or 0 if the structure size is variable.
 */
size_t structure::get_size() const
{
    size_t result(0);

    for(auto f : f_fields_by_name)
    {
        if((f.second->f_flags & field_t::FIELD_FLAG_VARIABLE_SIZE) != 0)
        {
            return 0;
        }

        if(f.second->f_description->f_type == struct_type_t::STRUCT_TYPE_RENAMED)
        {
            continue;
        }

        // the size of the structure field is ignored, it's always 1
        // and it has nothing to do with the sze of the resulting
        // binary
        //
        if(f.second->f_description->f_type != struct_type_t::STRUCT_TYPE_STRUCTURE)
        {
            result += f.second->size();
        }

        for(auto s : f.second->f_sub_structures)
        {
            result += s->get_size();
        }
    }

    return result;
}


size_t structure::get_current_size() const
{
    size_t result(0);

    for(auto f : f_fields_by_name)
    {
        if(f.second->f_description->f_type == struct_type_t::STRUCT_TYPE_RENAMED)
        {
            continue;
        }

        // the size of the structure field is ignored, it's always 1
        // and it has nothing to do with the sze of the resulting
        // binary
        //
        switch(f.second->f_description->f_type)
        {
        case struct_type_t::STRUCT_TYPE_STRUCTURE:
            break;

        case struct_type_t::STRUCT_TYPE_CSTRING:
        case struct_type_t::STRUCT_TYPE_P8STRING:
        case struct_type_t::STRUCT_TYPE_ARRAY8:
        case struct_type_t::STRUCT_TYPE_BUFFER8:
            result += 1 + f.second->f_size;
            break;

        case struct_type_t::STRUCT_TYPE_P16STRING:
        case struct_type_t::STRUCT_TYPE_ARRAY16:
        case struct_type_t::STRUCT_TYPE_BUFFER16:
            result += 2 + f.second->f_size;
            break;

        case struct_type_t::STRUCT_TYPE_P32STRING:
        case struct_type_t::STRUCT_TYPE_ARRAY32:
        case struct_type_t::STRUCT_TYPE_BUFFER32:
            result += 4 + f.second->f_size;
            break;

        default:
            result += f.second->f_size;
            break;

        }

        for(auto s : f.second->f_sub_structures)
        {
            result += s->get_size();
        }
    }

    return result;
}


field_t::pointer_t structure::get_field(std::string const & field_name, struct_type_t type) const
{
    // make sure we've parsed the descriptions
    //
    parse();

    auto field(f_fields_by_name.find(field_name));
    if(field == f_fields_by_name.end())
    {
        // bit fields have sub-names we can check for `field_name`
        //
        for(auto f : f_fields_by_name)
        {
            switch(f.second->f_description->f_type)
            {
            case struct_type_t::STRUCT_TYPE_BITS8:
            case struct_type_t::STRUCT_TYPE_BITS16:
            case struct_type_t::STRUCT_TYPE_BITS32:
            case struct_type_t::STRUCT_TYPE_BITS64:
            case struct_type_t::STRUCT_TYPE_BITS128:
            case struct_type_t::STRUCT_TYPE_BITS256:
            case struct_type_t::STRUCT_TYPE_BITS512:
                {
                    auto flag(f.second->f_flag_definitions.find(field_name));
                    if(flag != f.second->f_flag_definitions.end())
                    {
                        // found it!
                        //
                        return f.second;
                    }
                }
                break;

            default:
                // miss
                break;

            }
        }

        // we can't return a field and yet it is mandatory, throw an error
        // (if we change a description to still include old fields, we need
        // to have a way to point to the new field--see the RENAMED flag).
        //
        throw field_not_found(
                  "This description does not include field named \""
                + field_name
                + "\".");
    }

    field_t::pointer_t f(field->second);
    if(f->f_description->f_type == struct_type_t::STRUCT_TYPE_RENAMED)
    {
        std::string const new_name(f->f_description->f_sub_description->f_field_name);
        field = f_fields_by_name.find(new_name);
        if(field == f_fields_by_name.end())
        {
            throw field_not_found(
                      "This description renames field \""
                    + field_name
                    + "\" to \""
                    + new_name
                    + "\" but we could not find the new name field.");
        }
        f = field->second;

        // let programmers know that the old name is deprecated
        SNAP_LOG_DEBUG
            << "Deprecated field name \""
            << field_name
            << "\" was changed to \""
            << new_name
            << "\". Please change your code to use the new name."
            << SNAP_LOG_SEND;
    }

    if(type != struct_type_t::STRUCT_TYPE_END
    && f->f_description->f_type != type)
    {
        throw type_mismatch(
                  "This field type is \""
                + std::to_string(static_cast<int>(f->f_description->f_type))    // TODO: convert to name
                + "\" but we expected \""
                + std::to_string(static_cast<int>(type))
                + "\".");
    }

std::cerr << "returning the field alright...\n";
    return f;
}


int64_t structure::get_integer(std::string const & field_name) const
{
    auto f(get_field(field_name));

    verify_size(f->f_description->f_type, f->f_size);

    switch(f->f_description->f_type)
    {
    case struct_type_t::STRUCT_TYPE_INT8:
        {
            int8_t value(0);
            f_buffer->pread(&value, sizeof(value), f->f_offset);
            return value;
        }

    case struct_type_t::STRUCT_TYPE_INT16:
        {
            int16_t value(0);
            f_buffer->pread(&value, sizeof(value), f->f_offset);
            return value;
        }

    case struct_type_t::STRUCT_TYPE_INT32:
        {
            int32_t value(0);
            f_buffer->pread(&value, sizeof(value), f->f_offset);
            return value;
        }

    case struct_type_t::STRUCT_TYPE_INT64:
        {
            int64_t value(0);
            f_buffer->pread(&value, sizeof(value), f->f_offset);
            return value;
        }

    default:
        throw type_mismatch(
                  "This description type is \""
                + std::to_string(static_cast<int>(f->f_description->f_type))
                + "\" but we expected one of \""
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_INT8))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_INT16))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_INT32))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_INT64))
                + "\".");

    }
}


void structure::set_integer(std::string const & field_name, int64_t value)
{
    auto f(get_field(field_name));

    verify_size(f->f_description->f_type, f->f_size);

    switch(f->f_description->f_type)
    {
    case struct_type_t::STRUCT_TYPE_INT8:
        {
            int8_t v(value);
            f_buffer->pwrite(&v, sizeof(v), f->f_offset);
        }
        return;

    case struct_type_t::STRUCT_TYPE_INT16:
        {
            int16_t v(value);
            f_buffer->pwrite(&v, sizeof(v), f->f_offset);
        }
        return;

    case struct_type_t::STRUCT_TYPE_INT32:
        {
            int32_t v(value);
            f_buffer->pwrite(&v, sizeof(v), f->f_offset);
        }
        return;

    case struct_type_t::STRUCT_TYPE_INT64:
        {
            int64_t v(value);
            f_buffer->pwrite(&v, sizeof(v), f->f_offset);
        }
        return;

    default:
        throw type_mismatch(
                  "This description type is \""
                + std::to_string(static_cast<int>(f->f_description->f_type))
                + "\" but we expected one of \""
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_INT8))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_INT16))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_INT32))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_INT64))
                + "\".");

    }
}


uint64_t structure::get_uinteger(std::string const & field_name) const
{
std::cerr << "get uinteger for [" << field_name << "]\n";
    auto f(get_field(field_name));

std::cerr << "  verify size [" << field_name << "]\n";
    verify_size(f->f_description->f_type, f->f_size);

std::cerr << "  check type [" << field_name << "]\n";
    switch(f->f_description->f_type)
    {
    case struct_type_t::STRUCT_TYPE_BITS8:
    case struct_type_t::STRUCT_TYPE_UINT8:
        {
            uint8_t value(0);
            f_buffer->pread(&value, sizeof(value), f->f_offset);
            return value;
        }

    case struct_type_t::STRUCT_TYPE_BITS16:
    case struct_type_t::STRUCT_TYPE_UINT16:
        {
            uint16_t value(0);
            f_buffer->pread(&value, sizeof(value), f->f_offset);
            return value;
        }

    case struct_type_t::STRUCT_TYPE_BITS32:
    case struct_type_t::STRUCT_TYPE_UINT32:
    case struct_type_t::STRUCT_TYPE_VERSION:
        {
            uint32_t value(0);
std::cerr << "  pread UINT32 [" << field_name << "]\n";
            f_buffer->pread(&value, sizeof(value), f->f_offset);
std::cerr << "  read [" << value << "]\n";
            return value;
        }

    case struct_type_t::STRUCT_TYPE_BITS64:
    case struct_type_t::STRUCT_TYPE_UINT64:
    case struct_type_t::STRUCT_TYPE_REFERENCE:
    case struct_type_t::STRUCT_TYPE_OID:
    case struct_type_t::STRUCT_TYPE_TIME:
    case struct_type_t::STRUCT_TYPE_MSTIME:
    case struct_type_t::STRUCT_TYPE_USTIME:
        {
            uint64_t value(0);
            f_buffer->pread(&value, sizeof(value), f->f_offset);
            return value;
        }

    default:
        throw type_mismatch(
                  "This description type is \""
                + std::to_string(static_cast<int>(f->f_description->f_type))
                + "\" but we expected one of \""
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_BITS8))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_BITS16))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_BITS32))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_BITS64))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_UINT8))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_UINT16))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_UINT32))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_VERSION))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_UINT64))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_REFERENCE))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_OID))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_TIME))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_MSTIME))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_USTIME))
                + "\".");

    }
}


void structure::set_uinteger(std::string const & field_name, uint64_t value)
{
std::cerr << "set uinteger for [" << field_name << "] -> " << value << "\n";
    auto f(get_field(field_name));

std::cerr << "  verify size [" << field_name << "]\n";
    verify_size(f->f_description->f_type, f->f_size);

std::cerr << "  check type [" << field_name << "]\n";
    switch(f->f_description->f_type)
    {
    case struct_type_t::STRUCT_TYPE_BITS8:
    case struct_type_t::STRUCT_TYPE_UINT8:
        {
            uint8_t v(value);
            f_buffer->pwrite(&v, sizeof(v), f->f_offset);
        }
        return;

    case struct_type_t::STRUCT_TYPE_BITS16:
    case struct_type_t::STRUCT_TYPE_UINT16:
        {
            uint16_t v(value);
            f_buffer->pwrite(&v, sizeof(v), f->f_offset);
        }
        return;

    case struct_type_t::STRUCT_TYPE_BITS32:
    case struct_type_t::STRUCT_TYPE_UINT32:
    case struct_type_t::STRUCT_TYPE_VERSION:
        {
            uint32_t v(value);
            f_buffer->pwrite(&v, sizeof(v), f->f_offset);
        }
        return;

    case struct_type_t::STRUCT_TYPE_BITS64:
    case struct_type_t::STRUCT_TYPE_UINT64:
    case struct_type_t::STRUCT_TYPE_REFERENCE:
    case struct_type_t::STRUCT_TYPE_OID:
    case struct_type_t::STRUCT_TYPE_TIME:
    case struct_type_t::STRUCT_TYPE_MSTIME:
    case struct_type_t::STRUCT_TYPE_USTIME:
        {
            uint64_t v(value);
std::cerr << "  pread UINT64 [" << field_name << "]\n";
            f_buffer->pwrite(&v, sizeof(v), f->f_offset);
        }
        return;

    default:
        throw type_mismatch(
                  "This description type is \""
                + std::to_string(static_cast<int>(f->f_description->f_type))
                + "\" but we expected one of \""
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_BITS8))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_BITS16))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_BITS32))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_BITS64))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_UINT8))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_UINT16))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_UINT32))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_VERSION))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_UINT64))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_REFERENCE))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_OID))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_TIME))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_MSTIME))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_USTIME))
                + "\".");

    }
}


uint64_t structure::get_bits(std::string const & flag_name) const
{
    field_t::pointer_t f;
    flag_definition * flag(nullptr);
    for(auto field : f_fields_by_name)
    {
        auto it(field.second->f_flag_definitions.find(flag_name));
        if(it == field.second->f_flag_definitions.end())
        {
            flag = &it->second;
            f = field.second;
        }
    }
    if(flag == nullptr)
    {
        throw type_mismatch(
                  "get_bits() called with flag name \""
                + flag_name
                + "\" which has no flag definitions...");
    }

    verify_size(f->f_description->f_type, f->f_size);

    switch(f->f_description->f_type)
    {
    case struct_type_t::STRUCT_TYPE_BITS8:
        {
            uint8_t value(0);
            f_buffer->pread(&value, sizeof(value), f->f_offset);
            return (value & flag->mask()) >> flag->pos();
        }

    case struct_type_t::STRUCT_TYPE_BITS16:
        {
            uint16_t value(0);
            f_buffer->pread(&value, sizeof(value), f->f_offset);
            return (value & flag->mask()) >> flag->pos();
        }

    case struct_type_t::STRUCT_TYPE_BITS32:
        {
            uint32_t value(0);
            f_buffer->pread(&value, sizeof(value), f->f_offset);
            return (value & flag->mask()) >> flag->pos();
        }

    case struct_type_t::STRUCT_TYPE_BITS64:
        {
            uint64_t value(0);
            f_buffer->pread(&value, sizeof(value), f->f_offset);
            return (value & flag->mask()) >> flag->pos();
        }

    default:
        throw type_mismatch(
                  "This description type is \""
                + std::to_string(static_cast<int>(f->f_description->f_type))
                + "\" but we expected one of \""
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_BITS8))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_BITS16))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_BITS32))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_BITS64))
                + "\".");

    }
}


void structure::set_bits(std::string const & flag_name, uint64_t value)
{
    field_t::pointer_t f;
    flag_definition * flag(nullptr);
    for(auto field : f_fields_by_name)
    {
        auto it(field.second->f_flag_definitions.find(flag_name));
        if(it == field.second->f_flag_definitions.end())
        {
            flag = &it->second;
            f = field.second;
        }
    }
    if(flag == nullptr)
    {
        throw type_mismatch(
                  "get_bits() called with flag name \""
                + flag_name
                + "\" which has no flag definitions...");
    }

    verify_size(f->f_description->f_type, f->f_size);

    switch(f->f_description->f_type)
    {
    case struct_type_t::STRUCT_TYPE_BITS8:
    case struct_type_t::STRUCT_TYPE_BITS16:
    case struct_type_t::STRUCT_TYPE_BITS32:
    case struct_type_t::STRUCT_TYPE_BITS64:
        break;

    default:
        throw type_mismatch(
                  "This description type is \""
                + std::to_string(static_cast<int>(f->f_description->f_type))
                + "\" but we expected one of \""
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_BITS8))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_BITS16))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_BITS32))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_BITS64))
                + "\".");

    }

    if((value & (flag->mask() >> flag->pos())) != value)
    {
        throw invalid_number(
                  "Value \""
                + std::to_string(value)
                + "\" does not fit in flag field \""
                + flag->full_name()
                + "\".");
    }

    // some day we may want to optimize better, but this is the easiest
    // right now
    //
    uint64_t v(get_uinteger(f->f_description->f_field_name));
    v &= ~flag->mask();
    v |= value << flag->pos();
    set_uinteger(f->f_description->f_field_name, v);
}


int512_t structure::get_large_integer(std::string const & field_name) const
{
    auto f(get_field(field_name));

    verify_size(f->f_description->f_type, f->f_size);

    int512_t result;
    switch(f->f_description->f_type)
    {
    case struct_type_t::STRUCT_TYPE_INT8:
        f_buffer->pread(&result.f_value, sizeof(int8_t), f->f_offset);
        result.f_value[0] = static_cast<int8_t>(result.f_value[0]); // sign extend
sign_extend_64bit:
        result.f_value[1] = static_cast<int64_t>(result.f_value[0]) < 0
                                    ? -1
                                    : 0;
        result.f_value[2] = result.f_value[1];
        result.f_value[3] = result.f_value[1];
        result.f_value[4] = result.f_value[1];
        result.f_value[5] = result.f_value[1];
        result.f_value[6] = result.f_value[1];
        result.f_high_value = result.f_value[1];
        return result;

    case struct_type_t::STRUCT_TYPE_INT16:
        f_buffer->pread(&result.f_value, sizeof(int16_t), f->f_offset);
        result.f_value[0] = static_cast<int16_t>(result.f_value[0]); // sign extend
        goto sign_extend_64bit;

    case struct_type_t::STRUCT_TYPE_INT32:
        f_buffer->pread(&result.f_value, sizeof(int32_t), f->f_offset);
        result.f_value[0] = static_cast<int32_t>(result.f_value[0]); // sign extend
        goto sign_extend_64bit;

    case struct_type_t::STRUCT_TYPE_INT64:
        f_buffer->pread(&result.f_value, sizeof(int64_t), f->f_offset);
        result.f_value[0] = static_cast<int64_t>(result.f_value[0]); // sign extend
        goto sign_extend_64bit;

    case struct_type_t::STRUCT_TYPE_INT128:
        f_buffer->pread(&result.f_value, sizeof(int64_t) * 2, f->f_offset);

        result.f_value[2] = static_cast<int64_t>(result.f_value[1]) < 0
                                    ? -1
                                    : 0;
        result.f_value[3] = result.f_value[2];
        result.f_value[4] = result.f_value[2];
        result.f_value[5] = result.f_value[2];
        result.f_value[6] = result.f_value[2];
        result.f_high_value = result.f_value[2];

        return result;

    case struct_type_t::STRUCT_TYPE_INT256:
        f_buffer->pread(&result.f_value, sizeof(int64_t) * 4, f->f_offset);

        result.f_value[4] = static_cast<int64_t>(result.f_value[3]) < 0
                                    ? -1
                                    : 0;
        result.f_value[5] = result.f_value[4];
        result.f_value[6] = result.f_value[4];
        result.f_high_value = result.f_value[4];

        return result;

    case struct_type_t::STRUCT_TYPE_INT512:
        f_buffer->pread(&result.f_value, sizeof(int64_t) * 8, f->f_offset);

        return result;

    default:
        throw type_mismatch(
                  "This description type is \""
                + std::to_string(static_cast<int>(f->f_description->f_type))
                + "\" but we expected one of \""
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_INT8))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_INT16))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_INT32))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_INT64))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_INT128))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_INT256))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_INT512))
                + "\".");

    }
}


void structure::set_large_integer(std::string const & field_name, int512_t value)
{
    auto f(get_field(field_name));

    verify_size(f->f_description->f_type, f->f_size);

    switch(f->f_description->f_type)
    {
    case struct_type_t::STRUCT_TYPE_INT8:
    case struct_type_t::STRUCT_TYPE_INT16:
    case struct_type_t::STRUCT_TYPE_INT32:
    case struct_type_t::STRUCT_TYPE_INT64:
    case struct_type_t::STRUCT_TYPE_INT128:
    case struct_type_t::STRUCT_TYPE_INT256:
    case struct_type_t::STRUCT_TYPE_INT512:
        f_buffer->pwrite(value.f_value, f->f_size, f->f_offset);
        return;

    default:
        throw type_mismatch(
                  "This description type is \""
                + std::to_string(static_cast<int>(f->f_description->f_type))
                + "\" but we expected one of \""
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_INT8))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_INT16))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_INT32))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_INT64))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_INT128))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_INT256))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_INT512))
                + "\".");

    }
}


uint512_t structure::get_large_uinteger(std::string const & field_name) const
{
    auto f(get_field(field_name));

    verify_size(f->f_description->f_type, f->f_size);

    uint512_t result;
    switch(f->f_description->f_type)
    {
    case struct_type_t::STRUCT_TYPE_BITS8:
    case struct_type_t::STRUCT_TYPE_UINT8:
    case struct_type_t::STRUCT_TYPE_BITS16:
    case struct_type_t::STRUCT_TYPE_UINT16:
    case struct_type_t::STRUCT_TYPE_BITS32:
    case struct_type_t::STRUCT_TYPE_UINT32:
    case struct_type_t::STRUCT_TYPE_BITS64:
    case struct_type_t::STRUCT_TYPE_UINT64:
    case struct_type_t::STRUCT_TYPE_REFERENCE:
    case struct_type_t::STRUCT_TYPE_OID:
    case struct_type_t::STRUCT_TYPE_TIME:
    case struct_type_t::STRUCT_TYPE_MSTIME:
    case struct_type_t::STRUCT_TYPE_USTIME:
    case struct_type_t::STRUCT_TYPE_UINT128:
    case struct_type_t::STRUCT_TYPE_UINT256:
    case struct_type_t::STRUCT_TYPE_UINT512:
        f_buffer->pread(&result.f_value, f->f_size, f->f_offset);
        break;

    default:
        throw type_mismatch(
                  "This description type is \""
                + std::to_string(static_cast<int>(f->f_description->f_type))
                + "\" but we expected one of \""
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_BITS8))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_BITS16))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_BITS32))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_BITS64))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_UINT8))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_UINT16))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_UINT32))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_UINT64))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_UINT128))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_UINT256))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_UINT512))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_REFERENCE))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_OID))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_TIME))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_MSTIME))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_USTIME))
                + "\".");

    }

    return result;
}


void structure::set_large_uinteger(std::string const & field_name, uint512_t value)
{
    auto f(get_field(field_name));

    verify_size(f->f_description->f_type, f->f_size);

    switch(f->f_description->f_type)
    {
    case struct_type_t::STRUCT_TYPE_BITS8:
    case struct_type_t::STRUCT_TYPE_BITS16:
    case struct_type_t::STRUCT_TYPE_BITS32:
    case struct_type_t::STRUCT_TYPE_BITS64:
    case struct_type_t::STRUCT_TYPE_UINT8:
    case struct_type_t::STRUCT_TYPE_UINT16:
    case struct_type_t::STRUCT_TYPE_UINT32:
    case struct_type_t::STRUCT_TYPE_UINT64:
    case struct_type_t::STRUCT_TYPE_UINT128:
    case struct_type_t::STRUCT_TYPE_UINT256:
    case struct_type_t::STRUCT_TYPE_UINT512:
    case struct_type_t::STRUCT_TYPE_REFERENCE:
    case struct_type_t::STRUCT_TYPE_OID:
    case struct_type_t::STRUCT_TYPE_TIME:
    case struct_type_t::STRUCT_TYPE_MSTIME:
    case struct_type_t::STRUCT_TYPE_USTIME:
        f_buffer->pwrite(value.f_value, f->f_size, f->f_offset);
        break;

    default:
        throw type_mismatch(
                  "This description type is \""
                + std::to_string(static_cast<int>(f->f_description->f_type))
                + "\" but we expected one of \""
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_BITS8))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_BITS16))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_BITS32))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_BITS64))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_UINT8))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_UINT16))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_UINT32))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_UINT64))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_UINT128))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_UINT256))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_UINT512))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_REFERENCE))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_OID))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_TIME))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_MSTIME))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_USTIME))
                + "\".");

    }
}


float structure::get_float32(std::string const & field_name) const
{
    auto f(get_field(field_name, struct_type_t::STRUCT_TYPE_FLOAT32));

    verify_size(struct_type_t::STRUCT_TYPE_FLOAT32, f->f_size);

    float result;
    f_buffer->pread(&result, sizeof(float), f->f_offset);
    return result;
}


void structure::set_float32(std::string const & field_name, float value)
{
    auto f(get_field(field_name, struct_type_t::STRUCT_TYPE_FLOAT32));

    verify_size(struct_type_t::STRUCT_TYPE_FLOAT32, f->f_size);

    f_buffer->pwrite(&value, sizeof(float), f->f_offset);
}


double structure::get_float64(std::string const & field_name) const
{
    auto f(get_field(field_name, struct_type_t::STRUCT_TYPE_FLOAT64));

    verify_size(struct_type_t::STRUCT_TYPE_FLOAT64, f->f_size);

    double result;
    f_buffer->pread(&result, sizeof(double), f->f_offset);
    return result;
}


void structure::set_float64(std::string const & field_name, double value)
{
    auto f(get_field(field_name, struct_type_t::STRUCT_TYPE_FLOAT64));

    verify_size(struct_type_t::STRUCT_TYPE_FLOAT64, f->f_size);

    f_buffer->pwrite(&value, sizeof(double), f->f_offset);
}


long double structure::get_float128(std::string const & field_name) const
{
    auto f(get_field(field_name, struct_type_t::STRUCT_TYPE_FLOAT128));

    verify_size(struct_type_t::STRUCT_TYPE_FLOAT128, f->f_size);

    long double result;
    f_buffer->pread(&result, sizeof(long double), f->f_offset);
    return result;
}


void structure::set_float128(std::string const & field_name, long double value)
{
    auto f(get_field(field_name, struct_type_t::STRUCT_TYPE_FLOAT128));

    verify_size(struct_type_t::STRUCT_TYPE_FLOAT128, f->f_size);

    f_buffer->pwrite(&value, sizeof(long double), f->f_offset);
}


std::string structure::get_string(std::string const & field_name) const
{
    auto f(get_field(field_name));

    int skip(0);
    uint32_t size(f->f_size);
    switch(f->f_description->f_type)
    {
    case struct_type_t::STRUCT_TYPE_CSTRING:
        size -= 1;
        break;

    case struct_type_t::STRUCT_TYPE_P8STRING:
        skip = 1;
        size -= 1;
        break;

    case struct_type_t::STRUCT_TYPE_P16STRING:
        skip = 2;
        size -= 2;
        break;

    case struct_type_t::STRUCT_TYPE_P32STRING:
        skip = 4;
        size -= 4;
        break;

    default:
        throw string_not_terminated(
                  "This field was expected to be a string it is a \""
                + std::to_string(static_cast<int>(f->f_description->f_type))
                + "\" instead.");

    }

    std::string result(size, '\0');
    f_buffer->pread(result.data(), size, f->f_offset);
    return result;
}


void structure::set_string(std::string const & field_name, std::string const & value)
{
    auto f(get_field(field_name));

    int skip(0);
    uint32_t size(f->f_size);
    switch(f->f_description->f_type)
    {
    case struct_type_t::STRUCT_TYPE_CSTRING:
        size -= 1;
        break;

    case struct_type_t::STRUCT_TYPE_P8STRING:
        skip = 1;
        size -= 1;
        break;

    case struct_type_t::STRUCT_TYPE_P16STRING:
        skip = 2;
        size -= 2;
        break;

    case struct_type_t::STRUCT_TYPE_P32STRING:
        skip = 4;
        size -= 4;
        break;

    default:
        throw string_not_terminated(
                  "This field was expected to be a string it is a \""
                + std::to_string(static_cast<int>(f->f_description->f_type))
                + "\" instead.");

    }

    // verify the length (make it debug only?)
    //
    uint32_t length(0);
    f_buffer->pread(&length, skip, f->f_offset);
    if(length != size)
    {
        throw invalid_size(
                  "This string sizes do not match; found "
                + std::to_string(length)
                + ", expected "
                + std::to_string(size)
                + " instead.");
    }

    // TODO: this is not a pwrite()
    //      1. do an erase of the old string
    //      2. do an insert of the new string
    //      (or better, overwrite existing N characters and then either
    //      erase the extras or insert the new chars)
    //
    //      BUT ERASE and/or INSERT MEANS ALL FIELDS AFTER THAT GET THEIR
    //      OFFSET UPDATED!!!
    //
    f_buffer->pwrite(value.data(), size, f->f_offset + skip);

    if(f->f_description->f_type == struct_type_t::STRUCT_TYPE_CSTRING)
    {
        // `skip` not needed (we know it's 0 here)
        //
        uint8_t eos(0);
        f_buffer->pwrite(&eos, sizeof(uint8_t), f->f_offset + value.length());
    }
}


structure::pointer_t structure::get_structure(std::string const & field_name) const
{
    auto f(get_field(field_name, struct_type_t::STRUCT_TYPE_STRUCTURE));

    if(f->f_sub_structures.empty())
    {
        return structure::pointer_t();
    }

    return f->f_sub_structures[0];
}


void structure::set_structure(std::string const & field_name, structure::pointer_t & value)
{
    auto f(get_field(field_name, struct_type_t::STRUCT_TYPE_STRUCTURE));

    if(f->f_sub_structures.empty())
    {
        f->f_sub_structures.push_back(value);
    }
    else
    {
        f->f_sub_structures[0] = value;
    }
}


structure::vector_t structure::get_array(std::string const & field_name) const
{
    auto f(get_field(field_name));

    switch(f->f_description->f_type)
    {
    case struct_type_t::STRUCT_TYPE_ARRAY8:
    case struct_type_t::STRUCT_TYPE_ARRAY16:
    case struct_type_t::STRUCT_TYPE_ARRAY32:
        break;

    default:
        throw type_mismatch(
                  "The get_array() function expected a STRUCT_TYPE_ARRAY<size> field instead of \""
                + std::to_string(static_cast<int>(f->f_description->f_type))
                + "\".");

    }

    return f->f_sub_structures;
}


void structure::set_array(std::string const & field_name, structure::vector_t const & value)
{
    auto f(get_field(field_name));

    switch(f->f_description->f_type)
    {
    case struct_type_t::STRUCT_TYPE_ARRAY8:
    case struct_type_t::STRUCT_TYPE_ARRAY16:
    case struct_type_t::STRUCT_TYPE_ARRAY32:
        break;

    default:
        throw type_mismatch(
                  "The set_array() function expected a STRUCT_TYPE_ARRAY<size> field instead of \""
                + std::to_string(static_cast<int>(f->f_description->f_type))
                + "\".");

    }

    f->f_sub_structures = value;
}


buffer_t structure::get_buffer(std::string const & field_name) const
{
    auto f(get_field(field_name));

    switch(f->f_description->f_type)
    {
    case struct_type_t::STRUCT_TYPE_BUFFER8:
    case struct_type_t::STRUCT_TYPE_BUFFER16:
    case struct_type_t::STRUCT_TYPE_BUFFER32:
        break;

    default:
        throw type_mismatch(
                  "The get_buffer() function expected a STRUCT_TYPE_BUFFER<size> field instead of \""
                + std::to_string(static_cast<int>(f->f_description->f_type))
                + "\".");

    }

    buffer_t result;
    result.resize(f->f_size);
    f_buffer->pread(result.data(), f->f_size, f->f_offset);
    return result;
}


void structure::set_buffer(std::string const & field_name, buffer_t const & value)
{
    auto f(get_field(field_name));

    uint32_t max(0);
    uint32_t size_bytes(0);
    uint32_t size(value.size());
    switch(f->f_description->f_type)
    {
    case struct_type_t::STRUCT_TYPE_BUFFER8:
        max = 0xFF;
        size_bytes = 1;
        break;

    case struct_type_t::STRUCT_TYPE_BUFFER16:
        max = 0xFFFF;
        size_bytes = 2;
        break;

    case struct_type_t::STRUCT_TYPE_BUFFER32:
        max = 0xFFFFFFFF;
        size_bytes = 4;
        break;

    default:
        throw type_mismatch(
                  "The set_buffer() function expected a STRUCT_TYPE_BUFFER<size> field instead of \""
                + std::to_string(static_cast<int>(f->f_description->f_type))
                + "\".");

    }

    if(size > max)
    {
        throw snapdatabase_out_of_range(
                  "Size of input buffer ("
                + std::to_string(static_cast<int>(value.size()))
                + " too large to send it to the buffer; the maximum permitted by this field is "
                + std::to_string(max)
                + ".");
    }

    if(f->f_size > size)
    {
        // existing buffer too large, make it the right size (smaller)
        //
        f_buffer->perase(f->f_size - value.size(), f->f_offset + size_bytes + size);

        f_buffer->pwrite(&size, size_bytes, f->f_offset);
        f_buffer->pwrite(value.data(), size, f->f_offset + size_bytes);
        f->f_size = size_bytes + size;
    }
    else if(f->f_size < value.size())
    {
        // existing buffer too small, enlarge it
        //
        //     |*                   |
        //     | <------>           |
        //     |         <--------->|
        //     ^^   ^        ^
        //     ||   |        |
        //     ||   |        +----- new space (pinsert)
        //     ||   |
        //     ||   +---- existing space (pwrite)
        //     ||
        //     |+------ buffer size
        //     |
        //     +----- f->f_offset
        //
        // Size of each element is:
        //
        //     buffer size -- size_bytes
        //     existing space -- f->f_size - size_bytes
        //     new space -- value.size() - 'existing space'
        //

        f_buffer->pwrite(&size, size_bytes, f->f_offset);

        f_buffer->pwrite(value.data(), f->f_size - size_bytes, f->f_offset + size_bytes);

        uint32_t const insert_offset(f->f_size - size_bytes);
        f_buffer->pinsert(value.data() + insert_offset, size - (f->f_size + size_bytes), f->f_offset + f->f_size);

        f->f_size = size_bytes + size;
    }
    else
    {
        // same size, just overwrite
        //
        f_buffer->pwrite(value.data(), size, f->f_offset + size_bytes);
    }
}


void structure::parse() const
{
    if(!f_fields_by_name.empty())
    {
        // already parsed
        //
        return;
    }

    snap::NOTUSED(parse_descriptions(f_start_offset));
}


uint64_t structure::parse_descriptions(uint64_t offset) const
{
std::cerr << "parse structure vs buffer with offset: " << offset << "\n";
    for(struct_description_t const * def(f_descriptions);
        def->f_type != struct_type_t::STRUCT_TYPE_END;
        ++def)
    {
        std::string field_name(def->f_field_name);
std::cerr << "checking field \"" << field_name << "\" type " << static_cast<int>(def->f_type) << "\n";

        field_t::pointer_t f(std::make_shared<field_t>());
        f->f_description = def;
        f->f_offset = offset;
        bool has_sub_defs(false);
        size_t bit_field(0);
        switch(def->f_type)
        {
        case struct_type_t::STRUCT_TYPE_VOID:
            break;

        case struct_type_t::STRUCT_TYPE_BITS8:
            bit_field = 8;
#if __cplusplus >= 201700
            [[fallthrough]];
#endif
        case struct_type_t::STRUCT_TYPE_INT8:
        case struct_type_t::STRUCT_TYPE_UINT8:
            f->f_size = 1;
            if(f_buffer->count_buffers() != 0)
            {
                offset += 1;
            }
            break;

        case struct_type_t::STRUCT_TYPE_BITS16:
            bit_field = 16;
#if __cplusplus >= 201700
            [[fallthrough]];
#endif
        case struct_type_t::STRUCT_TYPE_INT16:
        case struct_type_t::STRUCT_TYPE_UINT16:
            f->f_size = 2;
            if(f_buffer->count_buffers() != 0)
            {
                offset += 2;
            }
            break;

        case struct_type_t::STRUCT_TYPE_BITS32:
            bit_field = 32;
#if __cplusplus >= 201700
            [[fallthrough]];
#endif
        case struct_type_t::STRUCT_TYPE_INT32:
        case struct_type_t::STRUCT_TYPE_UINT32:
        case struct_type_t::STRUCT_TYPE_FLOAT32:
        case struct_type_t::STRUCT_TYPE_VERSION:
            f->f_size = 4;
            if(f_buffer->count_buffers() != 0)
            {
                offset += 4;
            }
            break;

        case struct_type_t::STRUCT_TYPE_BITS64:
            bit_field = 64;
#if __cplusplus >= 201700
            [[fallthrough]];
#endif
        case struct_type_t::STRUCT_TYPE_INT64:
        case struct_type_t::STRUCT_TYPE_UINT64:
        case struct_type_t::STRUCT_TYPE_FLOAT64:
        case struct_type_t::STRUCT_TYPE_REFERENCE:
        case struct_type_t::STRUCT_TYPE_OID:
        case struct_type_t::STRUCT_TYPE_TIME:
        case struct_type_t::STRUCT_TYPE_MSTIME:
        case struct_type_t::STRUCT_TYPE_USTIME:
            f->f_size = 8;
            if(f_buffer->count_buffers() != 0)
            {
                offset += 8;
            }
            break;

        case struct_type_t::STRUCT_TYPE_BITS128:
            bit_field = 128;
#if __cplusplus >= 201700
            [[fallthrough]];
#endif
        case struct_type_t::STRUCT_TYPE_INT128:
        case struct_type_t::STRUCT_TYPE_UINT128:
        case struct_type_t::STRUCT_TYPE_FLOAT128:
            f->f_size = 16;
            if(f_buffer->count_buffers() != 0)
            {
                offset += 16;
            }
            break;

        case struct_type_t::STRUCT_TYPE_BITS256:
            bit_field = 256;
#if __cplusplus >= 201700
            [[fallthrough]];
#endif
        case struct_type_t::STRUCT_TYPE_INT256:
        case struct_type_t::STRUCT_TYPE_UINT256:
            f->f_size = 32;
            if(f_buffer->count_buffers() != 0)
            {
                offset += 32;
            }
            break;

        case struct_type_t::STRUCT_TYPE_BITS512:
            bit_field = 512;
#if __cplusplus >= 201700
            [[fallthrough]];
#endif
        case struct_type_t::STRUCT_TYPE_INT512:
        case struct_type_t::STRUCT_TYPE_UINT512:
            f->f_size = 64;
            if(f_buffer->count_buffers() != 0)
            {
                offset += 64;
            }
            break;

        case struct_type_t::STRUCT_TYPE_CSTRING:
            f->f_flags |= field_t::FIELD_FLAG_VARIABLE_SIZE;
            f->f_size = 1;           // include '\0' in the size
            if(f_buffer->count_buffers() != 0)
            {
                ++offset;

                // in this case we have to read the data to find the '\0'
                //
                for(;; ++offset)
                {
                    char c;
                    f_buffer->pread(&c, 1, offset);
                    if(c == 0)
                    {
                        ++offset;
                        break;
                    }
                }
            }
            break;

        case struct_type_t::STRUCT_TYPE_P8STRING:
        case struct_type_t::STRUCT_TYPE_BUFFER8:
            f->f_flags |= field_t::FIELD_FLAG_VARIABLE_SIZE;
            if(f_buffer->count_buffers() != 0)
            {
                uint8_t sz;
                f_buffer->pread(&sz, 1, offset);
                f->f_size = sz + 1;
                offset += f->f_size;
            }
            break;

        case struct_type_t::STRUCT_TYPE_P16STRING:
        case struct_type_t::STRUCT_TYPE_BUFFER16:
            f->f_flags |= field_t::FIELD_FLAG_VARIABLE_SIZE;
            if(f_buffer->count_buffers() != 0)
            {
                uint16_t sz;
                f_buffer->pread(&sz, 2, offset);
                f->f_size = (sz << 8) + (sz >> 8) + 2;
                offset += f->f_size;
            }
            break;

        case struct_type_t::STRUCT_TYPE_P32STRING:
        case struct_type_t::STRUCT_TYPE_BUFFER32:
            f->f_flags |= field_t::FIELD_FLAG_VARIABLE_SIZE;
            if(f_buffer->count_buffers() != 0)
            {
                uint32_t sz;
                f_buffer->pread(&sz, 4, offset);
                f->f_size =
                      ((reinterpret_cast<uint8_t *>(&sz)[0] << 24)
                    |  (reinterpret_cast<uint8_t *>(&sz)[1] << 16)
                    |  (reinterpret_cast<uint8_t *>(&sz)[2] <<  8)
                    |  (reinterpret_cast<uint8_t *>(&sz)[3] <<  0))
                    + 4;
                offset += f->f_size;
            }
            break;

        case struct_type_t::STRUCT_TYPE_STRUCTURE:
            // here f_size is a count, not a byte size
            f->f_size = 1;
            has_sub_defs = true;
            break;

        case struct_type_t::STRUCT_TYPE_ARRAY8:
            // here f_size is a count, not a byte size
            f->f_flags |= field_t::FIELD_FLAG_VARIABLE_SIZE;
            if(f_buffer->count_buffers() != 0)
            {
                uint8_t sz;
                f_buffer->pread(&sz, 1, offset);
                f->f_size = sz;
                ++offset;
            }
            has_sub_defs = true;
            break;

        case struct_type_t::STRUCT_TYPE_ARRAY16:
            // here f_size is a count, not a byte size
            f->f_flags |= field_t::FIELD_FLAG_VARIABLE_SIZE;
            if(f_buffer->count_buffers() != 0)
            {
                uint16_t sz;
                f_buffer->pread(&sz, 1, offset);
                f->f_size = (sz << 8) + (sz >> 8) + 2;
                offset += 2;
            }
            has_sub_defs = true;
            break;

        case struct_type_t::STRUCT_TYPE_ARRAY32:
            // here f_size is a count, not a byte size
            f->f_flags |= field_t::FIELD_FLAG_VARIABLE_SIZE;
            if(f_buffer->count_buffers() != 0)
            {
                uint32_t sz;
                f_buffer->pread(&sz, 4, offset);
                f->f_size =
                      ((reinterpret_cast<uint8_t *>(&sz)[0] << 24)
                    |  (reinterpret_cast<uint8_t *>(&sz)[1] << 16)
                    |  (reinterpret_cast<uint8_t *>(&sz)[2] <<  8)
                    |  (reinterpret_cast<uint8_t *>(&sz)[3] <<  0))
                    + 4;
                offset += 4;
            }
            has_sub_defs = true;
            break;

        case struct_type_t::STRUCT_TYPE_END:
        case struct_type_t::STRUCT_TYPE_RENAMED:
            throw invalid_size("This field does not offer a size which can be queried.");

        }

std::cerr << "check field offset vs size " << offset << " / " << f_buffer->size() << "\n";
        if(f_buffer->count_buffers() != 0
        && offset > f_buffer->size())
        {
            throw invalid_size(
                      "Field \""
                    + field_name
                    + "\" is too large for the specified data buffer.");
        }

        if(def->f_sub_description != nullptr)
        {
            if(!has_sub_defs)
            {
                throw snapdatabase_logic_error(
                          "Field \""
                        + field_name
                        + "\" has its \"f_sub_description\" field set to a pointer when its type doesn't allow it.");
            }

            f->f_sub_structures.reserve(f->f_size);
            for(size_t idx(0); idx < f->f_size; ++idx)
            {
                pointer_t s(std::make_shared<structure>(def->f_sub_description));
                s->set_virtual_buffer(f_buffer, offset);
                offset = s->parse_descriptions(offset);
                //s->f_end_offset = offset; -- TBD: would that be useful?

                f->f_sub_structures.push_back(s);
            }
        }
        else if(bit_field > 0)
        {
            // TODO: add support for 128, 256, and 512 at some point
            //       (if it becomes useful)
            //
            if(bit_field > 64)
            {
                bit_field = 64;
            }

            std::string::size_type pos(field_name.find('='));

            field_name = field_name.substr(0, pos);

            size_t bit_pos(0);
            std::string::size_type end(pos);
            for(;;)
            {
                std::string::size_type start(end + 1);
                if(start >= field_name.size())
                {
                    break;
                }
                end = field_name.find_first_of(":/", start);
                std::int64_t size(1);
                std::string flag_name;
                if(end == std::string::npos)
                {
                    // no ':' or '/', we found the last flag
                    // and it has a size of 1
                    //
                    flag_name = field_name.substr(start);
                }
                else
                {
                    flag_name = field_name.substr(start, end - start);

                    // name:size separator?
                    //
                    if(field_name[end] == ':')
                    {
                        start = end + 1;

                        std::string size_str;
                        end = field_name.find_first_of(":/", start);
                        if(end == std::string::npos)
                        {
                            // no '/', we found the last position
                            //
                            size_str = field_name.substr(start);
                        }
                        else if(field_name[end] == '/')
                        {
                            size_str = field_name.substr(start, end - start);
                        }
                        else
                        {
                            throw invalid_size(
                                  "The size of bit field \""
                                + flag_name
                                + "\" includes two colons.");
                        }

                        if(!advgetopt::validator_integer::convert_string(size_str, size))
                        {
                            throw invalid_size(
                                  "The size ("
                                + size_str
                                + ") of this bit field \""
                                + flag_name
                                + "\" is invalid.");
                        }
                        if(bit_pos + size > bit_field)
                        {
                            throw invalid_size(
                                  "The total number of bits used by bit field \""
                                + flag_name
                                + "\" overflows the maximum allowed of "
                                + std::to_string(bit_field)
                                + ".");
                        }
                    }
                }
                flag_definition bits(field_name, flag_name, bit_pos, size);
                f->f_flag_definitions[flag_name] = bits;

                bit_pos += size;
            }
        }

        const_cast<structure *>(this)->f_fields_by_name[field_name] = f;
    }

    return offset;
}



} // namespace snapdatabase
// vim: ts=4 sw=4 et
