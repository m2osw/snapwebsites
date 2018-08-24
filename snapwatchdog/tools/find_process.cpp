// Snap Websites Server -- command line tool to determine whether a process exists
// Copyright (c) 2018  Made to Order Software Corp.  All Rights Reserved
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
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

//#include "snapwatchdog.h"

// snapwebsites lib
//
#include <snapwebsites/log.h>
#include <snapwebsites/not_reached.h>
#include <snapwebsites/process.h>
#include <snapwebsites/snap_exception.h>
#include <snapwebsites/version.h>

// advgetopt lib
//
#include <advgetopt/advgetopt.h>

// C++ lib
//
#include <regex>


// last entry
//
#include <snapwebsites/poison.h>

namespace
{
    const std::vector<std::string> g_configuration_files;

    const advgetopt::getopt::option g_command_line_options[] =
    {
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            nullptr,
            nullptr,
            "Usage: %p [-<opt>]",
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
            's',
            0,
            "script",
            nullptr,
            "the process to look for was started as a script of the specified type (i.e. \"sh\", \"java\", \"python\", etc.)",
            advgetopt::getopt::argument_mode_t::required_argument
        },
        {
            'h',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            "help",
            nullptr,
            "show this help output",
            advgetopt::getopt::argument_mode_t::no_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
            "regex",
            nullptr,
            "view the --script (if used) and <process name> as regular expressions",
            advgetopt::getopt::argument_mode_t::no_argument
        },
        {
            'v',
            advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
            "verbose",
            nullptr,
            "make the output verbose",
            advgetopt::getopt::argument_mode_t::no_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            "version",
            nullptr,
            "show the version of %p and exit",
            advgetopt::getopt::argument_mode_t::no_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            nullptr,
            nullptr,
            "<process name>",
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
}
//namespace





int main(int argc, char * argv[])
{
    int exitval(1);

    try
    {
        advgetopt::getopt opt(argc, argv, g_command_line_options, g_configuration_files, nullptr);

        if(opt.is_defined("version"))
        {
            std::cout << SNAPWEBSITES_VERSION_STRING << std::endl;
            exit(0);
            snap::NOTREACHED();
        }

        if(opt.is_defined("help"))
        {
            opt.usage(advgetopt::getopt::status_t::no_error, "find_process");
            exit(1);
            snap::NOTREACHED();
        }

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
    exit( exitval );
    snap::NOTREACHED();
    return 0;
}

// vim: ts=4 sw=4 et
