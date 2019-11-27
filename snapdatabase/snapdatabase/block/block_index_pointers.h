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
 * \brief Block representing the database file header.
 *
 */

// self
//
#include    "snapdatabase/data/structure.h"



namespace snapdatabase
{



class block_index_pointers
    : public block
{
public:
    typedef std::shared_ptr<block_index_pointers>       pointer_t;

                                block_index_pointers(dbfile::pointer_t f, reference_t offset);


private:
};



} // namespace snapdatabase
// vim: ts=4 sw=4 et
