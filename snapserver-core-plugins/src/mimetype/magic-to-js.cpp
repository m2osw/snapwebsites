// Snap Websites Server -- transform magic definitions to a .js file
// Copyright (C) 2014-2017  Made to Order Software Corp.
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

#include "magic-to-js.h"

#include <snapwebsites/not_reached.h>
#include <snapwebsites/not_used.h>
#include <snapwebsites/snapwebsites.h>

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <memory>
#include <vector>

#include <math.h>


/** \file
 * \brief Tool used to transform magic files in .js files.
 *
 * This tool is used to parse magic data files to use in JavaScript
 * to detect file formats on file Drag & Drop.
 *
 * The documentation of the format of the files is found in the magic
 * man page:
 *
 * \code
 *      man 5 magic
 * \endcode
 *
 * The following is an approximation of the lexer:
 *
 * \code
 * start: comment
 *      | empty_line
 *      | command
 *      | line
 *
 * comment: '#' end_of_line new_line
 *
 * empty_line: new_line
 *           | spaces new_line
 *
 * command: '!' ':' cmd
 *
 * cmd: mimetype
 *    | apple
 *    | strength
 * 
 * mimetype: 'mimetype' spaces end_of_line new_line
 *
 * apple: 'apple' spaces end_of_line new_line
 *
 * strength: 'strength' spaces binop spaces number new_line
 *
 * line: level offset spaces type spaces value opt_message new_line
 *
 * level: '>'
 *      | level '>'
 *
 * offset: number
 *       | opt_index '(' opt_index number opt_size opt_adjustment ')'
 *
 * type: identifier
 *     | identifier '&' number
 *     | identifier '/' flags
 *     | identifier '/' number     -- search/123
 *
 * -- valid types are: byte, short, long, quad, float, double, string, pstring,
 *                     date, qdate, ldate, qldate, beid3, beshort, belong,
 *                     bequad, befloat, bedouble, bedate, deqdate, beldate,
 *                     beqldate, bestring16, leid3, leshort, lelong, lequad,
 *                     lefloat, ledouble, ledate, leqdate, leldate, leqldate,
 *                     lestring16, melong, medate, meldate, indirect, name,
 *                     use, regex, search, default, and 'u'-<integer type>
 *
 * value: str_value
 *      | num_value
 *      | '!' str_value
 *      | '!' num_value
 *
 * str_value: opt_str_comparison [! \n\r]+
 *
 * opt_str_comparison: '='
 *                   | '<'
 *                   | '>'
 *
 * num_value: opt_num_comparison number
 *          | x
 *
 * opt_num_comparison: opt_str_comparison
 *                   | '&'
 *                   | '^'
 *                   | '~'
 *
 * opt_message: (* empty *)
 *            | spaces
 *            | spaces end_of_line
 *
 * new_line: '\n'
 *         | '\r'
 *         | '\r' '\n'
 *
 * opt_spaces: (* empty *)
 *           | spaces
 *
 * spaces: space
 *       | spaces space
 *
 * space: ' '
 *      | '\t'
 *
 * identifier: [a-zA-Z_][0-9a-zA-Z_]*
 *
 * flags: [a-zA-Z]+
 *
 * -- valid flags for string are: WwcCtb
 * -- valid flags for pstring are: BHhLlJ
 *
 * opt_size: (* empty *)
 *         | '.' [bilmsBILS]
 *
 * opt_index: (* empty *)
 *          | &
 *
 * opt_adjustment: '+' number
 *               | '-' number
 *               | '*' number
 *               | '/' number
 *               | '%' number
 *               | '&' number
 *               | '|' number
 *               | '^' number
 *               | '(' offset ')'
 *
 * binop: '+' opt_spaces number
 *      | '-' opt_spaces number
 *      | '*' opt_spaces number
 *      | '/' opt_spaces number
 *
 * number: decimal
 *       | octal
 *       | hexadecimal
 *       | floating_point
 *
 * decimal: [1-9][0-9]+
 *
 * floating_point: [1-9][0-9]* '.' [0-9]* ( [eE] [-+]? [0-9]+ )?
 *
 * octal: 0[0-7]*
 *
 * hexadecimal: 0[xX][0-9a-fA-F]+
 *
 * end_of_line: .*
 * \endcode
 */


namespace
{

bool g_debug = false;

} // no name namespace


/** \brief Lexer used to read the data from the input files.
 *
 * The lexer transforms the input files in token.
 */
class lexer
{
public:
    enum class mode_t
    {
        LEXER_MODE_NORMAL,                  // normal parsing
        LEXER_MODE_NORMAL_WITHOUT_FLOATS,   // normal parsing, but no floats
        LEXER_MODE_MESSAGE,                 // read whatever up to the end of line as a string (keep spaces, do not convert integers, etc.)
        LEXER_MODE_REGEX                    // reading a regular expression (read as a string)
    };

    typedef std::shared_ptr<lexer>      pointer_t;
    typedef std::vector<std::string>    filenames_t;

    class token_t
    {
    public:
        enum class type_t
        {
            TOKEN_TYPE_EOT,         // end of token
            TOKEN_TYPE_CHARACTER,   // '\n' for new line, ' ' for spaces (space or tab), other operators as themselves
            TOKEN_TYPE_STRING,      // string/identifier depending on where it appears
            TOKEN_TYPE_INTEGER,     // decimal, hexadecimal, and octal
            TOKEN_TYPE_FLOAT,       // floating point ('.' is the trigger)
            TOKEN_TYPE_COMMAND      // !:<command> a string with "command"
        };
        typedef char            character_t;
        typedef std::string     string_t;
        typedef int64_t         integer_t;
        typedef double          float_t;

                        token_t()
                            : f_type(type_t::TOKEN_TYPE_EOT)
                        {
                        }

                        token_t(character_t character)
                            : f_type(type_t::TOKEN_TYPE_CHARACTER)
                            , f_character(character)
                        {
                        }

                        token_t(string_t string, bool is_string = true)
                            : f_type(is_string ? type_t::TOKEN_TYPE_STRING : type_t::TOKEN_TYPE_COMMAND)
                            , f_string(string)
                        {
                        }

                        token_t(integer_t integer)
                            : f_type(type_t::TOKEN_TYPE_INTEGER)
                            , f_integer(integer)
                        {
                        }

                        token_t(float_t floating_point)
                            : f_type(type_t::TOKEN_TYPE_FLOAT)
                            , f_float(floating_point)
                        {
                        }

        type_t          get_type() const { return f_type; }

        character_t     get_character() const { return f_character; }
        string_t        get_string() const { return f_string; }
        integer_t       get_integer() const { return f_integer; }
        float_t         get_float() const { return f_float; }

    private:
        type_t                      f_type = type_t::TOKEN_TYPE_EOT;

        // TODO: redefine controlled vars with the typedef's of this class
        char                        f_character = 0;
        std::string                 f_string;
        int64_t                     f_integer = 0;
        double                      f_float = 0.0;
    };

                    lexer(filenames_t fn);

    std::string     list_of_filenames() const;
    token_t         get_token(mode_t mode);
    std::string     current_filename() const { return f_filenames.empty() ? "<no filenames>" : f_filenames[f_fpos - 1]; }
    int32_t         current_line() const { return f_line; }

private:
    int             getc();
    void            ungetc(int c);
    token_t         get_normal_token(mode_t mode);
    token_t         get_message_token();
    token_t         get_identifier_token(int c);
    token_t         get_string_token();
    token_t         get_number_token(mode_t mode, int c);

    filenames_t                     f_filenames;
    size_t                          f_fpos = 0;
    int32_t                         f_line = 1;
    bool                            f_start_of_line = true;
    std::shared_ptr<std::ifstream>  f_file; // current stream
    std::vector<char>               f_unget;
};


/** \brief Print out a token.
 *
 * This function prints out a token to the specified output stream.
 *
 * \param[in,out] out  The stream where the token is written.
 * \param[in] token  The token to write out.
 *
 * \return A reference to the output stream passed in.
 */
std::ostream& operator << (std::ostream& out, lexer::token_t const& token)
{
    switch(token.get_type())
    {
    case lexer::token_t::type_t::TOKEN_TYPE_EOT:
        out << "end of token";
        break;

    case lexer::token_t::type_t::TOKEN_TYPE_CHARACTER:
        {
            char c(token.get_character());
            if(c == 0)
            {
                out << "character '\\0'";
            }
            else if(c == '\a')
            {
                out << "character '\\a'";
            }
            else if(c == '\b')
            {
                out << "character '\\b'";
            }
            else if(c == '\f')
            {
                out << "character '\\f'";
            }
            else if(c == '\n')
            {
                out << "character '\\n'";
            }
            else if(c == '\r')
            {
                out << "character '\\r'";
            }
            else if(c == '\t')
            {
                out << "character '\\t'";
            }
            else if(c == '\v')
            {
                out << "character '\\v'";
            }
            else if(c < 0x20 || c >= 0x7F)
            {
                out << "character '\\" << std::oct << std::setw(3) << static_cast<int>(c) << std::dec
                    << "' (\\x" << std::hex << std::uppercase << static_cast<int>(c)
                    << std::dec << std::nouppercase << ")";
            }
            else
            {
                out << "character '" << c << "'";
            }
        }
        break;

    case lexer::token_t::type_t::TOKEN_TYPE_STRING:
        out << "string \"" << token.get_string() << "\"";
        break;

    case lexer::token_t::type_t::TOKEN_TYPE_INTEGER:
        out << "integer " << token.get_integer() << " (0x"
            << std::hex << std::uppercase << token.get_integer()
            << std::dec << std::nouppercase << ")";
        break;

    case lexer::token_t::type_t::TOKEN_TYPE_FLOAT:
        out << "float " << token.get_float();
        break;

    case lexer::token_t::type_t::TOKEN_TYPE_COMMAND:
        out << "command !:" << token.get_string();
        break;

    }

    return out;
}


/* \brief Initializes a lexer.
 *
 * Magic files are text files. Everything is line based. The lexer
 * detects the different elements and has intelligence to parse a
 * line into separate tokens.
 *
 * The input is any number of files. Once the end of a file is reached,
 * the next file is read. A file is always considered to end with a newline
 * even if none are found in the file.
 *
 * \param[in] fn  The list of files to read from.
 */
lexer::lexer(filenames_t fn)
    : f_filenames(fn)
    //, f_fpos(0) -- auto-init
    //, f_line(1) -- auto-init
    //, f_start_of_line(true)
    //, f_file(nullptr) -- auto-init
    //, f_unget() -- auto-init
{
    if(fn.size() > 0)
    {
        f_file.reset(new std::ifstream);
        f_file->open(f_filenames[0]);
        if(!f_file->is_open())
        {
            std::cerr << "error: could not open file \"" << f_filenames[0] << "\".\n";
            exit(1);
        }
        f_fpos = 1;
    }
}


/** \brief Generate the list of filenames for documentation purposes.
 *
 * This function generates a list of filenames that can be output in the
 * output documentation.
 *
 * \return List of filenames in a string.
 */
std::string lexer::list_of_filenames() const
{
    std::string result;
    for(size_t i(0); i < f_filenames.size(); ++i)
    {
        result += " * \\li " + f_filenames[i] + "\n";
    }
    return result;
}


/** \brief Read one token.
 *
 * This function reads one token from the magic file.
 */
lexer::token_t lexer::get_token(mode_t mode)
{
    lexer::token_t token;
    switch(mode)
    {
    case mode_t::LEXER_MODE_NORMAL:
    case mode_t::LEXER_MODE_NORMAL_WITHOUT_FLOATS:
        token = get_normal_token(mode);
        break;

    case mode_t::LEXER_MODE_MESSAGE:
        token = get_message_token();
        break;

    case mode_t::LEXER_MODE_REGEX:
        token = get_string_token();
        break;

    default:
        throw std::logic_error("lexer::get_token() called with an invalid mode");

    }

    if(g_debug)
    {
        std::cerr << token << std::endl;
    }

    return token;
}


/** \brief Get one character from the input file.
 *
 * If the end of the current input file is reached (eof() is returned) then
 * the function tries to open the next file. If that fails, then the function
 * returns eof().
 *
 * \return The next character, or std::istream::traits_type::eof().
 */
int lexer::getc()
{
    if(!f_unget.empty())
    {
        int const c(f_unget.back());
        f_unget.pop_back();
        return c;
    }
    for(;;)
    {
        int const c(f_file->get());
        if(c != std::istream::traits_type::eof())
        {
            // get a character, return it
//std::cerr << static_cast<char>(c);
            return c;
        }
        // more files to read?
        if(f_fpos >= f_filenames.size())
        {
            return std::istream::traits_type::eof();
        }
        f_file.reset(new std::ifstream);
        f_file->open(f_filenames[f_fpos]);
        if(!f_file->is_open())
        {
            // file cannot be read...
            std::cerr << "error: could not open file \"" << f_filenames[f_fpos] << "\".\n";
            exit(1);
        }
        ++f_fpos;
        f_line = 1;
    }
}


/** \brief Restore a character.
 *
 * Note that we support restoring any character, although it is supposed to
 * be the last character read. You may call ungetc() any number of times.
 * Note that this does not modify the file stream in any way.
 *
 * \param[in] c  The character to restore.
 */
void lexer::ungetc(int c)
{
    if(c != std::istream::traits_type::eof())
    {
        f_unget.push_back(c);
    }
}


/** \brief Retrieve a token, here the parser transform the input to a type.
 *
 * This function reads one token and returns it.
 *
 * If the end of all the input files is reached, then the type_t::TOKEN_TYPE_EOT
 * token is returned.
 *
 * \return The next token.
 */
lexer::token_t lexer::get_normal_token(mode_t mode)
{
    // at this time the only reason we loop is a line commented out
    // or an empty line; anything else either returns or generates
    // an error and exit the tool at once
    for(;;)
    {
        bool is_start(f_start_of_line);
        f_start_of_line = false;
        int c(getc());
        switch(c)
        {
        case '#':
            if(is_start)
            {
                // skip the comment, it's just like a message!
                get_message_token();
                getc(); // skip the '\n' right away
                ++f_line;
                f_start_of_line = true; // next call we're at the start of the line
                break;
            }
            return get_string_token();

        case ' ':
        case '\t':
            // skip all the spaces between tokens and return ONE space
            for(;;)
            {
                c = getc();
                if(c != ' ' && c != '\t')
                {
                    ungetc(c);
                    break;
                }
            }
            return token_t(static_cast<token_t::character_t>(' '));

        case '\r':
            // remove \r\n if such is found
            c = getc();
            if(c != '\n')
            {
                ungetc(c);
            }
        case '\n':
            ++f_line;
            f_start_of_line = true; // next call we're at the start of the line
            if(is_start)
            {
                // no need to return empty lines
                break;
            }
            return token_t(static_cast<token_t::character_t>('\n'));

        case '>':
        case '<':
        case '=':
        case '&':
        case '^':
        case '*':
        case '/':
        case '+':
        case '-':
        case '(':
        case ')':
        case '.':
            return token_t(static_cast<token_t::character_t>(c));

        case '!':
            // TBD: should we force this check at the start of a line?
            //      (if it works like this for us, we will be just fine.)
            c = getc();
            if(c == ':')
            {
                // read an identifier
                token_t id(get_string_token());
                // and transform to a command
                return token_t(id.get_string(), false);
            }
            ungetc(c);
            return token_t(static_cast<token_t::character_t>('!'));

        default:
            if(c >= '0' && c <= '9')
            {
                return get_number_token(mode, c);
            }
            if((c >= 'a' && c <= 'z')
            || (c >= 'A' && c <= 'Z')
            || c == '_')
            {
                return get_identifier_token(c);
            }
            if(c == std::istream::traits_type::eof())
            {
                return token_t();
            }
            std::cerr << "error:" << f_filenames[f_fpos - 1]
                      << ":" << f_line
                      << ": unsupported character " << c
                      << " (0x" << std::hex << std::uppercase << c
                      << ") from input file.\n";
            exit(1);
            snap::NOTREACHED();

        }
    }
}


/** \brief Retrieve the message.
 *
 * This function reads characters up to the following new line character.
 * If the end of the file is found first, then the process stops on that
 * even too.
 *
 * \return The message token (a string token).
 */
lexer::token_t lexer::get_message_token()
{
    // the message ends the line, no special parsing of messages
    std::string message;
    for(;;)
    {
        int c(getc());
        if(c == std::istream::traits_type::eof())
        {
            // return type_t::TOKEN_TYPE_EOT
            return token_t(message);
        }
        if(c == '\r')
        {
            c = getc();
            if(c != '\n')
            {
                ungetc(c);
            }
            c = '\n';
        }
        if(c == '\n')
        {
            // we need a new line at the end of the string so keep it here
            ungetc('\n');
            return token_t(message);
        }
        message += c;
    }
}


/** \brief We found a digit, so reading a number.
 *
 * This function reads a number, either an integer, or if a period (.)
 * is found, a floating point.
 *
 * Integers support decimal, octal, and hexadecimal.
 *
 * Floating points only support decimal with 'e' for the exponent.
 *
 * This function does not detect a sign at the start of the number.
 *
 * \param[in] mode  The mode used to read this token.
 * \param[in] c  The start digit.
 */
lexer::token_t lexer::get_number_token(mode_t mode, int c)
{
    token_t::integer_t   ri(0);
    token_t::float_t     rf(0.0);

    int d(getc());

    // hexadecimal?
    if(c == '0')
    {
        if(d == 'x' || d == 'X')
        {
            // in C, hexadecimal is simple, any character can follow
            for(;;)
            {
                d = getc();
                if(d >= '0' && d <= '9')
                {
                    ri = ri * 16 + (d - '0');
                }
                else if(d >= 'a' && d <= 'f')
                {
                    ri = ri * 16 + (d - 'a' + 10);
                }
                else if(d >= 'A' && d <= 'F')
                {
                    ri = ri * 16 + (d - 'A' + 10);
                }
                else
                {
                    ungetc(d);
                    return token_t(ri);
                }
            }
        }

        // if no 'x' or 'X' then it is octal
        for(;;)
        {
            if(d >= '0' && d <= '7')
            {
                ri = ri * 8 + (d - '0');
            }
            else if(d == '8' || d == '9')
            {
                std::cerr << "error: invalid octal number in \"" << f_filenames[f_fpos - 1] << "\".\n";
                exit(1);
            }
            else
            {
                ungetc(d);
                return token_t(ri);
            }
            d = getc();
        }
        snap::NOTREACHED();
    }

    // first read the number as if it were an integer
    ri = c - '0';
    for(;;)
    {
        if(d >= '0' && d <= '9')
        {
            ri = ri * 10 + (d - '0');
        }
        else
        {
            break;
        }
        d = getc();
    }

    // floating point number?
    // TBD: we may need to support detecting 'e' or 'E' as a floating point too?
    if(d == '.'
    && mode == lexer::mode_t::LEXER_MODE_NORMAL_WITHOUT_FLOATS)
    {
        // TBD: for floating points we may want to use the strtod() or
        //      similar function to make sure that we get the same result
        //      as what other users would get in other languages.
        //      (those functions may have heuristics to properly handle
        //      very large or very small numbers which we may not have
        //      properly captured here.)
        double dec = 1.0;
        for(;;)
        {
            d = getc();
            if(d >= '0' && d <= '9')
            {
                dec *= 10.0;
                rf = rf + (d - '0') / dec;
            }
            else
            {
                break;
            }
        }
        if(d == 'e' || d == 'E')
        {
            // exponent
            double sign(1.0);
            d = getc();
            if(d == '-')
            {
                sign = -1.0;
                d = getc();
            }
            else if(d == '+')
            {
                d = getc();
            }
            if(d >= '0' && d <= '9')
            {
                token_t::float_t exponent(0.0);
                for(;;)
                {
                    exponent = exponent * 1 + (d - '0');
                    d = getc();
                    if(d < '0' || d > '9')
                    {
                        ungetc(d);
                        rf *= pow(10, exponent * sign);
                        return token_t(rf);
                    }
                }
            }
            else
            {
                std::cerr << "error: invalid floating point exponent, digits expected after the 'e', in \"" << f_filenames[f_fpos - 1] << "\".\n";
                exit(1);
            }
        }
        ungetc(d);
        return token_t(rf);
    }

    ungetc(d);
    return token_t(ri);
}


/** \brief Read one identifier.
 *
 * This function reads one C-like identifier. Identifiers are parsed from
 * the 3rd token in a standard line.
 *
 * \param[in] c  The first character that was already read.
 *
 * \return A string token.
 */
lexer::token_t lexer::get_identifier_token(int c)
{
    std::string identifier;
    for(;;)
    {
        identifier += c; // note: c may be '\0' here!
        c = getc();
        if((c < '0' || c > '9')
        && (c < 'a' || c > 'z')
        && (c < 'A' || c > 'Z')
        && c != '_')
        {
            // done reading this identifier
            ungetc(c);
            return token_t(identifier);
        }
    }
}


/** \brief Read one string ending with a space.
 *
 * This function reads one string that ends with a space. This string can
 * generally include any character. Special characters are added with a
 * backslash.
 *
 * \param[in] c  The first character that was already read.
 *
 * \return A string token.
 */
lexer::token_t lexer::get_string_token()
{
    std::string str;
    for(;;)
    {
        int c(getc());
        if(c == '\\') // really allow any character in identifier including spaces!
        {
            c = getc();
            if(c == std::istream::traits_type::eof())
            {
                return token_t(str);
            }
            // transform the backslash character
            switch(c)
            {
            case '0':
                {
                    int d(getc());
                    if(d == 'x' || d == 'X')
                    {
                        // hexadecimal character, get one or 2 more digits
                        c = 0;
                        int max_chars(2);
                        for(; max_chars > 0; --max_chars)
                        {
                            d = getc();
                            if(d >= '0' && d <= '7')
                            {
                                c = c * 16 + (d - '0');
                            }
                            else if(d >= 'a' && d <= 'f')
                            {
                                c = c * 16 + (d - 'a' + 10);
                            }
                            else if(d >= 'A' && d <= 'F')
                            {
                                c = c * 16 + (d - 'A' + 10);
                            }
                            else
                            {
                                break;
                            }
                        }
                        if(max_chars == 2)
                        {
                            // invalid \x without an hex digit
                            std::cerr << "error: invalid use of \\x without a valid hexadecimal number following in \"" << f_filenames[f_fpos - 1] << "\".\n";
                            exit(1);
                        }
                        break;
                    }
                    ungetc(d);
                }
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
                c = c - '0';
                for(int max_chars(3); max_chars > 0; --max_chars)
                {
                    int d(getc());
                    if(d >= '0' && d <= '7')
                    {
                        c = c * 8 + (d - '0');
                    }
                    else
                    {
                        break;
                    }
                }
                break;

            case 'a':
                c = '\a';
                break;

            case 'b':
                c = '\b';
                break;

            case 'f':
                c = '\f';
                break;

            case 'n':
                c = '\n';
                break;

            case 'r':
                c = '\r';
                break;

            case 't':
                c = '\t';
                break;

            case 'v':
                c = '\v';
                break;

            //default: -- keep 'c' as is
            }
        }
        else if(c == ' ' || c == '\t'
             || c == '\r' || c == '\n'
             || c == std::istream::traits_type::eof())
        {
            // done reading this string
            ungetc(c);
            return token_t(str);
        }
        str += c; // note: c may be '\0' here!
    }
    snap::NOTREACHED();
}


/** \brief Parse magic files.
 *
 * This class is used to parse magic files.
 */
class parser
{
public:
    typedef std::shared_ptr<parser> pointer_t;

    class entry_t
    {
    public:
        typedef std::shared_ptr<entry_t>        pointer_t;

        enum class type_t
        {
            ENTRY_TYPE_UNKNOWN,

            // int -- 1 byte
            ENTRY_TYPE_BYTE,
            ENTRY_TYPE_UBYTE,
            // int -- 2 bytes
            ENTRY_TYPE_SHORT,
            ENTRY_TYPE_LESHORT,
            ENTRY_TYPE_BESHORT,
            ENTRY_TYPE_USHORT,
            ENTRY_TYPE_ULESHORT,
            ENTRY_TYPE_UBESHORT,
            // int -- 4 bytes
            ENTRY_TYPE_LONG,
            ENTRY_TYPE_LELONG,
            ENTRY_TYPE_BELONG,
            ENTRY_TYPE_MELONG,
            ENTRY_TYPE_ULONG,
            ENTRY_TYPE_ULELONG,
            ENTRY_TYPE_UBELONG,
            ENTRY_TYPE_UMELONG,
            // int -- 4 bytes -- an ID3 size is 32 bits defined as: ((size & 0x0FFFFFFF) * 4)
            ENTRY_TYPE_BEID3,
            ENTRY_TYPE_LEID3,
            ENTRY_TYPE_UBEID3,
            ENTRY_TYPE_ULEID3,
            // int -- 8 bytes
            ENTRY_TYPE_QUAD,
            ENTRY_TYPE_BEQUAD,
            ENTRY_TYPE_LEQUAD,
            ENTRY_TYPE_UQUAD,
            ENTRY_TYPE_UBEQUAD,
            ENTRY_TYPE_ULEQUAD,
            // float -- 4 bytes
            ENTRY_TYPE_FLOAT,
            ENTRY_TYPE_BEFLOAT,
            ENTRY_TYPE_LEFLOAT,
            // float -- 8 bytes
            ENTRY_TYPE_DOUBLE,
            ENTRY_TYPE_BEDOUBLE,
            ENTRY_TYPE_LEDOUBLE,
            // "text" (if value includes characters considered binary bytes then it is considered binary too)
            ENTRY_TYPE_STRING,
            ENTRY_TYPE_PSTRING,
            ENTRY_TYPE_BESTRING16,
            ENTRY_TYPE_LESTRING16,
            ENTRY_TYPE_SEARCH,
            ENTRY_TYPE_REGEX,
            // date
            ENTRY_TYPE_DATE,
            ENTRY_TYPE_QDATE,
            ENTRY_TYPE_LDATE,
            ENTRY_TYPE_QLDATE,
            ENTRY_TYPE_BEDATE,
            ENTRY_TYPE_BEQDATE,
            ENTRY_TYPE_BELDATE,
            ENTRY_TYPE_BEQLDATE,
            ENTRY_TYPE_LEDATE,
            ENTRY_TYPE_LEQDATE,
            ENTRY_TYPE_LELDATE,
            ENTRY_TYPE_LEQLDATE,
            ENTRY_TYPE_MEDATE,
            ENTRY_TYPE_MELDATE,
            // special
            ENTRY_TYPE_INDIRECT,
            ENTRY_TYPE_DEFAULT,
            ENTRY_TYPE_NAME,
            ENTRY_TYPE_USE
        };

        typedef lexer::token_t::integer_t       integer_t;
        typedef lexer::token_t::float_t         float_t;

        // string & search flags
        static integer_t const  ENTRY_FLAG_COMPACT_BLANK        = 0x00000001; // W
        static integer_t const  ENTRY_FLAG_BLANK                = 0x00000002; // w
        static integer_t const  ENTRY_FLAG_LOWER_INSENSITIVE    = 0x00000004; // c
        static integer_t const  ENTRY_FLAG_UPPER_INSENSITIVE    = 0x00000008; // C
        static integer_t const  ENTRY_FLAG_TEXT_FILE            = 0x00000010; // t
        static integer_t const  ENTRY_FLAG_BINARY_FILE          = 0x00000020; // b
        // pstring sizes
        static integer_t const  ENTRY_FLAG_BYTE                 = 0x00000040; // B
        static integer_t const  ENTRY_FLAG_BE_SHORT             = 0x00000080; // H
        static integer_t const  ENTRY_FLAG_LE_SHORT             = 0x00000100; // h
        static integer_t const  ENTRY_FLAG_BE_LONG              = 0x00000200; // L
        static integer_t const  ENTRY_FLAG_LE_LONG              = 0x00000400; // l
        static integer_t const  ENTRY_FLAG_SELF_INCLUDED        = 0x00000800; // J (size includes itself + string)
        // compare value
        static integer_t const  ENTRY_FLAG_NOT                  = 0x00001000; // !value
        static integer_t const  ENTRY_FLAG_EQUAL                = 0x00002000; // =value
        static integer_t const  ENTRY_FLAG_LESS                 = 0x00004000; // <value
        static integer_t const  ENTRY_FLAG_GREATER              = 0x00008000; // >value
        static integer_t const  ENTRY_FLAG_ARE_SET              = 0x00010000; // &value   integer only
        static integer_t const  ENTRY_FLAG_ARE_CLEAR            = 0x00020000; // ^value   integer only
        static integer_t const  ENTRY_FLAG_NEGATE               = 0x00040000; // ~value   integer only
        static integer_t const  ENTRY_FLAG_TRUE                 = 0x00080000; // x        numbers only
        // regex flags
        static integer_t const  ENTRY_FLAG_LINES                = 0x00100000; // l        regex only
        static integer_t const  ENTRY_FLAG_CASE_INSENSITIVE     = 0x00200000; // c        regex only
        static integer_t const  ENTRY_FLAG_START_OFFSET         = 0x00400000; // s        regex only
        // offset flags
        static integer_t const  ENTRY_FLAG_RELATIVE             = 0x04000000; // &        before the offset
        static integer_t const  ENTRY_FLAG_INDIRECT_RELATIVE    = 0x08000000; // (&...)   before the indirect offset

        // indirect sizes (TBD: what are the "i and I"? why have "b and B"?)
        static integer_t const  ENTRY_FLAG_INDIRECT_BYTE        = 0x01000000000; // b or B (B not used in existing files)
        static integer_t const  ENTRY_FLAG_INDIRECT_BE_SHORT    = 0x02000000000; // S
        static integer_t const  ENTRY_FLAG_INDIRECT_LE_SHORT    = 0x04000000000; // s
        static integer_t const  ENTRY_FLAG_INDIRECT_BE_LONG     = 0x08000000000; // L
        static integer_t const  ENTRY_FLAG_INDIRECT_LE_LONG     = 0x10000000000; // l
        static integer_t const  ENTRY_FLAG_INDIRECT_ME_LONG     = 0x20000000000; // m
        static integer_t const  ENTRY_FLAG_INDIRECT_BE_ID3      = 0x40000000000; // I
        static integer_t const  ENTRY_FLAG_INDIRECT_LE_ID3      = 0x80000000000; // i

        void                set_level(integer_t level) { f_level = level; }
        integer_t           get_level() const { return f_level; }

        void                set_offset(integer_t offset) { f_offset = offset; }
        integer_t           get_offset() const { return f_offset; }

        void                set_type(type_t type) { f_type = type; }
        type_t              get_type() const { return f_type; }

        void                set_mask(integer_t mask) { f_mask = mask; }
        integer_t           get_mask() const { return f_mask; }

        void                set_maxlength(integer_t maxlength) { f_maxlength = maxlength; }
        integer_t           get_maxlength() const { return f_maxlength; }

        void                set_flags(integer_t flags) { f_flags |= flags; }
        void                clear_flags(integer_t flags) { f_flags &= ~flags; }
        integer_t           get_flags() const { return f_flags; }
        std::string         flags_to_js_operator() const;

        void                set_mimetype(std::string mimetype) { f_mimetype = mimetype; }
        std::string         get_mimetype() const { return f_mimetype; }

        void                set_integer(integer_t integer) { f_integer = integer; }
        integer_t           get_integer() const { return f_integer; }

        void                set_float(float_t flt) { f_float = flt; }
        float_t             get_float() const { return f_float; }

        void                set_string(std::string string) { f_string = string; }
        std::string         get_string() const { return f_string; }

    private:
        integer_t           f_level = 0;        // number of > at the start (0+)
        integer_t           f_offset = 0;       // no support for indirections at this point (it's not that complicated, just time consuming to make sure it works right.)
        type_t              f_type = type_t::ENTRY_TYPE_UNKNOWN;         // see enum
        integer_t           f_mask = 0;         // defined with the type as in: "long&0xF0F0F0F0"
        integer_t           f_maxlength = 0;    // search/<maxlength>
        integer_t           f_flags = 0;        // [p]string/<flags>, and NOT (!)
        std::string         f_mimetype;         // a string found after the !:mimetype ...
        integer_t           f_integer = 0;      // compare with this integer
        float_t             f_float = 0.0;      // compare with this float
        std::string         f_string;           // compare with this string (may include '\0')
    };
    typedef std::vector<entry_t::pointer_t>    entry_vector_t;

    parser(lexer::pointer_t& l, std::string const& magic_name)
        : f_lexer(l)
        , f_magic_name(magic_name)
    {
    }

    void                    parse();
    void                    output();

private:
    void                    output_entry(size_t start, size_t end, bool has_mime);
    void                    output_header();
    void                    output_footer();

    lexer::pointer_t        f_lexer;

    entry_vector_t          f_entries;
    std::string             f_magic_name;
};



std::string parser::entry_t::flags_to_js_operator() const
{
    if((f_flags & ENTRY_FLAG_NOT) != 0)
    {
        return "!==";
    }
    else
    {
        return "===";
    }
        // TODO: support <, >, &, ^, ~...
        //static integer_t const  ENTRY_FLAG_NOT                  = 0x00001000; // !value
        //static integer_t const  ENTRY_FLAG_EQUAL                = 0x00002000; // =value
        //static integer_t const  ENTRY_FLAG_LESS                 = 0x00004000; // <value
        //static integer_t const  ENTRY_FLAG_GREATER              = 0x00008000; // >value
        //static integer_t const  ENTRY_FLAG_ARE_SET              = 0x00010000; // &value   integer only
        //static integer_t const  ENTRY_FLAG_ARE_CLEAR            = 0x00020000; // ^value   integer only
        //static integer_t const  ENTRY_FLAG_NEGATE               = 0x00040000; // ~value   integer only
        //static integer_t const  ENTRY_FLAG_TRUE                 = 0x00080000; // x        numbers only
}


/** \brief Parse the magic files data.
 *
 * This function reads magic files and parse them for any number of
 * magic definitions.
 *
 * \todo
 * According to the magic documentation, all magic tests that apply
 * to text files need to be run after all the binary magic tests.
 * So at some point we would need to add a sorting capability which
 * ensures that such happens as expected.
 */
void parser::parse()
{
    entry_t::pointer_t e;
    for(;;)
    {
        lexer::token_t token(f_lexer->get_token(lexer::mode_t::LEXER_MODE_NORMAL));
        switch(token.get_type())
        {
        case lexer::token_t::type_t::TOKEN_TYPE_COMMAND:
            if(!e)
            {
                std::cerr << "error: a command without any line is not legal.\n";
                exit(1);
            }
            if(token.get_string() == "mime")
            {
                // these we accept!
                token = f_lexer->get_token(lexer::mode_t::LEXER_MODE_MESSAGE);
                std::string mimetype(token.get_string());
                while(mimetype[0] == ' ' || mimetype[0] == '\t')
                {
                    mimetype.erase(mimetype.begin());
                }
                e->set_mimetype(mimetype);
            }
            else if(token.get_string() == "apple" || token.get_string() == "strength")
            {
                // ignore those for now
                f_lexer->get_token(lexer::mode_t::LEXER_MODE_MESSAGE);
            }
            else
            {
                std::cerr << "error: unknown command (!:" << token.get_string() << ").\n";
                exit(1);
            }
            token = f_lexer->get_token(lexer::mode_t::LEXER_MODE_NORMAL);
            if(token.get_type() != lexer::token_t::type_t::TOKEN_TYPE_CHARACTER
            || token.get_character() != '\n')
            {
                std::cerr << "error: a command line is expected to end with a new line.\n";
                exit(1);
            }
            continue;

        case lexer::token_t::type_t::TOKEN_TYPE_EOT:
            // we are done parsing
            return;

        case lexer::token_t::type_t::TOKEN_TYPE_CHARACTER:
            // a line may start with characters (>)
            if(token.get_character() != '>')
            {
                std::cerr << "error: expected '>' to indicate the level of this line. Got " << token.get_character() << " instead.\n";
                exit(1);
            }
            e.reset(new entry_t);
            {
                int level(0);
                do
                {
                    ++level;
                    token = f_lexer->get_token(lexer::mode_t::LEXER_MODE_NORMAL);
                }
                while(token.get_type() == lexer::token_t::type_t::TOKEN_TYPE_CHARACTER
                   && token.get_character() == '>');
                e->set_level(level);
            }

            if(token.get_type() == lexer::token_t::type_t::TOKEN_TYPE_CHARACTER
            && token.get_character() == '&')
            {
                e->set_flags(entry_t::ENTRY_FLAG_RELATIVE);
                token = f_lexer->get_token(lexer::mode_t::LEXER_MODE_NORMAL);
            }

            {
                int offset_sign(1);
                if(token.get_type() == lexer::token_t::type_t::TOKEN_TYPE_CHARACTER
                && token.get_character() == '-')
                {
                    offset_sign = -1;
                    token = f_lexer->get_token(lexer::mode_t::LEXER_MODE_NORMAL);
                }

                if(token.get_type() == lexer::token_t::type_t::TOKEN_TYPE_INTEGER)
                {
                    // the actual offset
                    e->set_offset(token.get_integer() * offset_sign);
                    break;
                }

                if(offset_sign == -1)
                {
                    std::cerr << "error:" << f_lexer->current_filename()
                              << ":" << f_lexer->current_line()
                              << ": expected an integer after a '-' in the offset.\n";
                    exit(1);
                }
            }

            // indirect
            if(token.get_type() != lexer::token_t::type_t::TOKEN_TYPE_CHARACTER
            || token.get_character() != '(')
            {
                std::cerr << "error:" << f_lexer->current_filename()
                          << ":" << f_lexer->current_line()
                          << ": expected an integer, '&', or '(' after the level indication.\n";
                exit(1);
            }
            token = f_lexer->get_token(lexer::mode_t::LEXER_MODE_NORMAL);
            if(token.get_type() == lexer::token_t::type_t::TOKEN_TYPE_CHARACTER
            && token.get_character() == '&')
            {
                e->set_flags(entry_t::ENTRY_FLAG_INDIRECT_RELATIVE);
                token = f_lexer->get_token(lexer::mode_t::LEXER_MODE_NORMAL);
            }

            // indirect offset
            if(token.get_type() != lexer::token_t::type_t::TOKEN_TYPE_INTEGER)
            {
                std::cerr << "error: expected an integer for the indirect offset.\n";
                exit(1);
            }
            e->set_offset(token.get_integer());

            token = f_lexer->get_token(lexer::mode_t::LEXER_MODE_NORMAL);
            if(token.get_type() == lexer::token_t::type_t::TOKEN_TYPE_CHARACTER
            && token.get_character() == '.')
            {
                // NOTE: The documentation says that the size is
                //       optional, and if not defined, long is used
                //       (but they do not specify the endian, so I would
                //       imagine that the machine endian is to be used?!)
                token = f_lexer->get_token(lexer::mode_t::LEXER_MODE_NORMAL);
                if(token.get_type() != lexer::token_t::type_t::TOKEN_TYPE_STRING)
                {
                    std::cerr << "error: indirect offsets can be followed by a size (.b, .l, etc.), here the size is missing.\n";
                    exit(1);
                }
                std::string size(token.get_string());
                if(size.size() != 1)
                {
                    std::cerr << "error: indirect offsets size (.b, .l, etc.), must be exactly one chracter.\n";
                    exit(1);
                }
                switch(size[0])
                {
                case 'b':
                case 'B':
                    e->set_flags(entry_t::ENTRY_FLAG_INDIRECT_BYTE);
                    break;

                case 'S':
                    e->set_flags(entry_t::ENTRY_FLAG_INDIRECT_BE_SHORT);
                    break;

                case 's':
                    e->set_flags(entry_t::ENTRY_FLAG_INDIRECT_LE_SHORT);
                    break;

                case 'l':
                    e->set_flags(entry_t::ENTRY_FLAG_INDIRECT_BE_LONG);
                    break;

                case 'L':
                    e->set_flags(entry_t::ENTRY_FLAG_INDIRECT_LE_LONG);
                    break;

                case 'm':
                    e->set_flags(entry_t::ENTRY_FLAG_INDIRECT_ME_LONG);
                    break;

                case 'I':
                    e->set_flags(entry_t::ENTRY_FLAG_INDIRECT_BE_ID3);
                    break;

                case 'i':
                    e->set_flags(entry_t::ENTRY_FLAG_INDIRECT_LE_ID3);
                    break;

                default:
                    std::cerr << "error: invalid character used as an offset size (" << size[0] << ").\n";
                    exit(1);

                }
                token = f_lexer->get_token(lexer::mode_t::LEXER_MODE_NORMAL);
            }
            if(token.get_type() == lexer::token_t::type_t::TOKEN_TYPE_CHARACTER
            && token.get_character() != ')')
            {
                switch(token.get_character())
                {
                case '+':
                case '-':
                case '*':
                case '/':
                case '%':
                case '&':
                case '|':
                case '^':
                    //e->set_indirect_adjustment_operator(token.get_character());
                    break;

                default:
                    std::cerr << "error: indirect adjustment operator ("
                              << token.get_character() << ") not supported."
                              << std::endl;
                    exit(1);

                }
                token = f_lexer->get_token(lexer::mode_t::LEXER_MODE_NORMAL);
                if(token.get_type() == lexer::token_t::type_t::TOKEN_TYPE_CHARACTER
                && token.get_character() == '(')
                {
                    // case were we have a negative number and they
                    // generally use (<position>.<size>+(-<offset>))
                    //
                    int sign(1);
                    token = f_lexer->get_token(lexer::mode_t::LEXER_MODE_NORMAL);
                    if(token.get_type() == lexer::token_t::type_t::TOKEN_TYPE_CHARACTER
                    && token.get_character() == '-')
                    {
                        sign = -1;
                        token = f_lexer->get_token(lexer::mode_t::LEXER_MODE_NORMAL);
                    }
                    if(token.get_type() != lexer::token_t::type_t::TOKEN_TYPE_INTEGER)
                    {
                        std::cerr << "error:" << f_lexer->current_filename()
                                  << ":" << f_lexer->current_line()
                                  << ": indirect adjustment operator must be followed by an integer."
                                  << std::endl;
                        exit(1);
                    }
                    // Note: the + and - can be optimized by replacing the
                    //       integer with -integer and the '-' by '+'
                    //e->set_indirect_adjustment(token.get_integer() * sign);
                    token = f_lexer->get_token(lexer::mode_t::LEXER_MODE_NORMAL);
                    if(token.get_type() != lexer::token_t::type_t::TOKEN_TYPE_CHARACTER
                    && token.get_character() != ')')
                    {
                        std::cerr << "error:" << f_lexer->current_filename()
                                  << ":" << f_lexer->current_line()
                                  << ": indirect adjustment operator sub-offset must be ended by a ')'."
                                  << std::endl;
                        exit(1);
                    }
                    token = f_lexer->get_token(lexer::mode_t::LEXER_MODE_NORMAL);
                }
                else
                {
                    if(token.get_type() != lexer::token_t::type_t::TOKEN_TYPE_INTEGER)
                    {
                        // Note: in the documentation they say you can also have
                        //       another parenthesis layer as in: +(-4)
                        std::cerr << "error:" << f_lexer->current_filename()
                                  << ":" << f_lexer->current_line()
                                  << ": indirect adjustment operator must be followed by an integer."
                                  << std::endl;
                        exit(1);
                    }
                    // Note: the + and - can be optimized by replacing the
                    //       integer with -integer and the '-' by '+'
                    //e->set_indirect_adjustment(token.get_integer());
                    token = f_lexer->get_token(lexer::mode_t::LEXER_MODE_NORMAL);
                }
            }
            if(token.get_type() != lexer::token_t::type_t::TOKEN_TYPE_CHARACTER
            || token.get_character() != ')')
            {
                std::cerr << "error: an indirect offset must end with ')'.\n";
                exit(1);
            }
            break;

        case lexer::token_t::type_t::TOKEN_TYPE_INTEGER:
            // the offset for this line
            e.reset(new entry_t);
            e->set_offset(token.get_integer());
            break;

        default:
            std::cerr << "error: expected a standard line token: an integer optionally preceeded by '>' characters.\n";
            exit(1);

        }

        // after the offset we have to have a space then the type
        token = f_lexer->get_token(lexer::mode_t::LEXER_MODE_NORMAL);
        if(token.get_type() != lexer::token_t::type_t::TOKEN_TYPE_CHARACTER
        || token.get_character() != ' ')
        {
            std::cerr << "error: expected a space or tab after the offset.\n";
            exit(1);
        }

        token = f_lexer->get_token(lexer::mode_t::LEXER_MODE_NORMAL);
        if(token.get_type() != lexer::token_t::type_t::TOKEN_TYPE_STRING)
        {
            std::cerr << "error: expected a string to indicate the type on this line.\n";
            exit(1);
        }

        std::string type(token.get_string());
        if(type == "byte")
        {
            e->set_type(entry_t::type_t::ENTRY_TYPE_BYTE);
        }
        else if(type == "ubyte")
        {
            e->set_type(entry_t::type_t::ENTRY_TYPE_UBYTE);
        }
        else if(type == "short")
        {
            e->set_type(entry_t::type_t::ENTRY_TYPE_SHORT);
        }
        else if(type == "leshort")
        {
            e->set_type(entry_t::type_t::ENTRY_TYPE_LESHORT);
        }
        else if(type == "beshort")
        {
            e->set_type(entry_t::type_t::ENTRY_TYPE_BESHORT);
        }
        else if(type == "ushort")
        {
            e->set_type(entry_t::type_t::ENTRY_TYPE_USHORT);
        }
        else if(type == "uleshort")
        {
            e->set_type(entry_t::type_t::ENTRY_TYPE_ULESHORT);
        }
        else if(type == "ubeshort")
        {
            e->set_type(entry_t::type_t::ENTRY_TYPE_UBESHORT);
        }
        else if(type == "long")
        {
            e->set_type(entry_t::type_t::ENTRY_TYPE_LONG);
        }
        else if(type == "lelong")
        {
            e->set_type(entry_t::type_t::ENTRY_TYPE_LELONG);
        }
        else if(type == "belong")
        {
            e->set_type(entry_t::type_t::ENTRY_TYPE_BELONG);
        }
        else if(type == "melong")
        {
            e->set_type(entry_t::type_t::ENTRY_TYPE_MELONG);
        }
        else if(type == "ulong")
        {
            e->set_type(entry_t::type_t::ENTRY_TYPE_ULONG);
        }
        else if(type == "ulong")
        {
            e->set_type(entry_t::type_t::ENTRY_TYPE_ULONG);
        }
        else if(type == "ulelong")
        {
            e->set_type(entry_t::type_t::ENTRY_TYPE_ULELONG);
        }
        else if(type == "ubelong")
        {
            e->set_type(entry_t::type_t::ENTRY_TYPE_UBELONG);
        }
        else if(type == "umelong")
        {
            e->set_type(entry_t::type_t::ENTRY_TYPE_UMELONG);
        }
        else if(type == "beid3")
        {
            e->set_type(entry_t::type_t::ENTRY_TYPE_BEID3);
        }
        else if(type == "leid3")
        {
            e->set_type(entry_t::type_t::ENTRY_TYPE_LEID3);
        }
        else if(type == "ubeid3")
        {
            e->set_type(entry_t::type_t::ENTRY_TYPE_UBEID3);
        }
        else if(type == "uleid3")
        {
            e->set_type(entry_t::type_t::ENTRY_TYPE_ULEID3);
        }
        else if(type == "quad")
        {
            e->set_type(entry_t::type_t::ENTRY_TYPE_QUAD);
        }
        else if(type == "bequad")
        {
            e->set_type(entry_t::type_t::ENTRY_TYPE_BEQUAD);
        }
        else if(type == "lequad")
        {
            e->set_type(entry_t::type_t::ENTRY_TYPE_LEQUAD);
        }
        else if(type == "uquad")
        {
            e->set_type(entry_t::type_t::ENTRY_TYPE_UQUAD);
        }
        else if(type == "ubequad")
        {
            e->set_type(entry_t::type_t::ENTRY_TYPE_UBEQUAD);
        }
        else if(type == "ulequad")
        {
            e->set_type(entry_t::type_t::ENTRY_TYPE_ULEQUAD);
        }
        else if(type == "float")
        {
            e->set_type(entry_t::type_t::ENTRY_TYPE_FLOAT);
        }
        else if(type == "befloat")
        {
            e->set_type(entry_t::type_t::ENTRY_TYPE_BEFLOAT);
        }
        else if(type == "lefloat")
        {
            e->set_type(entry_t::type_t::ENTRY_TYPE_LEFLOAT);
        }
        else if(type == "double")
        {
            e->set_type(entry_t::type_t::ENTRY_TYPE_DOUBLE);
        }
        else if(type == "bedouble")
        {
            e->set_type(entry_t::type_t::ENTRY_TYPE_BEDOUBLE);
        }
        else if(type == "ledouble")
        {
            e->set_type(entry_t::type_t::ENTRY_TYPE_LEDOUBLE);
        }
        else if(type == "string")
        {
            e->set_type(entry_t::type_t::ENTRY_TYPE_STRING);
        }
        else if(type == "pstring")
        {
            e->set_type(entry_t::type_t::ENTRY_TYPE_PSTRING);
        }
        else if(type == "bestring16")
        {
            e->set_type(entry_t::type_t::ENTRY_TYPE_BESTRING16);
        }
        else if(type == "lestring16")
        {
            e->set_type(entry_t::type_t::ENTRY_TYPE_LESTRING16);
        }
        else if(type == "search")
        {
            e->set_type(entry_t::type_t::ENTRY_TYPE_SEARCH);
        }
        else if(type == "regex")
        {
            e->set_type(entry_t::type_t::ENTRY_TYPE_REGEX);
        }
        else if(type == "date")
        {
            e->set_type(entry_t::type_t::ENTRY_TYPE_DATE);
        }
        else if(type == "qdate")
        {
            e->set_type(entry_t::type_t::ENTRY_TYPE_QDATE);
        }
        else if(type == "ldate")
        {
            e->set_type(entry_t::type_t::ENTRY_TYPE_LDATE);
        }
        else if(type == "qldate")
        {
            e->set_type(entry_t::type_t::ENTRY_TYPE_QLDATE);
        }
        else if(type == "bedate")
        {
            e->set_type(entry_t::type_t::ENTRY_TYPE_BEDATE);
        }
        else if(type == "beqdate")
        {
            e->set_type(entry_t::type_t::ENTRY_TYPE_BEQDATE);
        }
        else if(type == "beldate")
        {
            e->set_type(entry_t::type_t::ENTRY_TYPE_BELDATE);
        }
        else if(type == "beqldate")
        {
            e->set_type(entry_t::type_t::ENTRY_TYPE_BEQLDATE);
        }
        else if(type == "ledate")
        {
            e->set_type(entry_t::type_t::ENTRY_TYPE_LEDATE);
        }
        else if(type == "leqdate")
        {
            e->set_type(entry_t::type_t::ENTRY_TYPE_LEQDATE);
        }
        else if(type == "leldate")
        {
            e->set_type(entry_t::type_t::ENTRY_TYPE_LELDATE);
        }
        else if(type == "leqldate")
        {
            e->set_type(entry_t::type_t::ENTRY_TYPE_LEQLDATE);
        }
        else if(type == "medate")
        {
            e->set_type(entry_t::type_t::ENTRY_TYPE_MEDATE);
        }
        else if(type == "meldate")
        {
            e->set_type(entry_t::type_t::ENTRY_TYPE_MELDATE);
        }
        else if(type == "indirect")
        {
            e->set_type(entry_t::type_t::ENTRY_TYPE_INDIRECT);
        }
        else if(type == "default")
        {
            e->set_type(entry_t::type_t::ENTRY_TYPE_DEFAULT);
        }
        else if(type == "name")
        {
            e->set_type(entry_t::type_t::ENTRY_TYPE_NAME);
        }
        else if(type == "use")
        {
            e->set_type(entry_t::type_t::ENTRY_TYPE_USE);
        }
        else
        {
            std::cerr << "error:" << f_lexer->current_filename()
                      << ":" << f_lexer->current_line()
                      << ": unknown type \"" << type << "\".\n";
            exit(1);
        }

        token = f_lexer->get_token(lexer::mode_t::LEXER_MODE_NORMAL);
        if(token.get_type() == lexer::token_t::type_t::TOKEN_TYPE_CHARACTER)
        {
            switch(token.get_character())
            {
            case '&': // <integer-type> & <integer>
                switch(e->get_type())
                {
                case entry_t::type_t::ENTRY_TYPE_BYTE:
                case entry_t::type_t::ENTRY_TYPE_UBYTE:
                case entry_t::type_t::ENTRY_TYPE_SHORT:
                case entry_t::type_t::ENTRY_TYPE_LESHORT:
                case entry_t::type_t::ENTRY_TYPE_BESHORT:
                case entry_t::type_t::ENTRY_TYPE_USHORT:
                case entry_t::type_t::ENTRY_TYPE_ULESHORT:
                case entry_t::type_t::ENTRY_TYPE_UBESHORT:
                case entry_t::type_t::ENTRY_TYPE_LONG:
                case entry_t::type_t::ENTRY_TYPE_LELONG:
                case entry_t::type_t::ENTRY_TYPE_BELONG:
                case entry_t::type_t::ENTRY_TYPE_MELONG:
                case entry_t::type_t::ENTRY_TYPE_ULONG:
                case entry_t::type_t::ENTRY_TYPE_ULELONG:
                case entry_t::type_t::ENTRY_TYPE_UBELONG:
                case entry_t::type_t::ENTRY_TYPE_UMELONG:
                case entry_t::type_t::ENTRY_TYPE_BEID3:
                case entry_t::type_t::ENTRY_TYPE_LEID3:
                case entry_t::type_t::ENTRY_TYPE_UBEID3:
                case entry_t::type_t::ENTRY_TYPE_ULEID3:
                case entry_t::type_t::ENTRY_TYPE_QUAD:
                case entry_t::type_t::ENTRY_TYPE_BEQUAD:
                case entry_t::type_t::ENTRY_TYPE_LEQUAD:
                case entry_t::type_t::ENTRY_TYPE_UQUAD:
                case entry_t::type_t::ENTRY_TYPE_UBEQUAD:
                case entry_t::type_t::ENTRY_TYPE_ULEQUAD:
                case entry_t::type_t::ENTRY_TYPE_DATE:
                case entry_t::type_t::ENTRY_TYPE_QDATE:
                case entry_t::type_t::ENTRY_TYPE_LDATE:
                case entry_t::type_t::ENTRY_TYPE_QLDATE:
                case entry_t::type_t::ENTRY_TYPE_BEDATE:
                case entry_t::type_t::ENTRY_TYPE_BEQDATE:
                case entry_t::type_t::ENTRY_TYPE_BELDATE:
                case entry_t::type_t::ENTRY_TYPE_BEQLDATE:
                case entry_t::type_t::ENTRY_TYPE_LEDATE:
                case entry_t::type_t::ENTRY_TYPE_LEQDATE:
                case entry_t::type_t::ENTRY_TYPE_LELDATE:
                case entry_t::type_t::ENTRY_TYPE_LEQLDATE:
                case entry_t::type_t::ENTRY_TYPE_MEDATE:
                case entry_t::type_t::ENTRY_TYPE_MELDATE:
                    break;

                default:
                    std::cerr << "error: a type followed by & must be an integral type.\n";
                    exit(1);
                    snap::NOTREACHED();

                }
                token = f_lexer->get_token(lexer::mode_t::LEXER_MODE_NORMAL);
                if(token.get_type() != lexer::token_t::type_t::TOKEN_TYPE_INTEGER)
                {
                    std::cerr << "error: a type followed by & must next be followed by an integer.\n";
                    exit(1);
                }
                e->set_mask(token.get_integer());
                token = f_lexer->get_token(lexer::mode_t::LEXER_MODE_NORMAL);
                break;

            case '/': // <string-type> '/' <flags>, or "search" '/' <number>
                switch(e->get_type())
                {
                case entry_t::type_t::ENTRY_TYPE_STRING:
                case entry_t::type_t::ENTRY_TYPE_BESTRING16:
                case entry_t::type_t::ENTRY_TYPE_LESTRING16:
                    token = f_lexer->get_token(lexer::mode_t::LEXER_MODE_NORMAL);
                    if(token.get_type() != lexer::token_t::type_t::TOKEN_TYPE_STRING)
                    {
                        std::cerr << "error: a search followed by / must next be followed by a set of flags.\n";
                        exit(1);
                    }
                    {
                        std::string const flags(token.get_string());
                        for(char const *f(flags.c_str()); *f != '\0'; ++f)
                        {
                            switch(*f)
                            {
                            case 'W':
                                e->set_flags(entry_t::ENTRY_FLAG_COMPACT_BLANK);
                                break;

                            case 'w':
                                e->set_flags(entry_t::ENTRY_FLAG_BLANK);
                                break;

                            case 'c':
                                e->set_flags(entry_t::ENTRY_FLAG_LOWER_INSENSITIVE);
                                break;

                            case 'C':
                                e->set_flags(entry_t::ENTRY_FLAG_UPPER_INSENSITIVE);
                                break;

                            case 't':
                                e->set_flags(entry_t::ENTRY_FLAG_TEXT_FILE);
                                break;

                            case 'b':
                                e->set_flags(entry_t::ENTRY_FLAG_BINARY_FILE);
                                break;

                            default:
                                std::cerr << "error:" << f_lexer->current_filename()
                                          << ":" << f_lexer->current_line()
                                          << ": invalid character used as a string, bestring16, or lestring16 ("
                                          << *f << ").\n";
                                exit(1);

                            }
                        }
                    }
                    token = f_lexer->get_token(lexer::mode_t::LEXER_MODE_NORMAL);
                    break;

                case entry_t::type_t::ENTRY_TYPE_PSTRING:
                    // only width of the string size is expected here
                    token = f_lexer->get_token(lexer::mode_t::LEXER_MODE_NORMAL);
                    if(token.get_type() != lexer::token_t::type_t::TOKEN_TYPE_STRING)
                    {
                        std::cerr << "error: a search followed by / must next be followed by a set of flags.\n";
                        exit(1);
                    }
                    {
                        std::string const flags(token.get_string());
                        for(char const *f(flags.c_str()); *f != '\0'; ++f)
                        {
                            switch(*f)
                            {
                            case 'B':
                                e->set_flags(entry_t::ENTRY_FLAG_BYTE);
                                break;

                            case 'H':
                                e->set_flags(entry_t::ENTRY_FLAG_BE_SHORT);
                                break;

                            case 'h':
                                e->set_flags(entry_t::ENTRY_FLAG_LE_SHORT);
                                break;

                            case 'L':
                                e->set_flags(entry_t::ENTRY_FLAG_BE_LONG);
                                break;

                            case 'l':
                                e->set_flags(entry_t::ENTRY_FLAG_LE_LONG);
                                break;

                            case 'J':
                                e->set_flags(entry_t::ENTRY_FLAG_SELF_INCLUDED);
                                break;

                            default:
                                std::cerr << "error: invalid character used as a pstring flag (pstring/" << *f << ").\n";
                                exit(1);

                            }
                        }
                    }
                    token = f_lexer->get_token(lexer::mode_t::LEXER_MODE_NORMAL);
                    break;

                case entry_t::type_t::ENTRY_TYPE_REGEX: // <regex> / <flags>  or  <regex> / <number>
                    token = f_lexer->get_token(lexer::mode_t::LEXER_MODE_NORMAL);
                    // TBD:
                    // I would imagine that both could be used (integer + flags)
                    // but it is not documented so at this point I read one or
                    // the other and that is enough with the existing files.
                    if(token.get_type() == lexer::token_t::type_t::TOKEN_TYPE_INTEGER)
                    {
                        // the number of lines to check the regex against
                        e->set_maxlength(token.get_integer());
                        token = f_lexer->get_token(lexer::mode_t::LEXER_MODE_NORMAL);
                        if(token.get_type() != lexer::token_t::type_t::TOKEN_TYPE_CHARACTER
                        || token.get_character() != '/')
                        {
                            // no extra flags
                            break;
                        }
                    }
                    if(token.get_type() == lexer::token_t::type_t::TOKEN_TYPE_STRING)
                    {
                        // regex flags are 'l', 's' and 'c'
                        std::string flags(token.get_string());
                        for(char const *f(flags.c_str()); *f != '\0'; ++f)
                        {
                            switch(*f)
                            {
                            case 'l':
                                e->set_flags(entry_t::ENTRY_FLAG_LINES);
                                break;

                            case 'c':
                                e->set_flags(entry_t::ENTRY_FLAG_CASE_INSENSITIVE);
                                break;

                            case 's':
                                e->set_flags(entry_t::ENTRY_FLAG_START_OFFSET);
                                break;

                            default:
                                std::cerr << "error: invalid character used as a regex flag (regex/" << *f << ").\n";
                                exit(1);

                            }
                        }
                    }
                    else
                    {
                        std::cerr << "error: a search followed by / must next be followed by an integer and/or flags.\n";
                        exit(1);
                    }
                    token = f_lexer->get_token(lexer::mode_t::LEXER_MODE_NORMAL);
                    break;

                case entry_t::type_t::ENTRY_TYPE_SEARCH:
                    token = f_lexer->get_token(lexer::mode_t::LEXER_MODE_NORMAL);
                    if(token.get_type() == lexer::token_t::type_t::TOKEN_TYPE_INTEGER)
                    {
                        e->set_maxlength(token.get_integer());
                        token = f_lexer->get_token(lexer::mode_t::LEXER_MODE_NORMAL);
                        if(token.get_type() != lexer::token_t::type_t::TOKEN_TYPE_CHARACTER
                        || token.get_character() != '/')
                        {
                            // no extra flags
                            break;
                        }
                    }
                    if(token.get_type() == lexer::token_t::type_t::TOKEN_TYPE_STRING)
                    {
                        std::string flags(token.get_string());
                        for(char const *f(flags.c_str()); *f != '\0'; ++f)
                        {
                            switch(*f)
                            {
                            case 'W':
                                e->set_flags(entry_t::ENTRY_FLAG_COMPACT_BLANK);
                                break;

                            case 'w':
                                e->set_flags(entry_t::ENTRY_FLAG_BLANK);
                                break;

                            case 'c':
                                e->set_flags(entry_t::ENTRY_FLAG_LOWER_INSENSITIVE);
                                break;

                            case 'C':
                                e->set_flags(entry_t::ENTRY_FLAG_UPPER_INSENSITIVE);
                                break;

                            case 't':
                                e->set_flags(entry_t::ENTRY_FLAG_TEXT_FILE);
                                break;

                            case 'b':
                                e->set_flags(entry_t::ENTRY_FLAG_BINARY_FILE);
                                break;

                            default:
                                std::cerr << "error: invalid character used as a search flag (" << *f << ").\n";
                                exit(1);

                            }
                        }
                    }
                    else
                    {
                        std::cerr << "error: a search followed by / must next be followed by an integer (count) or a string (flags).\n";
                        exit(1);
                    }
                    token = f_lexer->get_token(lexer::mode_t::LEXER_MODE_NORMAL);
                    break;

                default:
                    std::cerr << "error: a type followed by / must be a string type.\n";
                    exit(1);
                    snap::NOTREACHED();

                }
                break;

            }
        }

        if(token.get_type() != lexer::token_t::type_t::TOKEN_TYPE_CHARACTER
        || token.get_character() != ' ')
        {
            std::cerr << "error: expected a space or tab after the type.\n";
            exit(1);
        }

        // the next get_token() mode depends on the type so we do that
        // separately
        bool is_float(false);
        switch(e->get_type())
        {
        case entry_t::type_t::ENTRY_TYPE_FLOAT:
        case entry_t::type_t::ENTRY_TYPE_BEFLOAT:
        case entry_t::type_t::ENTRY_TYPE_LEFLOAT:
        case entry_t::type_t::ENTRY_TYPE_DOUBLE:
        case entry_t::type_t::ENTRY_TYPE_BEDOUBLE:
        case entry_t::type_t::ENTRY_TYPE_LEDOUBLE:
            is_float = true;
            /*FLOWTHROUGH*/
        case entry_t::type_t::ENTRY_TYPE_BYTE:
        case entry_t::type_t::ENTRY_TYPE_UBYTE:
        case entry_t::type_t::ENTRY_TYPE_SHORT:
        case entry_t::type_t::ENTRY_TYPE_LESHORT:
        case entry_t::type_t::ENTRY_TYPE_BESHORT:
        case entry_t::type_t::ENTRY_TYPE_USHORT:
        case entry_t::type_t::ENTRY_TYPE_ULESHORT:
        case entry_t::type_t::ENTRY_TYPE_UBESHORT:
        case entry_t::type_t::ENTRY_TYPE_LONG:
        case entry_t::type_t::ENTRY_TYPE_LELONG:
        case entry_t::type_t::ENTRY_TYPE_BELONG:
        case entry_t::type_t::ENTRY_TYPE_MELONG:
        case entry_t::type_t::ENTRY_TYPE_ULONG:
        case entry_t::type_t::ENTRY_TYPE_ULELONG:
        case entry_t::type_t::ENTRY_TYPE_UBELONG:
        case entry_t::type_t::ENTRY_TYPE_UMELONG:
        case entry_t::type_t::ENTRY_TYPE_BEID3:
        case entry_t::type_t::ENTRY_TYPE_LEID3:
        case entry_t::type_t::ENTRY_TYPE_UBEID3:
        case entry_t::type_t::ENTRY_TYPE_ULEID3:
        case entry_t::type_t::ENTRY_TYPE_QUAD:
        case entry_t::type_t::ENTRY_TYPE_BEQUAD:
        case entry_t::type_t::ENTRY_TYPE_LEQUAD:
        case entry_t::type_t::ENTRY_TYPE_UQUAD:
        case entry_t::type_t::ENTRY_TYPE_UBEQUAD:
        case entry_t::type_t::ENTRY_TYPE_ULEQUAD:
        case entry_t::type_t::ENTRY_TYPE_DATE:
        case entry_t::type_t::ENTRY_TYPE_QDATE:
        case entry_t::type_t::ENTRY_TYPE_LDATE:
        case entry_t::type_t::ENTRY_TYPE_QLDATE:
        case entry_t::type_t::ENTRY_TYPE_BEDATE:
        case entry_t::type_t::ENTRY_TYPE_BEQDATE:
        case entry_t::type_t::ENTRY_TYPE_BELDATE:
        case entry_t::type_t::ENTRY_TYPE_BEQLDATE:
        case entry_t::type_t::ENTRY_TYPE_LEDATE:
        case entry_t::type_t::ENTRY_TYPE_LEQDATE:
        case entry_t::type_t::ENTRY_TYPE_LELDATE:
        case entry_t::type_t::ENTRY_TYPE_LEQLDATE:
        case entry_t::type_t::ENTRY_TYPE_MEDATE:
        case entry_t::type_t::ENTRY_TYPE_MELDATE:
            // integers expect a number of flags so we manage these here
            token = f_lexer->get_token(lexer::mode_t::LEXER_MODE_NORMAL);
            // first check whether we have a '!' (must be the very first)
            if(token.get_type() == lexer::token_t::type_t::TOKEN_TYPE_CHARACTER
            && token.get_character() == '!')
            {
                e->set_flags(entry_t::ENTRY_FLAG_NOT);
                token = f_lexer->get_token(lexer::mode_t::LEXER_MODE_NORMAL);
            }
            {
                bool has_operator(token.get_type() == lexer::token_t::type_t::TOKEN_TYPE_CHARACTER);
                if(has_operator
                && token.get_character() != '-')
                {
                    // verify that it is legal with a floating point value if such
                    if(is_float)
                    {
                        switch(token.get_character())
                        {
                        case '&':
                        case '^':
                        case '~':
                            std::cerr << "error:" << f_lexer->current_filename()
                                    << ":" << f_lexer->current_line()
                                    << ": " << static_cast<char>(token.get_character())
                                    << " used with a floating point number.\n";
                            exit(1);
                            snap::NOTREACHED();

                        }
                    }
                    switch(token.get_character())
                    {
                    case '=':
                        e->set_flags(entry_t::ENTRY_FLAG_EQUAL);
                        break;

                    case '<':
                        e->set_flags(entry_t::ENTRY_FLAG_LESS);
                        break;

                    case '>':
                        e->set_flags(entry_t::ENTRY_FLAG_GREATER);
                        break;

                    case '&':
                        e->set_flags(entry_t::ENTRY_FLAG_ARE_SET);
                        break;

                    case '^':
                        e->set_flags(entry_t::ENTRY_FLAG_ARE_CLEAR);
                        break;

                    case '~':
                        e->set_flags(entry_t::ENTRY_FLAG_NEGATE);
                        break;

                    default:
                        std::cerr << "error:"
                                  << f_lexer->current_filename() << ":"
                                  << f_lexer->current_line() << ": unknown comparison operator "
                                  << token.get_character() << ".\n";
                        exit(1);
                        snap::NOTREACHED();

                    }
                    token = f_lexer->get_token(lexer::mode_t::LEXER_MODE_NORMAL);

                    // we allow spaces after an operator
                    if(token.get_type() == lexer::token_t::type_t::TOKEN_TYPE_CHARACTER
                    && token.get_character() == ' ')
                    {
                        token = f_lexer->get_token(lexer::mode_t::LEXER_MODE_NORMAL);
                    }
                }
                // one special case here: "x"
                if(token.get_type() == lexer::token_t::type_t::TOKEN_TYPE_STRING
                && token.get_string() == "x"
                && !has_operator)
                {
                    e->set_flags(entry_t::ENTRY_FLAG_TRUE);
                }
                else
                {
                    int sign(1);
                    if(token.get_type() == lexer::token_t::type_t::TOKEN_TYPE_CHARACTER
                    && token.get_character() == '-')
                    {
                        sign = -1;
                        token = f_lexer->get_token(lexer::mode_t::LEXER_MODE_NORMAL);
                    }
                    if(token.get_type() == lexer::token_t::type_t::TOKEN_TYPE_FLOAT)
                    {
                        if(!is_float)
                        {
                            std::cerr << "error:" << f_lexer->current_filename()
                                    << ":" << f_lexer->current_line()
                                    << ": an integer was expected for an entry specifying a number type.\n";
                            exit(1);
                        }

                        e->set_float(token.get_float() * static_cast<double>(sign));
                    }
                    else if(token.get_type() == lexer::token_t::type_t::TOKEN_TYPE_INTEGER)
                    {
                        if(is_float)
                        {
                            std::cerr << "error:" << f_lexer->current_filename()
                                    << ":" << f_lexer->current_line()
                                    << ": a floating point number was expected for an entry specifying a floating point type, got an integer.\n";
                            exit(1);
                        }

                        e->set_integer(token.get_integer() * sign);
                    }
                    else
                    {
                        std::cerr << "error:" << f_lexer->current_filename()
                                << ":" << f_lexer->current_line()
                                << ": an \"x\", an integer, or a floating point number were expected (instead token type is: "
                                << static_cast<int>(token.get_type())
                                << ").\n";
                        exit(1);
                    }
                }
            }
            break;

        case entry_t::type_t::ENTRY_TYPE_STRING:
        case entry_t::type_t::ENTRY_TYPE_PSTRING:
        case entry_t::type_t::ENTRY_TYPE_BESTRING16:
        case entry_t::type_t::ENTRY_TYPE_LESTRING16:
        case entry_t::type_t::ENTRY_TYPE_SEARCH:
            // strings can start with !, !=, !<, !>, =, <, >
            // however, we better read the string as a whole
            {
                token = f_lexer->get_token(lexer::mode_t::LEXER_MODE_REGEX);
                std::string str(token.get_string());
                if(str[0] == '!')
                {
                    str.erase(str.begin());
                    e->set_flags(entry_t::ENTRY_FLAG_NOT);
                }
                switch(str[0])
                {
                case '=':
                    str.erase(str.begin());
                    e->set_flags(entry_t::ENTRY_FLAG_EQUAL);
                    break;

                case '<':
                    str.erase(str.begin());
                    e->set_flags(entry_t::ENTRY_FLAG_LESS);
                    break;

                case '>':
                    str.erase(str.begin());
                    e->set_flags(entry_t::ENTRY_FLAG_GREATER);
                    break;

                }
                e->set_string(str);
            }
            break;

        case entry_t::type_t::ENTRY_TYPE_REGEX:
            token = f_lexer->get_token(lexer::mode_t::LEXER_MODE_REGEX);
            e->set_string(token.get_string());
            break;

        case entry_t::type_t::ENTRY_TYPE_NAME: // this creates a macro
            token = f_lexer->get_token(lexer::mode_t::LEXER_MODE_NORMAL);
            e->set_string(token.get_string());
            break;

        case entry_t::type_t::ENTRY_TYPE_USE: // this calls a macro
            token = f_lexer->get_token(lexer::mode_t::LEXER_MODE_NORMAL);
            e->set_string(token.get_string());
            break;

        case entry_t::type_t::ENTRY_TYPE_INDIRECT:
            // the indirect may or may not be followed by the 'x' before
            // the message... since we ignore the message we can also
            // ignore the x here
            break;

        case entry_t::type_t::ENTRY_TYPE_DEFAULT:
            token = f_lexer->get_token(lexer::mode_t::LEXER_MODE_NORMAL);
            if(token.get_type() != lexer::token_t::type_t::TOKEN_TYPE_STRING
            || token.get_string() != "x")
            {
                std::cerr << "error: default must always be used with \"x\".\n";
                exit(1);
            }
            e->set_flags(entry_t::ENTRY_FLAG_TRUE);
            break;

        case entry_t::type_t::ENTRY_TYPE_UNKNOWN:
            std::cerr << "error: entry type still unknown when defining its value.\n";
            exit(1);
            snap::NOTREACHED();

        }
        token = f_lexer->get_token(lexer::mode_t::LEXER_MODE_MESSAGE);
        if(token.get_type() == lexer::token_t::type_t::TOKEN_TYPE_STRING)
        {
            // We don't do anything with the message, but just in case I
            // show here that we'd have to skip the spaces before saving it
            //std::string msg(token.get_string());
            //while(msg[0] == ' ' || msg[0] == '\t')
            //{
            //    msg.erase(msg.begin());
            //}
            //e->set_message(msg);

            // we can switch back to normal to read the \n
            token = f_lexer->get_token(lexer::mode_t::LEXER_MODE_NORMAL);
        }
        if(token.get_type() != lexer::token_t::type_t::TOKEN_TYPE_CHARACTER
        || token.get_character() != '\n')
        {
            std::cerr << "error: expected an optional message and a new line at the end of the line.\n";
            exit(1);
        }

        f_entries.push_back(e);
    }
}


void parser::output()
{
    // the output is sent to stdout so that way we can save the data to
    // any file using a redirection or see it on the screen
    size_t const max_entries(f_entries.size());
    if(max_entries == 0)
    {
        std::cerr << "error: read some magic files, but did not get an valid entries...\n";
        exit(1);
    }

    if(f_entries[0]->get_level() != 0)
    {
        std::cerr << "error: the very first entry must always be a level zero entry.\n";
        exit(1);
    }

    output_header();

    bool has_mime(false);
    std::string name;
    size_t start(0);
    for(size_t i(0); i < max_entries; ++i)
    {
        if(f_entries[i]->get_level() == 0)
        {
            // if we get an entry with a mime type, then send it out
            if(has_mime)
            {
                output_entry(start, i, true);
                has_mime = false;
            }
            else if(!name.empty())
            {
                std::cout << "__macro_" << name << " = function(offset) {" << std::endl;
                output_entry(start, i, false);
                std::cout << "return false;};" << std::endl;
                name.clear();
            }
            start = i;
        }
        if(!f_entries[i]->get_mimetype().empty())
        {
            // this means it is worth encoding
            has_mime = true;
        }
        if(f_entries[i]->get_type() == entry_t::type_t::ENTRY_TYPE_NAME)
        {
            // found a macro
            name = f_entries[i]->get_string();
        }
    }
    if(has_mime)
    {
        output_entry(start, max_entries, true);
    }

    output_footer();
}


void parser::output_entry(size_t start, size_t end, bool has_mime)
{
    struct recursive_output
    {
        recursive_output(bool has_mime)
            : f_has_mime(has_mime)
        {
        }

        size_t output(size_t pos)
        {
            output_if(pos);
            size_t next_pos(pos + 1);
            if(next_pos < f_entries.size()
            && f_entries[pos]->get_level() <= f_entries[next_pos]->get_level())
            {
                // returns our new next_pos
                next_pos = output(next_pos);  // recursive call
            }
            else if(!f_has_mime)
            {
                std::cout << "return true;" << std::endl;
            }
            output_mimetype(pos);
            output_endif(pos);

            return next_pos;
        }

        void output_if(size_t pos)
        {
            typedef void (recursive_output::*output_func_t)(size_t pos);
            static output_func_t const output_by_type[] =
            {
                [static_cast<int>(entry_t::type_t::ENTRY_TYPE_UNKNOWN)] = &recursive_output::output_unknown,

                // int -- 1 byte
                [static_cast<int>(entry_t::type_t::ENTRY_TYPE_BYTE)] = &recursive_output::output_byte,
                [static_cast<int>(entry_t::type_t::ENTRY_TYPE_UBYTE)] = &recursive_output::output_ubyte,
                // int -- 2 bytes
                [static_cast<int>(entry_t::type_t::ENTRY_TYPE_SHORT)] = &recursive_output::output_short,
                [static_cast<int>(entry_t::type_t::ENTRY_TYPE_LESHORT)] = &recursive_output::output_leshort,
                [static_cast<int>(entry_t::type_t::ENTRY_TYPE_BESHORT)] = &recursive_output::output_beshort,
                [static_cast<int>(entry_t::type_t::ENTRY_TYPE_USHORT)] = &recursive_output::output_ushort,
                [static_cast<int>(entry_t::type_t::ENTRY_TYPE_ULESHORT)] = &recursive_output::output_uleshort,
                [static_cast<int>(entry_t::type_t::ENTRY_TYPE_UBESHORT)] = &recursive_output::output_ubeshort,
                // int -- 4 bytes
                [static_cast<int>(entry_t::type_t::ENTRY_TYPE_LONG)] = &recursive_output::output_long,
                [static_cast<int>(entry_t::type_t::ENTRY_TYPE_LELONG)] = &recursive_output::output_lelong,
                [static_cast<int>(entry_t::type_t::ENTRY_TYPE_BELONG)] = &recursive_output::output_belong,
                [static_cast<int>(entry_t::type_t::ENTRY_TYPE_MELONG)] = &recursive_output::output_melong,
                [static_cast<int>(entry_t::type_t::ENTRY_TYPE_ULONG)] = &recursive_output::output_ulong,
                [static_cast<int>(entry_t::type_t::ENTRY_TYPE_ULELONG)] = &recursive_output::output_ulelong,
                [static_cast<int>(entry_t::type_t::ENTRY_TYPE_UBELONG)] = &recursive_output::output_ubelong,
                [static_cast<int>(entry_t::type_t::ENTRY_TYPE_UMELONG)] = &recursive_output::output_umelong,
                // int -- 4 bytes -- an ID3 size is 32 bits defined as: ((size & 0x0FFFFFFF) * 4)
                [static_cast<int>(entry_t::type_t::ENTRY_TYPE_BEID3)] = &recursive_output::output_beid3,
                [static_cast<int>(entry_t::type_t::ENTRY_TYPE_LEID3)] = &recursive_output::output_leid3,
                [static_cast<int>(entry_t::type_t::ENTRY_TYPE_UBEID3)] = &recursive_output::output_ubeid3,
                [static_cast<int>(entry_t::type_t::ENTRY_TYPE_ULEID3)] = &recursive_output::output_uleid3,
                // int -- 8 bytes
                [static_cast<int>(entry_t::type_t::ENTRY_TYPE_QUAD)] = &recursive_output::output_quad,
                [static_cast<int>(entry_t::type_t::ENTRY_TYPE_BEQUAD)] = &recursive_output::output_bequad,
                [static_cast<int>(entry_t::type_t::ENTRY_TYPE_LEQUAD)] = &recursive_output::output_lequad,
                [static_cast<int>(entry_t::type_t::ENTRY_TYPE_UQUAD)] = &recursive_output::output_uquad,
                [static_cast<int>(entry_t::type_t::ENTRY_TYPE_UBEQUAD)] = &recursive_output::output_ubequad,
                [static_cast<int>(entry_t::type_t::ENTRY_TYPE_ULEQUAD)] = &recursive_output::output_ulequad,
                // float -- 4 bytes
                [static_cast<int>(entry_t::type_t::ENTRY_TYPE_FLOAT)] = &recursive_output::output_float,
                [static_cast<int>(entry_t::type_t::ENTRY_TYPE_BEFLOAT)] = &recursive_output::output_befloat,
                [static_cast<int>(entry_t::type_t::ENTRY_TYPE_LEFLOAT)] = &recursive_output::output_lefloat,
                // float -- 8 bytes
                [static_cast<int>(entry_t::type_t::ENTRY_TYPE_DOUBLE)] = &recursive_output::output_double,
                [static_cast<int>(entry_t::type_t::ENTRY_TYPE_BEDOUBLE)] = &recursive_output::output_bedouble,
                [static_cast<int>(entry_t::type_t::ENTRY_TYPE_LEDOUBLE)] = &recursive_output::output_ledouble,
                // "text" (if value includes characters considered binary bytes then it is considered binary too)
                [static_cast<int>(entry_t::type_t::ENTRY_TYPE_STRING)] = &recursive_output::output_string,
                [static_cast<int>(entry_t::type_t::ENTRY_TYPE_PSTRING)] = &recursive_output::output_pstring,
                [static_cast<int>(entry_t::type_t::ENTRY_TYPE_BESTRING16)] = &recursive_output::output_besearch16,
                [static_cast<int>(entry_t::type_t::ENTRY_TYPE_LESTRING16)] = &recursive_output::output_lesearch16,
                [static_cast<int>(entry_t::type_t::ENTRY_TYPE_SEARCH)] = &recursive_output::output_search,
                [static_cast<int>(entry_t::type_t::ENTRY_TYPE_REGEX)] = &recursive_output::output_regex,
                // date
                [static_cast<int>(entry_t::type_t::ENTRY_TYPE_DATE)] = &recursive_output::output_date,
                [static_cast<int>(entry_t::type_t::ENTRY_TYPE_QDATE)] = &recursive_output::output_qdate,
                [static_cast<int>(entry_t::type_t::ENTRY_TYPE_LDATE)] = &recursive_output::output_ldate,
                [static_cast<int>(entry_t::type_t::ENTRY_TYPE_QLDATE)] = &recursive_output::output_qldate,
                [static_cast<int>(entry_t::type_t::ENTRY_TYPE_BEDATE)] = &recursive_output::output_bedate,
                [static_cast<int>(entry_t::type_t::ENTRY_TYPE_BEQDATE)] = &recursive_output::output_beqdate,
                [static_cast<int>(entry_t::type_t::ENTRY_TYPE_BELDATE)] = &recursive_output::output_beldate,
                [static_cast<int>(entry_t::type_t::ENTRY_TYPE_BEQLDATE)] = &recursive_output::output_beqldate,
                [static_cast<int>(entry_t::type_t::ENTRY_TYPE_LEDATE)] = &recursive_output::output_ledate,
                [static_cast<int>(entry_t::type_t::ENTRY_TYPE_LEQDATE)] = &recursive_output::output_leqdate,
                [static_cast<int>(entry_t::type_t::ENTRY_TYPE_LELDATE)] = &recursive_output::output_leldate,
                [static_cast<int>(entry_t::type_t::ENTRY_TYPE_LEQLDATE)] = &recursive_output::output_leqldate,
                [static_cast<int>(entry_t::type_t::ENTRY_TYPE_MEDATE)] = &recursive_output::output_medate,
                [static_cast<int>(entry_t::type_t::ENTRY_TYPE_MELDATE)] = &recursive_output::output_meldate,
                // special
                [static_cast<int>(entry_t::type_t::ENTRY_TYPE_INDIRECT)] = &recursive_output::output_indirect,
                [static_cast<int>(entry_t::type_t::ENTRY_TYPE_DEFAULT)] = &recursive_output::output_default,
                [static_cast<int>(entry_t::type_t::ENTRY_TYPE_NAME)] = &recursive_output::output_name,
                [static_cast<int>(entry_t::type_t::ENTRY_TYPE_USE)] = &recursive_output::output_use
            };

            std::cout << "if(";
            (this->*output_by_type[static_cast<int>(f_entries[pos]->get_type())])(pos);
            std::cout << ")\n{\n";
        }

        void output_unknown(size_t pos)
        {
            snap::NOTUSED(pos);
            std::cerr << "error: found an unknown entry while outputing data.\n";
            exit(1);
        }

        void output_byte(size_t pos)
        {
            int64_t const be(f_entries[pos]->get_integer());
            std::cout << "buf[" << (f_has_mime ? "" : "offset+") << f_entries[pos]->get_offset() << "]"
                      << " "
                      << f_entries[pos]->flags_to_js_operator()
                      << " 0x"
                      << std::hex << std::uppercase
                      << (be & 0xff)
                      << std::dec << std::nouppercase;
        }

        void output_ubyte(size_t pos)
        {
            int64_t const be(f_entries[pos]->get_integer());
            std::cout << "buf[" << (f_has_mime ? "" : "offset+") << f_entries[pos]->get_offset() << "]"
                      << " "
                      << f_entries[pos]->flags_to_js_operator()
                      << " 0x"
                      << std::hex << std::uppercase
                      << (be & 0xff)
                      << std::dec << std::nouppercase;
        }

        void output_short(size_t pos)
        {
            snap::NOTUSED(pos);
            std::cerr << "error: type not implemented yet (short).\n";
            exit(1);
        }

        void output_leshort(size_t pos)
        {
            int64_t const le(f_entries[pos]->get_integer());
            int64_t const offset(f_entries[pos]->get_offset());
            std::cout << "buf[" << (f_has_mime ? "" : "offset+") << offset
                      << "] + buf[" << (f_has_mime ? "" : "offset+") << (offset + 1)
                      << "] * 256 "
                      << f_entries[pos]->flags_to_js_operator()
                      << " 0x"
                      << std::hex << std::uppercase
                      << (le & 0xffff)
                      << std::dec << std::nouppercase;
        }

        void output_beshort(size_t pos)
        {
            int64_t const be(f_entries[pos]->get_integer());
            int64_t const offset(f_entries[pos]->get_offset());
            std::cout << "buf[" << (f_has_mime ? "" : "offset+") << offset
                      << "] * 256 + buf[" << (f_has_mime ? "" : "offset+") << (offset + 1)
                      << " "
                      << f_entries[pos]->flags_to_js_operator()
                      << " 0x"
                      << std::hex << std::uppercase
                      << (be & 0xffff)
                      << std::dec << std::nouppercase;
        }

        void output_ushort(size_t pos)
        {
            snap::NOTUSED(pos);
            std::cerr << "error: type not implemented yet (ushort).\n";
            exit(1);
        }

        void output_uleshort(size_t pos)
        {
            int64_t const ule(f_entries[pos]->get_integer());
            int64_t const offset(f_entries[pos]->get_offset());
            std::cout << "buf[" << (f_has_mime ? "" : "offset+") << offset
                      << "] + buf[" << (f_has_mime ? "" : "offset+") << (offset + 1)
                      << " * 256 "
                      << f_entries[pos]->flags_to_js_operator()
                      << " 0x"
                      << std::hex << std::uppercase
                      << (ule & 0xffff)
                      << std::dec << std::nouppercase;
        }

        void output_ubeshort(size_t pos)
        {
            int64_t const ube(f_entries[pos]->get_integer());
            int64_t const offset(f_entries[pos]->get_offset());
            std::cout << "buf[" << (f_has_mime ? "" : "offset+") << offset
                      << "] * 256 + buf[" << (f_has_mime ? "" : "offset+") << (offset + 1)
                      << " "
                      << f_entries[pos]->flags_to_js_operator()
                      << " 0x"
                      << std::hex << std::uppercase
                      << (ube & 0xffff)
                      << std::dec << std::nouppercase;
        }

        void output_long(size_t pos)
        {
            // this is a machine byte order, I am not currently sure
            // on how we could really get that in JavaScript; for
            // now do a little endian since most users have x86 based
            // processors which are in little endian
            //
            int64_t const le(f_entries[pos]->get_integer());
            int64_t const offset(f_entries[pos]->get_offset());
            std::cout << "buf[" << (f_has_mime ? "" : "offset+") << offset
                      << "] + buf[" << (f_has_mime ? "" : "offset+") << (offset + 1)
                      << "] * 256 + buf[" << (f_has_mime ? "" : "offset+") << (offset + 2)
                      << "] * 65536 + buf[" << (f_has_mime ? "" : "offset+") << (offset + 3)
                      << "] * 16777216 "
                      << f_entries[pos]->flags_to_js_operator()
                      << " 0x"
                      << std::hex << std::uppercase
                      << (le & 0xffffffffLL)
                      << std::dec << std::nouppercase;
        }

        void output_lelong(size_t pos)
        {
            int64_t const le(f_entries[pos]->get_integer());
            int64_t const offset(f_entries[pos]->get_offset());
            std::cout << "buf[" << (f_has_mime ? "" : "offset+") << offset
                      << "] + buf[" << (f_has_mime ? "" : "offset+") << (offset + 1)
                      << "] * 256 + buf[" << (f_has_mime ? "" : "offset+") << (offset + 2)
                      << "] * 65536 + buf[" << (f_has_mime ? "" : "offset+") << (offset + 3)
                      << "] * 16777216 "
                      << f_entries[pos]->flags_to_js_operator()
                      << " 0x"
                      << std::hex << std::uppercase
                      << (le & 0xffffffffLL)
                      << std::dec << std::nouppercase;
        }

        void output_belong(size_t pos)
        {
            int64_t const be(f_entries[pos]->get_integer());
            int64_t const offset(f_entries[pos]->get_offset());
            std::cout << "buf[" << (f_has_mime ? "" : "offset+") << offset
                      << "] * 16777216 + buf[" << (f_has_mime ? "" : "offset+") << (offset + 1)
                      << "] * 65536 + buf[" << (f_has_mime ? "" : "offset+") << (offset + 2)
                      << "] * 256 + buf[" << (f_has_mime ? "" : "offset+") << (offset + 3)
                      << "] "
                      << f_entries[pos]->flags_to_js_operator()
                      << " 0x"
                      << std::hex << std::uppercase
                      << (be & 0xffffffffLL)
                      << std::dec << std::nouppercase;
        }

        void output_melong(size_t pos)
        {
            snap::NOTUSED(pos);
            std::cerr << "error: type not implemented yet (melong).\n";
            exit(1);
        }

        void output_ulong(size_t pos)
        {
            snap::NOTUSED(pos);
            std::cerr << "error: type not implemented yet (ulong).\n";
            exit(1);
        }

        void output_ulelong(size_t pos)
        {
            snap::NOTUSED(pos);
            std::cerr << "error: type not implemented yet (ulelong).\n";
            exit(1);
        }

        void output_ubelong(size_t pos)
        {
            int64_t const ube(f_entries[pos]->get_integer());
            int64_t const offset(f_entries[pos]->get_offset());
            std::cout << "buf[" << (f_has_mime ? "" : "offset+") << offset
                      << "] * 16777216 + buf[" << (f_has_mime ? "" : "offset+") << (offset + 1)
                      << "] * 65536 + buf[" << (f_has_mime ? "" : "offset+") << (offset + 2)
                      << "] * 256 + buf[" << (f_has_mime ? "" : "offset+") << (offset + 3)
                      << "] "
                      << f_entries[pos]->flags_to_js_operator()
                      << " 0x"
                      << std::hex << std::uppercase
                      << (ube & 0xffffffffLL)
                      << std::dec << std::nouppercase;
        }

        void output_umelong(size_t pos)
        {
            snap::NOTUSED(pos);
            std::cerr << "error: type not implemented yet (umelong).\n";
            exit(1);
        }

        void output_beid3(size_t pos)
        {
            snap::NOTUSED(pos);
            std::cerr << "error: type not implemented yet (beid3).\n";
            exit(1);
        }

        void output_leid3(size_t pos)
        {
            snap::NOTUSED(pos);
            std::cerr << "error: type not implemented yet (leid3).\n";
            exit(1);
        }

        void output_ubeid3(size_t pos)
        {
            snap::NOTUSED(pos);
            std::cerr << "error: type not implemented yet (ubeid3).\n";
            exit(1);
        }

        void output_uleid3(size_t pos)
        {
            snap::NOTUSED(pos);
            std::cerr << "error: type not implemented yet (uleid3).\n";
            exit(1);
        }

        void output_quad(size_t pos)
        {
            snap::NOTUSED(pos);
            std::cerr << "error: type not implemented yet (quad).\n";
            exit(1);
        }

        void output_bequad(size_t pos)
        {
            snap::NOTUSED(pos);
            std::cerr << "error: type not implemented yet (bequad).\n";
            exit(1);
        }

        void output_lequad(size_t pos)
        {
            snap::NOTUSED(pos);
            std::cerr << "error: type not implemented yet (lequad).\n";
            exit(1);
        }

        void output_uquad(size_t pos)
        {
            snap::NOTUSED(pos);
            std::cerr << "error: type not implemented yet (uquad).\n";
            exit(1);
        }

        void output_ubequad(size_t pos)
        {
            snap::NOTUSED(pos);
            std::cerr << "error: type not implemented yet (ubequad).\n";
            exit(1);
        }

        void output_ulequad(size_t pos)
        {
            snap::NOTUSED(pos);
            std::cerr << "error: type not implemented yet (ulequad).\n";
            exit(1);
        }

        void output_float(size_t pos)
        {
            snap::NOTUSED(pos);
            std::cerr << "error: type not implemented yet (float).\n";
            exit(1);
        }

        void output_befloat(size_t pos)
        {
            snap::NOTUSED(pos);
            std::cerr << "error: type not implemented yet (befloat).\n";
            exit(1);
        }

        void output_lefloat(size_t pos)
        {
            snap::NOTUSED(pos);
            std::cerr << "error: type not implemented yet (lefloat).\n";
            exit(1);
        }

        void output_double(size_t pos)
        {
            snap::NOTUSED(pos);
            std::cerr << "error: type not implemented yet (double).\n";
            exit(1);
        }

        void output_bedouble(size_t pos)
        {
            snap::NOTUSED(pos);
            std::cerr << "error: type not implemented yet (bedouble).\n";
            exit(1);
        }

        void output_ledouble(size_t pos)
        {
            snap::NOTUSED(pos);
            std::cerr << "error: type not implemented yet (ledouble).\n";
            exit(1);
        }

        void output_string(size_t pos)
        {
            parser::entry_t::integer_t offset(f_entries[pos]->get_offset());
            std::string const str(f_entries[pos]->get_string());
            for(size_t i(0); i < str.length(); ++i, ++offset)
            {
                std::cout << (i > 0 ? "\n&& " : "")
                          << "buf["
                          << (f_has_mime ? "" : "offset+")
                          << offset
                          << "] "
                          << f_entries[pos]->flags_to_js_operator()
                          << " 0x"
                          << std::hex << std::uppercase
                          << (static_cast<int>(str[i]) & 0xff)
                          << std::dec << std::nouppercase;
            }
        }

        void output_pstring(size_t pos)
        {
            snap::NOTUSED(pos);
            std::cerr << "error: type not implemented yet (pstring).\n";
            exit(1);
        }

        void output_besearch16(size_t pos)
        {
            snap::NOTUSED(pos);
            std::cerr << "error: type not implemented yet (besearch16).\n";
            exit(1);
        }

        void output_lesearch16(size_t pos)
        {
            snap::NOTUSED(pos);
            std::cerr << "error: type not implemented yet (lesearch16).\n";
            exit(1);
        }

        void output_search(size_t pos)
        {
            parser::entry_t::integer_t const offset(f_entries[pos]->get_offset());
            std::cout << "snapwebsites.BufferToMIMESystemImages.scan(buf,"
                      << offset << "," << f_entries[pos]->get_maxlength()
                      << ",{";
            std::string const str(f_entries[pos]->get_string());
            for(size_t i(0); i < str.length(); ++i)
            {
                std::cout << (i == 0 ? "" : ",")
                          << std::hex << std::uppercase
                          << "0x" << static_cast<int>(str[i])
                          << std::dec << std::nouppercase;
            }
            std::cout << "});";
        }

        void output_regex(size_t pos)
        {
            parser::entry_t::integer_t const offset(f_entries[pos]->get_offset());
            std::cout << "snapwebsites.BufferToMIMESystemImages.regex(buf,"
                      << offset << "," << f_entries[pos]->get_maxlength()
                      << ",{";
            std::string const str(f_entries[pos]->get_string());
            for(size_t i(0); i < str.length(); ++i)
            {
                std::cout << (i == 0 ? "" : ",")
                          << std::hex << std::uppercase
                          << "0x" << static_cast<int>(str[i])
                          << std::dec << std::nouppercase;
            }
            std::cout << "},"
                      << (
                            ((f_entries[pos]->get_flags() & entry_t::ENTRY_FLAG_LINES           ) != 0 ? 1 : 0)
                          | ((f_entries[pos]->get_flags() & entry_t::ENTRY_FLAG_CASE_INSENSITIVE) != 0 ? 2 : 0)
                          | ((f_entries[pos]->get_flags() & entry_t::ENTRY_FLAG_START_OFFSET    ) != 0 ? 4 : 0)
                         )
                      << ");";
        }

        void output_date(size_t pos)
        {
            snap::NOTUSED(pos);
            std::cerr << "error: type not implemented yet (date).\n";
            exit(1);
        }

        void output_qdate(size_t pos)
        {
            snap::NOTUSED(pos);
            std::cerr << "error: type not implemented yet (qdate).\n";
            exit(1);
        }

        void output_ldate(size_t pos)
        {
            snap::NOTUSED(pos);
            std::cerr << "error: type not implemented yet (ldate).\n";
            exit(1);
        }

        void output_qldate(size_t pos)
        {
            snap::NOTUSED(pos);
            std::cerr << "error: type not implemented yet (qldate).\n";
            exit(1);
        }

        void output_bedate(size_t pos)
        {
            snap::NOTUSED(pos);
            std::cerr << "error: type not implemented yet (bedate).\n";
            exit(1);
        }

        void output_beqdate(size_t pos)
        {
            snap::NOTUSED(pos);
            std::cerr << "error: type not implemented yet (beqdate).\n";
            exit(1);
        }

        void output_beldate(size_t pos)
        {
            snap::NOTUSED(pos);
            std::cerr << "error: type not implemented yet (beldate).\n";
            exit(1);
        }

        void output_beqldate(size_t pos)
        {
            snap::NOTUSED(pos);
            std::cerr << "error: type not implemented yet (beqldate).\n";
            exit(1);
        }

        void output_ledate(size_t pos)
        {
            snap::NOTUSED(pos);
            std::cerr << "error: type not implemented yet (ledate).\n";
            exit(1);
        }

        void output_leqdate(size_t pos)
        {
            snap::NOTUSED(pos);
            std::cerr << "error: type not implemented yet (leqdate).\n";
            exit(1);
        }

        void output_leldate(size_t pos)
        {
            snap::NOTUSED(pos);
            std::cerr << "error: type not implemented yet (leldate).\n";
            exit(1);
        }

        void output_leqldate(size_t pos)
        {
            snap::NOTUSED(pos);
            std::cerr << "error: type not implemented yet (leqldate).\n";
            exit(1);
        }

        void output_medate(size_t pos)
        {
            snap::NOTUSED(pos);
            std::cerr << "error: type not implemented yet (medate).\n";
            exit(1);
        }

        void output_meldate(size_t pos)
        {
            snap::NOTUSED(pos);
            std::cerr << "error: type not implemented yet (meldate).\n";
            exit(1);
        }

        void output_indirect(size_t pos)
        {
            snap::NOTUSED(pos);
            std::cerr << "error: type not implemented yet (indirect).\n";
            exit(1);
        }

        void output_default(size_t pos)
        {
            // default is always true
            snap::NOTUSED(pos);
            std::cout << "true";
        }

        void output_name(size_t pos)
        {
            snap::NOTUSED(pos);
            // this is already done in the caller which generates the
            // function declaration
        }

        void output_use(size_t pos)
        {
            std::cout << "__macro_" << f_entries[pos]->get_string()
                      << "("
                      << f_entries[pos]->get_offset()
                      << ")";
        }

        void output_mimetype(size_t pos)
        {
            std::string const mimetype(f_entries[pos]->get_mimetype());
            if(!mimetype.empty())
            {
                std::cout << "return \"" << mimetype << "\";" << std::endl;
            }
        }

        void output_endif(size_t pos)
        {
            snap::NOTUSED(pos);
            std::cout << "}" << std::endl;
        }

        // variable members
        entry_vector_t  f_entries;
        bool            f_has_mime;
    };
    recursive_output out(has_mime);

    // first remove all entries that we are not going to use (i.e.
    // anything at the end which does not include a MIME type)
    entry_t::integer_t l(-1);
    if(!has_mime)
    {
        l = f_entries[end - 1]->get_level();
    }
    size_t j(end);
    while(j > start)
    {
        --j;

        if(f_entries[j]->get_type() != entry_t::type_t::ENTRY_TYPE_NAME)
        {
            if(f_entries[j]->get_mimetype().empty())
            {
                if(f_entries[j]->get_level() <= l)
                {
                    out.f_entries.insert(out.f_entries.begin(), f_entries[j]);
                }
            }
            else
            {
                l = f_entries[j]->get_level();
                out.f_entries.insert(out.f_entries.begin(), f_entries[j]);
            }
        }
    }

    out.output(0);
}


void parser::output_header()
{
    std::string lower_magic_name(f_magic_name);
    std::transform(lower_magic_name.begin(), lower_magic_name.end(), lower_magic_name.begin(), ::tolower);

    std::cout <<
"/** @preserve\n"
" * WARNING: AUTO-GENERATED FILE, DO NOT EDIT. See Source: magic-to-js.cpp\n"
" * Name: mimetype-" << lower_magic_name << "\n"
" * Version: " << MIMETYPE_VERSION_STRING << "\n"
" * Browsers: all\n"
" * Copyright: Copyright 2014-2016 (c) Made to Order Software Corporation  All rights reverved.\n"
" * Depends: output (0.1.5.5)\n"
" * License: GPL 2.0\n"
" * Source: File generated by magic-to-js from magic library definition files.\n"
" */\n"
"\n"
"\n"
"//\n"
"// Inline \"command line\" parameters for the Google Closure Compiler\n"
"// See output of:\n"
"//    java -jar .../google-js-compiler/compiler.jar --help\n"
"//\n"
"// ==ClosureCompiler==\n"
"// @compilation_level ADVANCED_OPTIMIZATIONS\n"
"// @externs $CLOSURE_COMPILER/contrib/externs/jquery-1.9.js\n"
"// @externs plugins/output/externs/jquery-extensions.js\n"
"// ==/ClosureCompiler==\n"
"//\n"
"\n"
"/*jslint nomen: true, todo: true, devel: true */\n"
"/*global snapwebsites: false, jQuery: false, Uint8Array: true */\n"
"\n"
"\n"
"\n"
"/** \\brief Check for \"system\" images.\n"
" *\n"
" * This function checks for well known images. The function is generally\n"
" * very fast because it checks only the few very well known image file\n"
" * formats.\n"
" *\n"
" * @return {!snapwebsites.BufferToMIMESystemImages} A reference to this new\n"
" *                                                  object.\n"
" *\n"
" * @extends {snapwebsites.BufferToMIMETemplate}\n"
" * @constructor\n"
" */\n"
"snapwebsites.BufferToMIME" << f_magic_name << " = function()\n"
"{\n"
"    snapwebsites.BufferToMIME" << f_magic_name << ".superClass_.constructor.call(this);\n"
"\n"
"    return this;\n"
"};\n"
"\n"
"\n"
"/** \\brief Chain up the extension.\n"
" *\n"
" * This is the chain between this class and it's super.\n"
" */\n"
"snapwebsites.inherits(snapwebsites.BufferToMIME" << f_magic_name << ", snapwebsites.BufferToMIMETemplate);\n"
"\n"
"\n"
"/** \\brief Check for the " << f_magic_name << " file formats.\n"
" *\n"
" * This function checks for file formats as defined in the magic library.\n"
" * This version includes the descriptions from the following files:\n"
" *\n"
<< f_lexer->list_of_filenames() <<
" *\n"
" * @param {!Uint8Array} buf  The array of data to check for a known magic.\n"
" *\n"
" * @return {!string} The MIME type or the empty string if not determined.\n"
" *\n"
" * @override\n"
" */\n"
"snapwebsites.BufferToMIME" << f_magic_name << ".prototype.bufferToMIME = function(buf)\n"
"{\n"
;

}


void parser::output_footer()
{
    // close the function we opened in the header
    std::cout <<
"return \"\";\n"
"};\n"
"\n"
"// auto-initialize\n"
"jQuery(document).ready(\n"
"    function()\n"
"    {\n"
"        snapwebsites.OutputInstance.registerBufferToMIME(new snapwebsites.BufferToMIME" << f_magic_name << "());\n"
"    }\n"
");\n"
;

}



int usage()
{
    std::cout << "Usage: magic-to-js <input files> ..." << std::endl;
    std::cout << "You may also want to redirect the output to a .js file" << std::endl;
    std::cout << "  --debug | -d    print out debug information in stderr" << std::endl;
    std::cout << "  --help | -h     print out this help screen" << std::endl;
    std::cout << "  --lib-version   print out this tool's version" << std::endl;
    std::cout << "  --name | -n     specify the name of the magic MIME to output" << std::endl;
    std::cout << "  --version       print out this tool's version" << std::endl;
    exit(1);
}


int main(int argc, char *argv[])
{
    try
    {
        lexer::filenames_t fn;
        std::string magic_name;

        for(int i(1); i < argc; ++i)
        {
            if(strcmp(argv[i], "-h") == 0
            || strcmp(argv[i], "--help") == 0)
            {
                usage();
                snap::NOTREACHED();
            }
            if(strcmp(argv[i], "--version") == 0)
            {
                std::cout << MIMETYPE_VERSION_STRING << std::endl;
                exit(1);
                snap::NOTREACHED();
            }
            if(strcmp(argv[i], "--lib-version") == 0)
            {
                std::cout << SNAPWEBSITES_VERSION_MAJOR << "." << SNAPWEBSITES_VERSION_MINOR << "." << SNAPWEBSITES_VERSION_PATCH << std::endl;
                exit(1);
                snap::NOTREACHED();
            }
            if(strcmp(argv[i], "-d") == 0
            || strcmp(argv[i], "--debug") == 0)
            {
                std::cerr << "info: turning debug ON\n";
                g_debug = true;
            }
            else if(strcmp(argv[i], "-n") == 0
                 || strcmp(argv[i], "--name") == 0)
            {
                ++i;
                if(i >= argc)
                {
                    std::cerr << "error: -n/--name expect to be followed by one argument, the magic name." << std::endl;
                    exit(1);
                }
                magic_name = argv[i];
            }
            else
            {
                fn.push_back(argv[i]);
            }
        }

        if(fn.empty())
        {
            std::cerr << "error: expected at least one filename on the command line. Try --help for more info." << std::endl;
            exit(1);
        }

        if(magic_name.empty())
        {
            std::cerr << "error: a magic name must be specified (--name option)" << std::endl;
            exit(1);
        }

        lexer::pointer_t l(new lexer(fn));
        parser::pointer_t p(new parser(l, magic_name));
        p->parse();

        // it worked, the parser has now a pile of parsed lines we can
        // convert in JavaScript
        p->output();

        return 0;
    }
    catch(std::exception const & e)
    {
        std::cerr << "magic-to-js: exception: " << e.what() << std::endl;
        return 1;
    }
}


// vim: ts=4 sw=4 et
