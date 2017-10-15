// Snap Websites Server -- test the quoted_printable::encode()/decode() functions
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

#include <snapwebsites/quoted_printable.h>

int main(int argc, char *argv[])
{
    for(int i(1); i < argc; ++i)
    {
        printf("testing [%s]\n", argv[i]);
        std::string a(quoted_printable::encode(argv[i]));
        printf("  +--> encoded as [%s]\n", a.c_str());
        std::string b(quoted_printable::decode(a));
        printf("  +--> decoded as [%s] (%s)\n", b.c_str(), argv[i] == b ? "equal" : "changed");
    }

    return 0;
}

// vim: ts=4 sw=4 et
