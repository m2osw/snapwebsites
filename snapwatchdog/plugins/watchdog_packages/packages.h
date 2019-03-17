// Snap Websites Server -- watchdog packages
// Copyright (c) 2013-2019  Made to Order Software Corp.  All Rights Reserved
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

// our lib
//
#include "snapwatchdog/snapwatchdog.h"

// snapwebsites lib
//
#include <snapwebsites/plugins.h>
#include <snapwebsites/snap_child.h>

// Qt lib
//
#include <QDomDocument>


namespace snap
{
namespace packages
{


enum class name_t
{
    SNAP_NAME_WATCHDOG_PACKAGES_CACHE_FILENAME,
    SNAP_NAME_WATCHDOG_PACKAGES_PATH
};
char const * get_name(name_t name) __attribute__ ((const));



class packages_exception : public snap_exception
{
public:
    packages_exception(char const *        what_msg) : snap_exception("packages", what_msg) {}
    packages_exception(std::string const & what_msg) : snap_exception("packages", what_msg) {}
    packages_exception(QString const &     what_msg) : snap_exception("packages", what_msg) {}
};

class packages_exception_invalid_argument : public packages_exception
{
public:
    packages_exception_invalid_argument(char const *        what_msg) : packages_exception(what_msg) {}
    packages_exception_invalid_argument(std::string const & what_msg) : packages_exception(what_msg) {}
    packages_exception_invalid_argument(QString const &     what_msg) : packages_exception(what_msg) {}
};

class packages_exception_invalid_name : public packages_exception
{
public:
    packages_exception_invalid_name(char const *        what_msg) : packages_exception(what_msg) {}
    packages_exception_invalid_name(std::string const & what_msg) : packages_exception(what_msg) {}
    packages_exception_invalid_name(QString const &     what_msg) : packages_exception(what_msg) {}
};

class packages_exception_invalid_priority : public packages_exception
{
public:
    packages_exception_invalid_priority(char const *        what_msg) : packages_exception(what_msg) {}
    packages_exception_invalid_priority(std::string const & what_msg) : packages_exception(what_msg) {}
    packages_exception_invalid_priority(QString const &     what_msg) : packages_exception(what_msg) {}
};





class packages
    : public plugins::plugin
{
public:
                        packages();
                        packages(packages const & rhs) = delete;
    virtual             ~packages() override;

    packages &          operator = (packages const & rhs) = delete;

    static packages *   instance();

    // plugins::plugin implementation
    virtual QString     description() const override;
    virtual QString     dependencies() const override;
    virtual int64_t     do_update(int64_t last_updated) override;
    virtual void        bootstrap(snap_child * snap) override;

    // server signals
    void                on_process_watch(QDomDocument doc);

private:
    void                load_packages();
    void                load_xml(QString package_filename);

    watchdog_child *    f_snap = nullptr;
};

} // namespace packages
} // namespace snap
// vim: ts=4 sw=4 et
