// Snap Websites Server -- read a table XML file and put definitions in structures
// Copyright (c) 2016-2019  Made to Order Software Corp.  All Rights Reserved
//
// https://snapwebsites.org/
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
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA


// self
//
#include "snapwebsites/snap_tables.h"


// snapwebsites
//
#include "snapwebsites/glob_dir.h"
#include "snapwebsites/log.h"


// snapdev lib
//
#include <snapdev/not_used.h>


// Qt lib
//
#include <QFile>
#include <QDomDocument>


// last include
//
#include <snapdev/poison.h>



namespace snap
{


namespace
{


struct model_name_value_t
{
    char const *            f_name;
    snap_tables::model_t    f_model;
};


model_name_value_t model_name_values[] =
{
    { "content", snap_tables::model_t::MODEL_CONTENT },
    { "data",    snap_tables::model_t::MODEL_DATA    },
    { "queue",   snap_tables::model_t::MODEL_QUEUE   },
    { "log",     snap_tables::model_t::MODEL_LOG     },
    { "session", snap_tables::model_t::MODEL_SESSION }
};



struct kind_name_value_t
{
    char const *            f_name;
    snap_tables::kind_t     f_kind;
};


kind_name_value_t kind_name_values[] =
{
    { "thrift",  snap_tables::kind_t::KIND_THRIFT },
    { "blob",    snap_tables::kind_t::KIND_BLOB   }
};


}
// no name namespace



/** \class snap_tables
 * \brief Describe one table from our XML files.
 *
 * Whenever you start a process that needs to access our Cassandra database
 * you can read the schema from our table XML files. These files describe
 * the model (i.e. how the table gets used) and the data in the table.
 *
 * The XML file includes descriptions, table names, data such as name/value
 * pairs, etc.
 *
 * We support several basic models. In the old days, we wanted to use sorted
 * columns to support sorted lists within our tables. In our newest models,
 * we instead use a name/value model for most tables. We also allow ourselves
 * to use more or less columns than were usable in thrift. This just allows
 * us to avoid having to handle complicated data concatenations.
 *
 * The old thrift model gave us what is viewed as three CQL columns:
 *
 * * `key` -- this is the key access a row, simile to an ID in a an SQL
 * database; it is always unique; the key is used by Cassandra to decide
 * on which set of machines the data will reside (i.e. if the murmur3 of
 * the key is XYZ, then it goes on the N machines that support the range
 * that includes XYZ.)
 *
 * * `column1` -- this "strangely" named column was used to sort values
 * within a table; this was important to gain access to many values.
 * However, the truth was that this would prevent `column1` from being
 * defined on multiple computers, so a row with many `column1` but a
 * single `key` would put all the data on a single set of computers.
 * Using such for an index can have some reverse effect and make the
 * database slow over time.
 *
 * * `value` -- the value of the variable named by `column1`. For example,
 * we had a `created_on` field; `created_on` would be in `column1` and
 * `value` would be the timestamp when that row was created.
 *
 * With CQL, `column1` is not required and since we want to shift to
 * using blobs instead of name/value pairs managed by CQL (because that's
 * just way too slow on large amount of pages) we can actually change
 * our table definitions to just two columns:
 *
 * * `key` -- as above, the key referencing this row (i.e. a URL at this
 * moment, although later we want to use integers, see the "tree" feature
 * where each name is transformed in a number along the way. So a domain
 * becomes a number, a URL becomes a number, a page is assigned a number,
 * etc.)
 *
 * * `value` -- the value is a blob; a blob is a serie of name/value pairs.
 * We still have to define the exact format and it probably needs to be
 * described somewhere else anyway; for sure I want to use binary for
 * anything which isn't text so that way we can easily save numbers without
 * any loss of precision.
 *
 * Finally, in the CQL realm it is possible to create secondary indexes.
 * This allows you to avoid the `ALLOW FILTERING` and thus make things
 * go faster on a `SELECT` against certain columns. For example, the
 * `indexes` table makes use of a secondary column for its `value` column.
 * That way we can search for pages that are indexed instead of only index
 * pages. (i.e. search from both sides.)
 */





void snap_tables::column_t::set_name(QString const & name)
{
    f_name = name;
}


QString const & snap_tables::column_t::get_name() const
{
    return f_name;
}


void snap_tables::column_t::set_type(dbutils::column_type_t type)
{
    f_type = type;
}


dbutils::column_type_t snap_tables::column_t::get_type() const
{
    return f_type;
}


void snap_tables::column_t::set_required(bool required)
{
    f_required = required;
}


bool snap_tables::column_t::get_required() const
{
    return f_required;
}


void snap_tables::column_t::set_description(QString const & description)
{
    f_description = description;
}


QString snap_tables::column_t::get_description() const
{
    return f_description;
}


bool snap_tables::column_t::has_default_value() const
{
    return f_has_default;
}


void snap_tables::column_t::set_default(QString const & default_value)
{
    // if you set the default value then this column is considered to
    // have a default value
    //
    f_has_default = true;

    f_default = default_value;
}


QString snap_tables::column_t::get_default() const
{
    return f_default;
}


void snap_tables::column_t::set_min_value(double const min)
{
    f_min_value = min;
}


double snap_tables::column_t::get_min_value() const
{
    return f_min_value;
}


void snap_tables::column_t::set_max_value(double const max)
{
    f_max_value = max;
}


double snap_tables::column_t::get_max_value() const
{
    return f_max_value;
}


void snap_tables::column_t::set_min_length(int64_t const min)
{
    f_min_length = min;
}


int64_t snap_tables::column_t::get_min_length() const
{
    return f_min_length;
}


void snap_tables::column_t::set_max_length(int64_t const max)
{
    f_max_length = max;
}


int64_t snap_tables::column_t::get_max_length() const
{
    return f_max_length;
}


void snap_tables::column_t::set_validation(QString const & validation)
{
    f_validation = validation;
}


QString snap_tables::column_t::get_validation() const
{
    return f_validation;
}


/** \brief Set whether the output of this column should be limited.
 *
 * When dealing with column that can end up being really large (over
 * 256 bytes), you should mark them as limited. That way we can
 * display the first \em few bytes and not cover 100 screens of data
 * which would not be useful to you.
 *
 * By default the entire value is printed (the limited flag is false.)
 *
 * \param[in] limited  Whether it should be limited or not.
 */
void snap_tables::column_t::set_limited(bool limited)
{
    f_limited = limited;
}


/** \brief Get whether the output should be limited or not.
 *
 * This function returns true if the output of this column should be
 * limited so as not to cover the entire screen with data which would
 * not be useful.
 *
 * You are welcome to ignore this flag.
 *
 * \return true if the limited flag was set to true, false otherwise.
 */
bool snap_tables::column_t::get_limited() const
{
    return f_limited;
}




/** \brief Set the secondary index name.
 *
 * This function saves the name of the secondary index. The name is not
 * required as it can be given a default name automatically.
 *
 * The default name is built from the table name, the column name, and the
 * word "index" at the end:
 *
 * \code
 *     set_name(table.get_name() + "_" + index.get_column_name() + "_index");
 * \endcode
 *
 * \param[in] name  The name of the secondary index.
 */
void snap_tables::secondary_index_t::set_name(QString const & name)
{
    f_name = name;
}


/** \brief Get the secondary index name.
 *
 * THis function returns the name of the secondary index. If the string is
 * empty, then the default name was used.
 *
 * \return The name of the secondary index or an empty string for the default.
 *
 * \sa set_name()
 */
QString const & snap_tables::secondary_index_t::get_name() const
{
    return f_name;
}


/** \brief Set the column name.
 *
 * This function is used to set the name of the column used to generate
 * the secondary index.
 *
 * Note that a secondary index can only have one column named in their
 * indexes. This allows for much faster read and write without locking
 * which is what NoSQL databases strive for.
 *
 * There is still a way to have a secondary index on multiple columns,
 * but you will be the one responsible for managing the contents of
 * these columns so that they get added to the database in a single
 * column.
 *
 * For example, you may want an extra index against a date and priority
 * pair of columns. Say you define the date as a Unix timestamp in
 * seconds and the priority is a number between 1 and 100. What you
 * can do is multiply the Unix timestamp by 100 and add the priority
 * minus one. That gives you both values in a single int64 value which
 * you can save in one column.
 *
 * If you can't merge the two parameters in such a way, you can always
 * use a binary buffer (a blob in Cassandra) and save the values one
 * after another in binary. Then it will sort everything as expected
 * in your single blob column.
 *
 * \param[in] column  The name of the column the index is for.
 */
void snap_tables::secondary_index_t::set_column(QString const & column)
{
    f_column = column;
}


/** \brief Get the column name.
 *
 * A secondary index in Cassandra supports a single column (i.e. you can
 * only index one column at a time.)
 *
 * \return The name of the column used as the index.
 *
 * \sa set_column()
 */
QString const & snap_tables::secondary_index_t::get_column() const
{
    return f_column;
}







/** \brief Define the name of this schema.
 *
 * Each table has a name. This name must be a valid Cassandra name.
 * Mainly, it has to start with a letter and be composed of
 * letters, digits, and underscores and not be a reserved keyword
 * such as `SELECT`, `INDEX` or `AND`. The name is not case sensitive
 * although it is customary to write it in lowercase only. (Cassandra
 * saves the name in lowercase anyway.)
 *
 * \param[in] name  The name of the table.
 *
 * \sa get_name()
 */
void snap_tables::table_schema_t::set_name(QString const & name)
{
    f_name = name;
}


/** \brief Retrieve the table name.
 *
 * This function returns the name of the table.
 *
 * See the set_name() about restrictions to the name of a table.
 *
 * \return The name of the table.
 *
 * \sa set_name()
 */
QString const & snap_tables::table_schema_t::get_name() const
{
    return f_name;
}


/** \brief Set the description.
 *
 * This function is used to save the table description.
 *
 * The description is just information about the table. It does not
 * get saved in Cassandra. It is expected to be useful to the developers.
 *
 * \param[in] description  The table description.
 */
void snap_tables::table_schema_t::set_description(QString const & description)
{
    f_description = description;
}


/** \brief Retrieve the description.
 *
 * This function returns the description of the table.
 *
 * \return The table description.
 */
QString const & snap_tables::table_schema_t::get_description() const
{
    return f_description;
}


/** \brief Set the model.
 *
 * This function is used to set the model of the table. The model defines how
 * the table is expected to be used in general terms. This is used to
 * improve the speed of the table when using it.
 *
 * The model is one of:
 *
 * \li MODEL_CONTENT
 *
 * The table is expected to hold content, a few writes and many reads.
 *
 * \li MODEL_DATA
 *
 * The data table is expected to be written to once and read many times.
 *
 * \li MODEL_QUEUE
 *
 * The table is to be used like a FIFO. If you have to use this model,
 * you probably need to rethink how your are using Cassandra. You
 * probably want to use a different technology to support queues.
 *
 * \li MODEL_LOG
 *
 * This model is like the MODEL_DATA only you do a single write and
 * some reads, but the table is really expected to be written to often.
 * Also no updates are ever expected.
 *
 * \li MODEL_SESSION
 *
 * The session model is expected to have many reads and writes and
 * especially a TTL to the data so it automatically gets deleted after
 * a while (i.e. sessions time out.)
 *
 * \param[in] model  The new table model.
 */
void snap_tables::table_schema_t::set_model(model_t model)
{
    f_model = model;
}


/** \brief Retrieve the model.
 *
 * This function returns the current table model.
 *
 * To transform the model to a string, you can use the model_to_string()
 * function.
 *
 * \return The model used by this table.
 */
snap_tables::model_t snap_tables::table_schema_t::get_model() const
{
    return f_model;
}


/** \brief Set the kind of table.
 *
 * This function is used to set the kind of table to be used. The
 * kind defines the CQL schema. We currently support two formats.
 * The old format is the thrift like format and the new format is
 * the blob format that allows us to save all the fields (columns)
 * in a single large blob which we can read at once instead of
 * reading the data one cell at a time.
 *
 * The kind is one of:
 *
 * \li KIND_THRIFT
 *
 * The table is created with a CQL schema as follow:
 *
 *     key blob,
 *     column1 blob,
 *     value blob,
 *     primary key (key, column1)
 *
 * \li KIND_BLOB
 *
 * The table is created with a CQL schema as follow:
 *
 *     key blob,
 *     value blob,
 *     primary key (key)
 *
 * \param[in] kind  The new table kind.
 *
 * \sa get_kind()
 */
void snap_tables::table_schema_t::set_kind(kind_t kind)
{
    f_kind = kind;
}


/** \brief Retrieve the kind.
 *
 * This function returns the current table kind.
 *
 * To transform the kind to a string, you can use the kind_to_string()
 * function.
 *
 * \return The kind used by this table.
 *
 * \sa set_kind()
 */
snap_tables::kind_t snap_tables::table_schema_t::get_kind() const
{
    return f_kind;
}


/** \brief Mark the table as being dropped.
 *
 * This function marks the table as dropped (true) or active (false).
 *
 * When set to true, the snapdbproxy will automatically drop that table.
 * This is done because just removing the corresponding `*-tables.xml`
 * would not tell us whether the table needs to be dropped. You may
 * remove a plugin from one machine and still have it on another
 * machine, for example. So instead of just remove the `*-tables.xml`
 * you need to change the setup and put `drop="drop"` in the table
 * definition.
 *
 * \param[in] drop  The new status of the drop flag.
 */
void snap_tables::table_schema_t::set_drop(bool drop)
{
    f_drop = drop;
}


/** \brief Check whether the table is marked as a drop table.
 *
 * This function returns true if the `drop="drop"` flag was specified
 * in the table definition.
 *
 * \return Whether the drop flag is set in the table definition.
 */
bool snap_tables::table_schema_t::get_drop() const
{
    return f_drop;
}


/** \brief Attach this column to the schema.
 *
 * This function saves the column in this schema. Columns have a name.
 * If you set a column that was already set, the old one gets
 * overwritten.
 *
 * \param[in] column  The column to add to this schema.
 */
void snap_tables::table_schema_t::set_column(column_t const & column)
{
    f_columns[column.get_name()] = column;
}


/** \brief Retrieve the list of columns.
 *
 * This function returns the map of columns that were added using the
 * set_column() function.
 *
 * \return The map of columns of this schema.
 */
snap_tables::column_t::map_t const & snap_tables::table_schema_t::get_columns() const
{
    return f_columns;
}


/** \brief Add a secondary index.
 *
 * This function adds a secondary index to this table.
 *
 * \param[in] index  The index to add to this table.
 *
 * \sa get_secondary_indexes()
 */
void snap_tables::table_schema_t::set_secondary_index(secondary_index_t const & index)
{
    f_secondary_indexes[index.get_name()] = index;
}


/** \brief Retrieve the map of secondary indexes.
 *
 * This function returns the map of all the secondary indexes part
 * to this table.
 *
 * \return The map of secondary index.
 */
snap_tables::secondary_index_t::map_t const & snap_tables::table_schema_t::get_secondary_indexes() const
{
    return f_secondary_indexes;
}



/** \brief Load a directory of XML files defining tables.
 *
 * This function goes through a list of files in a directory and reads
 * all these files as XML files defining tables.
 *
 * Here the schema is how we see the tables, whereas, the database
 * system may see it differently.
 *
 * The function can be called any number of times to read files from
 * various places. However, loading the same files more than once
 * is considered to be an error and the function returns with false.
 *
 * \param[in] path  The path to the directory to read files from.
 *
 * \return true if the load process worked, false otherwise.
 */
bool snap_tables::load(QString const & path)
{
    // create the pattern
    //
    QString const pattern(QString("%1/*.xml").arg(path));

    // read the list of files
    //
    bool success(true);

    try
    {
        glob_dir files;
        files.set_path( pattern, GLOB_ERR | GLOB_NOSORT | GLOB_NOESCAPE );
        files.enumerate_glob( [&]( QString the_path )
            {
                success = success && load_xml(the_path);
            });
    }
    catch( std::exception const & x)
    {
        SNAP_LOG_ERROR("could not read \"")(pattern)("\" (what=")(x.what())(")!");
        success = false;
    }
    catch( ... )
    {
        SNAP_LOG_ERROR("could not read \"")(pattern)("\"! (got unknown exception)");
        success = false;
    }

    return success;
}


/** \brief Load one specific XML file.
 *
 * This function loads one specific XML file from a file on disk or in
 * Qt resources.
 *
 * The file must be valid XML. The format is defined in the tables.xml
 * file found in the library and which defines the core tables.
 *
 * \param[in] filename  The name of the XML file to load.
 *
 * \return true when the file loaded properly and the table definitions
 *         were all added successfully to this 'tables' class.
 */
bool snap_tables::load_xml(QString const & filename)
{
    // open the file
    //
    QFile file(filename);
    if(!file.open(QIODevice::ReadOnly))
    {
        SNAP_LOG_ERROR("tables::load_xml() could not open \"")
                      (filename)
                      ("\" resource file.");
        return false;
    }

    // read the file
    //
    QDomDocument doc;
    QString errmsg;
    int err_line(0);
    int err_column(0);
    if(!doc.setContent(&file, false, &errmsg, &err_line, &err_column))
    {
        SNAP_LOG_ERROR("could not read XML in \"")
                      (filename)
                      ("\", error:")
                      (err_line)
                      ("/")
                      (err_column)
                      (": ")
                      (errmsg)
                      (".");
        return false;
    }

    // get the list of <table> tags and go through them one by one
    //
    QDomNodeList table_tags(doc.elementsByTagName("table"));
    for(int t(0); t < table_tags.size(); ++t)
    {
        QDomElement table(table_tags.at(t).toElement());
        if(table.isNull())
        {
            continue;
        }

        table_schema_t schema;
        schema.set_name(table.attribute("name"));

        // make sure we are not loading a duplicate
        // if so we cannot be sure what to do so we throw
        //
        auto const it(f_schemas.find(schema.get_name()));
        if(it != f_schemas.end())
        {
            // TODO: we do not currently save where we found the first
            //       instance so the error is rather poor at this point...
            //
            SNAP_LOG_FATAL("found second definition of \"")
                          (schema.get_name())
                          ("\" in \"")
                          (filename)
                          ("\".");
            throw snap_table_invalid_xml_exception("snap_tables::load_xml(): table names loaded in snap::tables must all be unique and not be a reserved keyword");
        }

        schema.set_model(string_to_model(table.attribute("model")));

        if(table.hasAttribute("drop"))
        {
            schema.set_drop();
        }

        QDomElement description(table.firstChildElement("description"));
        if(!description.isNull())
        {
            schema.set_description(description.text());
        }

        QDomElement schema_tag(table.firstChildElement("schema"));
        if(schema_tag.isNull())
        {
            SNAP_LOG_FATAL("missing required <schema> tag.");
            throw snap_table_invalid_xml_exception("snap_tables::load_xml(): missing required <schema> tag.");
        }

        if(schema_tag.hasAttribute("kind"))
        {
            schema.set_kind(string_to_kind(schema_tag.attribute("kind")));
        }

        QDomNodeList column_tags(schema_tag.elementsByTagName("column"));
        for(int c(0); c < column_tags.size(); ++c)
        {
            QDomElement const column_info(column_tags.at(c).toElement());
            if(column_info.isNull())
            {
                continue;
            }

            column_t column;

            // in a "thrift" kind of table, columns define the names
            // appearing in "column1" and the type of value in "value"
            //
            // in a "blob" kind of table, columns are all saved together
            // in one value; we're in charge of transforming the blob
            // data from a binary buffer to a map of named values
            //
            if(!column_info.hasAttribute("name"))
            {
                SNAP_LOG_FATAL("a <column> must have a \"name\" attribute.");
                throw snap_table_invalid_xml_exception("snap_tables::load_xml(): found a column without a \"name\" attribute.");
            }

            column.set_name(column_info.attribute("name"));

            // make sure that each column is unique by name
            //
            column_t::map_t columns(schema.get_columns());
            if(columns.find(column.get_name()) != columns.end())
            {
                SNAP_LOG_FATAL("column \"")
                              (column.get_name())
                              ("\" is defined multiple times in table \"")
                              (schema.get_name())
                              ("\".");
                throw snap_table_invalid_xml_exception("snap_tables::load_xml(): found two columns with the same name (see logs for details.)");
            }

            if(column_info.hasAttribute("type"))
            {
                dbutils::column_type_t const column_type(dbutils::get_column_type(column_info.attribute("type")));
                column.set_type(column_type);
            }

            if(column_info.hasAttribute("required")
            && column_info.attribute("required") == "true")
            {
                column.set_required();
            }

            if(column_info.hasAttribute("limited")
            && column_info.attribute("limited") == "true")
            {
                column.set_limited();
            }

            QDomElement const column_description(column_info.firstChildElement("description"));
            if(!column_description.isNull())
            {
                column.set_description(column_description.text());
            }

            QDomElement const column_default(column_info.firstChildElement("default"));
            if(!column_default.isNull())
            {
                column.set_default(column_default.text());
            }

            QDomElement const min_value(column_info.firstChildElement("min-value"));
            if(!min_value.isNull())
            {
                column.set_min_value(min_value.text().toDouble());
            }

            QDomElement const max_value(column_info.firstChildElement("max-value"));
            if(!max_value.isNull())
            {
                column.set_max_value(max_value.text().toDouble());
            }

            QDomElement const min_length(column_info.firstChildElement("min-length"));
            if(!min_length.isNull())
            {
                column.set_min_length(min_length.text().toLongLong());
            }

            QDomElement const max_length(column_info.firstChildElement("max-length"));
            if(!max_length.isNull())
            {
                column.set_max_length(max_length.text().toLongLong());
            }

            QDomElement const validation(column_info.firstChildElement("validation"));
            if(!validation.isNull())
            {
                column.set_validation(validation.text());
            }

            schema.set_column(column);
        }

        QDomNodeList secondary_index_tags(table.elementsByTagName("secondary-index"));
        for(int i(0); i < secondary_index_tags.size(); ++i)
        {
            QDomElement const secondary_index_info(secondary_index_tags.at(i).toElement());
            if(secondary_index_info.isNull())
            {
                continue;
            }

            secondary_index_t secondary_index;

            // get name (name is optional, use column name if name is not defined)
            //
            if(secondary_index_info.hasAttribute("name"))
            {
                secondary_index.set_name(secondary_index_info.attribute("name"));
            }

            // get column
            //
            if(!secondary_index_info.hasAttribute("column"))
            {
                SNAP_LOG_FATAL("an <secondary-index> must have a \"column\" attribute.");
                throw snap_table_invalid_xml_exception("snap_tables::load_xml(): found a <secondary-index> without a \"column\" attribute.");
            }

            secondary_index.set_column(secondary_index_info.attribute("column"));

            schema.set_secondary_index(secondary_index);
        }

        // save the new schema in our map
        //
        f_schemas[schema.get_name()] = schema;
    }

    return true;
}


/** \brief Check whether the named table exists.
 *
 * This function can be used to check whether the definition of a
 * specific table is defined.
 *
 * \note
 * The function returns false for tables that are described and have
 * the drop="drop" attribute set.
 *
 * \param[in] name  The name of the table to search for.
 *
 * \return true if the table is defined in the list of schemas.
 */
bool snap_tables::has_table(QString const & name) const
{
    auto const it(f_schemas.find(name));
    if(it == f_schemas.end())
    {
        return false;
    }
    return !it->second.get_drop();
}


/** \brief Check whether the named table exists.
 *
 * This function can be used to check whether the definition of a
 * specific table is defined.
 *
 * \note
 * The function returns false for tables that are described and have
 * the drop="drop" attribute set.
 *
 * \param[in] name  The name of the table to search for.
 *
 * \return true if the table is defined in the list of schemas.
 */
snap_tables::table_schema_t * snap_tables::get_table(QString const & name) const
{
    auto const it(f_schemas.find(name));
    if(it == f_schemas.end())
    {
        throw snap_table_unknown_table_exception(
                  "table \""
                + name
                + "\" does not exist. Please use has_table() first to determine whether you can call get_table().");
    }

    return const_cast<snap::snap_tables::table_schema_t *>(&it->second);
}


/** \brief Retrieve all the schemas.
 *
 * This function returns a reference to the internal map of schemas.
 * It can be used to go through the list of table definitions.
 *
 * \return A reference to the internal map.
 */
snap_tables::table_schema_t::map_t const & snap_tables::get_schemas() const
{
    return f_schemas;
}


/** \brief Transform a string to a model enumeration.
 *
 * This function transforms the specified \p model string in a
 * number as defined in the model_t enumeration.
 *
 * \exception snap_table_invalid_xml_exception
 * If the specified \p model string does not represent a valid name
 * in the list of model_name_values, then this exception is raised.
 *
 * \param[in] model  The name of the model to transform.
 *
 * \return One of the model_t enumeration value.
 */
snap_tables::model_t snap_tables::string_to_model(QString const & model)
{
    for(size_t idx(0) ; idx < sizeof(model_name_values) / sizeof(model_name_values[0]); ++idx)
    {
        if(model_name_values[idx].f_name == model)
        {
            return model_name_values[idx].f_model;
        }
    }

    // we have to find it, no choice...
    //
    throw snap_table_invalid_xml_exception(QString("model named \"%1\" was not found, please verify spelling or Snap!'s versions").arg(model));
}


/** \brief Transform a model to a string.
 *
 * This function transforms the specified \p model enumeration number
 * in a display string so it can be displayed.
 *
 * \exception snap_table_invalid_xml_exception
 * If the specified \p model number is not valid this exception is raised.
 *
 * \param[in] model  The model to transform.
 *
 * \return A string representing the model.
 */
QString snap_tables::model_to_string(model_t model)
{
    size_t const idx(static_cast<size_t>(model));
    if(idx < sizeof(model_name_values) / sizeof(model_name_values[0]))
    {
        return QString::fromUtf8(model_name_values[idx].f_name);
    }

    // invalid enumeration number?
    //
    throw snap_table_invalid_xml_exception(QString("model_t \"%1\" is not a valid model enumeration").arg(static_cast<int>(model)));
}


/** \brief Transform a string to a kind enumeration.
 *
 * This function transforms the specified \p kind string in a
 * number as defined in the kind_t enumeration.
 *
 * \exception snap_table_invalid_xml_exception
 * If the specified \p kind string does not represent a valid name
 * in the list of kind_name_values, then this exception is raised.
 *
 * \param[in] kind  The name of the kind to transform.
 *
 * \return One of the kind_t enumeration value.
 */
snap_tables::kind_t snap_tables::string_to_kind(QString const & kind)
{
    for(size_t idx(0) ; idx < sizeof(kind_name_values) / sizeof(kind_name_values[0]); ++idx)
    {
        if(kind_name_values[idx].f_name == kind)
        {
            return kind_name_values[idx].f_kind;
        }
    }

    // we have to find it, no choice...
    //
    throw snap_table_invalid_xml_exception(QString("kind named \"%1\" was not found, please verify spelling or Snap!'s versions").arg(kind));
}


/** \brief Transform a kind to a string.
 *
 * This function transforms the specified \p kind enumeration number
 * in a display string so it can be displayed.
 *
 * \exception snap_table_invalid_xml_exception
 * If the specified \p kind number is not valid this exception is raised.
 *
 * \param[in] kind  The kind to transform.
 *
 * \return A string representing the kind.
 */
QString snap_tables::kind_to_string(kind_t kind)
{
    size_t const idx(static_cast<size_t>(kind));
    if(idx < sizeof(kind_name_values) / sizeof(kind_name_values[0]))
    {
        return QString::fromUtf8(kind_name_values[idx].f_name);
    }

    // invalid enumeration number?
    //
    throw snap_table_invalid_xml_exception(QString("kind_t \"%1\" is not a valid kind enumeration").arg(static_cast<int>(kind)));
}


} // namespace snap
// vim: ts=4 sw=4 et
