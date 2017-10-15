// Snap Websites Server -- menu manager
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


class menu_exception : public snap_exception
{
public:
    explicit menu_exception(char const *        what_msg) : snap_exception("menu", what_msg) {}
    explicit menu_exception(std::string const & what_msg) : snap_exception("menu", what_msg) {}
    explicit menu_exception(QString const &     what_msg) : snap_exception("menu", what_msg) {}
};

class menu_exception_missing_links_table : public menu_exception
{
public:
    explicit menu_exception_missing_links_table(char const *        what_msg) : menu_exception(what_msg) {}
    explicit menu_exception_missing_links_table(std::string const & what_msg) : menu_exception(what_msg) {}
    explicit menu_exception_missing_links_table(QString const &     what_msg) : menu_exception(what_msg) {}
};

class menu_exception_missing_data_table : public menu_exception
{
public:
    explicit menu_exception_missing_data_table(char const *        what_msg) : menu_exception(what_msg) {}
    explicit menu_exception_missing_data_table(std::string const & what_msg) : menu_exception(what_msg) {}
    explicit menu_exception_missing_data_table(QString const &     what_msg) : menu_exception(what_msg) {}
};

class menu_exception_invalid_name : public menu_exception
{
public:
    explicit menu_exception_invalid_name(char const *        what_msg) : menu_exception(what_msg) {}
    explicit menu_exception_invalid_name(std::string const & what_msg) : menu_exception(what_msg) {}
    explicit menu_exception_invalid_name(QString const &     what_msg) : menu_exception(what_msg) {}
};

class menu_exception_invalid_db_data : public menu_exception
{
public:
    explicit menu_exception_invalid_db_data(char const *        what_msg) : menu_exception(what_msg) {}
    explicit menu_exception_invalid_db_data(std::string const & what_msg) : menu_exception(what_msg) {}
    explicit menu_exception_invalid_db_data(QString const &     what_msg) : menu_exception(what_msg) {}
};




class menu
        : public plugins::plugin
        , public layout::layout_content
{
public:
                        menu();
                        ~menu();

    // plugins::plugin implementation
    static menu *       instance();
    virtual QString     settings_path() const;
    virtual QString     icon() const;
    virtual QString     description() const;
    virtual QString     dependencies() const;
    virtual int64_t     do_update(int64_t last_updated);
    virtual void        bootstrap(snap_child * snap);

    // layout::layout_content imlementation
    virtual void        on_generate_main_content(content::path_info_t & ipath, QDomElement & page, QDomElement & body);

private:
    void                content_update(int64_t variables_timestamp);

    snap_child *        f_snap = nullptr;
};

} // namespace menu
} // namespace snap
// vim: ts=4 sw=4 et
