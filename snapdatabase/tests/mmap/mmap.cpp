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

// Bloomfilter evaluation


#include <fcntl.h>
#include <iostream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>


void show(char *s)
{
    for(; *s != '\n'; ++s)
    {
        std::cerr << *s;
    }
    std::cerr << "\n";
}


int main()
{
    std::cerr << "---- open file\n";
    int fd = open("/etc/passwd", O_RDONLY);
    if(fd == -1)
    {
        std::cerr << "error: could not open file.\n";
        return 1;
    }

    std::cerr << "---- mmap(" << fd << ")\n";
    char * ptr(reinterpret_cast<char *>(mmap(
          nullptr
        , 200
        , PROT_READ
        , MAP_SHARED
        , fd
        , 0)));

    std::cerr << "---- show start\n";
    std::cerr << "---- ptr: " << reinterpret_cast<void *>(ptr) << "\n";
    show(ptr);

    std::cerr << "---- mmap() AGAIN\n";
    ptr = (reinterpret_cast<char *>(mmap(
          nullptr
        , 200
        , PROT_READ
        , MAP_SHARED
        , fd
        , 0)));

    std::cerr << "---- show AGAIN\n";
    show(ptr);

    std::cerr << "---- munmap() ...\n";
    munmap(ptr, 200);

    std::cerr << "---- show after unmap() we should SEGV now\n";
    show(ptr);

    std::cerr << "---- you should NOT see this message\n";

    return 0;
}



// vim: ts=4 sw=4 et
