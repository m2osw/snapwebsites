// Snap Websites Server -- feed management (RSS like feeds and aggregators)
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
#include "../content/content.h"


namespace snap
{
namespace feed
{


enum class name_t
{
    SNAP_NAME_FEED_ADMIN_SETTINGS,
    SNAP_NAME_FEED_AGE,
    SNAP_NAME_FEED_ATTACHMENT_TYPE,
    SNAP_NAME_FEED_DESCRIPTION,
    SNAP_NAME_FEED_EXTENSION,
    SNAP_NAME_FEED_MIMETYPE,
    SNAP_NAME_FEED_PAGE_LAYOUT,
    SNAP_NAME_FEED_SETTINGS_ALLOW_MAIN_ATOM_XML,
    SNAP_NAME_FEED_SETTINGS_ALLOW_MAIN_RSS_XML,
    SNAP_NAME_FEED_SETTINGS_DEFAULT_LOGO,
    SNAP_NAME_FEED_SETTINGS_PATH,
    SNAP_NAME_FEED_SETTINGS_TEASER_END_MARKER,
    SNAP_NAME_FEED_SETTINGS_TEASER_TAGS,
    SNAP_NAME_FEED_SETTINGS_TEASER_WORDS,
    SNAP_NAME_FEED_SETTINGS_TOP_MAXIMUM_NUMBER_OF_ITEMS_IN_ANY_FEED,
    SNAP_NAME_FEED_TITLE,
    SNAP_NAME_FEED_TTL,
    SNAP_NAME_FEED_TYPE
};
char const * get_name(name_t name) __attribute__ ((const));


class feed_exception : public snap_exception
{
public:
    explicit feed_exception(char const *        what_msg) : snap_exception("Feed", what_msg) {}
    explicit feed_exception(std::string const & what_msg) : snap_exception("Feed", what_msg) {}
    explicit feed_exception(QString const &     what_msg) : snap_exception("Feed", what_msg) {}
};



class feed
        : public plugins::plugin
{
public:
    static int const    DEFAULT_TEASER_WORDS = 200;
    static int const    DEFAULT_TEASER_TAGS  = 100;

                        feed();
                        ~feed();

    // plugins::plugin implementation
    static feed *       instance();
    virtual QString     settings_path() const;
    virtual QString     icon() const;
    virtual QString     description() const;
    virtual QString     dependencies() const;
    virtual int64_t     do_update(int64_t last_updated);
    virtual void        bootstrap(snap_child * snap);

    // server signals
    void                on_backend_process();

    // layout signals
    void                on_generate_page_content(content::path_info_t & ipath, QDomElement & page, QDomElement & body);

    // editor signals
    void                on_finish_editor_form_processing(content::path_info_t & ipath, bool & succeeded);

private:
    void                content_update(int64_t variables_timestamp);
    void                generate_feeds();
    void                mark_attachment_as_feed(snap::content::attachment_file & attachment);

    snap_child *        f_snap = nullptr;
    QString             f_feed_parser_xsl;
};


} // namespace feed
} // namespace snap
// vim: ts=4 sw=4 et
