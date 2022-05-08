// Snap Websites Server -- shorturl management (smaller URLs for all pages)
// Copyright (c) 2013-2019  Made to Order Software Corp.  All Rights Reserved
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
#include    "../path/path.h"
#include    "../sessions/sessions.h"


namespace snap
{
namespace shorturl
{


enum class name_t
{
    SNAP_NAME_SHORTURL_DATE,
    SNAP_NAME_SHORTURL_IDENTIFIER,
    SNAP_NAME_SHORTURL_ID_ROW,
    SNAP_NAME_SHORTURL_INDEX_ROW,
    SNAP_NAME_SHORTURL_NO_SHORTURL,
    SNAP_NAME_SHORTURL_TABLE,
    SNAP_NAME_SHORTURL_URL
};
char const * get_name(name_t name) __attribute__ ((const));


DECLARE_MAIN_EXCEPTION(shorturl_exception);



class shorturl
    : public serverplugins::plugin
    , public path::path_execute
    , public layout::layout_content
{
public:
    SERVERPLUGINS_DEFAULTS(shorturl);

    // serverplugins::plugin implementation
    virtual void        bootstrap() override;
    virtual time_t      do_update(time_t last_updated, unsigned int phase) override;

    libdbproxy::table::pointer_t get_shorturl_table();

    // content signals
    void                on_create_content(content::path_info_t & path, const QString & owner, const QString & type);
    void                on_page_cloned(content::content::cloned_tree_t const & tree);

    // layout signals
    void                on_generate_header_content(content::path_info_t & path, QDomElement & header, QDomElement & metadata);

    // layout::layout_content implementation
    virtual void        on_generate_main_content(content::path_info_t & path, QDomElement & page, QDomElement & body) override;

    // path signals
    void                on_check_for_redirect(content::path_info_t & ipath);
    //void                on_can_handle_dynamic_path(content::path_info_t & ipath, path::dynamic_plugin_t & plugin_info);

    // path::path_execute implementation
    virtual bool        on_path_execute(content::path_info_t & ipath) override;

    QString             get_shorturl(QString const & id, int base);
    QString             get_shorturl(uint64_t identifier);

    SNAP_SIGNAL(allow_shorturl, (content::path_info_t & ipath, QString const & owner, QString const & type, bool & allow), (ipath, owner, type, allow));

private:
    void                content_update(int64_t variables_timestamp);

    snap_child *                    f_snap = nullptr;
    libdbproxy::table::pointer_t    f_shorturl_table = libdbproxy::table::pointer_t();
};


} // namespace shorturl
} // namespace snap
// vim: ts=4 sw=4 et
