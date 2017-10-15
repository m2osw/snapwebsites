// Snap Websites Server -- oauth2 handling
// Copyright (C) 2012-2017  Made to Order Software Corp.
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
#include "../layout/layout.h"
#include "../path/path.h"


namespace snap
{
namespace oauth2
{

enum class name_t
{
    SNAP_NAME_OAUTH2_EMAIL,
    SNAP_NAME_OAUTH2_ENABLE,
    SNAP_NAME_OAUTH2_IDENTIFIER,
    SNAP_NAME_OAUTH2_IDENTIFIERS,
    SNAP_NAME_OAUTH2_SECRET,
    SNAP_NAME_OAUTH2_USER_ENABLE
};
char const * get_name(name_t name) __attribute__ ((const));



class oauth2_exception : public snap_exception
{
public:
    explicit oauth2_exception(char const *        what_msg) : snap_exception("oauth2", what_msg) {}
    explicit oauth2_exception(std::string const & what_msg) : snap_exception("oauth2", what_msg) {}
    explicit oauth2_exception(QString const &     what_msg) : snap_exception("oauth2", what_msg) {}
};







class oauth2
    : public plugins::plugin
    , public path::path_execute
{
public:
                            oauth2();
    virtual                 ~oauth2();

    // plugins::plugin implementation
    static oauth2 *         instance();
    virtual QString         settings_path() const;
    virtual QString         icon() const;
    virtual QString         description() const;
    virtual QString         dependencies() const;
    virtual int64_t         do_update(int64_t last_updated);
    virtual void            bootstrap(snap_child * snap);

    // server signals
    void                    on_process_cookies();

    // content signals
    void                    on_create_content(content::path_info_t & ipath, QString const & owner, QString const & type);

    // path::path_execute implementation
    virtual bool            on_path_execute(content::path_info_t & ipath);

    SNAP_SIGNAL_WITH_MODE(oauth2_authorized, (QString const & application), (application), NEITHER);
    SNAP_SIGNAL_WITH_MODE(oauth2_authenticated, (QString const & application), (application), NEITHER);

private:
    enum class oauth2_error_t
    {
        OAUTH2_INVALID_REQUEST,
        OAUTH2_INVALID_CLIENT,
        OAUTH2_INVALID_GRANT,
        OAUTH2_UNAUTHORIZED_CLIENT,
        OAUTH2_UNAUTHORIZED_GRANT_TYPE,
        OAUTH2_INVALID_SCOPE
    };

    void                    content_update(int64_t variables_timestamp);
    void                    require_oauth2_login();
    void                    application_login();
    void                    die(snap_child::http_code_t err_code, oauth2_error_t err_oauth2, QString err_name, QString const & err_description, QString const & err_details, QString const & err_help_uri);

    snap_child *            f_snap = nullptr;
};

} // namespace oauth2
} // namespace snap
// vim: ts=4 sw=4 et
