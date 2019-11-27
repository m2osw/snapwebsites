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
 * \brief Table file header.
 *
 * The table class handles the settings of the table and the files where the
 * data is saved.
 *
 * The _main file_ is used to save the schema. That's where we read it from.
 * This file also includes all the table settings, information about indexes,
 * etc.
 *
 * The table knows how to find all the files, create dbfile objects and
 * request those files to load blocks and thus the settings and data saved
 * in those files.
 *
 * A table is owned by a context.
 */

// self
//
#include    "snapdatabase/data/schema.h"
#include    "snapdatabase/data/xml.h"



namespace snapdatabase
{



namespace detail
{
class table_impl;
}


class context;
class dbfile;
typedef std::shared_ptr<dbfile>                 dbfile_pointer_t;
class block;
typedef std::shared_ptr<block>                  block_pointer_t;

class table
    : public std::enable_shared_from_this<table>
{
public:
    typedef std::shared_ptr<table>              pointer_t;
    typedef std::weak_ptr<table>                weak_pointer_t;
    typedef std::map<std::string, pointer_t>    map_t;

                                                table(
                                                      context * c
                                                    , xml_node::pointer_t x
                                                    , xml_node::map_t complex_types);

    void                                        load_extension(xml_node::pointer_t e);

    pointer_t                                   get_pointer();
    dbfile_pointer_t                            get_dbfile() const;
    version_t                                   version() const;
    std::string                                 name() const;
    model_t                                     model() const;
    column_ids_t                                row_key() const;
    schema_column::pointer_t                    column(std::string const & name) const;
    schema_column::pointer_t                    column(column_id_t id) const;
    schema_column::map_by_id_t                  columns_by_id() const;
    schema_column::map_by_name_t                columns_by_name() const;
    bool                                        is_sparse() const;
    bool                                        is_secure() const;
    std::string                                 description() const;
    size_t                                      get_size() const; // total size of the file right now
    size_t                                      get_page_size() const; // size of one block in bytes including the magic
    block_pointer_t                             get_block(reference_t offset);
    block_pointer_t                             allocate_new_block(dbtype_t type);
    bool                                        verify_schema();

private:
    std::shared_ptr<detail::table_impl>         f_impl;
};



} // namespace snapdatabase
// vim: ts=4 sw=4 et
