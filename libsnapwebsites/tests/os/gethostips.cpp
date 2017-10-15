//===============================================================================
// Copyright (c) 2017 by Made to Order Software Corporation
// 
// http://snapwebsites.org/
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
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//===============================================================================

#include <iostream>

#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main(int argc, char * argv[])
{
	if(argc == 1)
	{
		std::cerr << "usage: gethostips <name>" << std::endl;
		exit(1);
	}

	struct hostent * h (gethostbyname(argv[1]));

	std::cerr << "got name \"" << h->h_name << "\", type " << h->h_addrtype << ", length " << h->h_length << std::endl;

	for(char **a(h->h_addr_list); *a != nullptr; ++a)
	{
		char buf[256];
		std::cerr << "  IP: " << inet_ntop(h->h_addrtype, *a, buf, sizeof(buf)) << "\n";
	}

	return 0;
}

