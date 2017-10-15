// Snap Websites Server -- C++ object to safely handle a pipe
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

#include "snapwebsites/snap_pipe.h"

#include "snapwebsites/qstring_stream.h"

#include "snapwebsites/poison.h"


namespace snap
{



snap_pipe::snap_pipe(QString const & command, mode_t mode)
    : f_command(command)
    , f_mode(mode)
    , f_file(popen(f_command.toUtf8().data(), mode_t::PIPE_MODE_IN == mode ? "w" : "r"))
{
    if(f_file == nullptr)
    {
        throw snap_pipe_exception_cannot_open(QString("popen(\"%1\", \"%2\" failed to start command")
                                                .arg(f_command).arg(mode == mode_t::PIPE_MODE_IN ? "w" : "r"));
    }
}


snap_pipe::~snap_pipe()
{
    // make sure f_file gets closed
    close_pipe();
}


QString const& snap_pipe::get_command() const
{
    return f_command;
}


snap_pipe::mode_t snap_pipe::get_mode() const
{
    return f_mode;
}


int snap_pipe::close_pipe()
{
    int r(-1);
    if(f_file != nullptr)
    {
        if(ferror(f_file))
        {
            // must return -1 on error
            pclose(f_file);
        }
        else
        {
            r = pclose(f_file);
        }
        f_file = nullptr;
    }
    return r;
}


std::ostream::int_type snap_pipe::overflow(int_type c)
{
#ifdef DEBUG
    if(mode_t::PIPE_MODE_IN != f_mode)
    {
        throw snap_pipe_exception_cannot_write("pipe opened in read mode, cannot write to it");
    }
#endif

    if(c != EOF)
    {
        if(fputc(static_cast<int>(c), f_file) == EOF)
        {
            return EOF;
        }
    }
    return c;
}


std::ostream::int_type snap_pipe::underflow()
{
#ifdef DEBUG
    if(mode_t::PIPE_MODE_OUT != f_mode)
    {
        throw snap_pipe_exception_cannot_read("pipe opened in write mode, cannot read from it");
    }
#endif

    int c(fgetc(f_file));
    if(c < 0 || c > 255)
    {
        throw snap_pipe_exception_cannot_read(QString("snap_pipe::underflow(): fgetc() returned an error (%1)").arg(c));
    }

    return static_cast<std::ostream::int_type>(c);
}


//int snap_pipe::write(char const *buf, size_t size)
//{
//    return fwrite(buf, 1, size, f_file);
//}
//
//
//int snap_pipe::read(char *buf, size_t size)
//{
//    return fread(buf, 1, size, f_file);
//}


} // namespace snap

// vim: ts=4 sw=4 et
