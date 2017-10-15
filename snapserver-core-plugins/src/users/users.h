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
#include "../filter/filter.h"
#include "../sessions/sessions.h"
#include "../path/path.h"


#include <QByteArray>
#include <QString>

#include <QtCassandra/QCassandraRow.h>
#include <QtCassandra/QCassandraTable.h>
#include <QtCassandra/QCassandraValue.h>

namespace snap
{
namespace users
{

enum class name_t
{
    SNAP_NAME_USERS_ADMINISTRATIVE_SESSION_DURATION,
    SNAP_NAME_USERS_ANONYMOUS_PATH,
    SNAP_NAME_USERS_AUTHOR,
    SNAP_NAME_USERS_AUTHORED_PAGES,
    SNAP_NAME_USERS_AUTO_PATH,
    SNAP_NAME_USERS_BLACK_LIST,
    SNAP_NAME_USERS_BLOCKED_PATH,
    SNAP_NAME_USERS_CHANGING_PASSWORD_KEY,
    SNAP_NAME_USERS_CREATED_TIME,
    SNAP_NAME_USERS_CURRENT_EMAIL,
    SNAP_NAME_USERS_EMAIL_HISTORY_LIST_BASE,
    SNAP_NAME_USERS_EXAMPLE,
    SNAP_NAME_USERS_FORCE_LOWERCASE,
    SNAP_NAME_USERS_FORGOT_PASSWORD_EMAIL,
    SNAP_NAME_USERS_FORGOT_PASSWORD_IP,
    SNAP_NAME_USERS_FORGOT_PASSWORD_ON,
    SNAP_NAME_USERS_HIT_CHECK,
    SNAP_NAME_USERS_HIT_TRANSPARENT,
    SNAP_NAME_USERS_HIT_USER,
    SNAP_NAME_USERS_IDENTIFIER,
    SNAP_NAME_USERS_ID_ROW,
    SNAP_NAME_USERS_INDEX_ROW,
    SNAP_NAME_USERS_LAST_USER_PATH,
    SNAP_NAME_USERS_LAST_VERIFICATION_SESSION,
    SNAP_NAME_USERS_LOCALE,
    SNAP_NAME_USERS_LOCALES,
    SNAP_NAME_USERS_LOGIN_IP,
    SNAP_NAME_USERS_LOGIN_ON,
    SNAP_NAME_USERS_LOGIN_REDIRECT,
    SNAP_NAME_USERS_LOGIN_REFERRER,
    SNAP_NAME_USERS_LOGIN_SESSION,
    SNAP_NAME_USERS_LOGOUT_IP,
    SNAP_NAME_USERS_LOGOUT_ON,
    SNAP_NAME_USERS_LONG_SESSIONS,
    SNAP_NAME_USERS_MODIFIED,
    SNAP_NAME_USERS_MULTISESSIONS,
    SNAP_NAME_USERS_MULTIUSER,
    SNAP_NAME_USERS_NAME,
    SNAP_NAME_USERS_NEW_PATH,
    SNAP_NAME_USERS_NOT_MAIN_PAGE,
    SNAP_NAME_USERS_ORIGINAL_EMAIL,
    SNAP_NAME_USERS_ORIGINAL_IP,
    SNAP_NAME_USERS_PASSWORD,
    SNAP_NAME_USERS_PASSWORD_BLOCKED,
    SNAP_NAME_USERS_PASSWORD_DIGEST,
    SNAP_NAME_USERS_PASSWORD_MODIFIED,
    SNAP_NAME_USERS_PASSWORD_PATH,
    SNAP_NAME_USERS_PASSWORD_SALT,
    SNAP_NAME_USERS_PATH,
    SNAP_NAME_USERS_PERMISSIONS_PATH,
    SNAP_NAME_USERS_PICTURE,
    SNAP_NAME_USERS_PREVIOUS_LOGIN_IP,
    SNAP_NAME_USERS_PREVIOUS_LOGIN_ON,
    //SNAP_NAME_USERS_SESSION_COOKIE, -- use a random name instead
    SNAP_NAME_USERS_SOFT_ADMINISTRATIVE_SESSION,
    SNAP_NAME_USERS_STATUS,
    SNAP_NAME_USERS_TABLE,
    SNAP_NAME_USERS_TIMEZONE,
    SNAP_NAME_USERS_TOTAL_SESSION_DURATION,
    SNAP_NAME_USERS_USERNAME,
    SNAP_NAME_USERS_USER_PAGE_PATH,
    SNAP_NAME_USERS_USER_SESSION_DURATION,
    SNAP_NAME_USERS_VERIFIED_IP,
    SNAP_NAME_USERS_VERIFIED_ON,
    SNAP_NAME_USERS_VERIFY_EMAIL,
    SNAP_NAME_USERS_VERIFY_IGNORE_USER_AGENT,
    SNAP_NAME_USERS_VERIFY_IGNORE_USER_AGENT_FOR_PASSWORD,
    SNAP_NAME_USERS_WEBSITE_REFERENCE
};
char const * get_name(name_t name) __attribute__ ((const));



class users_exception : public snap_exception
{
public:
    explicit users_exception(char const *        what_msg) : snap_exception("users", what_msg) {}
    explicit users_exception(std::string const & what_msg) : snap_exception("users", what_msg) {}
    explicit users_exception(QString const &     what_msg) : snap_exception("users", what_msg) {}
};

class users_exception_invalid_email : public users_exception
{
public:
    explicit users_exception_invalid_email(char const *        what_msg) : users_exception(what_msg) {}
    explicit users_exception_invalid_email(std::string const & what_msg) : users_exception(what_msg) {}
    explicit users_exception_invalid_email(QString const &     what_msg) : users_exception(what_msg) {}
};

class users_exception_invalid_path : public users_exception
{
public:
    explicit users_exception_invalid_path(char const *        what_msg) : users_exception(what_msg) {}
    explicit users_exception_invalid_path(std::string const & what_msg) : users_exception(what_msg) {}
    explicit users_exception_invalid_path(QString const &     what_msg) : users_exception(what_msg) {}
};

class users_exception_size_mismatch : public users_exception
{
public:
    explicit users_exception_size_mismatch(char const *        what_msg) : users_exception(what_msg) {}
    explicit users_exception_size_mismatch(std::string const & what_msg) : users_exception(what_msg) {}
    explicit users_exception_size_mismatch(QString const &     what_msg) : users_exception(what_msg) {}
};

class users_exception_digest_not_available : public users_exception
{
public:
    explicit users_exception_digest_not_available(char const *        what_msg) : users_exception(what_msg) {}
    explicit users_exception_digest_not_available(std::string const & what_msg) : users_exception(what_msg) {}
    explicit users_exception_digest_not_available(QString const &     what_msg) : users_exception(what_msg) {}
};

class users_exception_encryption_failed : public users_exception
{
public:
    explicit users_exception_encryption_failed(char const *        what_msg) : users_exception(what_msg) {}
    explicit users_exception_encryption_failed(std::string const & what_msg) : users_exception(what_msg) {}
    explicit users_exception_encryption_failed(QString const &     what_msg) : users_exception(what_msg) {}
};

class users_exception_invalid_object : public users_exception
{
public:
    explicit users_exception_invalid_object(char const *        what_msg) : users_exception(what_msg) {}
    explicit users_exception_invalid_object(std::string const & what_msg) : users_exception(what_msg) {}
    explicit users_exception_invalid_object(QString const &     what_msg) : users_exception(what_msg) {}
};






class users
        : public plugins::plugin
        , public links::links_cloned
        , public path::path_execute
        , public layout::layout_content
        , public layout::layout_boxes
{
public:
    enum class login_mode_t
    {
        LOGIN_MODE_FULL,
        LOGIN_MODE_VERIFICATION
    };

    static int64_t const            NEW_RANDOM_INTERVAL = 5LL * 60LL * 1000000LL; // 5 min. in microseconds

    // the login status, returned by load_login_session(), is a set of flags
    typedef int                     login_status_t;
    typedef int64_t                 identifier_t;

    static identifier_t const       IDENTIFIER_INVALID   = -1;
    static identifier_t const       IDENTIFIER_ANONYMOUS =  0;

    static login_status_t const     LOGIN_STATUS_OK                     = 0x0000;
    static login_status_t const     LOGIN_STATUS_INVALID_RANDOM_NUMBER  = 0x0001;
    static login_status_t const     LOGIN_STATUS_INVALID_SESSION        = 0x0002;
    static login_status_t const     LOGIN_STATUS_SESSION_TYPE_MISMATCH  = 0x0004;
    static login_status_t const     LOGIN_STATUS_RANDOM_MISMATCH        = 0x0008;
    static login_status_t const     LOGIN_STATUS_USER_AGENT_MISMATCH    = 0x0010;
    static login_status_t const     LOGIN_STATUS_UNEXPECTED_PATH        = 0x0020;
    static login_status_t const     LOGIN_STATUS_PASSED_LOGIN_LIMIT     = 0x0040;

    static sessions::sessions::session_info::session_id_t const USERS_SESSION_ID_LOG_IN = 1;                    // login-form.xml
    static sessions::sessions::session_info::session_id_t const USERS_SESSION_ID_LOG_IN_BOX = 2;                // login-box-form.xml
    static sessions::sessions::session_info::session_id_t const USERS_SESSION_ID_REGISTER = 3;                  // register-form.xml
    static sessions::sessions::session_info::session_id_t const USERS_SESSION_ID_REGISTER_BOX = 4;              // register-box-form.xml
    static sessions::sessions::session_info::session_id_t const USERS_SESSION_ID_FORGOT_PASSWORD = 5;           // forgot-password-form.xml
    static sessions::sessions::session_info::session_id_t const USERS_SESSION_ID_VERIFY = 6;                    // verify-form.xml
    static sessions::sessions::session_info::session_id_t const USERS_SESSION_ID_LOG_IN_SESSION = 7;
    static sessions::sessions::session_info::session_id_t const USERS_SESSION_ID_VERIFY_EMAIL = 8;
    static sessions::sessions::session_info::session_id_t const USERS_SESSION_ID_FORGOT_PASSWORD_EMAIL = 9;
    static sessions::sessions::session_info::session_id_t const USERS_SESSION_ID_RESEND_EMAIL = 10;             // resend-email-form.xml
    static sessions::sessions::session_info::session_id_t const USERS_SESSION_ID_NEW_PASSWORD = 11;             // new-password-form.xml
    static sessions::sessions::session_info::session_id_t const USERS_SESSION_ID_REPLACE_PASSWORD = 12;         // replace-password-form.xml
    static sessions::sessions::session_info::session_id_t const USERS_SESSION_ID_PASSWORD = 13;                 // password-form.xml
    static sessions::sessions::session_info::session_id_t const USERS_SESSION_ID_VERIFY_CREDENTIALS = 14;       // verify-credentials-form.xml

    enum class status_t
    {
        STATUS_UNKNOWN,         // user has a status link and we do not know what it is
        STATUS_UNDEFINED,       // status not known
        STATUS_NOT_FOUND,       // user does not exist in database
        STATUS_VALID,           // user is registered and verified
        STATUS_NEW,             // user is registered but not yet verified (maked as "NEW")
        STATUS_BLOCKED,         // user got blocked (marked as "BLOCKED")
        STATUS_AUTO,            // user did not register, account was auto-generated (marked as "AUTO"); possibly to block emails
        STATUS_PASSWORD,        // user has to enter a new password (marked as "PASSWORD")
        STATUS_INVALID_EMAIL    // user entered an invalid email
    };

    class user_info_t
    {
    public:
        typedef QtCassandra::QCassandraCell::pointer_t  cell_t;
        typedef QtCassandra::QCassandraValue            value_t;

                                user_info_t();
                                user_info_t( QString      const & email_or_path );
                                user_info_t( identifier_t const   id            );

        bool                    user_is_an_example_from_email() const;

        void                    define_user    ( identifier_t const identifier, QString const & user_email );
        identifier_t            get_identifier () const;
        QString const &         get_user_email () const;
        QString const &         get_user_key   () const;
        QString                 get_user_key   ( QString const & user_email ) const;

        QString                 get_user_path( bool const leading_slash ) const;  // "[/]user/<ID>"

        bool                    value_exists ( QString const & v ) const;
        bool                    value_exists ( name_t  const   v ) const;
        cell_t                  get_cell     ( QString const & name ) const;
        cell_t                  get_cell     ( name_t  const   name ) const;
        value_t const &         get_value    ( QString const & name ) const;
        value_t const &         get_value    ( name_t  const   name ) const;
        void                    set_value    ( QString const & name, value_t const & value );
        void                    set_value    ( name_t  const   name, value_t const & value );
        void                    delete_value ( QString const & name );
        void                    delete_value ( name_t  const   name );

        void                    set_status   ( status_t const & v );
        status_t                get_status   () const;

        bool                    is_valid     () const;
        bool                    is_anonymous () const;
        bool                    is_user      () const;

        bool                    exists() const;
        void                    reset();

        void                    save_user_parameter( QString const & field_name, QtCassandra::QCassandraValue const &  value );
        void                    save_user_parameter( QString const & field_name, QString                      const &  value );
        void                    save_user_parameter( QString const & field_name, int64_t                      const &  value );
        //
        bool                    load_user_parameter( QString const & field_name, QtCassandra::QCassandraValue &        value ) const;
        bool                    load_user_parameter( QString const & field_name, QString                      &        value ) const;
        bool                    load_user_parameter( QString const & field_name, int64_t                      &        value ) const;

        static identifier_t     get_user_id_by_path( QString const & user_path );
        static QString          get_full_anonymous_path() __attribute__ ((const));

        void                    change_user_email( QString const & new_user_email );

    private:
        static int const        MAX_EMAIL_BACKUPS = 5;

        snap_child *            get_snap() const;
        void                    init_tables() const;
        void                    get_user_id_by_email();
        QtCassandra::QCassandraRow::pointer_t get_user_row() const;

        mutable snap_child *    f_snap = nullptr;
        mutable QtCassandra::QCassandraTable::pointer_t f_users_table;
        identifier_t            f_identifier  = IDENTIFIER_INVALID;
        QString                 f_user_email;
        mutable QString         f_user_key;
        status_t                f_status      = status_t::STATUS_UNDEFINED;
    };

    class user_security_t
    {
    public:
        void                        set_user_info(user_info_t const & user_info, QString const & email = QString(), bool const allow_example_domain = false);
        user_info_t const &         get_user_info() const;

        QString const &             get_email() const;

        void                        set_password(QString const & password);
        bool                        has_password() const;
        QString const &             get_password() const;

        void                        set_policy(QString const & policy);
        QString const &             get_policy() const;

        void                        set_bypass_blacklist(bool const bypass);
        bool                        get_bypass_blacklist() const;

        void                        set_status(status_t status);
        status_t                    get_status() const;

        bool                        get_allow_example_domain() const;

        void                        set_example(bool example);
        bool                        get_example() const;

        content::permission_flag &  get_secure();

    private:
        user_info_t                 f_user_info;
        QString                     f_email;
        QString                     f_password = "!";
        QString                     f_policy = "users";
        bool                        f_bypass_blacklist = false;
        bool                        f_allow_example_domain = false;
        bool                        f_example = false;
        content::permission_flag    f_secure;
        status_t                    f_status = status_t::STATUS_VALID;
    };

    class user_logged_info_t
    {
    public:
                                        user_logged_info_t(snap_child * snap, user_info_t const & user_info );

        content::path_info_t            user_ipath() const;

        void                            set_identifier(identifier_t identifier);
        identifier_t                    get_identifier() const;

        void                            set_password_policy(QString const & policy);
        QString const &                 get_password_policy() const;

        void                            set_user_info(user_info_t const & user_info);
        user_info_t const &             get_user_info() const;

        void                            force_password_change();
        void                            force_user_to_change_password();
        bool                            is_password_change_required() const;

        // f_uri is mutable so we can change it from anywhere
        void                            set_uri(QString const & uri) const;
        QString const &                 get_uri() const;

    private:
        snap_child *                    f_snap = nullptr;
        QString                         f_password_policy;
        user_info_t                     f_user_info;
        identifier_t                    f_identifier = 0;
        bool                            f_force_password_change = false;
        mutable QString                 f_uri;
    };

                            users();
    virtual                 ~users();

    // plugins::plugin implementation
    static users *          instance();
    virtual QString         settings_path() const;
    virtual QString         icon() const;
    virtual QString         description() const;
    virtual QString         dependencies() const;
    virtual int64_t         do_update(int64_t last_updated);
    virtual int64_t         do_dynamic_update(int64_t last_updated);
    virtual void            bootstrap(::snap::snap_child * snap);


    static QString          basic_email_canonicalization(QString const & email);

    // server signals
    void                    on_table_is_accessible(QString const & table_name, server::accessible_flag_t & accessible);
    void                    on_process_cookies();
    void                    on_attach_to_session();
    void                    on_detach_from_session();
    void                    on_define_locales(http_strings::WeightedHttpString & locales);
    void                    on_improve_signature(QString const & path, QDomDocument doc, QDomElement signature_tag);

    // path::path_execute implementation
    bool                    on_path_execute(content::path_info_t & ipath);

    // locale signals
    void                    on_set_locale();
    void                    on_set_timezone();

    // content signals
    void                    on_create_content(content::path_info_t & path, QString const & owner, QString const & type);

    // layout::layout_content implementation
    virtual void            on_generate_main_content(content::path_info_t & ipath, QDomElement & page, QDomElement & body);

    // layout::layout_boxes implementation (to be removed)
    virtual void            on_generate_boxes_content(content::path_info_t & page_ipath, content::path_info_t & ipath, QDomElement & page, QDomElement & boxes);

    // layout signals
    void                    on_generate_header_content(content::path_info_t & path, QDomElement & hader, QDomElement & metadata);
    void                    on_generate_page_content(content::path_info_t & ipath, QDomElement & page, QDomElement & body);

    // filter signals
    void                    on_replace_token(content::path_info_t & ipath, QDomDocument & xml, filter::filter::token_info_t & token);
    void                    on_token_help(filter::filter::token_help_t & help);

    // links::links_cloned implementation
    virtual void            repair_link_of_cloned_page(QString const & clone, snap_version::version_number_t branch_number, links::link_info const & source, links::link_info const & destination, bool const cloning);

    SNAP_SIGNAL_WITH_MODE( check_user_security,  (user_security_t & security),                       (security),          START_AND_DONE );
    SNAP_SIGNAL_WITH_MODE( user_registered,      (content::path_info_t & ipath, int64_t identifier), (ipath, identifier), NEITHER        );
    SNAP_SIGNAL_WITH_MODE( user_verified,        (content::path_info_t & ipath, int64_t identifier), (ipath, identifier), NEITHER        );
    SNAP_SIGNAL_WITH_MODE( user_logged_in,       (user_logged_info_t & logged_info),                 (logged_info),       NEITHER        );
    SNAP_SIGNAL_WITH_MODE( logged_in_user_ready, (),                                                 (),                  NEITHER        );
    //SNAP_SIGNAL_WITH_MODE(save_password, (QtCassandra::QCassandraRow::pointer_t row, QString const & user_password, QString const & policy), (row, user_password, policy), DONE);
    SNAP_SIGNAL_WITH_MODE( save_password,        (user_info_t & user_info, QString const & user_password, QString const & policy), (user_info, user_password, policy), DONE    );
    SNAP_SIGNAL_WITH_MODE( invalid_password,     (user_info_t & user_info, QString const & policy),                                (user_info, policy),                NEITHER );
    SNAP_SIGNAL_WITH_MODE( blocked_user,         (user_info_t & user_info, QString const & policy),                                (user_info, policy),                NEITHER );

    QtCassandra::QCassandraTable::pointer_t get_users_table() const;

    int64_t                 get_total_session_duration();
    int64_t                 get_user_session_duration();
    int64_t                 get_administrative_session_duration();
    bool                    get_soft_administrative_session();
    QString                 get_user_cookie_name();
    bool                    user_is_a_spammer();
    bool                    user_is_logged_in() const;
    bool                    user_has_administrative_rights() const;
    bool                    user_session_is_old() const;
    static QString          create_password();
    void                    create_password_salt(QByteArray & salt);
    void                    encrypt_password(QString const & digest, QString const & password, QByteArray const & salt, QByteArray & hash);
    status_t                register_user(QString const & email, QString const & password, QString & reason, bool allow_example_domain = false);
    void                    verify_user(content::path_info_t & ipath);
    status_t                user_status_from_email(QString const & email, QString & status_key);
    status_t                user_status_from_identifier(int64_t identifier, QString & status_key);
    status_t                user_status_from_user_path(QString const & user_path, QString & status_key);
    sessions::sessions::session_info const & get_session() const;
    void                    attach_to_session(QString const & name, QString const & data);
    QString                 detach_from_session(QString const & name);
    QString                 get_from_session(QString const & name) const;
    void                    set_referrer( QString path, user_info_t const & user_info );
    QString                 detach_referrer( user_info_t const & user_info );
    QString                 login_user(QString const & email, QString const & password, bool & validation_required, login_mode_t login_mode = login_mode_t::LOGIN_MODE_FULL, QString const & password_policy = "users");
    login_status_t          load_login_session(QString const & session_cookie, sessions::sessions::session_info & info, bool check_time_limit);
    void                    transparent_hit();
    bool                    is_transparent_hit();
    bool                    authenticated_user(identifier_t const id, sessions::sessions::session_info * info);
    void                    create_logged_in_user_session(user_info_t const & user_info);
    void                    user_logout();

    QString                 get_user_email(QString const & user_path) const;
    QString                 get_user_email(identifier_t const identifier) const;
    //identifier_t            get_user_identifier(QString const & user_path) const;
    QString                 get_user_path(QString const & email) const;

    user_info_t &           get_user_info();
    user_info_t const &     get_user_info() const;
    static user_info_t      get_user_info_by_id    ( identifier_t const & id );
    static user_info_t      get_user_info_by_email ( QString const & email   );
    static user_info_t      get_user_info_by_path  ( QString const & path    );
    static user_info_t      get_user_info_by_name  ( QString const & name    );
    user_info_t             get_last_logged_in_user_info() const;

private:
    void                    content_update         (int64_t variables_timestamp);
    void                    user_identifier_update (int64_t variables_timestamp);
    QString                 referrer_identifier(user_info_t const & user_info);

    void                    token_user_count(filter::filter::token_info_t & token);

    snap_child *            f_snap = nullptr;

    user_info_t             f_user_info;                        // user info including email address (may not be logged in)
    QString                 f_hit = "undefined";                // type of hit, if not "transparent", we will update the time limit
    bool                    f_user_logged_in = false;           // user is logged in only if this is true
    bool                    f_administrative_logged_in = false; // user is logged in and has administrative rights if this is true
    bool                    f_has_user_messages = false;        // whether there were messages when on_detach_from_session() was called
    QString                 f_user_changing_password_key;       // not quite logged in user
    std::shared_ptr<sessions::sessions::session_info> f_info;   // user, logged in or anonymous, cookie related information
};

} // namespace users
} // namespace snap
// vim: ts=4 sw=4 et
