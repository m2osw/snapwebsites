// Snap Websites Server -- listener check and response management
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

#include "../path/path.h"

namespace snap
{
namespace listener
{


class listener_exception : public snap_exception
{
public:
    explicit listener_exception(char const *        what_msg) : snap_exception("server-access", what_msg) {}
    explicit listener_exception(std::string const & what_msg) : snap_exception("server-access", what_msg) {}
    explicit listener_exception(QString const &     what_msg) : snap_exception("server-access", what_msg) {}
};

class listener_exception_status_missing : public listener_exception
{
public:
    explicit listener_exception_status_missing(char const *        what_msg) : listener_exception(what_msg) {}
    explicit listener_exception_status_missing(std::string const & what_msg) : listener_exception(what_msg) {}
    explicit listener_exception_status_missing(QString const &     what_msg) : listener_exception(what_msg) {}
};







class listener
        : public plugins::plugin
{
public:
                                listener();
                                ~listener();

    // plugins::plugin implementation
    static listener *           instance();
    virtual QString             icon() const;
    virtual QString             description() const;
    virtual QString             dependencies() const;
    virtual int64_t             do_update(int64_t last_updated);
    virtual void                bootstrap(snap_child * snap);

    // server signals
    void                        on_process_post(QString const & uri_path);

    SNAP_SIGNAL(listener_check, (snap_uri const & uri, content::path_info_t & page_ipath, QDomDocument doc, QDomElement result), (uri, page_ipath, doc, result));

private:
    void                        content_update(int64_t variables_timestamp);

    snap_child *                f_snap = nullptr;
};


} // namespace listener
} // namespace snap
// vim: ts=4 sw=4 et
