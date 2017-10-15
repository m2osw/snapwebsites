/** @preserve
 * Name: epayment-credit-card-form
 * Version: 0.0.1.44
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
 * \brief Various functions handling tabs and such in the popup form.
 *
 * This file defines a class with a set of functions used to handle the
 * credit card form that appears in a popup. This form includes tabs
 * and buttons which this class takes care of.
 */



/** \brief Snap ePaymentCreditCardForm constructor.
 *
 * The e-Payment credit card form handler. One singleton gets created
 * whenever the script gets loaded. It is used to handle the credit
 * card form that appears in a popup when a payment button gets clicked.
 *
 * @return {snapwebsites.ePaymentCreditCardForm}
 *
 * @extends {snapwebsites.ServerAccessCallbacks}
 * @constructor
 * @struct
 */
snapwebsites.ePaymentCreditCardForm = function()
{
    var that = this,
        editor = snapwebsites.EditorInstance,
        credit_card_form = editor.getFormByName("credit_card_form"),
        save_dialog = credit_card_form.getSaveDialog();

    this.tabItems_ = jQuery("div.tab-selector div.epayment-credit-card-tab");
    this.tabList_ = jQuery("ul.epayment-credit-card-tab-list li");
    this.firstTabButton_ = jQuery("ul.epayment-credit-card-tab-list li:first");
    this.lastTabButton_ = jQuery("ul.epayment-credit-card-tab-list li:last");

    this.tabList_
        .makeButton()
        .click(function(e)
            {
                var tab_name = jQuery(this).attr("for");

                e.preventDefault();

                that.tab_(snapwebsites.castToString(tab_name, "e-payment credit card tab name"));
            });

    // get the buttons
    this.cancel_ = jQuery("div.epayment-buttons a.cancel");
    this.back_ = jQuery("div.epayment-buttons a.back");
    this.next_ = jQuery("div.epayment-buttons a.next");
    this.send_ = jQuery("div.epayment-buttons a.send");
    this.sendWrapper_ = jQuery("div.epayment-buttons span.send-button-wrapper");

    // connect the buttons
    this.cancel_
        .makeButton()
        .click(function(e)
            {
                e.preventDefault();

                snapwebsites.PopupInstance.closePopup();
            });

    this.back_
        .makeButton()
        .click(function(e)
            {
                e.preventDefault();

                that.previousTab_();
            });

    this.next_
        .makeButton()
        .click(function(e)
            {
                e.preventDefault();

                that.nextTab_();
            });

    save_dialog.setPopup(this.send_);
    this.send_
        .makeButton()
        .click(function(e)
            {
                var card_number_widget = credit_card_form.getWidgetByName("card_number"),
                    card_number = card_number_widget.getValue(),
                    gateway_widget = credit_card_form.getWidgetByName("gateway"),
                    gateway = gateway_widget.getValue();

                e.preventDefault();

                if(that.luhnCheck(card_number))
                {
                    credit_card_form.saveData("save", { gateway: gateway });
                }
                else
                {
                    snapwebsites.OutputInstance.displayOneMessage(
                            "Invalid Field",
                            "The credit card number you entered does not seem valid. Please verify you entered your card number properly.",
                            "error",
                            true);
                }
            });

    // TODO: select "create user account" if the user is not currently
    //       logged in and allow the user to log in if they already
    //       have an account
    //
    this.tab_("card-info");

    return this;
};


/** \brief ePaymentCreditCard inherits the ServerAccessCallbacks.
 *
 * This class implements the Credit Card payment extension.
 */
snapwebsites.inherits(snapwebsites.ePaymentCreditCardForm, snapwebsites.ServerAccessCallbacks);


/** \brief The server access used by this facility.
 *
 * Used to send the credit card form to the server.
 *
 * @type {snapwebsites.ePaymentCreditCardForm}
 */
snapwebsites.ePaymentCreditCardFormInstance = null; // static


/** \brief The server access used by this facility.
 *
 * Used to send the credit card form to the server.
 *
 * @type {snapwebsites.ServerAccess}
 * @private
 */
snapwebsites.ePaymentCreditCardForm.prototype.serverAccess_ = null;


/** \brief List of tab items (div.epayment-credit-card-tab tags).
 *
 * This variable holds the list of tab items so we do not have to
 * reprocess the form each time we want to access those items.
 *
 * @type {?jQuery}
 * @private
 */
snapwebsites.ePaymentCreditCardForm.prototype.tabItems_ = null;


/** \brief List of tab buttons (li tags).
 *
 * This variable holds the list of tab buttons, which are HTML list
 * items (li tags) so we do not have to reprocess the form each time
 * we want to access those items.
 *
 * @type {?jQuery}
 * @private
 */
snapwebsites.ePaymentCreditCardForm.prototype.tabList_ = null;


/** \brief The first tab buttons.
 *
 * This variable holds the first tab button. If that button is the
 * one active, then we hide the "<< Back" button.
 *
 * @type {?jQuery}
 * @private
 */
snapwebsites.ePaymentCreditCardForm.prototype.firstTabButton_ = null;


/** \brief The last tab buttons.
 *
 * This variable holds the last tab button. If that button is the
 * one active, then we replace the "Next >>" button with the "Send"
 * button.
 *
 * @type {?jQuery}
 * @private
 */
snapwebsites.ePaymentCreditCardForm.prototype.lastTabButton_ = null;


/** \brief The "Next >>" button.
 *
 * This variable holds the jQuery object of the next button.
 *
 * @type {?jQuery}
 * @private
 */
snapwebsites.ePaymentCreditCardForm.prototype.next_ = null;


/** \brief The "<< Back" button.
 *
 * This variable holds the jQuery object of the back button.
 *
 * @type {?jQuery}
 * @private
 */
snapwebsites.ePaymentCreditCardForm.prototype.back_ = null;


/** \brief The "Send" button.
 *
 * This variable holds the jQuery object of the send button.
 *
 * @type {?jQuery}
 * @private
 */
snapwebsites.ePaymentCreditCardForm.prototype.send_ = null;


/** \brief The wrapper around the "Send" button.
 *
 * This variable holds the jQuery object of the send button wrapper which
 * is shown/hidden independently of the send button itself.
 *
 * @type {?jQuery}
 * @private
 */
snapwebsites.ePaymentCreditCardForm.prototype.sendWrapper_ = null;


/** \brief The "Cancel" button.
 *
 * This variable holds the jQuery object of the cancel button.
 *
 * @type {?jQuery}
 * @private
 */
snapwebsites.ePaymentCreditCardForm.prototype.cancel_ = null;


/** \brief Compute the Luhn number and return true if valid.
 *
 * This function computes the Luhn number to validate the card
 * number before forwarding the data to the server.
 *
 * @param {string} number  The credit card number to check.
 *
 * @return {boolean}  true if the number is considered valid.
 */
snapwebsites.ePaymentCreditCardForm.prototype.luhnCheck = function(number)
{
    var odd = true,
        idx = number.length,
        len = 0,
        d,
        luhn = 0;

    // got through digits in reverse order
    while(idx > 0)
    {
        --idx;

        d = number.charCodeAt(idx);
        if(d == 45  // '-'
        || d == 32) // ' '
        {
            // we ignore dashes and spaces
            //
            continue;
        }

        if(d < 48  // '0'
        || d > 57) // '9'
        {
            // we only accept digits otherwise
            //
            return false;
        }
        d -= 48;

        // Luhn "math"
        odd = !odd;
        if(odd)  // WARNING: (idx & 1) is NOT correct because we start at length and it could be odd/even the "other way around"
        {
            d *= 2;
        }
        if(d > 9)
        {
            d -= 9;
        }

        ++len;
        luhn += d;
    }

    // all credit card numbers are between 10 and 16 digits at this time
    if(len < 10
    || len > 16)
    {
        return false;
    }

    // Luhn number modulo 10 must be zero to be a valid card number
    return luhn % 10 === 0;
};


/** \brief One of the tabs was clicked, show the corresponding form.
 *
 * This function selects the tab named \p tab_name.
 *
 * @param {string} tab_name  The name of the tab to select.
 *
 * @private
 */
snapwebsites.ePaymentCreditCardForm.prototype.tab_ = function(tab_name)
{
    var status;

    this.tabItems_
        .hide()
        .filter("[data-name=" + tab_name + "]")
        .show();

    this.tabList_
        .removeClass("active")
        .filter("[for=" + tab_name + "]")
        .addClass("active");

    // replace the "Next >>" button with the "Send" button when
    // on last item
    //
    status = this.lastTabButton_.hasClass("active");
    this.next_.toggle(!status);
    this.sendWrapper_.toggle(status);

    // hide the "<< Back" button when on first item
    //
    status = this.firstTabButton_.hasClass("active");
    this.back_.toggle(!status);
};


/** \brief Go to the next tag.
 *
 * This function determines the next tag and then calls tab_() with its
 * name.
 *
 * @private
 */
snapwebsites.ePaymentCreditCardForm.prototype.nextTab_ = function()
{
    var tab_name = jQuery("ul.epayment-credit-card-tab-list li.active").next().attr("for");

    if(tab_name)
    {
        this.tab_(snapwebsites.castToString(tab_name, "e-payment credit card tab name"));
    }
};


/** \brief Go to the previous tag.
 *
 * This function determines the previous tag and then calls the tab_()
 * with its name.
 *
 * @private
 */
snapwebsites.ePaymentCreditCardForm.prototype.previousTab_ = function()
{
    var tab_name = jQuery("ul.epayment-credit-card-tab-list li.active").prev().attr("for");

    if(tab_name)
    {
        this.tab_(snapwebsites.castToString(tab_name, "e-payment credit card tab name"));
    }
};


/** \brief This facility button was clicked.
 *
 * This Stripe implementation sends the click to the server directly.
 * It expects the server to register the invoice and start the checkout
 * process with Stripe Express.
 *
 * @private
 */
snapwebsites.ePaymentCreditCardForm.prototype.sendForm_ = function()
{
    if(!this.serverAccess_)
    {
        this.serverAccess_ = new snapwebsites.ServerAccess(this);
    }

    this.serverAccess_.setURI(snapwebsites.castToString(jQuery("link[rel='page-uri']").attr("href"), "casting href of the page-uri link to a string in snapwebsites.ePaymentCreditCardForm.sendForm_()") + "?a=view");
    this.serverAccess_.showWaitScreen(1);
    this.serverAccess_.setData(
        {
            epayment__epayment_stripe: "checkout"
        });
    this.serverAccess_.send();

    // now we wait for an answer which should give us a URL to redirect
    // the user to a Stripe page
};



// auto-initialize
jQuery(document).ready(function()
    {
        snapwebsites.ePaymentCreditCardFormInstance = new snapwebsites.ePaymentCreditCardForm();
    });

// vim: ts=4 sw=4 et
