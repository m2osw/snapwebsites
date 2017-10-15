// Snap Websites Server -- base of each plugin so we can have direct callbacks
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

// ourselves
//
#include "server_status.h"

// snapwebsites lib
//
#include <snapwebsites/snapwebsites.h>

// C++ lib
//
#include <set>


namespace snap_manager
{


class plugin_base
        : public snap::plugins::plugin
{
public:
    virtual             ~plugin_base() override;

    virtual bool        display_value(QDomElement parent, status_t const & s, snap::snap_uri const & uri) = 0;
    virtual bool        apply_setting(QString const & button_name, QString const & field_name, QString const & new_value, QString const & old_value, std::set<QString> & services) = 0;
};


} // snap_manager namespace
// vim: ts=4 sw=4 et
