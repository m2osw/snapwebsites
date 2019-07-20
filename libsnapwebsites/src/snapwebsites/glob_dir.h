// Snap Websites Servers -- glob a directory and enumerate the files
// Copyright (c) 2016-2019  Made to Order Software Corp.  All Rights Reserved
//
// https://snapwebsites.org/
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
//
#pragma once

// snapwebsites lib
//
#include "snapwebsites/snap_exception.h"

// snapdev lib
//
#include <snapdev/raii_generic_deleter.h>

// Qt lib
//
#include <QString>

// C++ lib
//
#include <functional>
#include <memory>

// C lib
//
#include <glob.h>


namespace snap
{

class glob_dir_exception : public snap_exception
{
public:
    // no sub-name
    explicit glob_dir_exception(int const error_num, char const *        what_msg) : snap_exception(what_msg), f_error_num(error_num) {}
    explicit glob_dir_exception(int const error_num, std::string const & what_msg) : snap_exception(what_msg), f_error_num(error_num) {}
    explicit glob_dir_exception(int const error_num, QString const &     what_msg) : snap_exception(what_msg), f_error_num(error_num) {}

    int get_error_num() const { return f_error_num; }

private:
    int f_error_num = -1;
};


class glob_dir
{
public:
    typedef std::unique_ptr<glob_t, raii_pointer_deleter<glob_t, decltype(&::globfree), &::globfree>> glob_pointer_t;

                    glob_dir();
                    glob_dir( char const * path, int const flags = 0, bool allow_empty = false );
                    glob_dir( std::string const & path, int const flags = 0, bool allow_empty = false );
                    glob_dir( QString const & path, int const flags = 0, bool allow_empty = false );

    void            set_path( char const * path, int const flags = 0, bool allow_empty = false );
    void            set_path( std::string const & path, int const flags = 0, bool allow_empty = false );
    void            set_path( QString const & path, int const flags = 0, bool allow_empty = false );

    void            enumerate_glob( std::function<void (std::string path)> func ) const;
    void            enumerate_glob( std::function<void (QString path)> func ) const;

private:
    glob_pointer_t  f_dir = glob_pointer_t();
};

} // namespace snap
// vim: ts=4 sw=4 et
