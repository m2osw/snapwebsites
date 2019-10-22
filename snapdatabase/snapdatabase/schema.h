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
#include    "snapdatabase/dbfile.h"


// last include
//
#include    <snapdev/poison.h>



namespace snapdatabase
{



namespace detail
{
}


typedef std::vector<uint8_t>            buffer_t;
typedef uint16_t                        version_t;
typedef uint64_t                        flags_t;
typedef uint16_t                        column_id_t;
typedef std::vector<column_id_t>        column_ids_t;
typedef uint16_t                        column_type_t;


constexpr flag_t        COLUMN_FLAG_LIMITED     = 0x01;
constexpr flag_t        COLUMN_FLAG_REQUIRED    = 0x02;


class schema_complex_type
{
public:
    typedef std::map<std::string, schema_complex_type>
                                            map_t;

                                            schema_complex_type(xml_node::pointer_t x);

    std::string                             name() const;
    size_t                                  size() const;
    std::string                             type_name(int idx) const;
    struct_type_t                           type(int idx) const;

private:
    struct complex_type_t
    {
        typedef std::vector<field_t>        vector_t;

        std::string     f_name = std::string();
        struct_type_t   f_type = STRUCT_TYPE_VOID;
    };

    std::string                             f_name = std::string();
    complex_type_t::vector_t                f_complex_type = complex_type_t::vector_t();
};



class schema_column
{
public:
    typedef std::shared_ptr<schema_column>      pointer_t;
    typedef std::map<column_id_t, pointer_t>    map_by_id_t;
    typedef std::map<std::string, pointer_t>    map_by_name_t;

                                            schema_column(schema_table::pointer_t table, xml_node::pointer_t x);
                                            schema_column(schema_table::pointer_t table, structure::pointer_t const & s);

    void                                    from_structure(structure::pointer_t const & s);

    schema_table::pointer_t                 table() const;

    void                                    hash(uint64_t & h0, uint64_t & h1) const;
    std::string                             name() const;
    column_id_t                             column_id() const;
    column_type_t                           type() const;
    flags_t                                 flags() const;
    std::string                             encrypt_key_name() const;
    buffer_t                                default_value() const;
    buffer_t                                minimum_value() const;
    buffer_t                                maximum_value() const;
    buffer_t                                minimum_length() const;
    buffer_t                                maximum_length() const;
    buffer_t                                validation() const;

private:
    uint64_t                                f_hash[2] = { 0ULL, 0ULL };
    std::string                             f_name = std::string();
    column_id_t                             f_column_id = column_id_t();
    column_type_t                           f_type = column_type_t();
    flags_t                                 f_flags = flag_t();
    std::string                             f_encrypt_key_name = std::string();
    buffer_t                                f_default_value = buffer_t();
    buffer_t                                f_minimum_value = buffer_t();
    buffer_t                                f_maximum_value = buffer_t();
    buffer_t                                f_minimum_length = buffer_t();
    buffer_t                                f_maximum_length = buffer_t();
    buffer_t                                f_validation = buffer_t();

    // not saved on disk
    //
    schema_table::pointer_t                 f_schema_table = schema_table::pointer_t();
    std::string                             f_description = std::string();
};


class schema_secondary_index
{
public:
    typedef std::shared_ptr<schema_secondary_index> pointer_t;
    typedef std::vector<pointer_t>                  vector_t;

                                            schema_secondary_index();

    std::string                             get_index_name() const;
    void                                    set_index_name(std::string const & index_name);

    size_t                                  get_column_count();
    column_id_t                             get_column_id(int idx);
    void                                    add_column_id(column_id_t id);

private:
    std::string                             f_index_name = std::string();
    column_ids_t                            f_column_ids = column_ids_t();
};




class schema_table
{
public:
    typedef std::shared_ptr<schema_table>   pointer_t;

                                            schema_table(xml_node::pointer_t x);
                                            schema_table(buffer_t const & x);

    void                                    load_extension(xml_node::pointer_t e);

    void                                    from_binary(buffer_t const & x);
    buffer_t                                to_binary() const;

    version_t                               version() const;
    std::string                             name() const;
    model_t                                 model() const;
    column_ids_t                            row_key() const;
    schema_column_t::pointer_t              column(std::string const & name) const;
    schema_column_t::pointer_t              column(column_id_t id) const;
    map_by_id_t                             columns_by_id() const;
    map_by_name_t                           columns_by_name() const;

    std::string                             description() const;

private:
    version_t                               f_version = version_t();
    std::string                             f_name = std::string();
    flag_t                                  f_flags = flag_t();
    model_t                                 f_model = model_t::TABLE_MODEL_CONTENT;
    column_ids_t                            f_row_key = column_ids_t();
    schema_secondary_index::vector_t        f_secondary_indexes = schema_secondary_index::vector_t();
    schema_complex_type::map_t              f_complex_types = schema_complex_type::map_t();
    map_by_name_t                           f_columns_by_name = schema_column::map_t();
    map_by_id_t                             f_columns_by_id = schema_column::map_t();

    // not saved on disk
    //
    std::string                             f_description = std::string();
};



} // namespace snapdatabase
// vim: ts=4 sw=4 et
