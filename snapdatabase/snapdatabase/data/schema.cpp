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
 *
 * \todo
 * The `version` field is not going to be cross instance compatible.
 * Any new instance of a database file gets a schema with version 1.0.
 * That version increases as modifications to the schema are being applied
 * (for example, as you add a new plugin to the Snap! environment of a
 * website, the content table is likely to be updated and get a newer
 * version).
 * \todo
 * The problem with this mechanism is that the exact same schema on two
 * different nodes will not always have the same version. If you create
 * a new node when another has a schema version 1.15, then the new node
 * gets the same schema, but the version is set to 1.0.
 * \todo
 * On day to day matters, this has no bearing, but it could be really
 * confusing to administrators. There are two possible solutions: have
 * the version assigned using communication and use the latest version
 * for that table, latest version across your entire set of nodes.
 * The other, which is much easier as it requires no inter-node
 * communication, is to calculate an MD5 sum of the schema. As long as
 * thjat calculation doesn't change across versions of Snap! Database
 * then we're all good (but I don't think we can ever guarantee such
 * a thing, so that solution becomes complicated in that sense).
 */

// self
//
#include    "snapdatabase/data/schema.h"

#include    "snapdatabase/data/convert.h"
#include    "snapdatabase/data/script.h"


// snaplogger lib
//
#include    <snaplogger/message.h>


// C++ lib
//
#include    <iostream>
#include    <type_traits>


// boost lib
//
#include    <boost/algorithm/string.hpp>


// last include
//
#include    <snapdev/poison.h>



namespace snapdatabase
{



namespace
{



std::string             g_expiration_date = "expiration_date";



struct_description_t g_column_description[] =
{
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
          FieldName("flags=limited/required/blob/system/revision_type:2")
        , FieldType(struct_type_t::STRUCT_TYPE_BITS32)
    ),
    define_description(
          FieldName("encrypt_key_name")
        , FieldType(struct_type_t::STRUCT_TYPE_P16STRING)
    ),
    define_description(
          FieldName("default_value")
        , FieldType(struct_type_t::STRUCT_TYPE_BUFFER32)
    ),
    define_description(
          FieldName("minimum_value")
        , FieldType(struct_type_t::STRUCT_TYPE_BUFFER32)
    ),
    define_description(
          FieldName("maximum_value")
        , FieldType(struct_type_t::STRUCT_TYPE_BUFFER32)
    ),
    define_description(
          FieldName("minimum_length")
        , FieldType(struct_type_t::STRUCT_TYPE_UINT32)
    ),
    define_description(
          FieldName("maximum_length")
        , FieldType(struct_type_t::STRUCT_TYPE_UINT32)
    ),
    define_description(
          FieldName("validation")
        , FieldType(struct_type_t::STRUCT_TYPE_BUFFER32)
    ),
    end_descriptions()
};


struct_description_t g_column_reference[] =
{
    define_description(
          FieldName("column_id")
        , FieldType(struct_type_t::STRUCT_TYPE_UINT16)
    ),
    end_descriptions()
};


struct_description_t g_sort_column[] =
{
    define_description(
          FieldName("column_id")
        , FieldType(struct_type_t::STRUCT_TYPE_UINT16)
    ),
    define_description(
          FieldName("flags=descending/not_null")
        , FieldType(struct_type_t::STRUCT_TYPE_BITS32)
    ),
    define_description(
          FieldName("length")
        , FieldType(struct_type_t::STRUCT_TYPE_UINT32)
    ),
    define_description(
          FieldName("function")
        , FieldType(struct_type_t::STRUCT_TYPE_BUFFER32)
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
    ),
    define_description(
          FieldName("sort_columns")
        , FieldType(struct_type_t::STRUCT_TYPE_ARRAY16)
        , FieldSubDescription(g_sort_column)
    ),
    define_description(
          FieldName("filter")
        , FieldType(struct_type_t::STRUCT_TYPE_BUFFER32)
    ),
    end_descriptions()
};




struct_description_t g_table_description[] =
{
    define_description(
          FieldName("schema_version")
        , FieldType(struct_type_t::STRUCT_TYPE_VERSION)
    ),
    define_description(
          FieldName("added_on")
        , FieldType(struct_type_t::STRUCT_TYPE_TIME)
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
        , FieldSubDescription(g_column_reference)
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



index_type_t index_name_to_index_type(std::string const & name)
{
    if(name.empty())
    {
        return index_type_t::INDEX_TYPE_INVALID;
    }

    switch(name[0])
    {
    case 'e':
        if(name == "expiration")
        {
            return index_type_t::INDEX_TYPE_EXPIRATION;
        }
        break;

    case 'i':
        if(name == "indirect")
        {
            return index_type_t::INDEX_TYPE_INDIRECT;
        }
        break;

    case 'p':
        if(name == "primary")
        {
            return index_type_t::INDEX_TYPE_PRIMARY;
        }
        break;

    case 't':
        if(name == "tree")
        {
            return index_type_t::INDEX_TYPE_TREE;
        }
        break;

    }

    return validate_name(name)
                ? index_type_t::INDEX_TYPE_SECONDARY
                : index_type_t::INDEX_TYPE_INVALID;
}


std::string index_type_to_index_name(index_type_t type)
{
    switch(type)
    {
    case index_type_t::INDEX_TYPE_INDIRECT:   return "indirect";
    case index_type_t::INDEX_TYPE_PRIMARY:    return "primary";
    case index_type_t::INDEX_TYPE_EXPIRATION: return "expiration";
    case index_type_t::INDEX_TYPE_TREE:       return "tree";
    case index_type_t::INDEX_TYPE_INVALID:    break;
    case index_type_t::INDEX_TYPE_SECONDARY:  break;
    }

    // not a system type, return an empty string even for the secondary type
    //
    return std::string();
}



struct model_and_name_t
{
    model_t const           f_model = model_t::TABLE_MODEL_CONTENT;
    char const * const      f_name = nullptr;
};

#define MODEL_AND_NAME(name)    { model_t::TABLE_MODEL_##name, #name }

model_and_name_t g_model_and_name[] =
{
    MODEL_AND_NAME(CONTENT),
    MODEL_AND_NAME(DATA),
    MODEL_AND_NAME(DEFAULT),
    MODEL_AND_NAME(LOG),
    MODEL_AND_NAME(QUEUE),
    MODEL_AND_NAME(SEQUENCIAL),
    MODEL_AND_NAME(SESSION),
    MODEL_AND_NAME(TREE)
};


model_t name_to_model(std::string const & name)
{
#ifdef _DEBUG
    // verify in debug because if not in order we can't do a binary search
    for(size_t idx(1);
        idx < sizeof(g_model_and_name) / sizeof(g_model_and_name[0]);
        ++idx)
    {
        if(strcmp(g_model_and_name[idx - 1].f_name
                , g_model_and_name[idx].f_name) >= 0)
        {
            throw snapdatabase_logic_error(
                      "names in g_model_and_name are not in alphabetical order: "
                    + std::string(g_model_and_name[idx - 1].f_name)
                    + " >= "
                    + g_model_and_name[idx].f_name
                    + " (position: "
                    + std::to_string(idx)
                    + ").");
        }
    }
#endif

    if(name.empty())
    {
        return model_t::TABLE_MODEL_DEFAULT;
    }

    std::string const uc(boost::algorithm::to_upper_copy(name));

    int i(0);
    int j(sizeof(g_model_and_name) / sizeof(g_model_and_name[0]));
    while(i < j)
    {
        int const p((j - i) / 2 + i);
        int const r(uc.compare(g_model_and_name[p].f_name));
        if(r < 0)
        {
            i = p + 1;
        }
        else if(r > 0)
        {
            j = p;
        }
        else
        {
            return g_model_and_name[p].f_model;
        }
    }

    SNAP_LOG_WARNING
        << "Unknown model name \""
        << name
        << "\" for your table. Please check the spelling. The name is case insensitive."
        << SNAP_LOG_SEND;

    // return the default, this is just a warning
    //
    return model_t::TABLE_MODEL_DEFAULT;
}





// required constructor for copying in the map
schema_complex_type::schema_complex_type()
{
}


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
                          "The complex type was already ended with an explicit END. You cannot have additional types after that. Yet \""
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
                        + "\" in your complex type definition (we do not currently support complex types within other complex types).");
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

    std::string const & type_name(x->attribute("type"));
    f_type = name_to_struct_type(type_name);
    if(f_type == INVALID_STRUCT_TYPE)
    {
        schema_complex_type::pointer_t ct(table->complex_type(type_name));
        if(ct == nullptr)
        {
            throw invalid_xml(
                      "Found unknown type \""
                    + x->attribute("type")
                    + "\" in your \""
                    + f_name
                    + "\" column definition.");
        }

        // TODO: actually implement the complex type
        //       (at this time I'm thinking that the way to do it is
        //       to create one column per complex type column with the
        //       name defined as `<foo>.<blah>`--however, we may also
        //       want to keep the data in a single column and use
        //       the complex type to read/write it)
        //
        throw snapdatabase_not_yet_implemented(
                "full support for complex types not yet implemented");
    }

    // if the user defined an expiration date column, make sure it uses
    // the correct type otherwise that's a bug and it needs to be fixed
    //
    if(is_expiration_date_column())
    {
        switch(f_type)
        {
        case struct_type_t::STRUCT_TYPE_TIME:
        case struct_type_t::STRUCT_TYPE_MSTIME:
        case struct_type_t::STRUCT_TYPE_USTIME:
            break;

        default:
            throw type_mismatch(
                    "the \"expiration_date\" column must be assigned a  valid time type (TIME, MSTIME, USTIME), "
                  + to_string(f_type)
                  + " is not valid.");

        }
    }

    f_flags = 0;
    if(x->attribute("limited") == "limited")
    {
        // limit display of this column by default because it could be really
        // large
        //
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
            f_internal_size_limit = convert_to_int(child->text(), 32, unit_t::UNIT_SIZE);
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
            f_minimum_length = convert_to_uint(child->text(), 32);
        }
        else if(child->tag_name() == "max-length")
        {
            f_maximum_length = convert_to_uint(child->text(), 32);
        }
        else if(child->tag_name() == "validation")
        {
            std::string const code(child->text());
            if(!code.empty())
            {
                f_validation = compile_script(code);
            }
            else
            {
                f_validation.clear();
            }
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


schema_column::schema_column(schema_table::pointer_t table, structure::pointer_t s)
    : f_schema_table(table)
{
    from_structure(s);
}


schema_column::schema_column(
              schema_table_pointer_t table
            , std::string name
            , struct_type_t type
            , flag32_t flags)
    : f_name(name)
    , f_type(type)
    , f_flags(flags)
    , f_schema_table(table)
{
}


void schema_column::from_structure(structure::pointer_t s)
{
    f_name = s->get_string("name");
    f_column_id = s->get_uinteger("column_id");
    f_type = static_cast<struct_type_t>(s->get_uinteger("type"));
    f_flags = s->get_uinteger("flags");
    f_encrypt_key_name = s->get_string("encrypt_key_name");
    f_default_value = s->get_buffer("default_value");
    f_minimum_value = s->get_buffer("minimum_value");
    f_maximum_value = s->get_buffer("maximum_value");
    f_minimum_length = s->get_uinteger("minimum_length");
    f_maximum_length = s->get_uinteger("maximum_length");
    f_validation = s->get_buffer("validation");
}


/** \brief Return true if this column represents the "expiration_date" column.
 *
 * This function checks the name of the column. If the name is
 * "expiration_date", then the function returns true.
 *
 * \return true when the column name is "expiration_date".
 */
bool schema_column::is_expiration_date_column() const
{
    return f_name == g_expiration_date;
}


compare_t schema_column::compare(schema_column const & rhs) const
{
    compare_t result(compare_t::COMPARE_SCHEMA_EQUAL);

    if(f_name != rhs.f_name)
    {
        throw snapdatabase_logic_error(
                  "the schema_column::compare() function can only be called"
                  " with two columns having the same name. You called it"
                  " with a column named \""
                + f_name
                + "\" and the other \""
                + rhs.f_name
                + "\".");
    }

    //f_column_id -- these are adjusted accordingly on a merge

    if(f_type != rhs.f_type)
    {
        return compare_t::COMPARE_SCHEMA_DIFFER;
    }

    // the LIMITED flag is just a display flag, it's really not important
    // still request for an update if changed by end user
    //
    if((f_flags & ~COLUMN_FLAG_LIMITED) != (rhs.f_flags & ~COLUMN_FLAG_LIMITED))
    {
        return compare_t::COMPARE_SCHEMA_DIFFER;
    }
    if(f_flags != rhs.f_flags)
    {
        result = compare_t::COMPARE_SCHEMA_UPDATE;
    }

    if(f_encrypt_key_name != rhs.f_encrypt_key_name)
    {
        return compare_t::COMPARE_SCHEMA_DIFFER;
    }

    if(f_default_value != rhs.f_default_value)
    {
        result = compare_t::COMPARE_SCHEMA_UPDATE;
    }

    if(f_minimum_value != rhs.f_minimum_value)
    {
        return compare_t::COMPARE_SCHEMA_DIFFER;
    }

    if(f_maximum_value != rhs.f_maximum_value)
    {
        return compare_t::COMPARE_SCHEMA_DIFFER;
    }

    if(f_minimum_length != rhs.f_minimum_length)
    {
        return compare_t::COMPARE_SCHEMA_DIFFER;
    }

    if(f_maximum_length != rhs.f_maximum_length)
    {
        return compare_t::COMPARE_SCHEMA_DIFFER;
    }

    // we can't do much better here, unfortunately
    // but if the script changes many things can be affected
    //
    if(f_validation != rhs.f_validation)
    {
        return compare_t::COMPARE_SCHEMA_DIFFER;
    }

    return result;
}


schema_table::pointer_t schema_column::table() const
{
    return f_schema_table.lock();
}


column_id_t schema_column::column_id() const
{
    return f_column_id;
}


void schema_column::set_column_id(column_id_t id)
{
    if(f_column_id != COLUMN_NULL)
    {
        throw id_already_assigned(
                  "This column already has an identifier ("
                + std::to_string(static_cast<int>(f_column_id))
                + "). You can't assigned it another one.");
    }

    f_column_id = id;
}


std::string schema_column::name() const
{
    return f_name;
}


struct_type_t schema_column::type() const
{
    return f_type;
}


flag32_t schema_column::flags() const
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


std::uint32_t schema_column::minimum_length() const
{
    return f_minimum_length;
}


std::uint32_t schema_column::maximum_length() const
{
    return f_maximum_length;
}


buffer_t schema_column::validation() const
{
    return f_validation;
}













void schema_sort_column::from_xml(xml_node::pointer_t sc)
{
    f_column_name = sc->attribute("name");
    if(f_column_name.empty())
    {
        throw invalid_xml(
                  "Sort column in a secondary index must have a name attribute.");
    }

    std::string const direction(sc->attribute("direction"));
    if(direction == "desc"
    || direction == "descending")
    {
        f_flags |= SCHEMA_SORT_COLUMN_DESCENDING;
    }
    else
    {
        f_flags &= ~SCHEMA_SORT_COLUMN_DESCENDING;
    }

    if(sc->attribute("not-null") == "not-null")
    {
        f_flags |= SCHEMA_SORT_COLUMN_NOT_NULL;
    }
    else
    {
        f_flags &= ~SCHEMA_SORT_COLUMN_NOT_NULL;
    }

    std::string const code(sc->text());
    if(!code.empty())
    {
        f_function = compile_script(code);
    }
    else
    {
        f_function.clear();
    }

    std::string const length(sc->attribute("length"));
    if(!length.empty())
    {
        f_length = convert_to_uint(length, 32);
        if(f_length <= 0)
        {
            SNAP_LOG_WARNING
                << "The length of a sort column must be at least 1. "
                << f_length
                << "\" is not acceptable."
                << SNAP_LOG_SEND;

            f_length = SCHEMA_SORT_COLUMN_DEFAULT_LENGTH;
        }
    }
    else
    {
        f_length = SCHEMA_SORT_COLUMN_DEFAULT_LENGTH;
    }
}


compare_t schema_sort_column::compare(schema_sort_column const & rhs) const
{
    compare_t result(compare_t::COMPARE_SCHEMA_EQUAL);

    // ignore those, it doesn't get saved, the binary version has an empty
    // name (you can get the name using the column identifier instead
    //if(f_column_name != rhs.f_column_name)
    //{
    //    return compare_t::COMPARE_SCHEMA_DIFFER;
    //}

    if(f_column_id != rhs.f_column_id)
    {
        return compare_t::COMPARE_SCHEMA_DIFFER;
    }

    if(f_flags != rhs.f_flags)
    {
        return compare_t::COMPARE_SCHEMA_DIFFER;
    }

    if(f_length != rhs.f_length)
    {
        return compare_t::COMPARE_SCHEMA_DIFFER;
    }

    if(f_function != rhs.f_function)
    {
        return compare_t::COMPARE_SCHEMA_DIFFER;
    }

    return result;
}


std::string schema_sort_column::get_column_name() const
{
    return f_column_name;
}


column_id_t schema_sort_column::get_column_id() const
{
    return f_column_id;
}


void schema_sort_column::set_column_id(column_id_t column_id)
{
    f_column_id = column_id;
}


flag32_t schema_sort_column::get_flags() const
{
    return f_flags;
}


void schema_sort_column::set_flags(flag32_t flags)
{
    f_flags = flags;
}


bool schema_sort_column::is_ascending() const
{
    return (f_flags & SCHEMA_SORT_COLUMN_DESCENDING) == 0;
}


bool schema_sort_column::accept_null_columns() const
{
    return (f_flags & SCHEMA_SORT_COLUMN_NOT_NULL) == 0;
}


std::uint32_t schema_sort_column::get_length() const
{
    return f_length;
}


void schema_sort_column::set_length(std::uint32_t length)
{
    f_length = length;
}


buffer_t schema_sort_column::get_function() const
{
    return f_function;
}


void schema_sort_column::set_function(buffer_t const & function)
{
    f_function = function;
}






void schema_secondary_index::from_xml(xml_node::pointer_t si)
{
    f_index_name = si->attribute("name");

    index_type_t const type(index_name_to_index_type(f_index_name));
    switch(type)
    {
    case index_type_t::INDEX_TYPE_SECONDARY:
        break;

    case index_type_t::INDEX_TYPE_INVALID:
        throw invalid_xml(
                  "\""
                + f_index_name
                + "\" is not a valid secondary index name.");

    default:
        // this is very important since we will not otherwise notice
        // the duplication and it would break the rest of the database
        // functionality
        //
        throw invalid_xml(
                  "\""
                + f_index_name
                + "\" is a reserved index name, which can't be used as a secondary index name.");

    }

    std::string const distributed(si->attribute("distributed"));
    if(distributed.empty() || distributed == "distributed")
    {
        f_flags |= SECONDARY_INDEX_FLAG_DISTRIBUTED;
    }
    else if(distributed == "one-instance")
    {
        f_flags &= ~SECONDARY_INDEX_FLAG_DISTRIBUTED;
    }
    else
    {
        SNAP_LOG_WARNING
            << "Unknown distributed attribute value \""
            << distributed
            << "\" within a <secondary-index> tag ignored."
            << SNAP_LOG_SEND;

        // use the default when invalid
        //
        f_flags |= SECONDARY_INDEX_FLAG_DISTRIBUTED;
    }

    for(auto child(si->first_child());
        child != nullptr;
        child = child->next())
    {
        if(child->tag_name() == "order")
        {
            for(auto column_names(child->first_child());
                column_names != nullptr;
                column_names = column_names->next())
            {
                if(column_names->tag_name() == "column-name")
                {
                    schema_sort_column::pointer_t sort_column(std::make_shared<schema_sort_column>());
                    sort_column->from_xml(column_names);
                    f_sort_columns.push_back(sort_column); // vector because these are sorted by user

                    //std::string const name(sort_column->get_name());
                    //if(f_columns_by_name.find(name()) != f_columns_by_name.end())
                    //{
                    //    throw invalid_xml(
                    //              "Column \""
                    //            + c->name()
                    //            + "\" not found in index \"");
                    //            + f_index_name
                    //            + "\"."
                    //}
                }
            }
        }
        else if(child->tag_name() == "filter")
        {
            std::string const code(child->text());
            if(!code.empty())
            {
                f_filter = compile_script(code);
            }
            else
            {
                f_filter.clear();
            }
        }
    }
}


compare_t schema_secondary_index::compare(schema_secondary_index const & rhs) const
{
    compare_t result(compare_t::COMPARE_SCHEMA_EQUAL);

    if(f_index_name != rhs.f_index_name)
    {
        throw snapdatabase_logic_error(
                  "the schema_secondary_index::compare() function can only be"
                  " called with two secondary indexes having the same name."
                  " You called it with a column named \""
                + f_index_name
                + "\" and the other \""
                + rhs.f_index_name
                + "\".");
    }

    size_t const lhs_max(get_column_count());
    size_t const rhs_max(rhs.get_column_count());
    if(lhs_max != rhs_max)
    {
        return compare_t::COMPARE_SCHEMA_DIFFER;
    }

    for(size_t idx(0); idx < lhs_max; ++idx)
    {
        compare_t const sc_compare(f_sort_columns[idx]->compare(*rhs.f_sort_columns[idx]));
        if(sc_compare == compare_t::COMPARE_SCHEMA_DIFFER)
        {
            return compare_t::COMPARE_SCHEMA_DIFFER;
        }
        else if(sc_compare == compare_t::COMPARE_SCHEMA_UPDATE)
        {
            result = compare_t::COMPARE_SCHEMA_UPDATE;
        }
    }

    if(f_filter != rhs.f_filter)
    {
        return compare_t::COMPARE_SCHEMA_DIFFER;
    }

    if(f_flags != rhs.f_flags)
    {
        return compare_t::COMPARE_SCHEMA_DIFFER;
    }

    return result;
}


std::string schema_secondary_index::get_index_name() const
{
    return f_index_name;
}


void schema_secondary_index::set_index_name(std::string const & index_name)
{
    f_index_name = index_name;
}


flag32_t schema_secondary_index::get_flags() const
{
    return f_flags;
}


void schema_secondary_index::set_flags(flag32_t flags)
{
    f_flags = flags;
}


bool schema_secondary_index::get_distributed_index() const
{
    return (f_flags & SECONDARY_INDEX_FLAG_DISTRIBUTED) != 0;
}


void schema_secondary_index::set_distributed_index(bool distributed)
{
    if(distributed)
    {
        f_flags |= SECONDARY_INDEX_FLAG_DISTRIBUTED;
    }
    else
    {
        f_flags &= ~SECONDARY_INDEX_FLAG_DISTRIBUTED;
    }
}


size_t schema_secondary_index::get_column_count() const
{
    return f_sort_columns.size();
}


schema_sort_column::pointer_t schema_secondary_index::get_sort_column(int idx) const
{
    if(static_cast<size_t>(idx) >= f_sort_columns.size())
    {
        throw snapdatabase_out_of_range(
                  "Index ("
                + std::to_string(idx)
                + ") is too large to pick a sort column from secondary index \""
                + f_index_name
                + "\".");
    }

    return f_sort_columns[idx];
}


void schema_secondary_index::add_sort_column(schema_sort_column::pointer_t sc)
{
    f_sort_columns.push_back(sc);
}


buffer_t schema_secondary_index::get_filter() const
{
    return f_filter;
}


void schema_secondary_index::set_filter(buffer_t const & filter)
{
    f_filter = filter;
}












schema_table::schema_table()
{
    f_block_size = dbfile::get_system_page_size();
}


void schema_table::set_complex_types(schema_complex_type::map_pointer_t complex_types)
{
    f_complex_types = complex_types;
}


void schema_table::from_xml(xml_node::pointer_t x)
{
    if(x->tag_name() != "table")
    {
        throw invalid_xml(
                  "A table schema must be a \"keyspaces\" or \"context\". \""
                + x->tag_name()
                + "\" is not acceptable.");
    }

    // start at version 1.0
    //
    f_version.set_major(1);

    f_name = x->attribute("name");
    if(!validate_name(f_name))
    {
        throw invalid_xml(
                  "\""
                + f_name
                + "\" is not a valid table name.");
    }

    bool drop(x->attribute("drop") == "drop");
    if(drop)
    {
        // do not ever save a table when the DROP flag is set (actually
        // we want to delete the entire folder if it still exists!)
        //
        f_flags |= TABLE_FLAG_DROP;
        return;
    }

    if(x->attribute("sparse") == "sparse")
    {
        f_flags |= TABLE_FLAG_SPARSE;
    }

    if(x->attribute("secure") == "secure")
    {
        f_flags |= TABLE_FLAG_SECURE;
    }

    std::string const track(x->attribute("track"));
    advgetopt::string_list_t track_flags;
    advgetopt::split_string(track, track_flags, {","});
    for(auto tf : track_flags)
    {
        if(tf == "create")
        {
            f_flags |= TABLE_FLAG_TRACK_CREATE;
        }
        else if(tf == "update")
        {
            f_flags |= TABLE_FLAG_TRACK_UPDATE;
        }
        else if(tf == "delete")
        {
            f_flags |= TABLE_FLAG_TRACK_DELETE;
        }
        else
        {
            SNAP_LOG_WARNING
                << "Unknown track flag \""
                << tf
                << "\" within <table name=\""
                << f_name
                << "\" track=\"...\" ...> tag."
                << SNAP_LOG_SEND;
        }
    }

    xml_node::deque_t schemata;
    xml_node::deque_t secondary_indexes;

    f_model = name_to_model(x->attribute("model"));

    // 1. look for block-size, description, schema, and secondary-index tags
    //
    // here we only process the block-size and description
    // the other tags, we just keep a list of them and manage them later
    //
    for(auto child(x->first_child()); child != nullptr; child = child->next())
    {
        if(child->tag_name() == "block-size")
        {
            f_block_size = convert_to_uint(child->text(), 32);

            // TBD--we adjust the size in dbfile
            //size_t const page_size(dbfile::get_system_page_size());
            //if((f_block_size % page_size) != 0)
            //{
            //    throw invalid_xml(
            //              "Table \""
            //            + f_name
            //            + "\" is not compatible, block size "
            //            + std::to_string(f_block_size)
            //            + " is not supported because it is not an exact multiple of "
            //            + std::to_string(page_size)
            //            + ".");
            //}
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

    // 2. add system columns
    //

    // schema version -- to know which schema to use to read the data
    //
    // this one is managed as a very special case instead; the version
    // is saved as the first 4 bytes of any one row; plus on a read we
    // always auto-convert the data to the latest schema version so
    // having such a column would not be useful (i.e. it would always
    // be the exact same value as far as the end user is concerned)
    //
    //{
    //    auto c(std::make_shared<schema_column>(
    //                  shared_from_this()
    //                , "_schema_version"
    //                , struct_type_t::STRUCT_TYPE_VERSION
    //                , COLUMN_FLAG_REQUIRED | COLUMN_FLAG_SYSTEM));
    //
    //    f_columns_by_name[c->name()] = c;
    //}

    // object identifier -- to place the rows in our indirect index
    {
        auto c(std::make_shared<schema_column>(
                      shared_from_this()
                    , "_oid"
                    , struct_type_t::STRUCT_TYPE_OID
                    , COLUMN_FLAG_REQUIRED | COLUMN_FLAG_SYSTEM));

        f_columns_by_name[c->name()] = c;
    }

    // date when the row was created
    {
        auto c(std::make_shared<schema_column>(
                      shared_from_this()
                    , "_created_on"
                    , struct_type_t::STRUCT_TYPE_USTIME
                    , COLUMN_FLAG_SYSTEM));

        f_columns_by_name[c->name()] = c;
    }

    // when the row was last updated
    {
        auto c(std::make_shared<schema_column>(
                      shared_from_this()
                    , "_last_updated"
                    , struct_type_t::STRUCT_TYPE_USTIME
                    , COLUMN_FLAG_REQUIRED | COLUMN_FLAG_SYSTEM));

        f_columns_by_name[c->name()] = c;
    }

    // the date when it gets deleted automatically
    {
        auto c(std::make_shared<schema_column>(
                      shared_from_this()
                    , "_deleted_on"
                    , struct_type_t::STRUCT_TYPE_USTIME
                    , COLUMN_FLAG_SYSTEM));

        f_columns_by_name[c->name()] = c;
    }

    // ID of user who created this row
    {
        auto c(std::make_shared<schema_column>(
                      shared_from_this()
                    , "_created_by"
                    , struct_type_t::STRUCT_TYPE_UINT64
                    , COLUMN_FLAG_SYSTEM));

        f_columns_by_name[c->name()] = c;
    }

    // ID of user who last updated this row
    {
        auto c(std::make_shared<schema_column>(
                      shared_from_this()
                    , "_last_updated_by"
                    , struct_type_t::STRUCT_TYPE_UINT64
                    , COLUMN_FLAG_SYSTEM));

        f_columns_by_name[c->name()] = c;
    }

    // ID of user who deleted this row
    {
        auto c(std::make_shared<schema_column>(
                      shared_from_this()
                    , "_deleted_by"
                    , struct_type_t::STRUCT_TYPE_UINT64
                    , COLUMN_FLAG_SYSTEM));

        f_columns_by_name[c->name()] = c;
    }

    // version of the data in this row
    //
    // --------------------------------------------------------- resume -----
    //
    // The implementation is most TBD TBD TBD still. The following is my
    // current talk about it. However, I think I got most of it laid out
    // as I think it will be easiest: use the version + language to generate
    // keys in separate branch and revision specific indexes, which is very
    // similar to what I've done in Cassandra. We also probably need two
    // fields: one to read a specific version and one to write a revision
    // which would get automatically updated to a new branch and/or revision.
    //
    // See also:
    // void row::generate_mumur3(buffer_t & murmur3, version_t version, std::string const language);
    //
    // ------------------------------------------------ long discussion -----
    //
    // How this will be implemented is not clear at this point--it will
    // only be for the `content` table (Note: in our Cassandra implementation
    // we had a `content`, a `branch` and a `revision` table);
    //
    // The version itself would not be saved as a column per se, instead
    // it would be a form of sub-index where the type of column defines
    // how a read is handle based on the version:
    //
    // `global` -- the version is ignored for all global fields
    // `branch` -- the fields are assigned the `major` version; so when the
    // version is 1.1 or 1.100, the data returned is the same; however, you
    // have two separate values for versions 1.55 and 3.2
    // `revision` -- the fields are assigned the full version
    // (`major.minor`); each piece of data depends 100% on the version
    //
    // So on a `commit()`, global fields are always overwritten, branches
    // are overwritten on a per `major` version and revisions only on a
    // per `major.minor` version.
    //
    // As far as the client is concerned, though, such rows have a version
    // column which clearly defines each column's value
    //
    // The `_version` in a row can be set to an existing version in the row.
    // If not defined, then no branch or revision are created at all. If
    // set to version `0.0`, then that means create a new revision in the
    // latest existing branch (i.e. no revision 0 exists, it's either `0.1`
    // or undefined). At this point, I do not have a good idea to also force
    // the creation of a new branch, unless we convert this field to a string.
    // Then we can use all sorts of characters for the purpose (which means
    // we may want two fields--one for write as a string and one for read):
    //
    // 1. `*.1` -- create a new branch with a first revision 1
    // 2. `L.*` -- create a new revision in the [L]atest branch
    // 3. `L.L` -- overwrite/update the latest branch and revision fields
    // 4. `0.*` -- create a new revision in specified branch (here branch `0`)
    //
    // **IMPORTANT:** The revision also makes use of a language. If the
    // "_language" column is not defined, then use "xx" as the default.
    //
    // Implementation Ideas: (right now I think #3 is what we must use)
    //
    // 1. add the major version along the column ID when saving a branch
    //    value (ID:major:value); in effect we end up with many more columns
    //    for the one same row, only we just read those that have a major
    //    that matches the `_version` field; similarly, the revision is
    //    defined as column ID, major, minor (ID:major:minor:value)
    // 2. the row has a `reference_t` to a "branch array"; that array is
    //    a set of `reference_t` that point to all the columns specific to
    //    that branch, the `major` version is the index in that table (we
    //    have a map, though (`major => reference_t`) so that way older
    //    branches can be deleted if/when necessary; the revisions would be
    //    managed in a similar way; the main row has a reference to an array
    //    which has a map defined as `major:minor:language => reference_t`
    //    and the reference points to all the columns assigned that specific
    //    revision
    // 3. full fledge indexes which make use of the row key + major version
    //    for branches and row key + major + minor version + language for
    //    revision and add two more indexes in our headers just for those two;
    //    NOTE: with a full fledge index we can distribute the data
    //    between computers; whether we want to do that is still TBD
    //
    // The main problem with (1) is that one row will grow tremendously and
    // that will probably be impossible to manage after a while (on reads as
    // well as cheer size of the row). (2) is great although it certainly
    // requires a lot more specialized management to maintain the arrays.
    // (3) is probably the best since we should be able to reuse much of the
    // code handling indexes which will just be the standard row key plus
    // the necessary version info (major or major:minor). This is what we
    // have in Cassandra although we have to manually handle all the revisions
    // in our Snap! code.
    //
    {
        auto c(std::make_shared<schema_column>(
                      shared_from_this()
                    , "_version"
                    , struct_type_t::STRUCT_TYPE_VERSION
                    , COLUMN_FLAG_SYSTEM));

        f_columns_by_name[c->name()] = c;
    }

    // language code used in the "body", "title", etc.
    //
    // By default, we use a 2 letter code ISO-639-1 code, but this field
    // allows for any ISO encoding such as "en-us" and any macro language.
    // The low level system implementation doesn't care and won't verify
    // that the language is valid. We offer higher level functions to do
    // so if you'd like to verify before letting a user select a language.
    //
    // Use "xx" for an entry in a table that uses languages but does not
    // require one. Not defining the field means that no language is
    // specified. The system will automatically use "xx" for revisions.
    //
    {
        auto c(std::make_shared<schema_column>(
                      shared_from_this()
                    , "_language"
                    , struct_type_t::STRUCT_TYPE_P8STRING
                    , COLUMN_FLAG_SYSTEM));

        f_columns_by_name[c->name()] = c;
    }

    // current revision
    //
    // this is another entry in link with the branch/revision concept, we
    // need to display a page, we need to have a current version to display
    // that page; problem here is we need one such version per language
    //
    // in the old Cassandra database we also have a latest version, which
    // is also per language; this latest version gets used to create new
    // revisions effectively
    //
    // finally, we had a 'last edited version' because if you were to edit
    // and not save your editing, we wanted to save a version of the page
    // attached to your user and that was a form of "floating" version
    // (i.e. it was not yet assigned a full version/language pair)
    //
    // for now I leave this at that, but I think we'll need several more
    // fields to manage the whole set of possibilities (although things
    // such as the last edited page is per user so we can't just have one
    // field? well... maybe we track the last 100 edits and delete anything
    // that's too old and was not properly saved after that) -- the editing
    // versions can be called "draft"; which could also make use of the
    // language field to distinguish them: "<major>.<minor>::<language>-draft"
    //
    {
        auto c(std::make_shared<schema_column>(
                      shared_from_this()
                    , "_current_version"
                    , struct_type_t::STRUCT_TYPE_VERSION
                    , COLUMN_FLAG_SYSTEM));

        f_columns_by_name[c->name()] = c;
    }

    // "_expiration_date" -- we actually do not need an expiration date
    // column, the user can create his own "expiration_date" column which
    // will automatically get picked up by the system; i.e. rows with
    // a column with that name will automatically be added to the expiration
    // index, nothing more to do and the programmer has the ability to choose
    // the precision and what the value should be (it's just like a standard
    // column) -- see is_expiration_date_column()

    // 3. parse user columns
    //

    // Note: we need all the columns and eventually the schema from the
    //       existing table before we can assign the column identifiers;
    //       see the assign_column_ids() function for details
    //
    for(auto const & child : schemata)
    {
        process_columns(child);
    }

    // 4. the row-key is transformed in an array of column names
    //
    // the parameter in the XML is a string of column names separated
    // by commas
    //
    std::string const row_key_name(x->attribute("row-key"));
    advgetopt::split_string(row_key_name, f_row_key_names, {","});
    if(f_row_key_names.empty())
    {
        throw invalid_xml(
                  "A table schema must have a \"row-key\". \""
                + f_name
                + "\" is not acceptable.");
    }

    // 5. handle the secondary indexes
    //
    process_secondary_indexes(secondary_indexes);
}


void schema_table::load_extension(xml_node::pointer_t e)
{
    xml_node::deque_t secondary_indexes;

    for(auto child(e->first_child()); child != nullptr; child = child->next())
    {
        if(child->tag_name() == "schema")
        {
            process_columns(child);
        }
        else if(child->tag_name() == "secondary-index")
        {
            secondary_indexes.push_back(child);
        }
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

    process_secondary_indexes(secondary_indexes);
}


void schema_table::process_columns(xml_node::pointer_t column_definitions)
{
    for(auto column(column_definitions->first_child());
        column != nullptr;
        column = column->next())
    {
        auto c(std::make_shared<schema_column>(shared_from_this(), column));
        if(f_columns_by_name.find(c->name()) != f_columns_by_name.end())
        {
            SNAP_LOG_WARNING
                << "Column \""
                << f_name
                << '.'
                << c->name()
                << "\" defined twice. Second definition ignored."
                << SNAP_LOG_SEND;
            continue;
        }
        f_columns_by_name[c->name()] = c;
    }
}


void schema_table::process_secondary_indexes(xml_node::deque_t secondary_indexes)
{
    for(auto const & si : secondary_indexes)
    {
        schema_secondary_index::pointer_t index(std::make_shared<schema_secondary_index>());
        index->from_xml(si);
        f_secondary_indexes[index->get_index_name()] = index;
    }
}


/** \brief Compare two schema tables.
 *
 * This operator let you know whether two schema descriptions are considered
 * equal or not.
 *
 * The compare ignores some fields and flags because equality implies that
 * the content of the table, as in the data being inserted, selected,
 * updated, and deleted is not going to be different between the two
 * different schema_table descriptions. However, we still want to overwrite
 * the newest version with the new version if it has some differences.
 *
 * The return value tells you whether some differences
 * (COMPARE_SCHEMA_UPDATE), or important changes (COMPARE_SCHEMA_DIFFER)
 * where found. If the schemas as the exact same, then the function says
 * they are equal (COMPARE_SCHEMA_EQUAL). Note that in most cases, we expect
 * the function to return COMPARE_SCHEMA_EQUAL since schemata should rarely
 * change.
 *
 * \param[in] rhs  The righ hand side to compare this schema.
 *
 * \return One of the compare_t enumeration values.
 */
compare_t schema_table::compare(schema_table const & rhs) const
{
    compare_t result(compare_t::COMPARE_SCHEMA_EQUAL);

    //f_version -- we calculate the version
    //f_added_on -- this is dynamically assigned on creation

    if(f_name != rhs.f_name)
    {
        return compare_t::COMPARE_SCHEMA_DIFFER;
    }

    if(f_flags != rhs.f_flags)
    {
        return compare_t::COMPARE_SCHEMA_DIFFER;
    }

    if(f_model != rhs.f_model)
    {
        result = compare_t::COMPARE_SCHEMA_UPDATE;
    }

    if(f_block_size != rhs.f_block_size)
    {
        throw id_missing(
              "Block size cannot currently be changed. Please restore to "
            + std::to_string(f_block_size)
            + " instead of "
            + std::to_string(rhs.f_block_size)
            + ".");
    }

    if(f_row_key != rhs.f_row_key)
    {
        return compare_t::COMPARE_SCHEMA_DIFFER;
    }

    for(schema_secondary_index::map_t::const_iterator
            it(f_secondary_indexes.cbegin());
            it != f_secondary_indexes.cend();
            ++it)
    {
        schema_secondary_index::pointer_t rhs_secondary_index(rhs.secondary_index(it->first));
        if(rhs_secondary_index == nullptr)
        {
            return compare_t::COMPARE_SCHEMA_DIFFER;
        }
        compare_t const r(it->second->compare(*rhs_secondary_index));
        if(r == compare_t::COMPARE_SCHEMA_DIFFER)
        {
            return compare_t::COMPARE_SCHEMA_DIFFER;
        }
        if(r == compare_t::COMPARE_SCHEMA_UPDATE)
        {
            result = compare_t::COMPARE_SCHEMA_UPDATE;
        }
    }

    // loop through the RHS in case we removed a secondary index
    //
    for(schema_secondary_index::map_t::const_iterator
            it(rhs.f_secondary_indexes.cbegin());
            it != rhs.f_secondary_indexes.cend();
            ++it)
    {
        if(secondary_index(it->first) == nullptr)
        {
            return compare_t::COMPARE_SCHEMA_DIFFER;
        }
    }

    //f_columns_by_id -- we only have to compare one map
    //                   and at this point f_columns_by_id is expected to be
    //                   empty still
    //
    for(schema_column::map_by_name_t::const_iterator
            it(f_columns_by_name.cbegin());
            it != f_columns_by_name.cend();
            ++it)
    {
        schema_column::pointer_t rhs_column(rhs.column(it->first));
        if(rhs_column == nullptr)
        {
            // we could not find that column in the other schema,
            // so it's different
            //
            // TODO: make sure "renamed" columns are handled properly
            //       once we add that feature
            //
            return compare_t::COMPARE_SCHEMA_DIFFER;
        }
        compare_t r(it->second->compare(*rhs_column));
        if(r == compare_t::COMPARE_SCHEMA_DIFFER)
        {
            return compare_t::COMPARE_SCHEMA_DIFFER;
        }
        if(r == compare_t::COMPARE_SCHEMA_UPDATE)
        {
            result = compare_t::COMPARE_SCHEMA_UPDATE;
        }
    }

    // loop through the RHS in case we removed a column
    //
    for(schema_column::map_by_name_t::const_iterator
            it(rhs.f_columns_by_name.cbegin());
            it != rhs.f_columns_by_name.cend();
            ++it)
    {
        if(column(it->first) == nullptr)
        {
            // we could not find that column in the new schema,
            // so it's different
            //
            // TODO: make sure "renamed" columns are handled properly
            //       once we add that feature
            //
            return compare_t::COMPARE_SCHEMA_DIFFER;
        }
    }

    //f_description -- totally ignored; that's just noise

    return result;
}


void schema_table::from_binary(virtual_buffer::pointer_t b)
{
    structure::pointer_t s(std::make_shared<structure>(g_table_description));

    s->set_virtual_buffer(b, 0);

    f_version  = s->get_uinteger("schema_version");
    f_added_on = s->get_integer("added_on");
    f_name     = s->get_string("name");
    f_flags    = s->get_uinteger("flags");
    f_model    = static_cast<model_t>(s->get_uinteger("model"));

    {
        auto const field(s->get_field("row_key"));
        auto const max(field->size());
        for(std::remove_const<decltype(max)>::type idx(0); idx < max; ++idx)
        {
            f_row_key.push_back((*field)[idx]->get_uinteger("column_id"));
        }
    }

    {
        auto const field(s->get_field("secondary_indexes"));
        auto const max(field->size());
        for(std::remove_const<decltype(max)>::type idx(0); idx < max; ++idx)
        {
            schema_secondary_index::pointer_t secondary_index(std::make_shared<schema_secondary_index>());

            secondary_index->set_index_name((*field)[idx]->get_string("name"));
            secondary_index->set_flags((*field)[idx]->get_uinteger("flags"));

            auto const columns_field((*field)[idx]->get_field("sort_columns"));
            auto const columns_max(columns_field->size());
            for(std::remove_const<decltype(columns_max)>::type j(0); j < columns_max; ++j)
            {
                schema_sort_column::pointer_t sc(std::make_shared<schema_sort_column>());
                sc->set_column_id((*columns_field)[j]->get_uinteger("column_id"));
                sc->set_flags((*columns_field)[j]->get_uinteger("flags"));
                sc->set_length((*columns_field)[j]->get_uinteger("length"));
                sc->set_function((*columns_field)[j]->get_buffer("function"));
                secondary_index->add_sort_column(sc);
            }

            secondary_index->set_filter((*field)[idx]->get_buffer("filter"));

            f_secondary_indexes[secondary_index->get_index_name()] = secondary_index;
        }
    }

    {
        auto field(s->get_field("columns"));
        auto const max(field->size());
        for(std::remove_const<decltype(max)>::type idx(0); idx < max; ++idx)
        {
            schema_column::pointer_t column(std::make_shared<schema_column>(shared_from_this(), (*field)[idx]));
            if(column->column_id() == 0)
            {
                throw id_missing(
                      "loaded column \""
                    + column->name()
                    + "\" from the database and its column identifier is 0.");
            }

            f_columns_by_name[column->name()] = column;
            f_columns_by_id[column->column_id()] = column;
        }
    }
}


virtual_buffer::pointer_t schema_table::to_binary() const
{
    structure::pointer_t s(std::make_shared<structure>(g_table_description));
    s->init_buffer();
    s->set_uinteger("schema_version", f_version.to_binary());
    s->set_integer("added_on", f_added_on);
    s->set_string("name", f_name);
    s->set_uinteger("flags", f_flags);
    s->set_uinteger("block_size", f_block_size);
    s->set_uinteger("model", static_cast<uint8_t>(f_model));

    {
        auto const max(f_row_key.size());
        for(std::remove_const<decltype(max)>::type i(0); i < max; ++i)
        {
            structure::pointer_t column_id_structure(s->new_array_item("row_key"));
            column_id_structure->set_uinteger("column_id", f_row_key[i]);
        }
    }

    {
        for(auto it(f_secondary_indexes.cbegin());
                 it != f_secondary_indexes.cend();
                 ++it)
        {
            structure::pointer_t secondary_index_structure(s->new_array_item("secondary_indexes"));
            secondary_index_structure->set_string("name", it->second->get_index_name());
            secondary_index_structure->set_uinteger("flags", it->second->get_flags());
            secondary_index_structure->set_buffer("filter", it->second->get_filter());

            auto const jmax(it->second->get_column_count());
            for(std::remove_const<decltype(jmax)>::type j(0); j < jmax; ++j)
            {
                structure::pointer_t sort_column_structure(secondary_index_structure->new_array_item("sort_columns"));
                schema_sort_column::pointer_t sc(it->second->get_sort_column(j));
                sort_column_structure->set_uinteger("column_id", sc->get_column_id());
                sort_column_structure->set_uinteger("flags", sc->get_flags());
                sort_column_structure->set_uinteger("length", sc->get_length());
                sort_column_structure->set_buffer("function", sc->get_function());
            }
        }
    }

    {
        for(auto const & col : f_columns_by_id)
        {
            structure::pointer_t column_description(s->new_array_item("columns"));
            column_description->set_string("name", col.second->name());
            column_description->set_uinteger("column_id", col.second->column_id());
            column_description->set_uinteger("type", static_cast<uint16_t>(col.second->type()));
            column_description->set_uinteger("flags", col.second->flags());
            column_description->set_string("encrypt_key_name", col.second->encrypt_key_name());
            column_description->set_buffer("default_value", col.second->default_value());
            column_description->set_buffer("minimum_value", col.second->minimum_value());
            column_description->set_buffer("maximum_value", col.second->maximum_value());
            column_description->set_uinteger("minimum_length", col.second->minimum_length());
            column_description->set_uinteger("maximum_length", col.second->maximum_length());
            column_description->set_buffer("validation", col.second->validation());
        }
    }

    // we know it is zero so we ignore that one anyay
    //
    uint64_t start_offset(0);
    return s->get_virtual_buffer(start_offset);
}


version_t schema_table::schema_version() const
{
    return f_version;
}


/** \brief Set the version of the schema.
 *
 * This function is used only internally to set the version of the schema.
 * By default, all schemata are assigned version 1.0 on a read. However,
 * it may later be determined that this is an updated version of the
 * schema for a given table. In that case, the table will know what its
 * current version is (i.e. the latest version of the schema in that
 * table). Using that version + 1 is going to determine the new schema
 * version for this table and that's what gets assigned here.
 *
 * There are no other reasons to set the schema version. So you most
 * certainly never ever need to call this function ever.
 *
 * \param[in] version  The new schema version.
 */
void schema_table::set_schema_version(version_t version)
{
    f_version = version;
}


time_t schema_table::added_on() const
{
    return f_added_on;
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


bool schema_table::track_create() const
{
    return (f_flags & TABLE_FLAG_TRACK_CREATE) != 0;
}


bool schema_table::track_update() const
{
    return (f_flags & TABLE_FLAG_TRACK_UPDATE) != 0;
}


bool schema_table::track_delete() const
{
    return (f_flags & TABLE_FLAG_TRACK_DELETE) != 0;
}


column_ids_t schema_table::row_key() const
{
    return f_row_key;
}


void schema_table::assign_column_ids(schema_table::pointer_t existing_schema)
{
    if(!f_columns_by_id.empty())
    {
        return;
    }

    // if we have an existing schema, the same columns must be given the
    // exact same identifier or else it would all break
    //
    if(existing_schema != nullptr)
    {
        for(auto c : f_columns_by_name)
        {
            if(c.second->column_id() != 0)
            {
                throw snapdatabase_logic_error(
                          "Column \""
                        + f_name
                        + "."
                        + c.second->name()
                        + "\" was already given an identifier: "
                        + std::to_string(static_cast<int>(c.second->column_id()))
                        + ".");
            }

            schema_column::pointer_t e(existing_schema->column(c.first));
            if(e != nullptr)
            {
                // keep the same identifier as in the source schema
                //
                c.second->set_column_id(e->column_id());
                f_columns_by_id[e->column_id()] = c.second;
            }
        }
    }

    // in case new columns were added, we want to give them a new identifier
    // also in case old columns were removed, we can reuse their identifier
    //
    // Note: that works because each row has a reference to to the schema
    //       that was used when we created it and that means the column
    //       identifiers will be attached to the correct column
    //
    column_id_t id(1);
    for(auto c : f_columns_by_name)
    {
        if(c.second->column_id() != 0)
        {
            continue;
        }

        while(f_columns_by_id.find(id) != f_columns_by_id.end())
        {
            ++id;
        }

        c.second->set_column_id(id);
        f_columns_by_id[id] = c.second;
        ++id;
    }

    // the identifiers can now be used to define the row keys
    //
    for(auto const & n : f_row_key_names)
    {
        if(n == g_oid_column)
        {
            throw invalid_xml(
                      "The \""
                    + n
                    + "\" column is not acceptable for the primary key since you have no control over its value.");
        }
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
        if(c->column_id() == 0)
        {
            throw snapdatabase_logic_error(
                      "Somehow column \""
                    + f_name
                    + "."
                    + n
                    + "\" still has no identifier.");
        }
        f_row_key.push_back(c->column_id());
    }

    // and the secondary indexes can also be defined
    //
    for(auto const & index : f_secondary_indexes)
    {
        size_t const max(index.second->get_column_count());
        for(size_t idx(0); idx < max; ++idx)
        {
            schema_sort_column::pointer_t sc(index.second->get_sort_column(idx));
            std::string const n(sc->get_column_name());
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
            if(c->column_id() == 0)
            {
                throw snapdatabase_logic_error(
                          "Somehow column \""
                        + f_name
                        + "."
                        + n
                        + "\" still has no identifier.");
            }
            sc->set_column_id(c->column_id());
        }
    }
}


/** \brief Whether this schema includes an expiration date.
 *
 * The "expiration_date" column is used to expire a row. If the date in that
 * column is less than `now` then the row is considered expired. The row will
 * not be returned to you and will eventually get removed from the database
 * by one of our backend processes.
 *
 * The "expiration_date" is optional and in most cases not defined. This
 * function returns true if that table has that column.
 *
 * \return true if the table has an expiration date column.
 */
bool schema_table::has_expiration_date_column() const
{
    return f_columns_by_name.find(g_expiration_date) != f_columns_by_name.end();
}


schema_column::pointer_t schema_table::expiration_date_column() const
{
    return column(g_expiration_date);
}


schema_column::pointer_t schema_table::column(std::string const & name) const
{
    auto it(f_columns_by_name.find(name));
    if(it == f_columns_by_name.end())
    {
        return schema_column::pointer_t();
    }
    return it->second;
}


schema_column::pointer_t schema_table::column(column_id_t id) const
{
    auto it(f_columns_by_id.find(id));
    if(it == f_columns_by_id.end())
    {
        return schema_column::pointer_t();
    }
    return it->second;
}


schema_column::map_by_id_t schema_table::columns_by_id() const
{
    return f_columns_by_id;
}


schema_column::map_by_name_t schema_table::columns_by_name() const
{
    return f_columns_by_name;
}


schema_secondary_index::pointer_t schema_table::secondary_index(std::string const & name) const
{
    auto const & it(f_secondary_indexes.find(name));
    if(it != f_secondary_indexes.end())
    {
        return it->second;
    }

    return schema_secondary_index::pointer_t();
}


schema_complex_type::pointer_t schema_table::complex_type(std::string const & name) const
{
    auto const & it(f_complex_types->find(name));
    if(it != f_complex_types->end())
    {
        return it->second;
    }

    return schema_complex_type::pointer_t();
}


std::string schema_table::description() const
{
    return f_description;
}


std::uint32_t schema_table::block_size() const
{
    return f_block_size;
}


void schema_table::schema_offset(reference_t offset)
{
    f_schema_offset = offset;
}


reference_t schema_table::schema_offset() const
{
    return f_schema_offset;
}



std::string const & expiration_date_column_name()
{
    return g_expiration_date;
}


} // namespace snapdatabase
// vim: ts=4 sw=4 et
