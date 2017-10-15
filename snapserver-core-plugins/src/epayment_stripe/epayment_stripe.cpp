// Snap Websites Server -- handle the Stripe payment facility
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

#include "epayment_stripe.h"

#include "../editor/editor.h"
#include "../messages/messages.h"
#include "../output/output.h"
#include "../permissions/permissions.h"

#include <snapwebsites/log.h>
#include <snapwebsites/not_reached.h>
#include <snapwebsites/not_used.h>
#include <snapwebsites/qdomhelpers.h>
#include <snapwebsites/tcp_client_server.h>

#include <as2js/json.h>

#include <iostream>
#include <openssl/rand.h>

#include <snapwebsites/poison.h>


SNAP_PLUGIN_START(epayment_stripe, 1, 0)


/* \brief Get a fixed epayment name.
 *
 * The epayment plugin makes use of different names in the database. This
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
    case name_t::SNAP_NAME_EPAYMENT_STRIPE_CANCEL_PLAN_URL:
        return "epayment/stripe/cancel-plan";

    case name_t::SNAP_NAME_EPAYMENT_STRIPE_CANCEL_URL:
        return "epayment/stripe/cancel";

    case name_t::SNAP_NAME_EPAYMENT_STRIPE_CHARGE_URI:
        return "https://api.stripe.com/v1/charges";

    case name_t::SNAP_NAME_EPAYMENT_STRIPE_CLICKED_POST_FIELD:
        return "epayment__epayment_stripe";

    case name_t::SNAP_NAME_EPAYMENT_STRIPE_CREATED:
        return "epayment_stripe::created";

    case name_t::SNAP_NAME_EPAYMENT_STRIPE_CUSTOMER_KEY:
        return "epayment_stripe::customer_key";

    case name_t::SNAP_NAME_EPAYMENT_STRIPE_CUSTOMER_URI:
        return "https://api.stripe.com/v1/customers";

    case name_t::SNAP_NAME_EPAYMENT_STRIPE_DEBUG:
        return "epayment_stripe::debug";

    case name_t::SNAP_NAME_EPAYMENT_STRIPE_LAST_ATTEMPT:
        return "epayment_stripe::last_attempt";

    case name_t::SNAP_NAME_EPAYMENT_STRIPE_MAXIMUM_REPEAT_FAILURES:
        return "epayment_stripe::maximum_repeat_failures";

    case name_t::SNAP_NAME_EPAYMENT_STRIPE_SETTINGS_PATH:
        return "/admin/settings/epayment/stripe";

    case name_t::SNAP_NAME_EPAYMENT_STRIPE_TABLE:
        return "epayment_stripe";

    case name_t::SNAP_NAME_EPAYMENT_STRIPE_TEST_KEY:
        return "sk_test_BQokikJOvBiI2HlWgH4olfQ2";

    case name_t::SNAP_NAME_EPAYMENT_STRIPE_TOKEN_POST_FIELD:
        return "epayment__epayment_stripe_token";

    case name_t::SNAP_NAME_EPAYMENT_STRIPE_VERSION:
        return "2016-03-07";

    // ******************
    //    SECURE NAMES
    // ******************
    case name_t::SNAP_SECURE_NAME_EPAYMENT_STRIPE_CHARGE_HEADER:
        return "epayment_stripe::charge_header";

    case name_t::SNAP_SECURE_NAME_EPAYMENT_STRIPE_CHARGE_INFO:
        return "epayment_stripe::charge_info";

    case name_t::SNAP_SECURE_NAME_EPAYMENT_STRIPE_CHARGE_KEY:
        return "epayment_stripe::charge_key";

    case name_t::SNAP_SECURE_NAME_EPAYMENT_STRIPE_CREATE_CUSTOMER_HEADER:
        return "epayment_stripe::create_customer_header";

    case name_t::SNAP_SECURE_NAME_EPAYMENT_STRIPE_CUSTOMER_INFO:
        return "epayment_stripe::customer_info";

    case name_t::SNAP_SECURE_NAME_EPAYMENT_STRIPE_ERROR_REPLY:
        return "epayment_stripe::error_reply";

    case name_t::SNAP_SECURE_NAME_EPAYMENT_STRIPE_LAST_ERROR_MESSAGE:
        return "epayment_stripe::last_error_message";

    case name_t::SNAP_SECURE_NAME_EPAYMENT_STRIPE_REPEAT_PAYMENT:
        return "epayment_stripe::repeat_payment";

    case name_t::SNAP_SECURE_NAME_EPAYMENT_STRIPE_RETRIEVE_CUSTOMER_HEADER:
        return "epayment_stripe::retrieve_customer_header";

    case name_t::SNAP_SECURE_NAME_EPAYMENT_STRIPE_SECRET:
        return "epayment_stripe::secret";

    case name_t::SNAP_SECURE_NAME_EPAYMENT_STRIPE_TEST_SECRET:
        return "epayment_stripe::test_secret";

    case name_t::SNAP_SECURE_NAME_EPAYMENT_STRIPE_UPDATE_CUSTOMER_ERROR:
        return "epayment_stripe::update_customer_error";

    case name_t::SNAP_SECURE_NAME_EPAYMENT_STRIPE_UPDATE_CUSTOMER_HEADER:
        return "epayment_stripe::update_customer_header";

    case name_t::SNAP_SECURE_NAME_EPAYMENT_STRIPE_USER_KEY:
        return "epayment_stripe::user_key";

    default:
        // invalid index
        throw snap_logic_exception("invalid name_t::SNAP_NAME_EPAYMENT_STRIPE_...");

    }
    NOTREACHED();
}









/** \brief Initialize the epayment_stripe plugin.
 *
 * This function is used to initialize the epayment_stripe plugin object.
 *
 * Various documentations about Stripe available services:
 *
 * \li https://stripe.com/docs/api
 * \li https://stripe.com/docs/connect
 */
epayment_stripe::epayment_stripe()
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Clean up the epayment_stripe plugin.
 *
 * Ensure the epayment_stripe object is clean before it is gone.
 */
epayment_stripe::~epayment_stripe()
{
}


/** \brief Get a pointer to the epayment_stripe plugin.
 *
 * This function returns an instance pointer to the epayment_stripe plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the epayment_stripe plugin.
 */
epayment_stripe * epayment_stripe::instance()
{
    return g_plugin_epayment_stripe_factory.instance();
}


/** \brief Send users to the plugin settings.
 *
 * This path represents this plugin settings.
 */
QString epayment_stripe::settings_path() const
{
    return get_name(name_t::SNAP_NAME_EPAYMENT_STRIPE_SETTINGS_PATH);
}


/** \brief A path or URI to a logo for this plugin.
 *
 * This function returns a 64x64 icons representing this plugin.
 *
 * \return A path to the logo.
 */
QString epayment_stripe::icon() const
{
    return "/images/epayment/stripe-logo-64x64.png";
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
QString epayment_stripe::description() const
{
    return "The stripe e-Payment Facility plugin offers payment from the"
          " client's stripe account.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString epayment_stripe::dependencies() const
{
    return "|editor|epayment_creditcard|filter|messages|output|path|";
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
int64_t epayment_stripe::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2016, 4, 7, 23, 46, 40, content_update);

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
void epayment_stripe::content_update(int64_t variables_timestamp)
{
    NOTUSED(variables_timestamp);

    content::content::instance()->add_xml(get_plugin_name());
}


/** \brief Initialize the epayment_stripe.
 *
 * This function terminates the initialization of the epayment_stripe plugin
 * by registering for various events.
 *
 * \param[in] snap  The child handling this request.
 */
void epayment_stripe::bootstrap(snap_child * snap)
{
    f_snap = snap;

    SNAP_LISTEN(epayment_stripe, "server", server, table_is_accessible, _1, _2);
    SNAP_LISTEN(epayment_stripe, "layout", layout::layout, generate_header_content, _1, _2, _3);
    SNAP_LISTEN(epayment_stripe, "filter", filter::filter, replace_token, _1, _2, _3);
    SNAP_LISTEN(epayment_stripe, "epayment", epayment::epayment, repeat_payment, _1, _2, _3);
}


/** \brief Initialize the epayment_stripe table.
 *
 * This function creates the epayment_stripe table if it does not already
 * exist. Otherwise it simply initializes the f_payment_stripe_table
 * variable member.
 *
 * If the function is not able to create the table an exception is raised.
 *
 * The epayment_stripe table is used to save the payment identifiers so
 * we get an immediate reference back to the invoice. We use the name of
 * the website as the row (no protocol), then the Stripe payment identifier
 * for each invoice.
 *
 * \code
 *    snapwebsites.org
 *       PAY-4327271037362
 *          77  (as an int64_t)
 * \endcode
 *
 * \note
 * The table makes use of the domain only because the same website may
 * support HTTP and HTTPS for the exact same data. However, if your
 * website uses a sub-domain, that will be included. So in the example
 * above it could have been "www.snapwebsites.org" in which case it
 * is different from "snapwebsites.org".
 *
 * \return The pointer to the epayment_stripe table.
 */
QtCassandra::QCassandraTable::pointer_t epayment_stripe::get_epayment_stripe_table()
{
    if(!f_epayment_stripe_table)
    {
        f_epayment_stripe_table = f_snap->get_table(get_name(name_t::SNAP_NAME_EPAYMENT_STRIPE_TABLE));
    }
    return f_epayment_stripe_table;
}


/** \brief Setup page for the editor.
 *
 * The editor has a set of dynamic parameters that the users are offered
 * to setup. These parameters need to be sent to the user and we use this
 * function for that purpose.
 *
 * \todo
 * Look for a way to generate the editor data only if necessary (too
 * complex for now.)
 *
 * \param[in,out] ipath  The path being managed.
 * \param[in,out] header  The header being generated.
 * \param[in,out] metadata  The metadata being generated.
 */
void epayment_stripe::on_generate_header_content(content::path_info_t & ipath, QDomElement & header, QDomElement & metadata)
{
    NOTUSED(ipath);
    NOTUSED(metadata);

    QDomDocument doc(header.ownerDocument());

    // // make sure this is a product, if so, add product fields
    // links::link_info product_info(content::get_name(content::name_t::SNAP_NAME_CONTENT_PAGE_TYPE), true, ipath.get_key(), ipath.get_branch());
    // QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(product_info));
    // links::link_info product_child_info;
    // if(link_ctxt->next_link(product_child_info))
    // {
    //     // the link_info returns a full key with domain name
    //     // use a path_info_t to retrieve the cpath instead
    //     content::path_info_t type_ipath;
    //     type_ipath.set_path(product_child_info.key());
    //     if(type_ipath.get_cpath().startsWith(get_name(name_t::SNAP_NAME_EPAYMENT_PRODUCT_TYPE_PATH)))
    //     {
    //         // if the content is the main page then define the titles and body here
    //         FIELD_SEARCH
    //             (content::field_search::COMMAND_MODE, content::field_search::SEARCH_MODE_EACH)
    //             (content::field_search::COMMAND_ELEMENT, metadata)
    //             (content::field_search::COMMAND_PATH_INFO_REVISION, ipath)

    //             // /snap/head/metadata/epayment
    //             (content::field_search::COMMAND_CHILD_ELEMENT, "epayment")

    //             // /snap/head/metadata/epayment/product-name
    //             (content::field_search::COMMAND_FIELD_NAME, get_name(name_t::SNAP_NAME_EPAYMENT_PRODUCT_DESCRIPTION))
    //             (content::field_search::COMMAND_SELF)
    //             (content::field_search::COMMAND_IF_FOUND, 1)
    //                 // use page title as a fallback
    //                 (content::field_search::COMMAND_FIELD_NAME, content::get_name(content::name_t::SNAP_NAME_CONTENT_TITLE))
    //                 (content::field_search::COMMAND_SELF)
    //             (content::field_search::COMMAND_LABEL, 1)
    //             (content::field_search::COMMAND_SAVE, "product-description")

    //             // /snap/head/metadata/epayment/product-price
    //             (content::field_search::COMMAND_FIELD_NAME, get_name(name_t::SNAP_NAME_EPAYMENT_PRICE))
    //             (content::field_search::COMMAND_SELF)
    //             (content::field_search::COMMAND_SAVE, "product-price")

    //             // generate!
    //             ;
    //     }
    // }

    // we have a test to see whether the Stripe facility was properly setup
    // and if not we do not add the JavaScript because otherwise the button
    // will not work right...
    content::path_info_t settings_ipath;
    settings_ipath.set_path(get_name(name_t::SNAP_NAME_EPAYMENT_STRIPE_SETTINGS_PATH));

    content::content * content_plugin(content::content::instance());
    QtCassandra::QCassandraTable::pointer_t secret_table(content_plugin->get_secret_table());
    QtCassandra::QCassandraRow::pointer_t secret_row(secret_table->row(settings_ipath.get_key()));

    bool const debug(get_debug());

    QString const secret(get_stripe_key(debug));

    if(!secret.isEmpty())
    {
        // TODO: find a way to include e-Payment-Stripe data only if required
        //       (it may already be done! search on add_javascript() for info.)
        content::content::instance()->add_javascript(doc, "epayment-stripe");
        content::content::instance()->add_css(doc, "epayment-stripe");
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
void epayment_stripe::on_generate_main_content(content::path_info_t & ipath, QDomElement & page, QDomElement & body)
{
    // our pages are like any standard pages
    output::output::instance()->on_generate_main_content(ipath, page, body);
}


///** \brief This function gets called when a Stripe specific page gets output.
// *
// * This function has some special handling of the review and cancel
// * back links. These are used to make sure that Stripe information
// * gets saved in Cassandra as soon as possible (instead of waiting
// * for a click on the Cancel or Process buttons.)
// *
// * \param[in] ipath  The path of the page being executed.
// *
// * \return true if the path was properly displayed, false otherwise.
// */
//bool epayment_stripe::on_path_execute(content::path_info_t & ipath)
//{
//    QString const cpath(ipath.get_cpath());
//std::cerr << "***\n*** epayment_stripe::on_path_execute() cpath = [" << cpath << "]\n***\n";
//    if(cpath == get_name(name_t::SNAP_NAME_EPAYMENT_STRIPE_CANCEL_URL)
//    || cpath == get_name(name_t::SNAP_NAME_EPAYMENT_STRIPE_CANCEL_PLAN_URL))
//    {
//        QtCassandra::QCassandraTable::pointer_t epayment_stripe_table(get_epayment_stripe_table());
//
//        // the user canceled that invoice...
//        //
//        // http://www.your-domain.com/epayment/stripe/return?token=EC-123
//        //
//        snap_uri const main_uri(f_snap->get_uri());
//        if(!main_uri.has_query_option("token"))
//        {
//            messages::messages::instance()->set_error(
//                "Stripe Missing Option",
//                "Stripe returned to \"cancel\" invoice without a \"token\" parameter",
//                "Without the \"token\" parameter we cannot know which invoice this is linked with.",
//                false
//            );
//            f_snap->page_redirect(epayment::get_name(epayment::name_t::SNAP_NAME_EPAYMENT_FAILED_PATH), snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
//            NOTREACHED();
//        }
//        else
//        {
//            QString const token(main_uri.query_option("token"));
//
//            cancel_invoice(token);
//
//            f_snap->page_redirect(epayment::get_name(epayment::name_t::SNAP_NAME_EPAYMENT_CANCELED_PATH), snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
//            NOTREACHED();
//        }
//    }
//    else if(cpath == get_name(name_t::SNAP_NAME_EPAYMENT_STRIPE_RETURN_URL))
//    {
//        QtCassandra::QCassandraTable::pointer_t epayment_stripe_table(get_epayment_stripe_table());
//
//        for(;;)
//        {
//            // the user approved the payment!
//            // we can now execute it (immediately)
//            // then show the "thank you" page (also called return page)
//            //
//            // http://www.your-domain.com/epayment/stripe/return?paymentId=PAY-123&token=EC-123&PayerID=123
//            //
//            snap_uri const main_uri(f_snap->get_uri());
//            if(!main_uri.has_query_option("paymentId"))
//            {
//                messages::messages::instance()->set_error(
//                    "Stripe Missing Option",
//                    "Stripe replied without a paymentId parameter",
//                    "Without the \"paymentId\" parameter we cannot know which invoice this is linked with.",
//                    false
//                );
//                break;
//            }
//
//            QString const id(main_uri.query_option("paymentId"));
//std::cerr << "*** paymentId is [" << id << "] [" << main_uri.full_domain() << "]\n";
//            QString const date_invoice(epayment_stripe_table->row(main_uri.full_domain())->cell("id/" + id)->value().stringValue());
//            int const pos(date_invoice.indexOf(','));
//            if(pos < 1)
//            {
//                messages::messages::instance()->set_error(
//                    "Stripe Invalid Token",
//                    "Agreement token is missing the date of creation",
//                    "Somehow the agreement token does not include a comma and thus a \"date,invoice\".",
//                    false
//                );
//                break;
//            }
//            QString const token_date(date_invoice.mid(0, pos));
//            bool ok(false);
//            int64_t const token_date_created(token_date.toLongLong(&ok, 10));
//            if(!ok)
//            {
//                messages::messages::instance()->set_error(
//                    "Stripe Missing Option",
//                    "Stripe replied without a \"token\" parameter",
//                    "Without the \"token\" parameter we cannot know which invoice this is linked with.",
//                    false
//                );
//                break;
//            }
//            QString const invoice(date_invoice.mid(pos + 1));
//            content::path_info_t invoice_ipath;
//            invoice_ipath.set_path(invoice);
//
//            epayment::epayment *epayment_plugin(epayment::epayment::instance());
//
//            // TODO: add a test to see whether the invoice has already been
//            //       accepted, if so running the remainder of the code here
//            //       may not be safe (i.e. this would happen if the user hits
//            //       Reload on his browser.)
//            epayment::name_t status(epayment_plugin->get_invoice_status(invoice_ipath));
//            if(status != epayment::name_t::SNAP_NAME_EPAYMENT_INVOICE_STATUS_PENDING)
//            {
//                // TODO: support a default page in this case if the user is
//                //       the correct user (this is only for people who hit
//                //       reload, so no big deal right now)
//                messages::messages::instance()->set_error(
//                    "Stripe Processed",
//                    "Stripe invoice was already processed. Please go to your account to view your existing invoices.",
//                    QString("Found the invoice, but somehow it is not marked \"pending\" (it is \"%1\" instead).").arg(epayment::get_name(status)),
//                    false
//                );
//                break;
//            }
//
//            // Now get the payer identifier
//            if(!main_uri.has_query_option("PayerID"))
//            {
//                messages::messages::instance()->set_error(
//                    "Stripe Missing Option",
//                    "Stripe replied without a paymentId parameter",
//                    "Without the \"paymentId\" parameter we cannot know which invoice this is linked with.",
//                    false
//                );
//                break;
//            }
//            QString const payer_id(main_uri.query_option("PayerID"));
//
//            content::content *content_plugin(content::content::instance());
//            QtCassandra::QCassandraTable::pointer_t content_table(content_plugin->get_content_table());
//            QtCassandra::QCassandraTable::pointer_t secret_table(content_plugin->get_secret_table());
//            QtCassandra::QCassandraRow::pointer_t secret_row(secret_table->row(invoice_ipath.get_key()));
//
//            // save the PayerID value
//            secret_row->cell(get_name(name_t::SNAP_SECURE_NAME_EPAYMENT_STRIPE_PAYER_ID))->setValue(payer_id);
//
//            // Optionally, we may get a token that we check, just in case
//            // (for Stripe payments this token is not used at this time)
//            if(main_uri.has_query_option("token"))
//            {
//                // do we have a match?
//                QString const token(main_uri.query_option("token"));
//                QString const expected_token(secret_row->cell(get_name(name_t::SNAP_SECURE_NAME_EPAYMENT_STRIPE_PAYMENT_TOKEN))->value().stringValue());
//                if(expected_token != token)
//                {
//                    messages::messages::instance()->set_error(
//                        "Invalid Token",
//                        "Somehow the token identifier returned by Stripe was not the same as the one saved in your purchase. We cannot proceed with your payment.",
//                        QString("The payment token did not match (expected \"%1\", got \"%2\").").arg(expected_token).arg(token),
//                        false
//                    );
//                    break;
//                }
//            }
//
//            // Finally verify that the user is still the same guy using
//            // our cookie
//            users::users * users_plugin(users::users::instance());
//            QString const saved_id(users_plugin->detach_from_session(get_name(name_t::SNAP_SECURE_NAME_EPAYMENT_STRIPE_PAYMENT_ID)));
//            if(saved_id != id)
//            {
//                messages::messages::instance()->set_error(
//                    "Invalid Identifier",
//                    "Somehow the payment identifier returned by Stripe was not the same as the one saved in your session.",
//                    "If the identifiers do not match, we cannot show that user the corresponding cart if the user is not logged in.",
//                    false
//                );
//                break;
//            }
//
//            // TODO: add settings so the administrator can choose to setup
//            //       the amount of time to or or less than 1 day
//            int64_t const start_date(f_snap->get_start_date());
//            if(start_date > token_date_created + 86400000000LL) // 1 day in micro seconds
//            {
//                messages::messages::instance()->set_error(
//                    "Session Timedout",
//                    "You generated this payment more than a day ago. It timed out. Sorry about the trouble, but you have to start your order over.",
//                    "The invoice was created 1 day ago so this could be a hacker trying to get this invoice validated.",
//                    false
//                );
//                break;
//            }
//
//            // the URL to send the execute request to Stripe is saved in the
//            // invoice secret area
//            QString const execute_url(secret_row->cell(get_name(name_t::SNAP_SECURE_NAME_EPAYMENT_STRIPE_EXECUTE_PAYMENT))->value().stringValue());
//
//            http_client_server::http_client http;
//            //http.set_keep_alive(true); -- this is the default
//
//            std::string token_type;
//            std::string access_token;
//            if(!get_oauth2_token(http, token_type, access_token))
//            {
//                // a message was already generated if false
//                break;
//            }
//
//            //
//            // Ready to send the Execute message to Stripe, the payer identifier
//            // is the identifier we received in the last GET. The HTTP header is
//            // about the same as when sending a create payment order:
//            //
//            //   {
//            //     "payer_id": "123"
//            //   }
//            //
//            // Execute replies look like this:
//            //
//            //   {
//            //     "id": "PAY-123",
//            //     "create_time": "2014-12-31T23:18:55Z",
//            //     "update_time": "2014-12-31T23:19:39Z",
//            //     "state": "approved",
//            //     "intent": "sale",
//            //     "payer":
//            //     {
//            //       "payment_method": "stripe",
//            //       "payer_info":
//            //       {
//            //         "email": "stripe-buyer@stripe.com",
//            //         "first_name": "Test",
//            //         "last_name": "Buyer",
//            //         "payer_id": "123",
//            //         "shipping_address":
//            //         {
//            //           "line1": "1 Main St",
//            //           "city": "San Jose",
//            //           "state": "CA",
//            //           "postal_code": "95131",
//            //           "country_code": "US",
//            //           "recipient_name": "Test Buyer"
//            //         }
//            //       }
//            //     },
//            //     "transactions":
//            //     [
//            //       {
//            //         "amount":
//            //         {
//            //           "total": "111.34",
//            //           "currency": "USD",
//            //           "details":
//            //           {
//            //             "subtotal": "111.34"
//            //           }
//            //         },
//            //         "description": "Hello from Snap! Websites",
//            //         "related_resources":
//            //         [
//            //           {
//            //             "sale":
//            //             {
//            //               "id": "123",
//            //               "create_time": "2014-12-31T23:18:55Z",
//            //               "update_time": "2014-12-31T23:19:39Z",
//            //               "amount":
//            //               {
//            //                 "total": "111.34",
//            //                 "currency": "USD"
//            //               },
//            //               "payment_mode": "INSTANT_TRANSFER",
//            //               "state": "completed",
//            //               "protection_eligibility": "ELIGIBLE",
//            //               "protection_eligibility_type": "ITEM_NOT_RECEIVED_ELIGIBLE,UNAUTHORIZED_PAYMENT_ELIGIBLE",
//            //               "parent_payment": "PAY-123",
//            //               "links":
//            //               [
//            //                 {
//            //                   "href": "https://api.sandbox.stripe.com/v1/payments/sale/123",
//            //                   "rel": "self",
//            //                   "method": "GET"
//            //                 },
//            //                 {
//            //                   "href": "https://api.sandbox.stripe.com/v1/payments/sale/123/refund",
//            //                   "rel": "refund",
//            //                   "method": "POST"
//            //                 },
//            //                 {
//            //                   "href": "https://api.sandbox.stripe.com/v1/payments/payment/PAY-123",
//            //                   "rel": "parent_payment",
//            //                   "method": "GET"
//            //                 }
//            //               ]
//            //             }
//            //           }
//            //         ]
//            //       }
//            //     ],
//            //     "links":
//            //     [
//            //       {
//            //         "href": "https://api.sandbox.stripe.com/v1/payments/payment/PAY-123",
//            //         "rel": "self",
//            //         "method": "GET"
//            //       }
//            //     ]
//            //   }
//            //
//            QString const body(QString(
//                        "{"
//                            "\"payer_id\":\"%1\""
//                        "}"
//                    ).arg(payer_id)
//                );
//
//            http_client_server::http_request execute_request;
//            // execute_url is a full URL, for example:
//            //   https://api.sandbox.stripe.com/v1/payments/payment/PAY-123/execute
//            // and the set_uri() function takes care of everything for us in that case
//            execute_request.set_uri(execute_url.toUtf8().data());
//            //execute_request.set_path("...");
//            //execute_request.set_port(443); // https
//            execute_request.set_header("Accept", "application/json");
//            execute_request.set_header("Accept-Language", "en_US");
//            execute_request.set_header("Content-Type", "application/json");
//            execute_request.set_header("Authorization", QString("%1 %2").arg(token_type.c_str()).arg(access_token.c_str()).toUtf8().data());
//            execute_request.set_header("Stripe-Request-Id", invoice_ipath.get_key().toUtf8().data());
//            execute_request.set_data(body.toUtf8().data());
//            http_client_server::http_response::pointer_t response(http.send_request(execute_request));
//
//            secret_row->cell(get_name(name_t::SNAP_SECURE_NAME_EPAYMENT_STRIPE_EXECUTED_PAYMENT_HEADER))->setValue(QString::fromUtf8(response->get_original_header().c_str()));
//            secret_row->cell(get_name(name_t::SNAP_SECURE_NAME_EPAYMENT_STRIPE_EXECUTED_PAYMENT))->setValue(QString::fromUtf8(response->get_response().c_str()));
//
//            // looks pretty good, check the actual answer...
//            as2js::JSON::pointer_t json(new as2js::JSON);
//            as2js::StringInput::pointer_t in(new as2js::StringInput(response->get_response()));
//            as2js::JSON::JSONValue::pointer_t value(json->parse(in));
//            if(!value)
//            {
//                SNAP_LOG_ERROR("JSON parser failed parsing 'execute' response");
//                throw epayment_stripe_exception_io_error("JSON parser failed parsing 'execute' response");
//            }
//            as2js::JSON::JSONValue::object_t const& object(value->get_object());
//
//            // ID
//            // verify that the payment identifier corresponds to what we expect
//            if(object.find("id") == object.end())
//            {
//                SNAP_LOG_ERROR("'id' missing in 'execute' response");
//                throw epayment_stripe_exception_io_error("'id' missing in 'execute' response");
//            }
//            QString const execute_id(QString::fromUtf8(object.at("id")->get_string().to_utf8().c_str()));
//            if(execute_id != id)
//            {
//                SNAP_LOG_ERROR("'id' in 'execute' response is not the same as the invoice 'id'");
//                throw epayment_stripe_exception_io_error("'id' in 'execute' response is not the same as the invoice 'id'");
//            }
//
//            // INTENT
//            // verify that: "intent" == "sale"
//            if(object.find("intent") == object.end())
//            {
//                SNAP_LOG_ERROR("'intent' missing in 'execute' response");
//                throw epayment_stripe_exception_io_error("'intent' missing in 'execute' response");
//            }
//            if(object.at("intent")->get_string() != "sale")
//            {
//                SNAP_LOG_ERROR("'intent' in 'execute' response is not 'sale'");
//                throw epayment_stripe_exception_io_error("'intent' in 'execute' response is not 'sale'");
//            }
//
//            // STATE
//            // now check the state of the sale
//            if(object.find("state") == object.end())
//            {
//                SNAP_LOG_ERROR("'state' missing in 'execute' response");
//                throw epayment_stripe_exception_io_error("'state' missing in 'execute' response");
//            }
//            if(object.at("state")->get_string() == "approved")
//            {
//                // the execute succeeded, mark the invoice as paid
//                epayment_plugin->set_invoice_status(invoice_ipath, epayment::name_t::SNAP_NAME_EPAYMENT_INVOICE_STATUS_PAID);
//            }
//            else
//            {
//                // the execute did not approve the sale
//                // mark the invoice as failed...
//                epayment_plugin->set_invoice_status(invoice_ipath, epayment::name_t::SNAP_NAME_EPAYMENT_INVOICE_STATUS_FAILED);
//            }
//
//            f_snap->page_redirect(epayment::get_name(epayment::name_t::SNAP_NAME_EPAYMENT_THANK_YOU_PATH), snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
//            NOTREACHED();
//            break;
//        }
//        // redirect the user to a failure page
//        f_snap->page_redirect(epayment::get_name(epayment::name_t::SNAP_NAME_EPAYMENT_FAILED_PATH), snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
//        NOTREACHED();
//    }
//    else if(cpath == get_name(name_t::SNAP_NAME_EPAYMENT_STRIPE_RETURN_PLAN_URL))
//    {
//        // use a for() so we can 'break' instead of goto-ing
//        for(;;)
//        {
//            // the user approved the agreement!
//            // we can now execute it (immediately)
//            // then show the "thank you" page (also called return page)
//            //
//            // http://www.your-domain.com/epayment/stripe/return-plan?token=EC-123
//            //
//            snap_uri const main_uri(f_snap->get_uri());
//            if(!main_uri.has_query_option("token"))
//            {
//                messages::messages::instance()->set_error(
//                    "Stripe Missing Option",
//                    "Stripe replied without a \"token\" parameter",
//                    "Without the \"token\" parameter we cannot know which invoice this is linked with.",
//                    false
//                );
//                break;
//            }
//
//            QtCassandra::QCassandraTable::pointer_t epayment_stripe_table(get_epayment_stripe_table());
//
//            QString const token(main_uri.query_option("token"));
//SNAP_LOG_WARNING("*** token is [")(token)("] [")(main_uri.full_domain())("]");
//            QString const date_invoice(epayment_stripe_table->row(main_uri.full_domain())->cell("agreement/" + token)->value().stringValue());
//            int const pos(date_invoice.indexOf(','));
//            if(pos < 1)
//            {
//                messages::messages::instance()->set_error(
//                    "Stripe Invalid Token",
//                    "Agreement token is missing the date of creation",
//                    "Somehow the agreement token does not include a comma and thus a \"date,invoice\".",
//                    false
//                );
//                break;
//            }
//            QString const token_date(date_invoice.mid(0, pos));
//            bool ok(false);
//            int64_t const token_date_created(token_date.toLongLong(&ok, 10));
//            if(!ok)
//            {
//                messages::messages::instance()->set_error(
//                    "Stripe Missing Option",
//                    "Stripe agreement has an invalid invoice date.",
//                    QString("Invoice date \"%1\" is not a valid number.").arg(token_date),
//                    false
//                );
//                break;
//            }
//            QString const invoice(date_invoice.mid(pos + 1));
//            content::path_info_t invoice_ipath;
//            invoice_ipath.set_path(invoice);
//
//            epayment::epayment * epayment_plugin(epayment::epayment::instance());
//
//            // TODO: add a test to see whether the invoice has already been
//            //       accepted, if so running the remainder of the code here
//            //       may not be safe (i.e. this would happen if the user hits
//            //       Reload on his browser--to avoid that, we will want to
//            //       redirect the user once more.)
//            epayment::name_t status(epayment_plugin->get_invoice_status(invoice_ipath));
//            if(status != epayment::name_t::SNAP_NAME_EPAYMENT_INVOICE_STATUS_PENDING)
//            {
//                // TODO: support a default page in this case if the user is
//                //       the correct user (this is only for people who hit
//                //       reload, so no big deal right now)
//                messages::messages::instance()->set_error(
//                    "Stripe Processed",
//                    "Stripe invoice was already processed. Please go to your account to view your existing invoices.",
//                    QString("Found the invoice, but somehow it is not marked \"pending\" (it is \"%1\" instead).").arg(epayment::get_name(status)),
//                    false
//                );
//                break;
//            }
//
//            content::content * content_plugin(content::content::instance());
//            QtCassandra::QCassandraTable::pointer_t content_table(content_plugin->get_content_table());
//            QtCassandra::QCassandraTable::pointer_t secret_table(content_plugin->get_secret_table());
//            QtCassandra::QCassandraRow::pointer_t secret_row(secret_table->row(invoice_ipath.get_key()));
//
//            // No saved ID for agreements...
//            //
//            // TODO: replace that check with the token!
//            //
//            // Finally verify that the user is still the same guy using
//            // our cookie
//            //users::users * users_plugin(users::users::instance());
//            //QString const saved_id(users_plugin->detach_from_session(get_name(name_t::SNAP_SECURE_NAME_EPAYMENT_STRIPE_PAYMENT_ID)));
//            //if(saved_id != id)
//            //{
//            //    messages::messages::instance()->set_error(
//            //        "Invalid Identifier",
//            //        "Somehow the payment identifier returned by Stripe was not the same as the one saved in your session.",
//            //        "If the identifiers do not match, we cannot show that user the corresponding cart if the user is not logged in.",
//            //        false
//            //    );
//            //    break;
//            //}
//
//            // TODO: add settings so the administrator can choose to setup
//            //       the amount of time to or or less than 1 day
//            int64_t const start_date(f_snap->get_start_date());
//            if(start_date > token_date_created + 86400000000LL) // 1 day in micro seconds
//            {
//                messages::messages::instance()->set_error(
//                    "Session Timedout",
//                    "You generated this payment more than a day ago. It timed out. Sorry about the trouble, but you have to start your order over.",
//                    "The invoice was created 1 day ago so this could be a hacker trying to get this invoice validated.",
//                    false
//                );
//                break;
//            }
//
//            // the URL to send the execute request to Stripe is saved in the
//            // invoice secret area
//            QString const execute_url(secret_row->cell(get_name(name_t::SNAP_SECURE_NAME_EPAYMENT_STRIPE_EXECUTE_AGREEMENT))->value().stringValue());
//
//            http_client_server::http_client http;
//            //http.set_keep_alive(true) -- this is the default;
//
//            std::string token_type;
//            std::string access_token;
//            if(!get_oauth2_token(http, token_type, access_token))
//            {
//                // a message was already generated if false
//                break;
//            }
//
//            //
//            // Ready to send the Execute message to Stripe, the payer identifier
//            // is the identifier we received in the last GET. The HTTP header is
//            // about the same as when sending a create payment order:
//            //
//            //   {
//            //   }
//            //
//            // Execute replies look like this:
//            //
//            //   {
//            //      "id":"I-NFW80MGXX0YC",
//            //      "links":
//            //          [
//            //              {
//            //                  "href":"https://api.sandbox.stripe.com/v1/payments/billing-agreements/I-NFW80MGXX0YC",
//            //                  "rel":"self",
//            //                  "method":"GET"
//            //              }
//            //          ]
//            //   }
//            //
//
//            http_client_server::http_request execute_request;
//            // execute_url is a full URL, for example:
//            //   https://api.sandbox.stripe.com/v1/payments/payment/PAY-123/execute
//            // and the set_uri() function takes care of everything for us in that case
//            execute_request.set_uri(execute_url.toUtf8().data());
//            //execute_request.set_path("...");
//            //execute_request.set_port(443); // https
//            execute_request.set_header("Accept", "application/json");
//            execute_request.set_header("Accept-Language", "en_US");
//            execute_request.set_header("Content-Type", "application/json");
//            execute_request.set_header("Authorization", QString("%1 %2").arg(token_type.c_str()).arg(access_token.c_str()).toUtf8().data());
//            execute_request.set_header("Stripe-Request-Id", create_unique_request_id(invoice_ipath.get_key()));
//            execute_request.set_data("{}");
//            //SNAP_LOG_INFO("Request: ")(execute_request.get_request());
//            http_client_server::http_response::pointer_t response(http.send_request(execute_request));
//
//            secret_row->cell(get_name(name_t::SNAP_SECURE_NAME_EPAYMENT_STRIPE_EXECUTED_AGREEMENT_HEADER))->setValue(QString::fromUtf8(response->get_original_header().c_str()));
//            secret_row->cell(get_name(name_t::SNAP_SECURE_NAME_EPAYMENT_STRIPE_EXECUTED_AGREEMENT))->setValue(QString::fromUtf8(response->get_response().c_str()));
//
//            // we need a successful response
//            if(response->get_response_code() != 200
//            && response->get_response_code() != 201)
//            {
//                // I would think that responses with 500+ have no valid JSON
//                QString error_name("undefined");
//                QString error("Unknown error");
//                if(response->get_response_code() < 500)
//                {
//                    as2js::JSON::pointer_t json(new as2js::JSON);
//                    as2js::StringInput::pointer_t in(new as2js::StringInput(response->get_response()));
//                    as2js::JSON::JSONValue::pointer_t value(json->parse(in));
//                    if(value)
//                    {
//                        as2js::JSON::JSONValue::object_t const& object(value->get_object());
//                        if(object.find("message") != object.end())
//                        {
//                            error = QString::fromUtf8(object.at("message")->get_string().to_utf8().c_str());
//                        }
//                        if(object.find("name") != object.end())
//                        {
//                            error_name = QString::fromUtf8(object.at("name")->get_string().to_utf8().c_str());
//                        }
//                    }
//                }
//                messages::messages::instance()->set_error(
//                    "Payment Failed",
//                    QString("Somehow Stripe refused to process your payment: %1").arg(error),
//                    QString("The payment error type is %1.").arg(error_name),
//                    false
//                );
//                break;
//            }
//
//            // looks pretty good, check the actual answer...
//            as2js::JSON::pointer_t json(new as2js::JSON);
//            as2js::StringInput::pointer_t in(new as2js::StringInput(response->get_response()));
//            as2js::JSON::JSONValue::pointer_t value(json->parse(in));
//            if(!value)
//            {
//                SNAP_LOG_ERROR("JSON parser failed parsing 'execute' response");
//                throw epayment_stripe_exception_io_error("JSON parser failed parsing 'execute' response");
//            }
//            as2js::JSON::JSONValue::object_t const& object(value->get_object());
//
//            // ID
//            //
//            // we get a subscription ID in the result
//            if(object.find("id") == object.end())
//            {
//                SNAP_LOG_ERROR("'id' missing in 'execute' response");
//                throw epayment_stripe_exception_io_error("'id' missing in 'execute' response");
//            }
//            QString const execute_id(QString::fromUtf8(object.at("id")->get_string().to_utf8().c_str()));
//            secret_row->cell(get_name(name_t::SNAP_SECURE_NAME_EPAYMENT_STRIPE_AGREEMENT_ID))->setValue(execute_id);
//
//            // LINKS / SELF
//            //
//            // get the link marked as "self", this is the URL we need to
//            // use to handle this recurring payment
//            if(object.find("links") == object.end())
//            {
//                SNAP_LOG_ERROR("agreement links missing");
//                throw epayment_stripe_exception_io_error("agreement links missing");
//            }
//            QString agreement_url;
//            as2js::JSON::JSONValue::array_t const & links(object.at("links")->get_array());
//            size_t const max_links(links.size());
//            for(size_t idx(0); idx < max_links; ++idx)
//            {
//                as2js::JSON::JSONValue::object_t const & link_object(links[idx]->get_object());
//                if(link_object.find("rel") != link_object.end())
//                {
//                    as2js::String const rel(link_object.at("rel")->get_string());
//                    if(rel == "self")
//                    {
//                        // this is it! the URL to send the user to
//                        // the method has to be POST
//                        if(link_object.find("method") == link_object.end())
//                        {
//                            SNAP_LOG_ERROR("Stripe link \"self\" has no \"method\" parameter");
//                            throw epayment_stripe_exception_io_error("Stripe link \"self\" has no \"method\" parameter");
//                        }
//                        // this is set to GET although we can use it with PATCH
//                        // too...
//                        if(link_object.at("method")->get_string() != "GET")
//                        {
//                            SNAP_LOG_ERROR("Stripe link \"self\" has a \"method\" other than \"GET\"");
//                            throw epayment_stripe_exception_io_error("Stripe link \"self\" has a \"method\" other than \"GET\"");
//                        }
//                        if(link_object.find("href") == link_object.end())
//                        {
//                            SNAP_LOG_ERROR("Stripe link \"self\" has no \"href\" parameter");
//                            throw epayment_stripe_exception_io_error("Stripe link \"self\" has no \"href\" parameter");
//                        }
//                        as2js::String const& plan_url_str(link_object.at("href")->get_string());
//                        agreement_url = QString::fromUtf8(plan_url_str.to_utf8().c_str());
//                        secret_row->cell(get_name(name_t::SNAP_SECURE_NAME_EPAYMENT_STRIPE_AGREEMENT_URL))->setValue(agreement_url);
//                    }
//                }
//            }
//
//            if(agreement_url.isEmpty())
//            {
//                SNAP_LOG_ERROR("agreement \"self\" link missing");
//                throw epayment_stripe_exception_io_error("agreement \"self\" link missing");
//            }
//
//            // This is not actually true as far as I know... it gets
//            // paid in 1x recurring period instead...
//            epayment_plugin->set_invoice_status(invoice_ipath, epayment::name_t::SNAP_NAME_EPAYMENT_INVOICE_STATUS_PAID);
//
//            f_snap->page_redirect(epayment::get_name(epayment::name_t::SNAP_NAME_EPAYMENT_THANK_YOU_SUBSCRIPTION_PATH), snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
//            NOTREACHED();
//        }
//        f_snap->page_redirect(epayment::get_name(epayment::name_t::SNAP_NAME_EPAYMENT_FAILED_PATH), snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
//        NOTREACHED();
//    }
//
//    // output the page as the output plugin otherwise would by itself
//    //
//    // TBD: We may want to display an error page instead whenever the
//    //      process fails in some way
//    f_snap->output(layout::layout::instance()->apply_layout(ipath, this));
//
//    return true;
//}


void epayment_stripe::cancel_invoice(QString const & token)
{
    QtCassandra::QCassandraTable::pointer_t epayment_stripe_table(get_epayment_stripe_table());
    snap_uri const main_uri(f_snap->get_uri());
    QString const invoice(epayment_stripe_table->row(main_uri.full_domain())->cell("token/" + token)->value().stringValue());
    content::path_info_t invoice_ipath;
    invoice_ipath.set_path(invoice);

    epayment::epayment * epayment_plugin(epayment::epayment::instance());

    // the current state must be pending for us to cancel anythying
    epayment::name_t status(epayment_plugin->get_invoice_status(invoice_ipath));
    if(status != epayment::name_t::SNAP_NAME_EPAYMENT_INVOICE_STATUS_PENDING)
    {
        // TODO: support a default page in this case if the user is
        //       the correct user (this is only for people who hit
        //       reload, so no big deal right now)
        messages::messages::instance()->set_error(
            "Stripe Processed",
            "Stripe invoice was already processed. Please go to your account to view your existing invoices.",
            QString("Found the invoice, but somehow it is not marked \"pending\" (it is \"%1\" instead).").arg(epayment::get_name(status)),
            false
        );
        return;
    }

    epayment_plugin->set_invoice_status(invoice_ipath, epayment::name_t::SNAP_NAME_EPAYMENT_INVOICE_STATUS_CANCELED);

    // we can show this invoice to the user, the status will appear
    // those the user can see it was canceled
}


/** \brief Check whether we are running in debug mode or not.
 *
 * This function retrieves the current status of the debug flag from
 * the database.
 *
 * The function caches the result. Backends have to be careful to either
 * not use this value, or force a re-read by clearing the f_debug_defined
 * flag (although the Cassandra cache will also need a reset if we want
 * to really read the current value.)
 *
 * \return true if the Stripe system is currently setup in debug mode.
 */
bool epayment_stripe::get_debug()
{
    if(!f_debug_defined)
    {
        content::path_info_t settings_ipath;
        settings_ipath.set_path(get_name(name_t::SNAP_NAME_EPAYMENT_STRIPE_SETTINGS_PATH));

        content::content * content_plugin(content::content::instance());
        QtCassandra::QCassandraTable::pointer_t revision_table(content_plugin->get_revision_table());
        QtCassandra::QCassandraRow::pointer_t revision_row(revision_table->row(settings_ipath.get_revision_key()));

        // TODO: if backends require it, we want to add a reset of the
        //       revision_row before re-reading the debug flag here

        QtCassandra::QCassandraValue debug_value(revision_row->cell(get_name(name_t::SNAP_NAME_EPAYMENT_STRIPE_DEBUG))->value());
        f_debug = !debug_value.nullValue() && debug_value.signedCharValue();

        f_debug_defined = true;
    }

    return f_debug;
}


/** \brief Retrieve the stripe key so we can connect to stripe.com.
 *
 * This function reads the user stripe key in order to connect to
 * stripe.com.
 *
 * The key gets cached so calling this function many times will not
 * slow down the process much.
 *
 * \return The stripe key.
 */
QString epayment_stripe::get_stripe_key(bool const debug)
{
    int const idx(debug ? 1 : 0);
    if(!f_stripe_key_defined[idx])
    {
        content::path_info_t settings_ipath;
        settings_ipath.set_path(get_name(name_t::SNAP_NAME_EPAYMENT_STRIPE_SETTINGS_PATH));

        content::content * content_plugin(content::content::instance());
        QtCassandra::QCassandraTable::pointer_t secret_table(content_plugin->get_secret_table());
        QtCassandra::QCassandraRow::pointer_t secret_row(secret_table->row(settings_ipath.get_key()));

        // TODO: if backends require it, we want to add a reset of the
        //       revision_row before re-reading the debug flag here

        if(debug)
        {
            // Stripe provides a "public" test key which is really convenient!
            // However, the user should use its own key.
            //
            f_stripe_key[idx] = secret_row->cell(get_name(name_t::SNAP_SECURE_NAME_EPAYMENT_STRIPE_TEST_SECRET))->value().stringValue();
        }
        else
        {
            f_stripe_key[idx] = secret_row->cell(get_name(name_t::SNAP_SECURE_NAME_EPAYMENT_STRIPE_SECRET))->value().stringValue();
        }

        if(f_stripe_key[idx].isEmpty())
        {
            users::users * users_plugin(users::users::instance());
            messages::messages * messages_plugin(messages::messages::instance());

            // check whether the user can access that page, if so, then
            // give them a direct URL to the settings in the message,
            // that should make it easy for them
            //
            // TODO: make sure this works as expected with AJAX calls...
            //
            content::permission_flag settings_permissions;
            path::path::instance()->access_allowed(
                            users_plugin->get_user_info().get_user_path(false),
                            settings_ipath,
                            "administer",
                            permissions::get_name(permissions::name_t::SNAP_NAME_PERMISSIONS_LOGIN_STATUS_REGISTERED),
                            settings_permissions);

            if(settings_permissions.allowed())
            {
                // avoid repeating ourselves once the user is on the
                // very page where he can enter his information
                //
                content::path_info_t main_ipath;
                main_ipath.set_path(f_snap->get_uri().path());
                if(main_ipath.get_key() != settings_ipath.get_key())
                {
                    messages_plugin->set_error(
                            "Stripe Not Setup",
                            QString("The Stripe service is not yet properly setup. Please go to the <a href=\"%1\">Stripe Settings</a> page to enter your Stripe live key.").arg(settings_ipath.get_key()),
                            "An empty key generally happens because the administrator did not yet enter said key.",
                            false);
                }

                server_access::server_access * server_access_plugin(server_access::server_access::instance());
                server_access_plugin->ajax_redirect(settings_ipath.get_key(), "_top");
            }
            else
            {
                // TODO: People who do not have permission should see this
                //       but really only of the very few pages where the
                //       stripe payment would be required. At the same
                //       time, we should not give users without the
                //       required permissions to make a payment with
                //       e-Stripe if not properly setup anyway.
                //
                //messages_plugin->set_error(
                //    "Stripe Not Available",
                //    "The Stripe service is not currently available. Please contact the webmaster so the problem can be resolved.",
                //    "An empty key generally happens because the administrator did not yet enter said key.",
                //    false
                //);
            }

            return QString();
        }

        f_stripe_key_defined[idx] = true;
    }

    return f_stripe_key[idx];
}


/** \brief Get the "maximum repeat failures" the website accepts.
 *
 * This function retrieves the current maximum number of failures that
 * the owner of this website accepts with Stripe recurring fees (plans).
 * After that many, the system gives up and mark the invoice as failed.
 *
 * The function caches the value. Backends have to be careful to either
 * not use this value, or force a re-read by clearing the
 * f_maximum_repeat_failures_defined flag (although the Cassandra cache
 * will also need a reset if we want to really read the current value
 * from any other computer.)
 *
 * \return The maximum number of attempts to run a payment for a
 *         recurring fee.
 */
int8_t epayment_stripe::get_maximum_repeat_failures()
{
    if(!f_maximum_repeat_failures_defined)
    {
        content::path_info_t settings_ipath;
        settings_ipath.set_path(get_name(name_t::SNAP_NAME_EPAYMENT_STRIPE_SETTINGS_PATH));

        content::content * content_plugin(content::content::instance());
        QtCassandra::QCassandraTable::pointer_t revision_table(content_plugin->get_revision_table());
        QtCassandra::QCassandraRow::pointer_t revision_row(revision_table->row(settings_ipath.get_revision_key()));

        QtCassandra::QCassandraValue maximum_repeat_failures_value(revision_row->cell(get_name(name_t::SNAP_NAME_EPAYMENT_STRIPE_MAXIMUM_REPEAT_FAILURES))->value());
        if(maximum_repeat_failures_value.size() == sizeof(int8_t))
        {
            f_maximum_repeat_failures = maximum_repeat_failures_value.signedCharValue();
        }
        else
        {
            // the default is 5
            f_maximum_repeat_failures = 5;
        }

        f_maximum_repeat_failures_defined = true;
    }

    return f_debug;
}


///** \brief Get a current Stripe OAuth2 token.
// *
// * This function returns a currently valid OAuth2 token from the database
// * if available, or from Stripe if the one in the database timed out.
// *
// * Since the default timeout of an OAuth2 token from Stripe is 8h
// * (28800 seconds), we keep and share the token between all clients
// * (however, we do not share between websites since each website may
// * have a different client identifier and secret and thus there is
// * no point in trying to share between websites.)
// *
// * This means the same identifier may end up being used by many end
// * users within the 8h offered.
// *
// * \param[in,out] http  The HTTP request handler.
// * \param[out] token_type  Returns the type of OAuth2 used (i.e. "Bearer").
// * \param[out] access_token  Returns the actual OAuth2 cookie.
// *
// * \return true if the OAuth2 token is valid; false in all other cases.
// */
//bool epayment_stripe::get_oauth2_token(http_client_server::http_client & http, std::string & token_type, std::string & access_token)
//{
//    // make sure token data is as expected by default
//    token_type.clear();
//    access_token.clear();
//
//    // Save the authentication information in the stripe settings
//    // (since it needs to be secret, use the secret table)
//    content::path_info_t settings_ipath;
//    settings_ipath.set_path(get_name(name_t::SNAP_NAME_EPAYMENT_STRIPE_SETTINGS_PATH));
//
//    content::content *content_plugin(content::content::instance());
//    //QtCassandra::QCassandraTable::pointer_t revision_table(content_plugin->get_revision_table());
//    //QtCassandra::QCassandraRow::pointer_t revision_row(revision_table->row(settings_ipath.get_revision_key()));
//    QtCassandra::QCassandraTable::pointer_t secret_table(content_plugin->get_secret_table());
//    QtCassandra::QCassandraRow::pointer_t secret_row(secret_table->row(settings_ipath.get_key()));
//
//    bool const debug(get_debug());
//
//    // This entire job may be used by any user of the system so it has to
//    // be done while locked; it should rarely be a problem unless you have
//    // a really heavy load; although it will have all the data in memory
//    // in that case!
//    snap_lock lock(f_snap->get_context(), settings_ipath.get_key());
//
//    // If there is a saved OAuth2 which is not out of date, use that
//    QtCassandra::QCassandraValue secret_debug_value(secret_row->cell(get_name(name_t::SNAP_NAME_EPAYMENT_STRIPE_DEBUG))->value());
//    if(!secret_debug_value.nullValue()
//    && (secret_debug_value.signedCharValue() != 0) == debug) // if debug flag changed, it's toasted
//    {
//        QtCassandra::QCassandraValue expires_value(secret_row->cell(get_name(name_t::SNAP_SECURE_NAME_EPAYMENT_STRIPE_OAUTH2_EXPIRES))->value());
//        int64_t const current_date(f_snap->get_current_date());
//        if(expires_value.size() == sizeof(int64_t)
//        && expires_value.int64Value() > current_date) // we do not use 'start date' here because it could be wrong if the process was really slow
//        {
//            token_type = secret_row->cell(get_name(name_t::SNAP_SECURE_NAME_EPAYMENT_STRIPE_OAUTH2_TOKEN_TYPE))->value().stringValue().toUtf8().data();
//            access_token = secret_row->cell(get_name(name_t::SNAP_SECURE_NAME_EPAYMENT_STRIPE_OAUTH2_ACCESS_TOKEN))->value().stringValue().toUtf8().data();
//            return true;
//        }
//    }
//
//    QString client_id;
//    QString secret;
//
//    if(debug)
//    {
//        // User setup debug mode for now
//        client_id = secret_row->cell(get_name(name_t::SNAP_SECURE_NAME_EPAYMENT_STRIPE_SANDBOX_CLIENT_ID))->value().stringValue();
//        secret = secret_row->cell(get_name(name_t::SNAP_SECURE_NAME_EPAYMENT_STRIPE_SANDBOX_SECRET))->value().stringValue();
//    }
//    else
//    {
//        // Normal user settings
//        client_id = secret_row->cell(get_name(name_t::SNAP_SECURE_NAME_EPAYMENT_STRIPE_CLIENT_ID))->value().stringValue();
//        secret = secret_row->cell(get_name(name_t::SNAP_SECURE_NAME_EPAYMENT_STRIPE_SECRET))->value().stringValue();
//    }
//
//    if(client_id.isEmpty()
//    || secret.isEmpty())
//    {
//        messages::messages::instance()->set_error(
//            "Stripe not Properly Setup",
//            "Somehow this website Stripe settings are not complete.",
//            "The client_id or secret parameters were not yet defined.",
//            false
//        );
//        return false;
//    }
//
//    // get authorization code
//    //
//    // Stripe example:
//    //   curl -v https://api.sandbox.stripe.com/v1/oauth2/token
//    //     -H "Accept: application/json"
//    //     -H "Accept-Language: en_US"
//    //     -u "EOJ2S-Z6OoN_le_KS1d75wsZ6y0SFdVsY9183IvxFyZp:EClusMEUk8e9ihI7ZdVLF5cZ6y0SFdVsY9183IvxFyZp"
//    //     -d "grant_type=client_credentials"
//    //
//    // Curl output (when using "--trace-ascii -" on the command line):
//    //     0000: POST /v1/oauth2/token HTTP/1.1
//    //     0020: Authorization: Basic RU9KMlMtWjZPb05fbGVfS1MxZDc1d3NaNnkwU0ZkVnN
//    //     0060: ZOTE4M0l2eEZ5WnA6RUNsdXNNRVVrOGU5aWhJN1pkVkxGNWNaNnkwU0ZkVnNZOTE
//    //     00a0: 4M0l2eEZ5WnA=
//    //     00af: User-Agent: curl/7.35.0
//    //     00c8: Host: api.sandbox.stripe.com
//    //     00e6: Accept: application/json
//    //     0100: Accept-Language: en_US
//    //     0118: Content-Length: 29
//    //     012c: Content-Type: application/x-www-form-urlencoded
//    //     015d:
//    //
//    http_client_server::http_request authorization_request;
//    authorization_request.set_host(debug ? "api.sandbox.stripe.com" : "https://api.stripe.com");
//    //authorization_request.set_host("private.m2osw.com");
//    authorization_request.set_path("/v1/oauth2/token");
//    authorization_request.set_port(443); // https
//    authorization_request.set_header("Accept", "application/json");
//    authorization_request.set_header("Accept-Language", "en_US");
//    //authorization_request.set_header("Content-Type", "application/x-www-form-urlencoded"); -- automatic
//    //authorization_request.set_header("Authorization", "Basic " + base64_authorization_token.data());
//    authorization_request.set_basic_auth(client_id.toUtf8().data(), secret.toUtf8().data());
//    authorization_request.set_post("grant_type", "client_credentials");
//    //authorization_request.set_body(...);
//    http_client_server::http_response::pointer_t response(http.send_request(authorization_request));
//
//    // we need a successful response
//    if(response->get_response_code() != 200)
//    {
//        SNAP_LOG_ERROR("OAuth2 request failed");
//        throw epayment_stripe_exception_io_error("OAuth2 request failed");
//    }
//
//    // the response type must be application/json
//    if(!response->has_header("content-type")
//    || response->get_header("content-type") != "application/json")
//    {
//        SNAP_LOG_ERROR("OAuth2 request did not return application/json data");
//        throw epayment_stripe_exception_io_error("OAuth2 request did not return application/json data");
//    }
//
//    // save that info in case of failure we may have a chance to check
//    // what went wrong
//    signed char const debug_flag(debug ? 1 : 0);
//    secret_row->cell(get_name(name_t::SNAP_NAME_EPAYMENT_STRIPE_DEBUG))->setValue(debug_flag);
//    secret_row->cell(get_name(name_t::SNAP_SECURE_NAME_EPAYMENT_STRIPE_OAUTH2_HEADER))->setValue(QString::fromUtf8(response->get_original_header().c_str()));
//    secret_row->cell(get_name(name_t::SNAP_SECURE_NAME_EPAYMENT_STRIPE_OAUTH2_DATA))->setValue(QString::fromUtf8(response->get_response().c_str()));
//
//    // looks pretty good...
//    as2js::JSON::pointer_t json(new as2js::JSON);
//    as2js::StringInput::pointer_t in(new as2js::StringInput(response->get_response()));
//    as2js::JSON::JSONValue::pointer_t value(json->parse(in));
//    if(!value)
//    {
//        SNAP_LOG_ERROR("JSON parser failed parsing 'oauth2' response");
//        throw epayment_stripe_exception_io_error("JSON parser failed parsing 'oauth2' response");
//    }
//    as2js::JSON::JSONValue::object_t const& object(value->get_object());
//
//    // TOKEN TYPE
//    // we should always have a token_type
//    if(object.find("token_type") == object.end())
//    {
//        SNAP_LOG_ERROR("oauth token_type missing");
//        throw epayment_stripe_exception_io_error("oauth token_type missing");
//    }
//    // at this point we expect "Bearer", but we assume it could change
//    // since they are sending us a copy of that string
//    token_type = object.at("token_type")->get_string().to_utf8();
//    secret_row->cell(get_name(name_t::SNAP_SECURE_NAME_EPAYMENT_STRIPE_OAUTH2_TOKEN_TYPE))->setValue(QString::fromUtf8(token_type.c_str()));
//
//    // ACCESS TOKEN
//    // we should always have an access token
//    if(object.find("access_token") == object.end())
//    {
//        SNAP_LOG_ERROR("oauth access_token missing");
//        throw epayment_stripe_exception_io_error("oauth access_token missing");
//    }
//    access_token = object.at("access_token")->get_string().to_utf8();
//    secret_row->cell(get_name(name_t::SNAP_SECURE_NAME_EPAYMENT_STRIPE_OAUTH2_ACCESS_TOKEN))->setValue(QString::fromUtf8(access_token.c_str()));
//
//    // EXPIRES IN
//    // get the amount of time the token will last in seconds
//    if(object.find("expires_in") == object.end())
//    {
//        SNAP_LOG_ERROR("oauth expires_in missing");
//        throw epayment_stripe_exception_io_error("oauth expires_in missing");
//    }
//    // if defined, "expires_in" is an integer
//    int64_t const expires(object.at("expires_in")->get_int64().get());
//    int64_t const start_date(f_snap->get_start_date());
//    // we save an absolute time limit instead of a "meaningless" number of seconds
//    secret_row->cell(get_name(name_t::SNAP_SECURE_NAME_EPAYMENT_STRIPE_OAUTH2_EXPIRES))->setValue(start_date + expires * 1000000);
//
//    // SCOPE
//    // get the scope if available (for info at this point)
//    if(object.find("scope") != object.end())
//    {
//        QString const scope(QString::fromUtf8(object.at("scope")->get_string().to_utf8().c_str()));
//        secret_row->cell(get_name(name_t::SNAP_SECURE_NAME_EPAYMENT_STRIPE_OAUTH2_SCOPE))->setValue(scope);
//    }
//
//    // APP ID
//    // get the application ID if available
//    if(object.find("app_id") != object.end())
//    {
//        QString const app_id(QString::fromUtf8(object.at("app_id")->get_string().to_utf8().c_str()));
//        secret_row->cell(get_name(name_t::SNAP_SECURE_NAME_EPAYMENT_STRIPE_OAUTH2_APP_ID))->setValue(app_id);
//    }
//
//    return true;
//}
//
//
//
///** \brief Retrieve the plan of a product representing a subscription.
// *
// * With Stripe, we have to create a plan, then attach users to that plan
// * to simulate subscriptions. The subscription parameters, defined in
// * the product page, are used to create the Stripe plan.
// *
// * This function retrieves the plan parameters from the product, since
// * those parameter are not changing over time (well... not the plan
// * identifier, at least.) If the product does not yet include a Stripe
// * plan, then one is created.
// *
// * If the creation fails, the function currently throws.
// *
// * \note
// * We immediately activate the plan since there is no need for us to
// * have a plan in the state "CREATED".
// *
// * \param[in] http  The HTTP client object.
// * \param[in] token_type  The OAuth2 token (usually Bearer).
// * \param[in] access_token  The OAuth2 access (a random ID).
// * \param[in] recurring_product  The product marked as a subscription.
// * \param[in] recurring_setup_fee  The recurring fee if any.
// * \param[out] plan_id  Return the identifier of the Stripe plan.
// *
// * \return The URL to the Stripe plan.
// */
//QString epayment_stripe::get_product_plan(http_client_server::http_client & http, std::string const & token_type, std::string const & access_token,
//                                          epayment::epayment_product const & recurring_product, double const recurring_setup_fee, QString & plan_id)
//{
//    // if the product GUID was not defined, then the function throws
//    QString const guid(recurring_product.get_string_property(epayment::get_name(epayment::name_t::SNAP_NAME_EPAYMENT_PRODUCT)));
//    content::path_info_t product_ipath;
//    product_ipath.set_path(guid);
//
//    content::content *content_plugin(content::content::instance());
//    QtCassandra::QCassandraTable::pointer_t revision_table(content_plugin->get_revision_table());
//    QtCassandra::QCassandraRow::pointer_t row(revision_table->row(product_ipath.get_revision_key()));
//    QtCassandra::QCassandraTable::pointer_t secret_table(content_plugin->get_secret_table());
//    QtCassandra::QCassandraRow::pointer_t secret_row(secret_table->row(product_ipath.get_key()));
//
//    // This entire job may be used by any user of the system so it has to
//    // be done while locked; it should not add much downtime to the end
//    // user since users subscribe just once for a while in general
//    snap_lock lock(f_snap->get_context(), product_ipath.get_key());
//
//    plan_id = secret_row->cell(get_name(name_t::SNAP_SECURE_NAME_EPAYMENT_STRIPE_PLAN_ID))->value().stringValue();
//    if(!plan_id.isEmpty())
//    {
//        // although if the name_t::SNAP_SECURE_NAME_EPAYMENT_STRIPE_PLAN_ID is
//        // properly setup, we should always have a valid URL, but just
//        // in case, we verify that; if it is not valid, we create a
//        // new plan...
//        QString const plan_url(secret_row->cell(get_name(name_t::SNAP_SECURE_NAME_EPAYMENT_STRIPE_PLAN_URL))->value().stringValue());
//        if(!plan_url.isEmpty())
//        {
//            return plan_url;
//        }
//    }
//
//    epayment::recurring_t recurring(recurring_product.get_string_property(epayment::get_name(epayment::name_t::SNAP_NAME_EPAYMENT_RECURRING)));
//
//    //
//    // create a plan payment
//    //
//    // Note that the response does not give you any link other
//    // than the created plan. Next you need to PATCH to activate
//    // the plan, then create a billing agreement, send the
//    // user to the approval URL, and finally execute. Then the
//    // plan is considered started.
//    //
//    // https://developer.stripe.com/docs/integration/direct/create-billing-plan/
//    //
//    // XXX -- it looks like new Stripe plans should not be created
//    //        for each user; instead it feels like we should have
//    //        one plan per item with a recurring payment, then we
//    //        subscribe various users to that one plan...
//    //
//    // Stripe example:
//    //      curl -v POST https://api.sandbox.stripe.com/v1/payments/billing-plans
//    //          -H 'Content-Type:application/json'
//    //          -H 'Authorization: Bearer <Access-Token>'
//    //          -d '{
//    //              "name": "T-Shirt of the Month Club Plan",
//    //              "description": "Template creation.",
//    //              "type": "fixed",
//    //              "payment_definitions": [
//    //                  {
//    //                      "name": "Regular Payments",
//    //                      "type": "REGULAR",
//    //                      "frequency": "MONTH",
//    //                      "frequency_interval": "2",
//    //                      "amount": {
//    //                          "value": "100",
//    //                          "currency": "USD"
//    //                      },
//    //                      "cycles": "12",
//    //                      "charge_models": [
//    //                          {
//    //                              "type": "SHIPPING",
//    //                              "amount": {
//    //                                  "value": "10",
//    //                                  "currency": "USD"
//    //                              }
//    //                          },
//    //                          {
//    //                              "type": "TAX",
//    //                              "amount": {
//    //                                  "value": "12",
//    //                                  "currency": "USD"
//    //                              }
//    //                          }
//    //                      ]
//    //                  }
//    //              ],
//    //              "merchant_preferences": {
//    //                  "setup_fee": {
//    //                      "value": "1",
//    //                      "currency": "USD"
//    //                  },
//    //                  "return_url": "http://www.return.com",
//    //                  "cancel_url": "http://www.cancel.com",
//    //                  "auto_bill_amount": "YES",
//    //                  "initial_fail_amount_action": "CONTINUE",
//    //                  "max_fail_attempts": "0"
//    //              }
//    //          }'
//    //
//    // Response:
//    //      {
//    //          "id":"P-123",
//    //          "state":"CREATED",
//    //          "name":"Snap! Website Subscription",
//    //          "description":"Snap! Website Subscription",
//    //          "type":"INFINITE",
//    //          "payment_definitions":
//    //              [
//    //                  {
//    //                      "id":"PD-123",
//    //                      "name":"Product Test 4 -- subscription",
//    //                      "type":"REGULAR",
//    //                      "frequency":"Day",
//    //                      "amount":
//    //                          {
//    //                              "currency":"USD",
//    //                              "value":"2"
//    //                          },
//    //                      "cycles":"0",
//    //                      "charge_models":[],
//    //                      "frequency_interval":"1"
//    //                  }
//    //              ],
//    //          "merchant_preferences":
//    //              {
//    //                  "setup_fee":
//    //                      {
//    //                          "currency":"USD",
//    //                          "value":"0"
//    //                      },
//    //                  "max_fail_attempts":"0",
//    //                  "return_url":"http://csnap.m2osw.com/epayment/stripe/ready",
//    //                  "cancel_url":"http://csnap.m2osw.com/epayment/stripe/cancel",
//    //                  "auto_bill_amount":"NO",
//    //                  "initial_fail_amount_action":"CANCEL"
//    //              },
//    //          "create_time":"2015-01-06T23:21:37.008Z",
//    //          "update_time":"2015-01-06T23:21:37.008Z",
//    //          "links":
//    //              [
//    //                  {
//    //                      "href":"https://api.sandbox.stripe.com/v1/payments/billing-plans/P-123",
//    //                      "rel":"self",
//    //                      "method":"GET"
//    //                  }
//    //              ]
//    //      }
//    //
//
//    // create the body
//    as2js::String temp_str;
//    as2js::Position pos;
//    as2js::JSON::JSONValue::object_t empty_object;
//    as2js::JSON::JSONValue::array_t empty_array;
//    as2js::JSON::JSONValue::pointer_t field;
//    as2js::JSON::JSONValue::pointer_t body(new as2js::JSON::JSONValue(pos, empty_object));
//
//    // NAME
//    QString subscription_name(row->cell(content::get_name(content::name_t::SNAP_NAME_CONTENT_TITLE))->value().stringValue());
//    if(subscription_name.isEmpty())
//    {
//        // setup to a default name although all products should have
//        // a title since it is a mandatory field in a page!
//        subscription_name = "Snap! Websites Subscription";
//    }
//    {
//        temp_str = snap_dom::remove_tags(subscription_name).toUtf8().data();
//        field.reset(new as2js::JSON::JSONValue(pos, temp_str));
//        body->set_member("name", field);
//    }
//
//    // DESCRIPTION
//    QString const subscription_description(row->cell(content::get_name(content::name_t::SNAP_NAME_CONTENT_BODY))->value().stringValue());
//    {
//        if(subscription_description.isEmpty())
//        {
//            temp_str = subscription_name.toUtf8().data();
//        }
//        else
//        {
//            temp_str = snap_dom::remove_tags(subscription_description).toUtf8().data();
//        }
//        field.reset(new as2js::JSON::JSONValue(pos, temp_str));
//        body->set_member("description", field);
//    }
//
//    // TYPE
//    {
//        temp_str = recurring.is_infinite() ? "INFINITE" : "FIXED";
//        field.reset(new as2js::JSON::JSONValue(pos, temp_str));
//        body->set_member("type", field);
//    }
//
//    // PAYMENT DEFINITIONS
//    {
//        as2js::JSON::JSONValue::pointer_t payment_definitions(new as2js::JSON::JSONValue(pos, empty_array));
//        body->set_member("payment_definitions", payment_definitions);
//
//        {
//            as2js::JSON::JSONValue::pointer_t object(new as2js::JSON::JSONValue(pos, empty_object));
//            payment_definitions->set_item(payment_definitions->get_array().size(), object);
//
//            // ID -- set in response
//
//            // NAME
//            temp_str.from_utf8(recurring_product.get_string_property(epayment::get_name(epayment::name_t::SNAP_NAME_EPAYMENT_DESCRIPTION)).toUtf8().data());
//            field.reset(new as2js::JSON::JSONValue(pos, temp_str));
//            object->set_member("name", field);
//
//            // TYPE
//            temp_str.from_utf8("REGULAR");
//            field.reset(new as2js::JSON::JSONValue(pos, temp_str));
//            object->set_member("type", field);
//
//            // FREQUENCY INTERVAL
//            temp_str.from_utf8(QString("%1").arg(recurring.get_frequency() == epayment::recurring_t::FREQUENCY_TWICE_A_MONTH ? 15 : recurring.get_interval()).toUtf8().data());
//            field.reset(new as2js::JSON::JSONValue(pos, temp_str));
//            object->set_member("frequency_interval", field);
//
//            // FREQUENCY
//            switch(recurring.get_frequency())
//            {
//            case epayment::recurring_t::FREQUENCY_DAY:
//                temp_str = "DAY";
//                break;
//
//            case epayment::recurring_t::FREQUENCY_WEEK:
//                temp_str = "WEEK";
//                break;
//
//            case epayment::recurring_t::FREQUENCY_TWICE_A_MONTH:
//                // this is about 15/DAY, we already put 15 in the frequency_interval
//                temp_str = "DAY";
//                break;
//
//            case epayment::recurring_t::FREQUENCY_MONTH:
//                temp_str = "MONTH";
//                break;
//
//            case epayment::recurring_t::FREQUENCY_YEAR:
//                temp_str = "YEAR";
//                break;
//
//            }
//            field.reset(new as2js::JSON::JSONValue(pos, temp_str));
//            object->set_member("frequency", field);
//
//            // CYCLES
//            temp_str.from_utf8(QString("%1").arg(recurring.is_infinite() ? 0 : recurring.get_repeat()).toUtf8().data());
//            field.reset(new as2js::JSON::JSONValue(pos, temp_str));
//            object->set_member("cycles", field);
//
//            // AMOUNT
//            {
//                as2js::JSON::JSONValue::pointer_t amount(new as2js::JSON::JSONValue(pos, empty_object));
//                object->set_member("amount", amount);
//
//                // CURRENCY
//                temp_str = "USD";
//                field.reset(new as2js::JSON::JSONValue(pos, temp_str));
//                amount->set_member("currency", field);
//
//                // VALUE (Stripe expects a string for value)
//                // TODO: the number of decimals depends on the currency
//                //       (from what I read it can be 0, 2, or 3)
//                temp_str.from_utf8(QString("%1").arg(recurring_product.get_total(), 0, 'f', 2).toUtf8().data());
//                field.reset(new as2js::JSON::JSONValue(pos, temp_str));
//                amount->set_member("value", field);
//            } // amount
//
//            // CHARGE MODELS
//            // for shipping and taxes -- not used now
//        }
//
//    } // payment definitions
//
//    // MERCHANT PREFERENCES
//    {
//        as2js::JSON::JSONValue::pointer_t merchant_preferences(new as2js::JSON::JSONValue(pos, empty_object));
//        body->set_member("merchant_preferences", merchant_preferences);
//
//        // ID -- set in response
//
//        // SETUP FEE
//        if(recurring_setup_fee > 0.0)
//        {
//            as2js::JSON::JSONValue::pointer_t setup_fee(new as2js::JSON::JSONValue(pos, empty_object));
//            merchant_preferences->set_member("setup_fee", setup_fee);
//
//            temp_str = "USD";
//            field.reset(new as2js::JSON::JSONValue(pos, temp_str));
//            setup_fee->set_member("currency", field);
//
//            temp_str.from_utf8(QString("%1").arg(recurring_setup_fee, 0, 'f', 2).toUtf8().data());
//            field.reset(new as2js::JSON::JSONValue(pos, temp_str));
//            setup_fee->set_member("value", field);
//        }
//
//        // CANCEL URL
//        content::path_info_t cancel_url;
//        cancel_url.set_path(get_name(name_t::SNAP_NAME_EPAYMENT_STRIPE_CANCEL_URL));
//        temp_str.from_utf8(cancel_url.get_key().toUtf8().data());
//        field.reset(new as2js::JSON::JSONValue(pos, temp_str));
//        merchant_preferences->set_member("cancel_url", field);
//
//        // RETURN URL
//        content::path_info_t return_url;
//        return_url.set_path(get_name(name_t::SNAP_NAME_EPAYMENT_STRIPE_RETURN_PLAN_URL));
//        temp_str.from_utf8(return_url.get_key().toUtf8().data());
//        field.reset(new as2js::JSON::JSONValue(pos, temp_str));
//        merchant_preferences->set_member("return_url", field);
//
//        // NOTIFY URL -- set in response
//
//        // MAX FAIL ATTEMPTS
//        // TODO: default is zero, meaning try forever, have admins
//        //       choose this value
//        temp_str = "0";
//        field.reset(new as2js::JSON::JSONValue(pos, temp_str));
//        merchant_preferences->set_member("max_fail_attempts", field);
//
//        // AUTO BILL AMOUNT
//        // TODO: add support for automatic payments too
//        temp_str = "NO";
//        field.reset(new as2js::JSON::JSONValue(pos, temp_str));
//        merchant_preferences->set_member("auto_bill_amount", field);
//
//        // INITIAL FAIL AMOUNT ACTION
//        // TODO: add support for administration to select that on
//        //       a per product basis
//        temp_str = "CANCEL"; // CONTINUE or CANCEL
//        field.reset(new as2js::JSON::JSONValue(pos, temp_str));
//        merchant_preferences->set_member("initial_fail_amount_action", field);
//
//        // ACCEPTED PAYMENT TYPE -- set in response
//
//        // CHAR SET -- set in respone
//    } // merchant preferences
//
//std::cerr << "***\n*** PLAN JSON BODY: ["
//<< body->to_string().to_utf8()
//<< "]\n***\n";
//
//    http_client_server::http_request create_plan_request;
//    bool const debug(get_debug());
//    create_plan_request.set_host(debug ? "api.sandbox.stripe.com" : "api.stripe.com");
//    create_plan_request.set_path("/v1/payments/billing-plans/");
//    create_plan_request.set_port(443); // https
//    create_plan_request.set_header("Accept", "application/json");
//    create_plan_request.set_header("Accept-Language", "en_US");
//    create_plan_request.set_header("Content-Type", "application/json");
//    create_plan_request.set_header("Authorization", QString("%1 %2").arg(token_type.c_str()).arg(access_token.c_str()).toUtf8().data());
//    create_plan_request.set_header("Stripe-Request-Id", product_ipath.get_key().toUtf8().data());
//    create_plan_request.set_data(body->to_string().to_utf8());
//    http_client_server::http_response::pointer_t response(http.send_request(create_plan_request));
//
//    secret_row->cell(get_name(name_t::SNAP_SECURE_NAME_EPAYMENT_STRIPE_CREATED_PLAN_HEADER))->setValue(QString::fromUtf8(response->get_original_header().c_str()));
//    secret_row->cell(get_name(name_t::SNAP_SECURE_NAME_EPAYMENT_STRIPE_CREATED_PLAN))->setValue(QString::fromUtf8(response->get_response().c_str()));
//
//    // we need a successful response (it should always be 201)
//    if(response->get_response_code() != 200
//    && response->get_response_code() != 201)
//    {
//        SNAP_LOG_ERROR("creating a plan failed");
//        throw epayment_stripe_exception_io_error("creating a plan failed");
//    }
//
//    // the response type must be application/json
//    if(!response->has_header("content-type")
//    || response->get_header("content-type") != "application/json")
//    {
//        SNAP_LOG_ERROR("plan creation request did not return application/json data");
//        throw epayment_stripe_exception_io_error("plan creation request did not return application/json data");
//    }
//
//    // looks pretty good...
//    as2js::JSON::pointer_t json(new as2js::JSON);
//    as2js::StringInput::pointer_t in(new as2js::StringInput(response->get_response()));
//    as2js::JSON::JSONValue::pointer_t value(json->parse(in));
//    if(!value)
//    {
//        SNAP_LOG_ERROR("JSON parser failed parsing plan creation response");
//        throw epayment_stripe_exception_io_error("JSON parser failed parsing plan creation response");
//    }
//    as2js::JSON::JSONValue::object_t const& object(value->get_object());
//
//    // STATE
//    //
//    // the state should be "created" at this point
//    if(object.find("state") == object.end())
//    {
//        SNAP_LOG_ERROR("plan status missing");
//        throw epayment_stripe_exception_io_error("plan status missing");
//    }
//    // TODO: the case should not change, but Stripe suggest you test
//    //       statuses in a case insensitive manner
//    if(object.at("state")->get_string() != "CREATED")
//    {
//        SNAP_LOG_ERROR("Stripe plan status is not \"CREATED\" as expected");
//        throw epayment_stripe_exception_io_error("Stripe plan status is not \"CREATED\" as expected");
//    }
//
//    // ID
//    //
//    // get the "id" of this new plan
//    if(object.find("id") == object.end())
//    {
//        SNAP_LOG_ERROR("plan identifier missing");
//        throw epayment_stripe_exception_io_error("plan identifier missing");
//    }
//    as2js::String const id_string(object.at("id")->get_string());
//    plan_id = QString::fromUtf8(id_string.to_utf8().c_str());
//    secret_row->cell(get_name(name_t::SNAP_SECURE_NAME_EPAYMENT_STRIPE_PLAN_ID))->setValue(plan_id);
//
//    // save a back reference in the epayment_stripe table
//    QtCassandra::QCassandraTable::pointer_t epayment_stripe_table(get_epayment_stripe_table());
//    snap_uri const main_uri(f_snap->get_uri());
//    epayment_stripe_table->row(main_uri.full_domain())->cell("plan/" + plan_id)->setValue(product_ipath.get_key());
//
//    // LINKS / SELF
//    //
//    // get the link marked as "self", this is the URL we need to
//    // apply the following orders to the plan
//    if(object.find("links") == object.end())
//    {
//        SNAP_LOG_ERROR("plan links missing");
//        throw epayment_stripe_exception_io_error("plan links missing");
//    }
//    QString plan_url;
//    as2js::JSON::JSONValue::array_t const& links(object.at("links")->get_array());
//    size_t const max_links(links.size());
//    for(size_t idx(0); idx < max_links; ++idx)
//    {
//        as2js::JSON::JSONValue::object_t const& link_object(links[idx]->get_object());
//        if(link_object.find("rel") != link_object.end())
//        {
//            as2js::String const rel(link_object.at("rel")->get_string());
//            if(rel == "self")
//            {
//                // this is it! the URL to send the user to
//                // the method has to be POST
//                if(link_object.find("method") == link_object.end())
//                {
//                    SNAP_LOG_ERROR("Stripe link \"self\" has no \"method\" parameter");
//                    throw epayment_stripe_exception_io_error("Stripe link \"self\" has no \"method\" parameter");
//                }
//                // this is set to GET although we can use it with PATCH
//                // too...
//                if(link_object.at("method")->get_string() != "GET")
//                {
//                    SNAP_LOG_ERROR("Stripe link \"self\" has a \"method\" other than \"GET\"");
//                    throw epayment_stripe_exception_io_error("Stripe link \"self\" has a \"method\" other than \"GET\"");
//                }
//                if(link_object.find("href") == link_object.end())
//                {
//                    SNAP_LOG_ERROR("Stripe link \"self\" has no \"href\" parameter");
//                    throw epayment_stripe_exception_io_error("Stripe link \"self\" has no \"href\" parameter");
//                }
//                as2js::String const& plan_url_str(link_object.at("href")->get_string());
//                plan_url = QString::fromUtf8(plan_url_str.to_utf8().c_str());
//                secret_row->cell(get_name(name_t::SNAP_SECURE_NAME_EPAYMENT_STRIPE_PLAN_URL))->setValue(plan_url);
//            }
//        }
//    }
//
//    if(plan_url.isEmpty())
//    {
//        SNAP_LOG_ERROR("plan \"self\" link missing");
//        throw epayment_stripe_exception_io_error("payment \"self\" link missing");
//    }
//
//    //
//    // activate the plan immediately
//    //
//    // curl -v -k -X PATCH 'https://api.sandbox.stripe.com/v1/payments/billing-plans/P-123'
//    //      -H "Content-Type: application/json"
//    //      -H "Authorization: Bearer <Access-Token>"
//    //      -d '[
//    //          {
//    //              "path": "/",
//    //              "value": {
//    //                  "state": "ACTIVE"
//    //              },
//    //              "op": "replace"
//    //          }
//    //      ]'
//    //
//
//    // create the body (we reset it in this case)
//    body.reset(new as2js::JSON::JSONValue(pos, empty_array));
//    as2js::JSON::JSONValue::pointer_t update_plan(new as2js::JSON::JSONValue(pos, empty_object));
//    body->set_item(body->get_array().size(), update_plan);
//
//    // OP
//    {
//        temp_str = "replace";
//        field.reset(new as2js::JSON::JSONValue(pos, temp_str));
//        update_plan->set_member("op", field);
//    }
//
//    // PATH
//    {
//        temp_str = "/";
//        field.reset(new as2js::JSON::JSONValue(pos, temp_str));
//        update_plan->set_member("path", field);
//    }
//
//    // VALUE
//    {
//        as2js::JSON::JSONValue::pointer_t value_object(new as2js::JSON::JSONValue(pos, empty_object));
//        update_plan->set_member("value", value_object);
//
//        temp_str = "ACTIVE";
//        field.reset(new as2js::JSON::JSONValue(pos, temp_str));
//        value_object->set_member("state", field);
//    }
//
//std::cerr << "***\n*** ACTIVATED PLAN JSON BODY: ["
//<< body->to_string().to_utf8()
//<< "]\n***\n";
//
//    http_client_server::http_request activate_plan_request;
//    activate_plan_request.set_uri(plan_url.toUtf8().data());
//    //activate_plan_request.set_host(debug ? "api.sandbox.stripe.com" : "api.stripe.com");
//    //activate_plan_request.set_path("/v1/payments/billing-plans/");
//    //activate_plan_request.set_port(443); // https
//    activate_plan_request.set_command("PATCH");
//    activate_plan_request.set_header("Accept", "application/json");
//    activate_plan_request.set_header("Accept-Language", "en_US");
//    activate_plan_request.set_header("Content-Type", "application/json");
//    activate_plan_request.set_header("Authorization", QString("%1 %2").arg(token_type.c_str()).arg(access_token.c_str()).toUtf8().data());
//    activate_plan_request.set_header("Stripe-Request-Id", product_ipath.get_key().toUtf8().data());
//    activate_plan_request.set_data(body->to_string().to_utf8());
//    response = http.send_request(activate_plan_request);
//
//    secret_row->cell(get_name(name_t::SNAP_SECURE_NAME_EPAYMENT_STRIPE_ACTIVATED_PLAN_HEADER))->setValue(QString::fromUtf8(response->get_original_header().c_str()));
//    secret_row->cell(get_name(name_t::SNAP_SECURE_NAME_EPAYMENT_STRIPE_ACTIVATED_PLAN))->setValue(QString::fromUtf8(response->get_response().c_str()));
//std::cerr << "***\n*** answer is [" << QString::fromUtf8(response->get_response().c_str()) << "]\n***\n";
//
//    // we need a successful response (according to the documentation,
//    // it should always be 204, but we are getting a 200 answer)
//    if(response->get_response_code() != 200
//    && response->get_response_code() != 201
//    && response->get_response_code() != 204)
//    {
//        SNAP_LOG_ERROR("marking plan as ACTIVE failed");
//        throw epayment_stripe_exception_io_error("marking plan as ACTIVE failed");
//    }
//
//    return plan_url;
//}




void epayment_stripe::on_replace_token(content::path_info_t & ipath, QDomDocument & xml, filter::filter::token_info_t & token)
{
    NOTUSED(ipath);
    NOTUSED(xml);

    if(!token.is_namespace("epayment_stripe::"))
    {
        return;
    }

    if(token.is_token("epayment_stripe::process_buttons"))
    {
        // buttons used to run the final stripe process (i.e. execute
        // a payment); we also offer a Cancel button, just in case
        snap_uri const main_uri(f_snap->get_uri());
        if(main_uri.has_query_option("paymentId"))
        {
            QtCassandra::QCassandraTable::pointer_t epayment_stripe_table(get_epayment_stripe_table());
            QString const id(main_uri.query_option("paymentId"));
            QString const invoice(epayment_stripe_table->row(main_uri.full_domain())->cell("id/" + id)->value().stringValue());
            content::path_info_t invoice_ipath;
            invoice_ipath.set_path(invoice);

            epayment::epayment *epayment_plugin(epayment::epayment::instance());

            // TODO: add a test to see whether the invoice has already been
            //       accepted, if so running the remainder of the code here
            //       may not be safe (i.e. this would happen if the user hits
            //       Reload on his browser.)
            epayment::name_t status(epayment_plugin->get_invoice_status(invoice_ipath));
            if(status == epayment::name_t::SNAP_NAME_EPAYMENT_INVOICE_STATUS_PENDING)
            {
                token.f_replacement = "<div class=\"epayment_stripe-process-buttons\">"
                        "<a class=\"epayment_stripe-cancel\" href=\"#cancel\">Cancel</a>"
                        "<a class=\"epayment_stripe-process\" href=\"#process\">Process</a>"
                    "</div>";
            }
        }
    }
}


/** \brief Repeat a payment.
 *
 * This function captures a Stripe payment and if possible process a
 * repeat payment. The payment must have been authorized before by the
 * owner of the account.
 *
 * There can be mainly 3 failures although Stripe checks the dates so
 * there are four at this point:
 *
 * \li The user account has never processed such a payment. This should
 *     not happen if your code is all proper.
 * \li The user canceled the repeat payment and thus Stripe refuses to
 *     process any further money transfers.
 * \li The Stripe website is somehow not currently accessible.
 * \li The Stripe website decided that the charged appeared too soon or
 *     too late.
 *
 * Any other error is probably in this code.
 *
 * \param[in] first_invoice_ipath  The very first payment made for that
 *                                 recurring payment to be repeated.
 * \param[in] previous_invoice_ipath  The last invoice that was paid in
 *                                    this plan, or the same as
 *                                    first_invoice_ipath if no other
 *                                    invoices were paid since.
 * \param[in] new_invoice_ipath  The new invoice you just created.
 */
void epayment_stripe::on_repeat_payment(content::path_info_t & first_invoice_ipath, content::path_info_t & previous_invoice_ipath, content::path_info_t & new_invoice_ipath)
{
    NOTUSED(first_invoice_ipath);
    NOTUSED(previous_invoice_ipath);
    NOTUSED(new_invoice_ipath);

    // if no stripe key defined, it cannot be a repeat of a stripe charge
    //
    bool const debug(get_debug());
    QString const stripe_key(get_stripe_key(debug));
    if(stripe_key.isEmpty())
    {
        // we already generated an error if empty, leave now
        return;
    }

    content::content * content_plugin(content::content::instance());

    QtCassandra::QCassandraTable::pointer_t revision_table(content_plugin->get_revision_table());
    QtCassandra::QCassandraTable::pointer_t secret_table(content_plugin->get_secret_table());
    QtCassandra::QCassandraTable::pointer_t epayment_stripe_table(get_epayment_stripe_table());

    QtCassandra::QCassandraRow::pointer_t first_secret_row(secret_table->row(first_invoice_ipath.get_key()));
    QtCassandra::QCassandraValue customer_key(first_secret_row->cell(get_name(name_t::SNAP_SECURE_NAME_EPAYMENT_STRIPE_USER_KEY))->value());
    if(customer_key.nullValue())
    {
        // no Stripe customer, we cannot repeat this payment in this
        // plugin, just leave and let other plugins eventually do some work
        return;
    }

    QtCassandra::QCassandraRow::pointer_t customer_row(epayment_stripe_table->row(customer_key.binaryValue()));

    int64_t const start_date(f_snap->get_start_date());

    http_client_server::http_client http;

    {
        // Now actually charge the customer card
        //
        QtCassandra::QCassandraRow::pointer_t secret_row(secret_table->row(new_invoice_ipath.get_key()));
        QtCassandra::QCassandraRow::pointer_t revision_row(revision_table->row(new_invoice_ipath.get_revision_key()));

        secret_row->cell(get_name(name_t::SNAP_NAME_EPAYMENT_STRIPE_CREATED))->setValue(start_date);

        // create a charge now
        //
        http_client_server::http_request stripe_charge_request;

        // the URI changes slightly in case we are updating
        stripe_charge_request.set_uri(get_name(name_t::SNAP_NAME_EPAYMENT_STRIPE_CHARGE_URI));

        // the set_uri() does the set_path() and set_port() as expected
        //stripe_create_request.set_path("...");
        //stripe_create_request.set_port(443); // https
        stripe_charge_request.set_header("Accept", "application/json");
        stripe_charge_request.set_header("Stripe-Version", get_name(name_t::SNAP_NAME_EPAYMENT_STRIPE_VERSION)); // make sure our requests will work for the version we programmed them for
        //stripe_create_request.set_header("Accept-Language", "en_US"); -- at this time, stripe's API is 100% English
        //stripe_create_request.set_header("Content-Type", "application/x-www-form-urlencoded"); -- automatic with set_post() calls
        stripe_charge_request.set_basic_auth(stripe_key.toUtf8().data(), "");

        // POST variables

        // Basic variables
        double const grand_total(revision_row->cell(epayment::get_name(epayment::name_t::SNAP_NAME_EPAYMENT_GRAND_TOTAL))->value().safeDoubleValue());
        stripe_charge_request.set_post("amount", QString("%1").arg(static_cast<uint64_t>(grand_total * 100.0)).toUtf8().data()); // amount in cents

        // once we have time to add proper support for various currencies:
        // https://support.stripe.com/questions/which-currencies-does-stripe-support
        //
        stripe_charge_request.set_post("currency", "usd"); // force USD for now
        stripe_charge_request.set_post("capture", "true"); // make sure we always capture

        // capturing a fee on that charge?
        //stripe_charge_request.set_post("application_fee", 0); -- useful for systems like Order Made but not yet implemented
        //stripe_charge_request.set_post("destination", ...); -- useful for systems like Order Made but not yet implemented

        // pass the SNAP_NAME_CONTENT_TITLE as description, it often is
        // the invoice number, but could be more descriptive...
        //
        QString const invoice_description(revision_row->cell(content::get_name(content::name_t::SNAP_NAME_CONTENT_TITLE))->value().stringValue());
        stripe_charge_request.set_post("description", snap_dom::remove_tags(invoice_description).toUtf8().data());

        stripe_charge_request.set_post("metadata[user_id]", customer_key.stringValue().toUtf8().data()); // can make it easier to find the customer this way

        // We manage emails and setting the receipt_email would generate
        // a double email so we do not do it!
        //
        //stripe_charge_request.set_post("receipt_email", user_email);

        // If available, it is part of the customer's account
        //
        //stripe_charge_request.set_post("shipping[...]", ...);

        // Customer
        QString const customer_stripe_key(customer_row->cell(get_name(name_t::SNAP_NAME_EPAYMENT_STRIPE_CUSTOMER_KEY))->value().stringValue());
        stripe_charge_request.set_post("customer", customer_stripe_key.toUtf8().data());

        // Source
        //stripe_charge_request.set_post("source[...]", ...); -- the customer information is enough, they will use the default source though
        //                                                       once we support selecting any source, this will change with the source 'id'

        // Description appearing on the credit card bank statements
        //
        content::path_info_t epayment_settings;
        epayment_settings.set_path("admin/settings/epayment/store");
        QtCassandra::QCassandraRow::pointer_t epayment_settings_row(revision_table->row(epayment_settings.get_revision_key()));
        QString const store_name(epayment_settings_row->cell(epayment::get_name(epayment::name_t::SNAP_NAME_EPAYMENT_STORE_NAME))->value().stringValue());
        QString const invoice_number(revision_row->cell(epayment::get_name(epayment::name_t::SNAP_NAME_EPAYMENT_INVOICE_NUMBER))->value().stringValue());
        QString const statement(QString("%1 #%2").arg(store_name).arg(invoice_number));
        stripe_charge_request.set_post("statement_descriptor", statement.toUtf8().data());

        http_client_server::http_response::pointer_t charge_response(http.send_request(stripe_charge_request));

        // we can save the reply header as is
        //
        QtCassandra::QCassandraValue value;
        value.setStringValue(QString::fromUtf8(charge_response->get_original_header().c_str()));
        value.setTtl(365 * 86400); // keep for about 1 year
        secret_row->cell(QString("%1::%2")
                            .arg(get_name(name_t::SNAP_SECURE_NAME_EPAYMENT_STRIPE_CHARGE_HEADER))
                            .arg(start_date))
                  ->setValue(value);

        // NO DIRECT LOGGING OF RESPONSE, SEE WARNING AT START OF epayment_stripe::process_creditcard() FUNCTION
        //secret_row->cell(...)->setValue(QString::fromUtf8(charge_response->get_response().c_str()));

        // Stripe makes it simple, if anything fails, including a
        // payment, then the response code is not 200
        //
        if(charge_response->get_response_code() != 200)
        {
            // in this case we can save the response as errors should never
            // include sensitive data about the customer (TBD!)
            //
            // errors DO happen if the card is not valid (i.e. the customer
            // info, including the card details are checked in this step),
            // even though no charge gets triggered on creation or update!
            //

            log_error(charge_response, secret_row);
            return;
        }

        // looks pretty good, check the actual answer...
        as2js::JSON::pointer_t charge_json(new as2js::JSON);
        as2js::StringInput::pointer_t charge_json_input(new as2js::StringInput(charge_response->get_response()));
        as2js::JSON::JSONValue::pointer_t charge_json_value(charge_json->parse(charge_json_input));
        if(!charge_json_value)
        {
            // TBD: should we not just delete our data and start over?
            SNAP_LOG_FATAL("epayment_stripe::on_repeat_payment() JSON parser failed parsing 'create/update customer' response");
            throw epayment_stripe_exception_io_error("JSON parser failed parsing 'create/update customer' response");
        }
        as2js::JSON::JSONValue::object_t root_object(charge_json_value->get_object());

        // ID
        if(root_object.find("id") == root_object.end())
        {
            SNAP_LOG_FATAL("epayment_stripe::on_repeat_payment() 'id' missing in 'charge' response");
            throw epayment_stripe_exception_io_error("'id' missing in 'charge' response");
        }
        QString const charge_id(QString::fromUtf8(root_object.at("id")->get_string().to_utf8().c_str()));
        secret_row->cell(get_name(name_t::SNAP_SECURE_NAME_EPAYMENT_STRIPE_CHARGE_KEY))->setValue(charge_id);

        // to re-charge the same customer we need a link back to that customer
        secret_row->cell(get_name(name_t::SNAP_SECURE_NAME_EPAYMENT_STRIPE_USER_KEY))->setValue(customer_key);

        // now we want to save the JSON, only it includes some personal
        // data about the user:
        //
        //    brand
        //    dynamic_last4
        //    exp_month
        //    exp_year
        //    fingerprint
        //    last4
        //
        if(root_object.find("sources") != root_object.end())
        {
            as2js::JSON::JSONValue::pointer_t source(root_object.at("source"));
            source->set_member("brand", as2js::JSON::JSONValue::pointer_t());
            source->set_member("dynamic_last4", as2js::JSON::JSONValue::pointer_t());
            source->set_member("exp_month", as2js::JSON::JSONValue::pointer_t());
            source->set_member("exp_year", as2js::JSON::JSONValue::pointer_t());
            source->set_member("fingerprint", as2js::JSON::JSONValue::pointer_t());
            source->set_member("last4", as2js::JSON::JSONValue::pointer_t());
        }

        secret_row->cell(get_name(name_t::SNAP_SECURE_NAME_EPAYMENT_STRIPE_CHARGE_INFO))
                  ->setValue(QString::fromUtf8(charge_json_value->to_string().to_utf8().c_str()));

        epayment::epayment * epayment_plugin(epayment::epayment::instance());
        epayment_plugin->set_invoice_status(new_invoice_ipath, epayment::name_t::SNAP_NAME_EPAYMENT_INVOICE_STATUS_PAID);

        SNAP_LOG_INFO("epayment_stripe::on_repeat_payment() subscription charge succeeded.");
    }
}


std::string epayment_stripe::create_unique_request_id(QString const & main_id)
{
    time_t const start_time(f_snap->get_start_time());
    struct tm t;
    localtime_r(&start_time, &t);
    char buf[BUFSIZ];
    strftime(buf, sizeof(buf), "-%Y%m%d-%H%M%S", &t);
    return main_id.toUtf8().data() + std::string(buf);
}



/** \brief Check whether the cell can securily be used in a script.
 *
 * This signal is sent by the cell() function of snap_expr objects.
 * The plugin receiving the signal can check the table, row, and cell
 * names and mark that specific cell as secure. This will prevent the
 * script writer from accessing that specific cell.
 *
 * In case of the content plugin, this is used to protect all contents
 * in the secret table.
 *
 * The \p secure flag is used to mark the cell as secure. Simply call
 * the mark_as_secure() function to do so.
 *
 * \param[in] table  The table being accessed.
 * \param[in] accessible  Whether the cell is secure.
 *
 * \return This function returns true in case the signal needs to proceed.
 */
void epayment_stripe::on_table_is_accessible(QString const & table_name, server::accessible_flag_t & accessible)
{
    if(table_name == get_name(name_t::SNAP_NAME_EPAYMENT_STRIPE_TABLE))
    {
        // the stripe payment table includes all sorts of top-secret
        // identifiers so we do not want anyone to share such
        //
        accessible.mark_as_secure();
    }
}


/** \brief Define the test gateway.
 *
 * This function is a callback that is used by the system whenever it
 * wants to offer a specific gateway to process credit cards.
 *
 * \param[in,out] gateway_info  The structure that takes the various gateway
 *                              details.
 */
void epayment_stripe::gateway_features(epayment_creditcard::epayment_gateway_features_t & gateway_info)
{
    gateway_info.set_name("Stripe");
}


/** \brief Log an error sent back by Stripe.
 *
 * This function logs an error that was sent back by Stripe.
 *
 * It should only be called with responses that are not 200.
 *
 * \param[in] response  The HTTP response with the error.
 */
void epayment_stripe::log_error(http_client_server::http_response::pointer_t response, QtCassandra::QCassandraRow::pointer_t row)
{
    // make sure we are not called with an invalid response
    //
    if(response->get_response_code() < 300)
    {
        // 1XX and 2XX responses need to be managed differently
        throw epayment_stripe_exception_invalid_parameter("epayment_stripe::log_error() called with a valid response (i.e. error code is not 300 or more)");
    }

    int64_t const start_date(f_snap->get_start_date());

    // log the response in the database
    //
    QtCassandra::QCassandraValue value;
    value.setStringValue(QString::fromUtf8(response->get_response().c_str()));
    value.setTtl(365 * 86400); // keep for about 1 year
    row->cell(QString("%1::%2")
                        .arg(get_name(name_t::SNAP_SECURE_NAME_EPAYMENT_STRIPE_ERROR_REPLY))
                        .arg(start_date))
                ->setValue(value);

    // retrieve the error message
    //
    // since we already logged the whole JSON, we do not take the time
    // to check other parameters; we may do that for specific requests,
    // but not here
    //
    as2js::JSON::pointer_t error_json(new as2js::JSON);
    as2js::StringInput::pointer_t error_json_input(new as2js::StringInput(response->get_response()));
    as2js::JSON::JSONValue::pointer_t error_json_value(error_json->parse(error_json_input));
    if(!error_json_value)
    {
        SNAP_LOG_FATAL("JSON parser failed parsing error response");
        throw epayment_stripe_exception_invalid_error("JSON parser failed parsing error response");
    }
    as2js::JSON::JSONValue::object_t const & root_object(error_json_value->get_object());

    // "error"
    if(root_object.find("error") == root_object.end())
    {
        SNAP_LOG_ERROR("'error' missing in an error response");
        throw epayment_stripe_exception_invalid_error("'error' missing in error response");
    }
    as2js::JSON::JSONValue::object_t const & error_object(root_object.at("error")->get_object());

    // "message"
    if(error_object.find("message") == error_object.end())
    {
        SNAP_LOG_ERROR("'message' missing in an error response");
        throw epayment_stripe_exception_invalid_error("'message' missing in error response");
    }
    QString const message(QString::fromUtf8(error_object.at("message")->get_string().to_utf8().c_str()));

    //
    // other fields are:
    //
    //   "type"
    //   "param"
    //   "code"
    //
    // type can be:
    //
    //    api_connection_error
    //    api_error
    //    authentication_error
    //    card_error
    //    invalid_request_error
    //    rate_limit_error
    //
    // For more details see https://stripe.com/docs/api#errors
    //

    // since we have message, we can just save it in Cassandra as well
    // (and make it permanent in this case)
    //
    row->cell(get_name(name_t::SNAP_SECURE_NAME_EPAYMENT_STRIPE_LAST_ERROR_MESSAGE))->setValue(message);

    // TODO: somehow, add support so it is possible to translate error
    //       messages (Stripe does not provide such, unfortunately.)
    //       In which case we want to use the other fields to build
    //       the translated message.
    //

    // message the end user
    messages::messages::instance()->set_error(
        "Stripe Error",
        QString("An error occurred while processing your payment: %1").arg(message),
        "Inform the user of the error.",
        false
    );
}


/** \brief Test a credit card processing.
 *
 * This function is used to test the credit card processing mechanism.
 * The function just logs a message to let you know that it worked.
 *
 * \param[in,out] creditcard_info  The credit card information.
 * \param[in,out] save_info  The information from the editor form.
 *
 * \return true if the payment succeeded, false otherwise.
 */
bool epayment_stripe::process_creditcard(epayment_creditcard::epayment_creditcard_info_t & creditcard_info, editor::save_info_t & save_info)
{
    NOTUSED(save_info);

    //
    // WARNING: do not log the JSON responses as is, many include the
    //          expiration date and last 4 digits of the customer
    //          credit card and we do not want that liability on our
    //          systems!!!
    //

    users::users * users_plugin(users::users::instance());
    content::content * content_plugin(content::content::instance());
    epayment::epayment * epayment_plugin(epayment::epayment::instance());
    messages::messages * messages_plugin(messages::messages::instance());

    QtCassandra::QCassandraTable::pointer_t revision_table(content_plugin->get_revision_table());
    QtCassandra::QCassandraTable::pointer_t secret_table(content_plugin->get_secret_table());
    QtCassandra::QCassandraTable::pointer_t epayment_stripe_table(get_epayment_stripe_table());

    QtCassandra::QCassandraValue value;

    bool const debug(get_debug());
    int64_t const start_date(f_snap->get_start_date());
    QString const site_key(f_snap->get_site_key_with_slash());

    SNAP_LOG_INFO("epayment_stripe::process_creditcard() called")(debug ? " (debug turned on)" : "")(".");

#ifdef _DEBUG
// For debug purposes, you may check all the values with the following
std::cerr << "cc user_name [" << creditcard_info.get_user_name() << "]\n";
std::cerr << "cc number [" << creditcard_info.get_creditcard_number() << "]\n";
std::cerr << "cc security_code [" << creditcard_info.get_security_code() << "]\n";
std::cerr << "cc expiration_date_month [" << creditcard_info.get_expiration_date_month() << "]\n";
std::cerr << "cc expiration_date_year [" << creditcard_info.get_expiration_date_year() << "]\n";

std::cerr << "cc business_name [" << creditcard_info.get_billing_business_name() << "]\n";
std::cerr << "cc attention [" << creditcard_info.get_billing_attention() << "]\n";
std::cerr << "cc address1 [" << creditcard_info.get_billing_address1() << "]\n";
std::cerr << "cc address2 [" << creditcard_info.get_billing_address2() << "]\n";
std::cerr << "cc city [" << creditcard_info.get_billing_city() << "]\n";
std::cerr << "cc province [" << creditcard_info.get_billing_province() << "]\n";
std::cerr << "cc postal_code [" << creditcard_info.get_billing_postal_code() << "]\n";
std::cerr << "cc country [" << creditcard_info.get_billing_country() << "]\n";

std::cerr << "cc business_name [" << creditcard_info.get_delivery_business_name() << "]\n";
std::cerr << "cc attention [" << creditcard_info.get_delivery_attention() << "]\n";
std::cerr << "cc address1 [" << creditcard_info.get_delivery_address1() << "]\n";
std::cerr << "cc address2 [" << creditcard_info.get_delivery_address2() << "]\n";
std::cerr << "cc city [" << creditcard_info.get_delivery_city() << "]\n";
std::cerr << "cc province [" << creditcard_info.get_delivery_province() << "]\n";
std::cerr << "cc postal_code [" << creditcard_info.get_delivery_postal_code() << "]\n";
std::cerr << "cc country [" << creditcard_info.get_delivery_country() << "]\n";

std::cerr << "cc phone [" << creditcard_info.get_phone() << "]\n";
#endif

    content::path_info_t epayment_settings;
    epayment_settings.set_path("admin/settings/epayment/store");
    QtCassandra::QCassandraRow::pointer_t epayment_settings_row(revision_table->row(epayment_settings.get_revision_key()));

    QString const stripe_key(get_stripe_key(debug));
    if(stripe_key.isEmpty())
    {
        // we already generated an error if empty, leave now
        return false;
    }

    // in order to re-charge a card with Stripe, we need to create a
    // "customer" object on stripe; then we can use the customer
    // identifier in order to charge the card over and over again;
    // if no user is logged in, we just charge the card once...
    //

    // Get the invoice with its number and list of products
    //
    uint64_t invoice_number_ignore(0);
    content::path_info_t invoice_ipath;
    epayment::epayment_product_list plist;
    epayment_plugin->generate_invoice(invoice_ipath, invoice_number_ignore, plist);
    bool success(invoice_number_ignore != 0);
    if(!success)
    {
        messages_plugin->set_error(
            "Invoice Not Found",
            "Somehow we could be get an invoice to charge your account.",
            "No invoice... that can happen if your generate_invoice() callbacks all fail to generate a valid invoice.",
            false
        );
        return false;
    }

    // RAII to mark the invoice as failed unless we get a successful charge
    //
    class raii_invoice_status_t
    {
    public:
        raii_invoice_status_t(content::path_info_t & invoice_ipath)
            : f_epayment_plugin(epayment::epayment::instance())
            , f_invoice_ipath(invoice_ipath)
            , f_final_state(epayment::name_t::SNAP_NAME_EPAYMENT_INVOICE_STATUS_FAILED)
        {
            // mark invoice as being processed on creation
            //
            f_epayment_plugin->set_invoice_status(f_invoice_ipath, epayment::name_t::SNAP_NAME_EPAYMENT_INVOICE_STATUS_PROCESSING);
        }

        ~raii_invoice_status_t()
        {
            try
            {
                f_epayment_plugin->set_invoice_status(f_invoice_ipath, f_final_state);
            }
            catch(boost::exception_detail::clone_impl<boost::exception_detail::error_info_injector<boost::bad_function_call> > const &)
            {
            }
            catch(QtCassandra::QCassandraException const &)
            {
            }
        }

        void success()
        {
            f_final_state = epayment::name_t::SNAP_NAME_EPAYMENT_INVOICE_STATUS_PAID;
        }

    private:
        epayment::epayment *    f_epayment_plugin;
        content::path_info_t &  f_invoice_ipath;
        epayment::name_t        f_final_state;
    };
    raii_invoice_status_t safe_invoice_status(invoice_ipath);

    http_client_server::http_client http;
    //http.set_keep_alive(true) -- this is the default;

    // TODO: add a flag in the form so users may opt out of being registered
    //       on Stripe (in case your website does not offer subscriptions.)
    //
    auto const & user_info(users_plugin->get_user_info());
    QString const user_email(user_info.get_user_email());
    content::path_info_t user_ipath;
    user_ipath.set_path(user_info.get_user_path(false));
    QString const customer_key(user_ipath.get_key());

    bool create_customer_account(false);
    if(users_plugin->user_is_logged_in())
    {
        // by default we assume the user is okay with having his credit card
        // saved by Stripe
        //
        create_customer_account = revision_table
                        ->row(user_ipath.get_revision_key())
                        ->cell(epayment_creditcard::get_name(epayment_creditcard::name_t::SNAP_NAME_EPAYMENT_CREDITCARD_USER_ALLOWS_SAVING_TOKEN))
                        ->value().safeSignedCharValue(0, 1) != 0;
    }

    if(create_customer_account)
    {
        QtCassandra::QCassandraRow::pointer_t customer_row(epayment_stripe_table->row(customer_key));

        QString customer_stripe_key;

        bool update(false);
        if(customer_row->exists(get_name(name_t::SNAP_NAME_EPAYMENT_STRIPE_CREATED)))
        {
            // the user already exists in our database, so it has to exist
            // in Stripe database too...
            //
            customer_stripe_key = customer_row->cell(get_name(name_t::SNAP_NAME_EPAYMENT_STRIPE_CUSTOMER_KEY))
                                              ->value().stringValue();

            if(!customer_stripe_key.isEmpty())
            {
                // indeed, we already have a user, check to see whether Stripe
                // properly remembers too
                //
                http_client_server::http_request stripe_retrieve_customer_request;
                stripe_retrieve_customer_request.set_uri(QString("%1/%2")
                                .arg(get_name(name_t::SNAP_NAME_EPAYMENT_STRIPE_CUSTOMER_URI))
                                .arg(customer_stripe_key).toUtf8().data());
                stripe_retrieve_customer_request.set_header("Accept", "application/json");
                stripe_retrieve_customer_request.set_header("Stripe-Version", get_name(name_t::SNAP_NAME_EPAYMENT_STRIPE_VERSION)); // make sure our requests will work for the version we programmed them for
                stripe_retrieve_customer_request.set_basic_auth(stripe_key.toUtf8().data(), "");
                //stripe_create_request.set_header("Accept-Language", "en_US"); -- at this time, stripe's API is 100% English
                //stripe_create_request.set_header("Content-Type", "application/x-www-form-urlencoded"); -- no need since there no post data in this case
                // no POST parameters in this case

                http_client_server::http_response::pointer_t retrieve_response(http.send_request(stripe_retrieve_customer_request));

                // log the header, that has no customer data per se
                value.setStringValue(QString::fromUtf8(retrieve_response->get_original_header().c_str()));
                value.setTtl(365 * 86400);  // keep for about 1 year
                customer_row->cell(QString("%1::%2").arg(get_name(name_t::SNAP_SECURE_NAME_EPAYMENT_STRIPE_RETRIEVE_CUSTOMER_HEADER)).arg(start_date))
                            ->setValue(value);

                // NO DIRECT LOGGING OF RESPONSE, SEE WARNING AT START OF FUNCTION
                //customer_row->cell(...)->setValue(QString::fromUtf8(retrieve_response->get_response().c_str()));
    
                // Here Stripe makes it simple, if anything fails, including a
                // payment, then the response code is not 200
                //
                if(retrieve_response->get_response_code() == 200)
                {
                    // looks pretty good, check the actual answer...
                    as2js::JSON::pointer_t retrieve_json(new as2js::JSON);
                    as2js::StringInput::pointer_t retrieve_json_input(new as2js::StringInput(retrieve_response->get_response()));
                    as2js::JSON::JSONValue::pointer_t retrieve_json_value(retrieve_json->parse(retrieve_json_input));
                    if(!retrieve_json_value)
                    {
                        // TBD: should we not just delete our data and start over?
                        SNAP_LOG_FATAL("JSON parser failed parsing 'execute' response");
                        throw epayment_stripe_exception_io_error("JSON parser failed parsing 'execute' response");
                    }
                    as2js::JSON::JSONValue::object_t const & id_object(retrieve_json_value->get_object());
        
                    // ID
                    // verify that the customer identifier corresponds to what we expect
                    if(id_object.find("id") == id_object.end())
                    {
                        // TBD: should we not just delete our data and start over?
                        SNAP_LOG_FATAL("'id' missing in 'retrieve customer' response");
                        throw epayment_stripe_exception_io_error("'id' missing in 'execute' response");
                    }
                    QString const reply_id(QString::fromUtf8(id_object.at("id")->get_string().to_utf8().c_str()));
                    if(reply_id != customer_stripe_key)
                    {
                        // TBD: should we not just delete our data and start over?
                        SNAP_LOG_FATAL("'id' in 'retrieve customer' response is not the same as the input 'id'");
                        throw epayment_stripe_exception_io_error("'id' in 'retrieve customer' response is not the same as the invoice 'id'");
                    }

                    // TBD: log this JSON? We are going to have another
                    //      copy below so I am thinking to log that one
                    //      only...
        
                    // the update is nearly the same as the create so we
                    // reuse most of the code below
                    //
                    update = true;

                    SNAP_LOG_INFO("epayment_stripe::process_creditcard() doing an update.");
                }
                else if(retrieve_response->get_response_code() == 404)
                {
                    // somehow Stripe says that customer does not exist
                    // so we will re-create it below
                    //
                    SNAP_LOG_WARNING("epayment_stripe::process_creditcard() existing user \"")(user_email)("\" not present at Stripe, create now.");
                }
                else
                {
                    // all other errors are considered fatal at this point
                    //
                    log_error(retrieve_response, customer_row);
                    return false;
                }
            }
        }

        {
            // create or update the user
            //
            http_client_server::http_request stripe_create_request;

            // the URI changes slightly in case we are updating
            if(update)
            {
                stripe_create_request.set_uri(QString("%1/%2")
                        .arg(get_name(name_t::SNAP_NAME_EPAYMENT_STRIPE_CUSTOMER_URI))
                        .arg(customer_stripe_key).toUtf8().data());
            }
            else
            {
                stripe_create_request.set_uri(get_name(name_t::SNAP_NAME_EPAYMENT_STRIPE_CUSTOMER_URI));
            }

            // the set_uri() does the set_path() and set_port() as expected
            //stripe_create_request.set_path("...");
            //stripe_create_request.set_port(443); // https
            stripe_create_request.set_header("Accept", "application/json");
            stripe_create_request.set_header("Stripe-Version", get_name(name_t::SNAP_NAME_EPAYMENT_STRIPE_VERSION)); // make sure our requests will work for the version we programmed them for
            //stripe_create_request.set_header("Accept-Language", "en_US"); -- at this time, stripe's API is 100% English
            //stripe_create_request.set_header("Content-Type", "application/x-www-form-urlencoded"); -- automatic with set_post() calls
            stripe_create_request.set_basic_auth(stripe_key.toUtf8().data(), "");

            // POST variables

            // Basic variables
            //stripe_create_request.set_post("account_balance", 0); -- at this point we never have a balance even before we ever created the user
            //stripe_create_request.set_post("coupon", 0); -- we manage our own coupons
            //stripe_create_request.set_post("default_source", 0); -- we redefine the source
            stripe_create_request.set_post("description", QString("Customer %1 created on %2")
                                .arg(user_email)
                                .arg(f_snap->date_to_string(start_date, snap_child::date_format_t::DATE_FORMAT_SHORT))
                                .toUtf8().data());
            stripe_create_request.set_post("email", user_email.toUtf8().data());
            stripe_create_request.set_post("metadata[user_id]", customer_key.toUtf8().data()); // can make it easier to find the customer this way
            //if(!update)
            //{
            //    stripe_create_request.set_post("plan", ...); -- we handle our own plans
            //    stripe_create_request.set_post("quantity", ...); -- plan quantity, ignore
            //    stripe_create_request.set_post("tax_percent", ...); -- we manipulate taxes ourselves
            //    stripe_create_request.set_post("trial_end", ...); -- plan start date, ignore
            //}

            // Shipping details
            QString const delivery_address1(creditcard_info.get_delivery_address1().toUtf8().data());
            if(!delivery_address1.isEmpty())
            {
                stripe_create_request.set_post("shipping[address][line1]", delivery_address1.toUtf8().data());
                stripe_create_request.set_post("shipping[address][city]", creditcard_info.get_delivery_city().toUtf8().data());
                stripe_create_request.set_post("shipping[address][country]", creditcard_info.get_delivery_country().toUtf8().data());
                stripe_create_request.set_post("shipping[address][line2]", creditcard_info.get_delivery_address2().toUtf8().data());
                stripe_create_request.set_post("shipping[address][postal_code]", creditcard_info.get_delivery_postal_code().toUtf8().data());
                stripe_create_request.set_post("shipping[address][state]", creditcard_info.get_delivery_province().toUtf8().data());

                // name is a REQUIRED field, so we have got to have one...
                QString name(creditcard_info.get_delivery_business_name());
                if(name.isEmpty())
                {
                    name = creditcard_info.get_delivery_attention();
                    if(name.isEmpty())
                    {
                        // this one is REQUIRED in our form so we will always
                        // have something in the end
                        //
                        name = creditcard_info.get_user_name();
                    }
                }
                stripe_create_request.set_post("shipping[name]", name.toUtf8().data());

                stripe_create_request.set_post("shipping[phone]", creditcard_info.get_phone().toUtf8().data());
            }

            // Source (i.e. user's credit card information)
            stripe_create_request.set_post("source[object]", "card");
            stripe_create_request.set_post("source[exp_month]", creditcard_info.get_expiration_date_month().toUtf8().data());
            stripe_create_request.set_post("source[exp_year]", creditcard_info.get_expiration_date_year().toUtf8().data());
            stripe_create_request.set_post("source[number]", creditcard_info.get_creditcard_number().toUtf8().data());
            stripe_create_request.set_post("source[address_city]", creditcard_info.get_billing_city().toUtf8().data());
            stripe_create_request.set_post("source[address_country]", creditcard_info.get_billing_country().toUtf8().data());
            stripe_create_request.set_post("source[address_line1]", creditcard_info.get_billing_address1().toUtf8().data());
            stripe_create_request.set_post("source[address_line2]", creditcard_info.get_billing_address2().toUtf8().data());
            stripe_create_request.set_post("source[address_state]", creditcard_info.get_billing_province().toUtf8().data());
            stripe_create_request.set_post("source[address_zip]", creditcard_info.get_billing_postal_code().toUtf8().data());
            //stripe_create_request.set_post("source[currency]", "usd"); -- only for debit cards, managed account, and only "usd" so... not useful
            stripe_create_request.set_post("source[cvc]", creditcard_info.get_security_code().toUtf8().data());
            //stripe_create_request.set_post("source[default_for_currency]", "true"); -- only for debit cards, managed account, and only "usd" so... not useful
            //stripe_create_request.set_post("source[metadata]", ...); -- we have our own database for that
            stripe_create_request.set_post("source[name]", creditcard_info.get_user_name().toUtf8().data());

            http_client_server::http_response::pointer_t create_response(http.send_request(stripe_create_request));

            // we can save the reply header as is
            //
            value.setStringValue(QString::fromUtf8(create_response->get_original_header().c_str()));
            value.setTtl(365 * 86400); // keep for about 1 year
            customer_row->cell(QString("%1::%2")
                                .arg(get_name(update ? name_t::SNAP_SECURE_NAME_EPAYMENT_STRIPE_UPDATE_CUSTOMER_HEADER
                                                     : name_t::SNAP_SECURE_NAME_EPAYMENT_STRIPE_CREATE_CUSTOMER_HEADER))
                                .arg(start_date))
                        ->setValue(value);

            // NO DIRECT LOGGING OF RESPONSE, SEE WARNING AT START OF FUNCTION
            //customer_row->cell(...)->setValue(QString::fromUtf8(create_response->get_response().c_str()));

            // Stripe makes it simple, if anything fails, including a
            // payment, then the response code is not 200
            //
            if(create_response->get_response_code() != 200)
            {
                // in this case we can save the response as errors should never
                // include sensitive data about the customer (TBD!)
                //
                // errors DO happen if the card is not valid (i.e. the customer
                // info, including the card details are checked in this step),
                // even though no charge gets triggered on creation or update!
                //

                log_error(create_response, customer_row);
                return false;
            }

            // looks pretty good, check the actual answer...
            as2js::JSON::pointer_t create_json(new as2js::JSON);
            as2js::StringInput::pointer_t create_json_input(new as2js::StringInput(create_response->get_response()));
            as2js::JSON::JSONValue::pointer_t create_json_value(create_json->parse(create_json_input));
            if(!create_json_value)
            {
                // TBD: should we not just delete our data and start over?
                SNAP_LOG_FATAL("JSON parser failed parsing 'create/update customer' response");
                throw epayment_stripe_exception_io_error("JSON parser failed parsing 'create/update customer' response");
            }
            as2js::JSON::JSONValue::object_t root_object(create_json_value->get_object());

            // ID
            if(root_object.find("id") == root_object.end())
            {
                SNAP_LOG_FATAL("'id' missing in 'create/update customer' response");
                throw epayment_stripe_exception_io_error("'id' missing in 'create/update customer' response");
            }
            QString const customer_id(QString::fromUtf8(root_object.at("id")->get_string().to_utf8().c_str()));
            if(update)
            {
                // the customer stripe key cannot change on an update
                //
                if(customer_id != customer_stripe_key)
                {
                    // TBD: should we not just delete our data and start over?
                    SNAP_LOG_FATAL("'id' in 'create/update customer' response is not the same as the input 'id'");
                    throw epayment_stripe_exception_io_error("'id' in 'create/update customer' response is not the same as the invoice 'id'");
                }
            }
            else
            {
                // this is the customer key on stripe
                //
                customer_stripe_key = customer_id;

                customer_row->cell(get_name(name_t::SNAP_NAME_EPAYMENT_STRIPE_CREATED))->setValue(start_date);
                customer_row->cell(get_name(name_t::SNAP_NAME_EPAYMENT_STRIPE_CUSTOMER_KEY))->setValue(customer_stripe_key);
            }

            // now we want to save the JSON, only it includes some personal
            // data about the user:
            //
            //    brand
            //    dynamic_last4
            //    exp_month
            //    exp_year
            //    fingerprint
            //    last4
            //
            if(root_object.find("sources") != root_object.end())
            {
                as2js::JSON::JSONValue::object_t sources_object(root_object.at("sources")->get_object());
                if(sources_object.find("data") != sources_object.end())
                {
                    as2js::JSON::JSONValue::array_t data_array(sources_object.at("data")->get_array());
                    for(size_t idx(0); idx < data_array.size(); ++idx)
                    {
                        // remove data which is too "personal" for our database
                        data_array[idx]->set_member("brand", as2js::JSON::JSONValue::pointer_t());
                        data_array[idx]->set_member("dynamic_last4", as2js::JSON::JSONValue::pointer_t());
                        data_array[idx]->set_member("exp_month", as2js::JSON::JSONValue::pointer_t());
                        data_array[idx]->set_member("exp_year", as2js::JSON::JSONValue::pointer_t());
                        data_array[idx]->set_member("fingerprint", as2js::JSON::JSONValue::pointer_t());
                        data_array[idx]->set_member("last4", as2js::JSON::JSONValue::pointer_t());
                    }
                }
            }

            customer_row->cell(get_name(name_t::SNAP_SECURE_NAME_EPAYMENT_STRIPE_CUSTOMER_INFO))->setValue(QString::fromUtf8(create_json_value->to_string().to_utf8().c_str()));

            if(update)
            {
                SNAP_LOG_INFO("epayment_stripe::process_creditcard() update successful.");
            }
            else
            {
                SNAP_LOG_INFO("epayment_stripe::process_creditcard() new user created successfully.");
            }
        }

        {
            // Now actually charge the customer card
            //
            QtCassandra::QCassandraRow::pointer_t secret_row(secret_table->row(invoice_ipath.get_key()));
            QtCassandra::QCassandraRow::pointer_t revision_row(revision_table->row(invoice_ipath.get_revision_key()));

            secret_row->cell(get_name(name_t::SNAP_NAME_EPAYMENT_STRIPE_CREATED))->setValue(start_date);

            // create a charge now
            //
            http_client_server::http_request stripe_charge_request;

            // the URI changes slightly in case we are updating
            stripe_charge_request.set_uri(get_name(name_t::SNAP_NAME_EPAYMENT_STRIPE_CHARGE_URI));

            // the set_uri() does the set_path() and set_port() as expected
            //stripe_create_request.set_path("...");
            //stripe_create_request.set_port(443); // https
            stripe_charge_request.set_header("Accept", "application/json");
            stripe_charge_request.set_header("Stripe-Version", get_name(name_t::SNAP_NAME_EPAYMENT_STRIPE_VERSION)); // make sure our requests will work for the version we programmed them for
            //stripe_create_request.set_header("Accept-Language", "en_US"); -- at this time, stripe's API is 100% English
            //stripe_create_request.set_header("Content-Type", "application/x-www-form-urlencoded"); -- automatic with set_post() calls
            stripe_charge_request.set_basic_auth(stripe_key.toUtf8().data(), "");

            // POST variables

            // Basic variables
            double const grand_total(revision_row->cell(epayment::get_name(epayment::name_t::SNAP_NAME_EPAYMENT_GRAND_TOTAL))->value().safeDoubleValue());
            stripe_charge_request.set_post("amount", QString("%1").arg(static_cast<uint64_t>(grand_total * 100.0)).toUtf8().data()); // amount in cents

            // once we have time to add proper support for various currencies:
            // https://support.stripe.com/questions/which-currencies-does-stripe-support
            //
            stripe_charge_request.set_post("currency", "usd"); // force USD for now
            stripe_charge_request.set_post("capture", "true"); // make sure we always capture

            // capturing a fee on that charge?
            //stripe_charge_request.set_post("application_fee", 0); -- useful for systems like Order Made but not yet implemented
            //stripe_charge_request.set_post("destination", ...); -- useful for systems like Order Made but not yet implemented

            // pass the SNAP_NAME_CONTENT_TITLE as description, it often is
            // the invoice number, but could be more descriptive...
            //
            QString const invoice_description(revision_row->cell(content::get_name(content::name_t::SNAP_NAME_CONTENT_TITLE))->value().stringValue());
            stripe_charge_request.set_post("description", snap_dom::remove_tags(invoice_description).toUtf8().data());

            stripe_charge_request.set_post("metadata[user_id]", customer_key.toUtf8().data()); // can make it easier to find the customer this way

            // We manage emails and setting the receipt_email would generate
            // a double email so we do not do it!
            //
            //stripe_charge_request.set_post("receipt_email", user_email);

            // If available, it is part of the customer's account
            //
            //stripe_charge_request.set_post("shipping[...]", ...);

            // Customer
            stripe_charge_request.set_post("customer", customer_stripe_key.toUtf8().data());

            // Source
            //stripe_charge_request.set_post("source[...]", ...); -- the customer information is enough, they will use the default source though
            //                                                       once we support selecting any source, this will change with the source 'id'

            // Description appearing on the credit card bank statements
            //
            QString const store_name(epayment_settings_row->cell(epayment::get_name(epayment::name_t::SNAP_NAME_EPAYMENT_STORE_NAME))->value().stringValue());
            QString const invoice_number(revision_row->cell(epayment::get_name(epayment::name_t::SNAP_NAME_EPAYMENT_INVOICE_NUMBER))->value().stringValue());
            QString const statement(QString("%1 #%2").arg(store_name).arg(invoice_number));
            stripe_charge_request.set_post("statement_descriptor", statement.toUtf8().data());

            http_client_server::http_response::pointer_t charge_response(http.send_request(stripe_charge_request));

            // we can save the reply header as is
            //
            value.setStringValue(QString::fromUtf8(charge_response->get_original_header().c_str()));
            value.setTtl(365 * 86400); // keep for about 1 year
            secret_row->cell(QString("%1::%2")
                                .arg(get_name(name_t::SNAP_SECURE_NAME_EPAYMENT_STRIPE_CHARGE_HEADER))
                                .arg(start_date))
                        ->setValue(value);

            // NO DIRECT LOGGING OF RESPONSE, SEE WARNING AT START OF FUNCTION
            //secret_row->cell(...)->setValue(QString::fromUtf8(charge_response->get_response().c_str()));

            // Stripe makes it simple, if anything fails, including a
            // payment, then the response code is not 200
            //
            if(charge_response->get_response_code() != 200)
            {
                // in this case we can save the response as errors should never
                // include sensitive data about the customer (TBD!)
                //
                // errors DO happen if the card is not valid (i.e. the customer
                // info, including the card details are checked in this step),
                // even though no charge gets triggered on creation or update!
                //

                log_error(charge_response, secret_row);
                return false;
            }

            // looks pretty good, check the actual answer...
            as2js::JSON::pointer_t charge_json(new as2js::JSON);
            as2js::StringInput::pointer_t charge_json_input(new as2js::StringInput(charge_response->get_response()));
            as2js::JSON::JSONValue::pointer_t charge_json_value(charge_json->parse(charge_json_input));
            if(!charge_json_value)
            {
                // TBD: should we not just delete our data and start over?
                SNAP_LOG_FATAL("JSON parser failed parsing 'create/update customer' response");
                throw epayment_stripe_exception_io_error("JSON parser failed parsing 'create/update customer' response");
            }
            as2js::JSON::JSONValue::object_t root_object(charge_json_value->get_object());

            // ID
            if(root_object.find("id") == root_object.end())
            {
                SNAP_LOG_FATAL("'id' missing in 'charge' response");
                throw epayment_stripe_exception_io_error("'id' missing in 'charge' response");
            }
            QString const charge_id(QString::fromUtf8(root_object.at("id")->get_string().to_utf8().c_str()));
            secret_row->cell(get_name(name_t::SNAP_SECURE_NAME_EPAYMENT_STRIPE_CHARGE_KEY))->setValue(charge_id);

            // to re-charge the same customer we need a link back to that customer
            secret_row->cell(get_name(name_t::SNAP_SECURE_NAME_EPAYMENT_STRIPE_USER_KEY))->setValue(customer_key);

            // now we want to save the JSON, only it includes some personal
            // data about the user:
            //
            //    brand
            //    dynamic_last4
            //    exp_month
            //    exp_year
            //    fingerprint
            //    last4
            //
            if(root_object.find("sources") != root_object.end())
            {
                as2js::JSON::JSONValue::pointer_t source(root_object.at("source"));
                source->set_member("brand", as2js::JSON::JSONValue::pointer_t());
                source->set_member("dynamic_last4", as2js::JSON::JSONValue::pointer_t());
                source->set_member("exp_month", as2js::JSON::JSONValue::pointer_t());
                source->set_member("exp_year", as2js::JSON::JSONValue::pointer_t());
                source->set_member("fingerprint", as2js::JSON::JSONValue::pointer_t());
                source->set_member("last4", as2js::JSON::JSONValue::pointer_t());
            }

            secret_row->cell(get_name(name_t::SNAP_SECURE_NAME_EPAYMENT_STRIPE_CHARGE_INFO))
                      ->setValue(QString::fromUtf8(charge_json_value->to_string().to_utf8().c_str()));

            SNAP_LOG_INFO("epayment_stripe::process_creditcard() subscription charge succeeded.");

            // this was a subscription, let epayment_creditcard know
            //
            creditcard_info.set_subscription(true);
        }
    }
    else
    {
        // single charge
        //
        // Charge the customer card directly
        // We do not create a customer object in this case
        //
        QtCassandra::QCassandraRow::pointer_t secret_row(secret_table->row(invoice_ipath.get_key()));
        QtCassandra::QCassandraRow::pointer_t revision_row(revision_table->row(invoice_ipath.get_revision_key()));

        secret_row->cell(get_name(name_t::SNAP_NAME_EPAYMENT_STRIPE_CREATED))
                  ->setValue(start_date);

        // create a charge now
        //
        http_client_server::http_request stripe_charge_request;

        // the URI changes slightly in case we are updating
        stripe_charge_request.set_uri(get_name(name_t::SNAP_NAME_EPAYMENT_STRIPE_CHARGE_URI));

        // the set_uri() does the set_path() and set_port() as expected
        //stripe_create_request.set_path("...");
        //stripe_create_request.set_port(443); // https
        stripe_charge_request.set_header("Accept", "application/json");
        stripe_charge_request.set_header("Stripe-Version", get_name(name_t::SNAP_NAME_EPAYMENT_STRIPE_VERSION)); // make sure our requests will work for the version we programmed them for
        //stripe_create_request.set_header("Accept-Language", "en_US"); -- at this time, stripe's API is 100% English
        //stripe_create_request.set_header("Content-Type", "application/x-www-form-urlencoded"); -- automatic with set_post() calls
        stripe_charge_request.set_basic_auth(stripe_key.toUtf8().data(), "");

        // POST variables

        // Amount in cents
        //
        double const grand_total(revision_row->cell(epayment::get_name(epayment::name_t::SNAP_NAME_EPAYMENT_GRAND_TOTAL))->value().safeDoubleValue());
        stripe_charge_request.set_post("amount", QString("%1").arg(static_cast<uint64_t>(grand_total * 100.0)).toUtf8().data());

        // once we have time to add proper support for various currencies:
        // https://support.stripe.com/questions/which-currencies-does-stripe-support
        //
        stripe_charge_request.set_post("currency", "usd"); // force USD for now
        stripe_charge_request.set_post("capture", "true"); // make sure we always capture

        // capturing a fee on that charge?
        //stripe_charge_request.set_post("application_fee", 0); -- useful for systems like Order Made but not yet implemented
        //stripe_charge_request.set_post("destination", ...); -- useful for systems like Order Made but not yet implemented

        // pass the SNAP_NAME_CONTENT_TITLE as description, it often is
        // the invoice number, but could be more descriptive...
        //
        QString const invoice_description(revision_row->cell(content::get_name(content::name_t::SNAP_NAME_CONTENT_TITLE))->value().stringValue());
        stripe_charge_request.set_post("description", snap_dom::remove_tags(invoice_description).toUtf8().data());

        if(users_plugin->user_is_logged_in())
        {
            // put the customer key in the metadata for later reference
            // (in case the user did not want to be tracked too closely
            // it may still be logged in when doing the charge.)
            //
            stripe_charge_request.set_post("metadata[user_id]", customer_key.toUtf8().data()); // can make it easier to find the customer this way
        }

        // We manage emails and setting the receipt_email would generate
        // a double email so we do not do it!
        //
        //stripe_charge_request.set_post("receipt_email", user_email);

        // Add the shipping info if available
        // (delivery_address1 must be defined
        //
        QString const delivery_address1(creditcard_info.get_delivery_address1().toUtf8().data());
        if(!delivery_address1.isEmpty())
        {
            stripe_charge_request.set_post("shipping[address][city]", creditcard_info.get_delivery_city().toUtf8().data());
            stripe_charge_request.set_post("shipping[address][country]", creditcard_info.get_delivery_country().toUtf8().data());
            stripe_charge_request.set_post("shipping[address][line1]", delivery_address1.toUtf8().data());
            stripe_charge_request.set_post("shipping[address][line2]", creditcard_info.get_delivery_address2().toUtf8().data());
            stripe_charge_request.set_post("shipping[address][postal_code]", creditcard_info.get_delivery_postal_code().toUtf8().data());
            stripe_charge_request.set_post("shipping[address][state]", creditcard_info.get_delivery_province().toUtf8().data());

            // TODO: once we get that info...
            //stripe_charge_request.set_post("shipping[carrier]", ...);

            // name is a REQUIRED field, so we have got to have one...
            QString name(creditcard_info.get_delivery_business_name());
            if(name.isEmpty())
            {
                name = creditcard_info.get_delivery_attention();
                if(name.isEmpty())
                {
                    // this one is REQUIRED in our form so we will always
                    // have something in the end
                    //
                    name = creditcard_info.get_user_name();
                }
            }
            stripe_charge_request.set_post("shipping[name]", name.toUtf8().data());

            stripe_charge_request.set_post("shipping[phone]", creditcard_info.get_phone().toUtf8().data());
        }

        // Customer
        //stripe_charge_request.set_post("customer", ...); -- no customer in this case

        // Source (i.e. user's credit card information)
        stripe_charge_request.set_post("source[exp_month]", creditcard_info.get_expiration_date_month().toUtf8().data());
        stripe_charge_request.set_post("source[exp_year]", creditcard_info.get_expiration_date_year().toUtf8().data());
        stripe_charge_request.set_post("source[number]", creditcard_info.get_creditcard_number().toUtf8().data());
        stripe_charge_request.set_post("source[object]", "card");
        stripe_charge_request.set_post("source[cvc]", creditcard_info.get_security_code().toUtf8().data());
        stripe_charge_request.set_post("source[address_city]", creditcard_info.get_billing_city().toUtf8().data());
        stripe_charge_request.set_post("source[address_country]", creditcard_info.get_billing_country().toUtf8().data());
        stripe_charge_request.set_post("source[address_line1]", creditcard_info.get_billing_address1().toUtf8().data());
        stripe_charge_request.set_post("source[address_line2]", creditcard_info.get_billing_address2().toUtf8().data());
        stripe_charge_request.set_post("source[name]", creditcard_info.get_user_name().toUtf8().data());
        stripe_charge_request.set_post("source[address_state]", creditcard_info.get_billing_province().toUtf8().data());
        stripe_charge_request.set_post("source[address_zip]", creditcard_info.get_billing_postal_code().toUtf8().data());

        // Description appearing on the credit card bank statements
        //
        QString const store_name(epayment_settings_row->cell(epayment::get_name(epayment::name_t::SNAP_NAME_EPAYMENT_STORE_NAME))->value().stringValue());
        QString const invoice_number(revision_row->cell(epayment::get_name(epayment::name_t::SNAP_NAME_EPAYMENT_INVOICE_NUMBER))->value().stringValue());
        QString const statement(QString("%1 #%2").arg(store_name).arg(invoice_number));
        stripe_charge_request.set_post("statement_descriptor", statement.toUtf8().data());

        http_client_server::http_response::pointer_t charge_response(http.send_request(stripe_charge_request));

        // we can save the reply header as is
        //
        value.setStringValue(QString::fromUtf8(charge_response->get_original_header().c_str()));
        value.setTtl(365 * 86400); // keep for about 1 year
        secret_row->cell(get_name(name_t::SNAP_SECURE_NAME_EPAYMENT_STRIPE_CHARGE_HEADER))
                  ->setValue(value);

        // NO DIRECT LOGGING OF RESPONSE, SEE WARNING AT START OF FUNCTION
        //secret_row->cell(...)->setValue(QString::fromUtf8(charge_response->get_response().c_str()));

        // Stripe makes it simple, if anything fails, including a
        // payment, then the response code is not 200
        //
        if(charge_response->get_response_code() != 200)
        {
            // in this case we can save the response as errors should never
            // include sensitive data about the customer (TBD!)
            //
            // errors DO happen if the card is not valid (i.e. the customer
            // info, including the card details are checked in this step),
            // even though no charge gets triggered on creation or update!
            //

            log_error(charge_response, secret_row);
            return false;
        }

        // looks pretty good, check the actual answer...
        as2js::JSON::pointer_t charge_json(new as2js::JSON);
        as2js::StringInput::pointer_t charge_json_input(new as2js::StringInput(charge_response->get_response()));
        as2js::JSON::JSONValue::pointer_t charge_json_value(charge_json->parse(charge_json_input));
        if(!charge_json_value)
        {
            // TBD: should we not just delete our data and start over?
            SNAP_LOG_FATAL("JSON parser failed parsing 'create/update customer' response");
            throw epayment_stripe_exception_io_error("JSON parser failed parsing 'create/update customer' response");
        }
        as2js::JSON::JSONValue::object_t root_object(charge_json_value->get_object());

        // ID
        if(root_object.find("id") == root_object.end())
        {
            SNAP_LOG_FATAL("'id' missing in 'charge' response");
            throw epayment_stripe_exception_io_error("'id' missing in 'charge' response");
        }
        QString const charge_id(QString::fromUtf8(root_object.at("id")->get_string().to_utf8().c_str()));
        secret_row->cell(get_name(name_t::SNAP_SECURE_NAME_EPAYMENT_STRIPE_CHARGE_KEY))
                  ->setValue(charge_id);

        // now we want to save the JSON, only it includes some personal
        // data about the user:
        //
        //    brand
        //    dynamic_last4
        //    exp_month
        //    exp_year
        //    fingerprint
        //    last4
        //
        if(root_object.find("sources") != root_object.end())
        {
            as2js::JSON::JSONValue::pointer_t source(root_object.at("source"));
            source->set_member("brand", as2js::JSON::JSONValue::pointer_t());
            source->set_member("dynamic_last4", as2js::JSON::JSONValue::pointer_t());
            source->set_member("exp_month", as2js::JSON::JSONValue::pointer_t());
            source->set_member("exp_year", as2js::JSON::JSONValue::pointer_t());
            source->set_member("fingerprint", as2js::JSON::JSONValue::pointer_t());
            source->set_member("last4", as2js::JSON::JSONValue::pointer_t());
        }

        secret_row->cell(get_name(name_t::SNAP_SECURE_NAME_EPAYMENT_STRIPE_CHARGE_INFO))
                  ->setValue(QString::fromUtf8(charge_json_value->to_string().to_utf8().c_str()));

        SNAP_LOG_INFO("epayment_stripe::process_creditcard() simple charge succeeded.");
    }

    safe_invoice_status.success();

    return true;
}



// Stripe documentation at time of writing
//   https://stripe.com/docs/api

/*
 * Example using curl to send a processing request
 *
 * curl https://api.stripe.com/v1/charges 
 *     -u sk_test_BQokikJOvBiI2HlWgH4olfQ2: \
 *     -d amount=400 \
 *     -d currency=usd \
 *     -d "description=Charge for test@example.com" \
 *     -d "source[object]=card" \
 *     -d "source[number]=4242424242424242" \
 *     -d "source[exp_month]=12" \
 *     -d "source[exp_year]=2017" \
 *     -d "source[cvc]=123"
 *
 * There is the JSON answer from that request:
 *
 * {
 *   "id": "ch_17XnAd2eZvKYlo2CdClOKnBH",
 *   "object": "charge",
 *   "amount": 400,
 *   "amount_refunded": 0,
 *   "application_fee": null,
 *   "balance_transaction": "txn_17XnAd2eZvKYlo2C5zgluyFD",
 *   "captured": true,
 *   "created": 1453877571,
 *   "currency": "usd",
 *   "customer": null,
 *   "description": "Charge for test@example.com",
 *   "destination": null,
 *   "dispute": null,
 *   "failure_code": null,
 *   "failure_message": null,
 *   "fraud_details": {},
 *   "invoice": null,
 *   "livemode": false,
 *   "metadata": {},
 *   "order": null,
 *   "paid": true,
 *   "receipt_email": null,
 *   "receipt_number": null,
 *   "refunded": false,
 *   "refunds": {
 *     "object": "list",
 *     "data": [],
 *     "has_more": false,
 *     "total_count": 0,
 *     "url": "/v1/charges/ch_17XnAd2eZvKYlo2CdClOKnBH/refunds"
 *   },
 *   "shipping": null,
 *   "source": {
 *     "id": "card_17XnAd2eZvKYlo2CfuBraJUm",
 *     "object": "card",
 *     "address_city": null,
 *     "address_country": null,
 *     "address_line1": null,
 *     "address_line1_check": null,
 *     "address_line2": null,
 *     "address_state": null,
 *     "address_zip": null,
 *     "address_zip_check": null,
 *     "brand": "Visa",
 *     "country": "US",
 *     "customer": null,
 *     "cvc_check": "pass",
 *     "dynamic_last4": null,
 *     "exp_month": 12,
 *     "exp_year": 2017,
 *     "fingerprint": "Xt5EWLLDS7FJjR1c",
 *     "funding": "credit",
 *     "last4": "4242",
 *     "metadata": {},
 *     "name": null,
 *     "tokenization_method": null
 *   },
 *   "source_transfer": null,
 *   "statement_descriptor": null,
 *   "status": "succeeded"
 * }
 */


SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
