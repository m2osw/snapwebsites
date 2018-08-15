// Snap Websites -- tool to add/edit/remove DNS options
// Copyright (c) 2016-2018  Made to Order Software Corp.  All Rights Reserved
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

// dns
//
#include "dns.h"

// our lib
//
//#include "snapmanager/form.h"

// snapwebsites lib
//
//#include <snapwebsites/chownnm.h>
//#include <snapwebsites/log.h>
//#include <snapwebsites/mkdir_p.h>
//#include <snapwebsites/not_reached.h>
//#include <snapwebsites/not_used.h>

// Qt5
//
//#include <QtCore>

// C++ lib
//
//#include <fstream>

// last entry
//
#include <snapwebsites/poison.h>


namespace
{
//    QString const CREATE_MASTER_ZONE = QString("create_master_zone");
//    QString const CREATE_SLAVE_ZONE  = QString("create_slave_zone");
//    QString const SHOW_SLAVE_ZONES   = QString("show_slave_zones");

const std::vector<std::string> g_configuration_files; // Empty

const advgetopt::getopt::option g_options[] =
{
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        nullptr,
        nullptr,
        "Usage: %p [-<opt>] <filename>",
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
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
        "debug",
        nullptr,
        "run %p in debug mode",
        advgetopt::getopt::argument_mode_t::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "help",
        nullptr,
        "show %p help screen",
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
        0,
        nullptr,
        nullptr,
        "<named configuration file>",
        advgetopt::getopt::argument_mode_t::one_argument
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


/** \brief Edit various DNS options.
 *
 * This function is used to edit the BIND v9 file options.
 *
 * This is used to edit various parts of the options such as the
 * version, hostname, loggin, etc.
 *
 * Unfortunately BIND does not give us the option to add various
 * files in a directory with a proper order, etc. so we have to
 * parse the whole thing and add or edit options.
 */
class dns_options
{
public:
    enum class command_t
    {
        COMMAND_UNDEFINED,

        COMMAND_ADD,
        COMMAND_UPDATE,
        COMMAND_ADD_OR_UPDATE,
        COMMAND_REMOVE,
        COMMAND_GET
    };

    dns_options(int argc, char * argv[]);



private:
    advgetopt       f_opt;
    bool            f_debug = false;
    std::string     f_filename = std::string();
    command_t       f_command = command_t::COMMAND_UNDEFINED;
    std::string     f_parameter = std::string();
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
dns_options::dns_options(int argc, char * argv[])
    : f_opt(argc, argv, g_options, g_configuration_files)
{
}


/** \brief Run the specified command.
 *
 */
int dns_options::run()
{
    // on --version show the libsnapwebsites library version
    // (we do not have our own specific version at this point)
    //
    if(f_opt.is_defined("version"))
    {
        std::cout << SNAPWEBSITES_VERSION_STRING << std::endl;
        return 0;
    }

    // on --help print usage
    //
    if(f_opt.is_defined("help"))
    {
        f_opt.usage(advgetopt::getopt::status_t::no_error);
        snap::NOTREACHED();
        return 1;
    }

    // check the --debug
    //
    f_debug = f_opt.is_defined();

    // make sure there is a filename
    //
    if(!f_opt.is_defined("--"))
    {
        std::cerr << f_opt.get_program_name() << ":error: no filename was specified." << std::endl;
        return 1;
    }

    // get the filename
    //
    f_filename = f_opt.get_string("--");
    if(f_filename.empty())
    {
        std::cerr << f_opt.get_program_name() << ":error: an empty filename was specified." << std::endl;
        return 1;
    }

    // determine the command
    //
    std::map<std::string, command_t> commands
        {
            { "add",           command_t::COMMAND_ADD           },
            { "update",        command_t::COMMAND_UPDATE        },
            { "add-or-update", command_t::COMMAND_ADD_OR_UPDATE },
            { "remove",        command_t::COMMAND_REMOVE        },
            { "get",           command_t::COMMAND_GET           }
        };

    for(auto const & c : commands)
    {
        if(f_opt.is_defined(c.first))
        {
            if(f_command != command_t::COMMAND_UNDEFINED)
            {
                std::cerr << f_opt.get_program_name() << ":error: only one command can be specified on the command line." << std::endl;
                return 1;
            }

            f_command = c.second;
            f_parameter = f_opt.get_string(c.first);
        }
    }

    // then execute the command
    //
    int r(0);
    switch(f_command)
    {
    case command_t::COMMAND_ADD:
        r = add_option();
        break;

    case command_t::COMMAND_UPDATE:
        r = update_option();
        break;

    case command_t::COMMAND_ADD_OR_UPDATE:
        r = add_option();
        if(r == 2)
        {
            r = update_option();
        }
        break;

    case command_t::COMMAND_REMOVE:
        r = remove_option();
        break;

    case command_t::COMMAND_GET:
        r = get_option();
        break;

    default:
        std::cerr << f_opt.get_program_name() << ":error: no command was specified onthe command line." << std::endl;
        r = 1;
        break;

    }

    // done
    //
    return r;
}

/** \brief Search for an option, if not present, add it.
 *
 * This function reads the options file and search for a specified option.
 * If the option cannot be found, then the function adds it and returns 0.
 *
 * If the option is found, then it does not get modified and the function
 * returns 2 instad.
 *
 * \return 0 on success, 1 on error, 2 if the option already exists.
 */
int add_option()
{
}


}







/** \brief Implement the main() command.
 *
 * This tool accepts a set of command lines that are used to edit the
 * BIND configuration files. The following are the main commands one
 * can use to do such editing:
 *
 * \code
 * --add <parameter>=<value>
 * --update <parameter>=<value>
 * --add-or-update <parameter>=<value>
 * --remove <parameter>
 * --get <parameter>[=<default-value>]
 * \endcode
 *
 * The \<parameter> name accepts the dotted notation. For example, to
 * edit the version parameter in the options block, one writes:
 *
 * \code
 * --add 'options.version="none"'
 * \endcode
 *
 * Note that the quotations around "none" are important which is why
 * you need the single quote around the whole parameter. This --add
 * command says to search a parameter named "options". Once found, seach
 * for an option named "version" within a block following the options
 * parameter. If no version is found, add one at the end of the block.
 *
 * The \<value> is the new value for the parameter. As mentioned above
 * it may be necessary to use proper quotations on the command line
 * to get the correct results (i.e. if the value is a string such
 * as "named", then in your shell you must make sure to put that
 * between single quotes.)
 *
 * The --add option adds the specified option. If the option does not
 * exist, then add it at the end and exit with 0. If the option already
 * exists, it does not get updated and the command exits with 2.
 *
 * The --update option searches for the option and updates it. If the
 * option does exist, it gets updated and the command exits with 0. If the
 * option does not exist, nothing happens and the command exists with 2.
 *
 * The --add-or-update option searches for the option. It updates it
 * if it already exists. It adds it if it cannot be found yet. In
 * most cases this is the option you want to use to make sure an option
 * has a specific value. The command exits with 0 unless and error occurs.
 *
 * The --remove option searches for the specified option and deletes
 * it from the file. If it is not there, nothing happens.
 *
 * The --get option searches for the option and prints out its value.
 * If the option is not found, then nothing is printed and this tool
 * exits with 2. If your specify a value, that value is printed if
 * there is not such option (i.e. a default.)
 *
 * \warning
 * At this time the command line is not capable of executing more than
 * one command (i.e. it does not work like a script.) Use the command
 * multiple times to add/update/remove multiple fields.
 *
 * \param[in] argc  The number of argv options.
 * \param[in] argv  The command line options.
 */
int main(int argc, char * argv[])
{
    int r(0);
    try
    {
        dns_options o(argc, argv);
        r = o.run();
    }
    catch(std::exception const & e)
    {
        std::cerr << "dns_options:error: an exception occurred: " << e.what() << std::endl;
        r = 1;
    }
    return r;
}


// vim: ts=4 sw=4 et
