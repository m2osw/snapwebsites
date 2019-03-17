// Snap Websites Server -- handle Snap! files apache2 settings
// Copyright (c) 2016-2019  Made to Order Software Corp.  All Rights Reserved
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

//#include <QDomDocument>

namespace snap
{
namespace apache2
{


enum class name_t
{
    SNAP_NAME_SNAPMANAGERCGI_APACHE2_NAME
};
char const * get_name(name_t name) __attribute__ ((const));



class apache2_exception : public snap_exception
{
public:
    apache2_exception(char const *        what_msg) : snap_exception("apache2", what_msg) {}
    apache2_exception(std::string const & what_msg) : snap_exception("apache2", what_msg) {}
    apache2_exception(QString const &     what_msg) : snap_exception("apache2", what_msg) {}
};

class apache2_exception_invalid_argument : public apache2_exception
{
public:
    apache2_exception_invalid_argument(char const *        what_msg) : apache2_exception(what_msg) {}
    apache2_exception_invalid_argument(std::string const & what_msg) : apache2_exception(what_msg) {}
    apache2_exception_invalid_argument(QString const &     what_msg) : apache2_exception(what_msg) {}
};





class apache2
    : public snap_manager::plugin_base
{
public:
                            apache2();
                            apache2(apache2 const & rhs) = delete;
    virtual                 ~apache2() override;

    apache2 &               operator = (apache2 const & rhs) = delete;

    static apache2 *        instance();

    // plugins::plugin implementation
    virtual QString         description() const override;
    virtual QString         dependencies() const override;
    virtual int64_t         do_update(int64_t last_updated) override;
    virtual void            bootstrap(snap_child * snap) override;

    // manager overload
    virtual bool            display_value(QDomElement parent, snap_manager::status_t const & s, snap::snap_uri const & uri) override;
    virtual bool            apply_setting(QString const & button_name, QString const & field_name, QString const & new_value, QString const & old_value, std::set<QString> & affected_services) override;

    // server signal
    void                    on_retrieve_status(snap_manager::server_status & server_status);
    void                    on_handle_affected_services(std::set<QString> & affected_services);

private:
    void                    retrieve_status_of_conf(snap_manager::server_status & server_status,
                                                    std::string const & conf_namespace,
                                                    std::string const & conf_filename);

    snap_manager::manager * f_snap = nullptr;
};


} // namespace apache2
} // namespace snap
// vim: ts=4 sw=4 et
