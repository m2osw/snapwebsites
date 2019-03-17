// Snap Websites Server -- base exception of the Snap! library
// Copyright (c) 2011-2019  Made to Order Software Corp.  All Rights Reserved
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
#pragma once

// Qt lib
//
#include <QString>

// C++ lib
//
#include <stdexcept>

namespace snap
{

// Base exception that automatically logs a stack trace at the location
// of the exception (thus our exceptions are extremely slow!)
//
class snap_exception_base
{
public:
    static int const            STACK_TRACE_DEPTH = 20;

                                snap_exception_base(char const *        what_msg);
                                snap_exception_base(std::string const & what_msg);
                                snap_exception_base(QString const &     what_msg);
    virtual                     ~snap_exception_base() {}

    static void                 output_stack_trace( int const stack_trace_depth = STACK_TRACE_DEPTH );
};



// This is a runtime exception; these are expected to be caught
//
class snap_exception
    : public snap_exception_base
    , public std::runtime_error
{
public:
    // no sub-name
    explicit snap_exception(char const *        what_msg) : snap_exception_base(what_msg), runtime_error("Snap! Exception: " + std::string(what_msg)) {}
    explicit snap_exception(std::string const & what_msg) : snap_exception_base(what_msg), runtime_error("Snap! Exception: " + what_msg) {}
    explicit snap_exception(QString const &     what_msg) : snap_exception_base(what_msg), runtime_error(("Snap! Exception: " + what_msg).toUtf8().data()) {}

    // with a sub-name
    explicit snap_exception(char const * subname, char const *        what_msg) : snap_exception_base(what_msg), runtime_error(std::string("Snap! Exception:") + subname + ": " + what_msg) {}
    explicit snap_exception(char const * subname, std::string const & what_msg) : snap_exception_base(what_msg), runtime_error(std::string("Snap! Exception:") + subname + ": " + what_msg) {}
    explicit snap_exception(char const * subname, QString const &     what_msg) : snap_exception_base(what_msg), runtime_error(std::string("Snap! Exception:") + subname + ": " + what_msg.toUtf8().data()) {}
};


// This is a logic exception; you should not catch those, instead you
// should fix the code if they happen
//
class snap_logic_exception
    : public snap_exception_base
    , public std::logic_error
{
public:
    // no sub-name
    explicit snap_logic_exception(char const *        what_msg) : snap_exception_base(what_msg), logic_error("Snap! Exception: " + std::string(what_msg)) {}
    explicit snap_logic_exception(std::string const & what_msg) : snap_exception_base(what_msg), logic_error("Snap! Exception: " + what_msg) {}
    explicit snap_logic_exception(QString const &     what_msg) : snap_exception_base(what_msg), logic_error(("Snap! Exception: " + what_msg).toUtf8().data()) {}

    // with a sub-name
    explicit snap_logic_exception(char const * subname, char const *        what_msg) : snap_exception_base(what_msg), logic_error(std::string("Snap! Exception:") + subname + ": " + what_msg) {}
    explicit snap_logic_exception(char const * subname, std::string const & what_msg) : snap_exception_base(what_msg), logic_error(std::string("Snap! Exception:") + subname + ": " + what_msg) {}
    explicit snap_logic_exception(char const * subname, QString const &     what_msg) : snap_exception_base(what_msg), logic_error(std::string("Snap! Exception:") + subname + ": " + what_msg.toUtf8().data()) {}
};


// A basic I/O exception one can use anywhere
//
class snap_exception_io : public snap_exception
{
public:
    // no sub-name
    explicit snap_exception_io(char const *        what_msg) : snap_exception(what_msg) {}
    explicit snap_exception_io(std::string const & what_msg) : snap_exception(what_msg) {}
    explicit snap_exception_io(QString const &     what_msg) : snap_exception(what_msg) {}
};


// A basic invalid parameter exception
//
class snap_exception_invalid_parameter : public snap_logic_exception
{
public:
    // no sub-name
    explicit snap_exception_invalid_parameter(char const *        what_msg) : snap_logic_exception(what_msg) {}
    explicit snap_exception_invalid_parameter(std::string const & what_msg) : snap_logic_exception(what_msg) {}
    explicit snap_exception_invalid_parameter(QString const &     what_msg) : snap_logic_exception(what_msg) {}
};


// A basic missing parameter exception
//
class snap_exception_missing_parameter : public snap_logic_exception
{
public:
    // no sub-name
    explicit snap_exception_missing_parameter(char const *        what_msg) : snap_logic_exception(what_msg) {}
    explicit snap_exception_missing_parameter(std::string const & what_msg) : snap_logic_exception(what_msg) {}
    explicit snap_exception_missing_parameter(QString const &     what_msg) : snap_logic_exception(what_msg) {}
};


} // namespace snap

// vim: ts=4 sw=4 et
