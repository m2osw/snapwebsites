// Snap Websites Server -- handle electronic and not so electronic payments
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


/** \file
 * \brief Header of the epayment plugin.
 *
 * The file defines the various epayment plugin classes.
 */

namespace snap
{
namespace epayment
{


enum class name_t
{
    SNAP_NAME_EPAYMENT_CANCELED_PATH,
    SNAP_NAME_EPAYMENT_DESCRIPTION,
    SNAP_NAME_EPAYMENT_FAILED_PATH,
    SNAP_NAME_EPAYMENT_GRAND_TOTAL,
    SNAP_NAME_EPAYMENT_INVOICE_NUMBER,
    SNAP_NAME_EPAYMENT_INVOICE_STATUS,
    SNAP_NAME_EPAYMENT_INVOICE_STATUS_ABANDONED,
    SNAP_NAME_EPAYMENT_INVOICE_STATUS_CANCELED,
    SNAP_NAME_EPAYMENT_INVOICE_STATUS_COMPLETED,
    SNAP_NAME_EPAYMENT_INVOICE_STATUS_CREATED,
    SNAP_NAME_EPAYMENT_INVOICE_STATUS_FAILED,
    SNAP_NAME_EPAYMENT_INVOICE_STATUS_PAID,
    SNAP_NAME_EPAYMENT_INVOICE_STATUS_PENDING,
    SNAP_NAME_EPAYMENT_INVOICE_STATUS_PROCESSING,
    SNAP_NAME_EPAYMENT_INVOICE_STATUS_UNKNOWN,
    SNAP_NAME_EPAYMENT_LONG_DESCRIPTION,
    SNAP_NAME_EPAYMENT_PRICE,
    SNAP_NAME_EPAYMENT_PRODUCT,
    SNAP_NAME_EPAYMENT_PRODUCT_NAME,
    SNAP_NAME_EPAYMENT_PRODUCT_TYPE_PATH,
    SNAP_NAME_EPAYMENT_QUANTITY,
    SNAP_NAME_EPAYMENT_QUANTITY_INCREMENT,
    SNAP_NAME_EPAYMENT_QUANTITY_MAXIMUM,
    SNAP_NAME_EPAYMENT_QUANTITY_MINIMUM,
    SNAP_NAME_EPAYMENT_RECURRING,
    SNAP_NAME_EPAYMENT_RECURRING_SETUP_FEE,
    SNAP_NAME_EPAYMENT_SKU,
    SNAP_NAME_EPAYMENT_STORE_NAME,
    SNAP_NAME_EPAYMENT_THANK_YOU_PATH,
    SNAP_NAME_EPAYMENT_THANK_YOU_SUBSCRIPTION_PATH,
    SNAP_NAME_EPAYMENT_TOTAL,
    SNAP_NAME_EPAYMENT_USERS_ALLOW_SAVING_CREDIT_CARD_TOKEN
};
char const * get_name(name_t name) __attribute__ ((const));


class epayment_exception : public snap_exception
{
public:
    explicit epayment_exception(char const *        what_msg) : snap_exception("epayment", what_msg) {}
    explicit epayment_exception(std::string const & what_msg) : snap_exception("epayment", what_msg) {}
    explicit epayment_exception(QString const &     what_msg) : snap_exception("epayment", what_msg) {}
};

class epayment_invalid_type_exception : public epayment_exception
{
public:
    explicit epayment_invalid_type_exception(char const *        what_msg) : epayment_exception(what_msg) {}
    explicit epayment_invalid_type_exception(std::string const & what_msg) : epayment_exception(what_msg) {}
    explicit epayment_invalid_type_exception(QString const &     what_msg) : epayment_exception(what_msg) {}
};

class epayment_cannot_set_exception : public epayment_exception
{
public:
    explicit epayment_cannot_set_exception(char const *        what_msg) : epayment_exception(what_msg) {}
    explicit epayment_cannot_set_exception(std::string const & what_msg) : epayment_exception(what_msg) {}
    explicit epayment_cannot_set_exception(QString const &     what_msg) : epayment_exception(what_msg) {}
};

class epayment_cannot_unset_exception : public epayment_exception
{
public:
    explicit epayment_cannot_unset_exception(char const *        what_msg) : epayment_exception(what_msg) {}
    explicit epayment_cannot_unset_exception(std::string const & what_msg) : epayment_exception(what_msg) {}
    explicit epayment_cannot_unset_exception(QString const &     what_msg) : epayment_exception(what_msg) {}
};

class epayment_cannot_find_exception : public epayment_exception
{
public:
    explicit epayment_cannot_find_exception(char const *        what_msg) : epayment_exception(what_msg) {}
    explicit epayment_cannot_find_exception(std::string const & what_msg) : epayment_exception(what_msg) {}
    explicit epayment_cannot_find_exception(QString const &     what_msg) : epayment_exception(what_msg) {}
};

class epayment_missing_product_exception : public epayment_exception
{
public:
    explicit epayment_missing_product_exception(char const *        what_msg) : epayment_exception(what_msg) {}
    explicit epayment_missing_product_exception(std::string const & what_msg) : epayment_exception(what_msg) {}
    explicit epayment_missing_product_exception(QString const &     what_msg) : epayment_exception(what_msg) {}
};

class epayment_invalid_recurring_field_exception : public epayment_exception
{
public:
    explicit epayment_invalid_recurring_field_exception(char const *        what_msg) : epayment_exception(what_msg) {}
    explicit epayment_invalid_recurring_field_exception(std::string const & what_msg) : epayment_exception(what_msg) {}
    explicit epayment_invalid_recurring_field_exception(QString const &     what_msg) : epayment_exception(what_msg) {}
};




class epayment_product_list;



// the epayment_product represents one product in the cart
class epayment_product
{
public:
    enum class type_t
    {
        TYPE_STRING,
        TYPE_INTEGER,
        TYPE_FLOAT
    };

    void                clear();

    bool                verify_guid() const;

    void                set_property(QString const & name, QString const & value);
    void                set_property(QString const & name, int64_t const value);
    void                set_property(QString const & name, double const value);

    void                unset_property(QString const & name);

    bool                has_property(QString const & name) const;
    type_t              get_property_type(QString const & name) const;
    QString const       get_string_property(QString const & name) const;
    int64_t             get_integer_property(QString const & name) const;
    double              get_float_property(QString const & name) const;

    double              get_total() const;

private:
    friend epayment_product_list;

    class value_t
    {
    public:
                        value_t(); // required for std::map<> to work
                        value_t(QString const & value);
                        value_t(int64_t const value);
                        value_t(double const value);

        type_t          get_type() const;
        QString const & get_string_value(QString const  & name) const;
        int64_t         get_integer_value() const;
        double          get_float_value() const;

    private:
        type_t          f_type = type_t::TYPE_FLOAT;
        QString         f_string;
        int64_t         f_integer = 0;
        double          f_float = 0.0;
    };

    // name / value
    typedef std::map<QString, value_t>      properties_t;

    enum class verification_t
    {
        VERIFICATION_NOT_DONE,
        VERIFICATION_VALID,
        VERIFICATION_INVALID
    };

    // constructor is private so that way users cannot create a product directly
    // (see epayment_product_list::add_product(...) below)
    epayment_product(QString const & product, double const quantity, QString const & description);

    mutable properties_t                                f_properties;

    // when checking parameters from the database, keep those pointers for
    // later fast reference
    mutable verification_t                              f_verified = verification_t::VERIFICATION_NOT_DONE;
    mutable content::path_info_t                        f_product_ipath;
    mutable content::content *                          f_content_plugin = nullptr;
    mutable QtCassandra::QCassandraTable::pointer_t     f_revision_table;
    mutable QtCassandra::QCassandraRow::pointer_t       f_revision_row;
};


// the epayment_product represents the whole cart
// it can include "special" products such as shipping and taxes
class epayment_product_list
{
public:
    epayment_product &       add_product(QString const & product, double const quantity, QString const & description);

    bool                    empty() const;
    size_t                  size() const;
    void                    clear();

    double                  get_grand_total() const;
    epayment_product &      operator [] (int idx);
    epayment_product const & operator [] (int idx) const;

private:
    typedef std::vector<epayment_product>   product_list_t;

    product_list_t  f_products;
};




class recurring_t
{
public:
    typedef uint32_t            compressed_t;

    // we use shifts and masks instead of a structure with bit fields which
    // are not reliable in C/C++ (varies depending on the platform/processor)
    static int const            REPEAT_SHIFT = 20;
    static compressed_t const   REPEAT_MASK = 0x00000FFF;
    static int const            INTERVAL_SHIFT = 4;
    static compressed_t const   INTERVAL_MASK = 0x0000FFFF;
    static int const            FREQUENCY_SHIFT = 0;
    static compressed_t const   FREQUENCY_MASK = 0x0000000F;

    typedef int                 repeat_t;
    static repeat_t const       INFINITE_REPEAT = -1;
    static repeat_t const       MAX_REPEAT = 1000;

    typedef int                 interval_t;
    static interval_t const     MAX_INTERVAL_YEARS = 5; // i.e. 60 months, 260 weeks, 1830 days -- note that PayPal, for example, only allows up to 1/YEAR

    // ***********
    // WARNING: these values are saved as is in the database DO NOT CHANGE!
    // ***********
    typedef uint8_t             frequency_t;
    static frequency_t const    FREQUENCY_DAY = 1;
    static frequency_t const    FREQUENCY_WEEK = 2;
    static frequency_t const    FREQUENCY_TWICE_A_MONTH = 3; // i.e. 1st and 15th... most systems do not support this one though...
    static frequency_t const    FREQUENCY_MONTH = 4;
    static frequency_t const    FREQUENCY_YEAR = 5;

                            recurring_t();
                            recurring_t(QString const & field);

    void                    set(QString const & field);
    void                    set(repeat_t repeat, interval_t interval, frequency_t frequency);
    void                    set(compressed_t compressed);

    repeat_t                get_repeat() const;
    interval_t              get_interval() const;
    frequency_t             get_frequency() const;

    QString                 to_string() const;
    compressed_t            to_compressed() const;

    static QString          freq_to_string(frequency_t frequency);

    bool                    is_null() const;
    bool                    is_infinite() const;
    bool                    operator == (recurring_t const & rhs) const;
    bool                    operator != (recurring_t const & rhs) const;

private:
    repeat_t                f_repeat = INFINITE_REPEAT;
    interval_t              f_interval = 1;
    frequency_t             f_frequency = FREQUENCY_MONTH;
};



class epayment
        : public plugins::plugin
{
public:
                                epayment();
                                ~epayment();

    // plugins::plugin implementation
    static epayment *           instance();
    virtual QString             settings_path() const;
    virtual QString             icon() const;
    virtual QString             description() const;
    virtual QString             dependencies() const;
    virtual int64_t             do_update(int64_t last_updated);
    virtual void                bootstrap(snap_child * snap);

    // layout signals
    void                        on_generate_header_content(content::path_info_t & path, QDomElement & header, QDomElement & metadata);

    SNAP_SIGNAL_WITH_MODE(generate_invoice, (content::path_info_t & invoice_ipath, uint64_t & invoice_number, epayment_product_list & plist), (invoice_ipath, invoice_number, plist), NEITHER);
    SNAP_SIGNAL_WITH_MODE(retrieve_invoice, (content::path_info_t & invoice_ipath, uint64_t & invoice_number, epayment_product_list & plist), (invoice_ipath, invoice_number, plist), NEITHER);
    SNAP_SIGNAL(set_invoice_status, (content::path_info_t & invoice_ipath, name_t const status), (invoice_ipath, status));
    SNAP_SIGNAL(repeat_payment, (content::path_info_t & first_invoice_url, content::path_info_t & previous_invoice_url, content::path_info_t & new_invoice_url), (first_invoice_url, previous_invoice_url, new_invoice_url));

    name_t                      get_invoice_status(content::path_info_t & invoice_ipath);

    static recurring_t          parser_recurring_field(QString const & info);

private:
    void                        content_update(int64_t variables_timestamp);

    snap_child *                f_snap = nullptr;
};


} // namespace epayment
} // namespace snap
// vim: ts=4 sw=4 et
