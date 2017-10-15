// Snap Websites Server -- CPU watchdog: record CPU usage over time
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

#include <snapwebsites/plugins.h>
#include <snapwebsites/snap_child.h>

#include <QDomDocument>

namespace snap
{
namespace cpu
{


enum class name_t
{
    SNAP_NAME_WATCHDOG_CPU_NAME
};
char const * get_name(name_t name) __attribute__ ((const));



class cpu_exception : public snap_exception
{
public:
    cpu_exception(char const *        what_msg) : snap_exception("cpu", what_msg) {}
    cpu_exception(std::string const & what_msg) : snap_exception("cpu", what_msg) {}
    cpu_exception(QString const &     what_msg) : snap_exception("cpu", what_msg) {}
};

class cpu_exception_invalid_argument : public cpu_exception
{
public:
    cpu_exception_invalid_argument(char const *        what_msg) : cpu_exception(what_msg) {}
    cpu_exception_invalid_argument(std::string const & what_msg) : cpu_exception(what_msg) {}
    cpu_exception_invalid_argument(QString const &     what_msg) : cpu_exception(what_msg) {}
};





class cpu
        : public plugins::plugin
{
public:
                        cpu();
                        ~cpu();

    // plugins::plugin implementation
    static cpu *        instance();
    virtual QString     description() const;
    virtual QString     dependencies() const;
    virtual int64_t     do_update(int64_t last_updated);
    virtual void        bootstrap(snap_child * snap);

    // server signal
    void                on_process_watch(QDomDocument doc);

private:
    snap_child *        f_snap = nullptr;
};

} // namespace cpu
} // namespace snap
// vim: ts=4 sw=4 et
