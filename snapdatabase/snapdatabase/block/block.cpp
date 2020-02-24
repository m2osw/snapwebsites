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
 * \brief Base block implementation.
 *
 * The block base class handles the loading of the block in memory using
 * mmap() and gives information such as its type and location.
 */

// self
//
#include    "snapdatabase/block/block.h"

#include    "snapdatabase/exception.h"
#include    "snapdatabase/database/table.h"
#include    "snapdatabase/data/structure.h"


// snaplogger lib
//
#include    <snaplogger/message.h>


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


constexpr char const * g_errmsg_table = "block::~block() called with an f_data pointer, but f_table == nullptr.";
constexpr char const * g_errmsg_exception = "block::~block() tried to release the f_data by it threw an exception.";


}



block::block(descriptions_by_version_t const * structure_descriptions, dbfile::pointer_t f, reference_t offset)
    : f_file(f)
    , f_structure_descriptions(structure_descriptions)
    , f_offset(offset)
{
#ifdef _DEBUG
    // verify that the versions are in order
    //
    descriptions_by_version_t const * p(nullptr);
    descriptions_by_version_t const * d = f_structure_descriptions;
    for(; d->f_description != nullptr; ++d)
    {
        if(p != nullptr)
        {
            if(p->f_version <= d->f_version)
            {
                throw snapdatabase_logic_error(
                          "The versions in a structure definition array must be in order ("
                        + p->f_version.to_string()
                        + " <= "
                        + d->f_version.to_string()
                        + " when it should be '>').");
            }
        }
        p = d;
    }
    if(d == f_structure_descriptions)
    {
        throw snapdatabase_logic_error("The array of structure descriptions cannot be empty.");
    }
#endif

    // create the structure we want to use in memory
    //
    // this may not be the one used when loading data from a file.
    //
    f_structure = std::make_shared<structure>(f_structure_descriptions->f_description);
}


block::~block()
{
    if(f_data != nullptr)
    {
        if(f_table == nullptr)
        {
            SNAP_LOG_FATAL
                << g_errmsg_table
                << SNAP_LOG_SEND;
            std::cerr << g_errmsg_table << std::endl;
            std::terminate();
        }

        try
        {
            f_table->get_dbfile()->release_data(f_data);
            //f_data = nullptr;
        }
        catch(page_not_found const & e)
        {
            SNAP_LOG_FATAL
                << g_errmsg_exception
                << " ("
                << e.what()
                << ")."
                << SNAP_LOG_SEND;
            std::cerr
                << g_errmsg_exception
                << " ("
                << e.what()
                << ")."
                << std::endl;
            std::terminate();
        }
    }
}


table_pointer_t block::get_table() const
{
    if(f_table == nullptr)
    {
        throw invalid_xml("block::get_table() called before the table was defined.");
    }

    return f_table;
}


void block::set_table(table_pointer_t table)
{
    if(f_table != nullptr)
    {
        throw invalid_xml("block::set_table() called twice.");
    }

    f_table = table;
}


structure::pointer_t block::get_structure() const
{
    return f_structure;
}


structure::pointer_t block::get_structure(version_t version) const
{
    for(descriptions_by_version_t const * d = f_structure_descriptions;
                                          d->f_description != nullptr;
                                          ++d)
    {
        if(d->f_version == version)
        {
            return std::make_shared<structure>(f_structure_descriptions->f_description);
        }
    }

    throw snapdatabase_logic_error(
              "Block of type \""
            + to_string(get_dbtype())
            + "\" has no structure version "
            + version.to_string()
            + ".");
}


void block::clear_block()
{
    reference_t const offset(f_structure->get_size());
#ifdef _DEBUG
    if(offset == 0)
    {
        throw snapdatabase_logic_error("the structure of the block_free_block block cannot be dynamic.");
    }
#endif
    std::uint32_t const data_size(get_table()->get_page_size() - offset);

    memset(data(offset), 0, data_size);
}


dbtype_t block::get_dbtype() const
{
    return *reinterpret_cast<dbtype_t const *>(data(0));
}


void block::set_dbtype(dbtype_t type)
{
    // TODO: add verifications (i.e. go from FREE to any or any to FREE
    //       and maybe a few others)
    //
    if(*reinterpret_cast<dbtype_t *>(data(0)) != type)
    {
        *reinterpret_cast<dbtype_t *>(data(0)) = type;

        reference_t const size(f_structure->get_size());
        memset(data(sizeof(dbtype_t))
             , 0
             , size - sizeof(dbtype_t));
    }
}


version_t block::get_structure_version() const
{
    return version_t(static_cast<std::uint32_t>(f_structure->get_uinteger("header.version")));
}


void block::set_structure_version()
{
    // avoid a write if not required
    //
    if(get_structure_version() != f_structure_descriptions->f_version)
    {
        f_structure->set_uinteger("header.version", f_structure_descriptions->f_version.to_binary());
    }
}


reference_t block::get_offset() const
{
    return f_offset;
}


void block::set_data(data_t data)
{
    // the table retrieves the data pointer because it needs to determine
    // the block type (using the first 4 bytes) and so the data pointer
    // is already locked once and we can immediately save it in the block
    //
    f_data = data;
}


data_t block::data(reference_t offset)
{
    if(f_data == nullptr)
    {
        //f_data = get_table()->get_dbfile()->data(f_offset);
        throw snapdatabase_logic_error("block::data() called before set_data().");
    }

    return f_data + (offset % get_table()->get_page_size());
}


const_data_t block::data(reference_t offset) const
{
    if(f_data == nullptr)
    {
        //f_data = get_table()->get_dbfile()->data(f_offset);
        throw snapdatabase_logic_error("block::data() called before set_data().");
    }

    return f_data + (offset % get_table()->get_page_size());
}


void block::sync(bool immediate)
{
    get_table()->get_dbfile()->sync(f_data, immediate);
}


void block::from_current_file_version()
{
    version_t current_version(get_structure_version());
    if(f_structure_descriptions->f_version == current_version)
    {
        // same version, no conversion necessary
        //
        return;
    }

    throw snapdatabase_logic_error("from_current_file_version() not fully implemented yet.");
}



} // namespace snapdatabase
// vim: ts=4 sw=4 et
