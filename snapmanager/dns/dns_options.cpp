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

    int             run();

private:
    int             load_file();
    int             getc();
    void            ungetc(int c);
    int             get_token();

    int             add_option();
    int             update_option();
    int             remove_option();
    int             get_option();

    advgetopt       f_opt = advgetopt();
    bool            f_debug = false;
    std::string     f_filename = std::string();
    command_t       f_command = command_t::COMMAND_UNDEFINED;
    std::string     f_parameter = std::string();
    std::string     f_data = std::string();
    int             f_pos = 0;
    int             f_line = 1;
    std::string     f_unget = std::string();
    std::string     f_token = std::string();
    int             f_block_level = 0;
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


/** \brief Load a named option file in memory.
 *
 * This function loads an option file in memory in its entirety.
 * We will work on the file in memory and once done create a backup
 * and then save the new version.
 *
 * The content of the file is found in f_data once the function returns
 * and if it returns 0.
 *
 * \return 0 if the file could be loaded properly, 1 on errors
 */
int dns_options::load_file()
{
    // reset lexer parameters
    //
    f_pos = 0;
    f_line = 1;
    f_block_level = 0;

    // ready the file as input
    //
    std::ifstream in(f_filename, std::ios_base::in | std::ios_base::binary);
    if(!in.is_open())
    {
        // could not open file
        //
        std::cerr << "error: can't open file \"" << f_filename << "\".";
        return 1;
    }

    // get the file size
    //
    in.seekg(0, std::ios::end);
    std::ifstream::pos_type const size(in.tellg());
    in.seekg(0, std::ios::beg);

    // ready the buffer
    //
    f_data.resize(size);

    // read the data in the buffer
    //
    in.read(f_data.data(), size);

    // if something bad happened, return 1, otherwise 0
    //
    return in.bad() ? 1 : 0;
}


/** \brief Get one character from the input file.
 *
 * This function is an equivalent to a getc() on the specified BIND
 * configuration file. Everything happens in memory, though.
 *
 * This function handles the case where some characters were ungetc().
 *
 * \return The next character or EOF if the end of the data was reached.
 */
int dns_options::getc()
{
    std::string::size_type unget_length(f_unget.length() - 1);
    if(unget_length > 0)
    {
        // restore a character that was ungotten
        //
        --unget_length;
        int const c(f_unget[unget_length]);
        f_unget.resize(unget_length);
        return c;
    }

    if(f_pos >= f_data.size())
    {
        // no more data
        //
        return EOF;
    }
    else
    {
        // return next character
        //
        int const c(f_data[f_pos]);
        if(c == '\n')
        {
            ++f_line;
        }
        ++f_pos;
        return c;
    }
}


/** \brief Put a character back into the buffer.
 *
 * This function is used whenever we read one too many (or more)
 * characters. The getc() function first returns the last ungetc()
 * character.
 *
 * \param[in] c  The character to unget.
 */
void dns_options::ungetc(int c)
{
    // make sure this is a valid character for the f_unget buffer
    // ignore all others (especially EOF)
    //
    if(c >= 1 && c <= 255)
    {
        f_unget += c;
    }
}


/** \brief Get the next token.
 *
 * This function gets the next token and saves it to the f_token parameter.
 *
 * \return 0 when no errors were encountered, 1 otherwise.
 */
int dns_options::get_token()
{
    f_token.clear();
    for(;;)
    {
        int c(getc());
        switch(c)
        {
        case EOF:
            return 0;

        case ' ':
        case '\t':
        case '\r':
        case '\n':
        case '\f':
            // ignore "noise"
            break;

        case '#':   // comment introducer
            do
            {
                c = getc();
            }
            while(c != EOF && c != '\n' && c != '\r');
            break;

        case '/':   // probably a comment
            c = getc();
            if(c == '/')
            {
                // C++ like comment, similar to the '#...'
                //
                do
                {
                    c = getc();
                }
                while(c != EOF && c != '\n' && c != '\r');
            }
            else if(c == '*')
            {
                // C like comment, search for "*/"
                // BIND does not accept a comment within a comment
                //
                for(c = getc(); c != EOF; c = getc())
                {
                    if(c == '*')
                    {
                        c = getc();
                        if(c == '/')
                        {
                            break;
                        }
                        if(c == '*')  // support "****/" as end of comment
                        {
                            ungetc(c);
                        }
                    }
                }
            }
            else
            {
                // this is a "lone" '/' character, continue token
                //
                f_token += c;
                goto read_token;
            }
            break;

        case '"':
            // no single quote string support in BIND
            //
            for(c = getc(); c != EOF && c != '"'; c = getc())
            {
                // the bind lexer allows for escaped characters like in
                // most languages (although no hex or octal support)
                //
                if(c == '\\')
                {
                    c = getc();
                    if(c == EOF)
                    {
                        // do not add EOF to f_token and this is an error
                        // anyway (printed just after the loop)
                        //
                        break;
                    }
                }
                else if(c == '\n')
                {
                    // a newline in a string is not allow without
                    // being escaped; so the following would be calid:
                    //
                    // "start...\
                    // ...end"
                    //
                    std::cerr << "error:" << f_filename << ":" << f_line << ": quoted string includes a newline." << std::endl;
                    return 1;
                }
                f_token += c;
            }
            if(c != '"')
            {
                std::cerr << "error:" << f_filename << ":" << f_line << ": quoted string was never closed." << std::endl;
                return 1;
            }
            return 0;

        case ';':
            f_token = ";";
            return 0;

        case '{':
            ++f_block_level;
            goto read_token;

        case '}':
            if(f_block_level <= 0)
            {
                std::cerr << "error:" << f_filename << ":" << f_line << ": '}' mismatch, '{' missing for this one.";
            }
            else
            {
                --f_block_level;
            }
            goto read_token;

        default:
read_token:
            f_token += c;
            for(c = getc();; c = getc())
            {
                switch(c)
                {
                case EOF:
                case ' ':
                case '\t':
                case '\r':
                case '\n':
                case '\f':
                    return 0;

                case '{':
                case '}':
                case '"':
                case ';':
                    // restore that character too, it is a token on its own
                    //
                    ungetc(c);
                    return 0;

                default:
                    f_token += c;
                    break;

                }
            }
            break;

        }
    }
}


/** \brief Search for an option, if not present, add it.
 *
 * This function reads the options file and search for the specified option.
 * If the option cannot be found, then the function adds it and returns 0.
 *
 * If the option is found, then it does not get modified and the function
 * returns 2 instead.
 *
 * \return 0 on success, 1 on error, 2 if the option already exists.
 */
int dns_options::add_option()
{
}


/** \brief Search for an option, if present, update it.
 *
 * This function reads the options file and search for the specified option.
 * If the option could be found, then the function replaces its value and
 * returns 0.
 *
 * If the option is not found, then it does not get added and the
 * function returns 2 instead.
 *
 * \return 0 on success, 1 on error, 2 if the option does not exist.
 */
int dns_options::update_option()
{
}


/** \brief Search for an option, if present, remove it.
 *
 * This function reads the options file and search for the specified option.
 * If the option is found, it gets deleted and the function returns 0.
 *
 * If the option is not found, then nothing happens and the function returns
 * 2 instead.
 *
 * \return 0 on success, 1 on error, 2 if the option does not exist.
 */
int dns_options::remove_option()
{
}


/** \brief Search for an option, if present, print its value in stdout.
 *
 * This function reads the options file and search for the specified option.
 * If the option is found, it reads its value and prints it to stdout.
 * Remember that the value of an option may be a whole block of data.
 * That block of data may also be named. When an option is found, the
 * function returns 0.
 *
 * If the option is not found, then nothing happens and the function returns
 * 2 instead.
 *
 * \return 0 on success, 1 on error, 2 if the option does not exist.
 */
int dns_options::get_option()
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
