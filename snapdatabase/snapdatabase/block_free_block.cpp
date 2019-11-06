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
#include    "snapdatabase/block_free_block.h"


// last include
//
#include    <snapdev/poison.h>



namespace snapdatabase
{



namespace detail
{
}



// 'FREE'
struct_description_t * g_free_block_description =
{
    define_description(
          FieldName("magic")    // dbtype_t = FREE
        , FieldType(struct_type_t::STRUCT_TYPE_UINT32)
    ),
    define_description(
          FieldName("next_free_block")
        , FieldType(struct_type_t::STRUCT_TYPE_REFERENCE)
    ),
    // the rest are all zeroes
    end_descriptions()
};



block_free_block::block_free_block(dbfile::pointer_t f, file_addr_t offset)
    : block(f, offset)
    , f_structure(g_free_block_description, data(), offset)
{
}


file_addr_t block_free_block::get_next_free_block()
{
    return static_cast<file_addr_t>(f_structure.get_uinteger("next_free_block"));
}


void block_free_block::set_next_free_block(file_addr_t offset)
{
    f_structure.set_uinteger("next_free_block", offset);
}


block::pointer_t block_free_block::allocate_new_block(dbfile::pointer_t f, dbtype_t type)
{
    file_addr_t ptr(0);
    if(f->get_size() == 0)
    {
        switch(type)
        {
        case FILE_TYPE_SNAP_DATABASE_TABLE:
        case FILE_TYPE_EXTERNAL_INDEX:
        case FILE_TYPE_BLOOM_FILTER:
            break;

        default:
            throw snapdatabase_logic_error(
                      "a new file can't be created with type \""
                    + type_to_string(type)
                    + "\".");

        }

        // this is a new file, create 16 `FREE` blocks
        //
        f->append_free_block(0);
        file_addr_t next(f->get_page_size() * 2);
        for(int idx(0); idx < 14; ++idx, next += f->get_page_size())
        {
            f->append_free_block(next);
        }
        f->append_free_block(0);

        // ptr is already 0
    }
    else
    {
        // get next free block from the header
        //

        // WE DON'T HAVE AN f_table HERE AT THIS TIME, THOUGH...
        //
        file_snap_database_table::pointer_t header(f_table->get_block(0));
        ptr = header->get_first_free_block();
        if(ptr == 0)
        {
            ptr = f->append_free_block(0);

            file_addr_t next(ptr + f->get_page_size() * 2);
            for(int idx(0); idx < 14; ++idx, next += f->get_page_size())
            {
                f->append_free_block(next);
            }
            f->append_free_block(0);

            header->set_first_free_block(ptr + f->get_page_size());
        }
        else
        {
            block_free_block::pointer_t p(new block_free_block(f, ptr));
            header->set_first_free_block(p->get_next_free_block());
        }
    }
}



} // namespace snapdatabase
// vim: ts=4 sw=4 et
