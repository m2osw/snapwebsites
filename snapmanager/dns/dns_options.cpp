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
//#include "dns.h"

// advgetopt lib
//
#include "advgetopt/advgetopt.h"

// snapwebsites lib
//
//#include <snapwebsites/chownnm.h>
#include <snapwebsites/file_content.h>
//#include <snapwebsites/log.h>
//#include <snapwebsites/mkdir_p.h>
#include <snapwebsites/not_reached.h>
//#include <snapwebsites/not_used.h>
#include <snapwebsites/version.h>

// Qt5
//
//#include <QtCore>

// C++ lib
//
#include <iostream>
#include <vector>



// last entry
//
#include <snapwebsites/poison.h>


namespace
{

std::vector<std::string> const g_configuration_files; // Empty

advgetopt::getopt::option const g_options[] =
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
        'e',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "execute",
        nullptr,
        "define a command to execute, see manual for details about syntax;"
        " <keyword> ( '[' <keyword> | '\"' <string> '\"' ']' )*"                // a keyword with indexes
            " ( '.' field ( '[' <keyword> | '\"' <string> '\"' ']' )* )*"       // any number of fields with indexes
            " ( ( '?' | '+' )? '='"                                             // assignment operator (if not GET)
                " ( 'null' | (<keyword> | '\"' <string> '\"' )+ ) )?",          // value to assign or null (for REMOVE)
        advgetopt::getopt::argument_mode_t::required_argument
    },
    {
        'h',
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
        advgetopt::getopt::argument_mode_t::default_argument
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
        typedef std::vector<token>      vector_t;

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

    private:
        token_type_t    f_type = token_type_t::TOKEN_UNKNOWN;
        std::string     f_word = std::string();        // actual token (may be empty)
        int             f_start = -1;
        int             f_end = -1;
        int             f_line = -1;
        int             f_block_level = -1;
    };

    class keyword
    {
    public:
        typedef std::vector<keyword>    vector_t;

                        keyword(token const & t);

        void            set_command(token_type_t command);
        token_type_t    get_command() const;

        void            add_index(keyword const & k);
        void            add_field(keyword const & k);
        void            add_value(keyword const & k);

    private:
        token           f_token = token();          // keyword

        vector_t        f_index = vector_t();       // keyword[index1][index2][...]
        vector_t        f_fields = vector_t();      // keyword[index1][index2][...].field1[index1][...].field2[index1][...]...

        token_type_t    f_command = token_type_t::TOKEN_GET;        // = += ?=, by default GET

        vector_t        f_value = vector_t();       // keyword | string (if "= null" command becomes REMOVE)
    };

                        dns_options(int argc, char * argv[]);

    int                 run();

private:
    int                 load_file();
    int                 getc();
    void                ungetc(int c);
    token               get_token(bool extensions = false);

    int                 parse_command_line();
    int                 edit_option();

    advgetopt::getopt   f_opt; // initialized in constructor
    bool                f_debug = false;
    std::string         f_filename = std::string();
    std::string         f_execute = std::string();
    std::string         f_data = std::string();
    size_t              f_pos = 0;
    int                 f_line = 1;
    std::string         f_unget = std::string();
    token               f_token = token();
    int                 f_block_level = 0;
    keyword             f_keyword = keyword(token());
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





dns_options::keyword::keyword(token const & t)
    : f_token(t)
{
}


void dns_options::keyword::set_command(token_type_t command)
{
    f_command = command;
}


dns_options::token_type_t dns_options::keyword::get_command() const
{
    return f_command;
}


void dns_options::keyword::add_index(keyword const & k)
{
    f_index.push_back(k);
}


void dns_options::keyword::add_field(keyword const & k)
{
    f_fields.push_back(k);
}


void dns_options::keyword::add_value(keyword const & k)
{
    f_value.push_back(k);
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
    : f_opt(argc, argv, g_options, g_configuration_files, nullptr)
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
        f_opt.usage(advgetopt::getopt::status_t::no_error, "Usage: dns_options [--<opts>]");
        snap::NOTREACHED();
        return 1;
    }

    // check the --debug
    //
    f_debug = f_opt.is_defined("debug");

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

    // --execute "<code>"
    //
    if(!f_opt.is_defined("execute"))
    {
        std::cerr << f_opt.get_program_name() << ":error: mandatory --execute option missing." << std::endl;
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

    // then execute the command
    //
    r = edit_option();
    if(r != 0)
    {
        return r;
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
    snap::file_content file(f_filename);
    if(!file.read_all())
    {
        // could not open file for reading
        //
        std::cerr << "error: can't open file \"" << f_filename << "\" for reading.";
        return 1;
    }

    // ready the buffer
    //
    f_data = file.get_content();

    // if something bad happened, return 1, otherwise 0
    //
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
                        std::cerr << "error:" << f_filename << ":" << f_line << ": end of C-like comment not found before EOF." << std::endl;
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
                    std::cerr << "error:" << f_filename << ":" << f_line << ": quoted string includes a non-escaped newline." << std::endl;
                    result.set_type(token_type_t::TOKEN_ERROR);
                    result.set_end(f_pos - f_unget.length());
                    return result;
                }
                result += c;
            }
            if(c != '"')
            {
                std::cerr << "error:" << f_filename << ":" << f_line << ": quoted string was never closed." << std::endl;
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
                std::cerr << "error:" << f_filename << ":" << f_line << ": '}' mismatch, '{' missing for this one.";
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
            std::cerr << "error:<execute>:" << f_line << ": we first expected a keyword on your command line.";
        }
        return 1;
    }

    {
        keyword k(t);
        std::swap(f_keyword, k);
    }

    auto get_index = [&](keyword & p)
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
                        keyword i(t);
                        p.add_index(i);
                    }
                    break;

                default:
                    std::cerr << "error:<execute>:" << f_line << ": we expected a keyword or a quoted string as an index.";
                    return 1;

                }

                // after that (keyword | "string") we expect the ']'
                //
                t = get_token(true);
                if(t.get_type() != token_type_t::TOKEN_CLOSE_INDEX)
                {
                    std::cerr << "error:<execute>:" << f_line << ": we expected a ']' to close an index.";
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
            std::cerr << "error:<execute>:" << f_line << ": we expected a keyword or a quoted string as the field name.";
            return 1;
        }

        keyword f(t);
        f_keyword.add_field(f);

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
            std::cerr << "error:<execute>:" << f_line << ": nothing was expected after the ';'.";
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
        f_keyword.set_command(t.get_type());
        break;

    default:
        std::cerr << "error:<execute>:" << f_line << ": end of line or an assignment operator (=, ?=, +=) was expected.";
        return 1;

    }

    // we have an assignment, read the value
    //
    t = get_token(true);

    if(t.is_null())
    {
        if(f_keyword.get_command() != token_type_t::TOKEN_ASSIGN)
        {
            std::cerr << "error:<execute>:" << f_line << ": an assignment to null only works with the '=' operator.";
            return 1;
        }
        f_keyword.set_command(token_type_t::TOKEN_REMOVE);

        t = get_token(true);
        if(t.get_type() == token_type_t::TOKEN_END_OF_DEFINITION)
        {
            t = get_token(true);
        }
        if(t.get_type() != token_type_t::TOKEN_EOT)
        {
            std::cerr << "error:<execute>:" << f_line << ": an assignment to null cannot include anything else.";
            return 1;
        }

        // it worked, we have a REMOVE
        //
        return 0;
    }

    // read the value
    //
    for(;; t = get_token(true))
    {
        switch(t.get_type())
        {
        case token_type_t::TOKEN_EOT:
            return 0;

        case token_type_t::TOKEN_END_OF_DEFINITION:
            t = get_token(true);
            if(t.get_type() != token_type_t::TOKEN_EOT)
            {
                std::cerr << "error:<execute>:" << f_line << ": nothing was expected after the ';'.";
                return 1;
            }
            return 0;

        case token_type_t::TOKEN_KEYWORD:
        case token_type_t::TOKEN_STRING:
            {
                keyword const v(t);
                f_keyword.add_value(v);
            }
            break;

        default:
            std::cerr << "error:<execute>:" << f_line << ": the value can only be composed of keywords and quoted strings.";
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

    dns_options::token::vector_t in;
    for(;;)
    {
        token t(get_token());
        if(t.get_type() == token_type_t::TOKEN_EOT)
        {
            // done?
            //
            std::cerr << "--- TODO: found EOT of input...\n";
            break;
        }
        if(t.get_type() == token_type_t::TOKEN_ERROR)
        {
            // got an error while reading input file
            //
            std::cerr << "--- TODO: found TOKEN_ERROR reading input file...\n";
            return 1;
        }

        in.push_back(t);
    }

std::cerr << "try matching against command line...\n";

    return 0;
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
    catch(std::exception const & e)
    {
        std::cerr << "dns_options:error: an exception occurred: " << e.what() << std::endl;
        r = 1;
    }
    return r;
}


// vim: ts=4 sw=4 et
