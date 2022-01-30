// Copyright (c) 2018-2020  Made to Order Software Corp.  All Rights Reserved
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
// 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA


// snapwebsites lib
//
#include <snapwebsites/log.h>
#include <snapwebsites/process.h>
#include <snapwebsites/snap_exception.h>
#include <snapwebsites/version.h>


// snapdev lib
//
#include <snapdev/not_reached.h>


// advgetopt lib
//
#include <advgetopt/advgetopt.h>
#include <advgetopt/exception.h>


// boost lib
//
#include <boost/preprocessor/stringize.hpp>


// C++ lib
//
#include <regex>


// last include
//
#include <snapdev/poison.h>



namespace
{


advgetopt::option const g_command_line_options[] =
{
    {
        's',
        advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_REQUIRED,
        "script",
        nullptr,
        "the process to look for was started as a script of the specified type (i.e. \"sh\", \"java\", \"python\", etc.)",
        nullptr
    },
    {
        '\0',
        advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::GETOPT_FLAG_FLAG,
        "regex",
        nullptr,
        "view the --script (if used) and <process name> as regular expressions",
        nullptr
    },
    {
        'v',
        advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::GETOPT_FLAG_FLAG,
        "verbose",
        nullptr,
        "make the output verbose",
        nullptr
    },
    {
        '\0',
        advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_MULTIPLE | advgetopt::GETOPT_FLAG_DEFAULT_OPTION | advgetopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "--",
        nullptr,
        "<process name>",
        nullptr
    },
    {
        '\0',
        advgetopt::GETOPT_FLAG_END,
        nullptr,
        nullptr,
        nullptr,
        nullptr
    }
};





// until we have C++20 remove warnings this way
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
advgetopt::options_environment const g_command_line_options_environment =
{
    .f_project_name = "snapwebsites",
    .f_group_name = nullptr,
    .f_options = g_command_line_options,
    .f_options_files_directory = nullptr,
    .f_environment_variable_name = nullptr,
    .f_section_variables_name = nullptr,
    .f_configuration_files = nullptr,
    .f_configuration_filename = nullptr,
    .f_configuration_directories = nullptr,
    .f_environment_flags = advgetopt::GETOPT_ENVIRONMENT_FLAG_PROCESS_SYSTEM_PARAMETERS,
    .f_help_header = "Usage: %p [-<opt>] <process-name>\n"
                     "where -<opt> is one or more of:",
    .f_help_footer = "%c",
    .f_version = SNAPWEBSITES_VERSION_STRING,
    .f_license = "GNU GPL v2",
    .f_copyright = "Copyright (c) 2013-"
                   BOOST_PP_STRINGIZE(UTC_BUILD_YEAR)
                   " by Made to Order Software Corporation -- All Rights Reserved",
    //.f_build_date = UTC_BUILD_DATE,
    //.f_build_time = UTC_BUILD_TIME
};
#pragma GCC diagnostic pop



}
//namespace





int main(int argc, char * argv[])
{
    int exitval(1);

    try
    {
        advgetopt::getopt opt(g_command_line_options_environment, argc, argv);

        bool const verbose(opt.is_defined("verbose"));
        bool const regex(opt.is_defined("regex"));

        std::string const script(opt.get_string("script"));
        std::string const process_name(opt.get_string("--"));

        std::unique_ptr<std::regex> script_regex;
        std::unique_ptr<std::regex> process_name_regex;
        if(regex)
        {
            if(!script.empty())
            {
                script_regex.reset(new std::regex(script, std::regex::nosubs));
            }
            process_name_regex.reset(new std::regex(process_name, std::regex::nosubs));

            if(verbose)
            {
                std::cout << "find_process: using regular expressions for testing." << std::endl;
            }
        }
        std::smatch match;

        snap::process_list l;
        l.set_field(snap::process_list::field_t::COMMAND_LINE);

        for(;;)
        {
            snap::process_list::proc_info::pointer_t p(l.next());
            if(p == nullptr)
            {
                break;
            }

            // get the command name
            //
            // (note that if no command line was used we cannot currently
            // find a corresponding process)
            //
            std::string name(p->get_process_basename());
            if(name.empty())
            {
                continue;
            }

            // if we have a script we need to find the actual command
            // which is the first arguement without a '-'
            // (not 100% of the time, though, "sh -o blah command" won't work)
            //
            if(!script.empty())
            {
                std::string command;
                int const max(p->get_args_size());
                for(int idx(0); idx < max; ++idx)
                {
                    std::string const a(p->get_arg(idx));
                    if(a.length() > 0
                    && a[0] != '-')
                    {
                        command = a;
                    }
                }
                if(command.empty())
                {
                    // no command found, we can't match properly
                    //
                    if(verbose)
                    {
                        std::cout << "find_process: skipping \"" << name << "\" as it does not seem to define a command." << std::endl;
                    }
                    continue;
                }
                if(regex
                    ? !std::regex_match(name, match, *script_regex)
                    : name != script)
                {
                    continue;
                }
                if(verbose)
                {
                    std::cout << "find_process: found \"" << name << "\", its command is \"" << command << "\"." << std::endl;
                }
                std::string::size_type const pos(command.rfind('/'));
                if(pos == std::string::npos)
                {
                    name = command;
                }
                else
                {
                    name = command.substr(pos + 1);
                }
            }

            if(regex
                ? std::regex_match(name, match, *process_name_regex)
                : name == process_name)
            {
                // found it!
                //
                if(verbose)
                {
                    std::cout << "find_process: success! Found \"" << name << "\"." << std::endl;
                }
                exitval = 0;
                break;
            }
        }

        if(verbose && exitval != 0)
        {
            std::cout << "find_process: failure. Could not find \"" << process_name << "\"." << std::endl;
        }
    }
    catch( advgetopt::getopt_exit const & except )
    {
        return except.code();
    }
    catch( snap::snap_exception const & except )
    {
        SNAP_LOG_FATAL("find_process: snap_exception caught: ")(except.what());
    }
    catch( std::exception const & std_except )
    {
        SNAP_LOG_FATAL("find_process: std::exception caught: ")(std_except.what());
    }
    catch( ... )
    {
        SNAP_LOG_FATAL("find_process: unknown exception caught!");
    }

    // exit via the server so the server can clean itself up properly
    //
    exit(exitval);
    snapdev::NOT_REACHED();
    return 0;
}

// vim: ts=4 sw=4 et
