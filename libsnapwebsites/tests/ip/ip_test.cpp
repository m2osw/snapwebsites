// Snap Websites Server -- manage the snapbackend settings
// Copyright (C) 2016-2017  Made to Order Software Corp.
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

#include "snapwebsites/addr.h"
#include "snapwebsites/not_used.h"

#include <iostream>

using namespace snap;
using namespace snap_addr;
using namespace std;

int main( int argc, char *argv[] )
{
    NOTUSED(argc);
    NOTUSED(argv);

    try
    {
        addr::vector_t address_list( addr::get_local_addresses() );

        for( const auto& addr : address_list )
        {
            cout << "Interface name: " << addr.get_iface_name() << endl;

            cout << "Network type: ";
            switch( addr.get_network_type() )
            {
                case addr::network_type_t::NETWORK_TYPE_UNDEFINED  : cout << "Undefined";  break;
                case addr::network_type_t::NETWORK_TYPE_PRIVATE    : cout << "Private";    break;
                case addr::network_type_t::NETWORK_TYPE_CARRIER    : cout << "Carrier";    break;
                case addr::network_type_t::NETWORK_TYPE_LINK_LOCAL : cout << "Local Link"; break;
                case addr::network_type_t::NETWORK_TYPE_MULTICAST  : cout << "Multicast";  break;
                case addr::network_type_t::NETWORK_TYPE_LOOPBACK   : cout << "Loopback";   break;
                case addr::network_type_t::NETWORK_TYPE_ANY        : cout << "Any";        break;
                case addr::network_type_t::NETWORK_TYPE_UNKNOWN    : cout << "Unknown";    break;
            }
            cout << endl;

            std::string const ip_string( addr.get_ipv4or6_string() );
            cout << "IP address: " << ip_string;

            if( addr.is_ipv4() )
            {
                cout << " (ipv4)";
            }
            else
            {
                cout << " (ipv6)";
            }
            cout << endl << endl;
        }

        return 0;
    }
    catch(snap::snap_exception const & e)
    {
        std::cerr << "error: a Snap! exception occurred. " << e.what() << std::endl;
    }

    return 1;
}

// vim: ts=4 sw=4 et
