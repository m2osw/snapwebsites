// Snap Websites Server -- offer a plethora of localized editor widgets
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
#include "../editor/editor.h"


/** \file
 * \brief Header of the locale widgets plugin.
 *
 * The file defines the various locale_widgets plugin classes.
 */

namespace snap
{
namespace locale_widgets
{


//enum class name_t
//{
//    SNAP_NAME_LOCALE_WIDGETS_NAME
//};
//char const * get_name(name_t name) __attribute__ ((const));


//class locale_widgets_exception : public snap_exception
//{
//public:
//    explicit locale_widgets_exception(char const *        what_msg) : snap_exception("locale_widgets", what_msg) {}
//    explicit locale_widgets_exception(std::string const & what_msg) : snap_exception("locale_widgets", what_msg) {}
//    explicit locale_widgets_exception(QString const &     what_msg) : snap_exception("locale_widgets", what_msg) {}
//};








class locale_widgets : public plugins::plugin
{
public:
    // TODO: this seems to be duplicated from `class locale`
    //
    // the ICU library only gives us the timezone full name,
    // continent and city all the other parameters will be empty
    struct timezone_info_t
    {
        QString         f_2country;         // 2 letter country code
        int64_t         f_longitude = 0;    // city longitude
        int64_t         f_latitude = 0;     // city latitude
        QString         f_timezone_name;    // the full name of the timezone as is
        QString         f_continent;        // one of the 5 continents and a few other locations
        QString         f_country_or_state; // likely empty (Used for Argentina, Kentucky, Indiana...)
        QString         f_city;             // The main city for that timezone
        QString         f_comment;          // likely empty, a comment about this timezone
    };
    typedef QVector<timezone_info_t>    timezone_list_t;

                                locale_widgets();
                                ~locale_widgets();

    // plugin.cpp implementation
    static locale_widgets *     instance();
    virtual QString             settings_path() const;
    virtual QString             icon() const;
    virtual QString             description() const;
    virtual QString             help_uri() const;
    virtual QString             dependencies() const;
    virtual int64_t             do_update(int64_t last_updated);
    virtual void                bootstrap(snap_child * snap);

    // editor signals
    void                        on_init_editor_widget(content::path_info_t & ipath, QString const & field_id, QString const & field_type, QDomElement & widget, libdbproxy::row::pointer_t row);
    void                        on_prepare_editor_form(editor::editor * e);
    void                        on_string_to_value(editor::editor::string_to_value_info_t & value_info);
    void                        on_value_to_string(editor::editor::value_to_string_info_t & value_info);
    void                        on_validate_editor_post_for_widget(content::path_info_t & ipath, sessions::sessions::session_info & info, QDomElement const & widget, QString const & widget_name, QString const & widget_type, QString const & value, bool const is_secret);

private:
    void                        content_update(int64_t variables_timestamp);

    snap_child *                f_snap = nullptr;
};


} // namespace locale_widgets
} // namespace snap
// vim: ts=4 sw=4 et
