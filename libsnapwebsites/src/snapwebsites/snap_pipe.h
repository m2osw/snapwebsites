// Snap Websites Server -- wrapper of popen()/pclose() with iostream like functions
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
#pragma once

#include "snapwebsites/snap_thread.h"

#include <sstream>
#include <stdio.h>

#include <QVector>

namespace snap
{

class snap_pipe_exception : public snap_exception
{
public:
    explicit snap_pipe_exception(const char *        whatmsg) : snap_exception("snap_pipe", whatmsg) {}
    explicit snap_pipe_exception(const std::string & whatmsg) : snap_exception("snap_pipe", whatmsg) {}
    explicit snap_pipe_exception(const QString &     whatmsg) : snap_exception("snap_pipe", whatmsg) {}
};

class snap_pipe_exception_cannot_open : public snap_pipe_exception
{
public:
    explicit snap_pipe_exception_cannot_open(const char *        whatmsg) : snap_pipe_exception(whatmsg) {}
    explicit snap_pipe_exception_cannot_open(const std::string & whatmsg) : snap_pipe_exception(whatmsg) {}
    explicit snap_pipe_exception_cannot_open(const QString &     whatmsg) : snap_pipe_exception(whatmsg) {}
};

class snap_pipe_exception_cannot_write : public snap_pipe_exception
{
public:
    explicit snap_pipe_exception_cannot_write(const char *        whatmsg) : snap_pipe_exception(whatmsg) {}
    explicit snap_pipe_exception_cannot_write(const std::string & whatmsg) : snap_pipe_exception(whatmsg) {}
    explicit snap_pipe_exception_cannot_write(const QString &     whatmsg) : snap_pipe_exception(whatmsg) {}
};

class snap_pipe_exception_cannot_read : public snap_pipe_exception
{
public:
    explicit snap_pipe_exception_cannot_read(const char *        whatmsg) : snap_pipe_exception(whatmsg) {}
    explicit snap_pipe_exception_cannot_read(const std::string & whatmsg) : snap_pipe_exception(whatmsg) {}
    explicit snap_pipe_exception_cannot_read(const QString &     whatmsg) : snap_pipe_exception(whatmsg) {}
};




class snap_pipe : public std::streambuf
{
public:
    enum class mode_t
    {
        PIPE_MODE_IN,   // you will be writing to the command (<<)
        PIPE_MODE_OUT   // you will be reading from the command (>>)
    };

                                snap_pipe(QString const & command, mode_t mode);
                                ~snap_pipe();

    // prevent copies
                                snap_pipe(snap_pipe const & ) = delete;
    snap_pipe &                 operator = (snap_pipe const & ) = delete;

    int                         close_pipe();
    QString const &             get_command() const;
    mode_t                      get_mode() const;

protected:
    virtual int_type            overflow(int_type c) override;
    virtual int_type            underflow() override;

private:
    // you are not expected to use those
    //int                         write(char const *buf, size_t size);
    //int                         read(char *buf, size_t size);

    QString                     f_command;
    mode_t                      f_mode = mode_t::PIPE_MODE_IN;
    FILE *                      f_file = nullptr;
};


} // namespace snap
// vim: ts=4 sw=4 et
