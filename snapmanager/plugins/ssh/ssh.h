// Snap Websites Server -- handle user SSH id_rsa.pub key
// Copyright (C) 2016-2017  Made to Order Software Corp.
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

#include "snapmanager/manager.h"
#include "snapmanager/plugin_base.h"

#include <snapwebsites/plugins.h>

#include <QDomDocument>

namespace snap
{
namespace ssh
{


enum class name_t
{
    SNAP_NAME_SNAPMANAGERCGI_SSH_NAME
};
char const * get_name(name_t name) __attribute__ ((const));



class ssh_exception : public snap_exception
{
public:
    ssh_exception(char const *        what_msg) : snap_exception("ssh", what_msg) {}
    ssh_exception(std::string const & what_msg) : snap_exception("ssh", what_msg) {}
    ssh_exception(QString const &     what_msg) : snap_exception("ssh", what_msg) {}
};

class ssh_exception_invalid_argument : public ssh_exception
{
public:
    ssh_exception_invalid_argument(char const *        what_msg) : ssh_exception(what_msg) {}
    ssh_exception_invalid_argument(std::string const & what_msg) : ssh_exception(what_msg) {}
    ssh_exception_invalid_argument(QString const &     what_msg) : ssh_exception(what_msg) {}
};


class ssh_config
{
public:
    ssh_config( QString const & filepath );

    bool read();
    bool write();

    QString get_entry( QString const& name, QString const& default_value = {} ) const;
    void    set_entry( QString const& name, QString const& value );

private:
    QString     f_filepath;
    QStringList f_lines;
};



class ssh
        : public snap_manager::plugin_base
{
public:
                            ssh();
    virtual                 ~ssh() override;

    // plugins::plugin implementation
    static ssh *            instance();
    virtual QString         description() const;
    virtual QString         dependencies() const;
    virtual int64_t         do_update(int64_t last_updated);
    virtual void            bootstrap(snap_child * snap);

    // manager overload
    virtual bool            display_value(QDomElement parent, snap_manager::status_t const & s, snap::snap_uri const & uri) override;
    virtual bool            apply_setting(QString const & button_name, QString const & field_name, QString const & new_value, QString const & old_value, std::set<QString> & affected_services) override;

    // server signal
    void                    on_retrieve_status(snap_manager::server_status & server_status);

private:
    bool                    is_installed();

    snap_manager::manager * f_snap = nullptr;
};

} // namespace ssh
} // namespace snap
// vim: ts=4 sw=4 et
