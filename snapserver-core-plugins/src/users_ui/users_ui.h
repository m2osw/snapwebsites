// Snap Websites Server -- users handling
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
#include "../form/form.h"
#include "../layout/layout.h"
#include "../path/path.h"
#include "../users/users.h"
#include "../editor/editor.h"

namespace snap
{
namespace users_ui
{

class users_ui_exception : public snap_exception
{
public:
    explicit users_ui_exception(char const *        what_msg) : snap_exception("users_ui", what_msg) {}
    explicit users_ui_exception(std::string const & what_msg) : snap_exception("users_ui", what_msg) {}
    explicit users_ui_exception(QString const &     what_msg) : snap_exception("users_ui", what_msg) {}
};

class users_ui_exception_invalid_path : public users_ui_exception
{
public:
    explicit users_ui_exception_invalid_path(char const *        what_msg) : users_ui_exception(what_msg) {}
    explicit users_ui_exception_invalid_path(std::string const & what_msg) : users_ui_exception(what_msg) {}
    explicit users_ui_exception_invalid_path(QString const &     what_msg) : users_ui_exception(what_msg) {}
};







class users_ui
        : public plugins::plugin
        , public path::path_execute
        , public layout::layout_content
        , public layout::layout_boxes
        , public form::form_post
{
public:
                            users_ui();
    virtual                 ~users_ui();

    // plugins::plugin implementation
    static users_ui *       instance();
    virtual QString         settings_path() const;
    virtual QString         icon() const;
    virtual QString         description() const;
    virtual QString         help_uri() const;
    virtual QString         dependencies() const;
    virtual int64_t         do_update(int64_t last_updated);
    virtual void            bootstrap(::snap::snap_child * snap);

    // server signals
    void                    on_attach_to_session();
    void                    on_detach_from_session();

    // path::path_execute implementation
    bool                    on_path_execute(content::path_info_t & ipath);
    void                    on_check_for_redirect(content::path_info_t & ipath);

    // path signals
    void                    on_can_handle_dynamic_path(content::path_info_t & ipath, path::dynamic_plugin_t & plugin_info);

    // layout::layout_content implementation
    virtual void            on_generate_main_content(content::path_info_t & ipath, QDomElement & page, QDomElement & body);

    // layout::layout_boxes implementation
    virtual void            on_generate_boxes_content(content::path_info_t & page_ipath, content::path_info_t & ipath, QDomElement & page, QDomElement & boxes);

    // filter signals
    void                    on_replace_token(content::path_info_t & ipath, QDomDocument & xml, filter::filter::token_info_t & token);
    void                    on_token_help(filter::filter::token_help_t & help);

    // editor signals
    void                    on_init_editor_widget(content::path_info_t & ipath, QString const & field_name, QString const & field_type, QDomElement & widget, QtCassandra::QCassandraRow::pointer_t row);
    void                    on_finish_editor_form_processing(content::path_info_t & ipath, bool & succeeded);
    void					on_save_editor_fields(editor::save_info_t & save_info);

    // form stuff
    virtual void            on_process_form_post(content::path_info_t & ipath, sessions::sessions::session_info const & session_info);

    void                    send_to_replace_password_page(QString const & email, bool const set_status);
    bool                    resend_verification_email(QString const & email);

private:
    void                    content_update(int64_t variables_timestamp);
    void                    fix_owner_update(int64_t variables_timestamp);
    void                    show_user(content::path_info_t & cpath, QDomElement & page, QDomElement & body);
    void                    prepare_login_form();
    void                    logout_user(content::path_info_t & cpath, QDomElement & page, QDomElement & body);
    void                    prepare_basic_anonymous_form();
    void                    prepare_forgot_password_form();
    void                    prepare_password_form();
    void                    prepare_new_password_form();
    void                    process_login_form(users::users::login_mode_t login_mode);
    void                    prepare_verify_credentials_form();
    void                    process_register_form();
    void                    process_verify_form();
    void                    process_verify_resend_form();
    void                    process_forgot_password_form();
    void                    process_new_password_form();
    void                    process_password_form();
    void                    process_replace_password_form();
    void                    prepare_replace_password_form(QDomElement & body);
    void                    verify_email(QString const & email);
    void                    verify_password(content::path_info_t & cpath);
    void                    forgot_password_email(users::users::user_info_t const & user_info);

    void                    editor_widget_load_email_address(QDomElement & widget, QString const & id);

    snap_child *            f_snap = nullptr;
    QString                 f_user_changing_password_key;   // not quite logged in user
    bool                    f_user_changing_password_key_clear = true;
};

} // namespace users_ui
} // namespace snap
// vim: ts=4 sw=4 et
