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
#include    "snapdatabase/dbfile.h"


// last include
//
#include    <snapdev/poison.h>



namespace snapdatabase
{



namespace
{


struct name_to_struct_type
{
    char const *        f_name = nullptr;
    struct_type_t       f_type = struct_type_t::STRUCT_TYPE_END;
};

#define NAME_TO_STRUCT_TYPE(name)   { #name, struct_type_t::STRUCT_TYPE_##name }

name_to_struct_type g_name_to_struct_type[] =
{
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
    NAME_TO_STRUCT_TYPE(P16STRING),
    NAME_TO_STRUCT_TYPE(P32STRING),
    NAME_TO_STRUCT_TYPE(P8STRING),
    NAME_TO_STRUCT_TYPE(REFERENCE),
    NAME_TO_STRUCT_TYPE(RENAMED)
    NAME_TO_STRUCT_TYPE(STRUCTURE),
    NAME_TO_STRUCT_TYPE(UINT16),
    NAME_TO_STRUCT_TYPE(UINT32),
    NAME_TO_STRUCT_TYPE(UINT8),
    NAME_TO_STRUCT_TYPE(UINT64),
    NAME_TO_STRUCT_TYPE(UINT128),
    NAME_TO_STRUCT_TYPE(UINT256),
    NAME_TO_STRUCT_TYPE(UINT512),
    NAME_TO_STRUCT_TYPE(VERSION),
    NAME_TO_STRUCT_TYPE(VOID),
};



union float32_uint32_t
{
    float       f_float = 0.0l;
    uint32_t    f_int = 0;
};


union float64_uint64_t
{
    double      f_float = 0.0l;
    uint64_t    f_int = 0;
};


union float128_uint128_t
{
    long double f_float = 0.0l;
    uint64_t    f_int[2] = 0;
};





}
// no name namespace



struct_type_t name_to_struct_type(std::string const & type_name)
{
#ifdef _DEBUG
    for(size_t idx(0);
        idx < sizeof(g_name_to_struct_type) / sizeof(g_name_to_struct_type[0]);
        ++idx)
    {
        if(strcmp(g_name_to_struct_type[idx - 1].f_name
                , g_name_to_struct_type[idx].f_name) >= 0)
        {
            throw snapdatabase_logic_error(
                      "name to struct type not in alphabetical order "
                    + std::string(g_name_to_struct_type[idx - 1].f_name)
                    + " >= "
                    + g_name_to_struct_type[idx].f_name
                    + " (position: "
                    + std::to_string(idx));
        }
    }
#endif

    std::string const lc(boost::algorithm::to_upper(type_name));

    int j(sizeof(g_name_to_struct_type) / sizeof(g_name_to_struct_type[0]));
    int i(0);
    while(i < j)
    {
        int const p((j - i) / 2 + i);
        int const r(type_name.compare(g_name_to_struct_type[p].f_name));
        if(r < 0)
        {
            i = p + 1;
        }
        else if(r > 0)
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



flag_definition::flag_definition(
          std::string const & name
        , size_t pos
        , size_t size)
    : f_name(name)
    , f_pos(pos)
    , f_size(size)
    , f_mask(((1 << size) - 1) << pos)  // fails if size == 64...
{
    if(size == 0)
    {
        throw invalid_parameter(
                  "Bit field named \""
                + name
                + "\" can't have a size of 0.");
    }
    if(size >= 64)
    {
        throw invalid_parameter(
                  "Bit field named \""
                + name
                + "\" is too large ("
                + std::to_string(size)
                + " >= 64).");
    }
    if(pos + size > 64)
    {
        throw invalid_parameter(
                  "The mask of the bit field named \""
                + name
                + "\" does not fit in a uint64_t.");
    }
}


std::string flag_definition::name() const
{
    return f_name;
}


size_t flag_definition::pos() const
{
    return f_pos;
}


size_t flag_definition::size() const
{
    return f_size;
}


flag_t flag_definition::mask() const
{
    return f_mask;
}





field_t::~field_t()
{
    if((f_flags & FIELD_FLAG_ALLOCATED) != 0)
    {
        delete f_data;
    }
}


uint32_t field_t::size() const
{
    return f_size;
}


structure_pointer_t field_t::operator [] (int idx)
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





structure::structure(struct_description const * description)
    : f_descriptions(descriptions)
{
}


structure::structure(
              struct_description const * description
            , virtual_buffer & buffer
            , uint64_t start_offset)
    : f_descriptions(descriptions)
    , f_buffer(buffer)
    , f_start_offset(start_offset)
{
}


field_t & structure::get_field(std::string const & field_name, type_t type) const
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
            switch(f->f_description->f_type)
            {
            case struct_type_t::STRUCT_TYPE_BITS8:
            case struct_type_t::STRUCT_TYPE_BITS16:
            case struct_type_t::STRUCT_TYPE_BITS32:
            case struct_type_t::STRUCT_TYPE_BITS64:
            case struct_type_t::STRUCT_TYPE_BITS128:
            case struct_type_t::STRUCT_TYPE_BITS256:
            case struct_type_t::STRUCT_TYPE_BITS512:
                {
                    auto flag(f_flag_definitions.find(field_name));
                    if(flag == f_flag_definitions.end())
                    {
                        // found it!
                        //
                        return f;
                    }
                }
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

    field_t::pointer_t f(field.second);
    if(f->f_description->f_type == struct_type_t::STRUCT_TYPE_RENAMED)
    {
        std::string const new_name(f->f_description->f_sub_description->f_name);
        auto field(f_fields_by_name.find(new_name));
        if(field == f_fields_by_name.end())
        {
            throw field_not_found(
                      "This description renames field \""
                    + field_name
                    + "\" to \""
                    + new_name
                    + "\" but we could not find the new name field.");
        }
        f = field.second;

        // let programmers know that the old name is deprecated
        SNAP_LOG_DEBUG
            << "Deprecated field name \""
            << field_name
            << "\" was changed to \""
            << new_name
            << "\". Please change your code to use the new name."
            << SNAP_LOG_END;
    }

    if(type != struct_type_t::STRUCT_TYPE_END
    && f->f_description->f_type != type)
    {
        throw type_mismatch(
                  "This field type is \""
                + std::to_string(f->f_description->f_type)
                + "\" but we expected \""
                + std::to_string(static_cast<int>(type))
                + "\".");
    }

    return f;
}


int64_t structure::get_integer(std::string const & field_name) const
{
    auto f(get_field(field_name));

    switch(f->f_description->f_type)
    {
    case struct_type_t::STRUCT_TYPE_INT8:
        int8_t v;
        f_buffer.read(&v, 1, 0);
    int                                     pread(void * buf, uint64_t size, uint64_t offset, bool full = true) const;
        return static_cast<int8_t>(f->f_data[0]);

    case struct_type_t::STRUCT_TYPE_INT16:
        return static_cast<int16_t>(
               (f->f_data[0] << 8)
             | (f->f_data[1] << 0));

    case struct_type_t::STRUCT_TYPE_INT32:
        return static_cast<int32_t>(
             | (f->f_data[0] << 24)
             | (f->f_data[1] << 16)
             | (f->f_data[2] <<  8)
             | (f->f_data[3] <<  0));

    case struct_type_t::STRUCT_TYPE_INT64:
        return (f->f_data[0] << 56)
             | (f->f_data[1] << 48)
             | (f->f_data[2] << 40)
             | (f->f_data[3] << 32)
             | (f->f_data[4] << 24)
             | (f->f_data[5] << 16)
             | (f->f_data[6] <<  8)
             | (f->f_data[7] <<  0);

    }

    throw type_mismatch(
              "This description type is \""
            + std::to_string(f->f_description->f_type)
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


void structure::set_integer(std::string const & field_name, int64_t value)
{
    auto f(get_field(field_name));

    switch(f->f_description->f_type)
    {
    case struct_type_t::STRUCT_TYPE_INT8:
        f->f_data[0] = value;
        return;

    case struct_type_t::STRUCT_TYPE_INT16:
        f->f_data[0] = value >> 8;
        f->f_data[1] = value >> 0;
        return;

    case struct_type_t::STRUCT_TYPE_INT32:
        f->f_data[0] = value >> 24;
        f->f_data[1] = value >> 16;
        f->f_data[2] = value >>  8;
        f->f_data[3] = value >>  0;
        return;

    case struct_type_t::STRUCT_TYPE_INT64:
        f->f_data[0] = value >> 56;
        f->f_data[1] = value >> 48;
        f->f_data[2] = value >> 40;
        f->f_data[3] = value >> 32;
        f->f_data[4] = value >> 24;
        f->f_data[5] = value >> 16;
        f->f_data[6] = value >>  8;
        f->f_data[7] = value >>  0;
        return;

    }

    throw type_mismatch(
              "This description type is \""
            + std::to_string(f->f_description->f_type)
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


uint64_t structure::get_uinteger(std::string const & field_name) const
{
    auto f(get_field(field_name));

    switch(f->f_description->f_type)
    {
    case struct_type_t::STRUCT_TYPE_BITS8:
    case struct_type_t::STRUCT_TYPE_UINT8:
        return f->f_data[0];

    case struct_type_t::STRUCT_TYPE_BITS16:
    case struct_type_t::STRUCT_TYPE_UINT16:
        return (f->f_data[0] << 8)
             | (f->f_data[1] << 0);

    case struct_type_t::STRUCT_TYPE_BITS32:
    case struct_type_t::STRUCT_TYPE_UINT32:
        return (f->f_data[0] << 24)
             | (f->f_data[1] << 16)
             | (f->f_data[2] <<  8)
             | (f->f_data[3] <<  0);

    case struct_type_t::STRUCT_TYPE_BITS64:
    case struct_type_t::STRUCT_TYPE_UINT64:
    case struct_type_t::STRUCT_TYPE_REFERENCE:
        return (f->f_data[0] << 56)
             | (f->f_data[1] << 48)
             | (f->f_data[2] << 40)
             | (f->f_data[3] << 32)
             | (f->f_data[4] << 24)
             | (f->f_data[5] << 16)
             | (f->f_data[6] <<  8)
             | (f->f_data[7] <<  0);

    }

    throw type_mismatch(
              "This description type is \""
            + std::to_string(f->f_description->f_type)
            + "\" but we expected one of \""
            + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_BIT8))
            + ", "
            + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_BIT16))
            + ", "
            + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_BIT32))
            + ", "
            + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_BIT64))
            + ", "
            + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_UINT8))
            + ", "
            + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_UINT16))
            + ", "
            + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_UINT32))
            + ", "
            + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_UINT64))
            + ", "
            + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_REFERENCE))
            + "\".");
}


void structure::set_uinteger(std::string const & field_name, uint64_t value)
{
    auto f(get_field(field_name));

    switch(f->f_description->f_type)
    {
    case struct_type_t::STRUCT_TYPE_BITS8:
    case struct_type_t::STRUCT_TYPE_UINT8:
        f->f_data[0] = value;
        return;

    case struct_type_t::STRUCT_TYPE_BITS16:
    case struct_type_t::STRUCT_TYPE_UINT16:
        f->f_data[0] = value >> 8;
        f->f_data[1] = value >> 0;
        return;

    case struct_type_t::STRUCT_TYPE_BITS32:
    case struct_type_t::STRUCT_TYPE_UINT32:
        f->f_data[0] = value >> 24;
        f->f_data[1] = value >> 16;
        f->f_data[2] = value >>  8;
        f->f_data[3] = value >>  0;
        return;

    case struct_type_t::STRUCT_TYPE_BITS64:
    case struct_type_t::STRUCT_TYPE_UINT64:
    case struct_type_t::STRUCT_TYPE_REFERENCE:
        f->f_data[0] = value >> 56;
        f->f_data[1] = value >> 48;
        f->f_data[2] = value >> 40;
        f->f_data[3] = value >> 32;
        f->f_data[4] = value >> 24;
        f->f_data[5] = value >> 16;
        f->f_data[6] = value >>  8;
        f->f_data[7] = value >>  0;
        return;

    }

    throw type_mismatch(
              "This description type is \""
            + std::to_string(f->f_description->f_type)
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


uint64_t structure::get_bits(std::string const & flag_name) const
{
    auto f(get_field(flag_name));

    auto it(f->f_flag_definitions.find(flag_name));
    if(it == f->f_flags_definitions.end())
    {
        throw type_mismatch(
                  "get_bits() called with field \""
                + field_name
                + "\" which has no flag definitions...");
    }
    auto flag(it.second);

    switch(f->f_description->f_type)
    {
    case struct_type_t::STRUCT_TYPE_BITS8:
        return (f->f_data[0] & flag.mask()) >> flag.pos();

    case struct_type_t::STRUCT_TYPE_BITS16:
        return (((f->f_data[0] << 8)
               | (f->f_data[1] << 0)) & flag.mask()) >> flag.pos();

    case struct_type_t::STRUCT_TYPE_BITS32:
        return (((f->f_data[0] << 24)
               | (f->f_data[1] << 16)
               | (f->f_data[2] <<  8)
               | (f->f_data[3] <<  0)) & flag.mask()) >> flag.pos();

    case struct_type_t::STRUCT_TYPE_BITS64:
        return (((f->f_data[0] << 56)
               | (f->f_data[1] << 48)
               | (f->f_data[2] << 40)
               | (f->f_data[3] << 32)
               | (f->f_data[4] << 24)
               | (f->f_data[5] << 16)
               | (f->f_data[6] <<  8)
               | (f->f_data[7] <<  0)) & flag.mask()) >> flag.pos();

    }

    throw type_mismatch(
              "This description type is \""
            + std::to_string(f->f_description->f_type)
            + "\" but we expected one of \""
            + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_BIT8))
            + ", "
            + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_BIT16))
            + ", "
            + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_BIT32))
            + ", "
            + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_BIT64))
            + "\".");
}


void structure::set_bits(std::string const & flag_name, uint64_t value)
{
    auto f(get_field(flag_name));

    auto it(f->f_flag_definitions.find(flag_name));
    if(it == f->f_flags_definitions.end())
    {
        throw type_mismatch(
                  "set_bits() called with field \""
                + field_name
                + "\" which has no flag definitions...");
    }
    auto flag(it.second);

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
                + std::to_string(f->f_description->f_type)
                + "\" but we expected one of \""
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_BIT8))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_BIT16))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_BIT32))
                + ", "
                + std::to_string(static_cast<int>(struct_type_t::STRUCT_TYPE_BIT64))
                + "\".");

    }

    if((value & (flag.mask() >> flag.pos())) != value)
    {
        throw invalid_value(
                  "Value \""
                + std::to_string(value)
                + "\" does not fit in flag field \""
                + flag_name
                + "\".")
    }

    // some day we may want to optimize better, but this is the easiest
    // right now
    //
    uint64_t v(get_uinteger(field_name));
    v &= ~flag.mask();
    v |= value << flag.pos();
    set_uinteger(field_name, v);
}


int512_t structure::get_large_integer(std::string const & field_name) const
{
    auto f(get_field(field_name));

    int512_t result;
    switch(f->f_description->f_type)
    {
    case struct_type_t::STRUCT_TYPE_INT8:
        result.f_value[0] = static_cast<int8_t>(f->f_data[0]);
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
        result.f_value[0] = static_cast<int16_t>(
              (f->f_data[0] << 8)
            | (f->f_data[1] << 0));
        goto sign_extend_64bit;

    case struct_type_t::STRUCT_TYPE_INT32:
        result.f_value[0] = static_cast<int32_t>(
               (f->f_data[0] << 24)
             | (f->f_data[1] << 16)
             | (f->f_data[2] <<  8)
             | (f->f_data[3] <<  0));
        goto sign_extend_64bit;

    case struct_type_t::STRUCT_TYPE_INT64:
        result.f_value[0] =
               (f->f_data[0] << 56)
             | (f->f_data[1] << 48)
             | (f->f_data[2] << 40)
             | (f->f_data[3] << 32)
             | (f->f_data[4] << 24)
             | (f->f_data[5] << 16)
             | (f->f_data[6] <<  8)
             | (f->f_data[7] <<  0);
        goto sign_extend_64bit;

    case struct_type_t::STRUCT_TYPE_INT128:
        result.f_value[1] =
               (f->f_data[0] << 56)
             | (f->f_data[1] << 48)
             | (f->f_data[2] << 40)
             | (f->f_data[3] << 32)
             | (f->f_data[4] << 24)
             | (f->f_data[5] << 16)
             | (f->f_data[6] <<  8)
             | (f->f_data[7] <<  0);

        result.f_value[0] =
               (f->f_data[ 8] << 56)
             | (f->f_data[ 9] << 48)
             | (f->f_data[10] << 40)
             | (f->f_data[11] << 32)
             | (f->f_data[12] << 24)
             | (f->f_data[13] << 16)
             | (f->f_data[14] <<  8)
             | (f->f_data[15] <<  0);

        result.f_value[2] = (static_cast<int64_t>(result.f_value[1]) < 0
                                    ? -1
                                    : 0;
        result.f_value[3] = result.f_value[2];
        result.f_value[4] = result.f_value[2];
        result.f_value[5] = result.f_value[2];
        result.f_value[6] = result.f_value[2];
        result.f_high_value = result.f_value[2];

        return result;

    case struct_type_t::STRUCT_TYPE_INT256:
        result.f_value[3] =
               (f->f_data[0] << 56)
             | (f->f_data[1] << 48)
             | (f->f_data[2] << 40)
             | (f->f_data[3] << 32)
             | (f->f_data[4] << 24)
             | (f->f_data[5] << 16)
             | (f->f_data[6] <<  8)
             | (f->f_data[7] <<  0);

        result.f_value[2] =
               (f->f_data[ 8] << 56)
             | (f->f_data[ 9] << 48)
             | (f->f_data[10] << 40)
             | (f->f_data[11] << 32)
             | (f->f_data[12] << 24)
             | (f->f_data[13] << 16)
             | (f->f_data[14] <<  8)
             | (f->f_data[15] <<  0);

        result.f_value[1] =
               (f->f_data[16] << 56)
             | (f->f_data[17] << 48)
             | (f->f_data[18] << 40)
             | (f->f_data[19] << 32)
             | (f->f_data[20] << 24)
             | (f->f_data[21] << 16)
             | (f->f_data[22] <<  8)
             | (f->f_data[23] <<  0);

        result.f_value[0] =
               (f->f_data[24] << 56)
             | (f->f_data[25] << 48)
             | (f->f_data[26] << 40)
             | (f->f_data[27] << 32)
             | (f->f_data[28] << 24)
             | (f->f_data[29] << 16)
             | (f->f_data[30] <<  8)
             | (f->f_data[31] <<  0);

        result.f_value[4] = (static_cast<int64_t>(result.f_value[3]) < 0
                                    ? -1
                                    : 0;
        result.f_value[5] = result.f_value[4];
        result.f_value[6] = result.f_value[4];
        result.f_high_value = result.f_value[4];

        return result;

    case struct_type_t::STRUCT_TYPE_INT512:
        result.f_high_value =
               (f->f_data[0] << 56)
             | (f->f_data[1] << 48)
             | (f->f_data[2] << 40)
             | (f->f_data[3] << 32)
             | (f->f_data[4] << 24)
             | (f->f_data[5] << 16)
             | (f->f_data[6] <<  8)
             | (f->f_data[7] <<  0);

        result.f_value[6] =
               (f->f_data[ 8] << 56)
             | (f->f_data[ 9] << 48)
             | (f->f_data[10] << 40)
             | (f->f_data[11] << 32)
             | (f->f_data[12] << 24)
             | (f->f_data[13] << 16)
             | (f->f_data[14] <<  8)
             | (f->f_data[15] <<  0);

        result.f_value[5] =
               (f->f_data[16] << 56)
             | (f->f_data[17] << 48)
             | (f->f_data[18] << 40)
             | (f->f_data[19] << 32)
             | (f->f_data[20] << 24)
             | (f->f_data[21] << 16)
             | (f->f_data[22] <<  8)
             | (f->f_data[23] <<  0);

        result.f_value[4] =
               (f->f_data[24] << 56)
             | (f->f_data[25] << 48)
             | (f->f_data[26] << 40)
             | (f->f_data[27] << 32)
             | (f->f_data[28] << 24)
             | (f->f_data[29] << 16)
             | (f->f_data[30] <<  8)
             | (f->f_data[31] <<  0);

        result.f_value[3] =
               (f->f_data[32] << 56)
             | (f->f_data[33] << 48)
             | (f->f_data[34] << 40)
             | (f->f_data[35] << 32)
             | (f->f_data[36] << 24)
             | (f->f_data[37] << 16)
             | (f->f_data[38] <<  8)
             | (f->f_data[39] <<  0);

        result.f_value[2] =
               (f->f_data[40] << 56)
             | (f->f_data[41] << 48)
             | (f->f_data[42] << 40)
             | (f->f_data[43] << 32)
             | (f->f_data[44] << 24)
             | (f->f_data[45] << 16)
             | (f->f_data[46] <<  8)
             | (f->f_data[47] <<  0);

        result.f_value[1] =
               (f->f_data[48] << 56)
             | (f->f_data[49] << 48)
             | (f->f_data[50] << 40)
             | (f->f_data[51] << 32)
             | (f->f_data[52] << 24)
             | (f->f_data[53] << 16)
             | (f->f_data[54] <<  8)
             | (f->f_data[55] <<  0);

        result.f_value[0] =
               (f->f_data[56] << 56)
             | (f->f_data[57] << 48)
             | (f->f_data[58] << 40)
             | (f->f_data[59] << 32)
             | (f->f_data[60] << 24)
             | (f->f_data[61] << 16)
             | (f->f_data[62] <<  8)
             | (f->f_data[63] <<  0);

        return result;

    }

    throw type_mismatch(
              "This description type is \""
            + std::to_string(f->f_description->f_type)
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


void structure::set_large_integer(std::string const & field_name, int512_t value)
{
    auto f(get_field(field_name));

    switch(f->f_description->f_type)
    {
    case struct_type_t::STRUCT_TYPE_INT8:
        f->f_data[0] = value.f_value[0];
        return;

    case struct_type_t::STRUCT_TYPE_INT16:
        f->f_data[0] = value.f_value[0] >> 8;
        f->f_data[1] = value.f_value[0] >> 0;
        return;

    case struct_type_t::STRUCT_TYPE_INT32:
        f->f_data[0] = value.f_value[0] >> 24;
        f->f_data[1] = value.f_value[0] >> 16;
        f->f_data[2] = value.f_value[0] >>  8;
        f->f_data[3] = value.f_value[0] >>  0;
        return;

    case struct_type_t::STRUCT_TYPE_INT64:
        f->f_data[0] = value.f_value[0] >> 56;
        f->f_data[1] = value.f_value[0] >> 48;
        f->f_data[2] = value.f_value[0] >> 40;
        f->f_data[3] = value.f_value[0] >> 32;
        f->f_data[4] = value.f_value[0] >> 24;
        f->f_data[5] = value.f_value[0] >> 16;
        f->f_data[6] = value.f_value[0] >>  8;
        f->f_data[7] = value.f_value[0] >>  0;
        return;

    case struct_type_t::STRUCT_TYPE_INT128:
        f->f_data[0] = value.f_value[1] >> 56;
        f->f_data[1] = value.f_value[1] >> 48;
        f->f_data[2] = value.f_value[1] >> 40;
        f->f_data[3] = value.f_value[1] >> 32;
        f->f_data[4] = value.f_value[1] >> 24;
        f->f_data[5] = value.f_value[1] >> 16;
        f->f_data[6] = value.f_value[1] >>  8;
        f->f_data[7] = value.f_value[1] >>  0;

        f->f_data[ 8] = value.f_value[0] >> 56;
        f->f_data[ 9] = value.f_value[0] >> 48;
        f->f_data[10] = value.f_value[0] >> 40;
        f->f_data[11] = value.f_value[0] >> 32;
        f->f_data[12] = value.f_value[0] >> 24;
        f->f_data[13] = value.f_value[0] >> 16;
        f->f_data[14] = value.f_value[0] >>  8;
        f->f_data[15] = value.f_value[0] >>  0;

        return;

    case struct_type_t::STRUCT_TYPE_INT256:
        f->f_data[0] = value.f_value[3] >> 56;
        f->f_data[1] = value.f_value[3] >> 48;
        f->f_data[2] = value.f_value[3] >> 40;
        f->f_data[3] = value.f_value[3] >> 32;
        f->f_data[4] = value.f_value[3] >> 24;
        f->f_data[5] = value.f_value[3] >> 16;
        f->f_data[6] = value.f_value[3] >>  8;
        f->f_data[7] = value.f_value[3] >>  0;

        f->f_data[ 8] = value.f_value[2] >> 56;
        f->f_data[ 9] = value.f_value[2] >> 48;
        f->f_data[10] = value.f_value[2] >> 40;
        f->f_data[11] = value.f_value[2] >> 32;
        f->f_data[12] = value.f_value[2] >> 24;
        f->f_data[13] = value.f_value[2] >> 16;
        f->f_data[14] = value.f_value[2] >>  8;
        f->f_data[15] = value.f_value[2] >>  0;

        f->f_data[16] = value.f_value[1] >> 56;
        f->f_data[17] = value.f_value[1] >> 48;
        f->f_data[18] = value.f_value[1] >> 40;
        f->f_data[19] = value.f_value[1] >> 32;
        f->f_data[20] = value.f_value[1] >> 24;
        f->f_data[21] = value.f_value[1] >> 16;
        f->f_data[22] = value.f_value[1] >>  8;
        f->f_data[23] = value.f_value[1] >>  0;

        f->f_data[24] = value.f_value[0] >> 56;
        f->f_data[25] = value.f_value[0] >> 48;
        f->f_data[26] = value.f_value[0] >> 40;
        f->f_data[27] = value.f_value[0] >> 32;
        f->f_data[28] = value.f_value[0] >> 24;
        f->f_data[29] = value.f_value[0] >> 16;
        f->f_data[30] = value.f_value[0] >>  8;
        f->f_data[31] = value.f_value[0] >>  0;

        return;

    case struct_type_t::STRUCT_TYPE_INT512:
        f->f_data[0] = value.f_high_value >> 56;
        f->f_data[1] = value.f_high_value >> 48;
        f->f_data[2] = value.f_high_value >> 40;
        f->f_data[3] = value.f_high_value >> 32;
        f->f_data[4] = value.f_high_value >> 24;
        f->f_data[5] = value.f_high_value >> 16;
        f->f_data[6] = value.f_high_value >>  8;
        f->f_data[7] = value.f_high_value >>  0;

        f->f_data[ 8] = value.f_value[6] >> 56;
        f->f_data[ 9] = value.f_value[6] >> 48;
        f->f_data[10] = value.f_value[6] >> 40;
        f->f_data[11] = value.f_value[6] >> 32;
        f->f_data[12] = value.f_value[6] >> 24;
        f->f_data[13] = value.f_value[6] >> 16;
        f->f_data[14] = value.f_value[6] >>  8;
        f->f_data[15] = value.f_value[6] >>  0;

        f->f_data[16] = value.f_value[5] >> 56;
        f->f_data[17] = value.f_value[5] >> 48;
        f->f_data[18] = value.f_value[5] >> 40;
        f->f_data[19] = value.f_value[5] >> 32;
        f->f_data[20] = value.f_value[5] >> 24;
        f->f_data[21] = value.f_value[5] >> 16;
        f->f_data[22] = value.f_value[5] >>  8;
        f->f_data[23] = value.f_value[5] >>  0;

        f->f_data[24] = value.f_value[4] >> 56;
        f->f_data[25] = value.f_value[4] >> 48;
        f->f_data[26] = value.f_value[4] >> 40;
        f->f_data[27] = value.f_value[4] >> 32;
        f->f_data[28] = value.f_value[4] >> 24;
        f->f_data[29] = value.f_value[4] >> 16;
        f->f_data[30] = value.f_value[4] >>  8;
        f->f_data[31] = value.f_value[4] >>  0;

        f->f_data[32] = value.f_value[3] >> 56;
        f->f_data[33] = value.f_value[3] >> 48;
        f->f_data[34] = value.f_value[3] >> 40;
        f->f_data[35] = value.f_value[3] >> 32;
        f->f_data[36] = value.f_value[3] >> 24;
        f->f_data[37] = value.f_value[3] >> 16;
        f->f_data[38] = value.f_value[3] >>  8;
        f->f_data[39] = value.f_value[3] >>  0;

        f->f_data[40] = value.f_value[2] >> 56;
        f->f_data[41] = value.f_value[2] >> 48;
        f->f_data[42] = value.f_value[2] >> 40;
        f->f_data[43] = value.f_value[2] >> 32;
        f->f_data[44] = value.f_value[2] >> 24;
        f->f_data[45] = value.f_value[2] >> 16;
        f->f_data[46] = value.f_value[2] >>  8;
        f->f_data[47] = value.f_value[2] >>  0;

        f->f_data[48] = value.f_value[1] >> 56;
        f->f_data[49] = value.f_value[1] >> 48;
        f->f_data[50] = value.f_value[1] >> 40;
        f->f_data[51] = value.f_value[1] >> 32;
        f->f_data[52] = value.f_value[1] >> 24;
        f->f_data[53] = value.f_value[1] >> 16;
        f->f_data[54] = value.f_value[1] >>  8;
        f->f_data[55] = value.f_value[1] >>  0;

        f->f_data[56] = value.f_value[0] >> 56;
        f->f_data[57] = value.f_value[0] >> 48;
        f->f_data[58] = value.f_value[0] >> 40;
        f->f_data[59] = value.f_value[0] >> 32;
        f->f_data[60] = value.f_value[0] >> 24;
        f->f_data[61] = value.f_value[0] >> 16;
        f->f_data[62] = value.f_value[0] >>  8;
        f->f_data[63] = value.f_value[0] >>  0;

        return;

    }

    throw type_mismatch(
              "This description type is \""
            + std::to_string(f->f_description->f_type)
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


uint512_t structure::get_large_uinteger(std::string const & field_name) const
{
    auto f(get_field(field_name));

    uint512_t result;
    switch(f->f_description->f_type)
    {
    case struct_type_t::STRUCT_TYPE_BITS8:
    case struct_type_t::STRUCT_TYPE_UINT8:
        result.f_value[0] = f->f_data[0];
        return result;

    case struct_type_t::STRUCT_TYPE_BITS16:
    case struct_type_t::STRUCT_TYPE_UINT16:
        result.f_value[0] =
              (f->f_data[0] << 8)
            | (f->f_data[1] << 0);
        return result;

    case struct_type_t::STRUCT_TYPE_BITS32:
    case struct_type_t::STRUCT_TYPE_UINT32:
        result.f_value[0] =
               (f->f_data[0] << 24)
             | (f->f_data[1] << 16)
             | (f->f_data[2] <<  8)
             | (f->f_data[3] <<  0);
        return result;

    case struct_type_t::STRUCT_TYPE_BITS64:
    case struct_type_t::STRUCT_TYPE_UINT64:
    case struct_type_t::STRUCT_TYPE_REFERENCE:
        result.f_value[0] =
               (f->f_data[0] << 56)
             | (f->f_data[1] << 48)
             | (f->f_data[2] << 40)
             | (f->f_data[3] << 32)
             | (f->f_data[4] << 24)
             | (f->f_data[5] << 16)
             | (f->f_data[6] <<  8)
             | (f->f_data[7] <<  0);
        return result;

    case struct_type_t::STRUCT_TYPE_UINT128:
        result.f_value[1] =
               (f->f_data[0] << 56)
             | (f->f_data[1] << 48)
             | (f->f_data[2] << 40)
             | (f->f_data[3] << 32)
             | (f->f_data[4] << 24)
             | (f->f_data[5] << 16)
             | (f->f_data[6] <<  8)
             | (f->f_data[7] <<  0);

        result.f_value[0] =
               (f->f_data[ 8] << 56)
             | (f->f_data[ 9] << 48)
             | (f->f_data[10] << 40)
             | (f->f_data[11] << 32)
             | (f->f_data[12] << 24)
             | (f->f_data[13] << 16)
             | (f->f_data[14] <<  8)
             | (f->f_data[15] <<  0);

        return result;

    case struct_type_t::STRUCT_TYPE_UINT256:
        result.f_value[3] =
               (f->f_data[0] << 56)
             | (f->f_data[1] << 48)
             | (f->f_data[2] << 40)
             | (f->f_data[3] << 32)
             | (f->f_data[4] << 24)
             | (f->f_data[5] << 16)
             | (f->f_data[6] <<  8)
             | (f->f_data[7] <<  0);

        result.f_value[2] =
               (f->f_data[ 8] << 56)
             | (f->f_data[ 9] << 48)
             | (f->f_data[10] << 40)
             | (f->f_data[11] << 32)
             | (f->f_data[12] << 24)
             | (f->f_data[13] << 16)
             | (f->f_data[14] <<  8)
             | (f->f_data[15] <<  0);

        result.f_value[1] =
               (f->f_data[16] << 56)
             | (f->f_data[17] << 48)
             | (f->f_data[18] << 40)
             | (f->f_data[19] << 32)
             | (f->f_data[20] << 24)
             | (f->f_data[21] << 16)
             | (f->f_data[22] <<  8)
             | (f->f_data[23] <<  0);

        result.f_value[0] =
               (f->f_data[24] << 56)
             | (f->f_data[25] << 48)
             | (f->f_data[26] << 40)
             | (f->f_data[27] << 32)
             | (f->f_data[28] << 24)
             | (f->f_data[29] << 16)
             | (f->f_data[30] <<  8)
             | (f->f_data[31] <<  0);

        return result;

    case struct_type_t::STRUCT_TYPE_UINT512:
        result.f_value[7] =
               (f->f_data[0] << 56)
             | (f->f_data[1] << 48)
             | (f->f_data[2] << 40)
             | (f->f_data[3] << 32)
             | (f->f_data[4] << 24)
             | (f->f_data[5] << 16)
             | (f->f_data[6] <<  8)
             | (f->f_data[7] <<  0);

        result.f_value[6] =
               (f->f_data[ 8] << 56)
             | (f->f_data[ 9] << 48)
             | (f->f_data[10] << 40)
             | (f->f_data[11] << 32)
             | (f->f_data[12] << 24)
             | (f->f_data[13] << 16)
             | (f->f_data[14] <<  8)
             | (f->f_data[15] <<  0);

        result.f_value[5] =
               (f->f_data[16] << 56)
             | (f->f_data[17] << 48)
             | (f->f_data[18] << 40)
             | (f->f_data[19] << 32)
             | (f->f_data[20] << 24)
             | (f->f_data[21] << 16)
             | (f->f_data[22] <<  8)
             | (f->f_data[23] <<  0);

        result.f_value[4] =
               (f->f_data[24] << 56)
             | (f->f_data[25] << 48)
             | (f->f_data[26] << 40)
             | (f->f_data[27] << 32)
             | (f->f_data[28] << 24)
             | (f->f_data[29] << 16)
             | (f->f_data[30] <<  8)
             | (f->f_data[31] <<  0);

        result.f_value[3] =
               (f->f_data[32] << 56)
             | (f->f_data[33] << 48)
             | (f->f_data[34] << 40)
             | (f->f_data[35] << 32)
             | (f->f_data[36] << 24)
             | (f->f_data[37] << 16)
             | (f->f_data[38] <<  8)
             | (f->f_data[39] <<  0);

        result.f_value[2] =
               (f->f_data[40] << 56)
             | (f->f_data[41] << 48)
             | (f->f_data[42] << 40)
             | (f->f_data[43] << 32)
             | (f->f_data[44] << 24)
             | (f->f_data[45] << 16)
             | (f->f_data[46] <<  8)
             | (f->f_data[47] <<  0);

        result.f_value[1] =
               (f->f_data[48] << 56)
             | (f->f_data[49] << 48)
             | (f->f_data[50] << 40)
             | (f->f_data[51] << 32)
             | (f->f_data[52] << 24)
             | (f->f_data[53] << 16)
             | (f->f_data[54] <<  8)
             | (f->f_data[55] <<  0);

        result.f_value[0] =
               (f->f_data[56] << 56)
             | (f->f_data[57] << 48)
             | (f->f_data[58] << 40)
             | (f->f_data[59] << 32)
             | (f->f_data[60] << 24)
             | (f->f_data[61] << 16)
             | (f->f_data[62] <<  8)
             | (f->f_data[63] <<  0);

        return result;

    }

    throw type_mismatch(
              "This description type is \""
            + std::to_string(f->f_description->f_type)
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
            + "\".");
}


void structure::set_large_uinteger(std::string const & field_name, uint512_t value)
{
    auto f(get_field(field_name));

    uint512_t result;
    switch(f->f_description->f_type)
    {
    case struct_type_t::STRUCT_TYPE_BITS8:
    case struct_type_t::STRUCT_TYPE_UINT8:
        f->f_data[0] = result.f_value[0];
        return;

    case struct_type_t::STRUCT_TYPE_BITS16:
    case struct_type_t::STRUCT_TYPE_UINT16:
        f->f_data[0] = result.f_value[0] >> 8;
        f->f_data[1] = result.f_value[0] >> 0;
        return;

    case struct_type_t::STRUCT_TYPE_BITS32:
    case struct_type_t::STRUCT_TYPE_UINT32:
        f->f_data[0] = result.f_value[0] >> 24;
        f->f_data[1] = result.f_value[0] >> 16;
        f->f_data[2] = result.f_value[0] >>  8;
        f->f_data[3] = result.f_value[0] >>  0;
        return;

    case struct_type_t::STRUCT_TYPE_BITS64:
    case struct_type_t::STRUCT_TYPE_UINT64:
    case struct_type_t::STRUCT_TYPE_REFERENCE:
        f->f_data[0] = result.f_value[0] >> 56;
        f->f_data[1] = result.f_value[0] >> 48;
        f->f_data[2] = result.f_value[0] >> 40;
        f->f_data[3] = result.f_value[0] >> 32;
        f->f_data[4] = result.f_value[0] >> 24;
        f->f_data[5] = result.f_value[0] >> 16;
        f->f_data[6] = result.f_value[0] >>  8;
        f->f_data[7] = result.f_value[0] >>  0;
        return;

    case struct_type_t::STRUCT_TYPE_UINT128:
        f->f_data[0] = result.f_value[1] >> 56;
        f->f_data[1] = result.f_value[1] >> 48;
        f->f_data[2] = result.f_value[1] >> 40;
        f->f_data[3] = result.f_value[1] >> 32;
        f->f_data[4] = result.f_value[1] >> 24;
        f->f_data[5] = result.f_value[1] >> 16;
        f->f_data[6] = result.f_value[1] >>  8;
        f->f_data[7] = result.f_value[1] >>  0;

        f->f_data[ 8] = result.f_value[0] >> 56;
        f->f_data[ 9] = result.f_value[0] >> 48;
        f->f_data[10] = result.f_value[0] >> 40;
        f->f_data[11] = result.f_value[0] >> 32;
        f->f_data[12] = result.f_value[0] >> 24;
        f->f_data[13] = result.f_value[0] >> 16;
        f->f_data[14] = result.f_value[0] >>  8;
        f->f_data[15] = result.f_value[0] >>  0;

        return;

    case struct_type_t::STRUCT_TYPE_UINT256:
        f->f_data[0] = result.f_value[3] >> 56;
        f->f_data[1] = result.f_value[3] >> 48;
        f->f_data[2] = result.f_value[3] >> 40;
        f->f_data[3] = result.f_value[3] >> 32;
        f->f_data[4] = result.f_value[3] >> 24;
        f->f_data[5] = result.f_value[3] >> 16;
        f->f_data[6] = result.f_value[3] >>  8;
        f->f_data[7] = result.f_value[3] >>  0;

        f->f_data[ 8] = result.f_value[2] >> 56;
        f->f_data[ 9] = result.f_value[2] >> 48;
        f->f_data[10] = result.f_value[2] >> 40;
        f->f_data[11] = result.f_value[2] >> 32;
        f->f_data[12] = result.f_value[2] >> 24;
        f->f_data[13] = result.f_value[2] >> 16;
        f->f_data[14] = result.f_value[2] >>  8;
        f->f_data[15] = result.f_value[2] >>  0;

        f->f_data[16] = result.f_value[1] >> 56;
        f->f_data[17] = result.f_value[1] >> 48;
        f->f_data[18] = result.f_value[1] >> 40;
        f->f_data[19] = result.f_value[1] >> 32;
        f->f_data[20] = result.f_value[1] >> 24;
        f->f_data[21] = result.f_value[1] >> 16;
        f->f_data[22] = result.f_value[1] >>  8;
        f->f_data[23] = result.f_value[1] >>  0;

        f->f_data[24] = result.f_value[0] >> 56;
        f->f_data[25] = result.f_value[0] >> 48;
        f->f_data[26] = result.f_value[0] >> 40;
        f->f_data[27] = result.f_value[0] >> 32;
        f->f_data[28] = result.f_value[0] >> 24;
        f->f_data[29] = result.f_value[0] >> 16;
        f->f_data[30] = result.f_value[0] >>  8;
        f->f_data[31] = result.f_value[0] >>  0;

        return;

    case struct_type_t::STRUCT_TYPE_UINT512:
        f->f_data[0] = result.f_value[7] >> 56;
        f->f_data[1] = result.f_value[7] >> 48;
        f->f_data[2] = result.f_value[7] >> 40;
        f->f_data[3] = result.f_value[7] >> 32;
        f->f_data[4] = result.f_value[7] >> 24;
        f->f_data[5] = result.f_value[7] >> 16;
        f->f_data[6] = result.f_value[7] >>  8;
        f->f_data[7] = result.f_value[7] >>  0;

        f->f_data[ 8] = result.f_value[6] >> 56;
        f->f_data[ 9] = result.f_value[6] >> 48;
        f->f_data[10] = result.f_value[6] >> 40;
        f->f_data[11] = result.f_value[6] >> 32;
        f->f_data[12] = result.f_value[6] >> 24;
        f->f_data[13] = result.f_value[6] >> 16;
        f->f_data[14] = result.f_value[6] >>  8;
        f->f_data[15] = result.f_value[6] >>  0;

        f->f_data[16] = result.f_value[5] >> 56;
        f->f_data[17] = result.f_value[5] >> 48;
        f->f_data[18] = result.f_value[5] >> 40;
        f->f_data[19] = result.f_value[5] >> 32;
        f->f_data[20] = result.f_value[5] >> 24;
        f->f_data[21] = result.f_value[5] >> 16;
        f->f_data[22] = result.f_value[5] >>  8;
        f->f_data[23] = result.f_value[5] >>  0;

        f->f_data[24] = result.f_value[4] >> 56;
        f->f_data[25] = result.f_value[4] >> 48;
        f->f_data[26] = result.f_value[4] >> 40;
        f->f_data[27] = result.f_value[4] >> 32;
        f->f_data[28] = result.f_value[4] >> 24;
        f->f_data[29] = result.f_value[4] >> 16;
        f->f_data[30] = result.f_value[4] >>  8;
        f->f_data[31] = result.f_value[4] >>  0;

        f->f_data[32] = result.f_value[3] >> 56;
        f->f_data[33] = result.f_value[3] >> 48;
        f->f_data[34] = result.f_value[3] >> 40;
        f->f_data[35] = result.f_value[3] >> 32;
        f->f_data[36] = result.f_value[3] >> 24;
        f->f_data[37] = result.f_value[3] >> 16;
        f->f_data[38] = result.f_value[3] >>  8;
        f->f_data[39] = result.f_value[3] >>  0;

        f->f_data[40] = result.f_value[2] >> 56;
        f->f_data[41] = result.f_value[2] >> 48;
        f->f_data[42] = result.f_value[2] >> 40;
        f->f_data[43] = result.f_value[2] >> 32;
        f->f_data[44] = result.f_value[2] >> 24;
        f->f_data[45] = result.f_value[2] >> 16;
        f->f_data[46] = result.f_value[2] >>  8;
        f->f_data[47] = result.f_value[2] >>  0;

        f->f_data[48] = result.f_value[1] >> 56;
        f->f_data[49] = result.f_value[1] >> 48;
        f->f_data[50] = result.f_value[1] >> 40;
        f->f_data[51] = result.f_value[1] >> 32;
        f->f_data[52] = result.f_value[1] >> 24;
        f->f_data[53] = result.f_value[1] >> 16;
        f->f_data[54] = result.f_value[1] >>  8;
        f->f_data[55] = result.f_value[1] >>  0;

        f->f_data[56] = result.f_value[0] >> 56;
        f->f_data[57] = result.f_value[0] >> 48;
        f->f_data[58] = result.f_value[0] >> 40;
        f->f_data[59] = result.f_value[0] >> 32;
        f->f_data[60] = result.f_value[0] >> 24;
        f->f_data[61] = result.f_value[0] >> 16;
        f->f_data[62] = result.f_value[0] >>  8;
        f->f_data[63] = result.f_value[0] >>  0;

        return;

    }

    throw type_mismatch(
              "This description type is \""
            + std::to_string(f->f_description->f_type)
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
            + "\".");
}


float structure::get_float32(std::string const & field_name) const
{
    auto f(get_field(field_name, struct_type_t::STRUCT_TYPE_FLOAT32));

    float32_uint32_t convert;
    convert.f_int =
           (f->f_data[0] << 24)
         | (f->f_data[1] << 16)
         | (f->f_data[2] <<  8)
         | (f->f_data[3] <<  0);

    return convert.f_float;
}


void structure::set_float32(std::string const & field_name, float value)
{
    auto f(get_field(field_name, struct_type_t::STRUCT_TYPE_FLOAT32));

    float32_uint32_t convert;
    convert.f_float = value;

    f->f_data[0] = convert.f_int >> 24;
    f->f_data[1] = convert.f_int >> 16;
    f->f_data[2] = convert.f_int >>  8;
    f->f_data[3] = convert.f_int >>  0;
}


double structure::get_float64(std::string const & field_name) const
{
    auto f(get_field(field_name, struct_type_t::STRUCT_TYPE_FLOAT64));

    float64_uint64_t convert;
    convert.f_int =
           (f->f_data[0] << 56)
         | (f->f_data[1] << 48)
         | (f->f_data[2] << 40)
         | (f->f_data[3] << 32)
         | (f->f_data[4] << 24)
         | (f->f_data[5] << 16)
         | (f->f_data[6] <<  8)
         | (f->f_data[7] <<  0);

    return convert.f_float;
}


void structure::set_float64(std::string const & field_name, double value)
{
    auto f(get_field(field_name, struct_type_t::STRUCT_TYPE_FLOAT64));

    float64_uint64_t convert;
    convert.f_float = value;

    f->f_data[ 0] = convert.f_int >> 56;
    f->f_data[ 1] = convert.f_int >> 48;
    f->f_data[ 2] = convert.f_int >> 40;
    f->f_data[ 3] = convert.f_int >> 32;
    f->f_data[ 4] = convert.f_int >> 24;
    f->f_data[ 5] = convert.f_int >> 16;
    f->f_data[ 6] = convert.f_int >>  8;
    f->f_data[ 7] = convert.f_int >>  0;
}


long double structure::get_float128(std::string const & field_name) const
{
    auto f(get_field(field_name, struct_type_t::STRUCT_TYPE_FLOAT128));

    float128_uint128_t convert;

    convert.f_int[1] =
           (f->f_data[0] << 56)
         | (f->f_data[1] << 48)
         | (f->f_data[2] << 40)
         | (f->f_data[3] << 32)
         | (f->f_data[4] << 24)
         | (f->f_data[5] << 16)
         | (f->f_data[6] <<  8)
         | (f->f_data[7] <<  0);

    convert.f_int[0] =
           (f->f_data[ 8] << 56)
         | (f->f_data[ 0] << 48)
         | (f->f_data[10] << 40)
         | (f->f_data[11] << 32)
         | (f->f_data[12] << 24)
         | (f->f_data[13] << 16)
         | (f->f_data[14] <<  8)
         | (f->f_data[15] <<  0);

    return convert.f_float;
}


void structure::set_float128(std::string const & field_name, long double value)
{
    auto f(get_field(field_name, struct_type_t::STRUCT_TYPE_FLOAT128));

    float128_uint128_t convert;
    convert.f_float = value;

    f->f_data[ 0] = convert.f_int[1] >> 56;
    f->f_data[ 1] = convert.f_int[1] >> 48;
    f->f_data[ 2] = convert.f_int[1] >> 40;
    f->f_data[ 3] = convert.f_int[1] >> 32;
    f->f_data[ 4] = convert.f_int[1] >> 24;
    f->f_data[ 5] = convert.f_int[1] >> 16;
    f->f_data[ 6] = convert.f_int[1] >>  8;
    f->f_data[ 7] = convert.f_int[1] >>  0;

    f->f_data[ 8] = convert.f_int[0] >> 56;
    f->f_data[ 9] = convert.f_int[0] >> 48;
    f->f_data[10] = convert.f_int[0] >> 40;
    f->f_data[11] = convert.f_int[0] >> 32;
    f->f_data[12] = convert.f_int[0] >> 24;
    f->f_data[13] = convert.f_int[0] >> 16;
    f->f_data[14] = convert.f_int[0] >>  8;
    f->f_data[15] = convert.f_int[0] >>  0;
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

    return std::string(f->f_data + skip, size);
}


void structure::set_string(std::string const & field_name, std::string const & value)
{
    auto f(get_Field(field_name));

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

    return std::string(f->f_data + skip, size);
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


void structure::set_structure(std::string const & field_name, structure const & value)
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


void structure::set_array(std::string const & field_name, structure_vector_t const & value)
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

    return buffer_t(f->f_data, f->f_size);
}


void structure::set_buffer(std::string const & field_name, buffer_t const & value)
{
    
}


void structure::parse()
{
    if(!f_descriptions_by_name.empty())
    {
        // already parsed
        //
        return;
    }

    snap::NOT_USED(parse_descriptions(f_start_offset));
}


uint8_t * structure::parse_description(uint64_t offset)
{
    for(struct_description const * def(f_descriptions);
        def->f_type != struct_type_t::STRUCT_TYPE_END;
        ++def)
    {
        std::string field_name(def->f_field_name);

        field_t::pointer_t f(std::make_shared<field_t>());
        f->f_description = def;
        f->f_offset = offset;
        bool has_sub_defs(false);
        int bit_field(0);
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
            if(f_buffer->count_buffer() != 0)
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
        case struct_type_t::STRUCT_TYPE_VERSION:
            f->f_size = 2;
            if(f_buffer->count_buffer() != 0)
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
            f->f_size = 4;
            if(f_buffer->count_buffer() != 0)
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
            f->f_size = 8;
            if(f_buffer->count_buffer() != 0)
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
            if(f_buffer->count_buffer() != 0)
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
            if(f_buffer->count_buffer() != 0)
            {
                offset += 32;
            }
            break;

        case struct_type_t::STRUCT_TYPE_BITS512:
            bit_field = 512;
        case struct_type_t::STRUCT_TYPE_INT512:
        case struct_type_t::STRUCT_TYPE_UINT512:
            f->f_size = 64;
            if(f_buffer->count_buffer() != 0)
            {
                offset += 64;
            }
            break;

        case struct_type_t::STRUCT_TYPE_CSTRING:
            f->f_size = 1;           // include '\0' in the size
            if(f_buffer->count_buffer() != 0)
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
            if(f_buffer->count_buffer() != 0)
            {
                uint8_t sz;
                f_buffer->pread(&sz, 1, offset);
                f->f_size = sz + 1;
                offset += f->f_size;
            }
            break;

        case struct_type_t::STRUCT_TYPE_P16STRING:
        case struct_type_t::STRUCT_TYPE_BUFFER16:
            if(f_buffer->count_buffer() != 0)
            {
                uint16_t sz;
                f_buffer->pread(&sz, 2, offset);
                f->f_size = (sz << 8) + (sz >> 8) + 2;
                offset += f->f_size;
            }
            break;

        case struct_type_t::STRUCT_TYPE_P32STRING:
        case struct_type_t::STRUCT_TYPE_BUFFER32:
            if(f_buffer->count_buffer() != 0)
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
            if(f_buffer->count_buffer() != 0)
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
            if(f_buffer->count_buffer() != 0)
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
            if(f_buffer->count_buffer() != 0)
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

        if(f_buffer->count_buffer() != 0
        && offset > f_buffer.size())
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
            for(int idx(0); idx < f->f_size; ++idx)
            {
                pointer_t s(std::make_shared<structure>(def->f_sub_description, f_buffer, offset));
                offset = s->parse_descriptions(offset);
                s->f_end_offset = offset;

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

            for(std::string::size_type end(pos), size_t bit_pos(0); ; )
            {
                std::string::size_type start(end + 1);
                if(start >= field_name.size())
                {
                    break;
                }
                end = field_name.find_first_of(":/", start);
                std::int64_t size(1);
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
                                  "The total number of bits used in bit field \""
                                + flag_name
                                + "\" is more than the total allowed of "
                                + std::to_string(bit_field)
                                + ".");
                        }
                    }
                }
                flag_definition bits(flag_name, bit_pos, size);
                f->f_flag_definitions.push_back(bits);
            }

            field_name = field_name.substr(0, pos);
        }

        f_fields_by_name[field_name] = f;
    }

    return data;
}



} // namespace snapdatabase
// vim: ts=4 sw=4 et
