/** @preserve
 * Name: epayment
 * Version: 0.0.1.35
 * Browsers: all
 * Depends: editor (>= 0.0.3.262)
 * Copyright: Copyright 2013-2017 (c) Made to Order Software Corporation  All rights reverved.
 * License: GPL 2.0
 */

//
// Inline "command line" parameters for the Google Closure Compiler
// https://developers.google.com/closure/compiler/docs/js-for-compiler
//
// See output of:
//    java -jar .../google-js-compiler/compiler.jar --help
//
// ==ClosureCompiler==
// @compilation_level ADVANCED_OPTIMIZATIONS
// @externs $CLOSURE_COMPILER/contrib/externs/jquery-1.9.js
// @externs plugins/output/externs/jquery-extensions.js
// @js plugins/output/output.js
// @js plugins/output/popup.js
// @js plugins/server_access/server-access.js
// @js plugins/listener/listener.js
// @js plugins/editor/editor.js
// ==/ClosureCompiler==
//

/*
 * JSLint options are defined online at:
 *    http://www.jshint.com/docs/options/
 */
/*jslint nomen: true, todo: true, devel: true */
/*global snapwebsites: false, jQuery: false, FileReader: true, Blob: true */



/** \file
 * \brief The Snap! e-Payment organization.
 *
 * This file defines a the e-Payment facilities offered in JavaScript
 * classes.
 *
 * This includes ways to add electronic and not so electronic payment
 * facilities to a website.
 *
 * The current e-Payment environment looks like this:
 *
 * \code
 *  +-----------------------+  Inherit
 *  |                       |<-------------+
 *  | ServerAccessCallbacks |              |
 *  | (cannot instantiate)  |       +-----------------------------+
 *  +-----------------------+       |                             |
 *       ^                          | ePaymentFacilityBase        |
 *       |         +--------------->| (cannot instantiate)        |
 *       |         |                +-----------------------------+
 *       |         |                      ^
 *       |         | Reference            | Inherit
 *       |         |                      |
 *  +----+---------+----+           +-----+--------------------+
 *  |                   |<----------+                          |
 *  | ePayment          | Register  | ePaymentFacility...      |
 *  | (final)           | (1,n)     |                          |
 *  +-----+-------------+           +--------------------------+
 *    ^   |
 *    |   | Create (1,1)
 *    |   |      +-------------------+
 *    |   +----->|                   +--... (widgets, etc.)
 *    |          | EditorForm        |
 *    |          |                   |
 *    |          +-------------------+
 *    |
 *    | Create (1,1)
 *  +-+----------------------+
 *  |                        |
 *  |   jQuery()             |
 *  |   Initialization       |
 *  |                        |
 *  +------------------------+
 * \endcode
 *
 * The e-Payment facility will generally be available when the e-Commerce
 * cart is around. However, ultimately we will want to offer full
 * AJAX support so we can load the facility when needed to avoid slow
 * down on all pages of the website.
 */



/** \brief Snap ePaymentFacilityBase constructor.
 *
 * The e-Payment system accepts multiple payment facilities.
 *
 * The base class derives from the ServerAccessCallbacks class so that
 * way the facility can may use of AJAX if necessary (which is very
 * likely for quite a few facilities...)
 *
 * Various facilities may make use of common code offered by the e-Payment
 * implementation and other plugins. For example, any facility asking for
 * a Credit Card number can make use of the e-Payment forms that include
 * the credit card info, the user address, and the registration for an
 * account for the user.
 *
 * \code
 *  class ePaymentFacilityBase extends ServerAccessCallbacks
 *  {
 *  public:
 *      ePaymentFacilityBase();
 *
 *      final function setCartModifiedCallback(callback: Function()) : Void;
 *      abstract function getFacilityName() : String;
 *      abstract function getDisplayName() : String;
 *      virtual function getIcon() : String;
 *      virtual function getButtonHTML() : String;
 *      final function getPriority() : Number;
 *
 *      abstract function buttonClicked() : String;
 *
 *      virtual function serverAccessComplete(result: ServerAccessCallbacks.ResultData);
 *
 *  private:
 *      var cartModifiedCallback_: Function();
 *  };
 * \endcode
 *
 * @return {snapwebsites.ePaymentFacilityBase}
 *
 * @extends snapwebsites.ServerAccessCallbacks
 * @constructor
 * @struct
 */
snapwebsites.ePaymentFacilityBase = function()
{
    return this;
};


/** \brief ePaymentFacilityBase inherits the ServerAccessCallbacks.
 *
 * This class is the only facility visible to the e-Payment plugin
 * implementation. The actual facilities have to be added along to
 * enable them on your website. In other words, by adding an e-payment
 * facility plugin, you offer that facility to your customers. This
 * means you should only add the facilities you want to use and not
 * just all facilities.
 *
 * Facilities can be marked as in conflict, although this can be
 * done at a user level only since in reality facilities won't be
 * in conflict. Only offering more than one direct credit card
 * gate way will be (very) confusing to end users. (i.e. if you
 * have PaypalPro + Chase + CitiBank + Bank of America and other
 * such credit card gate way facilities, how is the customer to
 * choose which gate way to use for his payment?)
 */
snapwebsites.inherits(snapwebsites.ePaymentFacilityBase, snapwebsites.ServerAccessCallbacks);


/** \brief Function called if cart was modified by server.
 *
 * Whenever sending messages to the backend, the server may end up changing
 * the cart under the client's feet. When that happens the facility should
 * be warned by the server.
 *
 * The function is set to a function in the ePayment object whenever a
 * facility is registered. The eCommerceCart object sets a function in
 * the ePayment object, function which gets called whenever the ePayment
 * object function gets called.
 *
 * @type {?function()}
 * @private
 */
snapwebsites.ePaymentFacilityBase.prototype.cartModifiedCallback_ = null;


/** \brief Set cart modified callback
 *
 * This function is used to setup a callback in case a message
 * says that the cart was modified by the server and thus needs
 * to be reloaded from the server.
 *
 * @param {function()} callback  The callback function which will be
 *                               called on a change of the cart.
 */
snapwebsites.ePaymentFacilityBase.prototype.setCartModifiedCallback = function(callback)
{
    this.cartModifiedCallback_ = callback;
};


/** \brief Get the technical name of this facility.
 *
 * This function returns a string with the technical name of this facility.
 * The technical name does not change and is generally all lowercase with
 * underscores for spaces. This allows that name to be used in places such
 * as the class of the tags used to create HTML output for this facility.
 *
 * @throws {Error} The base type function throws an error as it should never
 *                 get called (requires override of abstract function.)
 *
 * @return {string}  The technical name of the product.
 */
snapwebsites.ePaymentFacilityBase.prototype.getFacilityName = function() // abstract
{
    throw new Error("snapwebsites.ePaymentFacilityBase.getFacilityName() is not implemented");
};


/** \brief Get the display name of this facility.
 *
 * This function returns a string that can be displayed to represent
 * this facility. This name may be translated in various languages
 * or at least change slightly depending on the language or country
 * selected by the current user.
 *
 * This is generally used as a fallback if the icon cannot be used or
 * no icon was made available for this facility.
 *
 * @throws {Error} The base type function throws an error as it should never
 *                 get called (requires override of abstract function.)
 *
 * @return {string}  The display name of the product.
 */
snapwebsites.ePaymentFacilityBase.prototype.getDisplayName = function() // abstract
{
    throw new Error("snapwebsites.ePaymentFacilityBase.getDisplayName() is not implemented");
};


/** \brief Get the icon used to represent that facility.
 *
 * This function returns a URI to an image representing the facilty.
 * In most cases, that image is the logo of the payment facility.
 *
 * By default the function returns an empty string. If you do not
 * override this default, then the facility has no icon.
 *
 * @return {string}  The URI to an image representing the facility.
 */
snapwebsites.ePaymentFacilityBase.prototype.getIcon = function() // abstract
{
    return "";
};


/** \brief Generate HTML for a button to show this payment facility.
 *
 * This function is used to generate HTML that is most satisfactory
 * for that payment facility. This button is used to display a list of
 * facilities one can choose from to make a payment.
 *
 * The Base implementation generates a default entry which is likely
 * enough for most facilities.
 *
 * @return {string}  Facility button to display in the client's window.
 */
snapwebsites.ePaymentFacilityBase.prototype.getButtonHTML = function()
{
    var name = this.getFacilityName(),
        icon = this.getIcon(),
        html = "<span class='facility-name name-"
             + name + "'>"
             + this.getDisplayName() + "</span>";

    // TBD: should there be a way to place the image first?
    //      (at this point we can float those items, but we cannot
    //      get them up/down the other way, I'm afraid...)
    if(icon)
    {
        // add the image tag only if there is an icon
        html += "<img class='facility-icon icon-" + name + "' src='" + icon + "'/>";
    }

    return html;
};


/** \brief Get the priority assigned to this facility.
 *
 * By default facilities are given a priority of zero (0). Increasing the
 * priority moves the facility in the front of the list (i.e. a priority
 * of 100 gives you priority over a priority of 5 and thus with 100 you
 * are more likely to appear in the front of the list.)
 *
 * At this point the priority is not limited. It can also be negative to
 * move facilities are the very end of the list.
 *
 * \todo
 * Actually implement the priority. Note that the facility itself cannot
 * override this function since it is marked final.
 *
 * @return {number}  The priority assigned to this facility, zero by default.
 * @final
 */
snapwebsites.ePaymentFacilityBase.prototype.getPriority = function()
{
    return 0;
};


/** \brief This facility button was clicked.
 *
 * This callback function is called whenever the facility button gets
 * clicked. This allows the facility to either open another dialog or
 * send a message to the server as required by that facility.
 *
 * The base class does not implement this function. It must be
 * re-implemented.
 */
snapwebsites.ePaymentFacilityBase.prototype.buttonClicked = function() // abstract
{
    throw new Error("snapwebsites.ePaymentFacilityBase.buttonClicked() is not implemented");
};


/** \brief Function called on completion.
 *
 * This function is called once the whole process is over. It is most
 * often used to do some cleanup.
 *
 * By default this function does nothing.
 *
 * @param {snapwebsites.ServerAccessCallbacks.ResultData} result  The
 *          resulting data with information about the error(s).
 */
snapwebsites.ePaymentFacilityBase.prototype.serverAccessComplete = function(result) // virtual
{
    var that = this,
        result_xml = result.jqxhr.responseXML;

    // manage messages if any
    snapwebsites.ePaymentFacilityBase.superClass_.serverAccessComplete.call(this, result);

    // on errors, it is not unlikely that we get a new cart which
    // we need to replicate in the HTML cart
    jQuery("snap", result_xml)
        .children("data[name='ecommerce__cart_modified']")
        .each(function() // "each"... there will be only one
            {
                if(that.cartModifiedCallback_)
                {
                    that.cartModifiedCallback_();
                }
            });
};



/** \brief Snap ePayment constructor.
 *
 * The ePayment includes everything required to handle electronic and
 * not so electronic payments:
 *
 * - Allow listing all the available payments
 * - Allow assignment of a priority to each payment facility
 *   (facilities with the highest priority appear first, possibly hiding
 *   the others and we add a 'More Choices ->' button)
 * - Allow AJAX to register a user account
 * - Allow AJAX to process the payment
 *
 * Note that many of the AJAX features have to directly be handled by
 * the facility since e-Payment does not otherwise know how to handle
 * those requests. e-Payment handles the AJAX user registration though.
 *
 * Note that this class is marked as final because it is used as a
 * singleton. To extend e-Payments you have to create payment facilities.
 *
 * \code
 *  final class ePayment extends ServerAccessCallbacks
 *  {
 *  public:
 *      ePayment();
 *
 *      function registerPaymentFacility(payment_facility: ePaymentFacilityBase) : Void;
 *      function hasPaymentFacility(feature_name: String) : Boolean;
 *      function getPaymentFacility(feature_name: String) : ePaymentFacilityBase;
 *
 *      function mainFacilitiesHTML(width: Number, back_button: Boolean) : String;
 *      function sortFacilitiesByPriority() : Void;
 *      function setCartModifiedCallback(callback: Function()) : Void;
 *
 *  private:
 *      static function compareFacilities_(a, b) : Number;
 *
 *      var paymentFacilities_: ePaymentFacilitiesBase;
 *      var sortedPaymentFacility_: Array of Number;
 *      var sorted_: Boolean := false;
 *      var cartModifiedCallback_: Function();
 *  };
 * \endcode
 *
 * The e-Payment plugin supports several steps to handle a payment. All the
 * steps are always being processed, although certain payment facilities
 * may ignore (skip) a step.
 *
 * Each facility can tell what is required, what is optional, what is
 * not acceptable (i.e. Google Checkout used to prevent systems from
 * asking for any user information, including their address and real
 * name!)
 *
 * \li The e-Payment plugin expects the user to start a Payment Request.
 * This allows the e-Payment system and the user of the the e-Payment to
 * know how to proceed.
 *
 * \li Check list of facilities; once the user specified what needs to be
 * purchased, a list of facilities may be offered to the user. In most cases,
 * all facilities will be available, although it can happen that the user
 * is registered and, for example, Checks are allowed for him. Other reasons
 * for a facility to be available or not is they user country, or the items
 * purchased.
 *
 * \li Log In or Register the user. This is an option offered by the
 * e-Payment system because many situations require a user to have an
 * account. Purchasing software means getting a download and we want to
 * limit downloads to registered users. Purchasing a page to put up an
 * ad will also require an account.
 *
 * \li From the list of facilities allowed, you can offer the user to
 * select which payment facility to use to pay for his purchase.
 * Future versions will allow the user to choose more than once facility
 * to make their payment (i.e. pay X with Paypal and Y with a credit card).
 * For companies that have bank accounts in various countries, we could
 * also offer payment to be made in various currency (i.e. user can choose
 * between paying in USD or EUR, for example.) If only one facility is
 * available to the user, we expect no selection here since there is no
 * choice anyway (i.e. only accept Paypal payments...)
 *
 * \li User information: real name, billing/shipping address (street, city,
 * postal code, country), phone number, credit card (number, expiration
 * date, verification code [CVV]).
 *
 * \li Leave room for a comment, just in case. It can be useful in many
 * situations (i.e. when ordering food, maybe you want to ask for the onions
 * to be removed!) We Want to offer a button "Add Comment" as well as a
 * comment box already open.
 *
 * Once a payment is ready to be processed, the code calls the process()
 * function. This has multiple phases which may or may not be required by
 * the various payment facilities. This processing is possible only once
 * all the necessary data is available (i.e. for a credit card payment, you
 * need to have the user name, a credit card number, expiration date, CVV,
 * and probably a zip code or address to further verify the card.) This
 * processing includes the following states:
 *
 * \li created -- the invoice was created (generate_invoice() called)
 * \li processing -- started payment processing;
 * \li pending -- waiting for an asynchroneous reply from a payment facility
 * (i.e. the old Paypal does not send replies immediately, although generally
 * it is very fast, it is definitively 100% asynchroneous); this step
 * only applies to facilities that require our system to wait for
 * confirmation asynchroneously (i.e. PayPal REST does not need this status);
 * \li canceled -- the payment was canceled before it was completed, in
 * most cases this is the user canceling (see failed too);
 * \li failed -- the payment failed (i.e. credit card refused, check
 * bounced, etc.);
 * \li paid -- the payment was confirmed, now we can ship (if shipping
 * or equivalent there is);
 * \li completed -- the shipping was done, or the payment was recieved
 * and there was no shipping.
 *
 * Note: We mention shipping in the table before because in most cases
 * a purchase goes from Paid to Completed when a required operator action
 * was performed to complete the sale. This could be something else than
 * shipping such as making a phone call or manually adding the name of the
 * user to a list (i.e. maybe you will have a draw and need each user's
 * name printed on a piece of paper...) or rendering a service.
 *
 * When a feature needs more information about its own status, it has to
 * use its own variable and not count on the e-Payment plugin to handle
 * its special status. In other words, as far as the e-Payment plugin is
 * concerned, while processing a payment, it is in mode "pending", nothing
 * more. It would otherwise make things way more complicated than they
 * need to be (i.e. dynamic statuses?)
 *
 * Products that do not require shipping anything, go to the completed
 * status directly.
 *
 * A payment is attached to the same session as the cart session. That
 * session can then be used to generate an invoice (assuming your plugin
 * does not itself generate an invoice and uses the payment facility
 * to get it paid for.)
 *
 * The e-Payment is not in charge of the invoice capability. The e-Commerce
 * system (or your own implementation) is in charge of that capability. The
 * e-Payment plugin will request for the invoice to be created only once it
 * starts the payment processing. This way we avoid creating invoices for
 * users who come visit (Especially because you can add items to a cart using
 * a simple link... and thus you could end up creating hundred of useless
 * invoices!) The e-Commerce cart only creates a Sales Order which is
 * converted to an invoice once the payment processing starts.
 *
 * \note
 * The e-Payment has one limit which should pretty much never be a problem:
 * it does not allow for the processing of more than one payment at a time.
 *
 * \todo
 * Allow for the use of more than one payment facility to pay for
 * one fee.
 *
 * \todo
 * Allow for selecting an address from the user account. This is assuming
 * we allow for more than one address to be registered with one user.
 *
 * \todo
 * We save all of that information in the invoice we generate along
 * the payment. We can also save part of that information in the user's
 * account. It is important to save it in the invoice because the
 * user can change his information and the invoice needs to not
 * change over time.
 *
 * @return {snapwebsites.ePayment}  The newly created object.
 *
 * @constructor
 * @extends {snapwebsites.ServerAccessCallbacks}
 * @struct
 */
snapwebsites.ePayment = function()
{
//#ifdef DEBUG
    if(jQuery("body").hasClass("snap-epayment-initialized"))
    {
        throw new Error("Only one e-Payment singleton can be created.");
    }
    jQuery("body").addClass("snap-epayment-initialized");
//#endif
    snapwebsites.ePayment.superClass_.constructor.call(this);

    this.paymentFacilities_ = {};

    return this;
};


/** \brief Mark the eCommerceCart as inheriting from the ServerAccessCallbacks.
 *
 * This class inherits from the ServerAccessCallbacks, which it uses
 * to send the server changes made by the client to the cart.
 * In the cart, that feature is pretty much always asynchroneous.
 */
snapwebsites.inherits(snapwebsites.ePayment, snapwebsites.ServerAccessCallbacks);


/** \brief The instance of the e-Payment object.
 *
 * This variable is used to save the instance of the one e-Payment
 * object used in Snap!
 *
 * @type {snapwebsites.ePayment}
 */
snapwebsites.ePaymentInstance = null; // static


/** \brief The list of payment facilities understood by the e-Payment plugin.
 *
 * This map is used to save the payment facilities when you call the
 * registerPaymentFacility() function.
 *
 * \note
 * The registered payment facilities cannot be unregistered.
 *
 * @type {Object}
 * @private
 */
snapwebsites.ePayment.prototype.paymentFacilities_; // = {}; -- initialized in constructor to avoid problems


/** \brief The list of payments sorted by priority.
 *
 * When displaying the payments to the end user, we use a priority to sort
 * the facilities. This way the administrator can choose to sort his payment
 * facilities in a way that makes more sense for his business.
 *
 * \warning
 * This array is initialized by a call to the sortFacilitiesByPriority()
 * function, which should always be performed before using this variable.
 *
 * @type {Array}
 * @private
 */
snapwebsites.ePayment.prototype.sortedPaymentFacility_; // = []; -- initialized each time the number of facilities changes


/** \brief Whether the sortedPaymentFacility_ is current.
 *
 * This flag is used to keep track of the sortedPaymentFacility_ and know
 * whether it is up to date. Whenever the map of payments is updated,
 * this flag is reset back to false. Once sorted, it gets set to true.
 *
 * Functions should not rely on this flag, instead, they should call the
 * sortFacilitiesByPriority() function which ensures the proper validity
 * of the sortedPaymentFacility_ array.
 *
 * @type {!boolean}
 * @private
 */
snapwebsites.ePayment.prototype.sorted_ = false;


/** \brief Function called when cart gets modified by server.
 *
 * Whenever sending messages to the backend, the server may end up changing
 * the cart under the client's feet. When that happens the facility should
 * be warned by the server.
 *
 * This function is set to a function from the outside of the ePayment
 * realm. Any implementation of a cart. The function is not required
 * and can be left to be set to null.
 *
 * @type {?function()}
 * @private
 */
snapwebsites.ePayment.prototype.cartModifiedCallback_ = null;


/** \brief Register a payment facility.
 *
 * This function is used to register a payment facility in the ePayment
 * object.
 *
 * This allows for any number of payment facilities and thus any number of
 * satisfied customers as they can use any mean to pay for their purchases.
 *
 * @param {snapwebsites.ePaymentFacilityBase} payment_facility  The payment facility to register.
 *
 * @final
 */
snapwebsites.ePayment.prototype.registerPaymentFacility = function(payment_facility)
{
    var that = this,
        name = payment_facility.getFacilityName();

    this.paymentFacilities_[name] = payment_facility;

    // in case the cart is modified, we want to call the Cart Modified Callback
    payment_facility.setCartModifiedCallback(function()
        {
            if(that.cartModifiedCallback_)
            {
                that.cartModifiedCallback_();
            }
        });
};


/** \brief Check whether a facility is available.
 *
 * This function checks the list of payment facilities to see whether
 * \p facility_name exists.
 *
 * @param {!string} facility_name  The name of the facility to check.
 *
 * @return {!boolean}  true if the facility is defined.
 *
 * @final
 */
snapwebsites.ePayment.prototype.hasPaymentFacility = function(facility_name)
{
    return this.paymentFacilities_[facility_name] instanceof snapwebsites.ePaymentFacilityBase;
};


/** \brief This function is used to get a payment facility.
 *
 * Payment facilities get registered with the registerPaymentFacility() function.
 * Later you may retrieve them using this function.
 *
 * @throws {Error} If the named \p facility_name was not yet registered, then this
 *                 function throws.
 *
 * @param {string} facility_name  The name of the product feature to retrieve.
 *
 * @return {snapwebsites.ePaymentFacilityBase}  The product feature object.
 *
 * @final
 */
snapwebsites.ePayment.prototype.getPaymentFacility = function(facility_name)
{
    if(this.paymentFacilities_[facility_name] instanceof snapwebsites.ePaymentFacilityBase)
    {
        return this.paymentFacilities_[facility_name];
    }
    throw new Error("getPaymentFacility(\"" + facility_name + "\") called when that facility is not yet defined.");
};


/** \brief Compare two payment facilities to determine which should be first.
 *
 * This function compares two payment facilities. If their priority is not
 * enough to know which is first and which is last, then the function compares
 * their display name (i.e. alphabetical order,) and since two facilities
 * should not have the same display name, this function should always return
 * -1 or +1 and never zero.
 *
 * \note
 * We use the localeCompare() function without forcing the result to +1
 * or -1 so you are likely to get varying results depending on the browser.
 * The result is always what the Array.sort() functon expects though.
 *
 * @param {snapwebsites.ePaymentFacilityBase} a  The left handside.
 * @param {snapwebsites.ePaymentFacilityBase} b  The right handside.
 *
 * @return {!number} -1 if 'a' is before 'b', +1 if 'a' is after 'b',
 *                   and 0 if 'a' and 'b' are equal.
 *
 * @private
 * @final
 */
snapwebsites.ePayment.compareFacilities_ = function(a, b) // static
{
    var pa = a.getPriority(),
        pb = b.getPriority(),
        na,
        nb;

    // WARNING: a larger priority comes first, so the compare "looks" inverted
    //          but it is correct
    if(pa > pb)
    {
        return -1;
    }
    if(pa < pb)
    {
        return 1;
    }

    // names are sorted as expected, following the client's locale
    na = a.getDisplayName();
    nb = b.getDisplayName();
    return na.localeCompare(nb);
};


/** \brief Sort the facilities.
 *
 * This function is used to sort the facilities using their priority
 * property. By default, facilities are saved in this object using
 * their technical name instead.
 *
 * The function marks the facilities as sorted to avoid sorting over
 * and over again. The flag is cleared each time the user adds a new
 * facility.
 */
snapwebsites.ePayment.prototype.sortFacilitiesByPriority = function()
{
    var key;

    if(this.sorted_)
    {
        return;
    }
    this.sorted_ = true;

    // reset the array
    this.sortedPaymentFacility_ = [];

    // add the facilities
    for(key in this.paymentFacilities_)
    {
        if(this.paymentFacilities_.hasOwnProperty(key))
        {
            this.sortedPaymentFacility_.push(this.paymentFacilities_[key]);
        }
    }

    // now do the sort
    this.sortedPaymentFacility_.sort(snapwebsites.ePayment.compareFacilities_);
};


/** \brief Set cart modified callback
 *
 * This function is used to setup a callback in case a message
 * says that the cart was modified by the server and thus needs
 * to be reloaded from the server.
 *
 * @param {function()} callback  The callback function which will be
 *                               called on a change of the cart.
 */
snapwebsites.ePayment.prototype.setCartModifiedCallback = function(callback)
{
    this.cartModifiedCallback_ = callback;
};


/** \brief List the main facilities for this website.
 *
 * Whenever an administrator sets up his e-Commerce system, he can choose
 * a set of payment facilities as being the main facilities (i.e. maybe
 * Paypal, Credit Cards, and Bitcoins.) If there are other facilities
 * they remain hidden and the function adds a 'More...' button.
 *
 * Facilities can be assigned a priority which is used to sort them.
 *
 * The \p width parameter is used to determine when to stop adding
 * facilities to the list. The 'More...' button gets added only if
 * all the facilities cannot fit within that width.
 *
 * \note
 * The width computation probably requires us to know the display width
 * which is not really known here. However, you can have the same number
 * of facilities displayed in a smaller or larger space depending on how
 * your output space is sized.
 *
 * @param {jQuery} cart_payment  The element where the HTML gets appended.
 * @param {number} width  The maximum width of the resulting HTML.
 * @param {boolean} back_button  Whether a back button should be added.
 */
snapwebsites.ePayment.prototype.appendMainFacilities = function(cart_payment, width, back_button)
{
    var that = this,
        html = "",
        icon_width = 150,
        idx,
        max,
        facility,
        name;

    this.sortFacilitiesByPriority();

    // TODO: get the width from the CSS so that way we can make sure this
    //       math is correct
    //
    max = Math.floor(width / (icon_width + 30));
    if(max < 1)
    {
        // should the minimum be more than 1?
        max = 1;
    }
    if(max > this.sortedPaymentFacility_.length)
    {
        max = this.sortedPaymentFacility_.length;
    }

    html += "<div class='epayment'><div class='epayment-header'>Please choose a mode of payment:</div><div class='epayment-facilities'>";
    if(max == 0)
    {
        html += "<div class='no-epayment-facilities'>No payment facilities have been installed on this website.</div>";
    }
    else
    {
        for(idx = 0; idx < max; ++idx)
        {
            facility = this.sortedPaymentFacility_[idx];
            name = facility.getFacilityName();

            html += "<div class='epayment-facility item-"
                  + name + "' data-name='" + name + "'>"
                  + facility.getButtonHTML()
                  + "</div>";
        }
        html += "<div class='clear-both'></div>";
    }
    html += "</div>";
    if(back_button || max < this.sortedPaymentFacility_.length)
    {
        html += "<div class='epayment-buttons'>";
        if(back_button)
        {
            html += "<a class='epayment-back' href='#back'>« Back</a>";
        }
        if(max < this.sortedPaymentFacility_.length)
        {
            html += "<a class='epayment-more' href='#more'>More</a>";
        }
        html += "<div class='clear-both'></div></div>";
    }
    html += "</div>";

    cart_payment.append(html);

    // now connect to those buttons
    cart_payment.find(".epayment-facility").click(function(e)
        {
            var facility_name = /** @type {string} */ (jQuery(this).data("name")),
                this_facility = that.getPaymentFacility(snapwebsites.castToString(facility_name, "epayment-facility tag had no 'name' attribute?"));

            e.preventDefault();
            e.stopPropagation();

            this_facility.buttonClicked();
        });
    cart_payment.find(".epayment-back").click(function(e)
        {
            e.preventDefault();
            e.stopPropagation();

            cart_payment.hide();
            cart_payment.parent().find(".cart-summary").show(); // TODO: fix totally broken dependency
        });
    cart_payment.find(".epayment-more").click(function(e)
        {
            e.preventDefault();
            e.stopPropagation();

            alert("\"More\" not implemented yet");
        });
};



// auto-initialize
jQuery(document).ready(function()
    {
        snapwebsites.ePaymentInstance = new snapwebsites.ePayment();
        // to add facilities, do something like this:
        //snapwebsites.ePaymentInstance.registerPaymentFacility(new snapwebsites.ePaymentFacility...());
    });

// vim: ts=4 sw=4 et
