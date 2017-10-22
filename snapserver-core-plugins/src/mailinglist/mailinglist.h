// Snap Websites Server -- mailing list system
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

// snapwebsites lib
//
#include <snapwebsites/snapwebsites.h>
#include <snapwebsites/plugins.h>
#include <snapwebsites/snap_child.h>

// Qt lib
//
#include <QMap>
#include <QVector>
#include <QByteArray>


namespace snap
{
namespace mailinglist
{


enum class name_t
{
    SNAP_NAME_MAILINGLIST_TABLE
};
char const * get_name(name_t name) __attribute__ ((const));




class mailinglist
        : public plugins::plugin
{
public:
    class list
    {
    public:
        static const int LIST_MAJOR_VERSION = 1;
        static const int LIST_MINOR_VERSION = 0;

        list(mailinglist * parent, QString const & list_name);
        virtual ~list();

        QString name() const;
        virtual QString next();

    private:
        mailinglist *                                           f_parent = nullptr;
        QString const                                           f_name;
        libdbproxy::table::pointer_t                 f_table;
        libdbproxy::row::pointer_t                   f_row;
        libdbproxy::cell_range_predicate::pointer_t    f_column_predicate;
        libdbproxy::cells                            f_cells;
        libdbproxy::cells::const_iterator            f_c;
        bool                                                    f_done = false;
    };

                        mailinglist();
                        ~mailinglist();

    // plugins::plugin implementation
    static mailinglist *instance();
    virtual QString     settings_path() const;
    virtual QString     icon() const;
    virtual QString     description() const;
    virtual QString     dependencies() const;
    virtual int64_t     do_update(int64_t last_updated);
    virtual void        bootstrap(snap_child * snap);

    libdbproxy::table::pointer_t get_mailinglist_table();

    SNAP_SIGNAL(name_to_list, (QString const & name, QSharedPointer<list> & emails), (name, emails));

private:
    void                content_update(int64_t variables_timestamp);

    snap_child *        f_snap = nullptr;
};

} // namespace mailinglist
} // namespace snap
// vim: ts=4 sw=4 et
