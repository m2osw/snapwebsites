// Snap Websites Server -- manage the bundles
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

// snapmanager lib
//
#include "snapmanager/manager.h"
#include "snapmanager/plugin_base.h"

// snapwebsites lib
//
#include <snapwebsites/plugins.h>

// Qt lib
//
#include <QDomDocument>




namespace snap
{
namespace bundles
{


enum class name_t
{
    SNAP_NAME_SNAPMANAGERCGI_BUNDLES_NAME
};
char const * get_name(name_t name) __attribute__ ((const));



class bundles_exception : public snap_exception
{
public:
    bundles_exception(char const *        what_msg) : snap_exception("bundles", what_msg) {}
    bundles_exception(std::string const & what_msg) : snap_exception("bundles", what_msg) {}
    bundles_exception(QString const &     what_msg) : snap_exception("bundles", what_msg) {}
};

class bundles_exception_invalid_argument : public bundles_exception
{
public:
    bundles_exception_invalid_argument(char const *        what_msg) : bundles_exception(what_msg) {}
    bundles_exception_invalid_argument(std::string const & what_msg) : bundles_exception(what_msg) {}
    bundles_exception_invalid_argument(QString const &     what_msg) : bundles_exception(what_msg) {}
};





class bundles
    : public snap_manager::plugin_base
{
public:
                            bundles();
                            bundles(bundles const & rhs) = delete;
    virtual                 ~bundles() override;

    bundles &               operator = (bundles const & rhs) = delete;

    static bundles *        instance();

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

private:
    void                    retrieve_bundles_status(snap_manager::server_status & server_status);

    snap_manager::manager * f_snap = nullptr;
};


} // namespace bundles
} // namespace snap
// vim: ts=4 sw=4 et
