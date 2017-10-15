// Snap Websites Server -- manage permissions for users, forms, etc.
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
#include "../layout/layout.h"


namespace snap
{
namespace permissions
{

enum class name_t
{
    SNAP_NAME_PERMISSIONS_ACTION_ADMINISTER,
    SNAP_NAME_PERMISSIONS_ACTION_DELETE,
    SNAP_NAME_PERMISSIONS_ACTION_EDIT,
    SNAP_NAME_PERMISSIONS_ACTION_NAMESPACE,
    SNAP_NAME_PERMISSIONS_ACTION_PATH,
    SNAP_NAME_PERMISSIONS_ACTION_VIEW,
    SNAP_NAME_PERMISSIONS_ADMINISTER_NAMESPACE,
    SNAP_NAME_PERMISSIONS_CHECK_PERMISSIONS,
    SNAP_NAME_PERMISSIONS_DIRECT_ACTION_ADMINISTER,
    SNAP_NAME_PERMISSIONS_DIRECT_ACTION_DELETE,
    SNAP_NAME_PERMISSIONS_DIRECT_ACTION_EDIT,
    SNAP_NAME_PERMISSIONS_DIRECT_ACTION_VIEW,
    SNAP_NAME_PERMISSIONS_DIRECT_GROUP,
    SNAP_NAME_PERMISSIONS_DIRECT_GROUP_RETURNING_REGISTERED_USER,
    SNAP_NAME_PERMISSIONS_DIRECT_NAMESPACE,
    SNAP_NAME_PERMISSIONS_DYNAMIC,
    SNAP_NAME_PERMISSIONS_EDIT_NAMESPACE,
    SNAP_NAME_PERMISSIONS_GROUP_NAMESPACE,
    //SNAP_NAME_PERMISSIONS_GROUPS,
    SNAP_NAME_PERMISSIONS_GROUPS_PATH,
    SNAP_NAME_PERMISSIONS_LAST_UPDATED,
    SNAP_NAME_PERMISSIONS_LINK_BACK_ADMINISTER,
    SNAP_NAME_PERMISSIONS_LINK_BACK_DELETE,
    SNAP_NAME_PERMISSIONS_LINK_BACK_EDIT,
    SNAP_NAME_PERMISSIONS_LINK_BACK_GROUP,
    SNAP_NAME_PERMISSIONS_LINK_BACK_NAMESPACE,
    SNAP_NAME_PERMISSIONS_LINK_BACK_VIEW,
    SNAP_NAME_PERMISSIONS_LOGIN_STATUS_SPAMMER,
    SNAP_NAME_PERMISSIONS_LOGIN_STATUS_VISITOR,
    SNAP_NAME_PERMISSIONS_LOGIN_STATUS_RETURNING_VISITOR,
    SNAP_NAME_PERMISSIONS_LOGIN_STATUS_RETURNING_REGISTERED,  // partial log in
    SNAP_NAME_PERMISSIONS_LOGIN_STATUS_REGISTERED,            // full log in
    SNAP_NAME_PERMISSIONS_MAKE_ADMINISTRATOR,
    SNAP_NAME_PERMISSIONS_MAKE_ROOT,
    SNAP_NAME_PERMISSIONS_NAMESPACE,
    SNAP_NAME_PERMISSIONS_PATH,
    SNAP_NAME_PERMISSIONS_PLUGIN,
    SNAP_NAME_PERMISSIONS_RIGHTS_PATH,
    SNAP_NAME_PERMISSIONS_SECURE_PAGE,
    SNAP_NAME_PERMISSIONS_SECURE_SITE,
    SNAP_NAME_PERMISSIONS_STATUS_PATH,
    SNAP_NAME_PERMISSIONS_USERS_PATH,
    SNAP_NAME_PERMISSIONS_VIEW_NAMESPACE
};
char const * get_name(name_t name) __attribute__ ((const));



class permissions_exception : public snap_exception
{
public:
    explicit permissions_exception(char const *        what_msg) : snap_exception("Permissions", what_msg) {}
    explicit permissions_exception(std::string const & what_msg) : snap_exception("Permissions", what_msg) {}
    explicit permissions_exception(QString const &     what_msg) : snap_exception("Permissions", what_msg) {}
};

class permissions_exception_invalid_group_name : public permissions_exception
{
public:
    explicit permissions_exception_invalid_group_name(char const *        what_msg) : permissions_exception(what_msg) {}
    explicit permissions_exception_invalid_group_name(std::string const & what_msg) : permissions_exception(what_msg) {}
    explicit permissions_exception_invalid_group_name(QString const &     what_msg) : permissions_exception(what_msg) {}
};

class permissions_exception_invalid_path : public permissions_exception
{
public:
    explicit permissions_exception_invalid_path(char const *        what_msg) : permissions_exception(what_msg) {}
    explicit permissions_exception_invalid_path(std::string const & what_msg) : permissions_exception(what_msg) {}
    explicit permissions_exception_invalid_path(QString const &     what_msg) : permissions_exception(what_msg) {}
};




class permissions
        : public plugins::plugin
        , public links::links_cloned
        , public layout::layout_content
        , public server::backend_action
{
public:
    static int64_t const        EXPECTED_TIME_ACCURACY_EPSILON = 10000; // 10ms

    class sets_t
    {
    public:
        typedef QVector<QString>        set_t;
        typedef QMap<QString, set_t>    req_sets_t;

                                sets_t(snap_child * snap, QString const & user_path, content::path_info_t & ipath, QString const & action, QString const & login_status);
                                ~sets_t();

        void                    set_login_status(QString const & status);
        QString const &         get_login_status() const;
                       
        void                    modified_user_permissions();
        bool                    read_from_user_cache();
        void                    save_to_user_cache();
        QString const &         get_user_cache_key();
        QString const &         get_user_path() const;
        content::path_info_t &  get_ipath() const;
        QString const &         get_action() const;

        void                    add_user_right(QString right);
        int                     get_user_rights_count() const;
        set_t const &           get_user_rights() const;

        void                    modified_plugin_permissions();
        bool                    read_from_plugin_cache();
        void                    save_to_plugin_cache();
        QString const &         get_plugin_cache_key();
        void                    add_plugin_permission(QString const & plugin, QString right);
        int                     get_plugin_rights_count() const;
        req_sets_t const &      get_plugin_rights() const;

        bool                    is_root() const;
        bool                    allowed() const;

    private:
                                sets_t(sets_t const &) = delete;
        sets_t                  operator = (sets_t const &) = delete;

        void                    get_cache_table();

        snap_child *                    f_snap = nullptr;
        QString                         f_user_path;
        content::path_info_t &          f_ipath;
        QString                         f_action;
        QString                         f_login_status;
        set_t                           f_user_rights;
        QString                         f_user_cache_key;
        req_sets_t                      f_plugin_permissions;
        QString                         f_plugin_cache_key;
        bool                            f_using_user_cache = false;
        bool                            f_user_cache_reset = false;
        bool                            f_using_plugin_cache = false;
        bool                            f_plugin_cache_reset = false;
        bool                            f_modified_user_permissions = false;
        bool                            f_modified_plugin_permissions = false;
    };

    enum secure_mode_t
    {
        SECURE_MODE_NO,
        SECURE_MODE_PER_PAGE,
        SECURE_MODE_ALWAYS
    };

                            permissions();
                            ~permissions();

    // plugins::plugin implementation
    static permissions *    instance();
    virtual QString         description() const;
    virtual QString         dependencies() const;
    virtual int64_t         do_update(int64_t last_updated);
    virtual void            bootstrap(snap_child * snap);

    // server::backend_action implementation
    virtual void            on_backend_action(QString const & action);

    // server signals
    void                    on_register_backend_action(server::backend_action_set & actions);
    void                    on_add_snap_expr_functions(snap_expr::functions_t & functions);

    // path signals
    void                    on_access_allowed(QString const & user_path, content::path_info_t & ipath, QString const & action, QString const & login_status, content::permission_flag & result);
    void                    on_validate_action(content::path_info_t & path, QString const & action, permission_error_callback & err_callback);
    void                    on_check_for_redirect(content::path_info_t & ipath);

    // layout::layout_content implementation
    virtual void            on_generate_main_content(content::path_info_t & ipath, QDomElement & page, QDomElement & body);

    // layout signals
    void                    on_generate_header_content(content::path_info_t & path, QDomElement & hader, QDomElement & metadata);

    // users signals
    void                    on_user_verified(content::path_info_t & ipath, int64_t identifier);

    // link signals
    void                    on_modified_link(links::link_info const & link, bool const created);

    // link_cloned implementation
    virtual void            repair_link_of_cloned_page(QString const & clone, snap_version::version_number_t branch_number, links::link_info const & source, links::link_info const & destination, bool const cloning);

    SNAP_SIGNAL(get_user_rights, (permissions *perms, sets_t & sets), (perms, sets));
    SNAP_SIGNAL(get_plugin_permissions, (permissions *perms, sets_t & sets), (perms, sets));
    SNAP_SIGNAL(permit_redirect_to_login_on_not_allowed, (content::path_info_t & ipath, bool & redirect_to_login), (ipath, redirect_to_login));

    void                    add_user_rights(QString const  & right, sets_t & sets);
    void                    add_plugin_permissions(QString const & plugin_name, QString const & group, sets_t & sets);
    QString const &         get_login_status();
    QString const &         get_user_path();
    void                    reset_permissions_cache();

private:
    void                    content_update(int64_t variables_timestamp);
    void                    recursive_add_user_rights(QString const & key, sets_t & sets);
    void                    recursive_add_plugin_permissions(QString const & plugin_name, QString const & key, sets_t & sets);
    void                    check_permissions(QString const & email, QString const & page, QString const & action, QString const & status);

    snap_child *                f_snap = nullptr;
    QString                     f_login_status;
    bool                        f_has_user_path = false;
    QString                     f_user_path;
    std::map<QString, bool>     f_valid_actions;
};

} // namespace permissions
} // namespace snap
// vim: ts=4 sw=4 et
