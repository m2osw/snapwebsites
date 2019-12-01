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

// self
//
#include    "main.h"


// snapdatabase lib
//
#include    <snapdatabase/data/structure.h>


// advgetopt lib
//
#include    <advgetopt/options.h>


namespace
{



constexpr snapdatabase::struct_description_t g_description1[] =
{
    snapdatabase::define_description(
          snapdatabase::FieldName("magic")
        , snapdatabase::FieldType(snapdatabase::struct_type_t::STRUCT_TYPE_UINT32)
    ),
    snapdatabase::define_description(
          snapdatabase::FieldName("count")
        , snapdatabase::FieldType(snapdatabase::struct_type_t::STRUCT_TYPE_UINT32)
    ),
    snapdatabase::define_description(
          snapdatabase::FieldName("size")
        , snapdatabase::FieldType(snapdatabase::struct_type_t::STRUCT_TYPE_UINT32)
    ),
    snapdatabase::define_description(
          snapdatabase::FieldName("next")
        , snapdatabase::FieldType(snapdatabase::struct_type_t::STRUCT_TYPE_REFERENCE)
    ),
    snapdatabase::define_description(
          snapdatabase::FieldName("previous")
        , snapdatabase::FieldType(snapdatabase::struct_type_t::STRUCT_TYPE_REFERENCE)
    ),
    snapdatabase::end_descriptions()
};



constexpr snapdatabase::struct_description_t g_description2[] =
{
    snapdatabase::define_description(
          snapdatabase::FieldName("magic")
        , snapdatabase::FieldType(snapdatabase::struct_type_t::STRUCT_TYPE_UINT32)
    ),
    snapdatabase::define_description(
          snapdatabase::FieldName("flags")
        , snapdatabase::FieldType(snapdatabase::struct_type_t::STRUCT_TYPE_UINT32)
    ),
    snapdatabase::define_description(
          snapdatabase::FieldName("name")
        , snapdatabase::FieldType(snapdatabase::struct_type_t::STRUCT_TYPE_P8STRING)
    ),
    snapdatabase::define_description(
          snapdatabase::FieldName("size")
        , snapdatabase::FieldType(snapdatabase::struct_type_t::STRUCT_TYPE_UINT64)
    ),
    snapdatabase::define_description(
          snapdatabase::FieldName("model")
        , snapdatabase::FieldType(snapdatabase::struct_type_t::STRUCT_TYPE_UINT16)
    ),
    snapdatabase::end_descriptions()
};




}
// no name namespace


CATCH_TEST_CASE("Structure", "[structure]")
{
    CATCH_START_SECTION("simple structure")
    {
        snapdatabase::structure::pointer_t description(std::make_shared<snapdatabase::structure>(g_description1));

        description->init_buffer();

        description->set_uinteger("magic", static_cast<uint32_t>(snapdatabase::dbtype_t::BLOCK_TYPE_BLOB));

        std::uint32_t count(123);
        description->set_uinteger("count", count);

        std::uint32_t size(900000);
        description->set_uinteger("size", size);

        snapdatabase::reference_t next(0xff00ff00ff00);
        description->set_uinteger("next", next);

        snapdatabase::reference_t previous(0xff11ff11ff11);
        description->set_uinteger("previous", previous);

        CATCH_REQUIRE(description->get_uinteger("magic") == static_cast<uint32_t>(snapdatabase::dbtype_t::BLOCK_TYPE_BLOB));
        CATCH_REQUIRE(description->get_uinteger("count") == count);
        CATCH_REQUIRE(description->get_uinteger("size") == size);
        CATCH_REQUIRE(description->get_uinteger("next") == next);
        CATCH_REQUIRE(description->get_uinteger("previous") == previous);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("structure with a string")
    {
        snapdatabase::structure::pointer_t description(std::make_shared<snapdatabase::structure>(g_description2));

        description->init_buffer();
std::cerr << "TEST: size = " << description->get_current_size() << "\n";

        description->set_uinteger("magic", static_cast<uint32_t>(snapdatabase::dbtype_t::BLOCK_TYPE_DATA));

        std::uint32_t flags(0x100105);
        description->set_uinteger("flags", flags);

        std::string const name("this is the name we want to include here");
        description->set_string("name", name);
std::cerr << "TEST: after name size = " << description->get_current_size() << "\n";

        uint64_t size(1LL << 53);
        description->set_uinteger("size", size);

        uint16_t model(33);
        description->set_uinteger("model", model);

        CATCH_REQUIRE(description->get_uinteger("magic") == static_cast<uint32_t>(snapdatabase::dbtype_t::BLOCK_TYPE_DATA));
        CATCH_REQUIRE(description->get_uinteger("flags") == flags);
        CATCH_REQUIRE(description->get_string("name") == name);
        CATCH_REQUIRE(description->get_uinteger("size") == size);
        CATCH_REQUIRE(description->get_uinteger("model") == model);
    }
    CATCH_END_SECTION()
}


// vim: ts=4 sw=4 et
