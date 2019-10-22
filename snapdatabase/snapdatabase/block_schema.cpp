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
#include    "snapdatabase/dbfile.h"


// last include
//
#include    <snapdev/poison.h>



namespace snapdatabase
{



namespace detail
{
}



schema_column::schema_column(schema_table::pointer_t table, xml const & x)
    : f_schema_table(table)
{
    if(x.tag_name() != "column")
    {
        throw invalid_xml(
                  "A column schema must be a \"column\" tag. \""
                + x.tag_name()
                + "\" is not acceptable.");
    }

    f_name = x.attribute("name");
    f_type = name_to_column_type(x.attribute("type"));

    f_flags = 0;
    if(x.attribute("limited"))
    {
        f_flags |= COLUMN_FLAG_LIMITED;
    }
    if(x.attribute("required"))
    {
        f_flags |= COLUMN_FLAG_REQUIRED;
    }

    f_encrypt_key_name = x.attribute("encrypt");

    for(auto child(x.root()); child != nullptr; child = child->next())
    {
        if(child->tag_name() == "description")
        {
            f_description = child->text();
        }
        else if(child->tag_name() == "default")
        {
            f_default_value = string_to_typed_buffer(f_type, child->text());
        }
        else if(child->tag_name() == "min-value")
        {
            f_minimum_value = string_to_typed_buffer(f_type, child->text());
        }
        else if(child->tag_name() == "max-value")
        {
            f_maximum_value = string_to_typed_buffer(f_type, child->text());
        }
        else if(child->tag_name() == "min-length")
        {
            f_minimum_length = string_to_typed_buffer(f_type, child->text());
        }
        else if(child->tag_name() == "max-length")
        {
            f_maximum_length = string_to_typed_buffer(f_type, child->text());
        }
        else if(child->tag_name() == "validation")
        {
            f_validation = compile_script(child->text());
        }
        else
        {
            // generate an error for unknown tags or ignore?
        }
    }
}


schema_column::schema_column(schema_table::pointer_t table, buffer_t const & x)
    : f_schema_table(table)
{
    from_binary(x);
}


void schema_column::from_binary(buffer_t const & x)
{
}


schema_block schema_column::to_binary() const
{
}


schema_table::pointer_t schema_column::table() const
{
    return f_schema_table;
}


column_id_t schema_column::column_id() const
{
    return f_column_id;
}


std::string schema_column::name() const
{
    return f_name;
}


column_type_t schema_column::type() const
{
    return f_type;
}


flag_t schema_column::flags() const
{
    return f_flags;
}


std::string schema_column::encrypt_key_name() const
{
    return f_encrypt_key_name;
}


buffer_t schema_column::default_value() const
{
    return f_default_value;
}


buffer_t schema_column::minimum_value() const
{
    return f_minimum_value;
}


buffer_t schema_column::maximum_value() const
{
    return f_maximum_value;
}


buffer_t schema_column::minimum_length() const
{
    return f_minimum_length;
}


buffer_t schema_column::maximum_length() const
{
    return f_maximum_length;
}


buffer_t schema_column::validation() const
{
    return f_validation;
}













schema_table::schema_table(xml const & x)
{
    if(x.tag_name() != "keyspaces"
    && x.tag_name() != "context")
    {
        throw invalid_xml(
                  "A table schema must be a \"keyspaces\" or \"context\". \""
                + x.tag_name()
                + "\" is not acceptable.");
    }

    bool drop(x.attribute("drop"));
    if(drop)
    {
        f_flags |= TABLE_FLAG_DROP;
        return;
    }

    // while loading, we have to keep the column names because we do not
    // yet have all the column identifiers
    //
    struct secondary_index_t
    {
        std::string     f_name = std::string();
        std::string     f_column_names = std::string();
    };
    std::vector<secondary_index_t>  secondary_indexes;

    f_name = x.attribute("name");
    f_model = name_to_model(x.attribute("model"));

    column_id_t col_id(1);
    for(auto child(x.root()); child != nullptr; child = child->next())
    {
        if(child->tag_name() == "description")
        {
            f_description = child->text();
        }
        else if(child->tag_name() == "schema")
        {
            for(auto column(child->first_child();
                column != nullptr;
                column = column->next())
            {
                auto c(new schema_column(column, col_id));
                f_column_by_id[c->column_id()] = c;
                f_column_by_name[c->name()] = c;
                ++col_id;
            }
        }
        else if(child->tag_name() == "secondary-index")
        {
            secondary_index_t si;
            si.f_name = child->attribute("name");
            si.f_columns = child->attribute("columns");
            secondary_indexes.push_back(si);
        }
        else
        {
            // generate an error for unknown tags or ignore?
        }
    }

    // the row-key is transformed in an array of column identifiers
    //
    // the parameter in the XML is a string of column names separated
    // by commas
    //
    row_key = x.attribute("row-key");

    advgetopt::string_list_t row_key_names;
    advgetopt::split_string(row_key, row_key_names, {","});

    for(auto n : row_key_names)
    {
        schema_column::pointer_t c(column(n));
        if(c == nullptr)
        {
            throw invalid_xml(
                      "A column referenced in the row-key attribute must exist. We could not find \""
                    + n
                    + "\".");
        }
        f_row_key.push_back(c);
    }
}


schema_table::schema_table(buffer_t const & x)
{
    from_binary(x);
}


void schema_table::from_binary(buffer_t const & x)
{
}


buffer_t schema_table::to_binary() const
{
}


version_t schema_table::version() const
{
    return f_version;
}


std::string schema_table::name() const
{
    return f_name;
}


model_t schema_table::model() const
{
    return f_model;
}


row_key_t schema_table::row_key() const
{
    return f_row_key;
}


schema_column_t::pointer_t schema_table::column(std::string const & name) const
{
    auto it(f_column_by_name.find(id));
    if(it == f_column_by_name.end())
    {
        return schema_column_t::pointer_t();
    }
    return it->second;
}


schema_column_t::pointer_t schema_table::column(column_id_t id) const
{
    auto it(f_column_by_id.find(id));
    if(it == f_column_by_id.end())
    {
        return schema_column_t::pointer_t();
    }
    return it->second;
}


map_by_name_t schema_table::columns_by_name() const
{
    return f_columns_by_name;
}


map_by_id_t schema_table::columns_by_id() const
{
    return f_columns_by_id;
}


std::string schema_table::description() const
{
    return f_description;
}



} // namespace snapdatabase
// vim: ts=4 sw=4 et
