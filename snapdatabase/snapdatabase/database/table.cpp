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
#include    "snapdatabase/database/row.h"

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
#include    "snapdatabase/block/block_primary_index.h"
#include    "snapdatabase/block/block_secondary_index.h"
#include    "snapdatabase/block/block_schema.h"
#include    "snapdatabase/block/block_schema_list.h"
#include    "snapdatabase/block/block_top_index.h"
#include    "snapdatabase/block/block_top_indirect_index.h"
#include    "snapdatabase/file/file_bloom_filter.h"
#include    "snapdatabase/file/file_external_index.h"
#include    "snapdatabase/file/file_snap_database_table.h"


// snapwebsites lib
//
#include    <snapwebsites/snap_child.h>


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



namespace detail
{






class cursor_state
{
public:
    typedef std::shared_ptr<cursor_state>   pointer_t;

    struct index_reference_t
    {
        typedef std::vector<index_reference_t>
                                            vector_t;

        reference_t                         f_row_reference = NULL_FILE_ADDR;
        std::uint32_t                       f_index_position = 0;       // position within the index at end of a read
    };

                                        cursor_state(index_type_t index_type, schema_secondary_index::pointer_t secondary_index);

    index_type_t                        get_index_type() const;
    schema_secondary_index::pointer_t   get_secondary_index() const;

    index_reference_t::vector_t const & get_index_references() const;
    void                                add_index_reference(index_reference_t const & position);
    block_entry_index::pointer_t        get_entry_index() const;
    void                                set_entry_index(block_entry_index::pointer_t entry_index);
    std::uint32_t                       get_entry_index_close_position() const;
    void                                set_entry_index_close_position(std::uint32_t position);

private:
    index_type_t                        f_index_type = index_type_t::INDEX_TYPE_INVALID;
    schema_secondary_index::pointer_t   f_secondary_index = schema_secondary_index::pointer_t();
    index_reference_t::vector_t         f_row_references = index_reference_t::vector_t();
    block_entry_index::pointer_t        f_entry_index = block_entry_index::pointer_t();
    std::uint32_t                       f_entry_index_position = std::uint32_t(0);
};


cursor_state::cursor_state(index_type_t index_type, schema_secondary_index::pointer_t secondary_index)
    : f_index_type(index_type)
    , f_secondary_index(secondary_index)
{
}


index_type_t cursor_state::get_index_type() const
{
    return f_index_type;
}


schema_secondary_index::pointer_t cursor_state::get_secondary_index() const
{
    return f_secondary_index;
}


cursor_state::index_reference_t::vector_t const & cursor_state::get_index_references() const
{
    return f_row_references;
}


void cursor_state::add_index_reference(index_reference_t const & position)
{
    f_row_references.push_back(position);
}


block_entry_index::pointer_t cursor_state::get_entry_index() const
{
    return f_entry_index;
}


void cursor_state::set_entry_index(block_entry_index::pointer_t entry_index)
{
    f_entry_index = entry_index;
}


std::uint32_t cursor_state::get_entry_index_close_position() const
{
    return f_entry_index_position;
}


void cursor_state::set_entry_index_close_position(std::uint32_t position)
{
    f_entry_index_position = position;
}






struct cursor_data
{
                                        cursor_data(
                                                  cursor::pointer_t cursor
                                                , cursor_state::pointer_t state
                                                , row::vector_t & rows);

    cursor::pointer_t                   f_cursor;
    cursor_state::pointer_t             f_state;
    row::vector_t &                     f_rows;
};


cursor_data::cursor_data(
          cursor::pointer_t cursor
        , cursor_state::pointer_t state
        , row::vector_t & rows)
    : f_cursor(cursor)
    , f_state(state)
    , f_rows(rows)
{
}







enum class commit_mode_t
{
    COMMIT_MODE_COMMIT,        // insert or update, fails only on errors
    COMMIT_MODE_INSERT,        // fails if it already exists
    COMMIT_MODE_UPDATE         // fails if it doesn't exists yet
};



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
    version_t                                   schema_version() const;
    bool                                        is_sparse() const;
    bool                                        is_secure() const;
    std::string                                 name() const;
    model_t                                     model() const;
    column_ids_t                                row_key() const;
    schema_column::pointer_t                    column(std::string const & name, version_t const & version) const;
    schema_column::pointer_t                    column(column_id_t id, version_t const & version) const;
    schema_column::map_by_id_t                  columns_by_id(version_t const & version) const;
    schema_column::map_by_name_t                columns_by_name(version_t const & version) const;
    std::string                                 description() const;
    size_t                                      get_size() const;
    size_t                                      get_page_size() const;
    block::pointer_t                            get_block(reference_t offset);
    block::pointer_t                            allocate_new_block(dbtype_t type);
    void                                        free_block(block::pointer_t block, bool clear_block);
    schema_table::pointer_t                     get_schema(version_t const & version);
    schema_secondary_index::pointer_t           secondary_index(std::string const & name) const;
    bool                                        row_commit(row_pointer_t row, commit_mode_t mode);
    void                                        row_insert(row::pointer_t row_data, cursor::pointer_t cur);
    void                                        row_update(row::pointer_t row_data, cursor::pointer_t cur);
    block_primary_index::pointer_t              get_primary_index_block(bool create);
    void                                        read_rows(cursor_data & data);

private:
    block::pointer_t                            allocate_block(dbtype_t type, reference_t offset);
    void                                        start_update_process(bool restart);
    reference_t                                 get_indirect_reference(oid_t oid);
    row::pointer_t                              get_indirect_row(oid_t oid);
    row::pointer_t                              get_row(reference_t row_reference);

    void                                        read_secondary(cursor_data & data);
    void                                        read_indirect(cursor_data & data);
    void                                        read_primary(cursor_data & data);
    void                                        read_expiration(cursor_data & data);
    void                                        read_tree(cursor_data & data);

    context *                                   f_context = nullptr;
    table *                                     f_table = nullptr;
    schema_table::pointer_t                     f_schema_table = schema_table::pointer_t();
    schema_table::map_by_version_t              f_schema_table_by_version = schema_table::map_by_version_t();
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


schema_table::pointer_t table_impl::get_schema(version_t const & version)
{
    // the very first time `get_schema()` is called, `version` must be
    // set to `0.0` (a.k.a. `version_t()`) which is how the latest schema
    // gets added to the table if necessary
    //
    if(f_schema_table_by_version.empty()
    && version != version_t())
    {
        throw snapdatabase_logic_error(
                    "table_impl::get_schema() called with a version other"
                    " than 0.0 when the table is not properly setup yet.");
    }

    // it's very costly to load a schema so we cache them
    // they are read-only anyway so they are never going to change
    // once loaded in our cache
    //
    auto const it(f_schema_table_by_version.find(version.to_binary()));
    if(it != f_schema_table_by_version.end())
    {
        return it->second;
    }

    // check for an existing schema in the table file
    //
    file_snap_database_table::pointer_t sdbt(
            std::static_pointer_cast<file_snap_database_table>(
                        get_block(0)));
    reference_t schema_offset(sdbt->get_table_definition());
    if(schema_offset == NULL_FILE_ADDR)
    {
        if(version != version_t()
        || !f_schema_table_by_version.empty())
        {
            // this is probably a logic error
            //
            throw schema_not_found(
                      "get_schema() did not find any schema; so definitely no schema with version "
                    + version.to_string()
                    + ".");
        }

        // no schema defined yet, just save ours and we're all good
        //
        // note that the version is 1.0 by default and we do not have
        // to change it in this case (it is the expected version)
        //
        f_schema_table->assign_column_ids();

        block_schema::pointer_t schm(
                std::static_pointer_cast<block_schema>(
                        allocate_new_block(dbtype_t::BLOCK_TYPE_SCHEMA)));
        virtual_buffer::pointer_t bin_schema(f_schema_table->to_binary());
        schm->set_schema(bin_schema);

        sdbt->set_table_definition(schm->get_offset());
        sdbt->sync(true);

        f_schema_table_by_version[f_schema_table->schema_version().to_binary()] = f_schema_table;

        return f_schema_table;
    }

    // there is at least one schema, load it
    //
    block_schema_list::pointer_t schl;
    block::pointer_t block(get_block(schema_offset));
    if(block->get_dbtype() == dbtype_t::BLOCK_TYPE_SCHEMA_LIST)
    {
        // we have a list of schemata in this table, search for the one
        // with `version`; if `version` is `0.0` then the very first one
        // will be returned (i.e. the current one)
        //
        schl = std::static_pointer_cast<block_schema_list>(block);
        schema_offset = schl->get_schema(version);
        if(schema_offset == NULL_FILE_ADDR)
        {
            throw schema_not_found(
                      "get_schema() did not find a schema with version "
                    + version.to_string()
                    + ".");
        }
        block = get_block(schema_offset);
    }

    block_schema::pointer_t schm(std::static_pointer_cast<block_schema>(block));
    virtual_buffer::pointer_t schema_data(schm->get_schema());
    schema_table::pointer_t schema(std::make_shared<schema_table>());
    schema->from_binary(schema_data);
    schema->schema_offset(schema_offset);

    if(f_schema_table_by_version.empty())
    {
        if(version != version_t())
        {
            // this is probably a logic error since we should not be
            // here if the `version` parameter is not `0.0`
            //
            throw schema_not_found(
                      "schema version "
                    + version.to_string()
                    + " not found.");
        }

        // still empty which means it's the first call and we need to
        // compare `f_schema_table` with `schema` to see whether it is
        // the same or not, if not, we want to add the `f_schema_table`
        // and start the background process to update the table schema
        //
        f_schema_table->assign_column_ids(schema);

        bool restart(false);
        compare_t const c(schema->compare(*f_schema_table));
        if(c == compare_t::COMPARE_SCHEMA_UPDATE)
        {
            // this is a simple update (i.e. the description changed and
            // we do not have to update all the rows)
            //
            virtual_buffer::pointer_t bin_schema(f_schema_table->to_binary());
            schm->set_schema(bin_schema);
        }
        else if(c == compare_t::COMPARE_SCHEMA_DIFFER)
        {
            if(schl == nullptr)
            {
                // create a BlockSchemaList
                //
                schl = std::static_pointer_cast<block_schema_list>(
                                allocate_new_block(dbtype_t::BLOCK_TYPE_SCHEMA_LIST));
                schl->add_schema(schema);

                sdbt->set_table_definition(schl->get_offset());
                sdbt->sync(true);
            }

            schm = std::static_pointer_cast<block_schema>(
                            allocate_new_block(dbtype_t::BLOCK_TYPE_SCHEMA));
            virtual_buffer::pointer_t bin_schema(f_schema_table->to_binary());
            schm->set_schema(bin_schema);
            f_schema_table->schema_offset(schm->get_offset());

            schl->add_schema(f_schema_table);
            schl->sync(true);
            restart = true;

            // `schema` is different from `f_schema_table` so cache it too
            //
            f_schema_table_by_version[schema->schema_version().to_binary()] = schema;
        }
        // else -- this table schema did not change

        f_schema_table_by_version[f_schema_table->schema_version().to_binary()] = f_schema_table;

        if(schl != nullptr)
        {
            start_update_process(restart);
        }

        return f_schema_table;
    }

    f_schema_table_by_version[schema->schema_version().to_binary()] = schema;
    return schema;
}


schema_secondary_index::pointer_t table_impl::secondary_index(std::string const & name) const
{
    return f_schema_table->secondary_index(name);
}


version_t table_impl::schema_version() const
{
    return f_schema_table->schema_version();
}


bool table_impl::is_sparse() const
{
    return f_schema_table->is_sparse();
}


bool table_impl::is_secure() const
{
    return f_schema_table->is_secure();
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


schema_column::pointer_t table_impl::column(std::string const & name, version_t const & version) const
{
    return const_cast<table_impl *>(this)->get_schema(version)->column(name);
}


schema_column::pointer_t table_impl::column(column_id_t id, version_t const & version) const
{
    return const_cast<table_impl *>(this)->get_schema(version)->column(id);
}


schema_column::map_by_id_t table_impl::columns_by_id(version_t const & version) const
{
    return const_cast<table_impl *>(this)->get_schema(version)->columns_by_id();
}


schema_column::map_by_name_t table_impl::columns_by_name(version_t const & version) const
{
    return const_cast<table_impl *>(this)->get_schema(version)->columns_by_name();
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
                    + to_string(type)
                    + "). You can go from a free to non-free and non-free to free only.");
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

    case dbtype_t::BLOCK_TYPE_PRIMARY_INDEX:
        b = std::make_shared<block_primary_index>(f_dbfile, offset);
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

    case dbtype_t::BLOCK_TYPE_TOP_INDIRECT_INDEX:
        b = std::make_shared<block_top_indirect_index>(f_dbfile, offset);
        break;

    default:
        throw snapdatabase_logic_error(
                  "allocate_block() called with an unknown dbtype_t value ("
                + to_string(type)
                + ").");

    }

    b->set_table(f_table->get_pointer());
    b->set_data(f_dbfile->data(offset));
    b->get_structure()->set_block(b, 0, get_page_size());
    b->set_dbtype(type);

    f_context->limit_allocated_memory();

    // we add this block to the list of blocks only after the call to
    // limit the allocated memory
    //
    f_blocks[offset] = b;

    return b;
}


/** \brief Process the database to update to the latest schema.
 *
 * One big problem with databases is to update their schema. In our
 * system, you can update the schema _at any time_ and continue to
 * run as if nothing had happened (that is, the update itself is
 * close to instantaneous).
 *
 * The update process happens dynamically and using this background
 * update process. The dynamic part happens because when reading a
 * row, we auto-update it to the latest version. So any future SELECT
 * and UPDATE will automatically see the new schema.
 *
 * The background update process actually makes use of the dynamic
 * update by doing a `SELECT * FROM \<table>` to read the entire
 * table once, but without a `LOCK` a standard system would impose.
 * (this runs in the background with the lowest possible priority
 * so it does not take any time).
 *
 * The process can be stopped when the database stops. It will
 * automatically restart when the database is brought back up.
 *
 * The update process algorithm goes like this:
 *
 * 1. set `update_last_oid` to `last_oid`
 * 2. set `update_oid` to 1
 * 3. read row at `update_oid`
 * 4. increment `update_oid`
 * 5. if `update_oid < update_last_iod` go to (3)
 * 6. remove the BlockSchemaList
 *
 * Note that the rows get automatically fixed as we read them, so
 * reading a row (as in (3) above) is enough to fix it. Saving the
 * current `last_iod` in `update_last_iod` allows us to avoid having
 * to check new rows that anyway were created with the newer schema.
 *
 * Step (6) is our signal that the process is done. i.e. when we still
 * have a BlockSchemaList block on a restart of the database system,
 * we call the start_update_process() to finish up any previous updates
 * (or restart with new updates if we just had yet another change).
 *
 * \param[in] restart  Whether to restart from the beginning.
 */
void table_impl::start_update_process(bool restart)
{
    // Note: at this point this should only get called on startup
    //       so there should be no need to check whether the process
    //       was already started or not
    //
    if(restart)
    {
        file_snap_database_table::pointer_t header(std::static_pointer_cast<file_snap_database_table>(get_block(0)));
        header->set_update_oid(1);
        header->set_update_last_oid(header->get_last_oid());
    }

    // TODO: implement the update background process; this runs a thread
    //       which works on updating the database until all the rows are
    //       using the latest schema version; at that point, the process
    //       removes the BlockSchemaList and keep only the latest schema
    //       in the header
    //
    // WARNING: in order for us to allow for a strong priority where
    //          this background process runs only if time allows, the
    //          best for us is to have a thread pool and post job
    //          requests that are prioritized; frontend requests get
    //          a really high priority and background requests very
    //          low ones;

    // see the snap_thread_pool for how we want to implement this

    // TODO: add a function to only read the version of the schema of a
    //       row so as to make this process as performent as possible
    //
    // Note: since any access to existing data will auto-update rows that
    //       are using an older schema, the counter will likely be wrong
    //       and we'll reach the end of the database before the counter
    //       reaches 0, but that's as well, we still will have worked out
    //       on the entire database (it would also be possible to let this
    //       process know that a certain row was fixed, but that's complex
    //       and probably not that useful; TBD)
    //

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

    // TODO: determine whether we want to clear the whole block or just
    //       remove the "next block" pointer and always clear on a free;
    //       it would probably be cleaner to do it on a free
    //
    b->clear_block();

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


bool table_impl::row_commit(row::pointer_t row_data, commit_mode_t mode)
{
    conditions cond;
    cond.set_columns({"_oid"});
    cond.set_key("primary", row_data, row::pointer_t());
    cursor::pointer_t cur(f_table->row_select(cond));

    row::pointer_t r(cur->next_row());
    if(r == nullptr)
    {
std::cerr << "+++ mode = " << static_cast<int>(mode) << "\n";
        if(mode == commit_mode_t::COMMIT_MODE_UPDATE)
        {
            throw row_not_found(
                      "Row with key \""
                    + std::string("...")
                    + "\" was not found so it can't be updated.");
        }

std::cerr << "+++ row_insert()\n";
        row_insert(row_data, cur);
    }
    else
    {
        if(mode == commit_mode_t::COMMIT_MODE_INSERT)
        {
            throw row_already_exists(
                      "Row with key \""
                    + std::string("...")
                    + "\" already exists so it can't be inserted.");
        }

        row_update(row_data, cur);
    }

    return true;
}


/** \brief Insert a new row.
 *
 * This is an internal function which the table_impl uses to insert a new
 * row.
 *
 * \note
 * The row_commit() is called first and determines whether to call
 * insert or update or generate an error.
 *
 * \param[in] row_data  The row to be inserted.
 */
void table_impl::row_insert(row::pointer_t row_data, cursor::pointer_t cur)
{
    // if inserting, we first need to allocation this row's OID
    //
    file_snap_database_table::pointer_t header(std::static_pointer_cast<file_snap_database_table>(get_block(0)));
    oid_t oid(header->get_first_free_oid());
    bool const must_exist(oid != NULL_OID);
    if(!must_exist)
    {
        // no free OID, generate a new one
        //
        oid = header->get_last_oid();
        header->set_last_oid(oid + 1);
    }

    // found a free OID, go to it in the indirect table and replace
    // the first free OID with the one in that table (i.e. unlink
    // `oid` from the list)
    //
    oid_t position_oid(oid);
    oid_t parent_oid(oid);
    block_indirect_index::pointer_t indr;
    reference_t offset(header->get_indirect_index());
std::cerr << "+++ GET INDIRECT INDEX: " << offset << " from " << oid << "\n";
    if(offset == NULL_FILE_ADDR)
    {
        // the very first time we'll hit a null
        //
        if(oid != 1)
        {
            throw snapdatabase_logic_error("the indirect index offset is null but the first OID is not 1.");
        }
        indr = std::static_pointer_cast<block_indirect_index>(
                        allocate_new_block(dbtype_t::BLOCK_TYPE_INDIRECT_INDEX));

        header->set_indirect_index(indr->get_offset());
    }
    else
    {
        block_top_indirect_index::pointer_t parent_tind;
        reference_t parent_offset(offset);
        block::pointer_t block(get_block(offset));
        while(block->get_dbtype() == dbtype_t::BLOCK_TYPE_TOP_INDIRECT_INDEX)
        {
            block_top_indirect_index::pointer_t tind(std::static_pointer_cast<block_top_indirect_index>(block));
            oid_t const save_oid(position_oid);
            offset = tind->get_reference(position_oid, must_exist);
std::cerr << "+++ GET REFERENCE FROM SUB-OID: " << offset << " from " << position_oid << "\n";
            if(offset == NULL_FILE_ADDR)
            {
                // no child exists yet, create an INDR
                //
                indr = std::static_pointer_cast<block_indirect_index>(
                                allocate_new_block(dbtype_t::BLOCK_TYPE_INDIRECT_INDEX));
                position_oid = save_oid;
                tind->set_reference(position_oid, indr->get_offset());
                break;
            }

            if(offset == MISSING_FILE_ADDR)
            {
                if(tind->get_block_level() >= 255)
                {
                    throw out_of_bounds("too many block levels.");
                }

                block_top_indirect_index::pointer_t top_tind(
                            std::static_pointer_cast<block_top_indirect_index>(
                                allocate_new_block(dbtype_t::BLOCK_TYPE_TOP_INDIRECT_INDEX)));
                top_tind->set_block_level(tind->get_block_level() + 1);

                if(parent_tind != nullptr)
                {
                    // we overflowed an intermediate entry we have to
                    // add an intermediate (the `top_tind` is actually
                    // an intermediate) so we need to add a link in the
                    // parent to this new `TIND`
                    //
                    position_oid = parent_oid;
                    parent_tind->set_reference(position_oid, parent_offset);
                }
                else
                {
                    header->set_indirect_index(top_tind->get_offset());
                }

                position_oid = save_oid - 1;
                top_tind->set_reference(position_oid, tind->get_offset());

                indr = std::static_pointer_cast<block_indirect_index>(
                                allocate_new_block(dbtype_t::BLOCK_TYPE_INDIRECT_INDEX));
                position_oid = save_oid;
                top_tind->set_reference(position_oid, indr->get_offset());
                break;
            }

            parent_tind = tind;
            parent_offset = offset;
            parent_oid = save_oid;
            block = get_block(parent_offset);
        }

        if(indr == nullptr)
        {
            if(block->get_dbtype() != dbtype_t::BLOCK_TYPE_INDIRECT_INDEX)
            {
                throw type_mismatch(
                          "expected block of type INDIRECT INDEX (INDR) (got \""
                        + to_string(block->get_dbtype())
                        + "\" instead).");
            }
            indr = std::static_pointer_cast<block_indirect_index>(block);
std::cerr << "+++ GOT AN EXISTING INDIRECT INDEX BLOCK! " << position_oid << " vs " << indr->get_max_count() << "\n";
            if(position_oid > indr->get_max_count())
            {
                oid_t const save_oid(position_oid);

                // that `INDR` is full, create a new top `TIND`
                //
                block_top_indirect_index::pointer_t top_tind(
                            std::static_pointer_cast<block_top_indirect_index>(
                                allocate_new_block(dbtype_t::BLOCK_TYPE_TOP_INDIRECT_INDEX)));
                if(parent_tind == nullptr)
                {
                    top_tind->set_block_level(1);

                    header->set_indirect_index(top_tind->get_offset());
                }
                else
                {
                    std::uint8_t block_level(parent_tind->get_block_level());
                    if(block_level <= 1)
                    {
                        throw snapdatabase_logic_error(
                                  "parent_tind block level is "
                                + std::to_string(static_cast<int>(parent_tind->get_block_level()))
                                + " which is not valid here, it is expected to be at least 2.");
                    }

                    // we may have many levels, we need to create them all
                    // in this case (we may later ameliorate our algorithm
                    // to avoid this early cascade...)
                    //
                    position_oid = parent_oid;
                    for(;;)
                    {
                        --block_level;
                        top_tind->set_block_level(block_level);

                        parent_tind->set_reference(position_oid, top_tind->get_offset());

                        if(block_level <= 1)
                        {
                            break;
                        }

                        parent_tind = top_tind;

                        top_tind = std::static_pointer_cast<block_top_indirect_index>(
                                allocate_new_block(dbtype_t::BLOCK_TYPE_TOP_INDIRECT_INDEX));
                    }
                }

                position_oid = save_oid - 1;
                top_tind->set_reference(position_oid, block->get_offset());

                indr = std::static_pointer_cast<block_indirect_index>(
                                allocate_new_block(dbtype_t::BLOCK_TYPE_INDIRECT_INDEX));
                position_oid = save_oid;
                top_tind->set_reference(position_oid, indr->get_offset());
            }
        }
    }

    // we always overwrite the _oid, actually the user should never set
    // this column directly
    //
    cell::pointer_t oid_cell(row_data->get_cell("_oid", true));
    oid_cell->set_oid(oid);

    block_free_space::pointer_t fspc;
    reference_t fspc_offset(header->get_blobs_with_free_space());
    if(fspc_offset == NULL_FILE_ADDR)
    {
        // not yet allocated, create a Free Space block
        //
        fspc = std::static_pointer_cast<block_free_space>(
                        allocate_new_block(dbtype_t::BLOCK_TYPE_FREE_SPACE));

        header->set_blobs_with_free_space(fspc->get_offset());
    }
    else
    {
        fspc = std::static_pointer_cast<block_free_space>(get_block(fspc_offset));

        assert(fspc->get_dbtype() == dbtype_t::BLOCK_TYPE_FREE_SPACE);
    }

    buffer_t const blob(row_data->to_binary());

    free_space_t free_space(fspc->get_free_space(blob.size()));

    assert(free_space.f_size >= blob.size());

    memcpy(free_space.f_block->data(free_space.f_reference), blob.data(), blob.size());
    indr->set_reference(position_oid, free_space.f_reference);

    block_entry_index::pointer_t entry_index(cur->get_state()->get_entry_index());
    if(entry_index != nullptr)
    {
        std::uint32_t const position(cur->get_state()->get_entry_index_close_position());
        conditions const & cond(cur->get_conditions());
        buffer_t const & key(cond.get_murmur_key());
        entry_index->add_entry(key, oid, position);
        return;
    }

    cursor_state::index_reference_t::vector_t const & index_references(cur->get_state()->get_index_references());
    if(index_references.empty())
    {
        // this happens when we have a brand new table
        //
        block_primary_index::pointer_t primary_index(get_primary_index_block(true));

        conditions const & cond(cur->get_conditions());
        buffer_t const & key(cond.get_murmur_key());

std::cerr << "+++ CREATE A NEW ENTRY INDEX THOUGH... (references is empty)\n";
        entry_index = std::static_pointer_cast<block_entry_index>(
                        allocate_new_block(dbtype_t::BLOCK_TYPE_ENTRY_INDEX));

        // a murmur key is 16 bytes
        //
        entry_index->set_key_size(16);

        entry_index->add_entry(key, oid);

// 1583048029
std::cerr << "set_top_index() -- " << static_cast<int>(key[14]) << " " << static_cast<int>(key[15]) << "\n";
        primary_index->set_top_index(key, entry_index->get_offset());
    }
    else
    {
throw snapdatabase_not_yet_implemented("table: TODO implement insert close to existing entry");
    }
}


void table_impl::row_update(row::pointer_t row_data, cursor::pointer_t cur)
{
// 'cur' has the OID which we can use to find the data (we will also save
// the exact location so we don't have to search again)
snapdev::NOT_USED(row_data, cur);
}


block_primary_index::pointer_t table_impl::get_primary_index_block(bool create)
{
    block_primary_index::pointer_t primary_index;

    file_snap_database_table::pointer_t header(std::static_pointer_cast<file_snap_database_table>(get_block(0)));
    reference_t const index_block_offset(header->get_primary_index_block());
    if(index_block_offset == NULL_FILE_ADDR)
    {
        if(create)
        {
            primary_index = std::static_pointer_cast<block_primary_index>(
                        allocate_new_block(dbtype_t::BLOCK_TYPE_PRIMARY_INDEX));

            header->set_primary_index_block(primary_index->get_offset());
        }
    }
    else
    {
        primary_index = std::static_pointer_cast<block_primary_index>(get_block(index_block_offset));
    }

    return primary_index;
}


/** \brief Retrieve the reference to a row.
 *
 * This function searches for a row by OID.
 *
 * \warning
 * This function is considered internal because it does not implement a
 * way to determine whether the OID points to an actual row or was released.
 * The only way to know whether it was released would be to go through the
 * list of OIDs which would be really slow. (TODO: implement such a function
 * for debug purposes)
 *
 * \exception snapdatabase_logic_error
 * The function must be called with a valid OID. If that OID cannot be found
 * in the database, then a logic error is returned. This is because this
 * function is not to be used to dynamically search for a row, which is not
 * currently doable on the indirect index (because some of the entries may
 * be Free OIDs and not existing OIDs). This is also why the row_insert()
 * implements its own search which is capable of properly finding a free
 * spot.
 *
 * \param[in] oid  The OID of an existing row.
 *
 * \return The reference to a row or NULL_FILE_ADDR.
 */
reference_t table_impl::get_indirect_reference(oid_t oid)
{
    // search for a row using its OID
    //
    // TODO: we probably want to keep track of all the blocks we handle here
    //       so the insert and update functions can make use of them; right
    //       now each OID accessor is optimized but that means we end up doing
    //       the search multiple times
    //
    file_snap_database_table::pointer_t header(std::static_pointer_cast<file_snap_database_table>(get_block(0)));
    reference_t offset(header->get_indirect_index());
    if(offset == NULL_FILE_ADDR)
    {
        throw snapdatabase_logic_error("somehow the get_indirect_reference() was called when no row exists.");
    }

    block::pointer_t block;
    for(;;)
    {
        block = get_block(offset);
        if(block->get_dbtype() != dbtype_t::BLOCK_TYPE_TOP_INDIRECT_INDEX)
        {
            break;
        }
        block_top_indirect_index::pointer_t tind(std::static_pointer_cast<block_top_indirect_index>(block));
        offset = tind->get_reference(oid, true);
        if(offset == NULL_FILE_ADDR)
        {
            throw snapdatabase_logic_error("somehow the get_indirect_reference() was called with a still unused OID.");
        }
    }

    if(block->get_dbtype() != dbtype_t::BLOCK_TYPE_INDIRECT_INDEX)
    {
        throw type_mismatch(
                  "expected block of type INDIRECT INDEX (INDR), got \""
                + to_string(block->get_dbtype())
                + "\" instead.");
    }

    block_indirect_index::pointer_t indr(std::static_pointer_cast<block_indirect_index>(block));
    return indr->get_reference(oid, true);
}


row::pointer_t table_impl::get_indirect_row(oid_t oid)
{
    return get_row(get_indirect_reference(oid));
}


row::pointer_t table_impl::get_row(reference_t row_reference)
{
    block_data::pointer_t data(std::static_pointer_cast<block_data>(get_block(row_reference)));
    const_data_t ptr(data->data(row_reference));
    std::uint32_t const size(block_free_space::get_size(ptr));
    row::pointer_t row(std::make_shared<row>(f_table->get_pointer()));

    // TODO: rework the from_binary() to access the ptr/size pair instead
    //       so we can avoid one copy
    //
    buffer_t const blob(ptr, ptr + size);
    row->from_binary(blob);

    return row;
}


void table_impl::read_rows(cursor_data & data)
{
    switch(data.f_state->get_index_type())
    {
    case index_type_t::INDEX_TYPE_SECONDARY:
        read_secondary(data);
        break;

    case index_type_t::INDEX_TYPE_INDIRECT:
        read_indirect(data);
        break;

    case index_type_t::INDEX_TYPE_PRIMARY:
        read_primary(data);
        break;

    case index_type_t::INDEX_TYPE_EXPIRATION:
        read_expiration(data);
        break;

    case index_type_t::INDEX_TYPE_TREE:
        read_tree(data);
        break;

    default:
        throw snapdatabase_logic_error("unexpected index type in read_rows().");

    }
}


void table_impl::read_secondary(cursor_data & data)
{
snapdev::NOT_USED(data);
std::cerr << "table: TODO implement read secondary...\n";
throw snapdatabase_not_yet_implemented("table: TODO implement read secondary");
}


void table_impl::read_indirect(cursor_data & data)
{
    file_snap_database_table::pointer_t header(std::static_pointer_cast<file_snap_database_table>(get_block(0)));
    reference_t tref(header->get_indirect_index());
    if(tref == NULL_FILE_ADDR)
    {
        // we have nothing here
        // (happens until we do some commit)
        //
        return;
    }

snapdev::NOT_USED(data);
std::cerr << "table: TODO implement read indirect...\n";
throw snapdatabase_not_yet_implemented("table: TODO implement read indirect");
}


void table_impl::read_primary(cursor_data & data)
{
    // the primary index has a single position at position 0
    //
std::cerr << "the position is: " << data.f_cursor->get_position() << "\n";
    if(data.f_cursor->get_position() > 0)
    {
        return;
    }

    block_primary_index::pointer_t primary_index(get_primary_index_block(false));
    if(primary_index == nullptr)
    {
        // we have nothing here
        // (happens until we do some commit)
        //
std::cerr << "read_primary: no primary index block!?\n";
        return;
    }

    // the primary key is "special" in that we get the content of the
    // columns and then calculate the murmur value; the murmur is what's
    // used as the key, not the content of the columns
    //
    conditions const & cond(data.f_cursor->get_conditions());
    buffer_t const & key(cond.get_murmur_key());
std::cerr << "read primary with \"set_top_index()\" -- " << static_cast<int>(key[14]) << " " << static_cast<int>(key[15]) << "\n";

    // we may have one `PIDX`
    //
    // TODO: consider making the primary index optional
    //
    reference_t ref(primary_index->get_top_index(key));
    if(ref == NULL_FILE_ADDR)
    {
        // no such entry, "SELECT" returns an empty list
        //
std::cerr << "read_primary: no top index reference!?\n";
        return;
    }
    block::pointer_t block(get_block(ref));

    // we can have any number of `TIDX` or directly an `EIDX`
    //
std::cerr << "read_primary: loop through top indexex if any!?\n";
    while(block->get_dbtype() == dbtype_t::BLOCK_TYPE_TOP_INDEX)
    {
std::cerr << "read_primary: got at least one top indexex!\n";
        // we have a top index
        //
        block_top_index::pointer_t top_index(std::static_pointer_cast<block_top_index>(block));
        ref = top_index->find_index(key);
        cursor_state::index_reference_t idx_ref = {
              ref
            , top_index->get_position()
        };
std::cerr << "read_primary: found a top index!?\n";
        data.f_state->add_index_reference(idx_ref);
        if(ref == NULL_FILE_ADDR)
        {
            // no such entry, "SELECT" returns an empty list
            //
std::cerr << "read_primary: top index has null reference!?\n";
            return;
        }
        block = get_block(ref);
    }

    if(block->get_dbtype() != dbtype_t::BLOCK_TYPE_ENTRY_INDEX)
    {
        throw type_mismatch(
                  "Found unexpected block of type \""
                + to_string(block->get_dbtype())
                + "\". Expected an  \""
                + to_string(dbtype_t::BLOCK_TYPE_ENTRY_INDEX)
                + "\".");
    }

    block_entry_index::pointer_t entry_index(std::static_pointer_cast<block_entry_index>(block));
    data.f_state->set_entry_index(entry_index);

    oid_t const oid(entry_index->find_entry(key));
    data.f_state->set_entry_index_close_position(entry_index->get_position());
    if(oid == NULL_FILE_ADDR)
    {
std::cerr << "read_primary: indirect index has null reference!?\n";
        return;
    }

std::cerr << "read_primary: reading row!?\n";
    row::pointer_t r(get_indirect_row(oid));
    data.f_rows.push_back(r);

//std::cerr << "table: TODO implement read primary...\n";
//throw snapdatabase_not_yet_implemented("table: TODO implement read primary");
}


void table_impl::read_expiration(cursor_data & data)
{
snapdev::NOT_USED(data);
std::cerr << "table: TODO implement read expiration...\n";
throw snapdatabase_not_yet_implemented("table: TODO implement read expiration");
}


void table_impl::read_tree(cursor_data & data)
{
snapdev::NOT_USED(data);
std::cerr << "table: TODO implement read tree...\n";
throw snapdatabase_not_yet_implemented("table: TODO implement read tree");
}







} // namespace detail







table::table(
          context * c
        , xml_node::pointer_t x
        , schema_complex_type::map_pointer_t complex_types)
    : f_impl(std::make_shared<detail::table_impl>(c, this, x, complex_types))
{
}


table::pointer_t table::get_pointer() const
{
    return const_cast<table *>(this)->shared_from_this();
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


version_t table::schema_version() const
{
    return f_impl->schema_version();
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


schema_column::pointer_t table::column(std::string const & name, version_t const & version) const
{
    return f_impl->column(name, version);
}


schema_column::pointer_t table::column(column_id_t id, version_t const & version) const
{
    return f_impl->column(id, version);
}


schema_column::map_by_id_t table::columns_by_id(version_t const & version) const
{
    return f_impl->columns_by_id(version);
}


schema_column::map_by_name_t table::columns_by_name(version_t const & version) const
{
    return f_impl->columns_by_name(version);
}


bool table::is_sparse() const
{
    return f_impl->is_sparse();
}


bool table::is_secure() const
{
    return f_impl->is_secure();
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


schema_table::pointer_t table::get_schema(version_t const & version)
{
    return f_impl->get_schema(version);
}


row::pointer_t table::row_new() const
{
    row::pointer_t row(std::make_shared<row>(get_pointer()));

    // save the date the row was created on
    //
    cell::pointer_t created_on(row->get_cell("_created_on", true));
    std::int64_t const created_on_value(snap::snap_child::get_current_date());
    created_on->set_time_us(created_on_value);

    return row;
}


cursor::pointer_t table::row_select(conditions const & cond)
{
    // verify that the index name is acceptable
    //
    std::string const & index_name(cond.get_index_name());

    index_type_t index_type(index_name_to_index_type(index_name));
    if(index_type == index_type_t::INDEX_TYPE_INVALID)
    {
        throw invalid_name(
                  "\""
                + index_name
                + "\" is not a valid index name.");
    }

    schema_secondary_index::pointer_t secondary_index;
    if(index_type == index_type_t::INDEX_TYPE_SECONDARY)
    {
        secondary_index = f_impl->secondary_index(index_name);
        if(secondary_index == nullptr)
        {
            throw invalid_name(
                      "\""
                    + index_name
                    + "\" is not a known system or secondary index in table \""
                    + name()
                    + "\".");
        }
    }

    detail::cursor_state::pointer_t state(std::make_shared<detail::cursor_state>(index_type, secondary_index));
    return std::make_shared<cursor>(get_pointer(), state, cond);
}


bool table::row_commit(row_pointer_t row)
{
    return f_impl->row_commit(row, detail::commit_mode_t::COMMIT_MODE_COMMIT);
}


bool table::row_insert(row_pointer_t row)
{
    return f_impl->row_commit(row, detail::commit_mode_t::COMMIT_MODE_INSERT);
}


bool table::row_update(row_pointer_t row)
{
    return f_impl->row_commit(row, detail::commit_mode_t::COMMIT_MODE_UPDATE);
}


void table::read_rows(cursor::pointer_t cursor)
{
    detail::cursor_data data(cursor, cursor->get_state(), cursor->get_rows());
    f_impl->read_rows(data);
}



} // namespace snapdatabase
// vim: ts=4 sw=4 et
