// Snap Websites Server -- manage the snapwatchdog settings
// Copyright (c) 2016-2021  Made to Order Software Corp.  All Rights Reserved
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

// snapmanager library
//
#include "snapmanager/manager.h"
#include "snapmanager/plugin_base.h"

// snapwebsites lib
//
#include <snapwebsites/plugins.h>
#include <snapwebsites/snap_string_list.h>

// Qt lib
//
#include <QDomDocument>



namespace snap
{
namespace watchdog
{


enum class name_t
{
    SNAP_NAME_SNAPMANAGERCGI_WATCHDOG_FROM_EMAIL
};
char const * get_name(name_t name) __attribute__ ((const));



class watchdog_exception : public snap_exception
{
public:
    watchdog_exception(char const *        what_msg) : snap_exception("watchdog", what_msg) {}
    watchdog_exception(std::string const & what_msg) : snap_exception("watchdog", what_msg) {}
    watchdog_exception(QString const &     what_msg) : snap_exception("watchdog", what_msg) {}
};

class watchdog_exception_invalid_argument : public watchdog_exception
{
public:
    watchdog_exception_invalid_argument(char const *        what_msg) : watchdog_exception(what_msg) {}
    watchdog_exception_invalid_argument(std::string const & what_msg) : watchdog_exception(what_msg) {}
    watchdog_exception_invalid_argument(QString const &     what_msg) : watchdog_exception(what_msg) {}
};





class watchdog
    : public snap_manager::plugin_base
{
public:
                            watchdog();
                            watchdog(watchdog const & rhs) = delete;
    virtual                 ~watchdog() override;

    watchdog &              operator = (watchdog const & rhs) = delete;

    static watchdog *       instance();

    // plugins::plugin implementation
    virtual QString         description() const override;
    virtual QString         dependencies() const override;
    virtual int64_t         do_update(int64_t last_updated) override;
    virtual void            bootstrap(snap_child * snap) override;

    // manager overload
    virtual bool            display_value(QDomElement parent, snap_manager::status_t const & s, snap::snap_uri const & uri) override;
    virtual bool            apply_setting(QString const & button_name, QString const & field_name, QString const & new_value, QString const & old_value, std::set<QString> & services) override;

    // server signal
    void                    on_retrieve_status(snap_manager::server_status & server_status);

    // snapmanager_cgi signal
    void                    on_generate_content(QDomDocument doc, QDomElement root, QDomElement output, QDomElement menu, snap::snap_uri const & uri);

private:
    void                    retrieve_bundles_status(snap_manager::server_status & server_status);
    bool                    install_bundle(bool const install, QString const & bundle_name, std::set<QString> & services);
    QString                 get_list_of_available_plugins();
    void                    get_plugin_names(QString processes_filename, snap_string_list * plugin_names);

    snap_manager::manager * f_snap = nullptr;
};

} // namespace watchdog
} // namespace snap
// vim: ts=4 sw=4 et
