// Copyright (c) 2019-2021  Made to Order Software Corp.  All Rights Reserved
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
 * \brief Handle a block structure.
 *
 * Each block contains a structure. The very first four bytes are always the
 * magic characters which define the type of the block. The remained of the
 * block is a _lose_ structure which very often changes in size because it
 * includes parameters such as a string or an array.
 *
 * Also in most cases arrays are also themselvess _lose_ structures (a few
 * are just numbers such as column ids or block references.)
 *
 * IMPORTANT: The types defined here are also the types that we accept in
 * a user table. Here we define structures and later tables.
 */

// self
//
#include    "snapdatabase/exception.h"
#include    "snapdatabase/data/virtual_buffer.h"
#include    "snapdatabase/bigint/bigint.h"


// snapdev lib
//
#include    <snapdev/init_structure.h>


// C++ lib
//
#include    <map>



namespace snapdatabase
{



constexpr   std::uint16_t                   STRUCTURE_VERSION_MAJOR = 0;
constexpr   std::uint16_t                   STRUCTURE_VERSION_MINOR = 1;


typedef std::uint64_t                       flags_t;










class version_t
{
public:
    constexpr       version_t()
                        : f_major(0)
                        , f_minor(0)
                    {
                    }

    constexpr       version_t(version_t const & v)
                        : f_major(v.f_major)
                        , f_minor(v.f_minor)
                    {
                    }

    constexpr       version_t(int major, int minor)
                        : f_major(major)
                        , f_minor(minor)
                    {
                        if(major < 0 || major >= 65536
                        || minor < 0 || minor >= 65536)
                        {
                            throw invalid_parameter(
                                      "major/minor version must be between 0 and 65535 inclusive, "
                                    + std::to_string(major)
                                    + "."
                                    + std::to_string(minor)
                                    + " is incorrect.");
                        }
                    }

    constexpr       version_t(std::uint32_t v)
                        : f_major(v >> 16)
                        , f_minor(v)
                    {
                    }

    version_t &     operator = (version_t const & v)
                    {
                        f_major = v.f_major;
                        f_minor = v.f_minor;

                        return *this;
                    }

    std::string     to_string() const
                    {
                        return std::to_string(static_cast<uint32_t>(f_major))
                             + "."
                             + std::to_string(static_cast<uint32_t>(f_minor));
                    }

    std::uint32_t   to_binary() const
                    {
                        return (static_cast<std::uint32_t>(f_major) << 16) + static_cast<std::uint32_t>(f_minor);
                    }

    void            from_binary(std::uint32_t v)
                    {
                        f_major = v >> 16;
                        f_minor = v;
                    }

    bool            is_null() const             { return f_major == 0 && f_minor == 0; }

    std::uint16_t   get_major() const           { return f_major; }
    void            set_major(uint16_t major)   { f_major = major; }
    std::uint16_t   get_minor() const           { return f_minor; }
    void            set_minor(uint16_t minor)   { f_minor = minor; }

    void            next_branch()               { ++f_major; f_minor = 0; }
    void            next_revision()             { ++f_minor; if(f_minor == 0) ++f_major; }

    bool            operator == (version_t const & rhs) const
                    {
                        return f_major == rhs.f_major
                            && f_minor == rhs.f_minor;
                    }
    bool            operator != (version_t const & rhs) const
                    {
                        return f_major != rhs.f_major
                            || f_minor != rhs.f_minor;
                    }
    bool            operator < (version_t const & rhs) const
                    {
                        return f_major < rhs.f_major
                            || (f_major == rhs.f_major && f_minor < rhs.f_minor);
                    }
    bool            operator <= (version_t const & rhs) const
                    {
                        return f_major < rhs.f_major
                            || (f_major == rhs.f_major && f_minor <= rhs.f_minor);
                    }
    bool            operator > (version_t const & rhs) const
                    {
                        return f_major > rhs.f_major
                            || (f_major == rhs.f_major && f_minor > rhs.f_minor);
                    }
    bool            operator >= (version_t const & rhs) const
                    {
                        return f_major > rhs.f_major
                            || (f_major == rhs.f_major && f_minor >= rhs.f_minor);
                    }

private:
    uint16_t        f_major = 0;
    uint16_t        f_minor = 0;
};


struct min_max_version_t
{
    constexpr       min_max_version_t(int min_major = 0, int min_minor = 0, int max_major = 0, int max_minor = 0)
                        : f_min_version(min_major, min_minor)
                        , f_max_version(max_major, max_minor)
                    {
                    }

    constexpr version_t       min() const { return f_min_version; }
    constexpr version_t       max() const { return f_max_version; }

    version_t const f_min_version = version_t();
    version_t const f_max_version = version_t();
};






constexpr int flag_position(flags_t const flag)
{
    switch(flag)
    {
    case (1ULL<< 0): return  0;
    case (1ULL<< 1): return  1;
    case (1ULL<< 2): return  2;
    case (1ULL<< 3): return  3;
    case (1ULL<< 4): return  4;
    case (1ULL<< 5): return  5;
    case (1ULL<< 6): return  6;
    case (1ULL<< 7): return  7;
    case (1ULL<< 8): return  8;
    case (1ULL<< 9): return  9;
    case (1ULL<<10): return 10;
    case (1ULL<<11): return 11;
    case (1ULL<<12): return 12;
    case (1ULL<<13): return 13;
    case (1ULL<<14): return 14;
    case (1ULL<<15): return 15;
    case (1ULL<<16): return 16;
    case (1ULL<<17): return 17;
    case (1ULL<<18): return 18;
    case (1ULL<<19): return 19;
    case (1ULL<<20): return 20;
    case (1ULL<<21): return 21;
    case (1ULL<<22): return 22;
    case (1ULL<<23): return 23;
    case (1ULL<<24): return 24;
    case (1ULL<<25): return 25;
    case (1ULL<<26): return 26;
    case (1ULL<<27): return 27;
    case (1ULL<<28): return 28;
    case (1ULL<<29): return 29;
    case (1ULL<<30): return 30;
    case (1ULL<<31): return 31;
    case (1ULL<<32): return 32;
    case (1ULL<<33): return 33;
    case (1ULL<<34): return 34;
    case (1ULL<<35): return 35;
    case (1ULL<<36): return 36;
    case (1ULL<<37): return 37;
    case (1ULL<<38): return 38;
    case (1ULL<<39): return 39;
    case (1ULL<<40): return 40;
    case (1ULL<<41): return 41;
    case (1ULL<<42): return 42;
    case (1ULL<<43): return 43;
    case (1ULL<<44): return 44;
    case (1ULL<<45): return 45;
    case (1ULL<<46): return 46;
    case (1ULL<<47): return 47;
    case (1ULL<<48): return 48;
    case (1ULL<<49): return 49;
    case (1ULL<<50): return 50;
    case (1ULL<<51): return 51;
    case (1ULL<<52): return 52;
    case (1ULL<<53): return 53;
    case (1ULL<<54): return 54;
    case (1ULL<<55): return 55;
    case (1ULL<<56): return 56;
    case (1ULL<<57): return 57;
    case (1ULL<<58): return 58;
    case (1ULL<<59): return 59;
    case (1ULL<<60): return 60;
    case (1ULL<<61): return 61;
    case (1ULL<<62): return 62;
    case (1ULL<<63): return 63;
    }
    return -1;
}


/** \brief Type of a field in the database files.
 *
 * When creating a description, we need to have a type for each item.
 * This enumeration gives us all the types that we support.
 *
 * \note
 * For block descriptions, the following types are not saved in the tables.
 * It is only part of the description structures. However, the type of a
 * field in a table has its type defined in the schema (it's not repeated
 * in each cell, though. That would be too heavy!)
 *
 * \todo
 * If it can help, consider supporting `CHAR(<size>)` which would allow us
 * to have strings with a fixed size (the result being that we'd then have
 * a table with a fixed size schema which can help in all sorts of ways).
 */
enum class struct_type_t : uint16_t
{
    STRUCT_TYPE_END,
    STRUCT_TYPE_VOID,

    // bits are used as flags or numbers that can use less than 8 bit
    // (i.e. 3 bits can be enough in many cases); each name defines
    // the bit field and its size like so:
    //
    //      <general-name>=<name1>:<size1>/<name2>:<size2>/...
    //
    // where by default size is 1 bit; we only support unsigned numbers
    // here; a field that gets removed can have its name removed and its
    // size is kept; this is the equivalent of a pad in the bit field
    //
    // the `<general-name>` is often set to "flags"
    //
    // this is very much an equivalent to bit fields in C/C++
    //
    STRUCT_TYPE_BITS8,              // 8 bits of bits
    STRUCT_TYPE_BITS16,             // 16 bits of bits
    STRUCT_TYPE_BITS32,             // 32 bits of bits
    STRUCT_TYPE_BITS64,             // 64 bits of bits
    STRUCT_TYPE_BITS128,            // 128 bits of bits
    STRUCT_TYPE_BITS256,            // 256 bits of bits
    STRUCT_TYPE_BITS512,            // 512 bits of bits

    STRUCT_TYPE_INT8,
    STRUCT_TYPE_UINT8,
    STRUCT_TYPE_INT16,
    STRUCT_TYPE_UINT16,
    STRUCT_TYPE_INT32,
    STRUCT_TYPE_UINT32,
    STRUCT_TYPE_INT64,
    STRUCT_TYPE_UINT64,
    STRUCT_TYPE_INT128,             // practical for MD5 and such
    STRUCT_TYPE_UINT128,
    STRUCT_TYPE_INT256,             // practical for SHA256
    STRUCT_TYPE_UINT256,
    STRUCT_TYPE_INT512,             // practical for SHA512
    STRUCT_TYPE_UINT512,

    STRUCT_TYPE_FLOAT32,
    STRUCT_TYPE_FLOAT64,
    STRUCT_TYPE_FLOAT128,

    STRUCT_TYPE_VERSION,            // UINT16:UINT16 (Major:Minor)

    STRUCT_TYPE_TIME,               // UINT64 equivalent to time_t (seconds)
    STRUCT_TYPE_MSTIME,             // UINT64 time in milliseconds
    STRUCT_TYPE_USTIME,             // UINT64 time in microseconds

    STRUCT_TYPE_P8STRING,           // UINT8 for size
    STRUCT_TYPE_P16STRING,          // UINT16 for size
    STRUCT_TYPE_P32STRING,          // UINT32 for size

    STRUCT_TYPE_STRUCTURE,          // one sub-structure (i.e. to access use "foo.blah")

    // array items get accessed with "foo[index]" (child structure has 1 field) and "foo[index].blah"
    // and to get the count use the hash character "#foo"
    STRUCT_TYPE_ARRAY8,             // UINT8 for count
    STRUCT_TYPE_ARRAY16,            // UINT16 for count
    STRUCT_TYPE_ARRAY32,            // UINT32 for count

    // buffers are an equivalent to uint8_t arrays, no need for a sub-structure description
    STRUCT_TYPE_BUFFER8,            // UINT8 for count
    STRUCT_TYPE_BUFFER16,           // UINT16 for count
    STRUCT_TYPE_BUFFER32,           // UINT32 for count

    STRUCT_TYPE_REFERENCE,          // UINT64 to another location in the file (offset 0 is start of file)
    STRUCT_TYPE_OID,                // UINT64 similar to a REFERENCE, but points to the TIND/INDR blocks (sizeof(OID) == sizeof(REFERENCE) must be true)

    STRUCT_TYPE_RENAMED             // there is no data attached to this one, the next description is the new name
};




constexpr struct_type_t             INVALID_STRUCT_TYPE(static_cast<struct_type_t>(-1));

constexpr ssize_t                   INVALID_SIZE = -1;
constexpr ssize_t                   VARIABLE_SIZE = -2;

std::string                         to_string(struct_type_t const & type);
struct_type_t                       name_to_struct_type(std::string const & type_name);
bool                                type_with_fixed_size(struct_type_t type);




typedef uint16_t                            struct_description_flags_t;

constexpr struct_description_flags_t        STRUCT_DESCRIPTION_FLAG_NONE         = 0x0000;
constexpr struct_description_flags_t        STRUCT_DESCRIPTION_MASK_OPTIONAL_BIT = 0x003F;  // use a field named "flags"
constexpr struct_description_flags_t        STRUCT_DESCRIPTION_FLAG_OPTIONAL     = 0x0040;


struct struct_description_t
{
    char const * const                      f_field_name = nullptr;
    struct_type_t const                     f_type = struct_type_t::STRUCT_TYPE_VOID;
    struct_description_flags_t const        f_flags = 0;
    char const * const                      f_default_value = nullptr;
    version_t const                         f_min_version = version_t();
    version_t const                         f_max_version = version_t();
    struct_description_t const * const      f_sub_description = nullptr;       // i.e. for an array, the type of the items
};


class FieldName
    : public snapdev::StructureValue<char const *>
{
public:
    constexpr FieldName()
        : snapdev::StructureValue<char const *>(nullptr)
    {
    }

    constexpr FieldName(char const * name)
        : snapdev::StructureValue<char const *>(name)
    {
    }
};


class FieldType
    : public snapdev::StructureValue<struct_type_t>
{
public:
    constexpr FieldType()
        : snapdev::StructureValue<struct_type_t>(struct_type_t::STRUCT_TYPE_END)
    {
    }

    constexpr FieldType(struct_type_t type)
        : snapdev::StructureValue<struct_type_t>(type)
    {
    }
};


class FieldFlags
    : public snapdev::StructureValue<struct_description_flags_t>
{
public:
    constexpr FieldFlags()
        : snapdev::StructureValue<struct_description_flags_t>(STRUCT_DESCRIPTION_FLAG_NONE)
    {
    }

    constexpr FieldFlags(struct_description_flags_t flags)
        : snapdev::StructureValue<struct_description_flags_t>(flags)
    {
    }
};


class FieldDefaultValue
    : public snapdev::StructureValue<char const *>
{
public:
    constexpr FieldDefaultValue()
        : snapdev::StructureValue<char const *>(nullptr)
    {
    }

    constexpr FieldDefaultValue(char const * default_value)
        : snapdev::StructureValue<char const *>(default_value)
    {
    }
};


class FieldVersion
    : public snapdev::StructureValue<min_max_version_t>
{
public:
    constexpr FieldVersion()
        : snapdev::StructureValue<min_max_version_t>(min_max_version_t())
    {
    }

    constexpr FieldVersion(int min_major, int min_minor, int max_major, int max_minor)
        : snapdev::StructureValue<min_max_version_t>(min_max_version_t(min_major, min_minor, max_major, max_minor))
    {
    }
};


class FieldSubDescription
    : public snapdev::StructureValue<struct_description_t const *>
{
public:
    constexpr FieldSubDescription()
        : snapdev::StructureValue<struct_description_t const *>(nullptr)
    {
    }

    constexpr FieldSubDescription(struct_description_t const * sub_description)
        : snapdev::StructureValue<struct_description_t const *>(sub_description)
    {
    }
};


template<class ...ARGS>
constexpr struct_description_t define_description(ARGS ...args)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
    struct_description_t s =
    {
        .f_field_name =          snapdev::find_field<FieldName          >(args...),        // no default, mandatory
        .f_type =                snapdev::find_field<FieldType          >(args...),        // no default, mandatory
        .f_flags =               static_cast<struct_description_flags_t>(
                                    snapdev::find_field<FieldFlags         >(args..., FieldFlags())),
        .f_default_value =       snapdev::find_field<FieldDefaultValue  >(args..., FieldDefaultValue()),
        .f_min_version =         snapdev::find_field<FieldVersion       >(args..., FieldVersion()).min(),
        .f_max_version =         snapdev::find_field<FieldVersion       >(args..., FieldVersion()).max(),
        .f_sub_description =     snapdev::find_field<FieldSubDescription>(args..., FieldSubDescription()),
    };
#pragma GCC diagnostic pop

    // TODO: once possible (C++17/20?) add verification tests here

    // whether a sub-description is allowed or not varies depending on the type
    //if(f_sub_description == nullptr)
    //{
    //}
    //else
    //{
    //}

    //if(f_min_version > f_max_version) ...

    return s;
}


constexpr struct_description_t end_descriptions()
{
    return define_description(
              FieldName(nullptr)
            , FieldType(struct_type_t::STRUCT_TYPE_END)
        );
}



struct descriptions_by_version_t
{
    version_t                       f_version = version_t();
    struct_description_t const *    f_description = nullptr;
};



class DescriptionVersion
    : public snapdev::StructureValue<version_t>
{
public:
    constexpr DescriptionVersion()
        : snapdev::StructureValue<version_t>(version_t())
    {
    }

    constexpr DescriptionVersion(int major, int minor)
        : snapdev::StructureValue<version_t>(version_t(major, minor))
    {
    }
};


class DescriptionDescription
    : public snapdev::StructureValue<struct_description_t const *>
{
public:
    constexpr DescriptionDescription()
        : snapdev::StructureValue<struct_description_t const *>(nullptr)
    {
    }

    constexpr DescriptionDescription(struct_description_t const * description)
        : snapdev::StructureValue<struct_description_t const *>(description)
    {
    }
};


template<class ...ARGS>
constexpr descriptions_by_version_t define_description_by_version(ARGS ...args)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
    descriptions_by_version_t d =
    {
        .f_version =         snapdev::find_field<DescriptionVersion       >(args...),         // no default, mandatory
        .f_description =     snapdev::find_field<DescriptionDescription   >(args...),         // no default, mandatory
    };
#pragma GCC diagnostic pop

    // TODO: once possible (C++17/20?) add verification tests here

    // either both are null or both are defined
    //if((f_description == nullptr) ^ f_version.zero())
    //{
    //     throw ...
    //}

    return d;
}


constexpr descriptions_by_version_t end_descriptions_by_version()
{
    return define_description_by_version(
              DescriptionVersion(0, 0)
            , DescriptionDescription(nullptr)
        );
}






class flag_definition
{
public:
    typedef std::shared_ptr<flag_definition>    pointer_t;
    typedef std::map<std::string, pointer_t>    map_t;

                            flag_definition();
                            flag_definition(
                                      std::string const & field_name
                                    , std::string const & flag_name
                                    , size_t pos
                                    , size_t size = 1);

    std::string             full_name() const;
    std::string             field_name() const;
    std::string             flag_name() const;
    size_t                  pos() const;
    size_t                  size() const;
    flags_t                 mask() const;

private:
    std::string             f_field_name = std::string();
    std::string             f_flag_name = std::string();
    size_t                  f_pos = 0;
    size_t                  f_size = 0;
    flags_t                 f_mask = 0;
};




class structure;
typedef std::shared_ptr<structure>          structure_pointer_t;
typedef std::vector<structure_pointer_t>    structure_vector_t;

class field_t
    : public std::enable_shared_from_this<field_t>
{
public:
    typedef std::shared_ptr<field_t>        pointer_t;
    typedef std::weak_ptr<field_t>          weak_pointer_t;
    typedef std::map<std::string, pointer_t> map_t;

    static constexpr std::uint32_t          FIELD_FLAG_VARIABLE_SIZE    = 0x0001;

                                            field_t(struct_description_t const * description);
                                            field_t(field_t const & rhs) = delete;
                                            ~field_t();

    field_t &                               operator = (field_t const & rhs) = delete;

    struct_description_t const *            description() const;
    pointer_t                               next() const;
    void                                    set_next(pointer_t next);
    pointer_t                               previous() const;
    void                                    set_previous(pointer_t previous);
    pointer_t                               first() const;
    pointer_t                               last() const;
    struct_type_t                           type() const;
    ssize_t                                 type_field_size() const;
    std::string                             field_name() const;
    std::string                             new_name() const; // for RENAMED fields
    std::uint32_t                           size() const;
    void                                    set_size(std::uint32_t size);
    bool                                    has_flags(std::uint32_t flags) const;
    std::uint32_t                           flags() const;
    void                                    set_flags(std::uint32_t flags);
    void                                    add_flags(std::uint32_t flags);
    void                                    clear_flags(std::uint32_t flags);
    flag_definition::pointer_t              find_flag_definition(std::string const & name) const;
    void                                    add_flag_definition(std::string const & name, flag_definition::pointer_t bits);
    std::uint64_t                           offset() const;
    void                                    set_offset(std::uint64_t offset);
    void                                    adjust_offset(std::int64_t adjust);
    structure_vector_t const &              sub_structures() const;
    structure_vector_t &                    sub_structures();
    structure_pointer_t                     operator [] (int idx) const;
    void                                    set_sub_structures(structure_vector_t const & v);

private:
    weak_pointer_t                          f_next = weak_pointer_t();
    weak_pointer_t                          f_previous = weak_pointer_t();
    struct_description_t const *            f_description = nullptr;
    std::uint32_t                           f_size = 0;
    std::uint32_t                           f_flags = 0;
    std::uint64_t                           f_offset = 0;
    structure_vector_t                      f_sub_structures = structure_vector_t();    // for ARRAY# and STRUCTURE
    flag_definition::map_t                  f_flag_definitions = flag_definition::map_t(); // for BIT representing flags
};


class structure
    : public std::enable_shared_from_this<structure>
{
public:
    typedef std::shared_ptr<structure>      pointer_t;
    typedef std::weak_ptr<structure>        weak_pointer_t;
    typedef std::vector<pointer_t>          vector_t;

                                            structure(struct_description_t const * descriptions, pointer_t parent = pointer_t());
                                            structure(structure const & rhs) = delete;

    structure &                             operator = (structure const & rhs) = delete;

    void                                    set_block(
                                                  block::pointer_t b
                                                , std::uint64_t offset
                                                , std::uint64_t size);
    void                                    init_buffer();
    void                                    set_virtual_buffer(
                                                  virtual_buffer::pointer_t buffer
                                                , std::uint64_t start_offset);
    virtual_buffer::pointer_t               get_virtual_buffer(reference_t & start_offset) const;

    size_t                                  get_size() const;
    size_t                                  get_current_size() const;

    pointer_t                               parent() const;
    field_t::pointer_t                      get_field(
                                                  std::string const & field_name
                                                , struct_type_t type = struct_type_t::STRUCT_TYPE_END) const;
    flag_definition::pointer_t              get_flag(
                                                  std::string const & flag_name
                                                , field_t::pointer_t & f) const;

    // bits, int/uint, all sizes up to 64 bits, reference
    std::int64_t                            get_integer(std::string const & field_name) const;
    void                                    set_integer(std::string const & field_name, std::int64_t value);

    std::uint64_t                           get_uinteger(std::string const & field_name) const;
    void                                    set_uinteger(std::string const & field_name, std::uint64_t value);

    uint64_t                                get_bits(std::string const & flag_name) const;
    void                                    set_bits(std::string const & flag_name, std::uint64_t value);

    // bits, int/uint, all sizes up to 512 bits
    int512_t                                get_large_integer(std::string const & field_name) const;
    void                                    set_large_integer(std::string const & field_name, int512_t value);

    uint512_t                               get_large_uinteger(std::string const & field_name) const;
    void                                    set_large_uinteger(std::string const & field_name, uint512_t value);

    // floating points (long double, even today, are not likely 128 bits, more like 80 to 102)
    float                                   get_float32(std::string const & field_name) const;
    void                                    set_float32(std::string const & field_name, float value);

    double                                  get_float64(std::string const & field_name) const;
    void                                    set_float64(std::string const & field_name, double value);

    long double                             get_float128(std::string const & field_name) const;
    void                                    set_float128(std::string const & field_name, long double value);

    // strings/buffers
    std::string                             get_string(std::string const & field_name) const;
    void                                    set_string(std::string const & field_name, std::string const & value);

    structure::pointer_t                    get_structure(std::string const & field_name) const;
    void                                    set_structure(std::string const & field_name, structure::pointer_t & value);

    structure::vector_t                     get_array(std::string const & field_name) const;
    structure::pointer_t                    new_array_item(std::string const & field_name);
    void                                    set_array(std::string const & field_name, structure::vector_t const & value);

    buffer_t                                get_buffer(std::string const & field_name) const;
    void                                    set_buffer(std::string const & field_name, buffer_t const & value);

    void                                    adjust_offsets(
                                                      reference_t offset_cutoff
                                                    , std::int64_t diff);

private:
    std::uint64_t                           parse() const;
    std::uint64_t                           parse_descriptions(std::uint64_t offset) const;
    void                                    verify_buffer_size();
    field_t::pointer_t                      find_field(std::string const & field_name);

    struct_description_t const *            f_descriptions = nullptr;
    weak_pointer_t                          f_parent = weak_pointer_t();
    virtual_buffer::pointer_t               f_buffer = virtual_buffer::pointer_t();
    reference_t                             f_start_offset = 0;
    mutable std::uint64_t                   f_original_size = 0;
    field_t::map_t                          f_fields_by_name = field_t::map_t();
};



std::ostream & operator << (std::ostream & out, version_t const & v);



} // namespace snapdatabase
// vim: ts=4 sw=4 et
