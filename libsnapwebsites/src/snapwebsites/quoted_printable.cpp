// Snap Websites Servers -- quote a string so it is only ASCII (for emails)
// Copyright (C) 2013-2017  Made to Order Software Corp.
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

#include "snapwebsites/quoted_printable.h"

#include "snapwebsites/not_reached.h"

#include "snapwebsites/poison.h"


namespace quoted_printable
{

std::string encode(std::string const & input, int flags)
{
    class result
    {
    public:
        result(size_t input_length, int flags)
            : f_flags(flags)
            //, f_buffer('\0') -- auto-init
            //, f_result("") -- auto-init
            //, f_line(0) -- auto-init
            //, f_cr(false) -- auto-init
        {
            f_result.reserve(input_length * 2);
        }

        char to_hex(int c)
        {
            c &= 15;
            if(c >= 0 && c <= 9)
            {
                return static_cast<char>(c + '0');
            }
            return static_cast<char>(c + 'A' - 10);
        }

        void add_byte(char c)
        {
            if(c == '\n' || c == '\r')
            {
                if(c != '\r' || (f_flags & QUOTED_PRINTABLE_FLAG_LFONLY) == 0)
                {
                    f_result += c;
                }
                f_line = 0;
                return;
            }
            // the maximum line length is 76
            // it is not clear whether that includes the CR+LF or not
            // "=\r\n" is 3 characters
            if(f_line >= 75)
            {
                if((f_flags & QUOTED_PRINTABLE_FLAG_LFONLY) == 0)
                {
                    f_result += "=\r\n";
                }
                else
                {
                    f_result += "=\n";
                }
                f_line = 0;
            }
            f_result += c;
            ++f_line;
        }

        void add_hex(int c)
        {
            // make sure there is enough space on the current line before
            // adding the 3 encoded bytes
            if(f_line >= 73)
            {
                // IMPORTANT: we cannot call add_byte while adding
                // an '=' as character 75 otherwise it will add "=\r\n"!
                if((f_flags & QUOTED_PRINTABLE_FLAG_LFONLY) == 0)
                {
                    f_result += "=\r\n";
                }
                else
                {
                    f_result += "=\n";
                }
                f_line = 0;
            }
            add_byte('=');
            add_byte(to_hex(c >> 4));
            add_byte(to_hex(c));
        }

        void add_data(char c)
        {
            if(c == ' ' || c == '\t')
            {
                // we have to buffer the last space or tab because they
                // cannot appear as the last character on a line (i.e.
                // when followed by \r or \n)
                if(f_buffer != '\0')
                {
                    add_byte(f_buffer);
                }
                f_buffer = c;
                return;
            }
            if(c == '\r')
            {
                f_cr = true;
            }
            else if(c == '\n' && f_cr)
            {
                // CR+LF already taken cared of
                f_cr = false;
                return;
            }
            else
            {
                f_cr = false;
            }
            if(c == '\n' || c == '\r')
            {
                // spaces and tabs must be encoded in this case
                if(f_buffer != '\0')
                {
                    add_hex(f_buffer);
                    f_buffer = '\0';
                }
                // force the CR+LF sequence
                add_byte('\r');
                add_byte('\n');
                return;
            }
            if(f_buffer != '\0')
            {
                add_byte(f_buffer);
                f_buffer = '\0';
            }
            add_byte(c);
        }

        bool encode_char(char c)
        {
            switch(c)
            {
            case '\n':
            case '\r':
            case '\t':
            case ' ':
                return (f_flags & QUOTED_PRINTABLE_FLAG_BINARY) != 0;

            case '=':
                return true;

            case '!': // !"#$@[\]^`{|}~
            case '"':
            case '#':
            case '$':
            case '@':
            case '[':
            case '\\':
            case ']':
            case '^':
            case '`':
            case '{':
            case '|':
            case '}':
            case '~':
                return (f_flags & QUOTED_PRINTABLE_FLAG_EDBIC) != 0;

            default:
                // note: the following won't match ' ' and '~' which are
                //       already captured by other cases
                return !(c >= ' ' && c <= '~');

            }
            snap::NOTREACHED();
        }

        void add_char(char c)
        {
            // a few controls are allowed as is
            if(encode_char(c))
            {
                // needs to be encoded
                add_hex(c);
            }
            else
            {
                add_data(c);
            }
        }

        void add_string(char const * s)
        {
            bool const lone_periods((f_flags & QUOTED_PRINTABLE_FLAG_NO_LONE_PERIOD) != 0);
            // reset the buffer, just in case
            f_buffer = '\0';
            for(; *s != '\0'; ++s)
            {
                if(lone_periods
                && *s == '.'
                && (s[1] == '\r' || s[1] == '\n' || s[1] == '\0')
                && (f_line == 0 || f_line >= 75))
                {
                    // special case of a lone period at the start of a line
                    add_hex('.');
                }
                else
                {
                    add_char(*s);
                }
            }

            // at the end we may still have a space or tab to add
            if(f_buffer != '\0')
            {
                add_hex(f_buffer);
                f_buffer = '\0';
            }
        }

        std::string get_result() const
        {
            return f_result;
        }

    private:
        int32_t         f_flags = 0;
        char            f_buffer = 0;
        std::string     f_result;
        int32_t         f_line = 0;
        bool            f_cr = false;
    };

    result r(input.length(), flags);
    r.add_string(input.c_str());
    return r.get_result();
}


std::string decode(std::string const & input)
{
    class result
    {
    public:
        result(std::string const & input)
            //: f_result("") -- auto-initiazlied
            : f_input(input)
            , f_str(f_input.c_str())
        {
        }

        int from_hex(int c)
        {
            if(c >= '0' && c <= '9')
            {
                return c - '0';
            }
            // note that the documentation clearly says that only capitalized
            // (A-F) characters are acceptable...
            if(c >= 'a' && c <= 'f')
            {
                return c - 'a' + 10;
            }
            if(c >= 'A' && c <= 'F')
            {
                return c - 'A' + 10;
            }
            return -1;
        }

        int get_byte()
        {
            for(;;)
            {
                if(*f_str == '\0')
                {
                    return '\0';
                }
                if(*f_str == '=')
                {
                    if(f_str[1] == '\r')
                    {
                        if(f_str[2] == '\n')
                        {
                            f_str += 3;
                        }
                        else
                        {
                            f_str += 2;
                        }
                        continue;
                    }
                    if(f_str[1] == '\n')
                    {
                        f_str += 2;
                        continue;
                    }
                }
                return *f_str++;
            }
        }

        int getc()
        {
            int c(get_byte());
            if(c == '\0')
            {
                return '\0';
            }
            if(c == '=')
            {
                // all equal must be followed by a newline (taken care
                // off already) or a 2 hex digits
                c = get_byte();
                int const p(from_hex(c));
                if(p == -1)
                {
                    return '?';
                }
                c = get_byte();
                int const q(from_hex(c));
                if(q == -1)
                {
                    return '?';
                }
                return p * 16 + q;
            }
            return c;
        }

        void process()
        {
            for(int c(getc()); c != '\0'; c = getc())
            {
                f_result += static_cast<char>(c);
            }
        }

        std::string get_result() const
        {
            return f_result;
        }

    private:
        std::string     f_input;
        std::string     f_result;
        char const *    f_str = nullptr;
    };

    result r(input);
    r.process();
    return r.get_result();
}

} // namespace quoted_printable
// vim: ts=4 sw=4 et
