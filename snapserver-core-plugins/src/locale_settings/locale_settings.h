// Snap Websites Server -- handle various locale information such as timezone and date output, number formatting for display, etc.
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
 * \brief Header of the locale plugin.
 *
 * This header file is named "snap_locale.h" and not "locale.h" because
 * there is a system file "locale.h" and having the same name prevents
 * the system file from being included properly.
 *
 * The file defines the various locale plugin classes.
 */

namespace snap
{
namespace locale_settings
{


enum class name_t
{
    SNAP_NAME_LOCALE_SETTINGS_LOCALE,
    SNAP_NAME_LOCALE_SETTINGS_TIMEZONE,
    SNAP_NAME_LOCALE_SETTINGS_PATH
};
char const * get_name(name_t name) __attribute__ ((const));


//class locale_settings_exception : public snap_exception
//{
//public:
//    explicit locale_settings_exception(char const *        what_msg) : snap_exception("locale_settings", what_msg) {}
//    explicit locale_settings_exception(std::string const & what_msg) : snap_exception("locale_settings", what_msg) {}
//    explicit locale_settings_exception(QString const &     what_msg) : snap_exception("locale_settings", what_msg) {}
//};








class locale_settings
        : public plugins::plugin
{
public:
                                locale_settings();
                                ~locale_settings();

    // plugins::plugin implementation
    static locale_settings *    instance();
    virtual QString             settings_path() const;
    virtual QString             icon() const;
    virtual QString             description() const;
    virtual QString             help_uri() const;
    virtual QString             dependencies() const;
    virtual int64_t             do_update(int64_t last_updated);
    virtual void                bootstrap(snap_child * snap);

    // filter signals
    void                        on_replace_token(content::path_info_t & ipath, QDomDocument & xml, filter::filter::token_info_t & token);
    void                        on_token_help(filter::filter::token_help_t & help);

private:
    void                        content_update(int64_t variables_timestamp);

    snap_child *                f_snap = nullptr;
};


} // namespace locale_settings
} // namespace snap
// vim: ts=4 sw=4 et
