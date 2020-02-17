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
 * \brief Types found in files.
 *
 * Each file and block has a few bytes at the start which generally defines
 * the type of the file and block.
 *
 * This file lists the various types we currently support. It is used by
 * the dbfile.cpp/.h and block.cpp/.h files.
 */

// C++ lib
//
#include    <cstdint>
#include    <string>


namespace snapdatabase
{



#define DBTYPE_NAME(s)       (static_cast<std::uint32_t>((s[0]<<0)|(s[1]<<8)|(s[2]<<16)|(s[3]<<24)))

enum class dbtype_t : std::uint32_t
{
    DBTYPE_UNKNOWN                  = DBTYPE_NAME("????"),

    FILE_TYPE_SNAP_DATABASE_TABLE   = DBTYPE_NAME("SDBT"),      // Snap! Database Table
    FILE_TYPE_EXTERNAL_INDEX        = DBTYPE_NAME("INDX"),      // External Index
    FILE_TYPE_BLOOM_FILTER          = DBTYPE_NAME("BLMF"),      // Bloom Filter

    BLOCK_TYPE_BLOB                 = DBTYPE_NAME("BLOB"),
    BLOCK_TYPE_DATA                 = DBTYPE_NAME("DATA"),
    BLOCK_TYPE_ENTRY_INDEX          = DBTYPE_NAME("EIDX"),
    BLOCK_TYPE_FREE_BLOCK           = DBTYPE_NAME("FREE"),
    BLOCK_TYPE_FREE_SPACE           = DBTYPE_NAME("FSPC"),
    BLOCK_TYPE_INDEX_POINTERS       = DBTYPE_NAME("IDXP"),
    BLOCK_TYPE_INDIRECT_INDEX       = DBTYPE_NAME("INDR"),
    BLOCK_TYPE_PRIMARY_INDEX        = DBTYPE_NAME("PIDX"),
    BLOCK_TYPE_SECONDARY_INDEX      = DBTYPE_NAME("SIDX"),
    BLOCK_TYPE_SCHEMA               = DBTYPE_NAME("SCHM"),
    BLOCK_TYPE_SCHEMA_LIST          = DBTYPE_NAME("SCHL"),
    BLOCK_TYPE_TOP_INDEX            = DBTYPE_NAME("TIDX"),
    BLOCK_TYPE_TOP_INDIRECT_INDEX   = DBTYPE_NAME("TIND"),
};

std::string                         to_string(dbtype_t type);



} // namespace snapdatabase
// vim: ts=4 sw=4 et
