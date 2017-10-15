/** @preserve
 * Name: epayment-credit-card
 * Version: 0.0.1.6
 * Browsers: all
 * Depends: epayment (>= 0.0.1.33)
 * Copyright: Copyright 2016-2017 (c) Made to Order Software Corporation  All rights reverved.
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
// @js plugins/epayment/epayment.js
// ==/ClosureCompiler==
//

/*
 * JSLint options are defined online at:
 *    http://www.jshint.com/docs/options/
 */
/*jslint nomen: true, todo: true, devel: true */
/*global snapwebsites: false, jQuery: false, FileReader: true, Blob: true */



/** \file
 * \brief e-Payment facilities that are credit card gateways use this class.
 *
 * This file defines the credit card form capability.
 */



/** \brief Snap ePaymentCreditCard constructor.
 *
 * The e-Payment Credit Card class allows you to open the credit card
 * form whenever required by your e-Payment facility. You instantiate
 * it when you need to present the form to an end user.
 *
 * The gateway parameter is the name of your plugin. It will be attached
 * to the form and used whenever the form gets submitted and your gateway
 * is expected to process the payment.
 *
 * @param {string} gateway  The name of the gateway (i.e. name of a plugin.)
 *
 * @return {snapwebsites.ePaymentCreditCard}
 *
 * @extends {snapwebsites.ServerAccessCallbacks}
 * @constructor
 * @struct
 */
snapwebsites.ePaymentCreditCard = function(gateway)
{
    this.gateway_ = gateway;

    this.popup_ = {
        id: "e-payment-credit-card",
        title: "Credit Card Details",
        path: "/epayment/credit-card?gateway=" + this.gateway_ + "&from=" + jQuery("link[rel='page-uri']").attr("href"),
        darken: 150,
        width: 500,
        height: 375
    };

    snapwebsites.PopupInstance.open(this.popup_);
    snapwebsites.PopupInstance.show(this.popup_);

    return this;
};


/** \brief ePaymentCreditCard inherits the ServerAccessCallbacks.
 *
 * This class implements the Credit Card payment extension.
 */
snapwebsites.inherits(snapwebsites.ePaymentCreditCard, snapwebsites.ServerAccessCallbacks);


/** \brief The credit card popup details.
 *
 * This variable is static, meaning that there is only one instance
 * even if you create multiple ePaymentCreditCard objects.
 *
 * @type {Object}
 * @private
 */
snapwebsites.ePaymentCreditCard.prototype.popup_ = null;


///** \brief The credit card popup window.
// *
// * This variable holds the popup window that was opened by the constructor.
// *
// * @type {Object}
// * @private
// */
//snapwebsites.ePaymentCreditCard.prototype.popupWindow_ = null;


/** \brief The name of the gateway using this form.
 *
 * This variable holds the name of the gateway that generated this form.
 * It is used when generating the form and later when submitting it.
 *
 * @type {string}
 * @private
 */
snapwebsites.ePaymentCreditCard.prototype.gateway_ = "";


///** \brief The server access used by this facility.
// *
// * Used to send the credit card form to the server.
// *
// * @type {snapwebsites.ServerAccess}
// * @private
// */
//snapwebsites.ePaymentCreditCard.prototype.serverAccess_ = null;


/** \brief Close this credit card form instance.
 *
 * This function can be called in order to close the popup
 * window the constructor created.
 */
snapwebsites.ePaymentCreditCard.prototype.closeWindow = function()
{
    snapwebsites.PopupInstance.hide(this.popup_);
};


///** \brief This facility button was clicked.
// *
// * This Stripe implementation sends the click to the server directly.
// * It expects the server to register the invoice and start the checkout
// * process with Stripe Express.
// */
//snapwebsites.ePaymentCreditCard.prototype.buttonClicked = function()
//{
//    if(!this.serverAccess_)
//    {
//        this.serverAccess_ = new snapwebsites.ServerAccess(this);
//    }
//
//    this.serverAccess_.setURI(snapwebsites.castToString(jQuery("link[rel='page-uri']").attr("href"), "casting href of the page-uri link to a string in snapwebsites.ePaymentFacilityStripe.buttonClicked()") + "?a=view");
//    this.serverAccess_.showWaitScreen(1);
//    this.serverAccess_.setData(
//        {
//            epayment__epayment_stripe: "checkout"
//        });
//    this.serverAccess_.send();
//
//    // now we wait for an answer which should give us a URL to redirect
//    // the user to a Stripe page
//};



// vim: ts=4 sw=4 et
