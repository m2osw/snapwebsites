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


CATCH_TEST_CASE("Structure Valid Version", "[structure] [version]")
{
    CATCH_START_SECTION("version conversion")
    {
        for(int n(0); n < 100; ++n)
        {
            int const major_version(rand() & 0xFFFF);
            int const minor_version(rand() & 0xFFFF);

            uint32_t const binary((major_version << 16) + minor_version);

            snapdatabase::version_t v1(major_version, minor_version);
            CATCH_REQUIRE(v1.get_major() == major_version);
            CATCH_REQUIRE(v1.get_minor() == minor_version);
            CATCH_REQUIRE(v1.to_binary() == binary);

            snapdatabase::version_t v2;
            CATCH_REQUIRE(v2.get_major() == 0);
            CATCH_REQUIRE(v2.get_minor() == 0);
            CATCH_REQUIRE(v2.is_null());
            CATCH_REQUIRE(v2 != v1);
            v2.from_binary(binary);
            CATCH_REQUIRE(v2.get_major() == major_version);
            CATCH_REQUIRE(v2.get_minor() == minor_version);
            CATCH_REQUIRE(v2.to_binary() == binary);
            CATCH_REQUIRE(v2 == v1);

            v2.next_revision();

            if(minor_version == 0xFFFF)
            {
                CATCH_REQUIRE(v2.get_major() == major_version + 1);
                CATCH_REQUIRE(v2.get_minor() == 0);
            }
            else
            {
                CATCH_REQUIRE(v2.get_major() == major_version);
                CATCH_REQUIRE(v2.get_minor() == minor_version + 1);
            }

            v2 = v1;
            int const new_major_version(rand() & 0xFFFF);
            v2.set_major(new_major_version);
            CATCH_REQUIRE(v2.get_major() == new_major_version);
            CATCH_REQUIRE(v2.get_minor() == minor_version);
            CATCH_REQUIRE(v2 != v1);

            int const new_minor_version(rand() & 0xFFFF);
            v2.set_minor(new_minor_version);
            CATCH_REQUIRE(v2.get_major() == new_major_version);
            CATCH_REQUIRE(v2.get_minor() == new_minor_version);
            CATCH_REQUIRE(v2 != v1);

            v2 = v1;
            CATCH_REQUIRE(v2.get_major() == major_version);
            CATCH_REQUIRE(v2.get_minor() == minor_version);
            CATCH_REQUIRE(v2.to_binary() == binary);
            CATCH_REQUIRE(v2 == v1);

            snapdatabase::version_t v3(v1);
            CATCH_REQUIRE_FALSE(v3.is_null());
            CATCH_REQUIRE(v3.get_major() == major_version);
            CATCH_REQUIRE(v3.get_minor() == minor_version);
            CATCH_REQUIRE(v3.to_binary() == binary);
            CATCH_REQUIRE(v3 == v1);
            CATCH_REQUIRE_FALSE(v3 > v1);
            CATCH_REQUIRE(v3 >= v1);
            CATCH_REQUIRE_FALSE(v3 < v1);
            CATCH_REQUIRE(v3 <= v1);

            std::string v3_str(v3.to_string());
            std::string version_str;
            version_str += std::to_string(major_version);
            version_str += '.';
            version_str += std::to_string(minor_version);
            CATCH_REQUIRE(v3_str == version_str);

            v3.next_branch();
            CATCH_REQUIRE(v3.get_major() == major_version + 1);
            CATCH_REQUIRE(v3.get_minor() == 0);
            CATCH_REQUIRE(v3.to_binary() == static_cast<uint32_t>((major_version + 1) << 16));
        }
    }
    CATCH_END_SECTION()
}

CATCH_TEST_CASE("Structure Overflown Version", "[structure] [version]")
{
    CATCH_START_SECTION("version overflow")
    {
        for(int n(0); n < 100; ++n)
        {
            int major_version(0);
            int minor_version(0);
            do
            {
                major_version = rand() ^ rand() * 65536;
                minor_version = rand() ^ rand() * 65536;
            }
            while(major_version < 65536
               && minor_version < 65536);

            CATCH_REQUIRE_THROWS_MATCHES(
                      snapdatabase::version_t(major_version, minor_version)
                    , snapdatabase::invalid_parameter
                    , Catch::Matchers::ExceptionMessage(
                              "snapdatabase_error: major/minor version must be between 0 and 65535 inclusive, "
                            + std::to_string(major_version)
                            + "."
                            + std::to_string(minor_version)
                            + " is incorrect."));
        }
    }
    CATCH_END_SECTION()
}

CATCH_TEST_CASE("Structure Overflow Version", "[structure] [version]")
{
    CATCH_START_SECTION("version compare")
    {
        for(int n(0); n < 100; ++n)
        {
            int major_version(rand() & 0xFFFF);
            int minor_version(rand() & 0xFFFF);
            int major_version2(rand() & 0xFFFF);
            while(major_version == major_version2)
            {
                major_version2 = rand() & 0xFFFF;
            }

            snapdatabase::version_t v1(major_version, minor_version);
            snapdatabase::version_t v2(major_version2, minor_version);
            if(major_version < major_version2)
            {
                CATCH_REQUIRE_FALSE(v1 == v2);
                CATCH_REQUIRE(v1 != v2);
                CATCH_REQUIRE(v1 < v2);
                CATCH_REQUIRE(v1 <= v2);
                CATCH_REQUIRE(v2 > v1);
                CATCH_REQUIRE(v2 >= v1);
            }
            else
            {
                CATCH_REQUIRE_FALSE(v1 == v2);
                CATCH_REQUIRE(v1 != v2);
                CATCH_REQUIRE(v1 > v2);
                CATCH_REQUIRE(v1 >= v2);
                CATCH_REQUIRE(v2 < v1);
                CATCH_REQUIRE(v2 <= v1);
            }
        }
    }
    CATCH_END_SECTION()
}


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

        description->set_uinteger("magic", static_cast<uint32_t>(snapdatabase::dbtype_t::BLOCK_TYPE_DATA));

        std::uint32_t flags(0x100105);
        description->set_uinteger("flags", flags);

        std::string const name("this is the name we want to include here");
        description->set_string("name", name);

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
