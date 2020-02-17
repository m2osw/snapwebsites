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
 * \brief Row header.
 *
 * The row header defines the row class which is used to transform data to
 * a binary buffer (often reference to as a blob) and vice versa.
 *
 * The row is used on the client to transform the data to transfer it to
 * file and the database servers and to receive it back from those devices.
 *
 * The server uses it to transform the data so as to sort it when working
 * with secondary indexes.
 *
 * \note
 * The primary key is a special case and we have access to it
 * _automatically_.
 */

// self
//
#include    "snapdatabase/database/cell.h"
#include    "snapdatabase/database/table.h"



namespace snapdatabase
{



class row
    : public std::enable_shared_from_this<row>
{
public:
    typedef std::shared_ptr<row>                pointer_t;
    typedef std::vector<pointer_t>              vector_t;

                                                row(table::pointer_t t);

    table::pointer_t                            get_table() const;

    buffer_t                                    to_binary() const;
    void                                        from_binary(buffer_t const & blob);

    cell::pointer_t                             get_cell(column_id_t const & column_id, bool create);
    cell::pointer_t                             get_cell(std::string const & column_name, bool create);
    void                                        delete_cell(column_id_t const & column_id);
    void                                        delete_cell(std::string const & column_name);
    cell::map_t                                 cells() const;

    bool                                        commit();
    bool                                        insert();
    bool                                        update();

    void                                        generate_mumur3(buffer_t & murmur3, version_t version = version_t(), std::string const language = std::string());

private:
    table::weak_pointer_t                       f_table = table::weak_pointer_t();
    cell::map_t                                 f_cells = cell::map_t();
};



} // namespace snapdatabase
// vim: ts=4 sw=4 et
