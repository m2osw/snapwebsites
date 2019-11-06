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
                                                      context::pointer_t c
                                                    , xml_node::pointer_t x
                                                    , xml_node::map_t complex_types);

    void                                        load_extension(xml_node::pointer_t e);

    version_t                                   version() const;
    std::string                                 name() const;
    model_t                                     model() const;
    column_ids_t                                row_key() const;
    schema_column_t::pointer_t                  column(std::string const & name) const;
    schema_column_t::pointer_t                  column(column_id_t id) const;
    map_by_id_t                                 columns_by_id() const;
    map_by_name_t                               columns_by_name() const;
    std::string                                 description() const;

private:
    context::weak_pointer_t                     f_context = context::weak_pointer_t();
    dbfile::pointer_t                           f_dbfile = dbfile::pointer_t();
    schema_table::pointer_t                     f_schema_table = schema_table::pointer_t();
    xml_node::map_t                             f_complex_types = xml_node::map_t();
};


table_impl::table_impl(
          context::pointer_t c
        , xml_node::pointer_t x
        , xml_node::map_t complex_types)
    : f_context(c)
    , f_dbfile(std::make_shared<dbfile>(c->get_path(), "data"))
    , f_schema_table(std::make_shared<schema_table>(x))
    , f_complex_types(complex_types)
{
    // open the file
    //
    f_file

    // the schema found in the XML file is the authoritative one
    // load the one from the table and if different, apply the
    // changes
    //
    auto current_schema(std::make_shared<schema_table>(file_schema));
}


void table_impl::load_extension(xml_node::pointer_t e)
{
    f_schema_table->load_extension(e);
}


std::string table_impl::version() const
{
    return f_schema_table->version();
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


schema_column_t::pointer_t table_impl::column(std::string const & name) const
{
    return f_schema_table->column(name);
}


schema_column_t::pointer_t table_impl::column(column_id_t id) const
{
    return f_schema_table->column(id);
}


map_by_id_t table_impl::columns_by_id() const
{
    return f_schema_table->columns_by_id();
}


map_by_name_t table_impl::columns_by_name() const
{
    return f_schema_table->columns_by_name();
}


std::string table_impl::description() const
{
    return f_schema_table->description();
}



} // namespace detail



table::table(
          context::pointer_t c
        , xml_node::pointer_t x
        , xml_node::map_t complex_types)
    : f_impl(new table_impl(c, x, complex_types))
{
}


version_t table::version() const
{
    return f_impl->version();
}


std::string table::name() const
{
    return f_impl->name();
}


type_t table::model() const
{
    return f_impl->mode();
}


column_ids_t table::row_key() const
{
    return f_impl->row_key();
}


schema_column_t::pointer_t table::column(std::string const & name) const
{
    return f_impl->column(name);
}


schema_column_t::pointer_t table::column(column_id_t id) const
{
    return f_impl->column(id);
}


map_by_id_t table::columns_by_id() const
{
    return f_impl->columns_by_id();
}


map_by_name_t table::columns_by_name() const
{
    return f_impl->columns_by_name();
}


std::string table::description() const
{
    return f_impl->description();
}



} // namespace snapdatabase
// vim: ts=4 sw=4 et
