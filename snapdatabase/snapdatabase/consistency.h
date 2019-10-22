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
#include    "snapdatabase/dbfile.h"


// last include
//
#include    <snapdev/poison.h>



namespace snapdatabase
{



enum class consistency_t : int8_t
{
    CONSISTENCY_INVALID = -1,

    CONSISTENCY_ZERO = 0,           // it works when only the client has a copy
    CONSISTENCY_ONE = 1,            // at least one database server has a copy
    CONSISTENCY_TWO = 2,            // at least one database server has a copy
    CONSISTENCY_THREE = 3,          // at least one database server has a copy
    CONSISTENCY_QUORUM = 4,         // at least a QUORUM (N / 2 + 1) servers have a copy (local or not)
    CONSISTENCY_ANY_QUORUM = 5,     // at least a QUORUM (N / 2 + 1) of any servers have a copy
    CONSISTENCY_LOCAL_QUORUM = 6,   // QUORUM in local database
    CONSISTENCY_EACH_QUORUM = 7,    // QUORUM in each data center
    CONSISTENCY_ANY = 8,            // any one database server available (may not be in the correct partition)
    CONSISTENCY_ALL = 9,            // all the servers in the partition have a copy
    CONSISTENCY_DEFAULT = 10        // use current default, on startup it is QUORUM
};



} // namespace snapdatabase
// vim: ts=4 sw=4 et
