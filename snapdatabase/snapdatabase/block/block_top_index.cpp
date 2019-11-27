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


// last include
//
#include    <snapdev/poison.h>



namespace snapdatabase
{


// We don't want to do that because each key would have a size which means
// we'd waste a lot of space
//
//struct_description_t g_index_description[] =
//{
//    define_description(
//          FieldName("pointer")
//        , FieldType(struct_type_t::STRUCT_TYPE_REFERENCE)
//    ),
//    define_description(
//          FieldName("key")
//        , FieldType(struct_type_t::STRUCT_TYPE_BUFFER32)
//    ),
//};



// 'TIDX'
constexpr struct_description_t g_top_index_description[] =
{
    define_description(
          FieldName("magic")    // dbtype_t = TIDX
        , FieldType(struct_type_t::STRUCT_TYPE_UINT32)
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
    //    , FieldType(struct_type_t::STRUCT_TYPE_ARRAY32)
    //    , FieldDescription(g_index_description)
    //),
    end_descriptions()
};




block_top_index::block_top_index(dbfile::pointer_t f, reference_t offset)
    : block(f, offset)
{
    f_structure = std::make_shared<structure>(g_top_index_description);
}


uint32_t block_top_index::get_count() const
{
    return static_cast<uint32_t>(f_structure->get_uinteger("count"));
}


void block_top_index::set_count(uint32_t id)
{
    f_structure->set_uinteger("count", id);
}


uint32_t block_top_index::get_size() const
{
    return static_cast<uint32_t>(f_structure->get_uinteger("size"));
}


void block_top_index::set_size(uint32_t size)
{
    f_structure->set_uinteger("size", size);
}


reference_t block_top_index::find_index(buffer_t key) const
{
    // the start offset is 1 x structure + alignment
    //
    uint32_t offset(f_structure->get_size() + sizeof(reference_t) - 1);
    offset = offset - offset % sizeof(reference_t);

    std::uint8_t const * buffer(data());
    std::uint32_t const count(get_count());
    std::uint32_t const size(get_size());
    int i(0);
    int j(count);
    while(i < j)
    {
        int const p((j - i) / 2 + i);
        uint8_t const * ptr(buffer + p * size);
        int const r(memcmp(ptr + sizeof(reference_t), key.data(), size));
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
            return * reinterpret_cast<reference_t const *>(ptr);
        }
    }

    // TBD: save current position close to point where we can do an insertion

    return 0;
}










} // namespace snapdatabase
// vim: ts=4 sw=4 et
