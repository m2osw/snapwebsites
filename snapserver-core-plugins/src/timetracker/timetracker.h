// Snap Websites Server -- timetracker structures
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
#include "../editor/editor.h"


namespace snap
{
namespace timetracker
{


enum class name_t
{
    SNAP_NAME_TIMETRACKER_BILLING_DURATION,
    SNAP_NAME_TIMETRACKER_DATE_QUERY_STRING,
    SNAP_NAME_TIMETRACKER_LOCATION,
    SNAP_NAME_TIMETRACKER_MAIN_PAGE,
    SNAP_NAME_TIMETRACKER_PATH,
    SNAP_NAME_TIMETRACKER_TRANSPORTATION
};
char const * get_name(name_t name) __attribute__ ((const));


class timetracker_exception : public snap_exception
{
public:
    explicit timetracker_exception(char const *        what_msg) : snap_exception("TimeTracker", what_msg) {}
    explicit timetracker_exception(std::string const & what_msg) : snap_exception("TimeTracker", what_msg) {}
    explicit timetracker_exception(QString const &     what_msg) : snap_exception("TimeTracker", what_msg) {}
};

class timetracker_exception_invalid_path : public timetracker_exception
{
public:
    explicit timetracker_exception_invalid_path(char const *        what_msg) : timetracker_exception(what_msg) {}
    explicit timetracker_exception_invalid_path(std::string const & what_msg) : timetracker_exception(what_msg) {}
    explicit timetracker_exception_invalid_path(QString const &     what_msg) : timetracker_exception(what_msg) {}
};



class timetracker
        : public plugins::plugin
        , public path::path_execute
        , public layout::layout_content
{
public:
                            timetracker();
                            ~timetracker();

    // plugin implementation
    static timetracker *    instance();
    virtual QString         settings_path() const;
    virtual QString         icon() const;
    virtual QString         description() const;
    virtual QString         dependencies() const;
    virtual int64_t         do_update(int64_t last_updated);
    virtual void            bootstrap(snap_child * snap);

    // path_execute implementation
    virtual bool            on_path_execute(content::path_info_t & ipath);

    // path signals
    //void                    on_can_handle_dynamic_path(content::path_info_t & ipath, path::dynamic_plugin_t & plugin_info);
    void                    on_check_for_redirect(content::path_info_t & ipath);

    // layout_content implementation
    virtual void            on_generate_main_content(content::path_info_t & ipath, QDomElement & page, QDomElement & body);

    // layout signals
    void                    on_generate_header_content(content::path_info_t & ipath, QDomElement & header, QDomElement & metadata);

    // filter signals
    void                    on_replace_token(content::path_info_t & ipath, QDomDocument & xml, filter::filter::token_info_t & token);
    void                    on_token_help(filter::filter::token_help_t & help);

    // editor signals
    //void                    on_finish_editor_form_processing(content::path_info_t & ipath, bool & succeeded);
    void                    on_init_editor_widget(content::path_info_t  & ipath, QString const  & field_id, QString const  & field_type, QDomElement  & widget, libdbproxy::row::pointer_t row);

private:
    void                    content_update(int64_t variables_timestamp);

    void                    add_calendar(int64_t identifier);
    QString                 token_main_page(content::path_info_t & ipath);
    QString                 token_calendar(content::path_info_t & ipath);
    void                    init_day_editor_widgets(QString const & field_id, QDomElement & widget);

    snap_child *            f_snap = nullptr;
};


} // namespace timetracker
} // namespace snap
// vim: ts=4 sw=4 et
