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
#include    <snapdatabase/database/context.h>
#include    <snapdatabase/database/row.h>
//#include    <snapdatabase/database/context.h>


// advgetopt lib
//
#include    <advgetopt/options.h>




CATCH_TEST_CASE("Context", "[centext]")
{
    CATCH_START_SECTION("create a context")
    {
        std::vector<std::string> const simple_context =
            {
                {
                    "<!-- name=simple-context -->\n"
                    "<context>\n"
                      "<table name='foo' sparse='sparse' model='queue' row-key='c2,c1'>\n" // drop="..." temporary="..." secure="...">
                        "<block-size>4096</block-size>\n"
                        "<description>Create a Context</description>\n"
                        "<schema>\n"
                          "<column name='c1' type='uint16'>\n" // limited="..." encrypt="..." required="..." blob="...">
                            "<description>column 1</description>\n"
                            "<external>1Mb</external>\n"
                            "<default>55</default>\n"
                            "<min-value>0</min-value>\n"
                            "<max-value>100</max-value>\n"
                            "<min-length>1</min-length>\n"
                            "<max-length>10</max-length>\n"
                            "<validation>c1 &gt; c2</validation>\n"
                          "</column>\n"
                          "<column name='c2' type='int16' required='required'>\n" // limited="..." encrypt="..." blob="...">
                            "<description>column 2</description>\n"
                            "<external>1Mb</external>\n"
                            "<default>-37</default>\n"
                            "<min-value>-100</min-value>\n"
                            "<max-value>100</max-value>\n"
                            "<min-length>5</min-length>\n"
                            "<max-length>25</max-length>\n"
                          "</column>\n"
                          "<column name='c3' type='uint64'>\n" // limited="..." encrypt="..." required="..." blob="...">
                            "<description>column 3</description>\n"
                            "<default>0</default>\n"
                          "</column>\n"
                        "</schema>\n"
                        "<secondary-index name='created_on'>\n"
                          "<order>\n"
                            "<column-name name='_created_on' direction='desc'/>\n"
                            "<column-name name='c2'>c2 * 16 + rand() % 16</column-name>\n"
                            "<column-name name='c1' not-null='not-null'/>\n"
                          "</order>\n"
                          "<filter>c3 > 100</filter>\n"
                        "</secondary-index>\n"
                        "<secondary-index name='priority'>\n"
                          "<order>\n"
                            "<column-name name='c3'/>\n"
                            "<column-name name='_created_on' direction='desc'>_created_on + c2</column-name>\n"
                            "<column-name name='_deleted_on' not-null='null'/>\n"
                          "</order>\n"
                          "<filter>c3 &gt; 100</filter>\n"
                        "</secondary-index>\n"
                      "</table>\n"
                    "</context>\n"
                }
            };

        std::string const created(SNAP_CATCH2_NAMESPACE::setup_context("simple-context", simple_context));
        CATCH_REQUIRE_FALSE(created.empty());
        if(created.empty())
        {
            return;
        }

        std::string database_path(created + "/database");
        std::string tables_path(created + "/tables");

        advgetopt::option options[] =
        {
            advgetopt::define_option(
                  advgetopt::Name("context")
                , advgetopt::Flags(advgetopt::standalone_all_flags<
                              advgetopt::GETOPT_FLAG_GROUP_OPTIONS>())
                , advgetopt::Help("context is mandatory")
                //, advgetopt::DefaultValue(database_path.c_str())
            ),
            advgetopt::define_option(
                  advgetopt::Name("table-schema-path")
                , advgetopt::Flags(advgetopt::command_flags<
                              advgetopt::GETOPT_FLAG_GROUP_OPTIONS
                            , advgetopt::GETOPT_FLAG_REQUIRED
                            , advgetopt::GETOPT_FLAG_MULTIPLE>())
                , advgetopt::Help("path to the list of table schemata is mandatory")
            ),
            advgetopt::end_options()
        };

        options[0].f_default = database_path.c_str();

        // TODO: once we have stdc++20, remove all defaults
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
        advgetopt::options_environment const options_environment =
        {
            .f_project_name = "database",
            .f_group_name = nullptr,
            .f_options = options,
        };
#pragma GCC diagnostic pop

        char const * cargv[] =
        {
            "/usr/bin/xontext",
            "--table-schema-path",
            tables_path.c_str(),
            nullptr
        };
        int const argc(sizeof(cargv) / sizeof(cargv[0]) - 1);
        char ** argv = const_cast<char **>(cargv);

        advgetopt::getopt::pointer_t opt(std::make_shared<advgetopt::getopt>(options_environment, argc, argv));
        snapdatabase::context::pointer_t context(snapdatabase::context::create_context(opt));

        // make sure to reset before creating a new version otherwise
        // we would have two contexts open simultaneously
        //
        context.reset();

        // try again, this time we hit the schema compare functionality
        // (i.e. the file already exists)
        //
        context = snapdatabase::context::create_context(opt);

        snapdatabase::table::pointer_t table(context->get_table("wrong_name"));
        CATCH_REQUIRE(table == nullptr);

        table = context->get_table("foo");
        CATCH_REQUIRE(table != nullptr);

        struct row_data_t
        {
            typedef std::vector<row_data_t> vector_t;

            std::uint16_t       f_c1 = 0;
            std::int16_t        f_c2 = 0;
            std::uint64_t       f_c3 = 0;
        };
        row_data_t::vector_t row_data;

        for(int count(0); count < 580; ++count)
        //for(int count(0); count < 163; ++count)
        //for(int count(0); count < 31; ++count)
        {
std::cerr << "+++ row count = " << count << "\n";
            snapdatabase::row::pointer_t row(table->row_new());

            snapdatabase::cell::pointer_t c1(row->get_cell("c1", true));
            std::uint16_t const c1_value(rand() & 0xFFFF);
            c1->set_uint16(c1_value);

            snapdatabase::cell::pointer_t c2(row->get_cell("c2", true));
            std::int16_t const c2_value(rand() & 0xFFFF);
            c2->set_int16(c2_value);

            snapdatabase::cell::pointer_t c3(row->get_cell("c3", true));
            std::uint64_t c3_value(static_cast<std::uint64_t>(rand()) ^ (static_cast<std::uint64_t>(rand()) << 32));
            c3_value &= -256;
            c3_value |= count + 1;
            c3->set_uint64(c3_value);

            row_data_t data;
            data.f_c1 = c1_value;
            data.f_c2 = c2_value;
            data.f_c3 = c3_value;
            row_data.push_back(data);

std::cerr << "---------------------- INSERT ROW\n";
            table->row_insert(row);

            // now verify that this and all the previous inserts worked
            // and all the data is still accessible
            //
            // the indexes vector is used to search for each row in a
            // random order instead of first to last
            //
            std::vector<int> indexes;
            for(size_t p(0); p < row_data.size(); ++p)
            {
                indexes.push_back(p);
            }
            for(size_t p(0); p < row_data.size(); ++p)
            {
                size_t const q(rand() % row_data.size());
                std::swap(indexes[p], indexes[q]);
            }
std::cerr << "---------------------- VERIFY " << row_data.size() << " ROWS\n";
            for(size_t p(0); p < row_data.size(); ++p)
            {
                row_data_t & d(row_data[indexes[p]]);

                snapdatabase::conditions cond;
                cond.set_columns({"c1", "c2", "c3"});
                snapdatabase::row::pointer_t key(table->row_new());
                snapdatabase::cell::pointer_t c2_key(key->get_cell("c2", true));
                c2_key->set_int16(d.f_c2);
                snapdatabase::cell::pointer_t c1_key(key->get_cell("c1", true));
                c1_key->set_uint16(d.f_c1);
                cond.set_key("primary", key, snapdatabase::row::pointer_t());

std::cerr << "---------------------- READ ROW: " << d.f_c2 << ", " << d.f_c1 << "\n";
                snapdatabase::cursor::pointer_t cursor(table->row_select(cond));
                snapdatabase::row::pointer_t r(cursor->next_row());
                CATCH_REQUIRE(r != nullptr);
                snapdatabase::cell::pointer_t c1_data(r->get_cell("c1", false));
                CATCH_REQUIRE(c1_data != nullptr);
                CATCH_REQUIRE(c1_data->get_uint16() == d.f_c1);
                snapdatabase::cell::pointer_t c2_data(r->get_cell("c2", false));
                CATCH_REQUIRE(c2_data != nullptr);
                CATCH_REQUIRE(c2_data->get_int16() == d.f_c2);
                snapdatabase::cell::pointer_t c3_data(r->get_cell("c3", false));
                CATCH_REQUIRE(c3_data != nullptr);
                CATCH_REQUIRE(c3_data->get_uint64() == d.f_c3);

                // only one primary row with a specific key
                //
std::cerr << "---------------------- VERIFY UNIQUE ROW\n";
                int const max(rand() % 3 + 1);
                for(int i(0); i < max; ++i)
                {
                    CATCH_REQUIRE(cursor->next_row() == nullptr);
                }
            }
        }

        context.reset();
    }
    CATCH_END_SECTION()
}


// vim: ts=4 sw=4 et
