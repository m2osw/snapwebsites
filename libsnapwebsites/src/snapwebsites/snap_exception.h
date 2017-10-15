// Snap Websites Server -- base exception of the Snap! library
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
#pragma once

#include <stdexcept>
#include <QString>

namespace snap
{

class snap_exception_base
{
public:
    static int const            STACK_TRACE_DEPTH = 20;

                                snap_exception_base();
    virtual                     ~snap_exception_base() {}

    static void                 output_stack_trace( int const stack_trace_depth = STACK_TRACE_DEPTH );
};



class snap_exception : public std::runtime_error, public snap_exception_base
{
public:
    // no sub-name
    explicit snap_exception(char const *        what_msg) : runtime_error("Snap! Exception: " + std::string(what_msg)) {}
    explicit snap_exception(std::string const & what_msg) : runtime_error("Snap! Exception: " + what_msg) {}
    explicit snap_exception(QString const &     what_msg) : runtime_error(("Snap! Exception: " + what_msg).toUtf8().data()) {}

    // with a sub-name
    explicit snap_exception(char const * subname, char const *        what_msg) : runtime_error(std::string("Snap! Exception:") + subname + ": " + what_msg) {}
    explicit snap_exception(char const * subname, std::string const & what_msg) : runtime_error(std::string("Snap! Exception:") + subname + ": " + what_msg) {}
    explicit snap_exception(char const * subname, QString const &     what_msg) : runtime_error(std::string("Snap! Exception:") + subname + ": " + what_msg.toUtf8().data()) {}
};


class snap_logic_exception : public std::logic_error, public snap_exception_base
{
public:
    // no sub-name
    explicit snap_logic_exception(char const *        what_msg) : logic_error("Snap! Exception: " + std::string(what_msg)) {}
    explicit snap_logic_exception(std::string const & what_msg) : logic_error("Snap! Exception: " + what_msg) {}
    explicit snap_logic_exception(QString const &     what_msg) : logic_error(("Snap! Exception: " + what_msg).toUtf8().data()) {}

    // with a sub-name
    explicit snap_logic_exception(char const * subname, char const *        what_msg) : logic_error(std::string("Snap! Exception:") + subname + ": " + what_msg) {}
    explicit snap_logic_exception(char const * subname, std::string const & what_msg) : logic_error(std::string("Snap! Exception:") + subname + ": " + what_msg) {}
    explicit snap_logic_exception(char const * subname, QString const &     what_msg) : logic_error(std::string("Snap! Exception:") + subname + ": " + what_msg.toUtf8().data()) {}
};


class snap_io_exception : public snap_exception
{
public:
    // no sub-name
    explicit snap_io_exception(char const *        what_msg) : snap_exception(what_msg) {}
    explicit snap_io_exception(std::string const & what_msg) : snap_exception(what_msg) {}
    explicit snap_io_exception(QString const &     what_msg) : snap_exception(what_msg) {}
};


} // namespace snap

// vim: ts=4 sw=4 et
