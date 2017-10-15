// Snap Websites Server -- handle payments via PayPal
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
#include "../epayment/epayment.h"
#include "../filter/filter.h"
#include "../layout/layout.h"
#include "../path/path.h"

#include <snapwebsites/http_client_server.h>


/** \file
 * \brief Header of the epayment_paypal plugin.
 *
 * The file defines the various epayment_paypal plugin classes.
 */

namespace snap
{
namespace epayment_paypal
{


enum class name_t
{
    SNAP_NAME_EPAYMENT_PAYPAL_CANCEL_PLAN_URL,
    SNAP_NAME_EPAYMENT_PAYPAL_CANCEL_URL,
    SNAP_NAME_EPAYMENT_PAYPAL_CLICKED_POST_FIELD,
    SNAP_NAME_EPAYMENT_PAYPAL_DEBUG,
    SNAP_NAME_EPAYMENT_PAYPAL_LAST_ATTEMPT,
    SNAP_NAME_EPAYMENT_PAYPAL_MAXIMUM_REPEAT_FAILURES,
    SNAP_NAME_EPAYMENT_PAYPAL_RETURN_PLAN_THANK_YOU,
    SNAP_NAME_EPAYMENT_PAYPAL_RETURN_PLAN_URL,
    SNAP_NAME_EPAYMENT_PAYPAL_RETURN_THANK_YOU,
    SNAP_NAME_EPAYMENT_PAYPAL_RETURN_URL,
    SNAP_NAME_EPAYMENT_PAYPAL_SETTINGS_PATH,
    SNAP_NAME_EPAYMENT_PAYPAL_TABLE,
    SNAP_NAME_EPAYMENT_PAYPAL_TOKEN_POST_FIELD,

    // SECURE (saved in "secret" table)
    SNAP_SECURE_NAME_EPAYMENT_PAYPAL_ACTIVATED_PLAN,
    SNAP_SECURE_NAME_EPAYMENT_PAYPAL_ACTIVATED_PLAN_HEADER,
    SNAP_SECURE_NAME_EPAYMENT_PAYPAL_AGREEMENT_ID,
    SNAP_SECURE_NAME_EPAYMENT_PAYPAL_AGREEMENT_TOKEN,
    SNAP_SECURE_NAME_EPAYMENT_PAYPAL_AGREEMENT_URL,
    SNAP_SECURE_NAME_EPAYMENT_PAYPAL_BILL_PLAN,
    SNAP_SECURE_NAME_EPAYMENT_PAYPAL_BILL_PLAN_HEADER,
    SNAP_SECURE_NAME_EPAYMENT_PAYPAL_CHECK_BILL_PLAN,
    SNAP_SECURE_NAME_EPAYMENT_PAYPAL_CHECK_BILL_PLAN_HEADER,
    SNAP_SECURE_NAME_EPAYMENT_PAYPAL_CLIENT_ID,
    SNAP_SECURE_NAME_EPAYMENT_PAYPAL_CREATED_AGREEMENT,
    SNAP_SECURE_NAME_EPAYMENT_PAYPAL_CREATED_AGREEMENT_HEADER,
    SNAP_SECURE_NAME_EPAYMENT_PAYPAL_CREATED_PAYMENT,
    SNAP_SECURE_NAME_EPAYMENT_PAYPAL_CREATED_PAYMENT_HEADER,
    SNAP_SECURE_NAME_EPAYMENT_PAYPAL_CREATED_PLAN,
    SNAP_SECURE_NAME_EPAYMENT_PAYPAL_CREATED_PLAN_HEADER,
    SNAP_SECURE_NAME_EPAYMENT_PAYPAL_EXECUTE_AGREEMENT,
    SNAP_SECURE_NAME_EPAYMENT_PAYPAL_EXECUTED_AGREEMENT,
    SNAP_SECURE_NAME_EPAYMENT_PAYPAL_EXECUTED_AGREEMENT_HEADER,
    SNAP_SECURE_NAME_EPAYMENT_PAYPAL_EXECUTED_PAYMENT,
    SNAP_SECURE_NAME_EPAYMENT_PAYPAL_EXECUTED_PAYMENT_HEADER,
    SNAP_SECURE_NAME_EPAYMENT_PAYPAL_EXECUTE_PAYMENT,
    SNAP_SECURE_NAME_EPAYMENT_PAYPAL_INVOICE_NUMBER,
    SNAP_SECURE_NAME_EPAYMENT_PAYPAL_INVOICE_SECRET_ID,
    SNAP_SECURE_NAME_EPAYMENT_PAYPAL_OAUTH2_ACCESS_TOKEN,
    SNAP_SECURE_NAME_EPAYMENT_PAYPAL_OAUTH2_APP_ID,
    SNAP_SECURE_NAME_EPAYMENT_PAYPAL_OAUTH2_DATA,
    SNAP_SECURE_NAME_EPAYMENT_PAYPAL_OAUTH2_EXPIRES,
    SNAP_SECURE_NAME_EPAYMENT_PAYPAL_OAUTH2_HEADER,
    SNAP_SECURE_NAME_EPAYMENT_PAYPAL_OAUTH2_SCOPE,
    SNAP_SECURE_NAME_EPAYMENT_PAYPAL_OAUTH2_TOKEN_TYPE,
    SNAP_SECURE_NAME_EPAYMENT_PAYPAL_PAYMENT_ID,
    SNAP_SECURE_NAME_EPAYMENT_PAYPAL_PAYMENT_TOKEN,
    SNAP_SECURE_NAME_EPAYMENT_PAYPAL_PAYER_ID,
    SNAP_SECURE_NAME_EPAYMENT_PAYPAL_PLAN_ID,
    SNAP_SECURE_NAME_EPAYMENT_PAYPAL_PLAN_URL,
    SNAP_SECURE_NAME_EPAYMENT_PAYPAL_REPEAT_PAYMENT,
    SNAP_SECURE_NAME_EPAYMENT_PAYPAL_SANDBOX_CLIENT_ID,
    SNAP_SECURE_NAME_EPAYMENT_PAYPAL_SANDBOX_SECRET,
    SNAP_SECURE_NAME_EPAYMENT_PAYPAL_SECRET
};
char const * get_name(name_t name) __attribute__ ((const));


class epayment_paypal_exception : public snap_exception
{
public:
    explicit epayment_paypal_exception(char const *        what_msg) : snap_exception("epayment_paypal", what_msg) {}
    explicit epayment_paypal_exception(std::string const & what_msg) : snap_exception("epayment_paypal", what_msg) {}
    explicit epayment_paypal_exception(QString const &     what_msg) : snap_exception("epayment_paypal", what_msg) {}
};

class epayment_paypal_exception_io_error : public snap_exception
{
public:
    explicit epayment_paypal_exception_io_error(char const *        what_msg) : snap_exception("epayment_paypal", what_msg) {}
    explicit epayment_paypal_exception_io_error(std::string const & what_msg) : snap_exception("epayment_paypal", what_msg) {}
    explicit epayment_paypal_exception_io_error(QString const &     what_msg) : snap_exception("epayment_paypal", what_msg) {}
};








class epayment_paypal
        : public plugins::plugin
        , public path::path_execute
        , public layout::layout_content
{
public:
                                epayment_paypal();
                                ~epayment_paypal();

    // plugins::plugin implementation
    static epayment_paypal *    instance();
    virtual QString             settings_path() const;
    virtual QString             icon() const;
    virtual QString             description() const;
    virtual QString             dependencies() const;
    virtual int64_t             do_update(int64_t last_updated);
    virtual void                bootstrap(snap_child * snap);

    QtCassandra::QCassandraTable::pointer_t     get_epayment_paypal_table();

    // server signals
    void                        on_table_is_accessible(QString const & table_name, server::accessible_flag_t & accessible);
    void                        on_process_post(QString const & uri_path);

    // path::path_execute implementation
    virtual bool                on_path_execute(content::path_info_t & ipath);

    // layout signals
    void                        on_generate_header_content(content::path_info_t & path, QDomElement & header, QDomElement & metadata);
    virtual void                on_generate_main_content(content::path_info_t & ipath, QDomElement & page, QDomElement & body);

    // filter signals
    void                        on_replace_token(content::path_info_t & ipath, QDomDocument & xml, filter::filter::token_info_t & token);
    void                        on_token_help(filter::filter::token_help_t & help);

    // epayment signals
    void                        on_repeat_payment(content::path_info_t & first_invoice_ipath, content::path_info_t & previous_invoice_ipath, content::path_info_t & new_invoice_ipath);

private:
    void                        content_update(int64_t variables_timestamp);
    void                        cancel_invoice(QString const & token);
    bool                        get_oauth2_token(http_client_server::http_client & http, std::string & token_type, std::string & access_token);
    QString                     get_product_plan(http_client_server::http_client & http, std::string const & token_type, std::string const & access_token, epayment::epayment_product const & recurring_product, double const recurring_fee, QString & plan_id);
    bool                        get_debug();
    int8_t                      get_maximum_repeat_failures();
    std::string                 create_unique_request_id(QString const  & main_id);

    snap_child *                                f_snap = nullptr;
    QtCassandra::QCassandraTable::pointer_t     f_epayment_paypal_table;
    bool                                        f_debug_defined = false;
    bool                                        f_debug = false;
    bool                                        f_maximum_repeat_failures_defined = false;
    int64_t                                     f_maximum_repeat_failures = 0;
};


} // namespace epayment_paypal
} // namespace snap
// vim: ts=4 sw=4 et
