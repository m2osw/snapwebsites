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
#pragma once


/** \file
 * \brief Context file header.
 *
 * The context class manages a set of tables. This represents one _database_
 * in the SQL world. The context is pretty shallow otherwise. Most of our
 * settings are in the tables (i.e. replication, compression, compaction,
 * filters, indexes, etc. all of these things are part of the tables).
 */

// self
//
#include    "snapdatabase/data/structure.h"
#include    "snapdatabase/data/xml.h"



namespace snapdatabase
{



//typedef uint64_t                        block_ref_t;
//typedef uint8_t                         flag8_t;
//typedef uint16_t                        flag16_t;
typedef uint32_t                        flag32_t;       // look into not using these, instead use the structure directly
typedef uint64_t                        flag64_t;

typedef uint16_t                        column_id_t;
typedef std::vector<column_id_t>        column_ids_t;



enum class model_t
{
    TABLE_MODEL_CONTENT,
    TABLE_MODEL_DATA,
    TABLE_MODEL_LOG,
    TABLE_MODEL_QUEUE,
    TABLE_MODEL_SEQUENCIAL,
    TABLE_MODEL_SESSION,
    TABLE_MODEL_TREE,

    TABLE_MODEL_DEFAULT = TABLE_MODEL_CONTENT
};

model_t             name_to_model(std::string const & name);


enum compare_t
{
    COMPARE_SCHEMA_EQUAL,
    COMPARE_SCHEMA_UPDATE,
    COMPARE_SCHEMA_DIFFER
};



// SAVED IN FILE, DO NOT CHANGE BIT LOCATIONS
constexpr flag64_t                          TABLE_FLAG_TEMPORARY    = (1LL << 0);
constexpr flag64_t                          TABLE_FLAG_SPARSE       = (1LL << 1);
constexpr flag64_t                          TABLE_FLAG_SECURE       = (1LL << 2);

// NEVER SAVED, used internally only
constexpr flag64_t                          TABLE_FLAG_DROP         = (1LL << 63);


// Special values
constexpr column_id_t                       COLUMN_NULL = 0;


// SAVED IN FILE, DO NOT CHANGE BIT LOCATIONS
constexpr flag32_t                          COLUMN_FLAG_LIMITED                 = (1ULL << 0);
constexpr flag32_t                          COLUMN_FLAG_REQUIRED                = (1ULL << 1);
constexpr flag32_t                          COLUMN_FLAG_BLOB                    = (1ULL << 2);
constexpr flag32_t                          COLUMN_FLAG_SYSTEM                  = (1ULL << 3);
constexpr flag32_t                          COLUMN_FLAG_REVISION_TYPE           = (3ULL << 4);   // TWO BITS (see COLUMN_REVISION_TYPE_...)

// Revision Types (after the shift, TBD: should we keep the shift?)
constexpr flag32_t                          COLUMN_REVISION_TYPE_GLOBAL         = 0;
constexpr flag32_t                          COLUMN_REVISION_TYPE_BRANCH         = 1;
constexpr flag32_t                          COLUMN_REVISION_TYPE_REVISION       = 2;
//constexpr flag32_t                          COLUMN_REVISION_TYPE_unused         = 3; -- current unused


// SAVED IN FILE, DO NOT CHANGE BIT LOCATIONS
constexpr flag32_t                          SECONDARY_INDEX_FLAG_DISTRIBUTED    = (1LL << 0);



class schema_complex_type
{
public:
    typedef std::map<std::string, schema_complex_type>
                                            map_t;

                                            schema_complex_type();
                                            schema_complex_type(xml_node::pointer_t x);

    std::string                             name() const;
    size_t                                  size() const;
    std::string                             type_name(int idx) const;
    struct_type_t                           type(int idx) const;

private:
    struct field_t
    {
        typedef std::vector<field_t>        vector_t;

        std::string             f_name = std::string();
        struct_type_t           f_type = struct_type_t::STRUCT_TYPE_VOID;
    };

    std::string                             f_name = std::string();
    field_t::vector_t                       f_fields = field_t::vector_t();
};




class schema_table;
typedef std::shared_ptr<schema_table>       schema_table_pointer_t;
typedef std::weak_ptr<schema_table>         schema_table_weak_pointer_t;

class schema_column
{
public:
    typedef std::shared_ptr<schema_column>      pointer_t;
    typedef std::map<column_id_t, pointer_t>    map_by_id_t;
    typedef std::map<std::string, pointer_t>    map_by_name_t;

                                            schema_column(schema_table_pointer_t table, xml_node::pointer_t x);
                                            schema_column(schema_table_pointer_t table, structure::pointer_t s);
                                            schema_column(
                                                      schema_table_pointer_t table
                                                    , std::string name
                                                    , struct_type_t type
                                                    , flag32_t flags);

    void                                    from_structure(structure::pointer_t s);
    compare_t                               compare(schema_column const & rhs) const;

    schema_table_pointer_t                  table() const;

    std::string                             name() const;
    column_id_t                             column_id() const;
    void                                    set_column_id(column_id_t id);
    struct_type_t                           type() const;
    flag32_t                                flags() const;
    std::string                             encrypt_key_name() const;
    buffer_t                                default_value() const;
    buffer_t                                minimum_value() const;
    buffer_t                                maximum_value() const;
    std::uint32_t                           minimum_length() const;
    std::uint32_t                           maximum_length() const;
    buffer_t                                validation() const;

private:
    std::string                             f_name = std::string();
    column_id_t                             f_column_id = column_id_t();
    struct_type_t                           f_type = struct_type_t();
    flag32_t                                f_flags = flag32_t();
    std::string                             f_encrypt_key_name = std::string();
    std::int32_t                            f_internal_size_limit = -1; // -1 = no limit; if size > f_internal_size_limit, save in external file
    buffer_t                                f_default_value = buffer_t();
    buffer_t                                f_minimum_value = buffer_t();
    buffer_t                                f_maximum_value = buffer_t();
    std::uint32_t                           f_minimum_length = 0;
    std::uint32_t                           f_maximum_length = 0;
    buffer_t                                f_validation = buffer_t();

    // not saved on disk
    //
    schema_table_weak_pointer_t             f_schema_table = schema_table_weak_pointer_t();
    std::string                             f_description = std::string();
};




class schema_secondary_index
{
public:
    typedef std::shared_ptr<schema_secondary_index> pointer_t;
    typedef std::map<std::string, pointer_t>        map_t;

    compare_t                               compare(schema_secondary_index const & rhs) const;

    std::string                             get_index_name() const;
    void                                    set_index_name(std::string const & index_name);

    bool                                    get_distributed_index() const;
    void                                    set_distributed_index(bool distributed);

    size_t                                  get_column_count();
    column_id_t                             get_column_id(int idx);
    void                                    add_column_id(column_id_t id);

private:
    std::string                             f_index_name = std::string();
    column_ids_t                            f_column_ids = column_ids_t();
    flag64_t                                f_flags = flag64_t();
};




class schema_table
    : public std::enable_shared_from_this<schema_table>
{
public:
    typedef std::shared_ptr<schema_table>   pointer_t;

    void                                    from_xml(xml_node::pointer_t x);
    void                                    load_extension(xml_node::pointer_t e);
    compare_t                               compare(schema_table const & rhs) const;

    void                                    from_binary(virtual_buffer::pointer_t b);
    virtual_buffer::pointer_t               to_binary() const;

    version_t                               schema_version() const;
    time_t                                  added_on() const;
    std::string                             name() const;
    model_t                                 model() const;
    bool                                    is_sparse() const;
    bool                                    is_secure() const;
    column_ids_t                            row_key() const;
    void                                    assign_column_ids();
    schema_column::pointer_t                column(std::string const & name) const;
    schema_column::pointer_t                column(column_id_t id) const;
    schema_column::map_by_id_t              columns_by_id() const;
    schema_column::map_by_name_t            columns_by_name() const;
    schema_secondary_index::pointer_t       secondary_index(std::string const & name) const;

    std::string                             description() const;
    std::uint32_t                           block_size() const;

private:
    version_t                               f_version = version_t();
    time_t                                  f_added_on = time(nullptr);
    std::string                             f_name = std::string();
    flag64_t                                f_flags = flag64_t();
    model_t                                 f_model = model_t::TABLE_MODEL_CONTENT;
    std::uint32_t                           f_block_size = 0;
    column_ids_t                            f_row_key = column_ids_t();
    schema_secondary_index::map_t           f_secondary_indexes = schema_secondary_index::map_t();
    schema_complex_type::map_t              f_complex_types = schema_complex_type::map_t();
    schema_column::map_by_name_t            f_columns_by_name = schema_column::map_by_name_t();
    schema_column::map_by_id_t              f_columns_by_id = schema_column::map_by_id_t();

    // not saved in database, only in XML
    //
    std::string                             f_description = std::string();
};



} // namespace snapdatabase
// vim: ts=4 sw=4 et
