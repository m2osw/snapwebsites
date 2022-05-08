// Copyright (c) 2016-2019  Made to Order Software Corp.  All Rights Reserved
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


DECLARE_MAIN_EXCEPTION(detectadblocker_exception);

DECLARE_EXCEPTION(detectadblocker_exception, detectadblocker_exception_invalid_path);



class detectadblocker
    : public serverplugins::plugin
    , public path::path_execute
{
public:
    SERVERPLUGINS_DEFAULTS(detectadblocker);

    // serverplugins implementation
    virtual void            bootstrap() override;
    virtual time_t          do_update(time_t last_updated, unsigned int phase) override;

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
