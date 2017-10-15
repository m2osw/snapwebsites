// Snap Websites Server -- path handling
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
#include "../content/content.h"


namespace snap
{
namespace path
{

//enum class name_t
//{
//    SNAP_NAME_PATH_PRIMARY_OWNER
//};
//char const * get_name(name_t name) __attribute__ ((const));


class dynamic_plugin_t
{
public:
                        dynamic_plugin_t() {}

    plugins::plugin *   get_plugin() const { return f_plugin; }
    void                set_plugin(plugins::plugin * p);

    plugins::plugin *   get_plugin_if_renamed() const { return f_plugin_if_renamed; }
    void                set_plugin_if_renamed(plugins::plugin * p, QString const & cpath);
    QString             get_renamed_path() const { return f_cpath_renamed; }

private:
                        // prevent copies or a user could reset the pointer!
                        dynamic_plugin_t(dynamic_plugin_t const & rhs);
                        dynamic_plugin_t & operator = (dynamic_plugin_t const & rhs);

    plugins::plugin *   f_plugin = nullptr;
    plugins::plugin *   f_plugin_if_renamed = nullptr;
    QString             f_cpath_renamed;
};


class path_execute
{
public:
    virtual         ~path_execute() {} // ensure proper virtual tables
    virtual bool    on_path_execute(content::path_info_t & ipath) = 0;
};


class path_error_callback : public permission_error_callback
{
public:
    path_error_callback(snap_child * snap, content::path_info_t & ipath);
    void set_plugin(plugins::plugin * p);
    void set_autologout(bool autologout = true);
    virtual void on_error(snap_child::http_code_t err_code, QString const & err_name, QString const & err_description, QString const & err_details, bool const err_by_mime_type);
    virtual void on_redirect(
            /* messages::set_error() */ QString const & err_name, QString const & err_description, QString const & err_details, bool err_security,
            /* snap_child::page_redirect() */ QString const & path, snap_child::http_code_t const http_code);

private:
    snap_child *            f_snap = nullptr;
    content::path_info_t &  f_ipath;
    plugins::plugin *       f_plugin = nullptr;
    bool                    f_autologout = false;
};


class path
        : public plugins::plugin
{
public:
                        path();
    virtual             ~path();

    // plugins::plugin implementation
    static path *       instance();
    virtual QString     description() const;
    virtual QString     dependencies() const;
    virtual void        bootstrap(snap_child * snap);

    // server signals
    void                on_init();
    void                on_execute(QString const & uri_path);
    void                on_improve_signature(QString const & url_path, QDomDocument doc, QDomElement signature);

    SNAP_SIGNAL(access_allowed, (QString const & user_path, content::path_info_t & ipath, QString const & action, QString const & login_status, content::permission_flag & result), (user_path, ipath, action, login_status, result));
    SNAP_SIGNAL_WITH_MODE(can_handle_dynamic_path, (content::path_info_t & ipath, dynamic_plugin_t & plugin_info), (ipath, plugin_info), NEITHER);
    SNAP_SIGNAL_WITH_MODE(page_not_found, (content::path_info_t & ipath), (ipath), NEITHER);
    SNAP_SIGNAL_WITH_MODE(validate_action, (content::path_info_t & ipath, QString const & action, permission_error_callback & err_callback), (ipath, action, err_callback), NEITHER);
    SNAP_SIGNAL(check_for_redirect, (content::path_info_t & ipath), (ipath));
    SNAP_SIGNAL_WITH_MODE(preprocess_path, (content::path_info_t & ipath, plugins::plugin *owner_plugin), (ipath, owner_plugin), NEITHER);

    plugin *            get_plugin(content::path_info_t & uri_path, permission_error_callback & err_callback);
    void                verify_permissions(content::path_info_t & ipath, permission_error_callback & err_callback);
    QString             define_action(content::path_info_t & ipath);
    void                handle_dynamic_path(plugins::plugin *p);
    void                add_restore_link_to_signature_for(QString const page_path);

private:
    snap_child *                    f_snap = nullptr;
    int64_t                         f_last_modified = 0;
    snap_string_list                f_add_restore_link_to_signature;
};

} // namespace path
} // namespace snap
// vim: ts=4 sw=4 et
