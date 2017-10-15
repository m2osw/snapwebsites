// Snap Websites Server -- handle the server status
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

#include <QString>

#include <map>

namespace snap_manager
{


class status_t
{
public:
    typedef std::map<QString, status_t>   map_t;

    enum class state_t
    {
        STATUS_STATE_UNDEFINED = -1,

        STATUS_STATE_DEBUG = 1,
        STATUS_STATE_INFO,
        STATUS_STATE_MODIFIED,
        STATUS_STATE_HIGHLIGHT,
        STATUS_STATE_WARNING,
        STATUS_STATE_ERROR,
        STATUS_STATE_FATAL_ERROR
    };

                    status_t();
                    status_t(state_t s, QString const & plugin_name, QString const & field_name, QString const & value);

    void            clear();

    void            set_state(state_t s);
    state_t         get_state() const;

    void            set_plugin_name(QString const & name);
    QString         get_plugin_name() const;

    void            set_field_name(QString const & name);
    QString         get_field_name() const;

    void            set_value(QString const & value);
    QString         get_value() const;

    QString         to_string() const;
    bool            from_string(QString const & line);

private:
    state_t         f_state = state_t::STATUS_STATE_INFO;
    QString         f_plugin_name;
    QString         f_field_name;
    QString         f_value;
};


} // snap_manager namespace
// vim: ts=4 sw=4 et
