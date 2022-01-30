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


/** \file
 * \brief Database file implementation.
 *
 * Each table uses one or more files. Each file is handled by a dbfile
 * object and a corresponding set of blocks.
 */

// self
//
#include    "snapdatabase/data/xml.h"

#include    "snapdatabase/data/convert.h"
#include    "snapdatabase/exception.h"


// libutf8 lib
//
#include    <libutf8/libutf8.h>


// snapdev lib
//
#include    <snapdev/not_reached.h>


// boost lib
//
#include    <boost/algorithm/string.hpp>


// C++ lib
//
#include    <fstream>
#include    <iostream>


// C lib
//
#include    <string.h>


// last include
//
#include    <snapdev/poison.h>



namespace snapdatabase
{



namespace
{


bool is_alpha(char c)
{
    return (c >= 'a' && c <= 'z')
        || (c >= 'A' && c <= 'Z')
        || c == '_';
}


bool is_digit(char c)
{
    return (c >= '0' && c <= '9')
        || c == '-';
}


bool is_space(char c)
{
    return c == ' '
        || c == '\t'
        || c == '\v'
        || c == '\f'
        || c == '\n'
        || c == '\r';
}


bool is_token(std::string const s)
{
    if(s.empty())
    {
        return false;
    }

    if(!is_alpha(s[0]))
    {
        return false;
    }

    std::string::size_type const max(s.length());
    for(std::string::size_type idx(1); idx < max; ++idx)
    {
        char const c(s[idx]);
        if(!is_alpha(c)
        && !is_digit(c)
        && c != '-')
        {
            return false;
        }
    }
    if(s[max - 1] == '-')
    {
        return false;
    }

    return true;
}



enum class token_t
{
    TOK_CLOSE_TAG,
    TOK_EMPTY_TAG,
    TOK_END_TAG,
    TOK_EOF,
    TOK_EQUAL,
    TOK_IDENTIFIER,
    TOK_OPEN_TAG,
    TOK_PROCESSOR,
    TOK_STRING,
    TOK_TEXT
};


class xml_parser
{
public:
                        xml_parser(std::string const & filename, xml_node::pointer_t & root);

private:
    void                read_xml(xml_node::pointer_t & root);
    token_t             get_token(bool parsing_attributes);
    void                unescape_entities();
    int                 getc();
    void                ungetc(int c);

    std::string         f_filename = std::string();
    std::ifstream       f_in;
    size_t              f_ungetc_pos = 0;
    int                 f_ungetc[4] = { '\0' };
    int                 f_line = 1;
    std::string         f_value = std::string();
};


xml_parser::xml_parser(std::string const & filename, xml_node::pointer_t & root)
    : f_filename(filename)
    , f_in(filename)
{
    if(!f_in.is_open())
    {
        int const e(errno);
        throw file_not_found(std::string("Could not open XML table file \"")
                           + filename
                           + "\": " + strerror(e) + ".");
    }

    read_xml(root);
}


/** \brief
 *
 * This function reads the XML but it does not verify the Schema format.
 * It does verify the XML syntax fairly strongly.
 *
 * \param[in] root  A reference to the root pointer where the results are saved.
 */
void xml_parser::read_xml(xml_node::pointer_t & root)
{
    token_t tok(get_token(false));

    auto skip_empty = [&]()
    {
        while(tok == token_t::TOK_TEXT)
        {
            boost::trim(f_value);
            if(!f_value.empty())
            {
                throw unexpected_token(
                          "File \""
                        + f_filename
                        + "\" cannot include text data before the root tag.");
            }
            tok = get_token(false);
        }
    };

    auto read_tag_attributes = [&](xml_node::pointer_t & tag)
    {
        for(;;)
        {
            tok = get_token(true);
            if(tok == token_t::TOK_END_TAG
            || tok == token_t::TOK_EMPTY_TAG)
            {
                return tok;
            }
            if(tok != token_t::TOK_IDENTIFIER)
            {
                throw invalid_xml("Expected the end of the tag (>) or an attribute name.");
            }
            std::string const name(f_value);
            tok = get_token(true);
            if(tok != token_t::TOK_EQUAL)
            {
                throw invalid_xml("Expected the '=' character between the attribute name and value.");
            }
            tok = get_token(true);
            if(tok != token_t::TOK_STRING)
            {
                throw invalid_xml("Expected a value of the attribute after the '=' sign.");
            }
            if(!tag->attribute(name).empty())
            {
                throw invalid_xml("Attribute \"" + name + "\" defined twice. We do not allow such.");
            }
            tag->set_attribute(name, f_value);
        }
    };

    skip_empty();
    if(tok == token_t::TOK_PROCESSOR)
    {
        tok = get_token(false);
    }
    skip_empty();

    // now we have to have the root tag
    if(tok != token_t::TOK_OPEN_TAG)
    {
        throw unexpected_token(
                  "File \""
                + f_filename
                + "\" cannot be empty or include anything other than a processor tag and comments before the root tag.");
    }
    root = std::make_shared<xml_node>(f_value);
    if(read_tag_attributes(root) == token_t::TOK_EMPTY_TAG)
    {
        throw unexpected_token(
                  "File \""
                + f_filename
                + "\" root tag cannot be an empty tag.");
    }
    tok = get_token(false);

    xml_node::pointer_t parent(root);
    while(tok != token_t::TOK_EOF)
    {
        switch(tok)
        {
        case token_t::TOK_OPEN_TAG:
            {
                xml_node::pointer_t child(std::make_shared<xml_node>(f_value));
                parent->append_child(child);
                if(read_tag_attributes(child) == token_t::TOK_END_TAG)
                {
                    parent = child;
                }
            }
            break;

        case token_t::TOK_CLOSE_TAG:
            if(parent->tag_name() != f_value)
            {
                throw unexpected_token(
                          "Unexpected token name \""
                        + f_value
                        + "\" in this closing tag. Expected \""
                        + parent->tag_name()
                        + "\" instead.");
            }
            parent = parent->parent();
            if(parent == nullptr)
            {
                for(;;)
                {
                    tok = get_token(false);
                    switch(tok)
                    {
                    case token_t::TOK_EOF:
                        // it worked, we're done
                        //
                        return;

                    case token_t::TOK_TEXT:
                        skip_empty();
                        break;

                    case token_t::TOK_PROCESSOR:
                        // completely ignore those
                        break;

                    default:
                        throw unexpected_token(
                                  "We reached the end of the XML file, but still found a token of type "
                                + std::to_string(static_cast<int>(tok))
                                + " instead of the end of the file.");

                    }
                }
            }
            break;

        case token_t::TOK_TEXT:
            parent->append_text(f_value);
            break;

        case token_t::TOK_EOF:
        case token_t::TOK_EMPTY_TAG:
        case token_t::TOK_END_TAG:
        case token_t::TOK_EQUAL:
        case token_t::TOK_IDENTIFIER:
        case token_t::TOK_PROCESSOR:
        case token_t::TOK_STRING:
            throw snapdatabase_logic_error("Received an unexpected token in the switch handler.");

        }
        tok = get_token(false);
    }
}


token_t xml_parser::get_token(bool parsing_attributes)
{
    f_value.clear();

    for(;;)
    {
        int c(getc());
        switch(c)
        {
        case EOF:
            return token_t::TOK_EOF;

        case ' ':
        case '\t':
        case '\v':
        case '\f':
        case '\n':
            if(parsing_attributes)
            {
                continue;
            }
            break;

        case '<':
            c = getc();
            switch(c)
            {
            case '?':
                // we do not parse the processor entry, we do not care about
                // it at the moment
                for(;;)
                {
                    c = getc();
                    if(c == EOF)
                    {
                        throw unexpected_eof("Found an unexpected sequence of character in a processor (\"<?...?>\") sequence.");
                    }
                    while(c == '?')
                    {
                        c = getc();
                        if(c == '>')
                        {
                            return token_t::TOK_PROCESSOR;
                        }
                        f_value += '?';
                    }
                    f_value += static_cast<char>(c);
                }
                snapdev::NOT_REACHED();
                return token_t::TOK_PROCESSOR;

            case '!':
                c = getc();
                if(is_alpha(c))
                {
                    // of course, this may be anything other than an element but still something we don't support
                    //
                    throw invalid_xml("Found an element definition (such as an \"<!ELEMENT...>\" sequence, which is not supported.");
                }
                if(c == '[')
                {
                    // <![CDATA[ ... or throw
                    //
                    char const * expected = "CDATA[";
                    for(int j(0); j < 6; ++j)
                    {
                        if(getc() != expected[j])
                        {
                            throw invalid_xml("Found an unexpected sequence of character in a \"<![CDATA[...\" sequence.");
                        }
                    }
                    for(;;)
                    {
                        c = getc();
                        if(c == EOF)
                        {
                            throw unexpected_eof("Found EOF while parsing a \"<![CDATA[...]]>\" sequence.");
                        }
                        if(c == ']')
                        {
                            c = getc();
                            if(c == ']')
                            {
                                c = getc();
                                while(c == ']')
                                {
                                    f_value += ']';
                                    c = getc();
                                }
                                if(c == '>')
                                {
                                    // this is just like some text
                                    // except we do not convert entities
                                    //
                                    return token_t::TOK_TEXT;
                                }
                                f_value += "]]";
                                f_value += static_cast<char>(c);
                            }
                            else
                            {
                                f_value += ']';
                                f_value += static_cast<char>(c);
                            }
                        }
                        else
                        {
                            f_value += static_cast<char>(c);
                        }
                    }
                }
                if(c == '-')
                {
                    c = getc();
                    if(c == '-')
                    {
                        bool found(false);
                        while(!found)
                        {
                            c = getc();
                            if(c == EOF)
                            {
                                throw unexpected_eof("Found EOF while parsing a comment (\"<!--...-->\") sequence.");
                            }
                            if(c == '-')
                            {
                                c = getc();
                                while(c == '-')
                                {
                                    c = getc();
                                    if(c == '>')
                                    {
                                        found = true;
                                        break;
                                    }
                                }
                            }
                        }
                        continue;
                    }
                }
                throw invalid_token(
                          std::string("Character '")
                        + static_cast<char>(c)
                        + "' was not expected after a \"<!\" sequence.");

            case '/':
                c = getc();
                while(is_space(c))
                {
                    c = getc();
                }
                if(!is_alpha(c))
                {
                    if(c == EOF)
                    {
                        throw unexpected_eof("Expected a tag name after \"</\", not EOF.");
                    }
                    throw invalid_token(
                              std::string("Character '")
                            + static_cast<char>(c)
                            + "' is not valid for a tag name.");
                }
                for(;;)
                {
                    f_value += static_cast<char>(c);
                    c = getc();
                    if(!is_alpha(c)
                    && !is_digit(c))
                    {
                        break;
                    }
                }
                while(is_space(c))
                {
                    c = getc();
                }
                if(c != '>')
                {
                    if(c == EOF)
                    {
                        throw unexpected_eof("Expected '>', not EOF.");
                    }
                    throw invalid_xml(
                              std::string("Found an unexpected '")
                            + static_cast<char>(c)
                            + "' in a closing tag, expected '>' instead.");
                }
                return token_t::TOK_CLOSE_TAG;

            }

            // in this case we need to read the name only, the attributes
            // will be read by the parser instead of the lexer
            //
            while(is_space(c))
            {
                c = getc();
            }
            if(!is_alpha(c))
            {
                if(c == EOF)
                {
                    throw unexpected_eof("Expected a tag name after \"</\", not EOF.");
                }
                throw invalid_token(
                          std::string("Character '")
                        + static_cast<char>(c)
                        + "' is not valid for a tag name.");
            }
            for(;;)
            {
                f_value += static_cast<char>(c);
                c = getc();
                if(!is_alpha(c)
                && !is_digit(c)
                && c != '-')
                {
                    break;
                }
            }
            if(isspace(c))
            {
                do
                {
                    c = getc();
                }
                while(isspace(c));
            }
            else if(c != '>' && c != '/')
            {
                throw invalid_token(
                          std::string("Character '")
                        + static_cast<char>(c)
                        + "' is not valid right after a tag name.");
            }
            ungetc(c);
            return token_t::TOK_OPEN_TAG;

        case '>':
            if(parsing_attributes)
            {
                return token_t::TOK_END_TAG;
            }
            break;

        case '/':
            if(parsing_attributes)
            {
                c = getc();
                if(c == '>')
                {
                    return token_t::TOK_EMPTY_TAG;
                }
                ungetc(c);
                c = '/';
            }
            break;

        case '=':
            if(parsing_attributes)
            {
                return token_t::TOK_EQUAL;
            }
            break;

        case '"':
        case '\'':
            if(parsing_attributes)
            {
                int quote(c);
                for(;;)
                {
                    c = getc();
                    if(c == quote)
                    {
                        unescape_entities();
                        return token_t::TOK_STRING;
                    }
                    if(c == '>')
                    {
                        throw invalid_token("Character '>' not expected inside a tag value. Please use \"&gt;\" instead.");
                    }
                    f_value += static_cast<char>(c);
                }
            }
            break;

        }

        if(parsing_attributes
        && is_alpha(c))
        {
            for(;;)
            {
                f_value += static_cast<char>(c);
                c = getc();
                if(!is_alpha(c)
                && !is_digit(c)
                && c != '-')
                {
                    ungetc(c);
                    return token_t::TOK_IDENTIFIER;
                }
            }
        }

        for(;;)
        {
            f_value += static_cast<char>(c);
            c = getc();
            if(c == '<'
            || c == EOF)
            {
                ungetc(c);
                unescape_entities();
                return token_t::TOK_TEXT;
            }
        }
    }
}


void xml_parser::unescape_entities()
{
    for(std::string::size_type pos(0);;)
    {
        pos = f_value.find('&', pos);
        if(pos == std::string::npos)
        {
            break;
        }
        std::string::size_type end(f_value.find(';', pos + 1));
        if(end == std::string::npos)
        {
            // generate an error here?
            //
            break;
        }
        std::string name(f_value.substr(pos + 1, end - pos - 1));
        if(name == "amp")
        {
            f_value.replace(pos, end - pos + 1, 1, '&');
            ++pos;
        }
        else if(name == "quot")
        {
            f_value.replace(pos, end - pos + 1, 1, '"');
            ++pos;
        }
        else if(name == "lt")
        {
            f_value.replace(pos, end - pos + 1, 1, '<');
            ++pos;
        }
        else if(name == "gt")
        {
            f_value.replace(pos, end - pos + 1, 1, '>');
            ++pos;
        }
        else if(name == "apos")
        {
            f_value.replace(pos, end - pos + 1, 1, '\'');
            ++pos;
        }
        else if(name.empty())
        {
            throw invalid_entity("the name of an entity cannot be empty ('&;' is not valid XML).");
        }
        else if(name[0] == '#')
        {
            if(name.length() == 1)
            {
                throw invalid_entity("a numeric entity must have a number ('&#; is not valid XML).");
            }
            if(name[1] == 'x'
            || name[1] == 'X')
            {
                name[0] = '0';
            }
            else
            {
                name[0] = ' ';
            }
            // TODO: enforce base 10 or 16
            //
            char32_t unicode(convert_to_int(name, 32));
            std::string const utf8(libutf8::to_u8string(unicode));
            f_value.replace(pos, end - pos + 1, utf8);
            pos += utf8.length();
        }
        else
        {
            throw invalid_entity(
                      "Unsupported entity ('&"
                    + name
                    + ";').");
        }
    }
}


int xml_parser::getc()
{
    if(f_ungetc_pos > 0)
    {
        --f_ungetc_pos;
//std::cerr << "re-getc() - '" << static_cast<char>(f_ungetc[f_ungetc_pos]) << "'\n";
        return f_ungetc[f_ungetc_pos];
    }

    int c(f_in.get());
    if(c == '\r')
    {
        ++f_line;
        c = f_in.get();
        if(c != '\n')
        {
            ungetc(c);
            c = '\n';
        }
    }
    else if(c == '\n')
    {
        ++f_line;
    }

//if(c == EOF)
//{
//std::cerr << "getc() - 'EOF'\n";
//}
//else
//{
//std::cerr << "getc() - '" << static_cast<char>(c) << "'\n";
//}
    return c;
}


void xml_parser::ungetc(int c)
{
    if(c != EOF)
    {
        if(f_ungetc_pos >= sizeof(f_ungetc) / sizeof(f_ungetc[0]))
        {
            throw snapdatabase_logic_error("Somehow the f_ungetc buffer was overflowed.");
        }

        f_ungetc[f_ungetc_pos] = c;
        ++f_ungetc_pos;
    }
}



} // empty namespace



xml_node::xml_node(std::string const & name)
    : f_name(name)
{
    if(!is_token(name))
    {
        throw invalid_token("\"" + name + "\" is not a valid token as a tag name.");
    }
}


std::string const & xml_node::tag_name() const
{
    return f_name;
}


std::string xml_node::text() const
{
    return f_text;
}


void xml_node::append_text(std::string const & text)
{
    f_text += text;
}


xml_node::attribute_map_t xml_node::all_attributes() const
{
    return f_attributes;
}


std::string xml_node::attribute(std::string const & name) const
{
    auto const it(f_attributes.find(name));
    if(it == f_attributes.end())
    {
        return std::string();
    }
    return it->second;
}


void xml_node::set_attribute(std::string const & name, std::string const & value)
{
    if(!is_token(name))
    {
        throw invalid_token("\"" + name + "\" is not a valid token as an attribute name.");
    }
    f_attributes[name] = value;
}


void xml_node::append_child(pointer_t n)
{
    if(n->f_next != nullptr
    || n->f_previous.lock() != nullptr)
    {
        throw node_already_in_tree("Somehow you are trying to add a child xml_node of a xml_node that was already added to a tree of nodes.");
    }

    auto l(last_child());
    if(l == nullptr)
    {
        f_child = n;
    }
    else
    {
        l->f_next = n;
        n->f_previous = l;
    }

    n->f_parent = shared_from_this();
}


xml_node::pointer_t xml_node::parent() const
{
    auto result(f_parent.lock());
    return result;
}


xml_node::pointer_t xml_node::first_child() const
{
    return f_child;
}


xml_node::pointer_t xml_node::last_child() const
{
    if(f_child == nullptr)
    {
        return xml_node::pointer_t();
    }

    pointer_t l(f_child);
    while(l->f_next != nullptr)
    {
        l = l->f_next;
    }

    return l;
}


xml_node::pointer_t xml_node::next() const
{
    return f_next;
}


xml_node::pointer_t xml_node::previous() const
{
    return f_previous.lock();
}


std::ostream & operator << (std::ostream & out, xml_node const & n)
{
    out << '<';
    out << n.tag_name();
    for(auto const & a : n.all_attributes())
    {
        out << a.first
            << "=\""
            << a.second
            << '"';
    }
    auto child(n.first_child());
    bool empty(child == nullptr);
    if(empty)
    {
        out << '/';
    }
    out << '>';
    if(!empty)
    {
        out << '\n';
        while(child != nullptr)
        {
            out << child;       // recursive call
            child = child->next();
        }
        out << '\n';
    }
    if(!n.text().empty())
    {
        out << n.text();
        if(!empty)
        {
            out << '\n';
        }
    }
    if(!empty)
    {
        out << "</"
            << n.tag_name()
            << '>';
    }

    return out;
}




xml::xml(std::string const & filename)
{
    xml_parser p(filename, f_root);
}


xml_node::pointer_t xml::root()
{
    return f_root;
}



} // namespace snapdatabase
// vim: ts=4 sw=4 et
