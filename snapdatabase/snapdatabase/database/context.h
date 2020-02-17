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
#include    "snapdatabase/database/table.h"


// advgetopt lib
//
#include    <advgetopt/advgetopt.h>



namespace snapdatabase
{



namespace detail
{
class context_impl;
}



//#pragma GCC diagnostic push
//#pragma GCC diagnostic ignored "-Weffc++"
//#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
class context
    //: public std::enable_shared_from_this<context>
{
public:
    typedef std::shared_ptr<context>        pointer_t;
    typedef std::weak_ptr<context>          weak_pointer_t;

                                            ~context();

    static pointer_t                        create_context(advgetopt::getopt::pointer_t opts);

    void                                    initialize();
    table::pointer_t                        get_table(std::string const & name) const;
    table::map_t                            list_tables() const;
    std::string                             get_path() const;
    void                                    limit_allocated_memory();
    size_t                                  get_config_size(std::string const & name) const;
    std::string                             get_config_string(std::string const & name, int idx) const;
    long                                    get_config_long(std::string const & name, int idx) const;

private:
                                            context(advgetopt::getopt::pointer_t opts);

    std::unique_ptr<detail::context_impl>   f_impl;
};
//#pragma GCC diagnostic pop



} // namespace snapdatabase
// vim: ts=4 sw=4 et
