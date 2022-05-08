// Copyright (c) 2011-2019  Made to Order Software Corp.  All Rights Reserved
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


// snapwebsites
//
#include    <snapwebsites/snapwebsites.h>


// libexcept
//
#include    <libexcept/exception.h>


// snaplogger
//
#include    <snaplogger/message.h>


// snapdev
//
#include    <snapdev/not_reached.h>


// last include
//
#include    <snapdev/poison.h>



int main(int argc, char *argv[])
{
    if(argc == 2)
    {
        QByteArray buffer(argv[1], strlen(argv[1]));
        QByteArray base64(buffer.toBase64());
        std::cerr << base64.data() << "\n";
        return 0;
    }

    if(argc == 3)
    {
        if(strcmp(argv[1], "-d") == 0)
        {
            QByteArray base64(argv[2], strlen(argv[2]));
            QByteArray buffer(QByteArray::fromBase64(base64));
            std::cerr << buffer.data() << "\n";
            return 0;
        }
    }

    std::cerr << "Usage: " << argv[0] << " <username:password> | -d <base64>\n";
    return 1;
}

// vim: ts=4 sw=4 et
