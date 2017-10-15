// Snap Websites Server -- test the date to string capability
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

#include <snapwebsites/snap_child.h>

#include <iostream>

struct date_t
{
    char const *    f_string;
    time_t          f_time;
};


date_t dates[] =
{
    { "Sun, 06 Nov 1994 08:49:37 GMT", 784111777 },
    { "Sunday, 06-Nov-94 08:49:37 GMT", 784111777 },
    { "Sun Nov  6 08:49:37 1994", 784111777 },
    { "06 Nov 1994 08:49:37 GMT", 784111777 },
    { "06-Nov-94 08:49:37 GMT", 784111777 },
    { "Nov  6 08:49:37 1994", 784111777 },
    { "Sun, 06 November 1994 08:49:37 GMT", 784111777 },
    { "Sunday, 06-November-94 08:49:37 GMT", 784111777 },
    { "Sun November  6 08:49:37 1994", 784111777 },
    { "06 November 1994 08:49:37 GMT", 784111777 },
    { "06-November-94 08:49:37 GMT", 784111777 },
    { "November  6 08:49:37 1994", 784111777 }
};


int main(int, char *[])
{
    size_t const max_dates(sizeof(dates) / sizeof(dates[0]));
    for(size_t i(0); i < max_dates; ++i)
    {
        std::cout << "--- Test date " << dates[i].f_string << std::endl;
        try
        {
            time_t const unix_time(snap::snap_child::string_to_date(dates[i].f_string));
            if(unix_time != dates[i].f_time)
            {
                std::cerr << "error: date \"" << dates[i].f_string << "\" returned " << unix_time << " expected " << dates[i].f_time << std::endl;
                return 1;
            }
        }
        catch(snap::snap_logic_exception const & e)
        {
            std::cerr << "error: date \"" << dates[i].f_string << "\" generated an exception: " << e.what() << std::endl;
            return 1;
        }
    }
    //printf("%ld\n", unix_time);

    //struct tm time_info;
    //gmtime_r(&unix_time, &time_info);
    //char buf[256];
    //strftime(buf, sizeof(buf) - 1, "%a, %d %b %Y %H:%M:%S GMT", &time_info);
    //buf[sizeof(buf) - 1] = '\0';
    //printf("%s\n", buf);

    //Sun, 06 Nov 1994 08:49:37 GMT  ; RFC 822, updated by RFC 1123
    //Sunday, 06-Nov-94 08:49:37 GMT ; RFC 850, obsoleted by RFC 1036
    //Sun Nov  6 08:49:37 1994       ; ANSI C's asctime() format

    return 0;
}

// vim: ts=4 sw=4 et
