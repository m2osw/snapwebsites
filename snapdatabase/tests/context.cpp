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
                    "<!-- name=simple-context -->"
                    "<context>"
                      "<table name='foo' sparse='sparse'>" // model="..." row-key="..." drop="..." temporary="..." sparse="..." secure="...">
                        "<block-size>4096</block-size>"
                        "<description>Create a Context</description>"
                        "<schema>"
                          "<column name='c1' type='uint16'>" // limited="..." encrypt="..." required="..." blob="...">
                            "<description>column 1</description>"
                            "<external>1Mb</external>"
                            "<default>55</default>"
                            "<min-value>0</min-value>"
                            "<max-value>100</max-value>"
                            "<min-length>1</min-length>"
                            "<max-length>10</max-length>"
                            "<validation>c1 &gt; c2</validation>"
                          "</column>"
                          "<column name='c2' type='int16'>" // limited="..." encrypt="..." required="..." blob="...">
                            "<description>column 2</description>"
                            "<external>1Mb</external>"
                            "<default>-37</default>"
                            "<min-value>-100</min-value>"
                            "<max-value>100</max-value>"
                            "<min-length>5</min-length>"
                            "<max-length>25</max-length>"
                          "</column>"
                        "</schema>"
                      "</table>"
                    "</context>"
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
        context.reset();
    }
    CATCH_END_SECTION()
}


// vim: ts=4 sw=4 et
