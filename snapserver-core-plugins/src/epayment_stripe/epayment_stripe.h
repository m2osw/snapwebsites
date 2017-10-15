// Snap Websites Server -- handle payments via Stripe
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
#include "../epayment/epayment.h"
#include "../epayment_creditcard/epayment_creditcard.h"
#include "../filter/filter.h"
#include "../layout/layout.h"
#include "../path/path.h"

// snapwebsites lib
//
#include <snapwebsites/http_client_server.h>


/** \file
 * \brief Header of the epayment_stripe plugin.
 *
 * The file defines the various epayment_stripe plugin classes.
 */

namespace snap
{
namespace epayment_stripe
{


enum class name_t
{
    SNAP_NAME_EPAYMENT_STRIPE_CANCEL_PLAN_URL,
    SNAP_NAME_EPAYMENT_STRIPE_CANCEL_URL,
    SNAP_NAME_EPAYMENT_STRIPE_CHARGE_URI,
    SNAP_NAME_EPAYMENT_STRIPE_CLICKED_POST_FIELD,
    SNAP_NAME_EPAYMENT_STRIPE_CREATED,
    SNAP_NAME_EPAYMENT_STRIPE_CUSTOMER_KEY,
    SNAP_NAME_EPAYMENT_STRIPE_CUSTOMER_URI,
    SNAP_NAME_EPAYMENT_STRIPE_DEBUG,
    SNAP_NAME_EPAYMENT_STRIPE_LAST_ATTEMPT,
    SNAP_NAME_EPAYMENT_STRIPE_MAXIMUM_REPEAT_FAILURES,
    SNAP_NAME_EPAYMENT_STRIPE_RETURN_PLAN_THANK_YOU,
    SNAP_NAME_EPAYMENT_STRIPE_RETURN_THANK_YOU,
    SNAP_NAME_EPAYMENT_STRIPE_SETTINGS_PATH,
    SNAP_NAME_EPAYMENT_STRIPE_TABLE,
    SNAP_NAME_EPAYMENT_STRIPE_TEST_KEY,
    SNAP_NAME_EPAYMENT_STRIPE_TOKEN_POST_FIELD,
    SNAP_NAME_EPAYMENT_STRIPE_VERSION,

    // SECURE (saved in "secret" table)
    SNAP_SECURE_NAME_EPAYMENT_STRIPE_CHARGE_HEADER,
    SNAP_SECURE_NAME_EPAYMENT_STRIPE_CHARGE_INFO,
    SNAP_SECURE_NAME_EPAYMENT_STRIPE_CHARGE_KEY,
    SNAP_SECURE_NAME_EPAYMENT_STRIPE_CREATE_CUSTOMER_HEADER,
    SNAP_SECURE_NAME_EPAYMENT_STRIPE_CUSTOMER_INFO,
    SNAP_SECURE_NAME_EPAYMENT_STRIPE_ERROR_REPLY,
    SNAP_SECURE_NAME_EPAYMENT_STRIPE_LAST_ERROR_MESSAGE,
    SNAP_SECURE_NAME_EPAYMENT_STRIPE_REPEAT_PAYMENT,
    SNAP_SECURE_NAME_EPAYMENT_STRIPE_RETRIEVE_CUSTOMER_HEADER,
    SNAP_SECURE_NAME_EPAYMENT_STRIPE_SECRET,
    SNAP_SECURE_NAME_EPAYMENT_STRIPE_TEST_SECRET,
    SNAP_SECURE_NAME_EPAYMENT_STRIPE_UPDATE_CUSTOMER_ERROR,
    SNAP_SECURE_NAME_EPAYMENT_STRIPE_UPDATE_CUSTOMER_HEADER,
    SNAP_SECURE_NAME_EPAYMENT_STRIPE_USER_KEY
};
char const * get_name(name_t name) __attribute__ ((const));


class epayment_stripe_exception : public snap_exception
{
public:
    explicit epayment_stripe_exception(char const *        what_msg) : snap_exception("epayment_stripe", what_msg) {}
    explicit epayment_stripe_exception(std::string const & what_msg) : snap_exception("epayment_stripe", what_msg) {}
    explicit epayment_stripe_exception(QString const &     what_msg) : snap_exception("epayment_stripe", what_msg) {}
};

class epayment_stripe_exception_invalid_parameter : public snap_exception
{
public:
    explicit epayment_stripe_exception_invalid_parameter(char const *        what_msg) : snap_exception("epayment_stripe", what_msg) {}
    explicit epayment_stripe_exception_invalid_parameter(std::string const & what_msg) : snap_exception("epayment_stripe", what_msg) {}
    explicit epayment_stripe_exception_invalid_parameter(QString const &     what_msg) : snap_exception("epayment_stripe", what_msg) {}
};

class epayment_stripe_exception_io_error : public snap_exception
{
public:
    explicit epayment_stripe_exception_io_error(char const *        what_msg) : snap_exception("epayment_stripe", what_msg) {}
    explicit epayment_stripe_exception_io_error(std::string const & what_msg) : snap_exception("epayment_stripe", what_msg) {}
    explicit epayment_stripe_exception_io_error(QString const &     what_msg) : snap_exception("epayment_stripe", what_msg) {}
};

class epayment_stripe_exception_invalid_error : public snap_exception
{
public:
    explicit epayment_stripe_exception_invalid_error(char const *        what_msg) : snap_exception("epayment_stripe", what_msg) {}
    explicit epayment_stripe_exception_invalid_error(std::string const & what_msg) : snap_exception("epayment_stripe", what_msg) {}
    explicit epayment_stripe_exception_invalid_error(QString const &     what_msg) : snap_exception("epayment_stripe", what_msg) {}
};








class epayment_stripe
        : public plugins::plugin
        //, public path::path_execute
        , public layout::layout_content
        , public epayment_creditcard::epayment_creditcard_gateway_t
{
public:
                                epayment_stripe();
                                ~epayment_stripe();

    // plugins::plugin implementation
    static epayment_stripe *    instance();
    virtual QString             settings_path() const;
    virtual QString             icon() const;
    virtual QString             description() const;
    virtual QString             dependencies() const;
    virtual int64_t             do_update(int64_t last_updated);
    virtual void                bootstrap(snap_child * snap);

    QtCassandra::QCassandraTable::pointer_t     get_epayment_stripe_table();

    // server signals
    void                        on_table_is_accessible(QString const & table_name, server::accessible_flag_t & accessible);

    // path::path_execute implementation
    //virtual bool                on_path_execute(content::path_info_t & ipath);

    // layout signals
    void                        on_generate_header_content(content::path_info_t & path, QDomElement & header, QDomElement & metadata);
    virtual void                on_generate_main_content(content::path_info_t & ipath, QDomElement & page, QDomElement & body);

    // filter signals
    void                        on_replace_token(content::path_info_t & ipath, QDomDocument & xml, filter::filter::token_info_t & token);

    // epayment signals
    void                        on_repeat_payment(content::path_info_t & first_invoice_ipath, content::path_info_t & previous_invoice_ipath, content::path_info_t & new_invoice_ipath);

    // epayment_creditcard::epayment_creditcard_gateway_t implementation
    virtual void                gateway_features(epayment_creditcard::epayment_gateway_features_t & gateway_info);
    virtual bool                process_creditcard(epayment_creditcard::epayment_creditcard_info_t & creditcard_info, editor::save_info_t & save_info);

private:
    void                        content_update(int64_t variables_timestamp);
    void                        cancel_invoice(QString const & token);
    bool                        get_oauth2_token(http_client_server::http_client & http, std::string & token_type, std::string & access_token);
    QString                     get_product_plan(http_client_server::http_client & http, std::string const & token_type, std::string const & access_token, epayment::epayment_product const & recurring_product, double const recurring_fee, QString & plan_id);
    bool                        get_debug();
    QString                     get_stripe_key(bool const debug);
    int8_t                      get_maximum_repeat_failures();
    std::string                 create_unique_request_id(QString const & main_id);
    void                        log_error(http_client_server::http_response::pointer_t response, QtCassandra::QCassandraRow::pointer_t row);

    snap_child *                                f_snap = nullptr;
    QtCassandra::QCassandraTable::pointer_t     f_epayment_stripe_table;
    bool                                        f_debug_defined = false;
    bool                                        f_debug = false;
    bool                                        f_maximum_repeat_failures_defined = false;
    bool                                        f_stripe_key_defined[2] = { false, false }; // 0- live, 1- test
    int64_t                                     f_maximum_repeat_failures = 0;
    QString                                     f_stripe_key[2]; // 0- live, 1- test
};


} // namespace epayment_stripe
} // namespace snap
// vim: ts=4 sw=4 et
