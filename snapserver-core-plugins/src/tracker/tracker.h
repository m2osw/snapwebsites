// Snap Websites Server -- list management (sort criteria)
// Copyright (C) 2014-2017  Made to Order Software Corp.
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

// other plugins
//
#include <snapwebsites/snapwebsites.h>


namespace snap
{
namespace tracker
{


enum class name_t
{
    SNAP_NAME_TRACKER_TABLE,
    SNAP_NAME_TRACKER_TRACKINGDATA
};
char const * get_name(name_t name) __attribute__ ((const));


class tracker_exception : public snap_exception
{
public:
    explicit tracker_exception(char const *        what_msg) : snap_exception("tracker", what_msg) {}
    explicit tracker_exception(std::string const & what_msg) : snap_exception("tracker", what_msg) {}
    explicit tracker_exception(QString const &     what_msg) : snap_exception("tracker", what_msg) {}
};

class tracker_exception_no_backend : public tracker_exception
{
public:
    explicit tracker_exception_no_backend(char const *        what_msg) : tracker_exception(what_msg) {}
    explicit tracker_exception_no_backend(std::string const & what_msg) : tracker_exception(what_msg) {}
    explicit tracker_exception_no_backend(QString const &     what_msg) : tracker_exception(what_msg) {}
};

class tracker_exception_invalid_number_of_parameters : public tracker_exception
{
public:
    explicit tracker_exception_invalid_number_of_parameters(char const *        what_msg) : tracker_exception(what_msg) {}
    explicit tracker_exception_invalid_number_of_parameters(std::string const & what_msg) : tracker_exception(what_msg) {}
    explicit tracker_exception_invalid_number_of_parameters(QString const &     what_msg) : tracker_exception(what_msg) {}
};







class tracker
        : public plugins::plugin
        , public server::backend_action
{
public:
                        tracker();
                        ~tracker();

    // plugins::plugin implementation
    static tracker *    instance();
    virtual QString     settings_path() const;
    virtual QString     icon() const;
    virtual QString     description() const;
    virtual QString     dependencies() const;
    virtual int64_t     do_update(int64_t last_updated);
    virtual void        bootstrap(snap_child * snap);

    libdbproxy::table::pointer_t get_tracker_table();

    // server signals
    void                on_attach_to_session();
    void                on_detach_from_session();
    void                on_register_backend_action(server::backend_action_set & actions);

    // server::backend_action implementation
    virtual void        on_backend_action(QString const & action);

private:
    void                initial_update(int64_t variables_timestamp);
    void                content_update(int64_t variables_timestamp);
    void                on_backend_tracking_data();

    snap_child *                            f_snap = nullptr;
    libdbproxy::table::pointer_t f_tracker_table;
    QString                                 f_email;
    QDomDocument                            f_doc;
};


} // namespace tracker
} // namespace snap
// vim: ts=4 sw=4 et
