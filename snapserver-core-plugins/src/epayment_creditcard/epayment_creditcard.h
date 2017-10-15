// Snap Websites Server -- epayment credit card extension
// Copyright (C) 2014-2017  Made to Order Software Corp.
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
namespace epayment_creditcard
{


enum class name_t
{
    SNAP_NAME_EPAYMENT_CREDITCARD_DEFAULT_COUNTRY,
    SNAP_NAME_EPAYMENT_CREDITCARD_GATEWAY,
    SNAP_NAME_EPAYMENT_CREDITCARD_SETTINGS_PATH,
    SNAP_NAME_EPAYMENT_CREDITCARD_SHOW_ADDRESS2,
    SNAP_NAME_EPAYMENT_CREDITCARD_SHOW_COUNTRY,
    SNAP_NAME_EPAYMENT_CREDITCARD_SHOW_BUSINESS_NAME,
    SNAP_NAME_EPAYMENT_CREDITCARD_SHOW_DELIVERY,
    SNAP_NAME_EPAYMENT_CREDITCARD_SHOW_ONE_NAME,
    SNAP_NAME_EPAYMENT_CREDITCARD_SHOW_PHONE,
    SNAP_NAME_EPAYMENT_CREDITCARD_SHOW_PROVINCE,
    SNAP_NAME_EPAYMENT_CREDITCARD_USER_ALLOWS_SAVING_TOKEN
};
char const * get_name(name_t name) __attribute__ ((const));


class epayment_creditcard_exception : public snap_exception
{
public:
    explicit epayment_creditcard_exception(char const *        what_msg) : snap_exception("server-access", what_msg) {}
    explicit epayment_creditcard_exception(std::string const & what_msg) : snap_exception("server-access", what_msg) {}
    explicit epayment_creditcard_exception(QString const &     what_msg) : snap_exception("server-access", what_msg) {}
};

class epayment_creditcard_exception_status_missing : public epayment_creditcard_exception
{
public:
    explicit epayment_creditcard_exception_status_missing(char const *        what_msg) : epayment_creditcard_exception(what_msg) {}
    explicit epayment_creditcard_exception_status_missing(std::string const & what_msg) : epayment_creditcard_exception(what_msg) {}
    explicit epayment_creditcard_exception_status_missing(QString const &     what_msg) : epayment_creditcard_exception(what_msg) {}
};

class epayment_creditcard_exception_gateway_missing : public epayment_creditcard_exception
{
public:
    explicit epayment_creditcard_exception_gateway_missing(char const *        what_msg) : epayment_creditcard_exception(what_msg) {}
    explicit epayment_creditcard_exception_gateway_missing(std::string const & what_msg) : epayment_creditcard_exception(what_msg) {}
    explicit epayment_creditcard_exception_gateway_missing(QString const &     what_msg) : epayment_creditcard_exception(what_msg) {}
};





class epayment_creditcard_info_t
{
public:
    // credit card
    void                        set_user_name(QString const & user_name);
    QString                     get_user_name() const;

    void                        set_creditcard_number(QString const & creditcard_number);
    QString                     get_creditcard_number() const;

    void                        set_security_code(QString const & security_code);
    QString                     get_security_code() const;

    void                        set_expiration_date_month(QString const & expiration_date_month);
    QString                     get_expiration_date_month() const;

    void                        set_expiration_date_year(QString const & expiration_date_year);
    QString                     get_expiration_date_year() const;

    // billing address
    void                        set_billing_business_name(QString const & business_name);
    QString                     get_billing_business_name() const;

    void                        set_billing_attention(QString const & attention);
    QString                     get_billing_attention() const;

    void                        set_billing_address1(QString const & address1);
    QString                     get_billing_address1() const;

    void                        set_billing_address2(QString const & address2);
    QString                     get_billing_address2() const;

    void                        set_billing_city(QString const & city);
    QString                     get_billing_city() const;

    void                        set_billing_province(QString const & province);
    QString                     get_billing_province() const;

    void                        set_billing_postal_code(QString const & postal_code);
    QString                     get_billing_postal_code() const;

    void                        set_billing_country(QString const & country);
    QString                     get_billing_country() const;

    // delivery address
    void                        set_delivery_business_name(QString const & business_name);
    QString                     get_delivery_business_name() const;

    void                        set_delivery_attention(QString const & attention);
    QString                     get_delivery_attention() const;

    void                        set_delivery_address1(QString const & address1);
    QString                     get_delivery_address1() const;

    void                        set_delivery_address2(QString const & address2);
    QString                     get_delivery_address2() const;

    void                        set_delivery_city(QString const & city);
    QString                     get_delivery_city() const;

    void                        set_delivery_province(QString const & province);
    QString                     get_delivery_province() const;

    void                        set_delivery_postal_code(QString const & postal_code);
    QString                     get_delivery_postal_code() const;

    void                        set_delivery_country(QString const & country);
    QString                     get_delivery_country() const;

    // other fields
    void                        set_phone(QString const & phone);
    QString                     get_phone() const;

    // set by processor
    void                        set_subscription(bool subscription);
    bool                        get_subscription() const;

private:
    QString                     f_user_name;
    QString                     f_creditcard_number;
    QString                     f_security_code;
    QString                     f_expiration_date_month;
    QString                     f_expiration_date_year;

    QString                     f_billing_business_name;
    QString                     f_billing_attention;
    QString                     f_billing_address1;
    QString                     f_billing_address2;
    QString                     f_billing_city;
    QString                     f_billing_province;
    QString                     f_billing_postal_code;
    QString                     f_billing_country;

    QString                     f_delivery_business_name;
    QString                     f_delivery_attention;
    QString                     f_delivery_address1;
    QString                     f_delivery_address2;
    QString                     f_delivery_city;
    QString                     f_delivery_province;
    QString                     f_delivery_postal_code;
    QString                     f_delivery_country;

    QString                     f_phone;

    bool                        f_subscription = false;
};


class epayment_gateway_features_t
{
public:
                                epayment_gateway_features_t(QString const & gateway);

    QString                     get_gateway() const;

    void                        set_name(QString const & name);
    QString                     get_name() const;

private:
    QString                     f_gateway;  // plugin name
    QString                     f_name;     // display name
};



class epayment_creditcard_gateway_t
{
public:
    virtual void                gateway_features(epayment_gateway_features_t & gateway_info) = 0;
    virtual bool                process_creditcard(epayment_creditcard_info_t & creditcard_info, editor::save_info_t & save_info) = 0;
};




class epayment_creditcard
        : public plugins::plugin
        , public epayment_creditcard_gateway_t
{
public:
                                epayment_creditcard();
                                ~epayment_creditcard();

    // plugins::plugin implementation
    static epayment_creditcard *instance();
    virtual QString             settings_path() const;
    virtual QString             icon() const;
    virtual QString             description() const;
    virtual QString             dependencies() const;
    virtual int64_t             do_update(int64_t last_updated);
    virtual void                bootstrap(snap_child * snap);

    // server signals
    void                        on_process_post(QString const & uri_path);

    // editor signals
    void                        on_dynamic_editor_widget(content::path_info_t & ipath, QString const & name, QDomDocument & editor_widgets);
    void                        on_save_editor_fields(editor::save_info_t & save_info);

    // epayment_creditcard::epayment_creditcard_gateway_t implementation
    virtual void                gateway_features(epayment_gateway_features_t & gateway_info);
    virtual bool                process_creditcard(epayment_creditcard_info_t & creditcard_info, editor::save_info_t & save_info);

private:
    void                        content_update(int64_t variables_timestamp);

    void                        setup_form(content::path_info_t & ipath, QDomDocument & editor_widgets);
    void                        setup_settings(QDomDocument & editor_widgets);

    snap_child *                f_snap = nullptr;
};


} // namespace epayment_creditcard
} // namespace snap
// vim: ts=4 sw=4 et
