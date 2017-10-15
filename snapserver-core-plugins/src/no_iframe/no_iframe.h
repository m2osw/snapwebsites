// Snap Websites Server -- no_iframe, to prevent others from showing your site in an iframe
// Copyright (C) 2017  Made to Order Software Corp.
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
namespace no_iframe
{


enum class name_t
{
    SNAP_NAME_NO_IFRAME_ALLOW_PATH,
    SNAP_NAME_NO_IFRAME_DISALLOW_PATH,
    SNAP_NAME_NO_IFRAME_MODE,
    SNAP_NAME_NO_IFRAME_MODE_PATH,
    SNAP_NAME_NO_IFRAME_PAGE_MODE
};
char const * get_name(name_t name) __attribute__ ((const));


class no_iframe_exception : public snap_exception
{
public:
    explicit no_iframe_exception(char const *        what_msg) : snap_exception("No IFrame", what_msg) {}
    explicit no_iframe_exception(std::string const & what_msg) : snap_exception("No IFrame", what_msg) {}
    explicit no_iframe_exception(QString const &     what_msg) : snap_exception("No IFrame", what_msg) {}
};

class no_iframe_exception_invalid_path : public no_iframe_exception
{
public:
    explicit no_iframe_exception_invalid_path(char const *        what_msg) : no_iframe_exception(what_msg) {}
    explicit no_iframe_exception_invalid_path(std::string const & what_msg) : no_iframe_exception(what_msg) {}
    explicit no_iframe_exception_invalid_path(QString const &     what_msg) : no_iframe_exception(what_msg) {}
};



class no_iframe
        : public plugins::plugin
{
public:
                            no_iframe();
                            ~no_iframe();

    // plugin implementation
    static no_iframe *      instance();
    virtual QString         settings_path() const;
    virtual QString         icon() const;
    virtual QString         description() const;
    virtual QString         dependencies() const;
    virtual int64_t         do_update(int64_t last_updated);
    virtual void            bootstrap(snap_child * snap);

    // layout signals
    void                    on_generate_header_content(content::path_info_t & ipath, QDomElement & header, QDomElement & metadata);

private:
    void                    content_update(int64_t variables_timestamp);

    snap_child *            f_snap = nullptr;
};


} // namespace no_iframe
} // namespace snap
// vim: ts=4 sw=4 et
