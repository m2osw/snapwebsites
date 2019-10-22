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
//#include    "snapdatabase/dbfile.h"


// last include
//
#include    <snapdev/poison.h>



namespace snapdatabase
{



typedef std::vector<uint8_t>            buffer_t;
typedef uint16_t                        version_t;
typedef uint64_t                        block_ref_t;
typedef uint9_t                         flag8_t;
typedef uint16_t                        flag16_t;
typedef uint32_t                        flag32_t;
typedef uint64_t                        flag64_t;
typedef std::vector<uint16_t>           row_key_t;
typedef uint16_t                        column_id_t;
typedef uint16_t                        column_type_t;

struct hash_t
{
    uint8_t             f_hash[16];
};

struct magic_t
{
    char                f_magic[4];
};


enum model_t
{
    TABLE_MODEL_CONTENT,
    TABLE_MODEL_DATA,
    TABLE_MODEL_LOG,
    TABLE_MODEL_QUEUE,
    TABLE_MODEL_SESSION,
    TABLE_MODEL_SEQUENCIAL,
    TABLE_MODEL_TREE
};


constexpr flag64_t                          SCHEMA_FLAG_TEMPORARY  = 1LL << 0;
constexpr flag64_t                          SCHEMA_FLAG_DROP       = 1LL << 1;

constexpr flag32_t                          COLUMN_FLAG_LIMITED    = (1LL << 0);
constexpr flag32_t                          COLUMN_FLAG_REQUIRED   = (1LL << 1);
constexpr flag32_t                          COLUMN_FLAG_ENCRYPT    = (1LL << 2);
constexpr flag32_t                          COLUMN_FLAG_DEFAULT    = (1LL << 3);
constexpr flag32_t                          COLUMN_FLAG_BOUNDS     = (1LL << 4);
constexpr flag32_t                          COLUMN_FLAG_LENGTH     = (1LL << 5);
constexpr flag32_t                          COLUMN_FLAG_VALIDATION = (1LL << 6);



// binary data in the schema block
struct schema_block_t
{
// 0
    magic_t         f_magic;                            // 32 bits

    uint16_t        f_version;                          // 16 bits
    uint8_t         f_model;                            // 8 bits
    uint8_t         f_pad1;                             // 8 bits

    block_ref_t     f_next_schema;                      // 64 bits

// 128
    hash_t          f_hash;                             // 128 bits

// 256
    flag64_t        f_flags;                            // 64 bits
    block_ref_t     f_table_name;                       // 64 bits  (offset to string)

// 384
    block_ref_t     f_columns;                          // 64 bits  (schema_column_header_t)
    block_ref_t     f_row_key;                          // 64 bits  (schema_row_key_header_t)

// 512
    //uint32_t        f_pad4;                             // 32 bits
    //uint16_t        f_pad5;                             // 16 bits
    //uint16_t        f_number_of_columns;                // 16 bits
};


struct schema_row_key_header_t
{
    uint16_t        f_row_key_size;                     // 16 bits  (followed by schema_row_key_t[])
};


struct schema_row_key_t
{
    uint16_t        f_key_id;                           // 16 bits  (array)
};


struct schema_column_header_t
{
    uint16_t        f_number_of_columns;
    uint16_t        f_pad1;
    uint32_t        f_pad2;
};


struct schema_column_t
{
// 0
    uint32_t        f_size;                             // 32 bits  (total size of column description)
    uint16_t        f_identifier;                       // 16 bits
    uint16_t        f_type;                             // 16 bits
    flag32_t        f_flags;                            // 32 bits
    uint32_t        f_value_size;                       // 32 bits  (unused if f_type is "variable")

// 128
    block_ref_t     f_column_name;                      // 64 bits  (offset to string)
};

struct schema_column_encrypt_key_name_t
{
    block_ref_t     f_encrypt_key_name;                 // 64 bits  (offset to string--optional)
};

struct schema_column_default_value_t
{
    block_ref_t     f_default_value;                    // 64 bits  (offset to buffer--optional)
};

struct schema_column_minimum_value_t
{
    block_ref_t     f_minimum_value;                    // 64 bits  (offset to buffer--optional)
};

struct schema_column_maximum_value_t
{
    block_ref_t     f_maximum_value;                    // 64 bits  (offset to buffer--optional)
};

struct schema_column_minimum_length_t
{
    block_ref_t     f_minimum_length;                   // 64 bits  (offset to buffer--optional)
};

struct schema_column_maximum_length_t
{
    block_ref_t     f_maximum_length;                   // 64 bits  (offset to buffer--optional)
};

struct schema_column_validation_t
{
    block_ref_t     f_validation;                       // 64 bits  (offset to compiled script--optional)
};





} // namespace snapdatabase
// vim: ts=4 sw=4 et
