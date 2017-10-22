// Snap Websites Server -- layout management
// Copyright (C) 2011-2017  Made to Order Software Corp.
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
#include "../content/content.h"


namespace snap
{
namespace layout_contrib
{

enum class name_t
{
    SNAP_NAME_LAYOUT_CONTRIB_BOOTSTRAP
};
char const * get_name(name_t name) __attribute__ ((const));






class layout_contrib
        : public plugins::plugin
{
public:
                            layout_contrib();
                            ~layout_contrib();

    // plugins::plugin implementation
    static layout_contrib * instance();
    virtual QString         icon() const override;
    virtual QString         description() const override;
    virtual QString         dependencies() const override;
    virtual int64_t         do_update(int64_t last_updated) override;
    virtual void            bootstrap(snap_child *snap) override;

private:
    void                content_update(int64_t variables_timestamp);
    void                do_layout_updates();
    void                install_layout(QString const & layout_name);
    void                finish_install_layout();

    void                generate_boxes(content::path_info_t & ipath, QString const & layout_name, QDomDocument doc);

    snap_child *                            f_snap = nullptr;
    libdbproxy::table::pointer_t f_content_table;
    std::vector<QString>                    f_initialized_layout;
};



} // namespace layout
} // namespace snap
// vim: ts=4 sw=4 et
