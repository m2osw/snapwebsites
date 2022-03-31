// Snap Websites Server -- menu manager
// Copyright (c) 2014-2019  Made to Order Software Corp.  All Rights Reserved
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
namespace menu
{

enum class name_t
{
    SNAP_NAME_MENU_NAMESPACE
};
char const * get_name(name_t name) __attribute__ ((const));


DECLARE_MAIN_EXCEPTION(menu_exception);

DECLARE_EXCEPTION(menu_exception, menu_exception_missing_links_table);
DECLARE_EXCEPTION(menu_exception, menu_exception_missing_data_table);
DECLARE_EXCEPTION(menu_exception, menu_exception_invalid_name);
DECLARE_EXCEPTION(menu_exception, menu_exception_invalid_db_data);




class menu
    : public cppthread::plugin
    , public layout::layout_content
{
public:
                        menu();
                        menu(menu const & rhs) = delete;
    virtual             ~menu() override;

    menu &              operator = (menu const & rhs) = delete;

    static menu *       instance();

    // plugins::plugin implementation
    virtual int64_t     do_update(int64_t last_updated) override;
    virtual void        bootstrap(snap_child * snap) override;

    // layout::layout_content imlementation
    virtual void        on_generate_main_content(content::path_info_t & ipath, QDomElement & page, QDomElement & body);

private:
    void                content_update(int64_t variables_timestamp);

    snap_child *        f_snap = nullptr;
};

} // namespace menu
} // namespace snap
// vim: ts=4 sw=4 et
