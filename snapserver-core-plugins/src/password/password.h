// Snap Websites Server -- check password strength
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


namespace snap
{
namespace password
{


enum class name_t
{
    SNAP_NAME_PASSWORD_BLOCKED_USER_COUNTER,
    SNAP_NAME_PASSWORD_BLOCKED_USER_COUNTER_LIFETIME,
    SNAP_NAME_PASSWORD_BLOCKED_USER_FIREWALL_DURATION,
    SNAP_NAME_PASSWORD_CHECK_BLACKLIST,
    SNAP_NAME_PASSWORD_CHECK_USERNAME,
    SNAP_NAME_PASSWORD_CHECK_USERNAME_REVERSED,
    SNAP_NAME_PASSWORD_COUNT_BAD_PASSWORD_503S,
    SNAP_NAME_PASSWORD_COUNT_FAILURES,
    SNAP_NAME_PASSWORD_DELAY_BETWEEN_PASSWORD_CHANGES,
    SNAP_NAME_PASSWORD_EXISTS_IN_BLACKLIST,
    SNAP_NAME_PASSWORD_INVALID_PASSWORDS_BLOCK_DURATION,
    SNAP_NAME_PASSWORD_INVALID_PASSWORDS_COUNTER,
    SNAP_NAME_PASSWORD_INVALID_PASSWORDS_COUNTER_LIFETIME,
    SNAP_NAME_PASSWORD_INVALID_PASSWORDS_SLOWDOWN,
    SNAP_NAME_PASSWORD_LIMIT_DURATION,
    SNAP_NAME_PASSWORD_MAXIMUM_DURATION,
    SNAP_NAME_PASSWORD_MINIMUM_DIGITS,
    SNAP_NAME_PASSWORD_MINIMUM_LENGTH,
    SNAP_NAME_PASSWORD_MINIMUM_LENGTH_OF_VARIATIONS,
    SNAP_NAME_PASSWORD_MINIMUM_LETTERS,
    SNAP_NAME_PASSWORD_MINIMUM_LOWERCASE_LETTERS,
    SNAP_NAME_PASSWORD_MINIMUM_OLD_PASSWORDS,
    SNAP_NAME_PASSWORD_MINIMUM_SPACES,
    SNAP_NAME_PASSWORD_MINIMUM_SPECIALS,
    SNAP_NAME_PASSWORD_MINIMUM_UNICODE,
    SNAP_NAME_PASSWORD_MINIMUM_UPPERCASE_LETTERS,
    SNAP_NAME_PASSWORD_MINIMUM_VARIATION,
    SNAP_NAME_PASSWORD_OLD_PASSWORDS_MAXIMUM_AGE,
    SNAP_NAME_PASSWORD_PREVENT_OLD_PASSWORDS,
    SNAP_NAME_PASSWORD_TABLE
};
char const * get_name(name_t name) __attribute__ ((const));


class password_exception : public snap_exception
{
public:
    explicit password_exception(char const *        what_msg) : snap_exception("password", what_msg) {}
    explicit password_exception(std::string const & what_msg) : snap_exception("password", what_msg) {}
    explicit password_exception(QString const &     what_msg) : snap_exception("password", what_msg) {}
};

class password_exception_invalid_content_xml : public password_exception
{
public:
    explicit password_exception_invalid_content_xml(char const *        what_msg) : password_exception(what_msg) {}
    explicit password_exception_invalid_content_xml(std::string const & what_msg) : password_exception(what_msg) {}
    explicit password_exception_invalid_content_xml(QString const &     what_msg) : password_exception(what_msg) {}
};





class policy_t
{
public:
    explicit            policy_t(QString const & policy_name = QString());

    void                count_password_characters(QString const & password);

    bool                get_limit_duration() const;
    int64_t             get_maximum_duration() const;
    int64_t             get_minimum_length() const;
    int64_t             get_minimum_lowercase_letters() const;
    int64_t             get_minimum_uppercase_letters() const;
    int64_t             get_minimum_letters() const;
    int64_t             get_minimum_digits() const;
    int64_t             get_minimum_spaces() const;
    int64_t             get_minimum_specials() const;
    int64_t             get_minimum_unicode() const;
    int64_t             get_minimum_variation() const;
    int64_t             get_minimum_length_of_variations() const;
    bool                get_check_blacklist() const;
    int64_t             get_check_username() const;
    bool                get_check_username_reversed() const;
    bool                get_prevent_old_passwords() const;
    int64_t             get_minimum_old_passwords() const;
    int64_t             get_old_passwords_maximum_age() const;
    int64_t             get_delay_between_password_changes() const;
    int64_t             get_invalid_passwords_counter() const;
    int64_t             get_invalid_passwords_block_duration() const;
    int64_t             get_invalid_passwords_counter_lifetime() const;
    int64_t             get_invalid_passwords_slowdown() const;
    int64_t             get_blocked_user_counter() const;
    QString const &     get_blocked_user_firewall_duration() const;
    int64_t             get_blocked_user_counter_lifetime() const;

    QString             compare(policy_t const & rhs) const;
    QString             is_blacklisted(QString const & user_password) const;

private:
    int64_t             f_maximum_duration = 92; // 3 months in days
    int64_t             f_minimum_length = 0;
    int64_t             f_minimum_lowercase_letters = 0;
    int64_t             f_minimum_uppercase_letters = 0;
    int64_t             f_minimum_letters = 0;
    int64_t             f_minimum_digits = 0;
    int64_t             f_minimum_spaces = 0;
    int64_t             f_minimum_specials = 0;
    int64_t             f_minimum_unicode = 0;
    int64_t             f_minimum_variation = 0;
    int64_t             f_minimum_length_of_variations = 0;
    int64_t             f_minimum_old_passwords = 0;
    int64_t             f_old_passwords_maximum_age = 0;
    int64_t             f_check_username = 2;
    int64_t             f_delay_between_password_changes = 0;
    int64_t             f_invalid_passwords_counter = 5;
    int64_t             f_invalid_passwords_block_duration = 3;
    int64_t             f_invalid_passwords_counter_lifetime = 1;
    int64_t             f_invalid_passwords_slowdown = 1;
    int64_t             f_blocked_user_counter = 5;
    QString             f_blocked_user_firewall_duration = "week";
    int64_t             f_blocked_user_counter_lifetime = 5;
    bool                f_limit_duration = false;
    bool                f_check_blacklist = false;
    bool                f_prevent_old_passwords = false;
    bool                f_check_username_reversed = true;
};



class blacklist_t
{
public:
    void                add_passwords(QString const & passwords);
    void                remove_passwords(QString const & passwords);

    void                reset_counters();
    size_t              passwords_applied() const;
    size_t              passwords_skipped() const;

private:
    void                passwords_to_list(QString const & passwords);

    snap_string_list    f_list;
    size_t              f_count = 0;
    size_t              f_skipped = 0;
};



class password
        : public plugins::plugin
        , public path::path_execute
        , public layout::layout_content
{
public:
                        password();
                        ~password();

    // plugins::plugin implementation
    static password *   instance();
    virtual QString     settings_path() const;
    virtual QString     icon() const;
    virtual QString     description() const;
    virtual QString     dependencies() const;
    virtual int64_t     do_update(int64_t last_updated);
    virtual void        bootstrap(snap_child * snap);

    // users signals
    void on_check_user_security ( users::users::user_security_t    & security    );
    void on_user_logged_in      ( users::users::user_logged_info_t & logged_info );
    void on_save_password       ( users::users::user_info_t        & user_info, QString const & user_password, QString const & password_policy );
    void on_invalid_password    ( users::users::user_info_t        & user_info, QString const & policy );
    void on_blocked_user        ( users::users::user_info_t        & user_info, QString const & policy );

    // path::path_execute implementation
    bool                on_path_execute(content::path_info_t & ipath);

    // editor signals
    void                on_prepare_editor_form(editor::editor * e);
    void                on_init_editor_widget(content::path_info_t & ipath, QString const & field_id, QString const & field_type, QDomElement & widget, libdbproxy::row::pointer_t data_row);

    // layout::layout_content implementation
    void                on_generate_main_content(content::path_info_t & ipath, QDomElement & page, QDomElement & body);

    // layout signals
    void                on_generate_page_content(content::path_info_t & ipath, QDomElement & page, QDomElement & body);

    libdbproxy::table::pointer_t get_password_table();
    QString             check_password_against_policy(users::users::user_info_t user_info, QString const & user_password, QString const & policy);
    QString             create_password(QString const & policy = "users");

private:
    void                content_update(int64_t variables_timestamp);
    void                on_path_execute__is_password_blacklisted(content::path_info_t & ipath);
    void                on_path_execute__blacklist_new_passwords(content::path_info_t & ipath);
    void                on_path_execute__blacklist_remove_passwords(content::path_info_t & ipath);

    snap_child *                            f_snap = nullptr;
    libdbproxy::table::pointer_t f_password_table;
    bool                                    f_added_policy = false;
};


} // namespace versions
} // namespace snap
// vim: ts=4 sw=4 et
