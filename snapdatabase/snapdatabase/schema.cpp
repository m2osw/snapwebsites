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
#include    "snapdatabase/schema.h"

#include    "snapdatabase/convert.h"


// C++ lib
//
#include    <type_traits>


// snaplogger lib
//
#include    <snaplogger/message.h>


// last include
//
#include    <snapdev/poison.h>



namespace snapdatabase
{



namespace
{






struct_description_t g_column_description[] =
{
    define_description(
          FieldName("hash")
        , FieldType(struct_type_t::STRUCT_TYPE_UINT128)
    ),
    define_description(
          FieldName("name")
        , FieldType(struct_type_t::STRUCT_TYPE_P8STRING)
    ),
    define_description(
          FieldName("column_id")
        , FieldType(struct_type_t::STRUCT_TYPE_UINT16)
    ),
    define_description(
          FieldName("type")
        , FieldType(struct_type_t::STRUCT_TYPE_UINT16)
    ),
    define_description(
          FieldName("flags=limited/required/encrypt/default_value/bounds/length/validation")
        , FieldType(struct_type_t::STRUCT_TYPE_BITS32)
    ),
    define_description(
          FieldName("encrypt_key_name")
        , FieldType(struct_type_t::STRUCT_TYPE_P16STRING)
        , FieldOptionalField(COLUMN_FLAG_ENCRYPT)
    ),
    define_description(
          FieldName("default_value")
        , FieldType(struct_type_t::STRUCT_TYPE_BUFFER32)
        , FieldOptionalField(COLUMN_FLAG_DEFAULT_VALUE)
    ),
    define_description(
          FieldName("minimum_value")
        , FieldType(struct_type_t::STRUCT_TYPE_BUFFER32)
        , FieldOptionalField(COLUMN_FLAG_BOUNDS)
    ),
    define_description(
          FieldName("maximum_value")
        , FieldType(struct_type_t::STRUCT_TYPE_BUFFER32)
        , FieldOptionalField(COLUMN_FLAG_BOUNDS)
    ),
    define_description(
          FieldName("minimum_length")
        , FieldType(struct_type_t::STRUCT_TYPE_UINT32)
        , FieldOptionalField(COLUMN_FLAG_LENGTH)
    ),
    define_description(
          FieldName("maximum_length")
        , FieldType(struct_type_t::STRUCT_TYPE_UINT32)
        , FieldOptionalField(COLUMN_FLAG_LENGTH)
    ),
    define_description(
          FieldName("validation")
        , FieldType(struct_type_t::STRUCT_TYPE_BUFFER32)
        , FieldOptionalField(COLUMN_FLAG_VALIDATION)
    ),
    end_descriptions()
};


struct_description_t g_table_column_reference[] =
{
    define_description(
          FieldName("column_id")
        , FieldType(struct_type_t::STRUCT_TYPE_UINT16)
    ),
    end_descriptions()
};


struct_description_t g_table_secondary_index[] =
{
    define_description(
          FieldName("name")
        , FieldType(struct_type_t::STRUCT_TYPE_P8STRING)
    ),
    define_description(
          FieldName("flags=distributed")
        , FieldType(struct_type_t::STRUCT_TYPE_BITS32)
        , FieldSubDescription(g_table_column_reference)
    ),
    define_description(
          FieldName("columns")
        , FieldType(struct_type_t::STRUCT_TYPE_ARRAY16)
        , FieldSubDescription(g_table_column_reference)
    ),
    end_descriptions()
};



constexpr flags_t                       TABLE_FLAG_TEMPORARY    = 0x0001;
constexpr flags_t                       TABLE_FLAG_SPARSE       = 0x0002;
constexpr flags_t                       TABLE_FLAG_SECURE       = 0x0004;

constexpr flags_t                       TABLE_FLAG_DROP         = 0x80000000;   // NEVER SAVED, used internally only

struct_description_t g_table_description[] =
{
    define_description(
          FieldName("version")
        , FieldType(struct_type_t::STRUCT_TYPE_VERSION)
    ),
    define_description(
          FieldName("name")
        , FieldType(struct_type_t::STRUCT_TYPE_P8STRING)
    ),
    define_description(
          FieldName("flags=temporary/sparse")
        , FieldType(struct_type_t::STRUCT_TYPE_BITS64)
    ),
    define_description(
          FieldName("block_size")
        , FieldType(struct_type_t::STRUCT_TYPE_UINT32)
    ),
    define_description(
          FieldName("model")
        , FieldType(struct_type_t::STRUCT_TYPE_UINT8)
    ),
    define_description(
          FieldName("row_key")
        , FieldType(struct_type_t::STRUCT_TYPE_ARRAY16)
        , FieldSubDescription(g_table_column_reference)
    ),
    define_description(
          FieldName("secondary_indexes")
        , FieldType(struct_type_t::STRUCT_TYPE_ARRAY16)
        , FieldSubDescription(g_table_secondary_index)
    ),
    define_description(
          FieldName("columns")
        , FieldType(struct_type_t::STRUCT_TYPE_ARRAY16)
        , FieldSubDescription(g_column_description)
    ),
    end_descriptions()
};





bool validate_name(std::string const & name, size_t max_length = 255)
{
    if(name.empty())
    {
        return false;
    }
    if(name.length() > max_length)
    {
        return false;
    }

    char c(name[0]);
    if((c < 'a' || c > 'z')
    && (c < 'A' || c > 'Z')
    && c != '_')
    {
        return false;
    }

    auto const max(name.length());
    for(std::remove_const<decltype(max)>::type idx(0); idx < max; ++idx)
    {
        c = name[idx];
        if((c < 'a' || c > 'z')
        && (c < 'A' || c > 'Z')
        && (c < '0' || c > '9')
        && c != '_')
        {
            return false;
        }
    }

    return true;
}


}
// no name namespace






/** \brief Initialize a complex type from an XML node.
 *
 * Once in a list of columns, a complex type becomes a
 * `STRUCT_TYPE_STRUCTURE`.
 */
schema_complex_type::schema_complex_type(xml_node::pointer_t x)
{
    if(x->tag_name() != "complex-type")
    {
        throw invalid_xml(
                  "A column schema must be a \"column\" tag. \""
                + x->tag_name()
                + "\" is not acceptable.");
    }

    f_name = x->attribute("name");

    struct_type_t last_type(struct_type_t::STRUCT_TYPE_VOID);
    for(auto child(x->first_child()); child != nullptr; child = child->next())
    {
        if(child->tag_name() == "type")
        {
            if(last_type == struct_type_t::STRUCT_TYPE_END)
            {
                throw invalid_xml(
                          "The complex type was already ended with an explicit END. You can have additional types after that. Yet \""
                        + child->text()
                        + "\" was found after the END.");
            }
            field_t ct;
            ct.f_name = child->attribute("name");
            ct.f_type = name_to_struct_type(child->text());
            if(ct.f_type == INVALID_STRUCT_TYPE)
            {
                throw invalid_xml(
                          "Found unknown type \""
                        + child->text()
                        + "\" in your complex type definition.");
            }
            last_type = ct.f_type;

            if(ct.f_type != struct_type_t::STRUCT_TYPE_END)
            {
                f_fields.push_back(ct);
            }
        }
        else
        {
            SNAP_LOG_WARNING
                << "Unknown tag \""
                << child->tag_name()
                << "\" within a <complex-type> tag ignored."
                << SNAP_LOG_SEND;
        }
    }
}


std::string schema_complex_type::name() const
{
    return f_name;
}


size_t schema_complex_type::size() const
{
    return f_fields.size();
}


std::string schema_complex_type::type_name(int idx) const
{
    if(static_cast<std::size_t>(idx) >= f_fields.size())
    {
        throw snapdatabase_out_of_range(
                "index ("
                + std::to_string(idx)
                + ") is too large for this complex type list of fields (max: "
                + std::to_string(f_fields.size())
                + ").");
    }

    return f_fields[idx].f_name;
}


struct_type_t schema_complex_type::type(int idx) const
{
    if(static_cast<std::size_t>(idx) >= f_fields.size())
    {
        throw snapdatabase_out_of_range(
                "index ("
                + std::to_string(idx)
                + ") is too large for this complex type list of fields (max: "
                + std::to_string(f_fields.size())
                + ").");
    }

    return f_fields[idx].f_type;
}






schema_column::schema_column(schema_table::pointer_t table, xml_node::pointer_t x)
    : f_schema_table(table)
{
    if(x->tag_name() != "column")
    {
        throw invalid_xml(
                  "A column schema must be a \"column\" tag. \""
                + x->tag_name()
                + "\" is not acceptable.");
    }

    f_name = x->attribute("name");
    if(!validate_name(f_name))
    {
        throw invalid_xml(
                  "\""
                + f_name
                + "\" is not a valid column name.");
    }

    f_type = name_to_struct_type(x->attribute("type"));
    if(f_type == INVALID_STRUCT_TYPE)
    {
        // TODO: search for complex type first
        //
        throw invalid_xml(
                  "Found unknown type \""
                + child->text()
                + "\" in your column definition.");
    }

    f_flags = 0;
    if(x->attribute("limited") == "limited")
    {
        f_flags |= COLUMN_FLAG_LIMITED;
    }
    if(x->attribute("required") == "required")
    {
        f_flags |= COLUMN_FLAG_REQUIRED;
    }
    if(x->attribute("blob") == "blob")
    {
        f_flags |= COLUMN_FLAG_BLOB;
    }

    f_encrypt_key_name = x->attribute("encrypt");

    for(auto child(x->first_child()); child != nullptr; child = child->next())
    {
        if(child->tag_name() == "description")
        {
            f_description = child->text();
        }
        else if(child->tag_name() == "default")
        {
            f_default_value = string_to_typed_buffer(f_type, child->text());
        }
        else if(child->tag_name() == "external")
        {
            f_internal_size_limit = convert_to_int(f_type, child->text(), 32);
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
            //
            SNAP_LOG_WARNING
                << "Unknown tag \""
                << child->tag_name()
                << "\" within a <column> tag ignored."
                << SNAP_LOG_SEND;
        }
    }
}


schema_column::schema_column(schema_table::pointer_t table, structure::pointer_t const & s)
    : f_schema_table(table)
{
    from_structure(s);
}


void schema_column::from_structure(structure::pointer_t const & s)
{
    auto const large_uint(s.get_large_uinteger("hash"));
    f_hash[0] = large_uint.f_value[0];
    f_hash[1] = large_uint.f_value[1];
    f_name = s.get_string("name");
    f_column_id = s.get_uinteger("column_id");
    f_type = s.get_uinteger("type");
    f_flags = s.get_uinteger("flags");
    f_encrypt_key_name = s.get_string("encrypt_key_name");
    f_default_value = s.get_buffer("default_value");
    f_minimum_value = s.get_buffer("minimum_value");
    f_maximum_value = s.get_buffer("maximum_value");
    f_minimum_length = s.get_buffer("minimum_length");
    f_maximum_length = s.get_buffer("maximum_length");
    f_validation = s.get_buffer("validation");
}


schema_table::pointer_t schema_column::table() const
{
    return f_schema_table;
}


column_id_t schema_column::column_id() const
{
    return f_column_id;
}


void schema_column::hash(uint64_t & h0, uint64_t & h1) const
{
    h0 = f_hash[0];
    h1 = f_hash[1];
}


std::string schema_column::name() const
{
    return f_name;
}


struct_type_t schema_column::type() const
{
    return f_type;
}


flags_t schema_column::flags() const
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













schema_table::schema_table(xml_node::pointer_t x)
{
    if(x->tag_name() != "table")
    {
        throw invalid_xml(
                  "A table schema must be a \"keyspaces\" or \"context\". \""
                + x.tag_name()
                + "\" is not acceptable.");
    }

    f_name = x->attribute("name");
    if(!validate_name(f_name))
    {
        throw invalid_xml(
                  "\""
                + f_name
                + "\" is not a valid table name.");
    }

    bool drop(x->attribute("drop"));
    if(drop)
    {
        // do not ever save a table when the DROP flag is set (actually
        // we want to delete the entire folder if it still exists!)
        //
        f_flags |= TABLE_FLAG_DROP;
        return;
    }

    if(x->attribute("temporary"))
    {
        f_flags |= TABLE_FLAG_TEMPORARY;
    }

    if(x->attribute("sparse"))
    {
        f_flags |= TABLE_FLAG_SPARSE;
    }

    if(x->attribute("secure"))
    {
        f_flags |= TABLE_FLAG_SECURE;
    }

    xml_node::deque_t schemata;
    xml_node::deque_t secondary_indexes;

    f_model = name_to_model(x->attribute("model"));

    // 1. fully parse the complex types on the first iteration
    //
    for(auto child(x->first_child()); child != nullptr; child = child->next())
    {
        if(child->tag_name() == "block-size")
        {
            f_block_size = convert_to_uint(child->text(), 32);

            size_t const page_size(dbfile::get_system_page_size());
            if((f_block_size % page_size) != 0)
            {
                throw invalid_xml(
                          "Table \""
                        + f_name
                        + "\" is not compatible, block size "
                        + std::to_string(f_block_size)
                        + " is not supported because it is not an exact multiple of "
                        + std::to_string(page_size)
                        + ".");
            }
        }
        else if(child->tag_name() == "description")
        {
            if(!f_description.empty())
            {
                throw invalid_xml(
                          "Table \""
                        + f_name
                        + "\" has two <description> tags, only one is allowed.");
            }
            f_description = child->text();
        }
        else if(child->tag_name() == "schema")
        {
            schemata.push_back(child);
        }
        else if(child->tag_name() == "secondary-index")
        {
            secondary_indexes.push_back(child);
        }
        else if(child->tag_name() == "complex-type")
        {
            schema_complex_type ct(child);
            f_complex_types[ct.name()] = ct;
        }
        else
        {
            // generate an error for unknown tags or ignore?
            //
            SNAP_LOG_WARNING
                << "Unknown tag \""
                << child->tag_name()
                << "\" within <table name=\""
                << f_name
                << "\"> tag ignored."
                << SNAP_LOG_SEND;
        }
    }

    // 2. parse the columns
    //
    column_id_t col_id(1);
    for(auto child : schemata)
    {
        for(auto column(child->first_child());
            column != nullptr;
            column = column->next())
        {
            auto c(new schema_column(column, col_id));
            if(f_column_by_name.find(c->name()) != f_column_by_name.end())
            {
                throw invalid_xml(
                          "Column \""
                        + f_name
                        + "."
                        + c->name()
                        + "\" defined twice.");
            }

            f_column_by_id[c->column_id()] = c;
            f_column_by_name[c->name()] = c;
            ++col_id;
        }
    }

    // 3. the row-key is transformed in an array of column identifiers
    //
    // the parameter in the XML is a string of column names separated
    // by commas
    //
    row_key = x->attribute("row-key");

    advgetopt::string_list_t row_key_names;
    advgetopt::split_string(row_key, row_key_names, {","});

    for(auto n : row_key_names)
    {
        schema_column::pointer_t c(column(n));
        if(c == nullptr)
        {
            throw invalid_xml(
                      "A column referenced in the row-key attribute of table \""
                    + f_name
                    + "\" must exist. We could not find \""
                    + f_name
                    + "."
                    + n
                    + "\".");
        }
        f_row_key.push_back(c->column_id());
    }

    // 4. the secondary indexes are transformed to array of columns
    //
    for(auto si : secondary_indexes)
    {
        schema_secondary_index index;
        index.set_index_name(si->attribute("name"));

        std::string const distributed(si->attribute("distributed"));
        if(distributed.empty() || distributed == "distributed")
        {
            index.set_distributed_index(true);
        }
        else if(distributed == "one-instance")
        {
            index.set_distributed_index(false);
        }
        else
        {
            SNAP_LOG_WARNING
                << "Unknown distributed attribute value \""
                << distributed
                << "\" within a <secondary-index> tag ignored."
                << SNAP_LOG_SEND;
        }

        std::string const columns(si->text());
        advgetopt::string_list_t column_names;
        advgetopt::split_string(
                  columns
                , column_names
                , {","});

        for(auto n : column_names)
        {
            schema_column::pointer_t c(column(n));
            if(c == nullptr)
            {
                throw invalid_xml(
                          "A column referenced in the secondary-index of table \""
                        + f_name
                        + "\" must exist. We could not find \""
                        + f_name
                        + "."
                        + n
                        + "\".");
            }
            index.add_column_name(c->column_id());
        }

        f_secondary_indexes.push_back(index);
    }
}


schema_table::schema_table(virtual_buffer_t const & b)
{
    from_binary(b);
}


void schema_table::load_extension(xml_node::pointer_t e)
{
    // determine the largest column identifier, but really this is not
    // the right way of assigning the ids
    //
    column_id_t col_id(0);
    for(auto c : f_column_by_id)
    {
        if(c->column_id() > col_id)
        {
            col_id = c->column_id();
        }
    }
    ++col_id;

    for(auto child(e->first_child()); child != nullptr; child = child->next())
    {
        if(child->tag_name() == "schema")
        {
            // TODO: move to sub-function & make sure we do not get duplicates
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
        // TODO: once we have a better handle on column identifiers?
        //else if(child->tag_name() == "secondary-index")
        //{
        //    secondary_index_t si;
        //    si.f_name = child->attribute("name");
        //    si.f_columns = child->attribute("columns");
        //    secondary_indexes.push_back(si);
        //}
        else
        {
            // generate an error for unknown tags or ignore?
            //
            SNAP_LOG_WARNING
                << "Unknown tag \""
                << child->tag_name()
                << "\" within a <table-extension> tag ignored."
                << SNAP_LOG_SEND;
        }
    }
}


void schema_table::from_binary(virtual_buffer_t & b)
{
    structure const s(g_table_description, b, 0);

    f_version = s.get_uinteger("version");
    f_name = s.get_string("name");
    f_flags = s.get_uinteger("flags");
    f_model = s.get_uinteger("model");

    {
        auto const field(s.get_field("row_key"));
        auto const max(field.size());
        for(decltype(max) idx(0); idx < max; ++idx)
        {
            f_row_key.push_back(field[idx]->get_uinteger("column_id"));
        }
    }

    {
        auto const field(s.get_field("secondary_indexes"));
        auto const max(field.size());
        for(decltype(max) idx(0); idx < max; ++idx)
        {
            schema_secondary_index::pointer_t secondary_index(std::make_shared<schema_secondary_index>());

            secondary_index->set_index_name(field[idx]->get_string("name"));

            auto const columns_field(field[idx]->get_field("columns"));
            auto const columns_max(columns_field.size());
            for(decltype(columns_max) j(0); j < columns_max; ++j)
            {
                secondary_index->add_column_id(field[idx]->get_uinteger("column_id"));
            }

            f_secondary_indexes.push_back(secondary_index);
        }
    }

    {
        auto const field(s.get_field("columns"));
        auto const max(field.size());
        for(decltype(max) idx(0); idx < max; ++idx)
        {
            schema_column::pointer_t column(std::make_shared<schema_column>(f_table, field[idx]));
            f_columns_by_name[column.name()] = column;
            f_columns_by_id[column.column_id()] = column;
        }
    }
}


virtual_buffer_t schema_table::to_binary() const
{
    structure s(g_table_description);

    s.get_uinteger("version", f_version);
    s.get_string("name", f_name);
    s.get_uinteger("flags", f_flags);
    s.get_uinteger("model", f_model);

    {
        structure::vector_t v;
        auto const max(f_row_key.size());
        for(decltype(max) i(0); i < max; ++i)
        {
            structure::pointer_t column_id_structure(std::make_shared<structure>(g_table_column_reference));
            column_id_structure->set_uinteger("column_id", f_row_key[i]);
            v.push_back(column_id_structure);
        }
        s.set_array("row_key", v);
    }

    {
        structure::vector_t v;
        auto const max(f_secondary_indexes.size());
        for(decltype(max) i(0); i < max; ++i)
        {
            structure::pointer_t secondary_index_structure(std::make_shared<structure>(g_table_secondary_index));
            secondary_index_structure->set_string("name", f_secondary_indexes[i].get_index_name());

            structure::vector_t c;
            auto const max(f_secondary_indexes[i].get_column_count());
            for(decltype(max) j(0); j < max; ++j)
            {
                structure::pointer_t column_id_structure(std::make_shared<structure>(g_table_column_reference));
                column_id_structure->set_uinteger("column_id", f_secondary_index[i].get_column_id(j));
                c.push_back(column_id_structure);
            }

            secondary_index_structure->set_array("columns", c);

            v.push_back(secondary_index_structure);
        }

        s.set_array("secondary_indexes", v);
    }

    {
        structure::vector_t v;
        for(auto col : f_columns_by_id)
        {
            structure::pointer_t column_description(std::make_shared<structure>(g_column_description));
            column_description->set_string("name", col->name());
            uint512_t hash;
            col->hash(hash.f_value[0], hash.f_value[1]);
            column_description->set_large_uinteger("hash", hash);
            column_description->set_uinteger("column_id", col->column_id());
            column_description->set_uinteger("type", col->type());
            column_description->set_uinteger("flags", col->flags());
            column_description->set_string("encrypt_key_name", col->encrypt_key_name());
            column_description->set_buffer("default_value", col->default_value());
            column_description->set_buffer("minimum_value", col->minimum_value());
            column_description->set_buffer("maximum_value", col->maximum_value());
            column_description->set_buffer("minimum_length", col->minimum_length());
            column_description->set_buffer("maximum_length", col->maximum_length());
            column_description->set_buffer("validation", col->validation());
            v.push_back(column_description);
        }
        s.set_array("columns", v);
    }
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


bool schema_table::is_sparse() const
{
    return (f_flags & TABLE_FLAG_SPARSE) != 0;
}


bool schema_table::is_secure() const
{
    return (f_flags & TABLE_FLAG_SECURE) != 0;
}


row_key_t schema_table::row_key() const
{
    return f_row_key;
}


schema_column::pointer_t schema_table::column(std::string const & name) const
{
    auto it(f_column_by_name.find(id));
    if(it == f_column_by_name.end())
    {
        return schema_column::pointer_t();
    }
    return it->second;
}


schema_column::pointer_t schema_table::column(column_id_t id) const
{
    auto it(f_column_by_id.find(id));
    if(it == f_column_by_id.end())
    {
        return schema_column::pointer_t();
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
