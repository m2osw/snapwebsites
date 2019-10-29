// Snap Websites -- tool to add/edit/remove DNS options
// Copyright (c) 2016-2019  Made to Order Software Corp.  All Rights Reserved
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


// self
//
//#include "dns_options.h"


// snapmanager lib
//
#include <snapmanager/version.h>


// advgetopt lib
//
#include "advgetopt/advgetopt.h"
#include "advgetopt/exception.h"


// snapwebsites lib
//
#include <snapwebsites/file_content.h>
#include <snapwebsites/version.h>


// snapdev lib
//
#include <snapdev/not_reached.h>


// boost lib
//
#include <boost/preprocessor/stringize.hpp>


// C++ lib
//
#include <iostream>
#include <vector>


// last include
//
#include <snapdev/poison.h>



namespace
{


advgetopt::option const g_options[] =
{
    {
        '\0',
        advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::GETOPT_FLAG_FLAG,
        "debug",
        nullptr,
        "run %p in debug mode",
        nullptr
    },
    {
        'e',
        advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_REQUIRED | advgetopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "execute",
        nullptr,
        "define a command to execute, see manual for details about syntax;"
        " <keyword> ( '[' <keyword> | '\"' <string> '\"' ']' )*"                // a keyword with indexes
            " ( '.' field ( '[' <keyword> | '\"' <string> '\"' ']' )* )*"       // any number of fields with indexes
            " ( ( '?' | '+' )? '='"                                             // assignment operator (if not GET)
                " ( 'null' | (<keyword> | '\"' <string> '\"' )+ ) )?",          // value to assign or null (for REMOVE)
        nullptr
    },
    {
        '\0',
        advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_FLAG,
        "stdout",
        nullptr,
        "print result in stdout instead of overwriting the input file",
        nullptr
    },
    {
        '\0',
        advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_DEFAULT_OPTION,
        "--",
        nullptr,
        "<named configuration file>",
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
advgetopt::options_environment const g_options_environment =
{
    .f_project_name = "snapwebsites",
    .f_options = g_options,
    .f_options_files_directory = nullptr,
    .f_environment_variable_name = nullptr,
    .f_configuration_files = nullptr,
    .f_configuration_filename = nullptr,
    .f_configuration_directories = nullptr,
    .f_environment_flags = advgetopt::GETOPT_ENVIRONMENT_FLAG_PROCESS_SYSTEM_PARAMETERS,
    .f_help_header = "Usage: %p [-<opt>] ...\n"
                     "where -<opt> is one or more of:",
    .f_help_footer = "%c",
    .f_version = SNAPMANAGER_VERSION_STRING,
//    .f_version = SNAPWEBSITES_VERSION_STRING,
    .f_license = "GNU GPL v2",
    .f_copyright = "Copyright (c) 2013-"
                   BOOST_PP_STRINGIZE(UTC_BUILD_YEAR)
                   " by Made to Order Software Corporation -- All Rights Reserved",
    //.f_build_date = UTC_BUILD_DATE,
    //.f_build_time = UTC_BUILD_TIME
};
#pragma GCC diagnostic pop



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
    enum class token_type_t
    {
        TOKEN_UNKNOWN,              // unknown token

        TOKEN_EOT,                  // end of tokens
        TOKEN_KEYWORD,              // option names & values
        TOKEN_STRING,               // "..."
        TOKEN_OPEN_BLOCK,           // "{"
        TOKEN_CLOSE_BLOCK,          // "}"
        TOKEN_END_OF_DEFINITION,    // ";"

        // extensions
        //
        TOKEN_OPEN_INDEX,           // "["
        TOKEN_CLOSE_INDEX,          // "]"
        TOKEN_FIELD,                // "."
        TOKEN_ASSIGN,               // "="
        TOKEN_UPDATE,               // "+="
        TOKEN_CREATE,               // "?="

        // special cases
        //
        TOKEN_REMOVE,               // "=" "null"
        TOKEN_GET,                  // no "=", no value

        TOKEN_ERROR                 // an error occurred
    };

    class token
    {
    public:
        void            set_type(token_type_t type);
        void            set_word(std::string const & word);
        void            set_start(int start);
        void            set_end(int end);
        void            set_line(int line);
        void            set_block_level(int level);

        token &         operator += (char c);

        bool            is_null() const;

        token_type_t    get_type() const;
        std::string     get_word() const;
        int             get_start() const;
        int             get_end() const;
        int             get_line() const;
        int             get_block_level() const;

        void            set_end_of_value(int end);
        int             get_end_of_value() const;

    private:
        token_type_t    f_type = token_type_t::TOKEN_UNKNOWN;
        std::string     f_word = std::string();        // actual token (may be empty)
        int             f_start = -1;
        int             f_end = -1;
        int             f_end_of_value = -1;
        int             f_line = -1;
        int             f_block_level = -1;
    };

    class keyword
        : public std::enable_shared_from_this<keyword>
    {
    public:
        typedef std::shared_ptr<keyword>    pointer_t;
        typedef std::weak_ptr<keyword>      weak_t;
        typedef std::vector<pointer_t>      vector_t;

                        keyword(token const & t);

        token const &   get_token() const;

        void            set_command(token_type_t command);
        token_type_t    get_command() const;

        void            add_index(keyword::pointer_t k);
        void            add_field(keyword::pointer_t k);
        void            add_value(keyword::pointer_t k);

        vector_t const &    get_indexes() const;
        vector_t const &    get_fields() const;
        vector_t const &    get_values() const;

        int             field_start() const;
        int             field_end() const;
        int             value_start() const;
        int             value_end() const;
        int             field_value_start() const;
        int             field_value_end() const;

        pointer_t       get_parent() const;

    private:
        token           f_token = token();          // keyword

        mutable pointer_t f_parent = nullptr;

        vector_t        f_index = vector_t();       // keyword[index1][index2][...]
        vector_t        f_fields = vector_t();      // keyword[index1][index2][...].field1[index1][...].field2[index1][...]...

        token_type_t    f_command = token_type_t::TOKEN_GET;        // = += ?=, by default GET

        vector_t        f_value = vector_t();       // keyword | string (if "= null" command becomes REMOVE)
    };

                        dns_options(int argc, char * argv[]);

    int                 run();

private:
    int                 load_file();
    int                 save_file();
    int                 getc();
    void                ungetc(int c);
    token               get_token(bool extensions = false);

    int                 parse_command_line();
    int                 edit_option();
    int                 recursive_option(keyword::pointer_t p);
    int                 match();
    keyword::pointer_t  match_fields(size_t & field_idx, keyword::pointer_t opt, keyword::pointer_t & previous_level);
    bool                match_indexes(keyword::pointer_t k, keyword::pointer_t o);

    advgetopt::getopt   f_opt; // initialized in constructor
    bool                f_debug = false;
    bool                f_stdout = false;
    std::string         f_filename = std::string();
    std::string         f_execute = std::string();
    std::string         f_data = std::string();
    size_t              f_pos = 0;
    int                 f_line = 1;
    std::string         f_unget = std::string();
    token               f_token = token();
    int                 f_block_level = 0;
    keyword::pointer_t  f_keyword = keyword::pointer_t();
    keyword::pointer_t  f_options = keyword::pointer_t();
};




void dns_options::token::set_type(token_type_t type)
{
    f_type = type;
}


void dns_options::token::set_word(std::string const & word)
{
    f_word = word;
}


void dns_options::token::set_start(int start)
{
    f_start = start;
}


void dns_options::token::set_end(int end)
{
    f_end = end;
}


void dns_options::token::set_line(int line)
{
    f_line = line;
}


void dns_options::token:: set_block_level(int level)
{
    f_block_level = level;
}


dns_options::token & dns_options::token::operator += (char c)
{
    f_word += c;
    return *this;
}


bool dns_options::token::is_null() const
{
    return f_type == token_type_t::TOKEN_KEYWORD
        && f_word == "null";
}


dns_options::token_type_t dns_options::token::get_type() const
{
    return f_type;
}


std::string dns_options::token::get_word() const
{
    return f_word;
}


int dns_options::token::get_start() const
{
    return f_start;
}


int dns_options::token::get_end() const
{
    return f_end;
}


int dns_options::token::get_line() const
{
    return f_line;
}


int dns_options::token::get_block_level() const
{
    return f_block_level;
}


void dns_options::token::set_end_of_value(int end)
{
    f_end_of_value = end;
}


int dns_options::token::get_end_of_value() const
{
    return f_end_of_value;
}









dns_options::keyword::keyword(token const & t)
    : f_token(t)
{
}


dns_options::token const & dns_options::keyword::get_token() const
{
    return f_token;
}


void dns_options::keyword::set_command(token_type_t command)
{
    f_command = command;
}


dns_options::token_type_t dns_options::keyword::get_command() const
{
    return f_command;
}


void dns_options::keyword::add_index(keyword::pointer_t k)
{
    k->f_parent = shared_from_this();
    f_index.push_back(k);
}


void dns_options::keyword::add_field(keyword::pointer_t k)
{
    k->f_parent = shared_from_this();
    f_fields.push_back(k);
}


void dns_options::keyword::add_value(keyword::pointer_t k)
{
    k->f_parent = shared_from_this();
    f_value.push_back(k);
}


dns_options::keyword::vector_t const & dns_options::keyword::get_indexes() const
{
    return f_index;
}


dns_options::keyword::vector_t const & dns_options::keyword::get_fields() const
{
    return f_fields;
}


dns_options::keyword::vector_t const & dns_options::keyword::get_values() const
{
    return f_value;
}


int dns_options::keyword::field_start() const
{
    if(!f_fields.empty())
    {
        return f_fields[0]->get_token().get_start();
    }

    return -1;
}


int dns_options::keyword::field_end() const
{
    if(!f_fields.empty())
    {
        return (*f_fields.rbegin())->get_token().get_end();
    }

    return -1;
}


int dns_options::keyword::value_start() const
{
    if(!f_value.empty())
    {
        return f_value[0]->get_token().get_start();
    }

    return -1;
}


int dns_options::keyword::value_end() const
{
    if(!f_value.empty())
    {
        return (*f_value.rbegin())->get_token().get_end();
    }

    return -1;
}


int dns_options::keyword::field_value_start() const
{
    if(!f_fields.empty())
    {
        return f_fields[0]->get_token().get_start();
    }

    if(!f_value.empty())
    {
        return f_value[0]->get_token().get_start();
    }

    return -1;
}


int dns_options::keyword::field_value_end() const
{
    if(!f_value.empty())
    {
        return (*f_value.rbegin())->get_token().get_end();
    }

    if(!f_fields.empty())
    {
        return (*f_fields.rbegin())->get_token().get_end();
    }

    return -1;
}


dns_options::keyword::pointer_t dns_options::keyword::get_parent() const
{
    return f_parent;
}




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
    : f_opt(g_options_environment, argc, argv)
{
}


/** \brief Run the specified command.
 *
 */
int dns_options::run()
{
    // check the --debug
    //
    f_debug = f_opt.is_defined("debug");

    // check the --stdout
    //
    f_stdout = f_opt.is_defined("stdout");

    // make sure there is a filename
    //
    if(!f_opt.is_defined("--"))
    {
        std::cerr << f_opt.get_program_name()
                  << ":error: no filename was specified."
                  << std::endl;
        return 1;
    }

    // get the filename
    //
    f_filename = f_opt.get_string("--");
    if(f_filename.empty())
    {
        std::cerr << f_opt.get_program_name()
                  << ":error: an empty filename was specified."
                  << std::endl;
        return 1;
    }

    // --execute "<code>"
    //
    if(!f_opt.is_defined("execute"))
    {
        std::cerr << f_opt.get_program_name()
                  << ":error: mandatory --execute option missing."
                  << std::endl;
        return 1;
    }

    // get and parse the command line
    //
    f_execute = f_opt.get_string("execute");
    int r(parse_command_line());
    if(r != 0)
    {
        return r;
    }

    // read the options from the input file
    //
    r = edit_option();
    if(r != 0)
    {
        return r;
    }

    // then execute the command
    //
    match();

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
    snap::file_content file(f_filename);
    if(!file.read_all())
    {
        // could not open file for reading
        //
        std::cerr << "dns_options:error: can't open file \""
                  << f_filename
                  << "\" for reading."
                  << std::endl;
        return 1;
    }

    // ready the buffer
    //
    f_data = file.get_content();

    // if something bad happened, return 1, otherwise 0
    //
    return 0;
}


/** \brief Save the updated file.
 *
 * This function saves the f_data buffer back to file. It is expected that
 * the f_data was modified before re-saving.
 *
 * \attention
 * This tool is not responsible to create backups. You may want to write
 * a script that does that first:
 *
 * \code
 *      cp /etc/bind/named.conf.options /etc/bind/named.conf.options.bak
 *      dns_options --execute 'options.version = "none"' /etc/bind/named.conf.options
 * \endcode
 *
 * \par
 * One reason for not having an auto-backup is because you are very likely
 * to update multiple fields and then the very first version would be lost
 * anyway. Letting you create one backup with `cp` first is likely way
 * cleaner.
 *
 * \return 0 if no error occurred, 1 otherwise.
 */
int dns_options::save_file()
{
    snap::file_content file(f_filename);
    file.set_content(f_data);
    file.write_all();
    return 0;
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
    std::string::size_type unget_length(f_unget.length());
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
        if(c == '\r')
        {
            ++f_pos;        // skip the '\r'
            ++f_line;

            if(f_pos < f_data.size()
            && f_data[f_pos] == '\n')
            {
                ++f_pos;    // skip the '\n' "silently"
            }
            return '\n'; // always return '\n' so the rest of the code can ignore '\r' altogether
        }
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
 * \param[in] extensions  Whether the function supports our extensions.
 *
 * \return 0 when no errors were encountered, 1 otherwise.
 */
dns_options::token dns_options::get_token(bool extensions)
{
    for(;;)
    {
        token result;
        result.set_start(f_pos - f_unget.length());
        result.set_line(f_line);
        result.set_block_level(f_block_level);

        int c(getc());
        switch(c)
        {
        case EOF:
            result.set_type(token_type_t::TOKEN_EOT);
            result.set_end(f_pos - f_unget.length());
            return result;

        case ' ':
        case '\t':
        case '\n':
        case '\f':
            // ignore "noise"
            break;

        case '#':   // comment introducer
            do
            {
                c = getc();
            }
            while(c != EOF && c != '\n');
            // ignore "noise"
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
                while(c != EOF && c != '\n');
                // ignore "noise"
            }
            else if(c == '*')
            {
                // C like comment, search for "*/"
                // BIND does not accept a comment within a comment
                //
                for(c = getc();; c = getc())
                {
                    if(c == EOF)
                    {
                        std::cerr << "dns_options:error:"
                                  << f_filename
                                  << ":"
                                  << f_line
                                  << ": end of C-like comment not found before EOF."
                                  << std::endl;
                        result.set_type(token_type_t::TOKEN_ERROR);
                        result.set_end(f_pos - f_unget.length());
                        return result;
                    }
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
                // ignore "noise"
            }
            else
            {
                // this is a "lone" '/' character, continue token
                //
                ungetc(c);
                c = '/';
                goto read_token;
            }
            break;

        case '"':
            // WARNING: no single quote string support in BIND
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
                    // "start...\x5C
                    // ...end"
                    //
                    std::cerr << "dns_options:error:"
                              << f_filename
                              << ":"
                              << f_line
                              << ": quoted string includes a non-escaped newline."
                              << std::endl;
                    result.set_type(token_type_t::TOKEN_ERROR);
                    result.set_end(f_pos - f_unget.length());
                    return result;
                }
                result += c;
            }
            if(c != '"')
            {
                std::cerr << "dns_options:error:"
                          << f_filename
                          << ":"
                          << f_line
                          << ": quoted string was never closed."
                          << std::endl;
                result.set_type(token_type_t::TOKEN_ERROR);
                result.set_end(f_pos - f_unget.length());
                return result;
            }
            result.set_type(token_type_t::TOKEN_STRING);
            result.set_end(f_pos - f_unget.length());
            return result;

        case ';':
            result.set_type(token_type_t::TOKEN_END_OF_DEFINITION);
            result.set_end(f_pos - f_unget.length());
            return result;

        case '{':
            result.set_type(token_type_t::TOKEN_OPEN_BLOCK);
            ++f_block_level;
            result.set_end(f_pos - f_unget.length());
            result.set_block_level(f_block_level);
            return result;

        case '}':
            if(f_block_level <= 0)
            {
                std::cerr << "dns_options:error:"
                          << f_filename
                          << ":"
                          << f_line
                          << ": '}' mismatch, '{' missing for this one."
                          << std::endl;
                result.set_type(token_type_t::TOKEN_ERROR);
                result.set_end(f_pos - f_unget.length());
                return result;
            }
            --f_block_level;
            result.set_type(token_type_t::TOKEN_CLOSE_BLOCK);
            result.set_end(f_pos - f_unget.length());
            result.set_block_level(f_block_level);
            return result;

        case '[':
            if(extensions)
            {
                result.set_type(token_type_t::TOKEN_OPEN_INDEX);
                result.set_end(f_pos - f_unget.length());
                return result;
            }
            goto read_token;

        case ']':
            if(extensions)
            {
                result.set_type(token_type_t::TOKEN_CLOSE_INDEX);
                result.set_end(f_pos - f_unget.length());
                return result;
            }
            goto read_token;

        case '.':
            if(extensions)
            {
                result.set_type(token_type_t::TOKEN_FIELD);
                result.set_end(f_pos - f_unget.length());
                return result;
            }
            goto read_token;

        case '=':
            if(extensions)
            {
                result.set_type(token_type_t::TOKEN_ASSIGN);
                result.set_end(f_pos - f_unget.length());
                return result;
            }
            goto read_token;

        case '+':
            if(extensions)
            {
                c = getc();
                if(c == '=')
                {
                    result.set_type(token_type_t::TOKEN_UPDATE);
                    result.set_end(f_pos - f_unget.length());
                    return result;
                }
                ungetc(c);
                c = '+';
            }
            goto read_token;

        case '?':
            if(extensions)
            {
                c = getc();
                if(c == '=')
                {
                    result.set_type(token_type_t::TOKEN_CREATE);
                    result.set_end(f_pos - f_unget.length());
                    return result;
                }
                ungetc(c);
                c = '+';
            }
            goto read_token;

        default:
read_token:
            result += c;
            for(c = getc();; c = getc())
            {
                switch(c)
                {
                case EOF:
                case ' ':
                case '\t':
                case '\n':
                case '\f':
                    // no need to unget this one
                    //
                    result.set_type(token_type_t::TOKEN_KEYWORD);
                    result.set_end(f_pos - f_unget.length());
                    return result;

                case '{':
                case '}':
                case '"':
                case ';':
                case '#':
                    // restore that character too, it is a token on its own
                    //
                    ungetc(c);
                    result.set_type(token_type_t::TOKEN_KEYWORD);
                    result.set_end(f_pos - f_unget.length());
                    return result;

                case '/':
                    // is that the start of a comment?
                    // if so we found the end of this token
                    //
                    c = getc();
                    if(c == '/' || c == '*')
                    {
                        ungetc(c);
                        ungetc('/');
                        result.set_type(token_type_t::TOKEN_KEYWORD);
                        result.set_end(f_pos - f_unget.length());
                        return result;
                    }
                    ungetc(c);
                    result += '/'; // it's not a comment, make it part of the keyword
                    break;

                case '[':
                case ']':
                case '=':
                case '?':
                case '+':
                case '.':
                    if(extensions)
                    {
                        ungetc(c);
                        result.set_type(token_type_t::TOKEN_KEYWORD);
                        result.set_end(f_pos - f_unget.length());
                        return result;
                    }
                    result += c;
                    break;

                default:
                    result += c;
                    break;

                }
            }
            snap::NOTREACHED();
            break;

        }
    }
}


/** \brief Parse the command line.
 *
 * See the main() function documentation for the definition of the
 * command line.
 *
 * \return 0 on success, 1 on error.
 */
int dns_options::parse_command_line()
{
    int r(0);

    f_pos = 0;
    f_line = 1;
    f_block_level = 0;

    f_data = f_execute;

    // the command line must start with a keyword
    //
    //     keyword
    //
    token t(get_token(true));
    if(t.get_type() != token_type_t::TOKEN_KEYWORD)
    {
        if(t.get_type() != token_type_t::TOKEN_ERROR)
        {
            std::cerr << "dns_options:error:<execute>:"
                      << f_line
                      << ": we first expected a keyword on your command line."
                      << std::endl;
        }
        return 1;
    }

    f_keyword = std::make_shared<keyword>(t);

    auto get_index = [&](keyword::pointer_t p)
        {
            for(;;)
            {
                t = get_token(true);
                switch(t.get_type())
                {
                case token_type_t::TOKEN_ERROR:
                    return 1;

                case token_type_t::TOKEN_KEYWORD:
                case token_type_t::TOKEN_STRING:
                    {
                        keyword::pointer_t i(std::make_shared<keyword>(t));
                        p->add_index(i);
                    }
                    break;

                default:
                    std::cerr << "dns_options:error:<execute>:"
                              << f_line
                              << ": we expected a keyword or a quoted string as an index."
                              << std::endl;
                    return 1;

                }

                // after that (keyword | "string") we expect the ']'
                //
                t = get_token(true);
                if(t.get_type() != token_type_t::TOKEN_CLOSE_INDEX)
                {
                    std::cerr << "dns_options:error:<execute>:"
                              << f_line
                              << ": we expected a ']' to close an index."
                              << std::endl;
                    return 1;
                }
                // skip the ']'
                //
                t = get_token(true);

                // if we do not have another '[' then we are done with indexes
                //
                if(t.get_type() != token_type_t::TOKEN_OPEN_INDEX)
                {
                    return 0;
                }
            }
        };

    t = get_token(true);
    if(t.get_type() == token_type_t::TOKEN_OPEN_INDEX)
    {
        // the keyword can be followed by any number of indexes
        //
        //      keyword [ index1 ] [ index2 ] ...
        //
        r = get_index(f_keyword);
        if(r != 0)
        {
            return r;
        }
    }

    while(t.get_type() == token_type_t::TOKEN_FIELD)
    {
        // the keyword and indexes can be followed by a field
        // and the field can be followed by indexes, any number
        // of fields with indexes can be defined
        //
        //      keyword [ index1 ] [ index2 ] . field [ index1 ] [ index2 ] ...
        //
        t = get_token(true);

        if(t.get_type() == token_type_t::TOKEN_ERROR)
        {
            return 1;
        }

        if(t.get_type() != token_type_t::TOKEN_KEYWORD
        && t.get_type() != token_type_t::TOKEN_STRING)
        {
            std::cerr << "dns_options:error:<execute>:"
                      << f_line
                      << ": we expected a keyword or a quoted string as the field name."
                      << std::endl;
            return 1;
        }

        keyword::pointer_t f(std::make_shared<keyword>(t));
        f_keyword->add_field(f);

        t = get_token(true);
        if(t.get_type() == token_type_t::TOKEN_OPEN_INDEX)
        {
            r = get_index(f);
            if(r != 0)
            {
                return r;
            }
        }
    }

    // if that's it (i.e. EOT), we have a GET
    //
    switch(t.get_type())
    {
    case token_type_t::TOKEN_EOT:
        // it worked, we have a GET
        //
        return 0;

    case token_type_t::TOKEN_END_OF_DEFINITION:
        t = get_token(true);
        if(t.get_type() != token_type_t::TOKEN_EOT)
        {
            std::cerr << "dns_options:error:<execute>:"
                      << f_line
                      << ": nothing was expected after the ';'."
                      << std::endl;
            return 1;
        }

        // it worked, we have a GET (totally ignore the ';')
        //
        return 0;

    case token_type_t::TOKEN_ASSIGN:
    case token_type_t::TOKEN_UPDATE:
    case token_type_t::TOKEN_CREATE:
        // here we have a SET, CREATE, UPDATE or a REMOVE depeneding on
        // the assignment token and the value; the assignment operator
        // is called the command which by default is set to GET
        //
        //      keyword [ index1 ] . field [ index1 ] ( = | += | ?= ) ...
        //
        f_keyword->set_command(t.get_type());
        break;

    default:
        std::cerr << "dns_options:error:<execute>:"
                  << f_line
                  << ": end of line or an assignment operator (=, ?=, +=) was expected."
                  << std::endl;
        return 1;

    }

    // we have an assignment, read the value
    //
    t = get_token(false);

    if(t.is_null())
    {
        if(f_keyword->get_command() != token_type_t::TOKEN_ASSIGN)
        {
            std::cerr << "dns_options:error:<execute>:"
                      << f_line
                      << ": an assignment to null only works with the '=' operator."
                      << std::endl;
            return 1;
        }
        f_keyword->set_command(token_type_t::TOKEN_REMOVE);

        t = get_token(false);
        if(t.get_type() == token_type_t::TOKEN_END_OF_DEFINITION)
        {
            t = get_token(false);
        }
        if(t.get_type() != token_type_t::TOKEN_EOT)
        {
            std::cerr << "dns_options:error:<execute>:"
                      << f_line
                      << ": an assignment to null cannot include anything else."
                      << std::endl;
            return 1;
        }

        // it worked, we have a REMOVE
        //
        return 0;
    }

    // read the value
    //
    for(;; t = get_token(false))
    {
        switch(t.get_type())
        {
        case token_type_t::TOKEN_EOT:
            return 0;

        case token_type_t::TOKEN_END_OF_DEFINITION:
            t = get_token(false);
            if(t.get_type() != token_type_t::TOKEN_EOT)
            {
                std::cerr << "dns_options:error:<execute>:"
                          << f_line
                          << ": nothing was expected after the ';'."
                          << std::endl;
                return 1;
            }
            return 0;

        case token_type_t::TOKEN_KEYWORD:
        case token_type_t::TOKEN_STRING:
            {
                keyword::pointer_t v(std::make_shared<keyword>(t));
                f_keyword->add_value(v);
            }
            break;

        default:
            std::cerr << "dns_options:error:<execute>:"
                      << f_line
                      << ": the command line value cannot include a block."
                      << std::endl;
            return 1;

        }
    }
}


/** \brief Search for an option, if not present, add it.
 *
 * This function parses the options file transforming it into tokens.
 *
 * The function checks those tokens against the option being edited.
 * First, the function searches for the blocks (blocks start with `{`
 * where the option is expected to be defined
 * (i.e. `\<name> { block-with-option }`).
 *
 * Note that at each new option we save the current parser position.
 * This is used to remove the option entirely in case the command
 * was `--remove`.
 *
 * Similarly, once an option name was parsed, we save the beginning
 * and end positions of the value of that option. This gives us the
 * ability to edit that value.
 *
 * Finally, if the option is not found in the block expected to hold
 * it, the save the position before the closing curvly brace (}) so
 * we can insert  the option there if the command asks us to do so.
 *
 * If the block is not even found, then the function can still add
 * the option by creating the whole block along the way.
 *
 * \return 0 on success, 1 on error, 2 if the option already exists.
 */
int dns_options::edit_option()
{
    load_file();

    f_options = std::make_shared<keyword>(token());

    int r(0);
    for(;;)
    {
        token t(get_token());
        if(t.get_type() == token_type_t::TOKEN_EOT)
        {
            // done?
            //
            break;
        }
        if(t.get_type() == token_type_t::TOKEN_ERROR)
        {
            // got an error while reading input file
            //
            return 1;
        }

        // got a keyword
        //
        keyword::pointer_t k(std::make_shared<keyword>(t));

        // read until we find an end of definition (a.k.a. ';')
        //
        bool more(true);
        do
        {
            t = get_token();
            switch(t.get_type())
            {
            case token_type_t::TOKEN_EOT:
                std::cerr << "dns_options:error: found EOT, expected a ';' before the end of the file."
                          << std::endl;
                return 1;

            case token_type_t::TOKEN_ERROR:
                return 1;

            case token_type_t::TOKEN_END_OF_DEFINITION:
                more = false;
                break;

            case token_type_t::TOKEN_OPEN_BLOCK:
                r = recursive_option(k);
                if(r != 0)
                {
                    return r;
                }
                break;

            case token_type_t::TOKEN_CLOSE_BLOCK:
                std::cerr << "dns_options:error: found '}' without first finding a '{'."
                          << std::endl;
                return 1;

            case token_type_t::TOKEN_KEYWORD:
            case token_type_t::TOKEN_STRING:
                k->add_field(std::make_shared<keyword>(t));
                break;

            default:
                std::cerr << "dns_options:error: unexpected token "
                          << static_cast<int>(t.get_type())
                          << "."
                          << std::endl;
                return 1;

            }
        }
        while(more);

        const_cast<token &>(k->get_token()).set_end_of_value(f_pos - f_unget.length());
        f_options->add_value(k);
    }

    return 0;
}


/** \brief Read the content of a block recursively
 *
 * This function reads the contents of one block ('{ ... }').
 *
 * Each line is added as a value of the \p in keyword which you pass as
 * a parameter.
 *
 * \note
 * When an error happens at any level of recursivity, it will unwind the
 * whole stack at once and return 1.
 *
 * \param[in] in  The keyword that will hold the block value.
 *
 * \return 0 when the block was read successfully, 1 otherwise
 */
int dns_options::recursive_option(keyword::pointer_t in)
{
    int r(0);
    for(;;)
    {
        token t(get_token());
        if(t.get_type() == token_type_t::TOKEN_EOT)
        {
            // done?
            //
            std::cerr << "dns_options:error: found end of input before '}'."
                      << std::endl;
            return 1;
        }
        if(t.get_type() == token_type_t::TOKEN_ERROR)
        {
            // got an error while reading input file
            //
            return 1;
        }
        if(t.get_type() == token_type_t::TOKEN_CLOSE_BLOCK)
        {
            // proper end of block, return now
            //
            return 0;
        }

        // got a keyword
        //
        keyword::pointer_t k(std::make_shared<keyword>(t));

        // read until we find an end of definition (a.k.a. ';')
        //
        bool more(true);
        do
        {
            t = get_token();
            switch(t.get_type())
            {
            case token_type_t::TOKEN_EOT:
                std::cerr << "dns_options:error: found EOT, expected a ';' before the end of the file."
                          << std::endl;
                return 1;

            case token_type_t::TOKEN_ERROR:
                return 1;

            case token_type_t::TOKEN_END_OF_DEFINITION:
                more = false;
                break;

            case token_type_t::TOKEN_OPEN_BLOCK:
                r = recursive_option(k);
                if(r != 0)
                {
                    return r;
                }
                break;

            case token_type_t::TOKEN_CLOSE_BLOCK:
                // end of block, return
                //
                std::cerr << "dns_options:warning: found '}' without a ';' to end the last line."
                          << std::endl;
                in->add_value(k);
                return 0;

            case token_type_t::TOKEN_KEYWORD:
            case token_type_t::TOKEN_STRING:
                k->add_field(std::make_shared<keyword>(t));
                break;

            default:
                std::cerr << "dns_options:error: unexpected token "
                          << static_cast<int>(t.get_type())
                          << "."
                          << std::endl;
                return 1;

            }
        }
        while(more);

        const_cast<token &>(k->get_token()).set_end_of_value(f_pos - f_unget.length());
        in->add_value(k);
    }

    snap::NOTREACHED();
    return 1;
}



/** \brief Go through and apply the command line expression.
 *
 * This function attempts to match the command line expression to the
 * file we just loaded and apply the command line operation as required.
 *
 * Supported operations are:
 *
 * \li GET (no assignment) -- retrieve a field's value; it gets printed in stdout
 * \li ASSIGN (=) -- add or update a field's value; by default the file is
 * modified with the change, use --stdout to get the result in the console
 * \li UPDATE (?=) -- update a field's value; like ASSIGN except that the value
 * must already exist, nothing happens otherwise
 * \li CREATE (+=) -- add a field with its value; if the parameter already
 * exists, leave it alone, otherwise add it at the end of the existing
 * value like the ASSIGN would otherwise do
 * \li REMOVE (= null) -- remove the field if it exists
 *
 * This currently works well for standalone fields. Fields for which you
 * want to replace an entire block (the whole value between quotes) is
 * likelyt to fail badly.
 *
 * \bug
 * Test and correct as required this implementation so we can update
 * any field including a whole block. At this time this does not work
 * properly.
 *
 * \bug
 * The creation is not recursive. So if you want to create `a { b { c 123; } }`
 * where `a` or `b` do not yet exist, it will not work. If `a` and `b` do not
 * exist, but `c` does exist, then we create `c` as expected.
 *
 * \bug
 * There are potential problems with the use of `"*"` and the creation of
 * fields that end up being set to `"*"` instead of an actual index name.
 */
int dns_options::match()
{
    // the match is in f_keyword
    //
    // what to match (the files of options) is in f_options
    //

// 'options.version [+?]= "none"'
// 'logging.channel["logs"].print-category [+?]= yes'
// 'logging.channel["logs"].print-time = null'
// 'logging.channel["*"].severity'
//
// sample data:
//
// options { version "1.2.3"; }
// logging { channel "any-name" { severity 123 } }

    auto const & k(f_keyword->get_token());
    auto const   type(k.get_type());
    auto const & word(k.get_word());

    auto const & values(f_options->get_values());
    for(auto const & v : values)
    {
        auto const o(v->get_token());
#if 0
std::cerr << "types: "
            << static_cast<int>(o.get_type())
            << "/"
            << static_cast<int>(type)
          << ", words: "
            << o.get_word()
            << "/"
            << word
          << "\n";
#endif
        if(o.get_type() == type
        && o.get_word() == word
        && match_indexes(f_keyword, v))
        {
            // if f_keyword has further fields, then we need to go further
            //
            keyword::pointer_t previous_level;
            size_t field_idx(0);
            keyword::pointer_t result(match_fields(field_idx, v, previous_level));
#if 0
std::cerr << " == matching result " << (result == nullptr ? "nullptr" : "YES")
          << ", previous_level " << (previous_level == nullptr ? "nullptr" : "YES")
          << "\n";
#endif
            if(result != nullptr)
            {
                int const start(result->field_value_start());
                int const end(result->field_value_end());

#if 0
std::cerr << "------- found [" << f_execute << "] (" << start << ", " << end << ")!\n";
#endif
                switch(f_keyword->get_command())
                {
                case token_type_t::TOKEN_ASSIGN:
                case token_type_t::TOKEN_UPDATE:
                    {
                        auto const & field(*f_keyword->get_fields().rbegin());
                        std::string field_name(field->get_token().get_word());
                        if(field_name == "_")
                        {
                            // nothing to do, the unnamed value already exists
                            //
                            break;
                        }

                        int const replacement_start(f_keyword->value_start());
                        int const replacement_end(f_keyword->value_end());
#if 0
std::cerr << "------- replacement ["
                    << f_execute.substr(replacement_start, replacement_end - replacement_start)
                    << "] -> ["
                    << f_execute.substr(replacement_end)
                    << "] (" << replacement_start << ", "
                    << replacement_end
                    << ", "
                    << replacement_end - replacement_start
                    << ")!\n";
#endif

                        if(start == -1
                        || end == -1
                        || replacement_start == -1
                        || replacement_end == -1)
                        {
                            std::cerr << "dns_options:error: start/end parameters not properly defined to SET/UPDATE this value."
                                      << std::endl;
                            return 1;
                        }

                        f_data = f_data.substr(0, start)
                               + f_execute.substr(replacement_start, replacement_end - replacement_start)
                               + f_data.substr(end);
                        if(f_stdout)
                        {
                            std::cout << f_data;
                        }
                        else
                        {
                            return save_file();
                        }
                    }
                    break;

                case token_type_t::TOKEN_CREATE:
                    // it exists, do not modify it in this case
                    break;

                case token_type_t::TOKEN_REMOVE:
                    // we have to remove that entry, `result` represents
                    // the value, so we have to get the parent and determine
                    // the start end of the parent instead
                    //
                    {
                        keyword::pointer_t parent(result->get_parent());
                        if(parent == nullptr)
                        {
                            std::cerr << "dns_options:error: no parent field found for a REMOVE."
                                      << std::endl;
                            return 1;
                        }

                        auto const & vals(parent->get_values());
                        auto vit(std::find(
                                  vals.begin()
                                , vals.end()
                                , result));
                        if(vit == vals.end())
                        {
                            std::cerr << "dns_options:error: invalid result, could not find it in the parent list of values."
                                      << std::endl;
                            return 1;
                        }

                        //int const remove_start(parent->field_value_start());
                        int const remove_start(result->get_token().get_start());

                        int remove_end(remove_start);
                        ++vit;
                        if(vit == vals.end())
                        {
#if 0
// I think that get_end_of_value() is better
                            // it was the last value... remove following
                            // spaces and ';'
                            //
                            // TODO: note that at this time a value inside braces
                            //       does not get removed correctly; we need to
                            //       look into a way to have a start/end in
                            //       the "parent" which encompasses the entire
                            //       value including the braces
                            //
                            for(remove_end = end; remove_end < static_cast<int>(f_data.length()); ++remove_end)
                            {
                                switch(f_data[remove_end])
                                {
                                case ';':
                                    ++remove_end;
                                    break;

                                case ' ':
                                case '\t':
                                case '\n':
                                case '\f':
                                    continue;

                                default:
                                    // keep any other character
                                    break;

                                }
                                break;
                            }
#endif
                            remove_end = parent->get_token().get_end_of_value();
                        }
                        else
                        {
                            // we have a following value, use its start point
                            // as our end point
                            //
                            remove_end = (*vit)->get_token().get_start();
                        }

                        f_data = f_data.substr(0, remove_start)
                               + f_data.substr(remove_end);
                        if(f_stdout)
                        {
                            std::cout << f_data;
                        }
                        else
                        {
                            return save_file();
                        }
                    }
                    break;

                case token_type_t::TOKEN_GET:
                    // print current value, so result->get_values()
                    //
                    if(start == -1
                    || end == -1)
                    {
                        std::cerr << "dns_options:error: start/end parameters not properly defined to GET this value."
                                  << std::endl;
                        return 1;
                    }

                    std::cout << f_data.substr(start, end - start) << std::endl;
                    break;

                default:
                    std::cerr << "dns_options:fatal error: unknown command in match()."
                              << std::endl;
                    return 1;

                }
                return 0;
            }
            else if(previous_level != nullptr)
            {
                switch(f_keyword->get_command())
                {
                case token_type_t::TOKEN_ASSIGN:
                case token_type_t::TOKEN_CREATE:
                    {
                        int end(previous_level->get_token().get_end_of_value());
                        if(end > 0
                        && f_data[end - 1] == ';')
                        {
                            --end;
                            // TODO: skip spaces too
                            if(end > 0
                            && f_data[end - 1] == '}')
                            {
                                --end;
                            }
                        }
                        int start(end);
                        while(start > 0
                           && f_data[start - 1] == '\n')
                        {
                            --start;
                        }

                        // here field_idx represents the index that matched,
                        // we want to increment it once to get at the right
                        // location
                        //
                        size_t const max(f_keyword->get_fields().size());
                        auto const & fields(f_keyword->get_fields());
                        std::string field_names;
                        std::string end_field;
                        size_t idx(field_idx);
                        for(; idx < max; ++idx)
                        {
                            std::string const name(fields[idx]->get_token().get_word());

                            // the special name "_" means that there is no
                            // name for that field
                            //
                            if(name != "_")
                            {
                                field_names += name;
                                for(auto i : fields[idx]->get_indexes())
                                {
                                    field_names += ' ';
                                    auto const & tok(i->get_token());
                                    if(tok.get_type() == token_type_t::TOKEN_STRING)
                                    {
                                        if(tok.get_word() == "*")
                                        {
                                            std::cerr << "dns_options:error: you cannot create or update a field using \"*\" as one of its indices."
                                                      << std::endl;
                                            return 1;
                                        }
                                        field_names += '"';
                                        field_names += tok.get_word();
                                        field_names += '"';
                                    }
                                    else
                                    {
                                        field_names += i->get_token().get_word();
                                    }
                                }
                                field_names += " ";

                                if(idx + 1 < max)
                                {
                                    field_names += "{\n\t";
                                    for(size_t j(0); j < idx + 1; ++j)
                                    {
                                        field_names += '\t';
                                        end_field += '\t';
                                    }
                                    end_field += "};\n";
                                }
                            }
                        }

                        int const replacement_start(f_keyword->value_start());
                        int const replacement_end(f_keyword->value_end());
#if 0
std::cerr << "------- CREATE: replacement [" << f_execute.substr(replacement_start, replacement_end - replacement_start)
                    << "] -> ["
                    << f_execute.substr(replacement_end)
                    << "] (" << replacement_start << ", "
                    << replacement_end
                    << ", "
                    << replacement_end - replacement_start
                    << ")!\n";
#endif

                        if(start == -1
                        || end == -1
                        || replacement_start == -1
                        || replacement_end == -1)
                        {
                            std::cerr << "dns_options:error: start/end parameters not properly defined to SET/CREATE this value."
                                      << std::endl;
                            return 1;
                        }

                        // here the added newlines and tab are quite arbitrary...
                        //
                        f_data = f_data.substr(0, start)
                               + "\n\t"
                                    + field_names
                                    + f_execute.substr(replacement_start, replacement_end - replacement_start)
                                            + ";\n"
                                    + end_field
                               + f_data.substr(end);
                        if(f_stdout)
                        {
                            std::cout << f_data;
                        }
                        else
                        {
                            return save_file();
                        }
                    }
                    return 0;

                case token_type_t::TOKEN_UPDATE:
                case token_type_t::TOKEN_REMOVE:
                    // these are silent ones, there is nothing to update
                    // or remove but we do not tell anything to the user
                    //
                    return 0;

                case token_type_t::TOKEN_GET:
                    // error below: field not found
                    break;

                default:
                    std::cerr << "dns_options:fatal error: unknown command in match()."
                              << std::endl;
                    return 1;

                }
            }

            // if we reach here, we had a partial match
            //
            break;
        }
    }

    switch(f_keyword->get_command())
    {
    case token_type_t::TOKEN_ASSIGN:
    case token_type_t::TOKEN_CREATE:
        {
            // TODO: the following only supports one level
            //       multiple levels require the '{' ... '}' at each level
            //
            std::string field_names(word);
            std::string end_field;
            for(auto i : f_keyword->get_indexes())
            {
                field_names += ' ';
                auto const & tok(i->get_token());
                if(tok.get_type() == token_type_t::TOKEN_STRING)
                {
                    if(tok.get_word() == "*")
                    {
                        std::cerr << "dns_options:error: you cannot create or update a field using \"*\" as one of its indices."
                                  << std::endl;
                        return 1;
                    }
                    field_names += '"';
                    field_names += tok.get_word();
                    field_names += '"';
                }
                else
                {
                    field_names += i->get_token().get_word();
                }
            }
            field_names += " ";

            size_t const max(f_keyword->get_fields().size());
            auto const & fields(f_keyword->get_fields());
            for(size_t idx(0); idx < max; ++idx)
            {
                std::string const name(fields[idx]->get_token().get_word());

                // the special name "_" means that there is no
                // name for that field
                //
                if(name != "_")
                {
                    if(idx + 1 < max)
                    {
                        field_names += "{\n";
                        for(size_t j(0); j < idx + 1; ++j)
                        {
                            field_names += '\t';
                            end_field += '\t';
                        }
                        end_field += "};\n";
                    }

                    field_names += name;
                    for(auto i : fields[idx]->get_indexes())
                    {
                        field_names += ' ';
                        auto const & tok(i->get_token());
                        if(tok.get_type() == token_type_t::TOKEN_STRING)
                        {
                            if(tok.get_word() == "*")
                            {
                                std::cerr << "dns_options:error: you cannot create or update a field using \"*\" as one of its indices."
                                          << std::endl;
                                return 1;
                            }
                            field_names += '"';
                            field_names += tok.get_word();
                            field_names += '"';
                        }
                        else
                        {
                            field_names += i->get_token().get_word();
                        }
                    }
                    field_names += " ";
                }
            }

            int const replacement_start(f_keyword->value_start());
            int const replacement_end(f_keyword->value_end());
#if 0
std::cerr << "------- TOTAL CREATE: replacement [" << f_execute.substr(replacement_start, replacement_end - replacement_start)
        << "] -> ["
        << f_execute.substr(replacement_end)
        << "] (" << replacement_start << ", "
        << replacement_end
        << ", "
        << replacement_end - replacement_start
        << ")!\n";
#endif

            if(replacement_start == -1
            || replacement_end == -1)
            {
                std::cerr << "dns_options:error: start/end parameters not properly defined to SET/CREATE this value."
                          << std::endl;
                return 1;
            }

            // make sure we have at least one empty line after the last option
            //
            if(f_data.length() >= 1
            && f_data[f_data.length() - 1] != '\n')
            {
                f_data += "\n";
            }
            if(f_data.length() >= 2
            && f_data[f_data.length() - 2] != '\n')
            {
                f_data += "\n";
            }

            std::string replacement;
            for(size_t idx(0); idx < max; ++idx)
            {
                replacement += '\t';
            }
            replacement += f_execute.substr(replacement_start, replacement_end - replacement_start);

            // here the added newlines and tab are quite arbitrary...
            //
            f_data += field_names
                      + "{\n"
                        + replacement
                      + ";\n"
                    + end_field
                    + "};\n"
                    + "\n";
            if(f_stdout)
            {
                std::cout << f_data;
            }
            else
            {
                return save_file();
            }
        }
        return 0;

    default:
        // only the ASSIGN and CREATE do miracles in this case
        break;

    }

    std::cerr << "dns_options:error: field \""
              << f_execute
              << "\" was not found."
              << std::endl;

    return 1;
}


dns_options::keyword::pointer_t dns_options::match_fields(size_t & field_idx, keyword::pointer_t opt, keyword::pointer_t & previous_level)
{
    auto const & fields(f_keyword->get_fields());
#if 0
std::cerr << "number of fields in match_fields: " << fields.size() << std::endl;
#endif
    if(field_idx >= fields.size())
    {
        // we reached the end of the fields defined on the command line
        //
        return opt;
    }

    previous_level = opt;

    auto const & values(opt->get_values());
    if(values.empty())
    {
        // we reached the end of the file options, this is not a match
        //
        return keyword::pointer_t();
    }

    auto const & f(fields[field_idx]);
    auto const & k(f->get_token());
    auto const   type(k.get_type());
    auto const & word(k.get_word());
    for(auto const & v : values)
    {
        auto const o(v->get_token());

#if 0
std::cerr << "  --- types: "
        << static_cast<int>(o.get_type())
        << "/"
        << static_cast<int>(type)
      << ", words: "
        << o.get_word()
        << "/"
        << word
      << " -> match_indexes = "
        << match_indexes(f, v)
      << "\n";
#endif

        if(o.get_type() == type
        && match_indexes(f, v))
        {
            if(word == "_")
            {
                // special case where we have to match the value, not the field
                // name (i.e. when there is no field name within the block)
                //
                // in this case we do not expect indexes, although it if
                // that's the case then match_indexes() returns true
                //
                // TODO: find a way to calculate the replacement only once
                //
                int const replacement_start(f_keyword->value_start());
                int const replacement_end(f_keyword->value_end());
                std::string replacement(f_execute.substr(replacement_start, replacement_end - replacement_start));
                if(o.get_word() == replacement)
                {
#if 0
std::cerr << "    *** MATCH VALUE! [" << replacement << "]\n";
#endif
                    ++field_idx;
                    return v;
                }
            }
            else if(o.get_word() == word)
            {
//std::cerr << "    *** MATCHED FIELDS!\n";
                ++field_idx;
                return match_fields(field_idx, v, previous_level);
            }
        }
    }

    return keyword::pointer_t();
}


bool dns_options::match_indexes(keyword::pointer_t kwd, keyword::pointer_t opt)
{
    // WARNING: the keywords (kwd--command line) have indexes
    //          which should match fields in object (opt--file contents)
    //
    auto const & expected_fields(kwd->get_indexes());
    auto const & existing_fields(opt->get_fields());

    size_t const max(expected_fields.size());
    if(max > existing_fields.size())
    {
        return false;
    }

    for(size_t idx(0); idx < max; ++idx)
    {
        auto const & expected_token(expected_fields[idx]->get_token());
        auto const & existing_token(existing_fields[idx]->get_token());
        if(expected_token.get_type() == token_type_t::TOKEN_KEYWORD)
        {
            // keywords have to match one to one
            //
            if(existing_token.get_type() != token_type_t::TOKEN_KEYWORD)
            {
                return false;
            }
            if(expected_token.get_word() != existing_token.get_word())
            {
                return false;
            }
        }
        else if(expected_token.get_type() == token_type_t::TOKEN_STRING)
        {
            if(existing_token.get_type() != token_type_t::TOKEN_KEYWORD
            && existing_token.get_type() != token_type_t::TOKEN_STRING)
            {
                return false;
            }
            std::string const word(expected_token.get_word());
            if(word != "*")
            {
                // words need to be a match
                //
                if(word != existing_token.get_word())
                {
                    return false;
                }
            }
        }
        else
        {
            return false;
        }
#if 0
std::cerr << "existing token in indexes: value start = "
          << existing_fields[idx]->value_start()
          << ", end = "
          << existing_fields[idx]->value_end()
          << ", field start = "
          << existing_fields[idx]->field_start()
          << ", end = "
          << existing_fields[idx]->field_end()
          << ", field value start = "
          << existing_fields[idx]->field_value_start()
          << ", end = "
          << existing_fields[idx]->field_value_end()
          << "\n";
#endif
    }

    return true;
}





}
// no name namespace







/** \brief Implement the main() command.
 *
 * This tool accepts command lines that are used to edit
 * BIND configuration files. It accepts and execution expression
 * and a filename to be edited.
 *
 * The expression is more or less defined as "variable-name" = "value".
 * The exact syntax is defined as:
 *
 * \code
 * <keyword> ( '[' <keyword> | '"' <string> '"' ']' )*
 *           ('.' <keyword> | '"' <string> '"'
 *              ( '[' <keyword> | '"' <string> '"' ']' )* )*
 *           ( ( '?' | '+' )? '=' ( 'null'
 *                  | (<keyword> | '"' <string> '"' )+ ) )?
 * \endcode
 *
 * This means:
 *
 * \li a keyword such as "options" (without the quotes)
 * \li optionally followed by one or more indexes defined as keywords or
 *     quoted strings
 * \li if no assignment follows, then the command is a GET
 * \li one of the supported assignment operators: '=' (SET), '?=' (SET if
 *     not yet defined), or '+=' (REPLACE, set if already defined)
 * \li the new value, if the "null" keyword is used (without the quotes)
 *     then the command is a REMOVE instead of an assignment; otherwise
 *     the keywords and quoted strings concatenated represent the new value.
 *
 * So for example to force the value of the `version` parameter in the
 * `options` block to the new value `"none"`, one writes:
 *
 * \code
 *    cd /var/bind
 *    sudo dns_options --execute 'options.version = "none"' named.conf.options
 * \endcode
 *
 * If instead you wanted to set the version only if not already set, use
 * the `?=` operator instead:
 *
 * \code
 *    cd /var/bind
 *    sudo dns_options --execute 'options.version ?= "none"' named.conf.options
 * \endcode
 *
 * And to update the version in case it is defined (leave it to its default
 * otherwise) then use the `+=` operator instead:
 *
 * \code
 *    cd /var/bind
 *    sudo dns_options --execute 'options.version += "none"' named.conf.options
 * \endcode
 *
 * The index can be used to make changes to the logs channel parameters as in;
 *
 * \code
 *    cd /var/bind
 *    sudo dns_options --execute 'logging.channel["logs"].print-category = yes' named.conf.options
 * \endcode
 *
 * To remove a parameter, such as the print-time of the logging channel:
 *
 * \code
 *    cd /var/bind
 *    sudo dns_options --execute 'logging.channel["logs"].print-time = null' named.conf.options
 * \endcode
 *
 * Finally, you may get the value, which gets printed in stdout, by not
 * assigning a value as in:
 *
 * \code
 *    cd /var/bind
 *    sudo dns_options --execute 'logging.channel["logs"].severity' named.conf.options
 * \endcode
 *
 * This last command may print:
 *
 * \code
 *    info
 * \endcode
 *
 * in your console.
 *
 * The system is capable of accepting any keyword or quoted string (although
 * the type is still checked) when using the asterisk as is:
 *
 * \code
 *    cd /var/bind
 *    sudo dns_options --execute 'logging.channel["*"].severity' named.conf.options
 * \endcode
 *
 * This means a named.conf file with:
 *
 * \code
 *    logging { channel "any-name" { severity 123 } }
 * \endcode
 *
 * will match and that last command returns 123 in your console. There is
 * another example where the asterisk is used in place of a keyword:
 *
 * \code
 *    cd /var/bind
 *    sudo dns_options --execute 'logging.*["logs"].severity' named.conf.options
 * \endcode
 *
 * Note that in BIND certain commands only accept quoted strings such as
 * `"none"`. This is why you need the single quotes around the whole
 * parameter of the --execute command. BIND does not accept strings using
 * single quotes. So there is no need to inverse the option. If you want
 * to use a dynamic parameter you can close and reopen as in:
 *
 * \code
 *    cd /var/bind
 *    sudo dns_options --execute 'options.query-source = address '$ADDR' port 53' named.conf.options
 * \endcode
 *
 * This assumes that the content of `$ADDR` is valid (i.e. it does not
 * include spaces, for example.)
 *
 * The value on the right of the assignment is going to be copied to
 * the configuration file pretty much verbatim (extra spaces and
 * comments are removed) so you want to make sure it is written as
 * expected by BIND.
 *
 * \warning
 * At this time the tool is not capable of executing more than one
 * command at a time (i.e. it does not work like a script.) Use the
 * command multiple times to add/update/remove multiple fields.
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
    catch( advgetopt::getopt_exit const & except )
    {
        return except.code();
    }
    catch(std::exception const & e)
    {
        std::cerr << "dns_options:error: an exception occurred: "
                  << e.what()
                  << std::endl;
        r = 1;
    }
    return r;
}


// vim: ts=4 sw=4 et
