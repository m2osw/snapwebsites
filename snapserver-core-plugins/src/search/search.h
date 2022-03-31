// Snap Websites Server -- search capability
// Copyright (c) 2012-2019  Made to Order Software Corp.  All Rights Reserved
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
#include "../layout/layout.h"


namespace snap
{
namespace search
{

enum class name_t
{
    SNAP_NAME_SEARCH_STATUS
};
char const * get_name(name_t name) __attribute__ ((const));



class search
    : public cppthread::plugin
{
public:
                            search();
                            search(search const & rhs) = delete;
    virtual                 ~search() override;

    search &                operator = (search const & rhs) = delete;

    // public plugins::plugin
    virtual int64_t         do_update(int64_t last_updated) override;
    virtual void            bootstrap(snap_child * snap) override;

    // server signals
    void                    on_improve_signature(QString const & path, QDomDocument doc, QDomElement signature);

    // layout signals
    void                    on_generate_page_content(content::path_info_t & ipath, QDomElement & page, QDomElement & body);

private:
    void                    content_update(int64_t variables_timestamp);

    snap_child *            f_snap = nullptr;
};

} // namespace search
} // namespace snap
// vim: ts=4 sw=4 et
