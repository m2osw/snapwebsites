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
#include    "snapdatabase/block/block_top_index.h"

#include    "snapdatabase/block/block_header.h"


// last include
//
#include    <snapdev/poison.h>



namespace snapdatabase
{


// We don't want to do that because each key would have a size which means
// we'd waste a lot of space; instead we have one size in the header
//
//struct_description_t g_index_description[] =
//{
//    define_description(
//          FieldName("reference")
//        , FieldType(struct_type_t::STRUCT_TYPE_REFERENCE)
//    ),
//    define_description(
//          FieldName("key")
//        , FieldType(struct_type_t::STRUCT_TYPE_BUFFER32)
//    ),
//};



namespace
{



// 'TIDX' -- top index
constexpr struct_description_t g_description[] =
{
    define_description(
          FieldName("header")
        , FieldType(struct_type_t::STRUCT_TYPE_STRUCTURE)
        , FieldSubDescription(detail::g_block_header)
    ),
    define_description(
          FieldName("count")
        , FieldType(struct_type_t::STRUCT_TYPE_UINT32)
    ),
    define_description(
          FieldName("size")
        , FieldType(struct_type_t::STRUCT_TYPE_UINT32)
    ),
    //define_description(
    //      FieldName("indexes")
    //    , FieldType(struct_type_t::STRUCT_TYPE_ARRAY32) -- we use "count" for the size
    //    , FieldDescription(g_index_description)
    //),
    end_descriptions()
};


constexpr descriptions_by_version_t const g_descriptions_by_version[] =
{
    define_description_by_version(
        DescriptionVersion(0, 1),
        DescriptionDescription(g_description)
    ),
    end_descriptions_by_version()
};



}
// no name namespace




block_top_index::block_top_index(dbfile::pointer_t f, reference_t offset)
    : block(g_descriptions_by_version, f, offset)
{
}


std::uint32_t block_top_index::get_count() const
{
    return static_cast<std::uint32_t>(f_structure->get_uinteger("count"));
}


void block_top_index::set_count(std::uint32_t id)
{
    f_structure->set_uinteger("count", id);
}


// IMPORTANT: size includes the entire index (reference_t + index data)
//
std::uint32_t block_top_index::get_size() const
{
    return static_cast<std::uint32_t>(f_structure->get_uinteger("size"));
}


void block_top_index::set_size(std::uint32_t size)
{
    // size can be really anything, we don't try to align anything
    //
    f_structure->set_uinteger("size", size);
}


reference_t block_top_index::find_index(buffer_t key) const
{
    // the start offset is just after the structure
    // no alignment requirements since we use memcmp() and memcpy()
    // and that way the size can be anything
    //
    // WARNING: keep in mind that the number of bytes we save in the
    //          top indexes may be shorter than the number of bytes
    //          in the key and in that case
    //
    //          TBD -- this may be totally wrong, we should be between
    //          this and the next top index entry really which could be
    //          a lot more (?!?)
    //
    std::uint8_t const * buffer(data(f_structure->get_size()));
    std::uint32_t const count(get_count());
    std::uint32_t const size(get_size());
    std::uint32_t const length(std::min(key.size(), size - sizeof(reference_t)));
    std::uint32_t i(0);
    std::uint32_t j(count);
    while(i < j)
    {
        f_position = (j - i) / 2 + i;
        std::uint8_t const * ptr(buffer + f_position * size);
        int const r(memcmp(ptr + sizeof(reference_t), key.data(), length));
        if(r < 0)
        {
            i = f_position + 1;
        }
        else if(r > 0)
        {
            j = f_position;
        }
        else
        {
            reference_t aligned_reference(0);
            memcpy(&aligned_reference, ptr, sizeof(reference_t));
            return aligned_reference;
        }
    }

    // TBD: save current position close to point where we can do an insertion

    return NULL_FILE_ADDR;
}


std::uint32_t block_top_index::get_position() const
{
    return f_position;
}



} // namespace snapdatabase
// vim: ts=4 sw=4 et
