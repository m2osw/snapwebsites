// Snap Websites Server -- snap websites CGI function tests
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

#include <snapwebsites/not_reached.h>
#include <snapwebsites/tcp_client_server.h>

#include <map>
#include <string>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>



void usage()
{
    fprintf(stderr, "Usage: test_ip_strings [-opt] URL\n");
    fprintf(stderr, "  where -opt is one of (each flag must appear separately):\n");
    fprintf(stderr, "    -h                   Print out this help screen\n");
    exit(1);
}


int main(int argc, char * argv[])
{
    int errcnt(0);

    // parse user options
    bool help(false);
    for(int i(1); i < argc; ++i)
    {
        if(argv[i][0] == '-')
        {
            switch(argv[i][1])
            {
            case 'h':
                usage();
                snap::NOTREACHED();

            default:
                fprintf(stderr, "error: unknown option '%c'.\n", argv[i][1]);
                help = true;
                break;

            }
        }
    }
    if(help)
    {
        usage();
        snap::NOTREACHED();
    }

    // test various IPv4
    char const * ipv4[] =
    {
        "127.0.0.1",
        "192.168.0.0",
        "255.255.255.255",
        "0.0.0.0",
        "0",
        "255.0xffffff",
        "12.0xFFFFFF",
        "10.3.0XFFFF",
        "10.3.0377.0377",
        "10.3.0177777"
    };
    size_t const max_ipv4(sizeof(ipv4) / sizeof(ipv4[0]));
    for(size_t idx(0); idx < max_ipv4; ++idx)
    {
        if(!tcp_client_server::is_ipv4(ipv4[idx]))
        {
            std::cerr << ipv4[idx] << " is not considered a valid IPv4 address." << std::endl;
            ++errcnt;
        }
    }

    // test various IPv6
    char const * ipv6[] =
    {
        "::127.0.0.1",
        "::ffff:192.168.0.0",
        "abc:034a:f00f:22::134d",
        "1000:1000:1000:1000:1000:1000:1000:1000",
        "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff",
        "::1",
        "3a::1",
        "abcd:ef::33:123f",
        "ffff:ffff:ffff:ffff:ffff:ffff:10.3.177.77"
    };
    size_t const max_ipv6(sizeof(ipv6) / sizeof(ipv6[0]));
    for(size_t idx(0); idx < max_ipv6; ++idx)
    {
        if(!tcp_client_server::is_ipv6(ipv6[idx]))
        {
            std::cerr << ipv6[idx] << " is not considered a valid IPv6 address." << std::endl;
            ++errcnt;
        }
    }

    return errcnt > 0 ? 1 : 0;
}

// vim: ts=4 sw=4 et
