// Snap Websites Server -- List watchdog: make sure the list processes work.
// Copyright (c) 2018-2019  Made to Order Software Corp.  All Rights Reserved
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
#include "snapwatchdog.h"

// snapwebsites lib
//
#include <snapwebsites/plugins.h>
#include <snapwebsites/snap_child.h>

// Qt lib
//
#include <QDomDocument>



namespace snap
{
namespace list
{


enum class name_t
{
    SNAP_NAME_WATCHDOG_LIST_NAME
};
char const * get_name(name_t name) __attribute__ ((const));



class list_exception : public snap_exception
{
public:
    list_exception(char const *        what_msg) : snap_exception("list", what_msg) {}
    list_exception(std::string const & what_msg) : snap_exception("list", what_msg) {}
    list_exception(QString const &     what_msg) : snap_exception("list", what_msg) {}
};

class list_exception_invalid_argument : public list_exception
{
public:
    list_exception_invalid_argument(char const *        what_msg) : list_exception(what_msg) {}
    list_exception_invalid_argument(std::string const & what_msg) : list_exception(what_msg) {}
    list_exception_invalid_argument(QString const &     what_msg) : list_exception(what_msg) {}
};





class list
    : public plugins::plugin
{
public:
                        list();
                        list(list const & rhs) = delete;
    virtual             ~list() override;

    list &              operator = (list const & rhs) = delete;

    static list *       instance();

    // plugins::plugin implementation
    virtual QString     description() const override;
    virtual QString     dependencies() const override;
    virtual int64_t     do_update(int64_t last_updated) override;
    virtual void        bootstrap(snap_child * snap) override;

    // server signal
    void                on_process_watch(QDomDocument doc);

private:
    void                local_journal(QDomElement e);
    void                count_files(QString const & filename, QDomElement e);
    void                snaplist_database(QDomElement e);

    watchdog_child *    f_snap = nullptr;
    QString             f_username = QString("snapwebsites");
    QString             f_groupname = QString("snapwebsites");
    uid_t               f_uid = -1;
    gid_t               f_gid = -1;
    int                 f_count = 0;
};


} // namespace list
} // namespace snap
// vim: ts=4 sw=4 et
