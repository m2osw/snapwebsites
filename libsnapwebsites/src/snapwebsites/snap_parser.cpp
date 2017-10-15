// Snap Websites Server -- advanced parser
// Copyright (C) 2011-2017  Made to Order Software Corp.
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

// ourselves
//
#include "snapwebsites/snap_parser.h"

// snapwebsites lib
//
#include "snapwebsites/log.h"
#include "snapwebsites/qstring_stream.h"

// Qt lib
//
#include <QList>
#include <QPointer>

// C++ lib
//
#include <iostream>

// last include
//
#include "snapwebsites/poison.h"


namespace snap
{
namespace parser
{

token_id_none_def TOKEN_ID_NONE;
token_id_integer_def TOKEN_ID_INTEGER;
token_id_float_def TOKEN_ID_FLOAT;
token_id_identifier_def TOKEN_ID_IDENTIFIER;
token_id_keyword_def TOKEN_ID_KEYWORD;
token_id_string_def TOKEN_ID_STRING;
token_id_literal_def TOKEN_ID_LITERAL;
token_id_empty_def TOKEN_ID_EMPTY;



QString token::to_string() const
{
    QString result;

    switch(f_id)
    {
    case token_t::TOKEN_ID_NONE_ENUM:
        result = "<no token>";
        break;

    case token_t::TOKEN_ID_INTEGER_ENUM:
        result = QString("int<%1>").arg(f_value.toInt());
        break;

    case token_t::TOKEN_ID_FLOAT_ENUM:
        result = QString("float<%1>").arg(f_value.toDouble());
        break;

    case token_t::TOKEN_ID_IDENTIFIER_ENUM:
        result = QString("identifier<%1>").arg(f_value.toString());
        break;

    case token_t::TOKEN_ID_KEYWORD_ENUM:
        result = QString("keyword<%1>").arg(f_value.toString());
        break;

    case token_t::TOKEN_ID_STRING_ENUM:
        result = QString("string<%1>").arg(f_value.toString());
        break;

    case token_t::TOKEN_ID_LITERAL_ENUM:
        result = QString("literal<%1>").arg(f_value.toString());
        break;

    case token_t::TOKEN_ID_EMPTY_ENUM:
        result = "empty<>";
        break;

    case token_t::TOKEN_ID_CHOICES_ENUM:
        result = QString("choices<...>");//.arg(f_value.toString());
        break;

    case token_t::TOKEN_ID_RULES_ENUM:
        result += " /* INVALID -- TOKEN_ID_RULES!!! */ ";
        break;

    case token_t::TOKEN_ID_NODE_ENUM:
        result += " /* INVALID -- TOKEN_ID_RULES!!! */ ";
        break;

    case token_t::TOKEN_ID_ERROR_ENUM:
        result += " /* INVALID -- TOKEN_ID_ERROR!!! */ ";
        break;

    default:
        result += " /* INVALID -- unknown token identifier!!! */ ";
        break;

    }

    return result;
}


/** \brief Set the input string for the lexer.
 *
 * This lexer accepts a standard QString as input. It will be what gets parsed.
 *
 * The input is never modified. It is parsed using the next_token() function.
 *
 * By default, the input is an empty string.
 *
 * \param[in] input  The input string to be parsed by this lexer.
 */
void lexer::set_input(const QString& input)
{
    f_input = input;
    f_pos = f_input.begin();
    f_line = 1;
}

/** \brief Read the next token.
 *
 * At this time we support the follow tokens:
 *
 * \li TOKEN_ID_NONE_ENUM -- the end of the input was reached
 *
 * \li TOKEN_ID_INTEGER_ENUM -- an integer ([0-9]+) number; always positive since
 *                the parser returns '-' as a separate literal
 *
 * \li TOKEN_ID_FLOAT_ENUM -- a floating point number with optinal exponent
 *                ([0-9]+\.[0-9]+([eE][+-]?[0-9]+)?); always positive since
 *                the parser returns '-' as a separate literal
 *
 * \li TOKEN_ID_IDENTIFIER_ENUM -- supports C like identifiers ([a-z_][a-z0-9_]*)
 *
 * \li TOKEN_ID_KEYWORD_ENUM -- an identifier that matches one of our keywords
 *                as defined in the keyword map
 *
 * \li TOKEN_ID_STRING_ENUM -- a string delimited by double quotes ("); support
 *                backslashes; returns the content of the string
 *                (the quotes are removed)
 *
 * \li TOKEN_ID_LITERAL_ENUM -- anything else except what gets removed (spaces,
 *                new lines, C or C++ like comments)
 *
 * \li TOKEN_ID_ERROR_ENUM -- an error occured, you can get the error message for
 *                more information
 *
 * The TOKEN_ID_LITERAL_ENUM may either return a character ('=' operator) or a
 * string ("/=" operator). The special literals are defined here:
 *
 * \li ++ - increment
 * \li += - add & assign
 * \li -- - decrement
 * \li -= - subtract & assign
 * \li *= - multiply & assign
 * \li ** - power
 * \li **= - power & assign
 * \li /= - divide & assign
 * \li %= - divide & assign
 * \li ~= - bitwise not & assign
 * \li &= - bitwise and & assign
 * \li && - logical and
 * \li &&= - logical and & assign
 * \li |= - bitwise or & assign
 * \li || - logical or
 * \li ||= - logical or & assign
 * \li ^= - bitwise xor & assign
 * \li ^^ - logical xor
 * \li ^^= - logical xor & assign
 * \li != - not equal
 * \li !== - exactly not equal
 * \li !< - rotate left
 * \li !> - rotate left
 * \li ?= - assign default if undefined
 * \li == - equal
 * \li === - exactly equal
 * \li <= - smaller or equal
 * \li << - shift left
 * \li <<= - shift left and assign
 * \li <? - minimum
 * \li <?= - minimum and assign
 * \li >= - larger or equal
 * \li >> - shift right
 * \li >>> - unsigned shift right
 * \li >>= - shift right and assign
 * \li >>>= - unsigned shift right and assign
 * \li >? - maximum
 * \li >?= - maximum and assign
 * \li := - required assignment
 * \li :: - namespace
 *
 * If the returned token says TOKEN_ID_NONE_ENUM then you reached the
 * end of the input. When it says TOKEN_ID_ERROR_ENUM, then the input
 * is invalid and the error message and line number can be retrieved
 * to inform the user.
 *
 * The parser supports any type of new lines (Unix, Windows and Mac.)
 *
 * \todo
 * Check for overflow on integers and doubles
 *
 * \todo
 * Should we include default keywords? (i.e. true, false, if, else,
 * etc.) so those cannot be used as identifiers in some places?
 *
 * \return The read token.
 */
token lexer::next_token()
{
    auto xdigit = [](int c)
    {
        if(c >= '0' && c <= '9')
        {
            return c - '0';
        }
        else if(c >= 'a' && c <= 'f')
        {
            return c - 'a' + 10;
        }
        else if(c >= 'A' && c <= 'F')
        {
            return c - 'A' + 10;
        }
        return -1;
    };
    token        result;

// restart is called whenever we find a comment or
// some other entry that just gets "deleted" from the input
// (i.e. new line, space...)
//
// Note: I don't use a do ... while(repeat); because in some cases
// we are inside several levels of switch() for() while() loops.
restart:

    // we reached the end of input
    if(f_pos == f_input.end())
    {
        return result;
    }

    switch(f_pos->unicode())
    {
    case '\n':
        ++f_pos;
        ++f_line;
        goto restart;

    case '\r':
        ++f_pos;
        ++f_line;
        if(f_pos != f_input.end() && *f_pos == '\n')
        {
            // skip "\r\n" as one end of line
            ++f_pos;
        }
        goto restart;

    case ' ':
    case '\t':
        ++f_pos;
        goto restart;

    case '+':
        result.set_id(token_t::TOKEN_ID_LITERAL_ENUM);
        result.set_value(*f_pos);
        ++f_pos;
        if(f_pos != f_input.end())
        {
            switch(f_pos->unicode())
            {
            case '=': // add and assign
                result.set_value("+=");
                ++f_pos;
                break;

            case '+': // increment
                result.set_value("++");
                ++f_pos;
                break;

            default:
                // ignore other characters
                break;

            }
        }
        break;

    case '-':
        result.set_id(token_t::TOKEN_ID_LITERAL_ENUM);
        result.set_value(*f_pos);
        ++f_pos;
        if(f_pos != f_input.end())
        {
            switch(f_pos->unicode())
            {
            case '=': // subtract and assign
                result.set_value("-=");
                ++f_pos;
                break;

            case '-': // decrement
                result.set_value("--");
                ++f_pos;
                break;

            default:
                // ignore other characters
                break;

            }
        }
        break;

    case '*':
        result.set_id(token_t::TOKEN_ID_LITERAL_ENUM);
        result.set_value(*f_pos);
        ++f_pos;
        if(f_pos != f_input.end())
        {
            switch(f_pos->unicode())
            {
            case '/': // invalid C comment end marker
                // in this case we don't have to restart since we
                // reached the end of the input
                f_error_code = lexer_error_t::LEXER_ERROR_INVALID_C_COMMENT;
                f_error_message = "comment terminator without introducer";
                f_error_line = f_line;
                result.set_id(token_t::TOKEN_ID_ERROR_ENUM);
                break;

            case '=': // multiply and assign
                result.set_value("*=");
                ++f_pos;
                break;

            case '*': // power
                result.set_value("**");
                ++f_pos;
                if(f_pos != f_input.end())
                {
                    if(*f_pos == '=')
                    {
                        // power and assign
                        result.set_value("**=");
                        ++f_pos;
                    }
                }
                break;

            default:
                // ignore other characters
                break;

            }
        }
        break;

    case '/': // divide
        result.set_id(token_t::TOKEN_ID_LITERAL_ENUM);
        result.set_value(*f_pos);
        ++f_pos;
        if(f_pos != f_input.end())
        {
            switch(f_pos->unicode())
            {
            case '/': // C++ comment -- skip up to eol
                for(++f_pos; f_pos != f_input.end(); ++f_pos)
                {
                    if(*f_pos == '\n' || *f_pos == '\r')
                    {
                        goto restart;
                    }
                }
                // in this case we don't have to restart since we
                // reached the end of the input
                result.set_id(token_t::TOKEN_ID_NONE_ENUM);
                break;

            case '*': // C comment -- skip up to */
                for(++f_pos; f_pos != f_input.end(); ++f_pos)
                {
                    if(f_pos + 1 != f_input.end() && *f_pos == '*' && f_pos[1] == '/')
                    {
                        f_pos += 2;
                        goto restart;
                    }
                }
                // in this case the comment was not terminated
                f_error_code = lexer_error_t::LEXER_ERROR_INVALID_C_COMMENT;
                f_error_message = "comment not terminated";
                f_error_line = f_line;
                result.set_id(token_t::TOKEN_ID_ERROR_ENUM);
                break;

            case '=': // divide and assign
                result.set_value("/=");
                ++f_pos;
                break;

            default:
                // ignore other characters
                break;

            }
        }
        break;

    case '%': // modulo
        result.set_id(token_t::TOKEN_ID_LITERAL_ENUM);
        result.set_value(*f_pos);
        ++f_pos;
        if(f_pos != f_input.end())
        {
            switch(f_pos->unicode())
            {
            case '=': // modulo and assign
                result.set_value("%=");
                ++f_pos;
                break;

            default:
                // ignore other characters
                break;

            }
        }
        break;

    case '~': // bitwise not
        result.set_id(token_t::TOKEN_ID_LITERAL_ENUM);
        result.set_value(*f_pos);
        ++f_pos;
        if(f_pos != f_input.end())
        {
            switch(f_pos->unicode())
            {
            case '=': // bitwise not and assign
                result.set_value("~=");
                ++f_pos;
                break;

            default:
                // ignore other characters
                break;

            }
        }
        break;

    case '&': // bitwise and
        result.set_id(token_t::TOKEN_ID_LITERAL_ENUM);
        result.set_value(*f_pos);
        ++f_pos;
        if(f_pos != f_input.end())
        {
            switch(f_pos->unicode())
            {
            case '=': // bitwise and & assign
                result.set_value("&=");
                ++f_pos;
                break;

            case '&': // logical and
                result.set_value("&&");
                ++f_pos;
                if(f_pos != f_input.end())
                {
                    if(*f_pos == '=')
                    {
                        // logical and & assign
                        result.set_value("&&=");
                        ++f_pos;
                    }
                }
                break;

            default:
                // ignore other characters
                break;

            }
        }
        break;

    case '|': // bitwise or
        result.set_id(token_t::TOKEN_ID_LITERAL_ENUM);
        result.set_value(*f_pos);
        ++f_pos;
        if(f_pos != f_input.end())
        {
            switch(f_pos->unicode())
            {
            case '=': // bitwise or & assign
                result.set_value("|=");
                ++f_pos;
                break;

            case '|': // logical or
                result.set_value("||");
                ++f_pos;
                if(f_pos != f_input.end())
                {
                    if(*f_pos == '=')
                    {
                        // logical or and assign
                        result.set_value("||=");
                        ++f_pos;
                    }
                }
                break;

            default:
                // ignore other characters
                break;

            }
        }
        break;

    case '^': // bitwise xor
        result.set_id(token_t::TOKEN_ID_LITERAL_ENUM);
        result.set_value(*f_pos);
        ++f_pos;
        if(f_pos != f_input.end())
        {
            switch(f_pos->unicode())
            {
            case '=': // bitwise xor & assign
                result.set_value("^=");
                ++f_pos;
                break;

            case '^': // logical xor
                result.set_value("^^");
                ++f_pos;
                if(f_pos != f_input.end())
                {
                    if(*f_pos == '=')
                    {
                        // logical xor and assign
                        result.set_value("^^=");
                        ++f_pos;
                    }
                }
                break;

            default:
                // ignore other characters
                break;

            }
        }
        break;

    case '!': // logical not
        result.set_id(token_t::TOKEN_ID_LITERAL_ENUM);
        result.set_value(*f_pos);
        ++f_pos;
        if(f_pos != f_input.end())
        {
            switch(f_pos->unicode())
            {
            case '=': // not equal
                result.set_value("!=");
                ++f_pos;
                if(f_pos != f_input.end())
                {
                    if(*f_pos == '=')
                    {
                        // exactly not equal (type checked)
                        result.set_value("!==");
                        ++f_pos;
                    }
                }
                break;

            case '<': // rotate left
                result.set_value("!<");
                ++f_pos;
                break;

            case '>': // rotate right
                result.set_value("!>");
                ++f_pos;
                break;

            default:
                // ignore other characters
                break;

            }
        }
        break;

    case '?': // ? by itself is used here and there generally similar to C/C++
        result.set_id(token_t::TOKEN_ID_LITERAL_ENUM);
        result.set_value(*f_pos);
        ++f_pos;
        if(f_pos != f_input.end())
        {
            switch(f_pos->unicode())
            {
            case '=': // assign if left hand side not set
                result.set_value("?=");
                ++f_pos;
                break;

            default:
                // ignore other characters
                break;

            }
        }
        break;

    case '=': // assign
        result.set_id(token_t::TOKEN_ID_LITERAL_ENUM);
        result.set_value(*f_pos);
        ++f_pos;
        if(f_pos != f_input.end())
        {
            switch(f_pos->unicode())
            {
            case '=': // equality check (compare)
                result.set_value("==");
                ++f_pos;
                if(f_pos != f_input.end())
                {
                    if(*f_pos == '=')
                    {
                        // exactly equal (type checked)
                        result.set_value("===");
                        ++f_pos;
                    }
                }
                break;

            default:
                // ignore other characters
                break;

            }
        }
        break;

    case '<': // greater than
        result.set_id(token_t::TOKEN_ID_LITERAL_ENUM);
        result.set_value(*f_pos);
        ++f_pos;
        if(f_pos != f_input.end())
        {
            switch(f_pos->unicode())
            {
            case '=': // smaller or equal
                result.set_value("<=");
                ++f_pos;
                break;

            case '<': // shift left
                result.set_value("<<");
                ++f_pos;
                if(f_pos != f_input.end())
                {
                    if(*f_pos == '=')
                    {
                        // shift left and assign
                        result.set_value("<<=");
                        ++f_pos;
                    }
                }
                break;

            case '?': // minimum
                result.set_value("<?");
                ++f_pos;
                if(f_pos != f_input.end())
                {
                    if(*f_pos == '=')
                    {
                        // minimum and assign
                        result.set_value("<?=");
                        ++f_pos;
                    }
                }
                break;

            default:
                // ignore other characters
                break;

            }
        }
        break;

    case '>': // less than
        result.set_id(token_t::TOKEN_ID_LITERAL_ENUM);
        result.set_value(*f_pos);
        ++f_pos;
        if(f_pos != f_input.end())
        {
            switch(f_pos->unicode())
            {
            case '=': // larger or equal
                result.set_value(">=");
                ++f_pos;
                break;

            case '>': // shift right
                result.set_value(">>");
                ++f_pos;
                if(f_pos != f_input.end())
                {
                    switch(f_pos->unicode())
                    {
                    case '=':
                        // shift right and assign
                        result.set_value(">>=");
                        ++f_pos;
                        break;

                    case '>':
                        // unsigned shift right
                        result.set_value(">>>");
                        ++f_pos;
                        if(f_pos != f_input.end())
                        {
                            if(*f_pos == '=')
                            {
                                // unsigned right shift and assign
                                result.set_value(">>>=");
                                ++f_pos;
                            }
                        }
                        break;

                    default:
                        // ignore other characters
                        break;

                    }
                }
                break;

            case '?': // maximum
                result.set_value(">?");
                ++f_pos;
                if(f_pos != f_input.end())
                {
                    if(*f_pos == '=')
                    {
                        // maximum and assign
                        result.set_value(">?=");
                        ++f_pos;
                    }
                }
                break;

            default:
                // ignore other characters
                break;

            }
        }
        break;

    case ':':
        result.set_id(token_t::TOKEN_ID_LITERAL_ENUM);
        result.set_value(*f_pos);
        ++f_pos;
        if(f_pos != f_input.end())
        {
            switch(f_pos->unicode())
            {
            case '=': // required
                result.set_value(":=");
                ++f_pos;
                break;

            case ':': // namespace
                result.set_value("::");
                ++f_pos;
                break;

            default:
                // ignore other characters
                break;

            }
        }
        break;

    case '"':
        {
            ++f_pos;
            QString str;
            while(f_pos != f_input.end() && *f_pos != '"')
            {
                if(*f_pos == '\n' || *f_pos == '\r')
                {
                    // strings cannot continue after the end of a line
                    break;
                }
                if(*f_pos == '\\')
                {
                    ++f_pos;
                    if(f_pos == f_input.end())
                    {
                        // this is an invalid backslash
                        break;
                    }
                    // TODO: add support for \x## and various other
                    //       escaped characters
                    switch(f_pos->unicode())
                    {
                    case 'a':
                        str += "\a";
                        break;

                    case 'b':
                        str += "\b";
                        break;

                    case 'f':
                        str += "\f";
                        break;

                    case 'n':
                        str += "\n";
                        break;

                    case 'r':
                        str += "\r";
                        break;

                    case 't':
                        str += "\t";
                        break;

                    case 'v':
                        str += "\v";
                        break;

                    case '0':
                    case '1':
                    case '2':
                    case '3':
                    case '4':
                    case '5':
                    case '6':
                    case '7':
                        // "\nnn" -- maximum of 3 digits
                        {
                            int v(f_pos->unicode() - '0');
                            if(f_pos + 1 != f_input.end() && (f_pos + 1)->unicode() >= '0' && (f_pos + 1)->unicode() <= '7')
                            {
                                ++f_pos;
                                v = v * 8 + f_pos->unicode() - '0';

                                if(f_pos + 1 != f_input.end() && (f_pos + 1)->unicode() >= '0' && (f_pos + 1)->unicode() <= '7')
                                {
                                    ++f_pos;
                                    v = v * 8 + f_pos->unicode() - '0';
                                }
                            }
                            str += QChar(v);
                        }
                        break;

                    case 'x':
                    case 'X':
                        {
                            if(f_pos + 1 != f_input.end() && std::isxdigit((f_pos + 1)->unicode()))
                            {
                                ++f_pos;
                                int v(xdigit(f_pos->unicode()));

                                if(f_pos + 1 != f_input.end() && std::isxdigit((f_pos + 1)->unicode()))
                                {
                                    ++f_pos;
                                    v = v * 16 + xdigit(f_pos->unicode());
                                }

                                str += QChar(v);
                            }
                        }
                        break;

                    case 'u':
                        // take 0 to 4 digits
                        {
                            int v(0);
                            for(int idx(0); idx < 4; ++idx)
                            {
                                if(f_pos == f_input.end()
                                || !std::isxdigit((f_pos + 1)->unicode()))
                                {
                                    break;
                                }
                                ++f_pos;
                                v = v * 16 + xdigit(f_pos->unicode());
                            }
                            str += QChar(v);
                        }
                        break;

                    case 'U':
                        // take 0 to 8 digits
                        {
                            uint v(0);
                            for(int idx(0); idx < 8; ++idx)
                            {
                                if(f_pos == f_input.end()
                                || !std::isxdigit((f_pos + 1)->unicode()))
                                {
                                    break;
                                }
                                ++f_pos;
                                v = v * 16 + xdigit(f_pos->unicode());
                            }
                            str += QString::fromUcs4(&v, 1);
                        }
                        break;

                    default:
                        // anything, keep as is (", ', ?, \)
                        str += *f_pos;
                        break;

                    }
                }
                else
                {
                    str += *f_pos;
                }
                ++f_pos;
            }
            if(f_pos == f_input.end())
            {
                f_error_code = lexer_error_t::LEXER_ERROR_INVALID_STRING;
                f_error_message = "invalid string";
                f_error_line = f_line;
                result.set_id(token_t::TOKEN_ID_ERROR_ENUM);
            }
            else
            {
                result.set_id(token_t::TOKEN_ID_STRING_ENUM);
                result.set_value(str);
                ++f_pos; // skip the closing quote
            }
        }
        break;

    case '0':
        // hexadecimal?
        if(f_pos + 1 != f_input.end() && (f_pos[1] == 'x' || f_pos[1] == 'X')
        && f_pos + 2 != f_input.end() && ((f_pos[2] >= '0' && f_pos[2] <= '9')
                                    || (f_pos[2] >= 'a' && f_pos[2] <= 'f')
                                    || (f_pos[2] >= 'A' && f_pos[2] <= 'F')))
        {
            bool ok;
            f_pos += 2; // skip the 0x or 0X
            QString::const_iterator start(f_pos);
            // parse number
            while(f_pos != f_input.end() && ((*f_pos >= '0' && *f_pos <= '9')
                    || (*f_pos >= 'a' && *f_pos <= 'f')
                    || (*f_pos >= 'A' && *f_pos <= 'F')))
            {
                ++f_pos;
            }
            result.set_id(token_t::TOKEN_ID_INTEGER_ENUM);
            QString value(start, static_cast<int>(f_pos - start));
            result.set_value(value.toULongLong(&ok, 16));
            if(!ok)
            {
                // as far as I know the only reason it can fail is because
                // it is too large (since we parsed a valid number!)
                f_error_code = lexer_error_t::LEXER_ERROR_INVALID_NUMBER;
                f_error_message = "number too large";
                f_error_line = f_line;
                result.set_id(token_t::TOKEN_ID_ERROR_ENUM);
            }
            break;
        }
        // no octal support at this point, octal is not available in
        // JavaScript by default!
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        {
            bool ok;
            // TODO: test overflows
            QString::const_iterator start(f_pos);
            // number
            do
            {
                ++f_pos;
            }
            while(f_pos != f_input.end() && *f_pos >= '0' && *f_pos <= '9');
            if(*f_pos == '.')
            {
                // skip the decimal point
                ++f_pos;

                // floating point
                while(f_pos != f_input.end() && *f_pos >= '0' && *f_pos <= '9')
                {
                    ++f_pos;
                }
                // TODO: add exponent support
                result.set_id(token_t::TOKEN_ID_FLOAT_ENUM);
                QString value(start, static_cast<int>(f_pos - start));
                result.set_value(value.toDouble(&ok));
            }
            else
            {
                result.set_id(token_t::TOKEN_ID_INTEGER_ENUM);
                QString value(start, static_cast<int>(f_pos - start));
                result.set_value(value.toULongLong(&ok));
            }
            if(!ok)
            {
                // as far as I know the only reason it can fail is because
                // it is too large (since we parsed a valid number!)
                f_error_code = lexer_error_t::LEXER_ERROR_INVALID_NUMBER;
                f_error_message = "number too large";
                f_error_line = f_line;
                result.set_id(token_t::TOKEN_ID_ERROR_ENUM);
            }
        }
        break;

    default:
        // TBD: add support for '$' for JavaScript?
        if((*f_pos >= 'a' && *f_pos <= 'z')
        || (*f_pos >= 'A' && *f_pos <= 'Z')
        || *f_pos == '_')
        {
            // identifier
            QString::const_iterator start(f_pos);
            ++f_pos;
            while(f_pos != f_input.end()
                && ((*f_pos >= 'a' && *f_pos <= 'z')
                    || (*f_pos >= 'A' && *f_pos <= 'Z')
                    || (*f_pos >= '0' && *f_pos <= '9')
                    || *f_pos == '_'))
            {
                ++f_pos;
            }
            QString identifier(start, static_cast<int>(f_pos - start));
            if(f_keywords.contains(identifier))
            {
                result.set_id(token_t::TOKEN_ID_KEYWORD_ENUM);
                result.set_value(f_keywords[identifier]);
            }
            else
            {
                result.set_id(token_t::TOKEN_ID_IDENTIFIER_ENUM);
                result.set_value(identifier);
            }
        }
        else
        {
            // in all other cases return a QChar
            result.set_id(token_t::TOKEN_ID_LITERAL_ENUM);
            result.set_value(*f_pos);
            ++f_pos;
        }
        break;

    }

// Only to help with debug sessions
//std::cerr << "lexer result: " << result.to_string() << "\n";

    return result;
}

void lexer::add_keyword(keyword& k)
{
    f_keywords[k.identifier()] = k.number();
}


int    keyword::g_next_number = 0;

keyword::keyword(lexer& parent, const QString& keyword_identifier, int index_number)
    : f_number(index_number == 0 ? ++g_next_number : index_number)
    , f_identifier(keyword_identifier)
{
    parent.add_keyword(*this);
}



rule::rule_data_t::rule_data_t()
    : f_token(token_t::TOKEN_ID_NONE_ENUM)
    //, f_value("") -- auto-init
    //, f_keyword() -- auto-init
    , f_choices(nullptr)
{
}

rule::rule_data_t::rule_data_t(rule_data_t const& s)
    : f_token(s.f_token)
    , f_value(s.f_value)
    , f_keyword(s.f_keyword)
    , f_choices(s.f_choices)
{
}

rule::rule_data_t::rule_data_t(choices& c)
    : f_token(token_t::TOKEN_ID_CHOICES_ENUM)
    //, f_value("") -- auto-init
    //, f_keyword() -- auto-init
    , f_choices(&c)
{
}

rule::rule_data_t::rule_data_t(token_t token)
    : f_token(token)
    //, f_value("") -- auto-init
    //, f_keyword() -- auto-init
    , f_choices(nullptr)
{
}

rule::rule_data_t::rule_data_t(const QString& value)
    : f_token(token_t::TOKEN_ID_LITERAL_ENUM)
    , f_value(value)
    //, f_keyword() -- auto-init
    , f_choices(nullptr)
{
}

rule::rule_data_t::rule_data_t(const keyword& k)
    : f_token(token_t::TOKEN_ID_KEYWORD_ENUM)
    //, f_value("") -- auto-init
    , f_keyword(k)
    , f_choices(nullptr)
{
}



rule::rule(choices& c)
    : f_parent(&c)
    //, f_tokens() -- auto-init
    , f_reducer(nullptr)
{
}

rule::rule(const rule& r)
    : f_parent(r.f_parent)
    , f_tokens(r.f_tokens)
    , f_reducer(r.f_reducer)
{
}

void rule::add_rules(choices& c)
{
    rule_data_t data(c);
    data.f_token = token_t::TOKEN_ID_RULES_ENUM;
    f_tokens.push_back(data);
}

void rule::add_choices(choices& c)
{
    f_tokens.push_back(rule_data_t(c));
}

void rule::add_token(token_t token)
{
    f_tokens.push_back(rule_data_t(token));
}

void rule::add_literal(const QString& value)
{
    f_tokens.push_back(rule_data_t(value));
}

void rule::add_keyword(const keyword& k)
{
    f_tokens.push_back(rule_data_t(k));
}

rule& rule::operator >> (const token_id& token)
{
    add_token(token);
    return *this;
}

rule& rule::operator >> (const QString& literal)
{
    add_literal(literal);
    return *this;
}

rule& rule::operator >> (char const *literal)
{
    add_literal(literal);
    return *this;
}

rule& rule::operator >> (keyword const& k)
{
    add_keyword(k);
    return *this;
}

rule& rule::operator >> (choices& c)
{
    add_choices(c);
    return *this;
}

rule& rule::operator >= (rule::reducer_t function)
{
    set_reducer(function);
    return *this;
}

rule& operator >> (token_id const& token_left, token_id const& token_right)
{
    rule *r(new rule);
    r->add_token(token_left);
    r->add_token(token_right);
    return *r;
}

rule& operator >> (token_id const& token, QString const& literal)
{
    rule *r(new rule);
    r->add_token(token);
    r->add_literal(literal);
    return *r;
}

rule& operator >> (token_id const& token, char const *literal)
{
    rule *r(new rule);
    r->add_token(token);
    r->add_literal(literal);
    return *r;
}

rule& operator >> (token_id const& token, keyword const& k)
{
    rule *r(new rule);
    r->add_token(token);
    r->add_keyword(k);
    return *r;
}

rule& operator >> (token_id const& token, choices& c)
{
    rule *r(new rule);
    r->add_token(token);
    r->add_choices(c);
    return *r;
}

rule& operator >> (QString const& literal, token_id const& token)
{
    rule *r(new rule);
    r->add_literal(literal);
    r->add_token(token);
    return *r;
}

rule& operator >> (QString const& literal_left, QString const& literal_right)
{
    rule *r(new rule);
    r->add_literal(literal_left);
    r->add_literal(literal_right);
    return *r;
}

rule& operator >> (QString const& literal, keyword const& k)
{
    rule *r(new rule);
    r->add_literal(literal);
    r->add_keyword(k);
    return *r;
}

rule& operator >> (QString const& literal, choices& c)
{
    rule *r(new rule);
    r->add_literal(literal);
    r->add_choices(c);
    return *r;
}

rule& operator >> (keyword const& k, token_id const& token)
{
    rule *r(new rule);
    r->add_keyword(k);
    r->add_token(token);
    return *r;
}

rule& operator >> (keyword const& k, QString const& literal)
{
    rule *r(new rule);
    r->add_keyword(k);
    r->add_literal(literal);
    return *r;
}

rule& operator >> (keyword const& k_left, keyword const& k_right)
{
    rule *r(new rule);
    r->add_keyword(k_left);
    r->add_keyword(k_right);
    return *r;
}

rule& operator >> (keyword const& k, choices& c)
{
    rule *r(new rule);
    r->add_keyword(k);
    r->add_choices(c);
    return *r;
}

rule& operator >> (choices& c, token_id const& token)
{
    rule *r(new rule);
    r->add_choices(c);
    r->add_token(token);
    return *r;
}

rule& operator >> (choices& c, QString const& literal)
{
    rule *r(new rule);
    r->add_choices(c);
    r->add_literal(literal);
    return *r;
}

rule& operator >> (choices& c, keyword const& k)
{
    rule *r(new rule);
    r->add_choices(c);
    r->add_keyword(k);
    return *r;
}

rule& operator >> (choices& c_left, choices& c_right)
{
    rule *r(new rule);
    r->add_choices(c_left);
    r->add_choices(c_right);
    return *r;
}

rule& operator >> (char const *literal, choices& c)
{
    rule *r(new rule);
    r->add_literal(literal);
    r->add_choices(c);
    return *r;
}

rule& operator >= (token_id const& token, rule::reducer_t function)
{
    rule *r(new rule);
    r->add_token(token);
    r->set_reducer(function);
    return *r;
}

rule& operator >= (QString const& literal, rule::reducer_t function)
{
    rule *r(new rule);
    r->add_literal(literal);
    r->set_reducer(function);
    return *r;
}

rule& operator >= (keyword const& k, rule::reducer_t function)
{
    rule *r(new rule);
    r->add_keyword(k);
    r->set_reducer(function);
    return *r;
}

rule& operator >= (choices& c, rule::reducer_t function)
{
    rule *r(new rule);
    r->add_choices(c);
    r->set_reducer(function);
    return *r;
}

QString rule::to_string() const
{
    QString        result;

    for(QVector<rule_data_t>::const_iterator ri = f_tokens.begin();
                                            ri != f_tokens.end(); ++ri)
    {
        if(ri != f_tokens.begin())
        {
            result += " ";
        }
        const rule_data_t& r(*ri);
        switch(r.f_token)
        {
        case token_t::TOKEN_ID_NONE_ENUM:
            result += "\xA4";  // currency sign used as the EOI marker
            break;

        case token_t::TOKEN_ID_INTEGER_ENUM:
            result += "TOKEN_ID_INTEGER";
            break;

        case token_t::TOKEN_ID_FLOAT_ENUM:
            result += "TOKEN_ID_FLOAT";
            break;

        case token_t::TOKEN_ID_IDENTIFIER_ENUM:
            result += "TOKEN_ID_IDENTIFIER";
            break;

        case token_t::TOKEN_ID_KEYWORD_ENUM:
            result += "keyword_" + r.f_keyword.identifier();
            break;

        case token_t::TOKEN_ID_STRING_ENUM:
            result += "TOKEN_ID_STRING";
            break;

        case token_t::TOKEN_ID_LITERAL_ENUM:
            result += "\"" + r.f_value + "\"";
            break;

        case token_t::TOKEN_ID_EMPTY_ENUM:
            // put the empty set for empty
            result += "\xF8";
            break;

        case token_t::TOKEN_ID_CHOICES_ENUM:
            // you can select the one with the pointer for debugging
            //result += QString("[0x%1] %2").arg(reinterpret_cast<qulonglong>(r.f_choices), 0, 16).arg(r.f_choices->name());
            result += QString("%2").arg(r.f_choices->name());
            break;

        case token_t::TOKEN_ID_NODE_ENUM:
            result += " /* INVALID -- TOKEN_ID_NODE!!! */ ";
            break;

        case token_t::TOKEN_ID_ERROR_ENUM:
            result += " /* INVALID -- TOKEN_ID_ERROR!!! */ ";
            break;

        default:
            result += " /* INVALID -- unknown token identifier!!! */ ";
            break;

        }
    }

    if(f_reducer != nullptr)
    {
        // show that we have a reducer
        result += " { ... }";
    }

    return result;
}




choices::choices(grammar *parent, const char *choice_name)
    : f_name(choice_name)
      //f_rules() -- auto-init
{
    if(parent != nullptr)
    {
        parent->add_choices(*this);
    }
}

choices::~choices()
{
    clear();
}

void choices::clear()
{
    int const max_rules(f_rules.count());
    for(int r = 0; r < max_rules; ++r)
    {
        delete f_rules[r];
    }
    f_rules.clear();
}


choices& choices::operator = (const choices& rhs)
{
    if(this != &rhs)
    {
        //f_name -- not changed, rhs.f_name is probably "internal"

        clear();

        // copy rhs rules
        int const max_rules(rhs.f_rules.count());
        for(int r = 0; r < max_rules; ++r)
        {
            f_rules.push_back(new rule(*rhs.f_rules[r]));
        }
    }

    return *this;
}

choices& choices::operator >>= (choices& rhs)
{
    if(this == &rhs)
    {
        throw snap_logic_exception("a rule cannot just be represented as itself");
    }

    rule *r(new rule);
    r->add_choices(rhs);
    f_rules.push_back(r);

    return *this;
}

choices& choices::operator >>= (rule& r)
{
    // in this case there are no choices
    if(r[0].get_token().get_id() == token_t::TOKEN_ID_RULES_ENUM)
    {
        this->operator = (r[0].get_choices());
    }
    else
    {
        f_rules.push_back(&r);
    }

    return *this;
}

choices& choices::operator >>= (token_id const& token)
{
    rule *r = new rule;
    r->add_token(token);
    f_rules.push_back(r);

    return *this;
}

choices& choices::operator >>= (QString const& literal)
{
    rule *r = new rule;
    r->add_literal(literal);
    f_rules.push_back(r);

    return *this;
}

choices& choices::operator >>= (keyword const& k)
{
    rule *r = new rule;
    r->add_keyword(k);
    f_rules.push_back(r);

    return *this;
}


rule& choices::operator | (rule& r)
{
    // left hand-side is this
    rule *l(new rule);
    l->add_choices(*this);

    return *l | r;
}

rule& operator | (rule& r_left, token_id const& token)
{
    choices *c(new choices(nullptr, "internal"));
    rule *r_right(new rule);
    r_right->add_token(token);
    c->add_rule(r_left);
    c->add_rule(*r_right);
    rule *r(new rule);
    r->add_rules(*c);
    return *r;
}

rule& operator | (token_id const& token, rule& r_right)
{
    choices *c(new choices(nullptr, "internal"));
    rule *r_left(new rule);
    r_left->add_token(token);
    c->add_rule(*r_left);
    c->add_rule(r_right);
    rule *r(new rule);
    r->add_rules(*c);
    return *r;
}

rule& operator | (rule& r_left, keyword const& k)
{
    choices *c(new choices(nullptr, "internal"));
    rule *r_right(new rule);
    r_right->add_keyword(k);
    c->add_rule(r_left);
    c->add_rule(*r_right);
    rule *r(new rule);
    r->add_rules(*c);
    return *r;
}

rule& operator | (rule& r_left, rule& r_right)
{
    // append to existing list?
    if(r_left[0].get_token().get_id() == token_t::TOKEN_ID_RULES_ENUM)
    {
        r_left[0].get_choices().add_rule(r_right);
        return r_left;
    }

    choices *c(new choices(nullptr, "internal"));
    c->add_rule(r_left);
    c->add_rule(r_right);
    rule *r(new rule);
    r->add_rules(*c);
    return *r;
}

rule& operator | (rule& r, choices& c)
{
    rule *l(new rule);
    l->add_choices(c);

    return r | *l;
}

void choices::add_rule(rule& r)
{
    f_rules.push_back(&r);
}



QString choices::to_string() const
{
    // you can select the one with the pointer for debugging
    //QString result(QString("[0x%1] %2: ").arg(reinterpret_cast<qulonglong>(this), 0, 16).arg(f_name));
    QString result(QString("%2: ").arg(f_name));

    for(QVector<rule *>::const_iterator ri = f_rules.begin();
                                        ri != f_rules.end(); ++ri)
    {
        if(ri != f_rules.begin())
        {
            result += "\n    | ";
        }
        rule const *r(*ri);
        result += r->to_string();
    }

    return result;
}






grammar::grammar()
    //: f_choices() -- auto-init
{
}

void grammar::add_choices(choices& c)
{
    f_choices.push_back(&c);
}

struct parser_state;
typedef QVector<parser_state *> state_array_t;
typedef QMap<parser_state *, int> state_map_t;
struct parser_state
{
    parser_state(parser_state * parent, choices & c, int r)
        //: f_lock(false) -- auto-init
        //, f_line(-1) -- auto-init
        : f_parent(parent)
        //, f_children() -- auto-init
        , f_choices(&c)
        , f_rule(r)
        //, f_position(0) -- auto-init
        //, f_node() -- auto-init
        //, f_add_on_reduce() -- auto-init
    {
        if(parent != nullptr)
        {
            parent->f_children.push_back(this);
        }
    }

    ~parser_state()
    {
//std::cerr << "destructor! " << this << "\n";
        try
        {
            clear();
        }
        catch(snap_logic_exception const &)
        {
        }
    }

    void clear()
    {
        if(!f_children.empty())
        {
            throw snap_logic_exception("clearing a state that has children is not allowed");
        }
        // if we have a parent make sure we're removed from the list
        // of children of that parent
        if(f_parent != nullptr)
        {
            int const p(f_parent->f_children.indexOf(this));
            if(p < 0)
            {
                throw snap_logic_exception("clearing a state with a parent that doesn't know about us is not allowed");
            }
            f_parent->f_children.remove(p);
            f_parent = nullptr;
        }
        // delete all the states to be executed on reduce
        // if they're still here, they can be removed
        while(!f_add_on_reduce.empty())
        {
            delete f_add_on_reduce.last();
            f_add_on_reduce.pop_back();
        }
        // useful for debug purposes
        f_choices = nullptr;
        f_rule = -1;
        f_position = -1;
    }

    void reset(parser_state * parent, choices & c, int const r)
    {
        f_parent = parent;
        if(parent != nullptr)
        {
            parent->f_children.push_back(this);
        }
        f_choices = &c;
        f_rule = r;
        f_position = 0;
        f_node.clear();
        f_add_on_reduce.clear();
    }

    static parser_state * alloc(state_array_t & free_states, parser_state * parent, choices & c, int const r)
    {
        parser_state * state;
        if(free_states.empty())
        {
            state = new parser_state(parent, c, r);
        }
        else
        {
            state = free_states.last();
            free_states.pop_back();
            state->reset(parent, c, r);
        }
        return state;
    }

    static void free(state_array_t & current, state_array_t & free_states, parser_state * s)
    {
#ifdef DEBUG
        if(s->f_lock)
        {
            throw snap_logic_exception("state that was not yet properly checked is getting deleted");
        }
#endif

        // recursively free all the children
        while(!s->f_children.empty())
        {
            free(current, free_states, s->f_children.last());
            //s->f_children.pop_back(); -- automatic in clear()
        }
        s->clear();
        int const pos(current.indexOf(s));
        if(pos != -1)
        {
            current.remove(pos);
        }
        free_states.push_back(s);
    }

    static parser_state * copy(state_array_t& free_states, parser_state * source)
    {
        parser_state * state(alloc(free_states, source->f_parent, *source->f_choices, source->f_rule));
        state->f_line = source->f_line;
        state->f_position = source->f_position;
        if(source->f_node != nullptr)
        {
            state->f_node = QSharedPointer<token_node>(new token_node(*source->f_node));
        }
        state->copy_reduce_states(free_states, source->f_add_on_reduce);
        return state;
    }

    void copy_reduce_states(state_array_t & free_states, state_array_t & add_on_reduce)
    {
        int const max_reduce(add_on_reduce.size());
        for(int i(0); i < max_reduce; ++i)
        {
            // we need to set the correct parent in the copy
            // and it is faster to correct in the source before the copy
            f_add_on_reduce.push_back(copy(free_states, add_on_reduce[i]));
        }
    }

    void add_token(token & t)
    {
        if(f_node == nullptr)
        {
            f_node = QSharedPointer<token_node>(new token_node);
            f_node->set_line(f_line);
        }
        f_node->add_token(t);
    }

    void add_node(QSharedPointer<token_node> n)
    {
        if(f_node == nullptr)
        {
            f_node = QSharedPointer<token_node>(new token_node);
            f_node->set_line(f_line);
        }
        f_node->add_node(n);
    }

    QString toString()
    {
        QString result;

        result = QString("0x%1-%2 [r:%3, p:%4/%5]")
                    .arg(reinterpret_cast<qulonglong>(this), 0, 16)
                    .arg(f_choices->name())
                    .arg(f_rule)
                    .arg(f_position)
                    .arg((*f_choices)[f_rule].count());
        if(f_parent != nullptr)
        {
            result += QString(" (parent 0x%5-%6)")
                    .arg(reinterpret_cast<qulonglong>(f_parent), 0, 16)
                    .arg(f_parent->f_choices->name());
        }

        return result;
    }

    /** \brief Display an array of states.
     *
     * This function displays the array of states as defined by the parameter
     * \p a. This prints all the parents of each element and also the list
     * of add on reduce if any.
     *
     * \param[in] a  The array to be displayed.
     */
#ifdef DEBUG
    static void display_array(const state_array_t & a)
    {
        SNAP_LOG_TRACE() << "+++ ARRAY (" << a.size() << " items)\n";
        for(state_array_t::const_iterator it(a.begin()); it != a.end(); ++it)
        {
            parser_state * state(*it);
            //std::cerr << "  state = " << state << "\n"; // for crash
            SNAP_LOG_TRACE() << "  current: " << state->toString() << "\n";
            for(state_array_t::const_iterator r(state->f_add_on_reduce.begin()); r != state->f_add_on_reduce.end(); ++r)
            {
                parser_state * s(*r);
                SNAP_LOG_TRACE() << "      add on reduce: " << s->toString() << "\n";
            }
            while(state->f_parent != nullptr)
            {
                state = state->f_parent;
                SNAP_LOG_TRACE() << "    parent: " << state->toString() << "\n";
            }
        }
        SNAP_LOG_TRACE() << "---\n";
    }

    void lock()
    {
        f_lock = true;
    }

    void unlock()
    {
        f_lock = false;
    }

#endif

    bool                            f_lock = false;

    int32_t                         f_line = -1;
    parser_state *                  f_parent = nullptr;
    state_array_t                   f_children;

    choices *                       f_choices = nullptr;
    int32_t                         f_rule = 0;
    int32_t                         f_position = 0;

    QSharedPointer<token_node>      f_node;
    state_array_t                   f_add_on_reduce;
};

/** \brief Move to the next token in a rule.
 *
 * Each state includes a position in one specific rule. This function moves
 * that pointer to the next position.
 *
 * When the end of the rule is reached, then the rule gets reduced. This means
 * calling the user reduce function and removing the rule from the current list
 * and replacing it with its parent.
 *
 * Reducing means removing the current state and putting it the list of
 * free state after we added the node tree to its parent. The parent is
 * then added to the list of current state as it becomes current again.
 *
 * When reducing a rule and moving up to the parent, the parent may then need
 * reduction too! Thus, the function loops and reduce this state and all of
 * its parent until a state that cannot be reduced anymore.
 *
 * This function also detects recursive rules and place those in the current
 * stack of states as expected. Note that next_token() is called on the
 * recursive rule too. This is a recursive function call, but it is very
 * unlikely to be called more than twice.
 *
 * \param[in] state  The state being moved.
 * \param[in] current  The list of current states
 * \param[in] free_states  The list of free states
 */
void next_token(parser_state *state, state_array_t& current, state_array_t& free_states)
{
    bool repeat;
    do
    {
        repeat = false;
        // move forward to the next token in this rule
        ++state->f_position;
        if(state->f_position >= (*state->f_choices)[state->f_rule].count())
        {
            if(state->f_position == (*state->f_choices)[state->f_rule].count())
            {
                repeat = true;

                // we reached the end of the rule, we can reduce it!
                // call user function
//std::cerr << "reduce -- " << state->f_choices->name() << ": " << (*state->f_choices)[state->f_rule].to_string() << "\n";
                (*state->f_choices)[state->f_rule].reduce(state->f_node);

                // add the recursive children in the current stack
                // check for recursive children (a: b | a ',' b)
                int const max_choices(state->f_choices->count());
                for(int i(0); i < max_choices; ++i)
                {
                    rule const& r((*state->f_choices)[i]);
                    if(token_t::TOKEN_ID_CHOICES_ENUM == r[0].get_token().get_id()
                    && state->f_choices == &r[0].get_choices())
                    {
                        parser_state *s(parser_state::alloc(free_states, state->f_parent, *state->f_choices, i));
                        //parser_state *s(parser_state::copy(free_states, state));
                        s->f_line = state->f_line;
                        s->add_node(state->f_node);
                        current.push_back(s);
//std::cerr << "** sub-next_token (recursive) " << reinterpret_cast<void*>(s) << "\n";
                        next_token(s, current, free_states); // we just reduced that one state!
//std::cerr << "**\n";
                    }
                }

                parser_state *p(state->f_parent);
                if(p->f_children.size() > 1)
                {
                    // the parent has several children which means we may get
                    // more than one reduce... to support that possibility
                    // duplicate the parent now
                    parser_state *new_parent(parser_state::copy(free_states, p));
                    p = new_parent;
//std::cerr << "    copy " << reinterpret_cast<void*>(state) << " to " << reinterpret_cast<void*>(p) << "\n";
                }
                p->add_node(state->f_node);

                // remove this state from the current set of rules
//std::cerr << "XXX delete " << reinterpret_cast<void*>(state) << " (parent: " << reinterpret_cast<void*>(p) << ")\n";
                parser_state::free(current, free_states, state);

                // continue with the parent which will get its
                // position increased on the next iteration
                state = p;
                current.push_back(state);
            }
            else
            {
                // forget about that state; we're reducing it for the second time?!
//std::cerr << ">>>>>>>>>>>>>>>>>>>> delete on > count (double reduce) " << reinterpret_cast<void*>(state) << "\n";
                parser_state::free(current, free_states, state);
            }
        }
        // else -- the user is not finished with this state
    }
    while(repeat);
//std::cerr << "next_token() returns with: " << (*state->f_choices)[state->f_rule].to_string() << "\n";

//std::cerr << "NEXT TOKEN: =================================================================\n";
//parser_state::display_array(current);
}

bool grammar::parse(lexer & input, choices & start)
{
    // the result of the parser against the lexer is a tree of tokens
    //
    // to run the parser, we need a state, this can be defined locally
    // because we do not need it in the result;
    //
    // create the root rule
    choices root(this, "root");
    root >>= start >> TOKEN_ID_NONE;
    // TODO: all the state pointers leak if we throw...
    parser_state * s(new parser_state(nullptr, root, 0));
    s->f_line = 1;

    state_array_t free_states;
    state_array_t current;
    current.push_back(s);
    while(!current.empty())
    {
        uint32_t const line(input.line());

        // we're working on the 'check' vector which is
        // a copy of the current vector so the current
        // vector can change in size
#ifdef DEBUG
//SNAP_LOG_TRACE("B: ================================================================= (line: ")(input.line())(")");
//parser_state::display_array(current);
#endif

        bool retry;
        do
        {
            retry = false;
            state_array_t check(current);
            for(state_array_t::const_iterator it(check.begin());
                            it != check.end(); ++it)
            {
                // it is a state, check whether the current entry
                // is a token or a rule
                parser_state *state(*it);
                const rule::rule_ref ref((*state->f_choices)[state->f_rule][state->f_position]);
                token_t token_id(ref.get_token().get_id());

                // only take care of choices in this loop (terminators are
                // handled in the next loop)
                if(token_id == token_t::TOKEN_ID_CHOICES_ENUM)
                {
                    // follow the choice by adding all of the rules it points to
                    choices * c(&ref.get_choices());

                    int const max_choices(c->count());
                    for(int r(0); r < max_choices; ++r)
                    {
                        rule::rule_ref const child_ref((*c)[r][0]);

                        // recursive?
                        if(token_t::TOKEN_ID_CHOICES_ENUM == child_ref.get_token().get_id()
                        && &child_ref.get_choices() == c)
                        {
                            // ignore recursive at this level, we take them
                            // in account when reducing instead
//std::cerr << "  SKIP RECURSIVE -- " << c->name() << "  --> " << (*c)[r].to_string() << "\n";
                            continue;
                        }
                        parser_state * child(parser_state::alloc(free_states, state, *c, r));
                        child->f_line = line;
//std::cerr << "  " << c->name() << "  --> " << (*c)[r].to_string() << "\n";

                        // check whether this is recursive; very important
                        // to avoid infinite loop; recurvise rules are used
                        // only when the concern rule gets reduced
                        // the child position is always 0 here (it's a new child)
                        bool recursive(false);
//                        token_t const child_token_id(child_ref.get_token().get_id());
//                        if(child_token_id == token_t::TOKEN_ID_CHOICES_ENUM)
//                        {
//                            // if the new child state starts with a 'choices'
//                            // and that's a 'choices' we already added
//                            // (including this very child,) then
//                            // that child is recursive
//                            choices *child_choices(&child_ref.get_choices());
//std::cerr << "  --> follow choice " << c->name() << " with sub-choice " << child_choices->name() << "\n";
//                            // start from ourselves
//                            int i(0);
//                            for(parser_state *p(child); p != nullptr && i < 2; p = p->f_parent, ++i)
//                            {
//                                if(child_choices == p->f_choices)
//                                {
//                                    if(p->f_parent == nullptr)
//                                    {
//                                        throw snap_logic_exception("invalid recursion (root cannot be recursive)");
//                                    }
//                                    // p may be ourselves so we cannot put that
//                                    // there, use the parent instead
//std::cerr << "  *** CHANGED TO REDUCE ***\n";
//                                    p->f_parent->f_add_on_reduce.push_back(child);
//                                    recursive = true;
//                                    break;
//                                }
//
//                                // cannot reduce any more than that if
//                                // this rule is not at the end of list of
//                                // choices
//                                //if(p->f_position + 1 < (*p->f_choices)[p->f_rule].count())
//                                //{
//                                //    // TODO: this is not correct if the
//                                //    //       following rule(s) support EMPTY
//                                //    break;
//                                //}
//                            }
//                        }

                        // if recursive it was already added to all the
                        // states where it needs to be; otherwise we add it
                        // to the current stack
                        if(!recursive)
                        {
                            current.push_back(child);
                        }
                    }
                    current.remove(current.indexOf(state));
                    retry = true;
                }
                else if(token_id == token_t::TOKEN_ID_EMPTY_ENUM)
                {
                    // we have to take care of empty rules here since anything
                    // coming after an empty rule has to be added to the list
                    // of rules here (it is very important because of the
                    // potential for recursive rules)
                    token t(token_t::TOKEN_ID_EMPTY_ENUM);
                    state->add_token(t);
                    next_token(state, current, free_states);
                    retry = true;
                }
            }
        } while(retry);
#ifdef DEBUG
//std::cerr << "A: ================================================================= (line: " << input.line() << ")\n";
//parser_state::display_array(current);
#endif

        // get the first token
        token t(input.next_token());
#ifdef DEBUG
//std::cerr << ". token type: " << t.to_string() << " to try against \n";
#endif

        state_array_t check(current);
#ifdef DEBUG
        // lock all those states to make sure we don't delete the wrong one
        for(state_array_t::const_iterator it(check.begin());
                        it != check.end(); ++it)
        {
            (*it)->lock();
        }
#endif
        for(state_array_t::const_iterator it(check.begin());
                        it != check.end(); ++it)
        {
            // it is a state, check whether the current entry
            // is a token or a rule
            parser_state *state(*it);
            rule::rule_ref const ref((*state->f_choices)[state->f_rule][state->f_position]);
            token_t const token_id(ref.get_token().get_id());
            if(token_id == token_t::TOKEN_ID_CHOICES_ENUM
            || token_id == token_t::TOKEN_ID_EMPTY_ENUM)
            {
                throw snap_logic_exception("this should never happen since the previous for() loop removed all of those!");
            }
            else
            {
                bool remove(false);
                if(t.get_id() != token_id)
                {
                    remove = true;
                }
                else
                {
                    switch(token_id)
                    {
                    case token_t::TOKEN_ID_LITERAL_ENUM:
                        // a literal must match exactly
                        if(t.get_value().toString() != ref.get_value())
                        {
                            remove = true;
                        }
                        break;

                    case token_t::TOKEN_ID_KEYWORD_ENUM:
                        // a keyword must match exactly
                        if(t.get_value().toInt() != ref.get_keyword().number())
                        {
                            remove = true;
                        }
                        break;

                    case token_t::TOKEN_ID_IDENTIFIER_ENUM:
                    case token_t::TOKEN_ID_STRING_ENUM:
                    case token_t::TOKEN_ID_INTEGER_ENUM:
                    case token_t::TOKEN_ID_FLOAT_ENUM:
                        // this is a match whatever the value
                        break;

                    case token_t::TOKEN_ID_NONE_ENUM:
                        // this state is the root state, this means the result
                        // is really the child node of this current state
                        //
                        f_result = qSharedPointerDynamicCast<token_node, token>((*state->f_node)[0]);
                        return true;

                    default:
                        // at this point other tokens are rejected here
                        throw snap_parser_unexpected_token(QString("unexpected token %1").arg(static_cast<int>(token_id)));

                    }
                }
#ifdef DEBUG
                state->unlock();
#endif
                if(remove)
                {
//std::cerr << "<*> delete unmatched state: " << state->f_choices->name() << "\n";
                    parser_state::free(current, free_states, state);
                }
                else
                {
                    // save this token as it was accepted
                    state->add_token(t);
//std::cerr << ">>> next token (IN)\n";
                    next_token(state, current, free_states);
//std::cerr << ">>> next token (OUT)\n";
                }
            }
        }
    }

    return false;
}



} // namespace parser
} // namespace snap

// vim: ts=4 sw=4 et
