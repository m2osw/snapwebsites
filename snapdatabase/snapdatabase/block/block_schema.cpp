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
#include    "snapdatabase/block/block_schema.h"

#include    "snapdatabase/block/block_header.h"
#include    "snapdatabase/database/table.h"


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



// 'SCHM'
constexpr struct_description_t g_description[] =
{
    define_description(
          FieldName("header")
        , FieldType(struct_type_t::STRUCT_TYPE_STRUCTURE)
        , FieldSubDescription(detail::g_block_header)
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



block_schema::block_schema(dbfile::pointer_t f, reference_t offset)
    : block(g_descriptions_by_version, f, offset)
{
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


virtual_buffer::pointer_t block_schema::get_schema() const
{
    virtual_buffer::pointer_t result(std::make_shared<virtual_buffer>());

    reference_t const offset(f_structure->get_size());
    block_schema::pointer_t s(std::static_pointer_cast<block_schema>(const_cast<block_schema *>(this)->shared_from_this()));
    for(;;)
    {
        result->add_buffer(s, offset, s->get_size());
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


void block_schema::set_schema(virtual_buffer::pointer_t schema)
{
    reference_t const offset(f_structure->get_size());
#ifdef _DEBUG
    if(offset == 0)
    {
        throw snapdatabase_logic_error("the structure of the block_schema block cannot be dynamic.");
    }
#endif
    std::uint32_t const size_per_page(get_table()->get_page_size() - offset);

    std::uint32_t remaining_size(schema->size());
    block_schema * s(this);
    for(std::uint32_t pos(0);;)
    {
        data_t d(s->data());
        std::uint32_t const size(std::min(size_per_page, remaining_size));
        schema->pread(d + offset, size, pos);
        s->set_size(size);

        reference_t next(s->get_next_schema_block());

        pos += size;
        remaining_size -= size;
        if(remaining_size == 0)
        {
            s->set_next_schema_block(NULL_FILE_ADDR);
            s->sync(false);

            // free any "next" block (happens when a schema shrinks)
            //
            while(next != NULL_FILE_ADDR)
            {
                block_schema::pointer_t next_schema(std::static_pointer_cast<block_schema>(get_table()->get_block(next)));
                if(next_schema == nullptr)
                {
                    throw snapdatabase_logic_error(
                              "reading of the next schema block at "
                            + std::to_string(next)
                            + " failed.");
                }
                next = next_schema->get_next_schema_block();
                get_table()->free_block(next_schema, false);
            }

            break;
        }

        if(next == NULL_FILE_ADDR)
        {
            // create a new block and link it
            //
            block_schema::pointer_t new_block(std::static_pointer_cast<block_schema>(get_table()->allocate_new_block(dbtype_t::BLOCK_TYPE_SCHEMA)));
            s->set_next_schema_block(new_block->get_offset());
            s->sync(false);
            s = new_block.get();
        }
        else
        {
            block_schema::pointer_t next_schema(std::static_pointer_cast<block_schema>(get_table()->get_block(next)));
            if(next_schema == nullptr)
            {
                throw snapdatabase_logic_error(
                          "reading of the next schema block at "
                        + std::to_string(next)
                        + " failed.");
            }

            s->sync(false);
            s = next_schema.get();
        }
    }
}


} // namespace snapdatabase
// vim: ts=4 sw=4 et
