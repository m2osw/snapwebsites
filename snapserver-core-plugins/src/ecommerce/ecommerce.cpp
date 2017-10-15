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

#include "ecommerce.h"

#include "../editor/editor.h"
#include "../locale/snap_locale.h"
#include "../messages/messages.h"
#include "../output/output.h"
#include "../permissions/permissions.h"
#include "../shorturl/shorturl.h"

#include <snapwebsites/log.h>
#include <snapwebsites/not_reached.h>
#include <snapwebsites/not_used.h>
#include <snapwebsites/qdomxpath.h>
#include <snapwebsites/snap_lock.h>

#include <iostream>

#include <QDateTime>

#include <snapwebsites/poison.h>


/** \file
 * \brief Define the base of the e-Commerce Cart/Product Manager.
 *
 * This file defines the base Product Manager which alloes you
 * to handle a very large number of capabilities on any one
 * product.
 *
 * It also manages the user cart on the backend here and the front
 * end using JavaScript code.
 *
 * The following lists the capabilities of the Product Manager
 * documenting the fields used by the Product Manager to define
 * a product details:
 *
 * \li Brief Description -- epayment::description if defined, otherwise
 * fallback and use content::title instead -- the title of a product
 * page is considered to be the brief description of the
 * product, it is often viewed as the display name (or end user name)
 * of the product.
 *
 * \li Name -- epayment::product_name -- the technical name of the
 * product; most often only used internally. This gives you the
 * possibility to create several pages with the exact same name and
 * still distinguish each product using their technical name (although
 * the URI is also a unique identifier for these products and the cart
 * uses the URI...)
 *
 * \li Price -- epayment::price -- the current sale price of the
 * product. Costs and inventory value are managed by the inventory
 * extension, not be the base ecommerce plugin.
 *
 * \li Standard Price -- ecommerce::standard_price -- the price setup by
 * the manufacturer; if undefined, use epayment::price.
 *
 * \li Minimum Quantity -- epayment::min_quantity -- minimum number
 * of items to be able to checkout (i.e. you sell pens with a company
 * name and force customers to get at leat 100.)
 *
 * \li Maximum Quantity -- epayment::max_quantity -- maximum number
 * of items to be able to checkout (i.e. you sell paid for accounts
 * on your website, users cannot by more than 1 at a time.) When the
 * stock handling plugin is installed, this may be limited to what
 * is available in the stock.
 *
 * \li Quantity Multiple -- epayment::multiple -- quantity has to
 * be a multiple of this number to be valid.
 *
 * \li Quantity Unit -- ecommerce::quantity_unit -- one of pounds,
 * kilos, grammes, meters, ..., or a simple count. List of units can
 * be managed by the end user.
 *
 * \li Category -- ecommerce::category -- one or more categories
 * linked to this product; this is a standard link so it is used
 * in the branch table and not in the revision table.
 * TBD -- management of the tags used for product categorization;
 *        at this point I am thinking of a set of taxonomy tags
 *        under a specific ecommerce/category path and each entry
 *        is one name and their children are the various choices
 *        i.e. ecommerce/category/color/blue and
 *             ecommerce/category/color/red to select the blue
 *             and red colors
 *
 * \li Logo -- ecommerce::logo -- one image representing the product
 * or the brand of the product.
 *
 * \li Display Image -- ecommerce::image -- one display image, to
 * show on the website page. This is generally small enough to fit
 * in a standard page.
 *
 * \li Images -- ecommerce::images -- one or more images that
 * display the product in a fullscreen size manner possibly with
 * a full zoom feature while moving the mouse.
 * TBD -- this has to be a list, we can easily have many attachments
 *        to a single page, but a field representing a list is a
 *        bit of an annoyment to handle. Especially if we want
 *        to be able to give each image a few parameters, so this
 *        is probably going to be a full structure which is saved
 *        using the serialization.
 */


SNAP_PLUGIN_START(ecommerce, 1, 0)


/* \brief Get a fixed ecommerce name.
 *
 * The ecommerce plugin makes use of different names in the database. This
 * function ensures that you get the right spelling for a given name.
 *
 * \param[in] name  The name to retrieve.
 *
 * \return A pointer to the name.
 */
char const * get_name(name_t name)
{
    switch(name)
    {
    case name_t::SNAP_NAME_ECOMMERCE_CART_MODIFIED_POST_FIELD:
        return "ecommerce__cart_modified";

    case name_t::SNAP_NAME_ECOMMERCE_CART_PRODUCTS:
        return "ecommerce::cart_products";

    case name_t::SNAP_NAME_ECOMMERCE_CART_PRODUCTS_POST_FIELD:
        return "ecommerce__cart_products";

    case name_t::SNAP_NAME_ECOMMERCE_INVOICE_NUMBER: // int64_t
        return "ecommerce::invoice_number";

    case name_t::SNAP_NAME_ECOMMERCE_INVOICE_PATH:
        return "ecommerce::invoice_path";

    case name_t::SNAP_NAME_ECOMMERCE_INVOICES_PATH:
        return "invoices";

    case name_t::SNAP_NAME_ECOMMERCE_INVOICE_TABLE:
        return "invoice";

    case name_t::SNAP_NAME_ECOMMERCE_JAVASCRIPT_CART:
        return "js/ecommerce/ecommerce-cart.js";

    default:
        // invalid index
        throw snap_logic_exception("invalid SNAP_NAME_ECOMMERCE_...");

    }
    NOTREACHED();
}




// At first I was thinking I could replicate the JavaScript code, but
// realistically, I think it is better to use XSLT 2.0 files to replicate
// the cart as an invoice on the server side. Not only that, the invoice
// could that way look (completely) different. Finally, the ecommerce
// system needs to generate an on-screen invoice and a PDF invoice
// (and eventually other formats like RTF and Text.)
//
// namespace
// {
// 
// 
// class ecommerce_column_header_t
// {
// public:
// 
// ecommerce_column_header_t(QString const& name, QString const& display_name)
//     : f_name(name)
//     , f_display_name(display_name)
// {
// }
// 
// 
// QString get_name() const
// {
//     return f_name;
// }
// 
// 
// QString get_display_name() const
// {
//     return f_display_name;
// }
// 
// private:
// QString         f_name;
// QString         f_display_name;
// };
// 
// 
// class ecommerce_column_cell_t
// {
// public:
// 
// ecommerce_column_cell_t()
// {
// }
// 
// 
// ecommerce_column_cell_t(QString const& column_name, QString const& value)
//     : f_name(column_name)
//     , f_value(value)
// {
// }
// 
// 
// QString const get_name() const
// {
//     return f_name;
// }
// 
// 
// QString const& get_value() const
// {
//     return f_value;
// }
// 
// 
// private:
// QString             f_name;
// QString             f_value;
// };
// 
// 
// class ecommerce_columns_t
// {
// typedef std::vector<ecommerce_column_header_t>          column_headers_t;
// typedef std::map<QString, size_t>                       column_map_t;       // column headers mapped by name
// typedef std::vector<ecommerce_column_cell_t>            row_t;
// typedef std::vector<row_t>                              rows_t;
// 
// public:
// 
// size_t size()
// {
//     return f_column_headers.size();
// }
// 
// 
// void add_column_header(QString const& column_name, QString const& display_name, QString const& before_column)
// {
//     //ecommerce_column_header_t *header(new ecommerce_column_header_t(column_name, display_name));
// 
//     if(!before_column.isEmpty())
//     {
//         size_t const max(f_column_headers.size());
//         for(size_t i(0); i < max; ++i)
//         {
//             if(before_column == f_column_headers[i].get_name())
//             {
//                 f_column_headers.insert(f_column_headers.begin() + i, ecommerce_column_header_t(column_name, display_name));
//                 return;
//             }
//         }
//     }
// 
//     f_column_headers.push_back(ecommerce_column_header_t(column_name, display_name));
// }
// 
// 
// ecommerce_column_header_t get_column_header(size_t index)
// {
//     if(index >= f_column_headers.size())
//     {
//         throw snap_logic_exception(QString("index %1 is out of bounds in ecommerce_column::get_column_header() (max is %1)").arg(index).arg(f_column_headers.size()));
//     }
//     return f_column_headers[index];
// }
// 
// 
// void generate_columns_map()
// {
//     size_t const max(f_column_headers.size());
//     for(size_t i(0); i < max; ++i)
//     {
//         QString const name(f_column_headers[i].get_name());
//         if(f_column_map.find(name) != f_column_map.end())
//         {
//             throw snap_logic_exception(QString("you defined two header columns with the same name \"%1\"").arg(name));
//         }
//         f_column_map[name] = i;
//     }
// }
// 
// 
// size_t get_column_index(QString const& name)
// {
//     if(f_column_map.find(name) != f_column_map.end())
//     {
//         throw snap_logic_exception(QString("header named \"%1\" not found in snapwebsites.eCommerceColumns.getColumnIndex()").arg(name));
//     }
//     return f_column_map[name];
// }
// 
// 
// void add_column_data(size_t row_index, QString const& column_name, QString const& value)
// {
//     if(f_rows.size() <= row_index)
//     {
//         f_rows.resize(row_index + 1);
//     }
// 
//     if(f_rows[row_index].empty())
//     {
//         f_rows[row_index].resize(f_column_headers.size());
//     }
// 
//     size_t i(get_column_index(column_name));
//     f_rows[row_index][i] = ecommerce_column_cell_t(column_name, value);
// }
// 
// 
// ecommerce_column_cell_t get_column_data(size_t row_index, size_t column_index)
// {
//     if(row_index >= f_rows.size())
//     {
//         throw snap_logic_exception(QString("row_index %1 larger than the number of rows: %2").arg(row_index).arg(f_rows.size()));
//     }
//     if(column_index >= f_rows[row_index].size())
//     {
//         throw snap_logic_exception(QString("column_index %1 larger than the number of cells: %2").arg(column_index).arg(f_rows[row_index].size()));
//     }
//     return f_rows[row_index][column_index];
// }
// 
// 
// private:
// column_headers_t        f_column_headers;
// column_map_t            f_column_map;
// rows_t                  f_rows;             // arrays of cells
// };
// 
// 
// 
// }
// // no name namespace






/** \brief Initialize the ecommerce plugin.
 *
 * This function is used to initialize the ecommerce plugin object.
 */
ecommerce::ecommerce()
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Clean up the ecommerce plugin.
 *
 * Ensure the ecommerce object is clean before it is gone.
 */
ecommerce::~ecommerce()
{
}


/** \brief Get a pointer to the ecommerce plugin.
 *
 * This function returns an instance pointer to the ecommerce plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the ecommerce plugin.
 */
ecommerce * ecommerce::instance()
{
    return g_plugin_ecommerce_factory.instance();
}


/** \brief Send users to the plugin settings.
 *
 * This path represents this plugin settings.
 */
QString ecommerce::settings_path() const
{
    return "/admin/settings/ecommerce";
}


/** \brief A path or URI to a logo for this plugin.
 *
 * This function returns a 64x64 icons representing this plugin.
 *
 * \return A path to the logo.
 */
QString ecommerce::icon() const
{
    return "/images/ecommerce/ecommerce-logo-64x64.png";
}


/** \brief Return the description of this plugin.
 *
 * This function returns the English description of this plugin.
 * The system presents that description when the user is offered to
 * install or uninstall a plugin on his website. Translation may be
 * available in the database.
 *
 * \return The description in a QString.
 */
QString ecommerce::description() const
{
    return "The e-Commerce plugin offers all the necessary features a"
        " website needs to offer a full e-Commerce environment so your"
        " users can purchase your goods and services. The base plugin"
        " includes many features directly available to you without the"
        " need for other plugins. However, you want to install the"
        " ecommerce-payment plugin and at least one of the payments"
        " gateway in order to allow for the actual payments.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString ecommerce::dependencies() const
{
    return "|filter|layout|output|permissions|shorturl|";
}


/** \brief Check whether updates are necessary.
 *
 * This function updates the database when a newer version is installed
 * and the corresponding updates where not run.
 *
 * This works for newly installed plugins and older plugins that were
 * updated.
 *
 * \param[in] last_updated  The UTC Unix date when the website was last updated (in micro seconds).
 *
 * \return The UTC Unix date of the last update of this plugin.
 */
int64_t ecommerce::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2017, 6, 6, 23, 33, 34, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Update the database with our content references.
 *
 * Send our content to the database so the system can find us when a
 * user references our administration pages, etc.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added
 *                        to the database by this update (in micro-seconds).
 */
void ecommerce::content_update(int64_t variables_timestamp)
{
    NOTUSED(variables_timestamp);

    content::content::instance()->add_xml(get_plugin_name());
}


/** \brief Initialize the ecommerce.
 *
 * This function terminates the initialization of the ecommerce plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void ecommerce::bootstrap(snap_child * snap)
{
    f_snap = snap;

    SNAP_LISTEN(ecommerce, "server", server, process_post, _1);
    SNAP_LISTEN(ecommerce, "layout", layout::layout, generate_header_content, _1, _2, _3);
    SNAP_LISTEN(ecommerce, "epayment", epayment::epayment, generate_invoice, _1, _2, _3);
    SNAP_LISTEN(ecommerce, "filter", filter::filter, replace_token, _1, _2, _3);
    SNAP_LISTEN(ecommerce, "filter", filter::filter, token_help, _1);
    SNAP_LISTEN(ecommerce, "path", path::path, preprocess_path, _1, _2);
}


/** \brief Setup page for the e-Commerce plugin.
 *
 * The e-Commerce module offers a JavaScript cart which we want on all
 * pages of an e-Commerce website since the user may want to checkout
 * at any time.
 *
 * \param[in,out] ipath  The path being managed.
 * \param[in,out] header  The header being generated.
 * \param[in,out] metadata  The metadata being generated.
 */
void ecommerce::on_generate_header_content(content::path_info_t & ipath, QDomElement & header, QDomElement & metadata)
{
    NOTUSED(ipath);
    NOTUSED(metadata);

    QDomDocument doc(header.ownerDocument());

    // TODO: find a way to include e-Commerce data only if required
    //       (it may already be done! search on add_javascript() for info.)
    content::content::instance()->add_javascript(doc, "ecommerce");
    content::content::instance()->add_javascript(doc, "ecommerce-cart");
    content::content::instance()->add_css(doc, "ecommerce");
}


/** \brief Check the URL and process the POST data accordingly.
 *
 * This function manages the posted cart data. All we do, really,
 * is save the cart in the user's session. That simple. We do
 * this as fast as possible so as to quickly reply to the user.
 * Since we do not have to check permissions for more pages and
 * do not have to generate any heavy HTML output, it should be
 * dead fast.
 *
 * The cart data is not checked here. It will be once we generate
 * the actual invoice.
 *
 * The function counts the number of products to make sure that
 * it does not go over the imposed limit. Also each tag cannot
 * be any larger than a certain size so we also calculate the
 * total byte size and impose that limit too.
 *
 * \todo
 * Add a cart session? I think that the user session is enough plus
 * we will have an editor session since the cart is to have the
 * quantity fields accessible as editor widgets. At this point, I
 * leave this open. It won't matter much if the user is logged in
 * on a secure server (i.e. using HTTPS which is generally
 * mandatory when you use the e-Commerce feature.)
 *
 * \param[in] uri_path  The path received from the HTTP server.
 */
void ecommerce::on_process_post(QString const& uri_path)
{
    // TODO: see doc above
    //QString const editor_full_session(f_snap->postenv("_editor_session"));
    //if(editor_full_session.isEmpty())
    //{
    //    // if the _editor_session variable does not exist, do not consider this
    //    // POST as an Editor POST
    //    return;
    //}

    // make sure this is a cart post
    char const *cart_products(get_name(name_t::SNAP_NAME_ECOMMERCE_CART_PRODUCTS_POST_FIELD));
    if(!f_snap->postenv_exists(cart_products))
    {
        return;
    }

    content::path_info_t ipath;
    ipath.set_path(uri_path);

    QString const cart_xml(f_snap->postenv(cart_products));
    users::users::instance()->attach_to_session(get_name(name_t::SNAP_NAME_ECOMMERCE_CART_PRODUCTS), cart_xml);

    // create the AJAX response
    server_access::server_access *server_access_plugin(server_access::server_access::instance());
    server_access_plugin->create_ajax_result(ipath, true);
    server_access_plugin->ajax_output();
}


/** \brief This function gets called when a dynamic path gets executed.
 *
 * This function checks the dynamic path supported. If the path
 * is the ecommerce-cart.js file, then the file generates a JavaScript file
 * and returns that to the client. This file is always marked as
 * requiring a reload (i.e. no caching allowed.)
 *
 * \param[in] ipath  The path of the page being executed.
 */
bool ecommerce::on_path_execute(content::path_info_t& ipath)
{
    QString const cpath(ipath.get_cpath());
    if(cpath == get_name(name_t::SNAP_NAME_ECOMMERCE_JAVASCRIPT_CART))
    {
        // check whether we have some products in the cart, if so
        // spit them out now! (with the exception of the product
        // this very page represents if it does represent a product)

        // we do not start spitting out any code up until the time we
        // know that there is at least one product in the cart

        // get the session information
        QString const cart_xml(users::users::instance()->get_from_session(get_name(name_t::SNAP_NAME_ECOMMERCE_CART_PRODUCTS)));

        QString js(QString("// e-Commerce Cart generated on %1\n").arg(QDateTime::currentDateTime().toString()));
        QDomDocument doc;
        doc.setContent(cart_xml);
        QDomXPath products_xpath;
        products_xpath.setXPath("/cart/product");
        QDomXPath::node_vector_t product_tags(products_xpath.apply(doc));
        int const max_products(product_tags.size());

        snap_uri const main_uri(f_snap->get_uri());
        bool const no_types(main_uri.has_query_option("no-types"));

        // first add all the product types
        bool first(true);
        for(int i(0); i < max_products; ++i)
        {
            // we found the product, retrieve its description and price
            QDomElement product(product_tags[i].toElement());
            QString const guid(product.attribute("guid"));
            if(ipath.get_key() == guid // this page is a product?
            || !no_types)
            {
                // TODO: We must verify that the GUID points to a product
                //       AND that the user has enough permissions to see
                //       that product; if not then the user should not be
                //       able to add that product to the cart in the first
                //       place so we can err and stop the processing
                //
                // get the data in local variables
                content::path_info_t product_ipath;
                product_ipath.set_path(guid);
                content::field_search::search_result_t product_result;
                FIELD_SEARCH
                    (content::field_search::command_t::COMMAND_MODE, content::field_search::mode_t::SEARCH_MODE_EACH)
                    (content::field_search::command_t::COMMAND_PATH_INFO_REVISION, product_ipath)

                    // DESCRIPTION
                    (content::field_search::command_t::COMMAND_FIELD_NAME, epayment::get_name(epayment::name_t::SNAP_NAME_EPAYMENT_DESCRIPTION))
                    (content::field_search::command_t::COMMAND_SELF)
                    (content::field_search::command_t::COMMAND_IF_FOUND, 1)
                        // use page title as a fallback
                        (content::field_search::command_t::COMMAND_FIELD_NAME, content::get_name(content::name_t::SNAP_NAME_CONTENT_TITLE))
                        (content::field_search::command_t::COMMAND_SELF)
                    (content::field_search::command_t::COMMAND_LABEL, 1)

                    // PRICE
                    (content::field_search::command_t::COMMAND_FIELD_NAME, epayment::get_name(epayment::name_t::SNAP_NAME_EPAYMENT_PRICE))
                    (content::field_search::command_t::COMMAND_SELF)

                    // get the 2 results
                    (content::field_search::command_t::COMMAND_RESULT, product_result)

                    // retrieve!
                    ;

                if(product_result.size() == 2)
                {
                    QString product_description(product_result[0].stringValue());
                    if(!product_description.isEmpty())
                    {
                        double const price(product_result[1].safeDoubleValue(10.00));

                        // add a product type
                        if(first)
                        {
                            first = false;
                            js += "jQuery(document).ready(function(){"
                                "snapwebsites.eCommerceCartInstance.setInitializing(true)\n";
                        }
                        QString guid_safe_quotes(guid);
                        guid_safe_quotes.replace("'", "\\'");
                        product_description.replace("'", "\\'");
                        js += ".registerProductType({"
                                "'ecommerce::features':    'ecommerce::basic',"
                                "'ecommerce::guid':        '" + guid_safe_quotes + "',"
                                "'ecommerce::description': '" + product_description + "',"
                                "'ecommerce::price':       " + price +
                            "})\n";
                    }
                }
            }
        }
        if(!first)
        {
            js += ";\n";
        }

        // second add the product to the cart, including their quantity
        // and attributes
        for(int i(0); i < max_products; ++i)
        {
            if(first)
            {
                first = false;
                js += "jQuery(document).ready(function(){\n";
            }

            // retrieve the product GUID and quantity
            // TBD: check that the product is valid? Here it is less of a
            //      problem since that's the cart itself
            QDomElement product(product_tags[i].toElement());
            QString const guid(product.attribute("guid"));
            QString const quantity(product.attribute("q"));
            QString guid_safe_quotes(guid);
            guid_safe_quotes.replace("'", "\\'");
            js += "snapwebsites.eCommerceCartInstance.addProduct('" + guid_safe_quotes + "', " + quantity + ");\n";
            // TODO: we need to add support for attributes
        }

        if(!first)
        {
            js += "snapwebsites.eCommerceCartInstance.setInitializing(false);});\n";
        }

        f_snap->output(js);
        // make sure it is a text/javascript and it was expired already
        f_snap->set_header("Content-Type", "text/javascript; charset=utf8", snap_child::HEADER_MODE_EVERYWHERE);
        f_snap->set_header("Expires", "Sat,  1 Jan 2000 00:00:00 GMT", snap_child::HEADER_MODE_EVERYWHERE);
        f_snap->set_header("Cache-Control", "no-store, no-cache, must-revalidate, post-check=0, pre-check=0", snap_child::HEADER_MODE_EVERYWHERE);

        return true;
    }

    return false;
}


/** \brief Check whether the user added e-Commerce query strings.
 *
 * The query string understood by e-Commerce allows administrators
 * to add items to the cart without having the end user click on
 * any button.
 *
 * \param[in,out] ipath  The path being worked on.
 * \param[in] path_plugin  The pointer to the plugin path.
 */
void ecommerce::on_preprocess_path(content::path_info_t& ipath, plugins::plugin *path_plugin)
{
    NOTUSED(ipath);
    NOTUSED(path_plugin);

    snap_uri const main_uri(f_snap->get_uri());
    if(main_uri.has_query_option("cart"))
    {
        // the "cart" option is a set of commands that we want to apply
        // now to the cart; if no cart exists, create a new one
        // the Short URL support is optional
        shorturl::shorturl *shorturl_plugin(plugins::exists("shorturl")
                        ? shorturl::shorturl::instance() // dynamic_cast<shorturl::shorturl *>(plugins::get_plugin("shorturl"))
                        : nullptr);
        QString const cart_code(main_uri.query_option("cart"));
        struct product_t
        {
            void clear()
            {
                f_attributes.clear();
                f_product.clear();
                f_operation = '*'; // keep as is if already defined
                f_quantity = 1.0;
            }

            typedef uint32_t    operation_t;
            typedef double      quantity_t;

            snap_string_list    f_attributes;
            QString             f_product;
            operation_t         f_operation = '*';
            quantity_t          f_quantity = 1.0;
        };
        product_t product;
        std::vector<product_t> product_list;
        bool valid(true);
        bool keep(true);
        QChar const *code(cart_code.data());
        while(code->unicode() != '\0' && valid)
        {
            switch(code->unicode())
            {
            case 'a':
                {
                    QString path;

                    ++code; // skip the 'a'
                    if(code->unicode() >= '0' && code->unicode() <= '9')
                    {
                        if(shorturl_plugin == nullptr)
                        {
                            messages::messages::instance()->set_error(
                                "Unsupported Product Path",
                                "e-Commerce attributes cannot use a Short URL number without the shorturl plugin running.",
                                "shorturl not available.",
                                false
                            );
                            valid = false;
                        }
                        else
                        {
                            // this is the shortcut identifier
                            uint64_t sc(0);
                            for(; code->unicode() >= '0' && code->unicode() <= '9'; ++code)
                            {
                                sc = sc * 10 + code->unicode() - '0';
                            }
                            path = shorturl_plugin->get_shorturl(sc);
                            valid = !path.isEmpty();
                        }
                    }
                    else
                    {
                        // this is the path to the product, it has to be
                        // written between exclamation marks
                        if(code->unicode() != '!')
                        {
                            messages::messages::instance()->set_error(
                                "Invalid Product Path",
                                QString("e-Commerce product paths in the cart=... option must be written between exclamation marks (!)."),
                                "unquoted names are not accepted as product paths.",
                                false
                            );
                            valid = false;
                        }
                        else
                        {
                            ++code; // skip the '!'
                            path.clear();
                            for(; code->unicode() != '\0' && code->unicode() != '!'; ++code)
                            {
                                path += code->unicode();
                            }
                            if(code->unicode() == '\0')
                            {
                                messages::messages::instance()->set_error(
                                    "Invalid Product Path",
                                    QString("e-Commerce product paths in the cart=... option must be written between exclamation points (!)."),
                                    "unquoted names are not accepted as product paths.",
                                    false
                                );
                                valid = false;
                            }
                            else
                            {
                                ++code; // skip the '!'
                            }
                        }
                    }
                    // got a valid attribute path
                    product.f_attributes << path;
                }
                break;

            case 'e':
                ++code; // skip the 'e'
                keep = false;
                break;

            case 'p':
                ++code; // skip the 'p'
                if(code->unicode() >= '0' && code->unicode() <= '9')
                {
                    if(shorturl_plugin == nullptr)
                    {
                        messages::messages::instance()->set_error(
                            "Unsupported Product Path",
                            "e-Commerce products cannot use a Short URL number without the shorturl plugin running.",
                            "shorturl not available.",
                            false
                        );
                        valid = false;
                    }
                    else
                    {
                        // this is the shortcut identifier
                        int sc(0);
                        for(; code->unicode() >= '0' && code->unicode() <= '9'; ++code)
                        {
                            sc = sc * 10 + code->unicode() - '0';
                        }
                        product.f_product = shorturl_plugin->get_shorturl(sc);
                        valid = !product.f_product.isEmpty();
                    }
                }
                else
                {
                    // this is the path to the product, it has to be
                    // written between colons
                    if(code->unicode() != '!')
                    {
                        messages::messages::instance()->set_error(
                            "Invalid Product Path",
                            QString("e-Commerce product paths in the cart=... option must be written between exclamation points (!)."),
                            "invalid numbers are not accepted as quantities and no product gets added.",
                            false
                        );
                        valid = false;
                    }
                    else
                    {
                        ++code; // skip the '!'
                        for(; code->unicode() != '\0' && code->unicode() != '!'; ++code)
                        {
                            product.f_product += code->unicode();
                        }
                        if(code->unicode() == '\0')
                        {
                            messages::messages::instance()->set_error(
                                "Invalid Product Path",
                                QString("e-Commerce product paths in the cart=... option must be written between exclamation points (!)."),
                                "invalid numbers are not accepted as quantities and no product gets added.",
                                false
                            );
                            valid = false;
                        }
                        else
                        {
                            ++code; // skip the '!'
                            // add the product now
                        }
                    }
                }
                // TODO SECURITY: verify quantity versus product

                // add product in 'path' with 'quantity' and 'attributes'
                product_list.push_back(product);
                product.clear();
                break;

            case 'q':
                ++code;
                if(code->unicode() == '*'
                || code->unicode() == '='
                || code->unicode() == '+'
                || code->unicode() == '-')
                {
                    product.f_operation = code->unicode();
                    ++code;
                }
                else if(code->unicode() == ' ')
                {
                    // we mean '+', browser must send ' '...
                    product.f_operation = '+';
                    ++code;
                }
                product.f_quantity = 0;
                for(; code->unicode() >= '0' && code->unicode() <= '9'; ++code)
                {
                    product.f_quantity = product.f_quantity * 10.0 + static_cast<double>(code->unicode() - '0');
                }
                if(code->unicode() == '.')
                {
                    ++code;
                    double f(10.0);
                    for(; code->unicode() >= '0' && code->unicode() <= '9'; ++code, f *= 10.0)
                    {
                        product.f_quantity += static_cast<double>(code->unicode() - '0') / f;
                    }
                }
                break;

            default:
                messages::messages::instance()->set_error(
                    "Invalid Cart Query String",
                    QString("The cart query string does not understand the '%1' character.").arg(code->unicode()),
                    "unsupported character found in the cart query string",
                    false
                );
                valid = false;
                break;

            }
        }
        if(valid)
        {
            // the whole cart info is valid, apply it
            users::users *users_plugin(users::users::instance());
            QDomDocument doc;
            QDomElement cart_tag;
            if(keep)
            {
                // read the existing cart if the user did not want to
                // empty it first
                QString cart_xml(users_plugin->get_from_session(get_name(name_t::SNAP_NAME_ECOMMERCE_CART_PRODUCTS)));
                doc.setContent(cart_xml);
                cart_tag = doc.documentElement();
                if(cart_tag.tagName() != "cart") // make sure it is a cart element
                {
                    cart_tag.clear();
                    // clear the document too because it is probably no good
                    doc = QDomDocument();
                }
            }
            if(cart_tag.isNull())
            {
                cart_tag = doc.createElement("cart");
                doc.appendChild(cart_tag);
            }
            for(auto it(product_list.begin()); it != product_list.end(); ++it)
            {
                QDomNodeList product_tags(doc.documentElement().elementsByTagName("product"));
                int const max_products(product_tags.size());
                bool found(false);
                for(int i(0); i < max_products; ++i)
                {
                    QDomElement p(product_tags.at(i).toElement());
                    QString const guid(p.attribute("guid"));
                    if(guid == it->f_product)
                    {
                        // TODO: check the attributes too
                        switch(it->f_operation)
                        {
                        case '=':
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
                            if(it->f_quantity == 0.0)
#pragma GCC diagnostic pop
                            {
                                // remove the item from the cart
                                p.parentNode().removeChild(p);
                            }
                            else
                            {
                                // force quantity to what user specified
                                p.setAttribute("q", it->f_quantity);
                            }
                            found = true;
                            break;

                        case '-':
                            {
                                QString const quantity_str(p.attribute("q"));
                                bool ok;
                                double quantity(quantity_str.toDouble(&ok));
                                if(!ok)
                                {
                                    quantity = 0.0;
                                }
                                if(quantity <= it->f_quantity)
                                {
                                    // removing all items is equivalent to deleting
                                    p.parentNode().removeChild(p);
                                }
                                else
                                {
                                    p.setAttribute("q", quantity - it->f_quantity);
                                }
                                found = true;
                            }
                            break;

                        case '+':
                            {
                                QString const quantity_str(p.attribute("q"));
                                bool ok;
                                double quantity(quantity_str.toDouble(&ok));
                                if(!ok)
                                {
                                    quantity = 0.0;
                                }
                                p.setAttribute("q", quantity + it->f_quantity);
                                found = true;
                            }
                            break;

                        case '*':
                            // by default ignore if it exists
                            found = true;
                            break;

                        default:
                            throw snap_logic_exception(QString("invalid product.f_operation \"%1\"").arg(it->f_operation));;

                        }
                        // exit for() loop
                        break;
                    }
                }
                if(!found && it->f_operation != '-' && it->f_quantity > 0)
                {
                    QDomElement p(doc.createElement("product"));
                    cart_tag.appendChild(p);
                    p.setAttribute("guid", it->f_product);
                    p.setAttribute("q", it->f_quantity);
                    // TODO: add product attributes

                    // now verify that this product is indeed allowed
                    // (otherwise you could add nearly any page in there!)
                    content::path_info_t product_ipath;
                    product_ipath.set_path(it->f_product);
                    product_allowed(p, product_ipath);
                }
            }
            users_plugin->attach_to_session(get_name(name_t::SNAP_NAME_ECOMMERCE_CART_PRODUCTS), doc.toString(-1));
        }
    }
}


/** \brief Generate the page main content.
 *
 * This function generates the main content of the page. Other
 * plugins will also have the event called if they subscribed and
 * thus will be given a chance to add their own content to the
 * main page. This part is the one that (in most cases) appears
 * as the main content on the page although the content of some
 * columns may be interleaved with this content.
 *
 * Note that this is NOT the HTML output. It is the \<page\> tag of
 * the snap XML file format. The theme layout XSLT will be used
 * to generate the final output.
 *
 * \param[in,out] ipath  The path being managed.
 * \param[in,out] page  The page being generated.
 * \param[in,out] body  The body being generated.
 */
void ecommerce::on_generate_main_content(content::path_info_t & ipath, QDomElement & page, QDomElement & body)
{
    // our pages are like any standard pages
    output::output::instance()->on_generate_main_content(ipath, page, body);
}


/** \brief This function is called whenever the e-Payment requires an invoice.
 *
 * Whenever the e-Payment is ready to process an invoice, it sends this
 * signal. This allows any other plugin to replace the e-Commerce plugin
 * and still be able to generate invoices as required.
 *
 * The generation of invoices is expected to happen once a payment was
 * selected and the user cannot cancel anymore. This way we avoid
 * creating invoices that do not get furfilled (although they may be
 * cancelled later and in some cases, like when dealing with a payment
 * facility such as PayPal we may end up without payment anyway.)
 *
 * The function does not return anything, since it is a signal, but it
 * is possible to know whether it worked by testing the \p invoice_number
 * variable. If still zero, then no valid invoice was created and an
 * error was most certainly already generated (i.e. a message was posted.)
 *
 * \note
 * See the e-Payment JavaScript constructor (plugin/epayment/epayment.js)
 * for more information about the invoice status. This function is expected
 * to setup the invoice as "created".
 *
 * \param[in,out] invoice_ipath  Will be set to the invoice ipath if
 *                               invoice_number is not zero on return.
 * \param[in,out] invoice_number  The new invoice number, if zero,
 *                                still undefined.
 * \param[in,out] plist  List of products in the invoice.
 */
void ecommerce::on_generate_invoice(content::path_info_t& invoice_ipath, uint64_t& invoice_number, epayment::epayment_product_list& plist)
{
    // invoice was already defined?
    if(invoice_number != 0)
    {
        return;
    }

    // get the session information
    users::users *users_plugin(users::users::instance());
    QString cart_xml(users_plugin->get_from_session(get_name(name_t::SNAP_NAME_ECOMMERCE_CART_PRODUCTS)));
    if(cart_xml.isEmpty())
    {
        // we should not be able to get here if the cart is empty
        // (although a hacker could send such a request.)
        messages::messages::instance()->set_error(
            "Cart is Empty",
            "Before you can check out, you must include items in your cart.",
            "Someone reached the cart check out when his cart is empty (no XML).",
            false
        );
        return;
    }

    QDomDocument doc;
    doc.setContent(cart_xml);
    QDomXPath products_xpath;
    products_xpath.setXPath("/cart/product");
    QDomXPath::node_vector_t product_tags(products_xpath.apply(doc));
    int const max_products(product_tags.size());

    // the number of products in the cart should always be 1 or more
    if(max_products == 0)
    {
        // we should not be able to get here if no products were in the cart
        // (although a hacker could send such a request.)
        messages::messages::instance()->set_error(
            "Cart is Empty",
            "Before you can check out, you must include items in your cart.",
            "Someone reached the cart check out when his cart is empty (no products).",
            false
        );
        return;
    }

    content::content * content_plugin(content::content::instance());
    QtCassandra::QCassandraTable::pointer_t revision_table(content_plugin->get_revision_table());

    // TODO: loop through all the products to allow for other plugins to
    //       "interfere" (verify) that everything in the cart is fine;
    //       for instance, the stock manager plugin could return an error
    //       saying that a certain product is just not available and the
    //       reseller does not know whether it would be possible to get
    //       more for sale.

    // loop through all the products to make sure they are valid:
    for(int i(0); i < max_products; ++i)
    {
        // got a product
        QDomElement product(product_tags[i].toElement());
        QString const guid(product.attribute("guid"));
        content::path_info_t product_ipath;
        product_ipath.set_path(guid);

        // now give other plugins a chance to verify that the product is
        // allowed to be in this user's cart; if not, the plugin is
        // expected to remove the item from the XML DOM with:
        //     product.parentNode().removeChild(product);
        product_allowed(product, product_ipath);

        // item was removed?
        if(!product.parentNode().isNull())
        {
            content::field_search::search_result_t product_result;

            // TODO: create a "load_product()" function so we do not repeat this
            //       all over theplace! -- only it should probably be in the
            //       e-Payment plugin?
            FIELD_SEARCH
                (content::field_search::command_t::COMMAND_MODE, content::field_search::mode_t::SEARCH_MODE_EACH)
                (content::field_search::command_t::COMMAND_PATH_INFO_REVISION, product_ipath)

                // DESCRIPTION
                (content::field_search::command_t::COMMAND_FIELD_NAME, epayment::get_name(epayment::name_t::SNAP_NAME_EPAYMENT_DESCRIPTION))
                (content::field_search::command_t::COMMAND_SELF)
                (content::field_search::command_t::COMMAND_IF_FOUND, 1)
                    // use page title as a fallback
                    (content::field_search::command_t::COMMAND_FIELD_NAME, content::get_name(content::name_t::SNAP_NAME_CONTENT_TITLE))
                    (content::field_search::command_t::COMMAND_SELF)
                (content::field_search::command_t::COMMAND_LABEL, 1)

                // PRICE
                (content::field_search::command_t::COMMAND_FIELD_NAME, epayment::get_name(epayment::name_t::SNAP_NAME_EPAYMENT_PRICE))
                (content::field_search::command_t::COMMAND_SELF)

                // get the 2 results
                (content::field_search::command_t::COMMAND_RESULT, product_result)

                // retrieve!
                ;

            if(product_result.size() == 2)
            {
                bool ok(false);

                // add a product type
                QString const quantity_string(product.attribute("q"));
                double quantity(quantity_string.toDouble(&ok));
                if(!ok)
                {
                    messages::messages::instance()->set_error(
                        "Invalid Quantity",
                        QString("Could not parse quantity \"%1\" as a valid decimal number.").arg(quantity_string),
                        "We got a cart with an invalid quantity",
                        false
                    );
                    // TBD: should we stop here? At this point we go on
                    //      also the quantity should always be okay...
                    quantity = 1.0;
                }

                QString const product_description(product_result[0].stringValue());
                if(product_result[1].size() != sizeof(int64_t))
                {
                    messages::messages::instance()->set_error(
                        "Invalid Price",
                        "Invalid size of a price in that product definition.",
                        "We got a cart with an invalid price",
                        false
                    );
                    // TBD: should we stop here? At this point we go on
                    //      also the price should always be okay...
                }
                // what kind of a default is that 10.00?!
                double const price(product_result[1].safeDoubleValue(10.00));

                // create a product in the plist
                epayment::epayment_product& p(plist.add_product(product_ipath.get_key(), quantity, product_description));
                p.set_property(epayment::get_name(epayment::name_t::SNAP_NAME_EPAYMENT_PRICE), price);

                // TODO: we need to add support for attributes and put them
                //       in the long description

                // TODO: we need to include other factors (per line taxes, shipping, etc.)
                //       in many cases such fees are calculated on a per line basis
                //       but only the totals are shown below
            }
            else
            {
                // well, could not get basic info, remove it!
                product.parentNode().removeChild(product);
            }
        }
    }

    // search the product tags again, since some could have been removed
    product_tags = products_xpath.apply(doc);
    int const new_max_products = product_tags.size();
    if(new_max_products != max_products)
    {
        // save the new DOM as a string back in the database
        cart_xml = doc.toString(-1);
        users_plugin->attach_to_session(get_name(name_t::SNAP_NAME_ECOMMERCE_CART_PRODUCTS), cart_xml);

        // since the cart changed we need to send it back to the client
        // otherwise the client will show the wrong cart (unless we force
        // a reload of the page, but then we would lose the error messages)
        server_access::server_access::instance()->ajax_append_data(get_name(name_t::SNAP_NAME_ECOMMERCE_CART_MODIFIED_POST_FIELD), cart_xml.toUtf8());
    }

    // the number of products could have dropped to zero now...
    if(new_max_products == 0)
    {
        messages::messages::instance()->set_error(
            "Cart is Empty",
            "All the products in your cart were automatically removed rendering your cart empty.",
            "Plugins decided to remove one or more products from the cart and now it is empty!",
            false
        );
        return;
    }

    // if the number of products changed, we MUST show the new version
    // of the cart to the client before proceeding; this should be
    // pretty rare, but like anything else, it is required
    if(new_max_products != max_products)
    {
        // Note: this error is to make sure that there is a user message
        //       in the end, however, the plugin removing a product should
        //       always itself generate a detailed message.
        messages::messages::instance()->set_error(
            "Cart Auto-Modified",
            "We had to update your cart as some products could not be kept in it. Please check the newer version and feel free to attempt a checkout once ready.",
            "Plugins decided to remove one or more products from the cart so it changed!",
            false
        );
        return;
    }

    // create a lock to generate the next unique invoice number
    content::path_info_t invoices_ipath;
    invoices_ipath.set_path(get_name(name_t::SNAP_NAME_ECOMMERCE_INVOICES_PATH));
    QtCassandra::QCassandraTable::pointer_t content_table(content_plugin->get_content_table());
    QtCassandra::QCassandraRow::pointer_t content_row(content_table->row(invoices_ipath.get_key()));
    {
        snap_lock lock(invoices_ipath.get_key());

        // retrieve the current invoice number and increment by one
        QtCassandra::QCassandraValue invoice_number_value(content_row->cell(get_name(name_t::SNAP_NAME_ECOMMERCE_INVOICE_NUMBER))->value());
        if(invoice_number_value.size() == sizeof(uint64_t))
        {
            invoice_number = invoice_number_value.uint64Value();
        }
        ++invoice_number;
        invoice_number_value.setUInt64Value(invoice_number);
        content_row->cell(get_name(name_t::SNAP_NAME_ECOMMERCE_INVOICE_NUMBER))->setValue(invoice_number_value);
    }
    invoices_ipath.get_child(invoice_ipath, QString("%1").arg(invoice_number));
    invoice_ipath.force_branch(snap_version::SPECIAL_VERSION_USER_FIRST_BRANCH);
    invoice_ipath.force_revision(static_cast<snap_version::basic_version_number_t>(snap_version::SPECIAL_VERSION_FIRST_REVISION));
    invoice_ipath.force_locale("xx"); // TODO: what locale should we use here?!
std::cerr << "***\n*** from invoices " << invoices_ipath.get_key() << " create invoice at: " << invoice_ipath.get_key() << "...\n***\n";

    // create the invoice page
    content_plugin->create_content(invoice_ipath, "ecommerce", "ecommerce/invoice");

    // TODO: as expected in a future version, we will create an object to send
    //       along the create_content() instead of having this separate.
    int64_t const start_date(f_snap->get_start_date());
    QtCassandra::QCassandraRow::pointer_t revision_row(revision_table->row(invoice_ipath.get_revision_key()));
    revision_row->cell(content::get_name(content::name_t::SNAP_NAME_CONTENT_CREATED))->setValue(start_date);
    QString const title(QString("Invoice #%1").arg(invoice_number));
    revision_row->cell(content::get_name(content::name_t::SNAP_NAME_CONTENT_TITLE))->setValue(title);
    QString const body; // empty for now... will be generated later
                        // by a backend or on the fly as we decide then
                        // (we could also have a tag transformed on the fly
                        // something like: [ecommerce::invoice(###)])
    revision_row->cell(content::get_name(content::name_t::SNAP_NAME_CONTENT_BODY))->setValue(body);
    revision_row->cell(get_name(name_t::SNAP_NAME_ECOMMERCE_CART_PRODUCTS))->setValue(cart_xml);

    // the default status is "created" which is likely to be updated
    // right behind this call...
    epayment::epayment::instance()->set_invoice_status(invoice_ipath, epayment::name_t::SNAP_NAME_EPAYMENT_INVOICE_STATUS_CREATED);

    // now it is safe to remove the cart in the session since a copy
    // was just saved in the invoice; in its place we put an invoice
    // URL so for users without an account we still have access
    users_plugin->attach_to_session(get_name(name_t::SNAP_NAME_ECOMMERCE_INVOICE_PATH), invoice_ipath.get_key());
    NOTUSED(users_plugin->detach_from_session(get_name(name_t::SNAP_NAME_ECOMMERCE_CART_PRODUCTS)));

    // The "actual" generation of the invoice should be using an XSLT
    // file and not C++ code; that way we can easily extend the display.
    // We also want to implement a backend to generate a PDF file of the
    // invoice. Tha should be the exact same XML data used with the
    // on-screen XSLT file, only we simplify the output so it works with
    // the HTML to PDF tool we use. The backend can also send an email to
    // the client if they asked for a copy in their email, and fax a copy
    // to the client if so they asked too.
    //
    //double grand_total(0.0);
    //for(int i(0); i < max_products; ++i)
    //{
    //    // we found the product, get its details
    //    QDomElement product(product_tags[i].toElement());

    //    // TODO: verify that the GUID points to a product
    //    QString const guid(product.attribute("guid"));

    //    // get the data in local variables
    //    content::path_info_t product_ipath;
    //    product_ipath.set_path(guid);
    //    content::field_search::search_result_t product_result;
    //    // TODO: create a "load_product()" function so we do not repeat this
    //    //       all over theplace!
    //    FIELD_SEARCH
    //        (content::field_search::COMMAND_MODE, content::field_search::SEARCH_MODE_EACH)
    //        (content::field_search::COMMAND_PATH_INFO_REVISION, product_ipath)

    //        // DESCRIPTION
    //        (content::field_search::COMMAND_FIELD_NAME, epayment::get_name(epayment::name_t::SNAP_NAME_EPAYMENT_DESCRIPTION))
    //        (content::field_search::COMMAND_SELF)
    //        (content::field_search::COMMAND_IF_FOUND, 1)
    //            // use page title as a fallback
    //            (content::field_search::COMMAND_FIELD_NAME, content::get_name(content::name_t::SNAP_NAME_CONTENT_TITLE))
    //            (content::field_search::COMMAND_SELF)
    //        (content::field_search::COMMAND_LABEL, 1)

    //        // PRICE
    //        (content::field_search::COMMAND_FIELD_NAME, epayment::get_name(epayment::name_t::SNAP_NAME_EPAYMENT_PRICE))
    //        (content::field_search::COMMAND_SELF)

    //        // get the 2 results
    //        (content::field_search::COMMAND_RESULT, product_result)

    //        // retrieve!
    //        ;

    //    if(product_result.size() == 2)
    //    {
    //        bool ok;

    //        // add a product type
    //        QString const quantity_string(product.attribute("q"));
    //        double quantity(quantity_string.toDouble(&ok));
    //        if(!ok)
    //        {
    //            messages::messages::instance()->set_error(
    //                "Invalid Quantity",
    //                QString("Could not parse quantity \"%1\" as a valid decimal number.").arg(quantity_string),
    //                "We got a cart with an invalid quantity",
    //                false
    //            );
    //            // TBD: should we stop here? At this point we go on
    //            //      also the quantity should also be okay...
    //            quantity = 1.0;
    //        }

    //        QString const product_description(product_result[0].stringValue());
    //        QString const price_string(product_result[1].safeDoubleValue(10.00)); <- this changed
    //        double price(price_string.toDouble(&ok));
    //        if(!ok)
    //        {
    //            messages::messages::instance()->set_error(
    //                "Invalid Price",
    //                QString("Could not parse price \"%1\" as a valid decimal number.").arg(price_string),
    //                "We got a cart with an invalid price",
    //                false
    //            );
    //            // TBD: should we stop here? At this point we go on
    //            //      also the quantity should also be okay...
    //            price = 10.00;  // what kind of a default is that?!
    //        }

    //        // TODO: we need to add support for attributes

    //        // TODO: we need to include other factors (per line taxes, shipping, etc.)
    //        //       in many cases such fees are calculated on a per line basis
    //        //       but only the totals are shown below
    //        double total(floor(price * quantity * 100.0));

    //        grand_total += total;
    //    }
    //}

    //// TODO: add footer costs

    //grand_total /= 100.0;
}


/** \brief Check whether a product is allowed in this cart.
 *
 * Before creating an invoice for a user, we verify that each product is
 * indeed a product that the user is allowed to checkout. The default
 * function runs the following checks:
 *
 * \li Page has a type (this is very much like a low level system check.)
 * \li Page type is "ecommerce/product", i.e. a product.
 * \li Price is defined, even if negative or zero.
 * \li Current user has enough rights to access the product.
 *
 * Note that the test on whether the user has enough rights should always
 * return true, even if the cart was created when the user was logged in
 * and now he is not. This is because such shops will force the user to
 * log back in whenever they go to the cart checkout.
 *
 * \param[in] keep  The product being checked as defined in the cart XML.
 * \param[in] product_ipath  The path in the database of the product.
 *
 * \return true if the signal should be processed, false otherwise.
 */
bool ecommerce::product_allowed_impl(QDomElement product, content::path_info_t & product_ipath)
{
    // Is this GUID pointing to a page which represents a product at least?
    links::link_info product_info(content::get_name(content::name_t::SNAP_NAME_CONTENT_PAGE_TYPE), true, product_ipath.get_key(), product_ipath.get_branch());
    QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(product_info));
    links::link_info product_child_info;
    if(!link_ctxt->next_link(product_child_info))
    {
        messages::messages::instance()->set_error(
            "Invalid Cart",
            "Your cart includes an invalid product identifier.",
            QString("Page \"%1\" does not have a 'content::page_type' field.").arg(product_ipath.get_key()),
            false
        );
        // This should rather rarely happen.
        // (it could happen if the product was deleted and the
        // user comes back a "few days" later... after the product
        // got completely removed from the main website area
        // i.e. no more redirect or clear error that it was deleted.)
        product.parentNode().removeChild(product);
        return false;
    }

    // the link_info returns a full key with domain name
    // use a path_info_t to retrieve the cpath instead
    content::path_info_t type_ipath;
    type_ipath.set_path(product_child_info.key());
    if(!type_ipath.get_cpath().startsWith(epayment::get_name(epayment::name_t::SNAP_NAME_EPAYMENT_PRODUCT_TYPE_PATH)))
    {
        messages::messages::instance()->set_error(
            "Invalid Cart",
            "Your cart includes an invalid product identifier.",
            QString("GUID \"%1\" does not point to a page representing a product. It has an invalid type.").arg(product_ipath.get_key()),
            false
        );
        // This can happen in the real world since an administrator could
        // transform a page that was a product in a page that is not a
        // product anymore while someone has that product in his/her cart.
        // So we cannot return here...
        product.parentNode().removeChild(product);
        return false;
    }

    // verify that there is a price, without a price it is not a valid
    // product either...
    content::content *content_plugin(content::content::instance());
    QtCassandra::QCassandraTable::pointer_t revision_table(content_plugin->get_revision_table());
    if(revision_table->row(product_ipath.get_revision_key())->cell(epayment::get_name(epayment::name_t::SNAP_NAME_EPAYMENT_PRICE))->value().size() != sizeof(double))
    {
        // no price?!
        messages::messages::instance()->set_error(
            "Invalid Cart",
            "Your cart includes an invalid product identifier.",
            QString("Product GUID \"%1\" does not point to a page representing a product. Price is not defined.").arg(product_ipath.get_key()),
            false
        );
        // Again, the product may have changed between the time the user
        // added it to his cart and now, so we should just remove it from
        // the cart and go on.
        product.parentNode().removeChild(product);
        return false;
    }

    // verify that the user can access the product
    users::users * users_plugin(users::users::instance());
    QString const & login_status(permissions::permissions::instance()->get_login_status());
    QString const & user_path(users_plugin->get_user_info().get_user_path(false));
    content::permission_flag allowed;
    path::path::instance()->access_allowed(user_path, product_ipath, "view", login_status, allowed);
    if(!allowed.allowed())
    {
        // not allowed?!
        messages::messages::instance()->set_error(
            "Invalid Cart",
            "Your cart includes a product you do not have the right to access.",
            QString("Product GUID \"%1\" is not accessible by this user. It should not have been added to the cart.").arg(product_ipath.get_key()),
            false
        );
        // Again, the product may have been given more stringent
        // permissions since the user added it to his cart and now
        // it is not allowed to have it there...
        product.parentNode().removeChild(product);
        return false;
    }

    // TODO: we probably want to go through the product attributes here
    //       and send another message such as attribute_allowed()...

    return true;
}


void ecommerce::on_replace_token(content::path_info_t & ipath, QDomDocument & xml, filter::filter::token_info_t & token)
{
    NOTUSED(ipath);
    NOTUSED(xml);

    if(!token.is_namespace("ecommerce::"))
    {
        return;
    }

    if(token.is_token("ecommerce::invoice"))
    {
        // parameter is one of:
        //   "last-invoice" -- in this case we use the invoice defined in the cookie
        //   "<relative path to invoice>" -- the direct path to an invoice
        //   "<number>" -- an invoice number
        users::users * users_plugin(users::users::instance());
        content::content * content_plugin(content::content::instance());
        QtCassandra::QCassandraTable::pointer_t content_table(content_plugin->get_content_table());
        bool has_invoice(false);
        content::path_info_t invoice_ipath;
        token.verify_args(1, 1);
        filter::filter::parameter_t which(token.get_arg("which", 0, filter::filter::token_t::TOK_STRING));
        if(which.f_value == "last-invoice")
        {
            // WARNING: this can be bogus if the same user creates two
            //          invoices in two separate browsers...
            QString const invoice(users_plugin->get_from_session(get_name(name_t::SNAP_NAME_ECOMMERCE_INVOICE_PATH)));
            if(!invoice.isEmpty())
            {
                invoice_ipath.set_path(invoice);
                has_invoice = true;
            }
        }
        else if(which.f_value.startsWith("invoices/"))
        {
            // must be followed by a decimal number
            QString const no(which.f_value.right(which.f_value.length() - 9));
            bool ok(false);
            no.toInt(&ok, 10);
            if(ok)
            {
                invoice_ipath.set_path(which.f_value);
                has_invoice = true;
            }
        }
        else // keep invoice number last
        {
            bool ok(false);
            which.f_value.toInt(&ok, 10);
            if(ok)
            {
                invoice_ipath.set_path("invoices/" + which.f_value);
                has_invoice = true;
            }
        }
        if(has_invoice)
        {
            if(content_table->exists(invoice_ipath.get_key())
            && content_table->row(invoice_ipath.get_key())->exists(content::get_name(content::name_t::SNAP_NAME_CONTENT_CREATED)))
            {
                // the invoice exists

                // make sure we have enough permissions to view
                // that invoice
                //
                permissions::permissions * permissions_plugin(permissions::permissions::instance());
                QString const & login_status(permissions_plugin->get_login_status());
                content::permission_flag result;
                path::path::instance()->access_allowed(permissions_plugin->get_user_path(), invoice_ipath, "view", login_status, result);
                if(!result.allowed())
                {
                    token.error(QString("You do not have enough access right to %1.").arg(invoice_ipath.get_cpath()));
                    return;
                }

                locale::locale * locale_plugin(locale::locale::instance());
                locale_plugin->set_timezone();
                locale_plugin->set_locale();
                QString const invoice_status(content_table->row(invoice_ipath.get_key())->cell(epayment::get_name(epayment::name_t::SNAP_NAME_EPAYMENT_INVOICE_STATUS))->value().stringValue());
                int64_t const invoice_date_us(content_table->row(invoice_ipath.get_key())->cell(content::get_name(content::name_t::SNAP_NAME_CONTENT_CREATED))->value().int64Value());
                time_t const invoice_date_sec(invoice_date_us / 1000000);
                QString const invoice_date_str(locale_plugin->format_date(invoice_date_sec));
                QString const invoice_time_str(locale_plugin->format_time(invoice_date_sec));
                token.f_replacement = QString(
                    "<div class=\"ecommerce-invoice\">"
                        "<div class=\"ecommerce-invoice-details\">"
                            "<div class=\"ecommerce-invoice-status\"><span class=\"invoice-label\">Status:</span> <span class=\"invoice-value\">%1</span></div>"
                            "<div class=\"ecommerce-invoice-date\"><span class=\"invoice-label\">Date:</span> <span class=\"invoice-value\">%2</span></div>"
                            "<div class=\"ecommerce-invoice-time\"><span class=\"invoice-label\">Time:</span> <span class=\"invoice-value\">%3</span></div>"
                        "</div>"
                        "<div>At some point we'll actually show the invoice here...</div>"
                    "</div>")
                    .arg(invoice_status)
                    .arg(invoice_date_str)
                    .arg(invoice_time_str);
            }
            else
            {
                // the invoice is missing
                token.error(QString("there is no invoice as defined by \"%1\".").arg(which.f_value));
            }
        }
        else
        {
            // invalid filter usage
            messages::messages::instance()->set_error(
                "e-Commerce Filter Missued",
                QString("We could not determine which invoice to display using \"%1\".").arg(which.f_value),
                "Filter could not determine what the ecommerce::invoice() parameter was about.",
                false
            );
            token.error(QString("unknown e-Commerce invoice specification: \"%1\".").arg(which.f_value));
            return;
        }
    }
}


void ecommerce::on_token_help(filter::filter::token_help_t & help)
{
    help.add_token("ecommerce::invoice",
        "Generate an invoice in HTML of the specified invoice. The first parameter is the invoice number or the words 'last-invoice' [which]. Trying to display an invoice with an invalid number fails with an error. The current user must have enough permissions to view that invoice or an error is generated.");
}



SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
