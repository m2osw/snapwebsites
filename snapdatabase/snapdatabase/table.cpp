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
#include    "snapdatabase/table.h"

#include    "snapdatabase/context.h"

// all the blocks since we create them here
//
#include    "snapdatabase/block_blob.h"
#include    "snapdatabase/block_data.h"
#include    "snapdatabase/block_entry_index.h"
#include    "snapdatabase/block_free_block.h"
#include    "snapdatabase/block_free_space.h"
#include    "snapdatabase/block_index_pointers.h"
#include    "snapdatabase/block_indirect_index.h"
#include    "snapdatabase/block_secondary_index.h"
#include    "snapdatabase/block_schema.h"
#include    "snapdatabase/block_top_index.h"
#include    "snapdatabase/file_bloom_filter.h"
#include    "snapdatabase/file_external_index.h"
#include    "snapdatabase/file_snap_database_table.h"



// last include
//
#include    <snapdev/poison.h>



namespace snapdatabase
{



namespace detail
{


class table_impl
{
public:
                                                table_impl(
                                                      context * c
                                                    , table * t
                                                    , xml_node::pointer_t x
                                                    , xml_node::map_t complex_types);
                                                table_impl(table_impl const & rhs) = delete;

    table_impl                                  operator = (table_impl const & rhs) = delete;

    void                                        load_extension(xml_node::pointer_t e);

    dbfile::pointer_t                           get_dbfile() const;
    version_t                                   version() const;
    bool                                        is_secure() const;
    bool                                        is_sparse() const;
    std::string                                 name() const;
    model_t                                     model() const;
    column_ids_t                                row_key() const;
    schema_column::pointer_t                    column(std::string const & name) const;
    schema_column::pointer_t                    column(column_id_t id) const;
    schema_column::map_by_id_t                  columns_by_id() const;
    schema_column::map_by_name_t                columns_by_name() const;
    std::string                                 description() const;
    size_t                                      get_size() const;
    size_t                                      get_page_size() const;
    block::pointer_t                            allocate_block(dbtype_t type, reference_t offset);
    block::pointer_t                            get_block(reference_t offset);
    block::pointer_t                            allocate_new_block(dbtype_t type);

private:
    context *                                   f_context = nullptr;
    table *                                     f_table = nullptr;
    schema_table::pointer_t                     f_schema_table = schema_table::pointer_t();
    dbfile::pointer_t                           f_dbfile = dbfile::pointer_t();
    xml_node::map_t                             f_complex_types = xml_node::map_t();
    block::map_t                                f_blocks = block::map_t();
};


table_impl::table_impl(
          context * c
        , table * t
        , xml_node::pointer_t x
        , xml_node::map_t complex_types)
    : f_context(c)
    , f_table(t)
    , f_schema_table(std::make_shared<schema_table>(x))
    , f_dbfile(std::make_shared<dbfile>(c->get_path(), f_schema_table->name(), "main"))
    , f_complex_types(complex_types)
{

    // open the main database file
    //

    // the schema found in the XML file is the authoritative one
    // load the one from the table and if different, apply the
    // changes
    //
    auto current_schema(f_schema_table);

}


void table_impl::load_extension(xml_node::pointer_t e)
{
    f_schema_table->load_extension(e);
}


dbfile::pointer_t table_impl::get_dbfile() const
{
    return f_dbfile;
}


version_t table_impl::version() const
{
    return f_schema_table->version();
}


bool table_impl::is_secure() const
{
    return f_schema_table->is_secure();
}


bool table_impl::is_sparse() const
{
    return f_schema_table->is_sparse();
}


std::string table_impl::name() const
{
    return f_schema_table->name();
}


model_t table_impl::model() const
{
    return f_schema_table->model();
}


column_ids_t table_impl::row_key() const
{
    return f_schema_table->row_key();
}


schema_column::pointer_t table_impl::column(std::string const & name) const
{
    return f_schema_table->column(name);
}


schema_column::pointer_t table_impl::column(column_id_t id) const
{
    return f_schema_table->column(id);
}


schema_column::map_by_id_t table_impl::columns_by_id() const
{
    return f_schema_table->columns_by_id();
}


schema_column::map_by_name_t table_impl::columns_by_name() const
{
    return f_schema_table->columns_by_name();
}


std::string table_impl::description() const
{
    return f_schema_table->description();
}


size_t table_impl::get_size() const
{
    return f_dbfile->get_size();
}


size_t table_impl::get_page_size() const
{
    return f_dbfile->get_page_size();
}


block::pointer_t table_impl::allocate_block(dbtype_t type, reference_t offset)
{
    block::pointer_t b;
    switch(type)
    {
    case dbtype_t::FILE_TYPE_SNAP_DATABASE_TABLE:
        b = std::make_shared<file_snap_database_table>(f_dbfile, offset);
        break;

    case dbtype_t::FILE_TYPE_EXTERNAL_INDEX:
        b = std::make_shared<file_external_index>(f_dbfile, offset);
        break;

    case dbtype_t::FILE_TYPE_BLOOM_FILTER:
        b = std::make_shared<file_bloom_filter>(f_dbfile, offset);
        break;

    case dbtype_t::BLOCK_TYPE_BLOB:
        b = std::make_shared<block_blob>(f_dbfile, offset);
        break;

    case dbtype_t::BLOCK_TYPE_DATA:
        b = std::make_shared<block_data>(f_dbfile, offset);
        break;

    case dbtype_t::BLOCK_TYPE_ENTRY_INDEX:
        b = std::make_shared<block_entry_index>(f_dbfile, offset);
        break;

    case dbtype_t::BLOCK_TYPE_FREE_BLOCK:
        throw snapdatabase_logic_error("You can't allocate a Free Block with allocate_new_block()");

    case dbtype_t::BLOCK_TYPE_FREE_SPACE:
        b = std::make_shared<block_free_space>(f_dbfile, offset);
        break;

    case dbtype_t::BLOCK_TYPE_INDEX_POINTERS:
        b = std::make_shared<block_index_pointers>(f_dbfile, offset);
        break;

    case dbtype_t::BLOCK_TYPE_INDIRECT_INDEX:
        b = std::make_shared<block_indirect_index>(f_dbfile, offset);
        break;

    case dbtype_t::BLOCK_TYPE_SECONDARY_INDEX:
        b = std::make_shared<block_secondary_index>(f_dbfile, offset);
        break;

    case dbtype_t::BLOCK_TYPE_SCHEMA:
        b = std::make_shared<block_schema>(f_dbfile, offset);
        break;

    case dbtype_t::BLOCK_TYPE_TOP_INDEX:
        b = std::make_shared<block_top_index>(f_dbfile, offset);
        break;

    default:
        throw snapdatabase_logic_error(
                  "allocate_new_block() called with an unknown dbtype_t value ("
                + std::to_string(static_cast<int>(type))
                + ").");

    }

    b->set_dbtype(type);
    b->set_table(f_table->get_pointer());
    b->get_structure()->set_block(b, f_dbfile->get_page_size());

    return b;
}


block::pointer_t table_impl::get_block(reference_t offset)
{
    auto it(f_blocks.find(offset));
    if(it != f_blocks.end())
    {
        return it->second;
    }

    if(offset >= f_dbfile->get_size())
    {
        throw snapdatabase_logic_error("Requested a block with an offset >= to the existing file size.");
    }

    data_t d(f_dbfile->data(offset));
    dbtype_t type(*reinterpret_cast<dbtype_t *>(d));

    block::pointer_t b(allocate_block(type, offset));

    f_blocks[offset] = b;

    f_context->limit_allocated_memory();

    return b;
}


block::pointer_t table_impl::allocate_new_block(dbtype_t type)
{
    if(type == dbtype_t::BLOCK_TYPE_FREE_BLOCK)
    {
        throw snapdatabase_logic_error("You can't allocate a Free Block with allocate_new_block().");
    }

    reference_t offset(0);
    if(f_dbfile->get_size() == 0)
    {
        switch(type)
        {
        case dbtype_t::FILE_TYPE_SNAP_DATABASE_TABLE:
        case dbtype_t::FILE_TYPE_EXTERNAL_INDEX:
        case dbtype_t::FILE_TYPE_BLOOM_FILTER:
            break;

        default:
            throw snapdatabase_logic_error(
                      "a new file can't be created with type \""
                    + dbtype_to_string(type)
                    + "\".");

        }

        // this is a new file, create 16 `FREE` blocks
        //
        f_dbfile->append_free_block(0);
        size_t const page_size(f_dbfile->get_page_size());
        reference_t next(page_size * 2);
        for(int idx(0); idx < 14; ++idx, next += page_size)
        {
            f_dbfile->append_free_block(next);
        }
        f_dbfile->append_free_block(0);

        // offset is already 0
    }
    else
    {
        // get next free block from the header
        //
        file_snap_database_table::pointer_t header(std::static_pointer_cast<file_snap_database_table>(get_block(0)));
        offset = header->get_first_free_block();
        if(offset == 0)
        {
            offset = f_dbfile->append_free_block(0);

            size_t const page_size(f_dbfile->get_page_size());
            reference_t next(offset + page_size * 2);
            for(int idx(0); idx < 14; ++idx, next += page_size)
            {
                f_dbfile->append_free_block(next);
            }
            f_dbfile->append_free_block(0);

            header->set_first_free_block(offset + page_size);
        }
        else
        {
            block_free_block::pointer_t p(std::make_shared<block_free_block>(f_dbfile, offset));
            header->set_first_free_block(p->get_next_free_block());
        }
    }

    // this should probably use a factory for better extendability
    // but at this time we don't need such at all
    //
    block::pointer_t b(allocate_block(type, offset));
    f_blocks[offset] = b;

    f_context->limit_allocated_memory();

    return b;
}





} // namespace detail



table::table(
          context * c
        , xml_node::pointer_t x
        , xml_node::map_t complex_types)
    : f_impl(std::make_shared<detail::table_impl>(c, this, x, complex_types))
{
}


table::pointer_t table::get_pointer()
{
    return shared_from_this();
}


void table::load_extension(xml_node::pointer_t e)
{
    f_impl->load_extension(e);
}


dbfile::pointer_t table::get_dbfile() const
{
    return f_impl->get_dbfile();
}


version_t table::version() const
{
    return f_impl->version();
}


std::string table::name() const
{
    return f_impl->name();
}


model_t table::model() const
{
    return f_impl->model();
}


column_ids_t table::row_key() const
{
    return f_impl->row_key();
}


schema_column::pointer_t table::column(std::string const & name) const
{
    return f_impl->column(name);
}


schema_column::pointer_t table::column(column_id_t id) const
{
    return f_impl->column(id);
}


schema_column::map_by_id_t table::columns_by_id() const
{
    return f_impl->columns_by_id();
}


schema_column::map_by_name_t table::columns_by_name() const
{
    return f_impl->columns_by_name();
}


bool table::is_secure() const
{
    return f_impl->is_secure();
}


bool table::is_sparse() const
{
    return f_impl->is_sparse();
}


std::string table::description() const
{
    return f_impl->description();
}


size_t table::get_size() const
{
    return f_impl->get_size();
}


size_t table::get_page_size() const
{
    return f_impl->get_page_size();
}


block::pointer_t table::get_block(reference_t offset)
{
    return f_impl->get_block(offset);
}


block::pointer_t table::allocate_new_block(dbtype_t type)
{
    return f_impl->allocate_new_block(type);
}



} // namespace snapdatabase
// vim: ts=4 sw=4 et
