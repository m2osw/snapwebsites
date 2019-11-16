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
#include    "snapdatabase/block_schema.h"

#include    "snapdatabase/table.h"


// last include
//
#include    <snapdev/poison.h>



namespace snapdatabase
{



// 'SCHM'
constexpr struct_description_t g_block_schema[] =
{
    define_description(
          FieldName("magic")    // dbtype_t = SCHM
        , FieldType(struct_type_t::STRUCT_TYPE_UINT32)
    ),
    define_description(
          FieldName("size")
        , FieldType(struct_type_t::STRUCT_TYPE_UINT32)
    ),
    define_description(
          FieldName("next_schema_block")
        , FieldType(struct_type_t::STRUCT_TYPE_REFERENCE)
    ),
    end_descriptions()
};



block_schema::block_schema(dbfile::pointer_t f, reference_t offset)
    : block(f, offset)
{
    f_structure = std::make_shared<structure>(g_block_schema);
}


std::uint32_t block_schema::get_size()
{
    return static_cast<uint32_t>(f_structure->get_uinteger("size"));
}


void block_schema::set_size(std::uint32_t size)
{
    f_structure->set_uinteger("size", size);
}


reference_t block_schema::get_next_schema_block()
{
    return static_cast<reference_t>(f_structure->get_uinteger("next_schema_block"));
}


void block_schema::set_next_schema_block(reference_t offset)
{
    f_structure->set_uinteger("next_schema_block", offset);
}


virtual_buffer block_schema::get_schema() const
{
    virtual_buffer result;

    reference_t const offset(f_structure->get_size());
    block_schema::pointer_t s(std::static_pointer_cast<block_schema>(const_cast<block_schema *>(this)->shared_from_this()));
    for(;;)
    {
        block::pointer_t block_ptr(s);
        result.add_buffer(block_ptr, offset, s->get_size());
        reference_t next(s->get_next_schema_block());
        if(next == 0)
        {
            return result;
        }

        s = std::static_pointer_cast<block_schema>(get_table()->get_block(next));
        if(s == nullptr)
        {
            throw snapdatabase_logic_error("block_schema::get_schema() failed reading the list of blocks (bad pointer).");
        }
    }
}


void block_schema::set_schema(virtual_buffer & schema)
{
    reference_t const offset(f_structure->get_size());
    uint32_t const size_per_page(get_table()->get_page_size() - offset);

    uint32_t remaining_size(schema.size());
    block_schema * s(this);
    for(uint32_t pos(0);;)
    {
        data_t d(s->data());
        uint32_t const size(std::min(size_per_page, remaining_size));
        schema.pread(d + offset, size, pos);
        s->set_size(size);

    //int      pread(void * buf, uint64_t size, uint64_t offset, bool full = true) const;

        pos += size;
        remaining_size -= size;
        if(remaining_size == 0)
        {
            break;
        }

        reference_t next(s->get_next_schema_block());
        if(next == 0)
        {
            // create a new block and link it
            //
            block_schema::pointer_t new_block(std::static_pointer_cast<block_schema>(get_table()->allocate_new_block(dbtype_t::BLOCK_TYPE_SCHEMA)));
            s->set_next_schema_block(new_block->get_offset());
            s = new_block.get();
        }
        else
        {
            block_schema::pointer_t next_schema(std::static_pointer_cast<block_schema>(get_table()->get_block(next)));
            if(next_schema == nullptr)
            {
                throw snapdatabase_logic_error("block_schema::get_schema() failed reading the list of blocks.");
            }

            s = next_schema.get();
        }
    }
}


} // namespace snapdatabase
// vim: ts=4 sw=4 et
