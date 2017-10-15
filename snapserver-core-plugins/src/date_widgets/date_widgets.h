// Snap Websites Server -- handle the JavaScript WYSIWYG editor
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
#include "../editor/editor.h"


namespace snap
{
namespace date_widgets
{

//class date_widgets_exception : public snap_exception
//{
//public:
//    explicit date_widgets_exception(char const *        what_msg) : snap_exception("date_widgets", what_msg) {}
//    explicit date_widgets_exception(std::string const & what_msg) : snap_exception("date_widgets", what_msg) {}
//    explicit date_widgets_exception(QString const &     what_msg) : snap_exception("date_widgets", what_msg) {}
//};
//
//class date_widgets_exception_invalid_argument : public date_widgets_exception
//{
//public:
//    explicit date_widgets_exception_invalid_argument(char const *        what_msg) : editor_exception(what_msg) {}
//    explicit date_widgets_exception_invalid_argument(std::string const & what_msg) : editor_exception(what_msg) {}
//    explicit date_widgets_exception_invalid_argument(QString const &     what_msg) : editor_exception(what_msg) {}
//};



enum class name_t
{
    SNAP_NAME_DATE_WIDGETS_DROPDOWN_TYPE
};
char const * get_name(name_t name) __attribute__ ((const));


class date_widgets
        : public plugins::plugin
{
public:
                            date_widgets();
                            ~date_widgets();

    // plugins::plugin implementation
    static date_widgets *   instance();
    //virtual QString         settings_path() const;
    virtual QString         icon() const;
    virtual QString         description() const;
    virtual QString         dependencies() const;
    virtual int64_t         do_update(int64_t last_updated);
    virtual void            bootstrap(snap_child * snap);

    // editor signals
    void                    on_prepare_editor_form(editor::editor * e);
    void                    on_value_to_string(editor::editor::value_to_string_info_t & value_info);
    void                    on_string_to_value(editor::editor::string_to_value_info_t & value_info);
    void                    on_init_editor_widget(content::path_info_t & ipath, QString const & field_id, QString const & field_type, QDomElement & widget, QtCassandra::QCassandraRow::pointer_t row);
    void                    on_validate_editor_post_for_widget(content::path_info_t & ipath, sessions::sessions::session_info & info, QDomElement const & widget, QString const & widget_name, QString const & widget_type, QString const & value, bool const is_secret);

private:
    void                    content_update(int64_t variables_timestamp);
    QString                 range_to_year(QString const range_date);

    snap_child *            f_snap = nullptr;
};

} // namespace date_widgets
} // namespace snap
// vim: ts=4 sw=4 et
