// Snap Servers -- loadavg file handling
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

// C lib
//
#include <arpa/inet.h>

// C++ lib
//
#include <string>
#include <vector>


namespace snap
{


// the sequential file is an array of these items
//
struct loadavg_item
{
    typedef std::vector<loadavg_item>       vector_t;

    int64_t                     f_timestamp = 0;
    struct sockaddr_in6         f_address = sockaddr_in6();
    float                       f_avg = 0.0f;
};



class loadavg_file
{
public:
    bool                        load();
    bool                        save() const;

    void                        add(loadavg_item const & item);
    bool                        remove_old_entries(int how_old);
    loadavg_item const *        find(struct sockaddr_in6 const & addr) const;
    loadavg_item const *        find_least_busy() const;

private:
    loadavg_item::vector_t      f_items;
};


} // namespace snap
// vim: ts=4 sw=4 et
