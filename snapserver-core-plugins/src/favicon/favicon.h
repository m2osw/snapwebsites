// Snap Websites Server -- favicon management (little icon in tab)
// Copyright (C) 2013-2017  Made to Order Software Corp.
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
#include "../path/path.h"


namespace snap
{
namespace favicon
{


enum class name_t
{
    SNAP_NAME_FAVICON_ICON,
    SNAP_NAME_FAVICON_ICON_PATH,
    SNAP_NAME_FAVICON_IMAGE,
    SNAP_NAME_FAVICON_SETTINGS
};
char const * get_name(name_t name) __attribute__ ((const));


class favicon_exception : public snap_exception
{
public:
    explicit favicon_exception(char const *        what_msg) : snap_exception("Favorite Icon", what_msg) {}
    explicit favicon_exception(std::string const & what_msg) : snap_exception("Favorite Icon", what_msg) {}
    explicit favicon_exception(QString const &     what_msg) : snap_exception("Favorite Icon", what_msg) {}
};



class favicon
        : public plugins::plugin
        , public path::path_execute
{
public:
                            favicon();
                            ~favicon();

    // plugins::plugin implementation
    static favicon *        instance();
    virtual QString         settings_path() const;
    virtual QString         icon() const;
    virtual QString         description() const;
    virtual QString         dependencies() const;
    virtual int64_t         do_update(int64_t last_updated);
    virtual void            bootstrap(snap_child * snap);

    // server signal
    void                    on_improve_signature(QString const & path, QDomDocument doc, QDomElement signature_tag);

    // path::path_execute implementation
    virtual bool            on_path_execute(content::path_info_t & url);

    // path signals
    void                    on_can_handle_dynamic_path(content::path_info_t & ipath, path::dynamic_plugin_t & plugin_info);

    // layout signals
    void                    on_generate_header_content(content::path_info_t & ipath, QDomElement & header, QDomElement & metadata);
    void                    on_generate_page_content(content::path_info_t & ipath, QDomElement & page, QDomElement & body);

private:
    void                    content_update(int64_t variables_timestamp);
    void                    output(content::path_info_t & ipath);
    void                    get_icon(content::path_info_t & cpath, content::field_search::search_result_t & result);
    void                    save_clicked_icon();

    snap_child *            f_snap = nullptr;
};


} // namespace favicon
} // namespace snap
// vim: ts=4 sw=4 et
