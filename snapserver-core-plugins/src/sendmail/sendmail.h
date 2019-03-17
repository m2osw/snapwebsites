// Snap Websites Server -- queue emails for the backend to send
// Copyright (c) 2013-2019  Made to Order Software Corp.  All Rights Reserved
//
// https://snapwebsites.org/
// contact@m2osw.com
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
#include "../users/users.h"

#include "../test_plugin_suite/test_plugin_suite.h"

// snapwebsites lib
//
#include <snapwebsites/snap_backend.h>
#include <snapwebsites/email.h>


namespace snap
{
namespace sendmail
{

enum class name_t
{
    SNAP_NAME_SENDMAIL,
    SNAP_NAME_SENDMAIL_BOUNCED,
    SNAP_NAME_SENDMAIL_BOUNCED_ARRIVAL_DATE,
    SNAP_NAME_SENDMAIL_BOUNCED_DIAGNOSTIC_CODE,
    SNAP_NAME_SENDMAIL_BOUNCED_EMAIL,
    SNAP_NAME_SENDMAIL_BOUNCED_FAILED,
    SNAP_NAME_SENDMAIL_BOUNCED_NOTIFICATION,
    SNAP_NAME_SENDMAIL_BOUNCED_RAW,
    SNAP_NAME_SENDMAIL_BYPASS_BLACKLIST,
    SNAP_NAME_SENDMAIL_CONTENT_TYPE,
    SNAP_NAME_SENDMAIL_CREATED,
    SNAP_NAME_SENDMAIL_EMAIL,
    SNAP_NAME_SENDMAIL_EMAIL_ENCRYPTION,
    SNAP_NAME_SENDMAIL_EMAIL_FREQUENCY,
    SNAP_NAME_SENDMAIL_EMAILS_TABLE,
    SNAP_NAME_SENDMAIL_FIELD_EMAIL,
    SNAP_NAME_SENDMAIL_FIELD_LEVEL,
    SNAP_NAME_SENDMAIL_FREQUENCY,
    SNAP_NAME_SENDMAIL_FREQUENCY_DAILY,
    SNAP_NAME_SENDMAIL_FREQUENCY_IMMEDIATE,
    SNAP_NAME_SENDMAIL_FREQUENCY_MONTHLY,
    SNAP_NAME_SENDMAIL_FREQUENCY_WEEKLY,
    SNAP_NAME_SENDMAIL_INDEX,
    SNAP_NAME_SENDMAIL_IS_ADMIN,
    SNAP_NAME_SENDMAIL_LAYOUT_NAME,
    SNAP_NAME_SENDMAIL_LEVEL_ANGRYLIST,
    SNAP_NAME_SENDMAIL_LEVEL_BLACKLIST,
    SNAP_NAME_SENDMAIL_LEVEL_ORANGELIST,
    SNAP_NAME_SENDMAIL_LEVEL_PURPLELIST,
    SNAP_NAME_SENDMAIL_LEVEL_WHITELIST,
    SNAP_NAME_SENDMAIL_LISTS,
    SNAP_NAME_SENDMAIL_MAXIMUM_TIME,
    SNAP_NAME_SENDMAIL_MINIMUM_TIME,
    SNAP_NAME_SENDMAIL_NEW,
    SNAP_NAME_SENDMAIL_SENDING_STATUS,
    SNAP_NAME_SENDMAIL_STATUS,
    SNAP_NAME_SENDMAIL_STATUS_DELETED,
    SNAP_NAME_SENDMAIL_STATUS_FAILED,
    SNAP_NAME_SENDMAIL_STATUS_INVALID,
    SNAP_NAME_SENDMAIL_STATUS_LOADING,
    SNAP_NAME_SENDMAIL_STATUS_NEW,
    SNAP_NAME_SENDMAIL_STATUS_READ,
    SNAP_NAME_SENDMAIL_STATUS_SENDING,
    SNAP_NAME_SENDMAIL_STATUS_SENT,
    SNAP_NAME_SENDMAIL_STATUS_SPAM,
    SNAP_NAME_SENDMAIL_STATUS_UNSUBSCRIBED,
    SNAP_NAME_SENDMAIL_STOP,
    SNAP_NAME_SENDMAIL_UNSUBSCRIBE_ON,
    SNAP_NAME_SENDMAIL_UNSUBSCRIBE_PATH,
    SNAP_NAME_SENDMAIL_UNSUBSCRIBE_SELECTION,
    SNAP_NAME_SENDMAIL_USER_AGENT,
};
const char * get_name(name_t name) __attribute__ ((const));


class sendmail_exception : public snap_exception
{
public:
    explicit sendmail_exception(char const *        what_msg) : snap_exception("sendmail", what_msg) {}
    explicit sendmail_exception(std::string const & what_msg) : snap_exception("sendmail", what_msg) {}
    explicit sendmail_exception(QString const &     what_msg) : snap_exception("sendmail", what_msg) {}
};

class sendmail_exception_no_backend : public sendmail_exception
{
public:
    explicit sendmail_exception_no_backend(char const *        what_msg) : sendmail_exception(what_msg) {}
    explicit sendmail_exception_no_backend(std::string const & what_msg) : sendmail_exception(what_msg) {}
    explicit sendmail_exception_no_backend(QString const &     what_msg) : sendmail_exception(what_msg) {}
};

class sendmail_exception_invalid_argument : public sendmail_exception
{
public:
    explicit sendmail_exception_invalid_argument(char const *        what_msg) : sendmail_exception(what_msg) {}
    explicit sendmail_exception_invalid_argument(std::string const & what_msg) : sendmail_exception(what_msg) {}
    explicit sendmail_exception_invalid_argument(QString const &     what_msg) : sendmail_exception(what_msg) {}
};

class sendmail_exception_too_many_levels : public sendmail_exception
{
public:
    explicit sendmail_exception_too_many_levels(char const *        what_msg) : sendmail_exception(what_msg) {}
    explicit sendmail_exception_too_many_levels(std::string const & what_msg) : sendmail_exception(what_msg) {}
    explicit sendmail_exception_too_many_levels(QString const &     what_msg) : sendmail_exception(what_msg) {}
};




class sendmail
    : public plugins::plugin
    , public server::backend_action
    , public layout::layout_content
{
public:
    static const sessions::sessions::session_info::session_id_t SENDMAIL_SESSION_ID_MESSAGE = 1;
    static const sessions::sessions::session_info::session_id_t SENDMAIL_SESSION_EMAIL_ENCRYPTION = 2;

                            sendmail();
                            sendmail(sendmail const & rhs) = delete;
    virtual                 ~sendmail() override;

    sendmail &              operator = (sendmail const & rhs) = delete;

    static sendmail *       instance();

    // plugins::plugin implementation
    virtual QString         icon() const override;
    virtual QString         description() const override;
    virtual QString         dependencies() const override;
    virtual int64_t         do_update(int64_t last_updated) override;
    virtual void            bootstrap(snap_child * snap) override;

    libdbproxy::table::pointer_t get_emails_table();

    // server signals
    void                    on_register_backend_cron(server::backend_action_set & actions);
    void                    on_replace_token(content::path_info_t & cpath, QDomDocument & xml, filter::filter::token_info_t & token);
    void                    on_token_help(filter::filter::token_help_t & help);

    // server::backend_action implementation
    virtual void            on_backend_action(QString const & action) override;

    // layout::layout_content
    virtual void            on_generate_main_content(content::path_info_t & path, QDomElement & page, QDomElement & body) override;

    // users signals
    void                    on_check_user_security(users::users::user_security_t & security);

    bool                    validate_email(QString const & user_email, email const * e);
    void                    post_email(email const & e);
    QString                 default_from() const;
    bool                    parse_email(QString const & email_data, email & e, bool bounce_email);

    SNAP_SIGNAL_WITH_MODE(filter_email, (email & e), (e), NEITHER);

    // links test suite
    SNAP_TEST_PLUGIN_SUITE_SIGNALS()

private:
    void                    content_update(int64_t variables_timestamp);
    void                    check_bounced_emails();
    void                    reorganize_bounce_email(QByteArray const & column_key, QString const & bounce_report);
    void                    process_bounce_email(QByteArray const & column_key, QString const & bounce_report, email const * e);
    void                    process_emails();
    void                    attach_email(email const & e);
    void                    attach_user_email(email const & e);
    void                    run_emails();
    void                    sendemail(QString const & key, QString const & unique_key);

    // tests
    SNAP_TEST_PLUGIN_TEST_DECL(test_parse_email_basic)
    SNAP_TEST_PLUGIN_TEST_DECL(test_parse_email_mixed)
    SNAP_TEST_PLUGIN_TEST_DECL(test_parse_email_report)

    snap_child *                    f_snap = nullptr;
    snap_backend *                  f_backend = nullptr;
    email                           f_email = email(); // email being processed
};

} // namespace sendmail
} // namespace snap
// vim: ts=4 sw=4 et
