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

// Tell catch we want it to add the runner code in this file.
#define CATCH_CONFIG_RUNNER

// self
//
#include    "main.h"


// snapdatabase lib
//
#include    <snapdatabase/version.h>


// libexcept lib
//
#include    <libexcept/exception.h>


// snaplogger lib
//
#include    <snaplogger/logger.h>


// snapdev lib
//
#include    <snapdev/not_used.h>


// C++ lib
//
#include    <fstream>


// C lib
//
#include    <sys/stat.h>
#include    <sys/types.h>



namespace SNAP_CATCH2_NAMESPACE
{





std::string setup_context(std::string const & sub_path, std::vector<std::string> const & xmls)
{
    std::string path(g_tmp_dir() + "/" + sub_path);

    if(mkdir(path.c_str(), 0700) != 0)
    {
        if(errno != EEXIST)
        {
            CATCH_REQUIRE(!"could not create context path");
            return std::string();
        }
    }

    if(mkdir((path + "/tables").c_str(), 0700) != 0)
    {
        if(errno != EEXIST)
        {
            CATCH_REQUIRE(!"could not create table path");
            return std::string();
        }
    }

    if(mkdir((path + "/database").c_str(), 0700) != 0)
    {
        if(errno != EEXIST)
        {
            CATCH_REQUIRE(!"could not create database path");
            return std::string();
        }
    }

    for(auto const & x : xmls)
    {
        char const * s(x.c_str());
        CATCH_REQUIRE((s[0] == '<'
                    && s[1] == '!'
                    && s[2] == '-'
                    && s[3] == '-'
                    && s[4] == ' '
                    && s[5] == 'n'
                    && s[6] == 'a'
                    && s[7] == 'm'
                    && s[8] == 'e'
                    && s[9] == '='));
        s += 10;
        char const * e(s);
        while(*e != ' ')
        {
            ++e;
        }
        std::string const name(s, e - s);

        std::string const filename(path + "/tables/" + name + ".xml");
        {
            std::ofstream o(filename);
            o << x;
        }

        // the table.xsd must pass so we can make sure that our tests make
        // use of up to date XML code and that table.xsd is also up to date
        //
        std::string const verify_table("xmllint --noout --nonet --schema snapdatabase/snapdatabase/data/tables.xsd " + filename);
        std::cout << "running: " << verify_table << std::endl;
        int const r(system(verify_table.c_str()));
        CATCH_REQUIRE(r == 0);
    }

    return path;
}


void init_callback()
{
    libexcept::set_collect_stack(libexcept::collect_stack_t::COLLECT_STACK_NO);
}


int init_tests(Catch::Session & session)
{
    snapdev::NOT_USED(session);

    snaplogger::logger::pointer_t l(snaplogger::logger::get_instance());
    l->add_console_appender();
    l->set_severity(snaplogger::severity_t::SEVERITY_ALL);

    SNAP_LOG_ERROR
        << "This is an error through the logger..."
        << SNAP_LOG_SEND;

    return 0;
}


}



int main(int argc, char * argv[])
{
    return SNAP_CATCH2_NAMESPACE::snap_catch2_main(
              "snapdatabase"
            , SNAPDATABASE_VERSION_STRING
            , argc
            , argv
            , SNAP_CATCH2_NAMESPACE::init_callback
            , nullptr
            , SNAP_CATCH2_NAMESPACE::init_tests
        );
}


// vim: ts=4 sw=4 et
