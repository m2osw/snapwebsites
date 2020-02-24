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
#include    "snapdatabase/database/row.h"


// murmur3 lib
//
#include    <murmur3/murmur3.h>


// C++ lib
//
#include    <iostream>


// last include
//
#include    <snapdev/poison.h>



namespace snapdatabase
{



namespace
{



/** \brief The seed used to generate murmur3 keys.
 *
 * We need to use one specific seed to generate all our mumur3 keys. This
 * one is defined here and used by the table to generate the keys for
 * your tables.
 *
 * The ability to change the seed is not currently offered because
 *
 * 1. It is unlikely a necessity
 * 2. The exact same seed must be used on all computers in your cluster
 * 3. If you lose the seed, you lose access to your data (you need to
 *    re-insert it with the new seed)
 *
 * So at this point I have it hard coded.
 */
std::uint32_t   g_murmur3_seed = 0x6BC4A931;



} // no name namespace



row::row(table::pointer_t t)
    : f_table(t)
{
}


table::pointer_t row::get_table() const
{
    return f_table.lock();
}


buffer_t row::to_binary() const
{
    buffer_t result;

    // save the schema version first to make sure we can extract the
    // data whatever the version
    //
    table::pointer_t t(get_table());
    push_be_uint32(result, t->schema_version().to_binary());

    // TODO: have several loops:
    //
    //    1. columns that are needed by filters
    //    2. data that we want to compress
    //    3. data that we want to encrypt
    //
    // Ultimately, filters should work against any columns, but speed wise
    // it's just not good if compressed and/or encrypted;
    //
    for(auto const & c : f_cells)
    {
        c.second->column_id_to_binary(result);
        c.second->value_to_binary(result);
    }

    if(result.size() > std::numeric_limits<std::uint32_t>::max())
    {
        // TODO: we need to add support for large rows (i.e. using the
        //       `BLOB` block or external file)
        //
        throw invalid_size("size of row too large");
    }

    return result;
}


/** \brief Transform a blob into a set of cells in a row.
 *
 * This function transforms the specified \p blob in a set of cells in this
 * row.
 *
 * \todo
 * We need to consider looking into not defining all the cells if the user
 * only asked for a few of them. This may actually be a feature to implement
 * in the to_binary() function. In any even if the SELECT only requests
 * column "A" then we should only return that one column and not all of them.
 * This will save us a lot of bandwidth, but it also means that the row is
 * incomplete and can't be written back to the database. So we have to have
 * a form of special case. (We also want to support updates without all the
 * data available in the row; i.e. with parts only available on disk...)
 *
 * \param[in] blob  The blob to extract to this row object.
 */
void row::from_binary(buffer_t const & blob)
{
    table::pointer_t t(f_table.lock());
    size_t pos(0);
    version_t const version(read_be_uint32(blob, pos));
    if(version != t->schema_version())
    {
        // the schema changed, make sure to
        //
        // read & convert the old row
        //    AND
        // save the new version of the row to the database
        //
        while(pos < blob.size())
        {
            column_id_t const column_id(cell::column_id_from_binary(blob, pos));
            schema_column::pointer_t exist_schema(t->column(column_id, version));
            if(exist_schema == nullptr)
            {
                throw column_not_found(
                          "Column with identifier "
                        + std::to_string(static_cast<int>(column_id))
                        + " does not exist in \""
                        + get_table()->name()
                        + "\" schema version "
                        + version.to_string()
                        + " (from_binary).");
            }
            cell::pointer_t c(std::make_shared<cell>(exist_schema));
            c->value_from_binary(blob, pos); // we MUST read or skip that data, so make sure to do that

            schema_column::pointer_t current_schema(t->column(exist_schema->name()));
            if(current_schema != nullptr)
            {
                cell::pointer_t cell(get_cell(column_id, true));
                cell->copy_from(*c);
            }
            //else -- instead of a useless call to c->value_from_binary() we should also have a c->skip_binary_value()
        }
    }
    else
    {
        while(pos + sizeof(std::uint16_t) <= blob.size())
        {
            column_id_t const column_id(cell::column_id_from_binary(blob, pos));
            if(column_id == 0)
            {
                // this happens because we align the data (although we may
                // not want to do that?)
                break;
            }

std::cerr << "READ COLUMN DATA... pos = " << pos << " vs " << blob.size() << " column ID " << column_id << "\n";
            cell::pointer_t cell(get_cell(column_id, true));
            cell->value_from_binary(blob, pos);
        }
    }
}


cell::pointer_t row::get_cell(column_id_t const & column_id, bool create)
{
    auto it(f_cells.find(column_id));
    if(it != f_cells.end())
    {
            return it->second;
    }

    schema_column::pointer_t column(get_table()->column(column_id));
    if(column == nullptr)
    {
        throw column_not_found(
                  "Column with identifier "
                + std::to_string(static_cast<int>(column_id))
                + " does not exist in \""
                + get_table()->name()
                + "\" (get_cell).");
    }

    if(!create)
    {
        return cell::pointer_t();
    }

    cell::pointer_t c(std::make_shared<cell>(column));
    f_cells[column_id] = c;
    return c;
}


cell::pointer_t row::get_cell(std::string const & column_name, bool create)
{
    schema_column::pointer_t column(get_table()->column(column_name));
    if(column == nullptr)
    {
        throw column_not_found(
                  "Column \""
                + column_name
                + "\" does not exist in \""
                + get_table()->name()
                + "\".");
    }

    auto it(f_cells.find(column->column_id()));
    if(it != f_cells.end())
    {
            return it->second;
    }

    if(!create)
    {
        return cell::pointer_t();
    }

    cell::pointer_t c(std::make_shared<cell>(column));
    f_cells[column->column_id()] = c;
    return c;
}


void row::delete_cell(column_id_t const & column_id)
{
    auto it(f_cells.find(column_id));
    if(it != f_cells.end())
    {
        f_cells.erase(it);
    }
}


void row::delete_cell(std::string const & column_name)
{
    schema_column::pointer_t column(get_table()->column(column_name));
    if(column == nullptr)
    {
        throw column_not_found(
                  "Column \""
                + column_name
                + "\" does not exist in \""
                + get_table()->name()
                + "\".");
    }

    delete_cell(column->column_id());
}


cell::map_t row::cells() const
{
    return f_cells;
}


bool row::commit()
{
    return get_table()->row_commit(shared_from_this());
}


bool row::insert()
{
    return get_table()->row_insert(shared_from_this());
}


bool row::update()
{
    return get_table()->row_update(shared_from_this());
}


/** \brief Generate a key used to index a row.
 *
 * This function generates the murmur3 key used to index the primary row.
 * This key is a Murmur hash version 3. The first \em few bits are used
 * to define which computer receives the row. The remainder are used to
 * index the data in a table on one of those computers.
 *
 * The function is capable of generating the key for a branch or a revision.
 * For a branch, define the version so it is not `0.0` so `is_null()` returns
 * false. For a revision, set the version and the language. If you have an
 * entry without a language, use "xx" as the language (i.e. language neutral).
 *
 * \li (1) version.is_null(), global
 * \li (2) !version.is_null(), language.empty(), branch
 * \li (3) !version.is_null(), !language.empty(), revision
 *
 * \attention
 * The key on the client side never specify the version or language. Those
 * are used internally when the database system needs to know exactly which
 * data is required.
 *
 * \attention
 * In Cassandra we were able to go through the list of branches and revisions
 * (with a SELECT) and we may also want to be able to do that here in which
 * case those keys would not be murmur3 keys. Instead we'd use a secondary
 * index which uses non-hashed keys. Especially, we need to be able to find
 * the latest of each revision based on the language. Not only that, we may
 * need to find the latest version in language "en" or the languages available
 * at this version (i.e. that would require us to have multiple secondary
 * indexes for revisions). We could of course use both: have the murmur3 to
 * create the rows and also have secondary indexes to do sorted searches of
 * revisions.
 *
 * \param[in] murmur3  The buffer where the hash gets saved.
 * \param[in] version  The branch & revision version.
 * \param[in] language  The language of the revision.
 */
void row::generate_mumur3(buffer_t & murmur3, version_t version, std::string const language)
{
    // get the data from the columns used to access the primary key index
    //
    buffer_t key;
    table::pointer_t table(get_table());
    column_ids_t const ids(table->row_key());
    bool add_separator(false);
    for(auto id : ids)
    {
        cell::pointer_t const cell(get_cell(id, false));
        if(cell == nullptr)
        {
            // TBD: these columns are all required otherwise we would not be
            // able to calculate the primary key; that being said, later
            // maybe we could support a default in a primary key column
            //
            throw column_not_found(
                      "Column with ID #"
                    + table->column(id, false)->name()
                    + " is not set, but it is mandatory to search the primary key of table \""
                    + table->name()
                    + "\".");
        }

        if(add_separator)
        {
            // we add the separator only if necessary (i.e. when more data
            // follows that column)
            //
            push_uint8(key, 255);
        }
        cell->value_to_binary(key);
        add_separator = !cell->has_fixed_type();
    }
    if(!version.is_null())
    {
        // at least we have a branch, maybe a revision too
        //
        if(add_separator)
        {
            push_uint8(key, 255);
        }
        push_be_uint16(key, version.get_major());
        //add_separator = false;

        if(!language.empty())
        {
            // we also have a revision
            //
            push_be_uint16(key, version.get_minor());

            uint8_t const * s(reinterpret_cast<uint8_t const *>(language.c_str()));
            key.insert(key.end(), s, s + language.length());
            //add_separator = true;
        }
    }

    // make sure it's the correct size
    //
    murmur3.resize(16);

    // generate the actual murmur version 3
    //
    MurmurHash3_x64_128(key.data(), key.size(), g_murmur3_seed, murmur3.data());
}


} // namespace snapdatabase
// vim: ts=4 sw=4 et
