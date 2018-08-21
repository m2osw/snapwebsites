/*
 * Text:
 *      snapserver/src/snapconfig.cpp
 *
 * Description:
 *      Retrieve a parameter from a snap configuration file, allow for the
 *      editing of a snap configuration file parameter, all from the command
 *      line.
 *
 * License:
 *      Copyright (c) 2014-2018  Made to Order Software Corp.  All Rights Reserved
 * 
 *      https://snapwebsites.org/
 *      contact@m2osw.com
 * 
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

// snapmanager lib
//
#include "snapmanager/manager.h"

// snapwebsite lib
//
#include <snapwebsites/snap_config.h>
#include <snapwebsites/version.h>

//#include <snapwebsites/snap_expr.h>
//#include <snapwebsites/qstring_stream.h>
//#include <snapwebsites/snapwebsites.h>

// advgetopt lib
//
#include <advgetopt/advgetopt.h>

// C++ lib
//
#include <iostream>


// last entry
//
#include <snapwebsites/poison.h>


const std::vector<std::string> g_configuration_files; // Empty

advgetopt::getopt::option const g_options[] =
{
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        nullptr,
        nullptr,
        "Usage: %p -<opt> <configuration filename> <field name> [<new value>]",
        advgetopt::getopt::argument_mode_t::help_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        nullptr,
        nullptr,
        "where -<opt> is one or more of:",
        advgetopt::getopt::argument_mode_t::help_argument
    },
    {
        'h',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "help",
        nullptr,
        "Show usage and exit.",
        advgetopt::getopt::argument_mode_t::no_argument
    },
    {
        '\0',
        0,
        "version",
        nullptr,
        "the version of this tool",
        advgetopt::getopt::argument_mode_t::no_argument
    },
    {
        '\0',
        0,
        nullptr,
        nullptr,
        "<configuration name> <field name> [<new value>]",
        advgetopt::getopt::argument_mode_t::default_multiple_argument
    },
    {
        '\0',
        0,
        nullptr,
        nullptr,
        nullptr,
        advgetopt::getopt::argument_mode_t::end_of_options
    }
};



class snapconfig
{
public:
                        snapconfig(int argc, char * argv[]);

    void                run();

private:
    advgetopt::getopt   f_opt;
};


/** \brief Initialize the DNS options object.
 *
 * This constructor parses the command line options and returns. It
 * does not try to interpret the command line  at all, this is reserved
 * to the run() function which has the ability to return an exit code.
 *
 * \param[in] argc  The number of arguments in argv.
 * \param[in] argv  The array of arguments found on the command line.
 */
snapconfig::snapconfig(int argc, char * argv[])
    : f_opt(argc, argv, g_options, g_configuration_files, "")
{
    if(f_opt.is_defined("version"))
    {
        std::cout << SNAPWEBSITES_VERSION_STRING << std::endl;
        exit(1);
    }
    if(f_opt.is_defined("help"))
    {
        f_opt.usage(advgetopt::getopt::status_t::no_error, "Usage: %s -<opt> <configuration filename> <field name> [<new value>]\n", argv[0]);
        exit(1);
    }

    if(!f_opt.is_defined("--"))
    {
        std::cerr << f_opt.get_program_name() << ":error: no configuration name, field name, and value defined." << std::endl;
        exit(1);
    }

    int const sz(f_opt.size("--"));

    if(sz < 2)
    {
        std::cerr << f_opt.get_program_name() << ":error: to the minimum a configuration name and a field name are required." << std::endl;
        exit(1);
    }

    if(sz > 3)
    {
        std::cerr << f_opt.get_program_name() << ":error: to the maximum a configuration name, a field name, and a value can be defined." << std::endl;
        exit(1);
    }
}


/** \brief Run the command.
 *
 * This checks whether we have 2 or 3 parameters, if 2 we read the field
 * and if 3 we write to the field. Note that the writing will happen in
 * the `snapwebsites.d` sub-directory.
 */
void snapconfig::run()
{
    std::string const & config_name(f_opt.get_string("--", 0));
    std::string::size_type const pos(config_name.find_first_of("./"));
    if(pos != std::string::npos)
    {
        std::cerr << f_opt.get_program_name() << "error: the configuration name must be a simple name like \"snapserver\"" << std::endl;
        exit(1);
    }

    // TODO: verify that the name is a "simeple name" (no period and no slashes)

    snap::snap_config config(config_name);

    std::string const field_name(f_opt.get_string("--", 1));

    if(f_opt.size("--") == 2)
    {
        // retrieval, get the value and print in std::cout
        //
        if(config.has_parameter(field_name))
        {
            std::cout << static_cast<std::string>(config[field_name]) << std::endl;
        }
        else
        {
            std::cout << std::endl;
        }
    }
    else
    {
        std::string const new_value(f_opt.get_string("--", 2));

        // replacement, get the new value and save it in the configuration
        //
        snap_manager::manager::pointer_t m(new snap_manager::manager(true));
        char * argv[2] = { const_cast<char *>("snapconfig"), nullptr };
        m->init(1, argv);

        // the get_override_filename() works only if we set that value
        // to a full path
        //
        //std::string const & override_filename(config.get_override_filename());
        std::string const override_filename("/etc/snapwebsites/snapwebsites.d/" + config_name + ".conf");
        m->replace_configuration_value(
                  QString::fromUtf8(override_filename.c_str())
                , QString::fromUtf8(field_name.c_str())
                , QString::fromUtf8(new_value.c_str()));
    }
}





int main(int argc, char * argv[])
{
    try
    {
        snapconfig s(argc, argv);
        s.run();
        return 0;
    }
    catch(std::exception const & e)
    {
        std::cerr << "snapconfig: exception: " << e.what() << std::endl;
        return 1;
    }
}

// vim: ts=4 sw=4 et
