// Snap Websites Server -- cookie consent using silktide JavaScript plugin
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
 * \brief Header of the cookie consent silktide plugin.
 *
 * The file defines the various cookie consent silktide plugin classes.
 */

namespace snap
{
namespace cookie_consent_silktide
{


enum class name_t
{
    SNAP_NAME_COOKIE_CONSENT_SILKTIDE_CONSENT_DURATION,
    SNAP_NAME_COOKIE_CONSENT_SILKTIDE_DISMISS,
    SNAP_NAME_COOKIE_CONSENT_SILKTIDE_DOMAIN,
    SNAP_NAME_COOKIE_CONSENT_SILKTIDE_JAVASCRIPT_FILENAME,
    SNAP_NAME_COOKIE_CONSENT_SILKTIDE_JAVASCRIPT_OPTIONS,
    SNAP_NAME_COOKIE_CONSENT_SILKTIDE_JAVASCRIPT_OPTIONS_DEPENDENCY,
    SNAP_NAME_COOKIE_CONSENT_SILKTIDE_JAVASCRIPT_OPTIONS_PARENT_PATH,
    SNAP_NAME_COOKIE_CONSENT_SILKTIDE_JAVASCRIPT_PLUGIN,
    SNAP_NAME_COOKIE_CONSENT_SILKTIDE_JAVASCRIPT_TYPE,
    SNAP_NAME_COOKIE_CONSENT_SILKTIDE_JAVASCRIPT_VERSION,
    SNAP_NAME_COOKIE_CONSENT_SILKTIDE_LEARN_MORE_LABEL,
    SNAP_NAME_COOKIE_CONSENT_SILKTIDE_LEARN_MORE_URI,
    SNAP_NAME_COOKIE_CONSENT_SILKTIDE_MESSAGE,
    SNAP_NAME_COOKIE_CONSENT_SILKTIDE_PATH,
    SNAP_NAME_COOKIE_CONSENT_SILKTIDE_THEME
};
char const * get_name(name_t name) __attribute__ ((const));


//class cookie_consent_silktide_exception : public snap_exception
//{
//public:
//    explicit cookie_consent_silktide_exception(char const *        what_msg) : snap_exception("locale_settings", what_msg) {}
//    explicit cookie_consent_silktide_exception(std::string const & what_msg) : snap_exception("locale_settings", what_msg) {}
//    explicit cookie_consent_silktide_exception(QString const &     what_msg) : snap_exception("locale_settings", what_msg) {}
//};








class cookie_consent_silktide
        : public plugins::plugin
{
public:
                                cookie_consent_silktide();
                                ~cookie_consent_silktide();

    // plugins::plugin implementation
    static cookie_consent_silktide * instance();
    virtual QString             settings_path() const;
    virtual QString             icon() const;
    virtual QString             description() const;
    virtual QString             dependencies() const;
    virtual int64_t             do_update(int64_t last_updated);
    virtual void                bootstrap(snap_child * snap);

    // content signals
    void                        on_generate_header_content(content::path_info_t & ipath, QDomElement & header, QDomElement & metadata);

    // editor signals
    void                        on_save_editor_fields(editor::save_info_t & save_info);

private:
    void                        content_update(int64_t variables_timestamp);

    snap_child *                f_snap = nullptr;
};


} // namespace cookie_consent_silktide
} // namespace snap
// vim: ts=4 sw=4 et
