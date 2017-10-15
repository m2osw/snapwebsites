// Snap Websites Server -- read a table XML file and put definitions in structures
// Copyright (C) 2016-2017  Made to Order Software Corp.
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

#include "snapwebsites/snap_tables.h"

#include "snapwebsites/glob_dir.h"
#include "snapwebsites/log.h"
#include "snapwebsites/not_used.h"

#include <QFile>
#include <QDomDocument>

#include "snapwebsites/poison.h"


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
    { "data",    snap_tables::model_t::MODEL_DATA },
    { "queue",   snap_tables::model_t::MODEL_QUEUE },
    { "log",     snap_tables::model_t::MODEL_LOG },
    { "session", snap_tables::model_t::MODEL_SESSION }
};


}
// no name namespace




void snap_tables::table_schema_t::set_name(QString const & name)
{
    f_name = name;
}


QString const & snap_tables::table_schema_t::get_name() const
{
    return f_name;
}


void snap_tables::table_schema_t::set_description(QString const & description)
{
    f_description = description;
}


QString const & snap_tables::table_schema_t::get_description() const
{
    return f_description;
}


void snap_tables::table_schema_t::set_model(model_t model)
{
    f_model = model;
}


snap_tables::model_t snap_tables::table_schema_t::get_model() const
{
    return f_model;
}


void snap_tables::table_schema_t::set_drop(bool drop)
{
    f_drop = drop;
}


bool snap_tables::table_schema_t::get_drop() const
{
    return f_drop;
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
        SNAP_LOG_ERROR("could not read \"")(pattern)("\"!");
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
        SNAP_LOG_ERROR("tables::load_xml() could not open \"")(filename)("\" resource file.");
        return false;
    }

    // read the file
    //
    QDomDocument doc;
    QString errmsg;
    int line(0);
    int column(0);
    if(!doc.setContent(&file, false, &errmsg, &line, &column))
    {
        SNAP_LOG_ERROR("tables::load_xml() could not read XML in \"")(filename)("\", error:")(line)("/")(column)(": ")(errmsg)(".");
        return false;
    }

    // get the list of <table> tags and go through them one by one
    //
    QDomNodeList table_tags(doc.elementsByTagName("table"));
    for(int idx(0); idx < table_tags.size(); ++idx)
    {
        QDomElement table(table_tags.at(idx).toElement());
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
            SNAP_LOG_FATAL("tables::load_xml() found second definition of \"")(schema.get_name())("\" in \"")(filename)("\".");
            throw snap_table_invalid_xml_exception("table names loaded in snap::tables must all be unique");
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

        // TODO: parse the actual schema (would only be useful for
        //       dbutils.cpp at this point)

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


/** \brief Transform a string to model enumeration.
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


} // namespace snap
// vim: ts=4 sw=4 et
