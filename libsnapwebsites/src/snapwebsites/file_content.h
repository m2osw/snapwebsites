// Snap Websites Servers -- handle file content (i.e. read all / write all)
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
#pragma once

#include "snapwebsites/snap_exception.h"

// C++ lib
//
#include <memory>
#include <vector>

namespace snap
{

class file_content_exception : public snap_exception
{
public:
    explicit file_content_exception(char const *        whatmsg) : snap_exception("file_content", whatmsg) {}
    explicit file_content_exception(std::string const & whatmsg) : snap_exception("file_content", whatmsg) {}
    explicit file_content_exception(QString const &     whatmsg) : snap_exception("file_content", whatmsg) {}
};

class file_content_exception_invalid_parameter : public file_content_exception
{
public:
    explicit file_content_exception_invalid_parameter(char const *        whatmsg) : file_content_exception(whatmsg) {}
    explicit file_content_exception_invalid_parameter(std::string const & whatmsg) : file_content_exception(whatmsg) {}
    explicit file_content_exception_invalid_parameter(QString const &     whatmsg) : file_content_exception(whatmsg) {}
};

class file_content_exception_io_error : public file_content_exception
{
public:
    explicit file_content_exception_io_error(char const *        whatmsg) : file_content_exception(whatmsg) {}
    explicit file_content_exception_io_error(std::string const & whatmsg) : file_content_exception(whatmsg) {}
    explicit file_content_exception_io_error(QString const &     whatmsg) : file_content_exception(whatmsg) {}
};







class file_content
{
public:
    typedef std::shared_ptr<file_content>   pointer_t;

                                file_content(std::string const & filename, bool create_missing_directories = false, bool temporary = false);
    virtual                     ~file_content();

    std::string const &         get_filename() const;
    bool                        exists() const;

    bool                        read_all();
    bool                        write_all(std::string const & filename = std::string());

    void                        set_content(std::string const & new_content);
    std::string const &         get_content() const;

protected:
    std::string                 f_filename = std::string();
    std::string                 f_content = std::string();
    bool                        f_temporary = false;
};



} // namespace snap
// vim: ts=4 sw=4 et
