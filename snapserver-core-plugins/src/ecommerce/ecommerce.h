// Snap Websites Server -- handle a cart, checkout, wishlist, affiliates...
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
#include "../path/path.h"
#include "../filter/filter.h"
#include "../layout/layout.h"
#include "../epayment/epayment.h"


/** \file
 * \brief Header of the ecommerce plugin.
 *
 * The file defines the various ecommerce plugin classes.
 */

namespace snap
{
namespace ecommerce
{


enum class name_t
{
    SNAP_NAME_ECOMMERCE_CART_MODIFIED_POST_FIELD,
    SNAP_NAME_ECOMMERCE_CART_PRODUCTS,
    SNAP_NAME_ECOMMERCE_CART_PRODUCTS_POST_FIELD,
    SNAP_NAME_ECOMMERCE_INVOICE_NUMBER,
    SNAP_NAME_ECOMMERCE_INVOICE_PATH,
    SNAP_NAME_ECOMMERCE_INVOICES_PATH,
    SNAP_NAME_ECOMMERCE_INVOICE_TABLE,
    SNAP_NAME_ECOMMERCE_JAVASCRIPT_CART
};
char const * get_name(name_t name) __attribute__ ((const));


//class ecommerce_exception : public snap_exception
//{
//public:
//    explicit ecommerce_exception(char const *        what_msg) : snap_exception("ecommerce", what_msg) {}
//    explicit ecommerce_exception(std::string const & what_msg) : snap_exception("ecommerce", what_msg) {}
//    explicit ecommerce_exception(QString cons t&     what_msg) : snap_exception("ecommerce", what_msg) {}
//};







class ecommerce
        : public plugins::plugin
        , public path::path_execute
        , public layout::layout_content
{
public:
                                ecommerce();
    virtual                     ~ecommerce();

    // plugins::plugin implementation
    static ecommerce *          instance();
    virtual QString             settings_path() const;
    virtual QString             icon() const;
    virtual QString             description() const;
    virtual QString             dependencies() const;
    virtual int64_t             do_update(int64_t last_updated);
    virtual void                bootstrap(snap_child * snap);

    // server signals
    void                        on_process_post(QString const & uri_path);

    // path::path_execute implementation
    virtual bool                on_path_execute(content::path_info_t & ipath);

    // path signals
    void                        on_preprocess_path(content::path_info_t & ipath, plugins::plugin * path_plugin);

    // layout::layout_content implementation
    virtual void                on_generate_main_content(content::path_info_t & ipath, QDomElement & page, QDomElement & body);

    // layout signals
    void                        on_generate_header_content(content::path_info_t & path, QDomElement & header, QDomElement & metadata);

    // filter signals
    void                        on_replace_token(content::path_info_t & ipath, QDomDocument & xml, filter::filter::token_info_t & token);
    void                        on_token_help(filter::filter::token_help_t & help);

    // epayment signals
    void                        on_generate_invoice(content::path_info_t & invoice_ipath, uint64_t & invoice_number, epayment::epayment_product_list & plist);

    SNAP_SIGNAL(product_allowed, (QDomElement product, content::path_info_t & product_ipath), (product, product_ipath));

private:
    void                        content_update(int64_t variables_timestamp);

    snap_child *                f_snap = nullptr;
};


} // namespace ecommerce
} // namespace snap
// vim: ts=4 sw=4 et
