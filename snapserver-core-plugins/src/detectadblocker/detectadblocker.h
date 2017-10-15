// Snap Websites Server -- detectadblocker structures
// Copyright (C) 2016-2017  Made to Order Software Corp.
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
namespace detectadblocker
{


enum class name_t
{
    SNAP_NAME_DETECTADBLOCKER_INFORM_SERVER,
    SNAP_NAME_DETECTADBLOCKER_PATH,
    SNAP_NAME_DETECTADBLOCKER_PREVENT_ADS_DURATION,
    SNAP_NAME_DETECTADBLOCKER_SETTINGS_PATH,
    SNAP_NAME_DETECTADBLOCKER_STATUS_SESSION_NAME
};
char const * get_name(name_t name) __attribute__ ((const));


class detectadblocker_exception : public snap_exception
{
public:
    explicit detectadblocker_exception(char const *        what_msg) : snap_exception("DetectAdBlocker", what_msg) {}
    explicit detectadblocker_exception(std::string const & what_msg) : snap_exception("DetectAdBlocker", what_msg) {}
    explicit detectadblocker_exception(QString const &     what_msg) : snap_exception("DetectAdBlocker", what_msg) {}
};

class detectadblocker_exception_invalid_path : public detectadblocker_exception
{
public:
    explicit detectadblocker_exception_invalid_path(char const *        what_msg) : detectadblocker_exception(what_msg) {}
    explicit detectadblocker_exception_invalid_path(std::string const & what_msg) : detectadblocker_exception(what_msg) {}
    explicit detectadblocker_exception_invalid_path(QString const &     what_msg) : detectadblocker_exception(what_msg) {}
};



class detectadblocker
        : public plugins::plugin
        , public path::path_execute
{
public:
                            detectadblocker();
                            ~detectadblocker();

    // plugin implementation
    static detectadblocker *instance();
    virtual QString         settings_path() const;
    virtual QString         icon() const;
    virtual QString         description() const;
    virtual QString         dependencies() const;
    virtual int64_t         do_update(int64_t last_updated);
    virtual void            bootstrap(snap_child * snap);

    // snapwebsites signals
    void                    on_detach_from_session();

    // path_execute implementation
    virtual bool            on_path_execute(content::path_info_t & ipath);

    // path signals
    void                    on_check_for_redirect(content::path_info_t & ipath);

    // layout signals
    void                    on_generate_header_content(content::path_info_t & ipath, QDomElement & header, QDomElement & metadata);

    bool                    was_detected() const;

private:
    void                    content_update(int64_t variables_timestamp);

    snap_child *            f_snap = nullptr;
    bool                    f_detected = false;
};


} // namespace detectadblocker
} // namespace snap
// vim: ts=4 sw=4 et
