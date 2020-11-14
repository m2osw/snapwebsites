// Copyright (c) 2019-2020  Made to Order Software Corp.  All Rights Reserved
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
 * \brief Block Entry Index implementation.
 *
 * The data is indexed using a Block Entry Index as the bottom block. This
 * is the block which includes the remainder of the key and then a pointer
 * to the actual data or to an `IDXP` if the entry points to multiple
 * rows (i.e. secondary index allowing duplicates).
 */

// self
//
#include    "snapdatabase/block/block_entry_index.h"

#include    "snapdatabase/block/block_header.h"
#include    "snapdatabase/database/table.h"


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



// We don't want to do that because each key would have a size which means
// we'd waste a lot of space; instead we have one size in the header
//
//struct_description_t g_entry_description[] =
//{
//    define_description(
//          FieldName("flags")
//        , FieldType(struct_type_t::STRUCT_TYPE_BITS8)
//    ),
//    define_description(
//          FieldName("reference_or_oid")  -- sizeof(reference_t) == sizeof(oid_t)
//        , FieldType(struct_type_t::STRUCT_TYPE_OID)
//    ),
//    define_description(
//          FieldName("key")   -- max. size for the key here should be around 1Kb
//        , FieldType(struct_type_t::STRUCT_TYPE_BUFFER16)
//    ),
//};



// 'EIDX'
constexpr struct_description_t g_description[] =
{
    define_description(
          FieldName("header")
        , FieldType(struct_type_t::STRUCT_TYPE_STRUCTURE)
        , FieldSubDescription(detail::g_block_header)
    ),
    define_description(
          FieldName("count")
        , FieldType(struct_type_t::STRUCT_TYPE_UINT32)
    ),
    define_description(
          FieldName("size")
        , FieldType(struct_type_t::STRUCT_TYPE_UINT32)
    ),
    define_description(
          FieldName("next")
        , FieldType(struct_type_t::STRUCT_TYPE_REFERENCE)
    ),
    define_description(
          FieldName("previous")
        , FieldType(struct_type_t::STRUCT_TYPE_REFERENCE)
    ),
    //define_description(
    //      FieldName("entries")
    //    , FieldType(struct_type_t::STRUCT_TYPE_ARRAY32) -- we use "count" for the size
    //    , FieldDescription(g_entry_description)
    //),
    end_descriptions()
};


constexpr descriptions_by_version_t const g_descriptions_by_version[] =
{
    define_description_by_version(
        DescriptionVersion(0, 1),
        DescriptionDescription(g_description)
    ),
    end_descriptions_by_version()
};



}
// no name namespace




block_entry_index::block_entry_index(dbfile::pointer_t f, reference_t offset)
    : block(g_descriptions_by_version, f, offset)
{
}


std::uint32_t block_entry_index::get_count() const
{
    return static_cast<uint32_t>(f_structure->get_uinteger("count"));
}


void block_entry_index::set_count(std::uint32_t count)
{
    f_structure->set_uinteger("count", count);
}


std::uint32_t block_entry_index::get_size() const
{
    return static_cast<uint32_t>(f_structure->get_uinteger("size"));
}


// WARNING: you probably meant to use the set_data_size(); this function
//          takes a size which represents the entire entry:
//              flags, INDR or IDXP, key data
//
// these entries all have the same size; if we are to support variable
// size entry indexes, we will create a VIDX block and include a size
// for the key
//
void block_entry_index::set_size(std::uint32_t size)
{
    if(size < sizeof(std::uint8_t) + sizeof(oid_t) + 1)
    {
        throw snapdatabase_logic_error("the size of a block_entry_index must be large enough to support a flag, an oid_t, and at the very least one byte from your key.");
    }

    f_structure->set_uinteger("size", size);
}


void block_entry_index::set_key_size(std::uint32_t size)
{
    set_size(sizeof(std::uint8_t) + sizeof(oid_t) + size);
}


reference_t block_entry_index::get_next() const
{
    return static_cast<reference_t>(f_structure->get_uinteger("next"));
}


void block_entry_index::set_next(reference_t offset)
{
    f_structure->set_uinteger("next", offset);
}


reference_t block_entry_index::get_previous() const
{
    return static_cast<reference_t>(f_structure->get_uinteger("previous"));
}


void block_entry_index::set_previous(reference_t offset)
{
    f_structure->set_uinteger("previous", offset);
}


oid_t block_entry_index::find_entry(buffer_t const & key) const
{
    // the start offset is just after the structure
    // no alignment requirements since we use memcmp() and memcpy()
    // and that way the size can be anything
    //
    std::uint32_t const count(get_count());
    if(count == 0)
    {
std::cerr << "count == 0 -- entry block is empty?!\n";
        f_position = 0;
        return NULL_FILE_ADDR;
    }
std::cerr << "key being sought -> ";
for(int k(0); k < 16; ++k)
{
std::cerr << " " << std::hex << static_cast<int>(key[k]) << std::dec;
}
std::cerr << "\n";

    std::uint8_t const * buffer(data(f_structure->get_size()));
    std::uint32_t const size(get_size());
    std::uint32_t const length(std::min(size - sizeof(std::uint8_t) - sizeof(reference_t), key.size()));
    std::uint32_t i(0);
    std::uint32_t j(count);
std::cerr << "count = " << count << " -- if less than 3 we may fail our search?!\n";
    while(i < j)
    {
        f_position = (j - i) / 2 + i;
        std::uint8_t const * ptr(buffer + f_position * size);
std::cerr << "   f_position = " << f_position << "\n";
for(int k(0); k < 1 + 8 + 16; ++k)
{
std::cerr << " " << std::hex << static_cast<int>(ptr[k]) << std::dec;
}
std::cerr << "\n";
for(int k(0); k < 16; ++k)
{
std::cerr << " " << std::hex << static_cast<int>(key[k]) << std::dec;
}
std::cerr << "\n";
        int const r(memcmp(ptr + sizeof(std::uint8_t) + sizeof(reference_t), key.data(), length));
std::cerr << " and r = " << r << " for size " << length << "\n";
        if(r < 0)
        {
            ++f_position;
            i = f_position;
        }
        else if(r > 0)
        {
            j = f_position;
        }
        else
        {
            reference_t aligned_reference(0);
            memcpy(&aligned_reference, ptr + sizeof(std::uint8_t), sizeof(reference_t));
std::cerr << "   found it -> " << aligned_reference << "\n";
            return aligned_reference;
        }
    }

    // TBD: save current position close to point where we can do an insertion

std::cerr << "--- NOT FOUND ---\n";
    return NULL_FILE_ADDR;
}


std::uint32_t block_entry_index::get_position() const
{
    return f_position;
}


/** \brief Add a new entry to an entry index.
 *
 * This function is used to add one entry to the entry index block.
 *
 * The \p key represents the entry position in the block.
 *
 * The \p position_oid is the `INDR` position of the block being added
 * to the index. Internally, this reference may get saved in an array
 * in a separate `IDXP` block when multiple rows have the same key and
 * non-unique entries are allowed in that table.
 *
 * The \p close_position is the index within this block as returned
 * by the get_position() function. This allows the function to avoid
 * having to search for the position once more.
 *
 * \param[in] key  The key to index this entry by.
 * \param[in] position_oid  The OID of the row being inserted.
 * \param[in] close_position  The position within this block or -1.
 */
void block_entry_index::add_entry(buffer_t const & key, oid_t position_oid, std::int32_t close_position)
{
    // the start offset is just after the structure
    // no alignment requirements since we use memcmp() and memcpy()
    // and that way the size can be anything
    //
    std::uint8_t * buffer(data(f_structure->get_size()));
    std::uint32_t const count(get_count());
std::cerr << "add_entry() starting with count = " << count << "and OID=" << position_oid << "\n";
    std::uint32_t const size(get_size());
    std::uint32_t const length(get_size() - sizeof(std::uint8_t) - sizeof(reference_t));
    std::uint32_t const min_length(std::min(length, static_cast<std::uint32_t>(key.size())));

    if(size < sizeof(std::uint8_t) + sizeof(oid_t) + 1)
    {
        throw snapdatabase_logic_error("the size of this block_entry_index is not yet defined calling add_entry().");
    }

    if(close_position < 0)
    {
        close_position = 0;
        std::uint32_t i(0);
        std::uint32_t j(count);
        while(i < j)
        {
            close_position = (j - i) / 2 + i;
            std::uint8_t const * ptr(buffer + close_position * size);
std::cerr << "\n--- ptr = " << reinterpret_cast<void const *>(buffer)
                    << " + " << sizeof(std::uint8_t)
                    << " + " << sizeof(reference_t)
                    << " + " << close_position * size

                    << " data = " << reinterpret_cast<void const *>(key.data())
                    << " len = " << min_length
                    << " pos = " << close_position
                    << " size = " << size
                    << " ptr = " << reinterpret_cast<void const *>(ptr)
                    << "\n";
            int const r(memcmp(ptr + sizeof(std::uint8_t) + sizeof(reference_t), key.data(), min_length));
            if(r < 0)
            {
                ++close_position;
                i = close_position;
            }
            else if(r > 0)
            {
                j = close_position;
            }
            else
            {
                // in this case we add the OID to the existing entry which
                // we have to convert to a PIDX if not already defined as
                // such--
                //
throw snapdatabase_not_yet_implemented("block EIDX non-unique case");
                break;
            }
        }
    }

    size_t const page_size(get_table()->get_page_size());
    size_t const max_count((page_size - f_structure->get_size()) / size);
    if(count >= max_count)
    {
        // here the close_position value can't be negative
        //
        if(static_cast<std::uint32_t>(close_position) >= max_count)
        {
            // the new entry needs to be inserted in another block
            // (the next block if enough room, otherwise add a new
            // block "in between")
            //
throw snapdatabase_not_yet_implemented("block EIDX overflow case 1");
        }

        // insert this new entry here, but last entry needs to be moved
        // to another EIDX
        //
throw snapdatabase_not_yet_implemented("block EIDX overflow case 2");
    }

    std::uint32_t entries_after(count - close_position);
    if(entries_after > 0)
    {
        memmove(buffer + (close_position + 1) * size
              , buffer + close_position * size
              , (count - close_position) * size);
    }

    // flags
    //
    buffer[close_position * size] =
                (key.size() <= length ? ENTRY_INDEX_FLAG_COMPLETE : 0);

    // reference_t/oid_t
    //
    // since this is the first one, it's always an `oid_t`
    //
    memcpy(buffer + close_position * size + sizeof(std::uint8_t)
         , &position_oid
         , sizeof(oid_t));

    // key data
    //
    memcpy(buffer + close_position * size + sizeof(std::uint8_t) + sizeof(oid_t)
         , key.data()
         , min_length);
    if(key.size() < length)
    {
        // the key length is shorter so make sure to clear the remaining
        // space to all zeroes
        //
        memset(buffer + close_position * size + sizeof(std::uint8_t) + sizeof(oid_t) + key.size()
             , 0
             , length - key.size());
    }

    // in this case we added one entry
    //
    set_count(count + 1);
}






} // namespace snapdatabase
// vim: ts=4 sw=4 et
