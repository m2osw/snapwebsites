// Snap Websites Server -- handle the JavaScript WYSIWYG editor
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
#include "../editor/editor.h"


namespace snap
{
namespace date_widgets
{


enum class name_t
{
    SNAP_NAME_DATE_WIDGETS_DROPDOWN_TYPE
};
char const * get_name(name_t name) __attribute__ ((const));


class date_widgets
    : public serverplugins::plugin
{
public:
    SERVERPLUGINS_DEFAULTS(date_widgets);

    // serverplugins::plugin implementation
    virtual void            bootstrap() override;
    virtual time_t          do_update(time_t last_updated, unsigned int phase) override;

    // editor signals
    void                    on_prepare_editor_form(editor::editor * e);
    void                    on_value_to_string(editor::editor::value_to_string_info_t & value_info);
    void                    on_string_to_value(editor::editor::string_to_value_info_t & value_info);
    void                    on_init_editor_widget(content::path_info_t & ipath, QString const & field_id, QString const & field_type, QDomElement & widget, libdbproxy::row::pointer_t row);
    void                    on_validate_editor_post_for_widget(content::path_info_t & ipath, sessions::sessions::session_info & info, QDomElement const & widget, QString const & widget_name, QString const & widget_type, QString const & value, bool const is_secret);

private:
    void                    content_update(int64_t variables_timestamp);
    QString                 range_to_year(QString const range_date);

    snap_child *            f_snap = nullptr;
};

} // namespace date_widgets
} // namespace snap
// vim: ts=4 sw=4 et
