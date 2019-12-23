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
#include    "snapdatabase/database/table.h"

#include    "snapdatabase/database/context.h"

// all the blocks since we create them here
//
#include    "snapdatabase/block/block_blob.h"
#include    "snapdatabase/block/block_data.h"
#include    "snapdatabase/block/block_entry_index.h"
#include    "snapdatabase/block/block_free_block.h"
#include    "snapdatabase/block/block_free_space.h"
#include    "snapdatabase/block/block_header.h"
#include    "snapdatabase/block/block_index_pointers.h"
#include    "snapdatabase/block/block_indirect_index.h"
#include    "snapdatabase/block/block_secondary_index.h"
#include    "snapdatabase/block/block_schema.h"
#include    "snapdatabase/block/block_top_index.h"
#include    "snapdatabase/file/file_bloom_filter.h"
#include    "snapdatabase/file/file_external_index.h"
#include    "snapdatabase/file/file_snap_database_table.h"


// C++ lib
//
#include    <iostream>


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
                                                    , schema_complex_type::map_pointer_t complex_types);
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
    block::pointer_t                            get_block(reference_t offset);
    block::pointer_t                            allocate_new_block(dbtype_t type);
    void                                        free_block(block::pointer_t block, bool clear_block);
    bool                                        verify_schema();

private:
    block::pointer_t                            allocate_block(dbtype_t type, reference_t offset);

    context *                                   f_context = nullptr;
    table *                                     f_table = nullptr;
    schema_table::pointer_t                     f_schema_table = schema_table::pointer_t();
    dbfile::pointer_t                           f_dbfile = dbfile::pointer_t();
    block::map_t                                f_blocks = block::map_t();
};


table_impl::table_impl(
          context * c
        , table * t
        , xml_node::pointer_t x
        , schema_complex_type::map_pointer_t complex_types)
    : f_context(c)
    , f_table(t)
    , f_schema_table(std::make_shared<schema_table>())
{
    f_schema_table->set_complex_types(complex_types);
    f_schema_table->from_xml(x);
    f_dbfile = std::make_shared<dbfile>(c->get_path(), f_schema_table->name(), "main");
    f_dbfile->set_page_size(f_schema_table->block_size());
}


void table_impl::load_extension(xml_node::pointer_t e)
{
    f_schema_table->load_extension(e);
}


dbfile::pointer_t table_impl::get_dbfile() const
{
    return f_dbfile;
}


bool table_impl::verify_schema()
{
    // load schema from dbfile (if present) and then compare against schema
    // from XML; if different increase version, save new version in file and
    // start process to update the table existing data; once the update is
    // done, we can remove the old schema from the file (i.e. any new writes
    // always use the newest version of the schema; the rows that use an
    // older version, use that specific schema until they get updated to the
    // new format.)
    //
    file_snap_database_table::pointer_t sdbt(
            std::static_pointer_cast<file_snap_database_table>(
                        get_block(0)));

    reference_t const schema_offset(sdbt->get_table_definition());
    if(schema_offset == 0)
    {
        // no schema defined yet, just save ours and we're all good
        //
        f_schema_table->assign_column_ids();

        block_schema::pointer_t schm(
                std::static_pointer_cast<block_schema>(
                        allocate_new_block(dbtype_t::BLOCK_TYPE_SCHEMA)));
        virtual_buffer::pointer_t bin_schema(f_schema_table->to_binary());
        schm->set_schema(bin_schema);

        sdbt->set_table_definition(schm->get_offset());
        sdbt->sync(true);
    }
    else
    {
        // load the binary schema (it may reside on multiple blocks and we
        // have to read the entire schema at once)
        //
        block_schema::pointer_t schm(std::static_pointer_cast<block_schema>(get_block(schema_offset)));
        virtual_buffer::pointer_t current_schema_data(schm->get_schema());
        schema_table::pointer_t current_schema_table(std::make_shared<schema_table>());
        current_schema_table->from_binary(current_schema_data);

        f_schema_table->assign_column_ids(current_schema_table);

        compare_t const c(current_schema_table->compare(*f_schema_table));
        if(c == compare_t::COMPARE_SCHEMA_UPDATE)
        {
            virtual_buffer::pointer_t bin_schema(f_schema_table->to_binary());
            schm->set_schema(bin_schema);
        }
        else if(c == compare_t::COMPARE_SCHEMA_DIFFER)
        {
std::cerr << "table: TODO: schemata differ...\n";
throw snapdatabase_not_yet_implemented("differing schemata not handled yet");
        }
        // else -- this table schema did not change
    }

    return true;
}


version_t table_impl::version() const
{
    return f_schema_table->schema_version();
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
    auto it(f_blocks.find(offset));
    if(it != f_blocks.end())
    {
        if(type == it->second->get_dbtype())
        {
            return it->second;
        }
        // TBD: I think only FREE blocks can be replaced by something else
        //      and vice versa or we've got a bug on our hands
        //
        if(type != dbtype_t::BLOCK_TYPE_FREE_BLOCK
        && it->second->get_dbtype() != dbtype_t::BLOCK_TYPE_FREE_BLOCK)
        {
            throw snapdatabase_logic_error(
                      "allocate_block() called a non-free block type trying to allocate a non-free block ("
                    + std::to_string(static_cast<int>(type))
                    + "). You can go from a free to non-free and non-free to free, but not the opposite.");
        }
        //it->second->replacing(); -- this won't work right at this time TODO...
        f_blocks.erase(it);
    }

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
        //throw snapdatabase_logic_error("You can't allocate a Free Block with allocate_block()");
        b = std::make_shared<block_free_block>(f_dbfile, offset);
        break;

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
                  "allocate_block() called with an unknown dbtype_t value ("
                + std::to_string(static_cast<int>(type))
                + ").");

    }

    b->set_table(f_table->get_pointer());
    b->set_data(f_dbfile->data(offset));
    b->get_structure()->set_block(b, 0, f_dbfile->get_page_size());
    b->set_dbtype(type);

    f_context->limit_allocated_memory();

    // we add this block to the list of blocks only after the call to
    // limit the allocated memory
    //
    f_blocks[offset] = b;

    return b;
}


block::pointer_t table_impl::get_block(reference_t offset)
{
    if(offset != 0
    && offset >= f_dbfile->get_size())
    {
        throw snapdatabase_logic_error("Requested a block with an offset >= to the existing file size.");
    }

    structure::pointer_t s(std::make_shared<structure>(g_block_header));
    data_t d(f_dbfile->data(offset));
    virtual_buffer::pointer_t header(std::make_shared<virtual_buffer>());
#ifdef _DEBUG
    if(s->get_size() != BLOCK_HEADER_SIZE)
    {
        throw snapdatabase_logic_error("sizeof(g_block_header) != BLOCK_HEADER_SIZE");
    }
#endif
    header->pwrite(d, s->get_size(), 0, true);
    s->set_virtual_buffer(header, 0);
    dbtype_t const type(static_cast<dbtype_t>(s->get_uinteger("magic")));
    //version_t const version(s->get_uinteger("version"));

    block::pointer_t b(allocate_block(type, offset));

    // this last call is used to convert the binary data from the
    // file version to the latest running version; the result will
    // be saved back in the block so that way the conversion doesn't
    // happen over and over again; if the version is already up to
    // date, then nothing happens
    //
    b->from_current_file_version();

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
                    + to_string(type)
                    + "\".");

        }

        // this is a new file, create 16 `FREE` blocks
        //
        f_dbfile->append_free_block(NULL_FILE_ADDR);
        size_t const page_size(f_dbfile->get_page_size());
        reference_t next(page_size * 2);
        for(int idx(0); idx < 14; ++idx, next += page_size)
        {
            f_dbfile->append_free_block(next);
        }
        f_dbfile->append_free_block(NULL_FILE_ADDR);

        // offset is already 0
    }
    else
    {
        switch(type)
        {
        case dbtype_t::FILE_TYPE_SNAP_DATABASE_TABLE:
        case dbtype_t::FILE_TYPE_EXTERNAL_INDEX:
        case dbtype_t::FILE_TYPE_BLOOM_FILTER:
            throw snapdatabase_logic_error(
                      "a file type such as \""
                    + to_string(type)
                    + "\" is only for when you create a file.");

        default:
            break;

        }

        // get next free block from the header
        //
        file_snap_database_table::pointer_t header(std::static_pointer_cast<file_snap_database_table>(get_block(0)));
        offset = header->get_first_free_block();
        if(offset == NULL_FILE_ADDR)
        {
            offset = f_dbfile->append_free_block(NULL_FILE_ADDR);

            size_t const page_size(f_dbfile->get_page_size());
            reference_t next(offset + page_size * 2);
            for(int idx(0); idx < 14; ++idx, next += page_size)
            {
                f_dbfile->append_free_block(next);
            }
            f_dbfile->append_free_block(NULL_FILE_ADDR);

            header->set_first_free_block(offset + page_size);
        }
        else
        {
            block_free_block::pointer_t p(std::static_pointer_cast<block_free_block>(get_block(offset)));
            header->set_first_free_block(p->get_next_free_block());
        }
    }

    // this should probably use a factory for better extendability
    // but at this time we don't need such at all
    //
    block::pointer_t b(allocate_block(type, offset));
    b->set_structure_version();
    return b;
}


void table_impl::free_block(block::pointer_t block, bool clear_block)
{
    if(block == nullptr)
    {
        return;
    }

    reference_t const offset(block->get_offset());
    block_free_block::pointer_t p(std::static_pointer_cast<block_free_block>(
            allocate_block(dbtype_t::BLOCK_TYPE_FREE_BLOCK, offset)));

    if(clear_block)
    {
        p->clear_block();
    }

    file_snap_database_table::pointer_t header(std::static_pointer_cast<file_snap_database_table>(get_block(0)));
    reference_t const next_offset(header->get_first_free_block());
    p->set_next_free_block(next_offset);
    header->set_first_free_block(offset);
}





} // namespace detail



table::table(
          context * c
        , xml_node::pointer_t x
        , schema_complex_type::map_pointer_t complex_types)
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


void table::free_block(block::pointer_t block, bool clear_block)
{
    return f_impl->free_block(block, clear_block);
}


bool table::verify_schema()
{
    return f_impl->verify_schema();
}


} // namespace snapdatabase
// vim: ts=4 sw=4 et
