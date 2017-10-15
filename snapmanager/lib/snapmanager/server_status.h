// Snap Websites Server -- handle the server status
// Copyright (C) 2016  Made to Order Software Corp.
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

#include "status.h"


namespace snap_manager
{




class server_status
{
public:
                            server_status(QString const & filename);
                            server_status(QString const & cluster_status_path, QString const & server);
                            ~server_status();

    // "vector access"
    void                    set_field(status_t const & status);
    QString                 get_field(QString const & plugin_name, QString const & field_name) const;
    status_t::state_t       get_field_state(QString const & plugin_name, QString const & field_name) const;
    status_t const &        get_field_status(QString const & plugin_name, QString const & field_name) const;
    int                     size() const;
    status_t::map_t const & get_statuses() const;
    size_t                  count_warnings() const;
    size_t                  count_errors() const;

    QString                 to_string() const;
    bool                    from_string(QString const & status);

    QString const &         get_filename() const;

    // whether the read or write generated an error
    bool                    has_error() const;

    // file access (read)
    bool                    read_all();
    bool                    read_header();

    // file access (write)
    bool                    write();

private:
    void                    close();

    // reading helper functions
    bool                    open_for_read();
    bool                    readline(QString & result);
    bool                    readvar(QString & name, QString & value);

    QString                 f_filename;
    status_t::map_t         f_statuses;
    int                     f_fd = -1;
    FILE *                  f_file = nullptr;
    bool                    f_has_error = false;
};



} // snap_manager namespace
// vim: ts=4 sw=4 et
