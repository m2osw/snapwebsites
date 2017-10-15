/** @preserve
 * Name: bookkeeping-add-client
 * Version: 0.0.1.13
 * Browsers: all
 * Copyright: Copyright 2016-2017 (c) Made to Order Software Corporation  All rights reverved.
 * Depends: output (0.1.5)
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
// @js plugins/bookkeeping/bookkeeping-client.js
// ==/ClosureCompiler==
//

/*jslint nomen: true, todo: true, devel: true */
/*global jQuery: false, Uint8Array: true */



/** \brief Bookkeeping constructor.
 *
 * The Bookkeeping Client is a singleton handling the Bookkeeping
 * Client window which has an Add Client button.
 *
 * @return {snapwebsites.BookkeepingAddClient}  The newly created object.
 *
 * @constructor
 * @struct
 * @extends {snapwebsites.ServerAccessCallbacks}
 */
snapwebsites.BookkeepingAddClient = function()
{
    var that = this,
        editor = snapwebsites.EditorInstance,
        bookkeeping_add_client_form = editor.getFormByName("bookkeeping_add_client"),
        save_dialog,
        save_button;

    if(!bookkeeping_add_client_form)
    {
        // form not present, ignore, we are probably not on a "Day" page
        return this;
    }

    save_button = jQuery("div.buttons a.bookkeeping-button.save-button");

    save_dialog = bookkeeping_add_client_form.getSaveDialog();
    save_dialog.setPopup(save_button);

    bookkeeping_add_client_form.setTimedoutCallback(function(bookkeeping_add_client_form)
        {
            that.closeAddClientPopup_();
        });

    bookkeeping_add_client_form.setSaveFunctionOnSuccess(function(bookkeeping_add_client_form, result)
        {
            that.closeAddClientPopup_();
        });

    save_button
        .makeButton()
        .click(function(e)
            {
                var client_name     = bookkeeping_add_client_form.getWidgetByName("client_name"),
                    client_address1 = bookkeeping_add_client_form.getWidgetByName("client_address1"),
                    client_city     = bookkeeping_add_client_form.getWidgetByName("client_city"),
                    client_state    = bookkeeping_add_client_form.getWidgetByName("client_state"),
                    client_zip      = bookkeeping_add_client_form.getWidgetByName("client_zip");

                // avoid the '#' from appearing in the URI
                e.preventDefault();
                e.stopPropagation();

                // We want to create a new page so for the purpose we
                // use a ServerAccess object
                //
                if(!that.serverAccess_)
                {
                    that.serverAccess_ = new snapwebsites.ServerAccess(that);
                }
                that.serverAccess_.setURI("/bookkeeping/client/add-client");
                that.serverAccess_.showWaitScreen(1);
                that.serverAccess_.setData(
                        {
                            client_name:     client_name    .getValue(),
                            client_address1: client_address1.getValue(),
                            client_city:     client_city    .getValue(),
                            client_state:    client_state   .getValue(),
                            client_zip:      client_zip     .getValue()
                        });
                that.serverAccess_.send(e);
            });

    jQuery("div.buttons a.bookkeeping-button.cancel-button")
        .makeButton()
        .click(function()
            {
                that.closeAddClientPopup_();
            });

    return this;
};


/** \brief BookkeepingAddClient inherits the ServerAccessCallbacks.
 *
 * The BookkeepingAddClient class inherit the ServerAccessCallbacks class
 * so it knows when an AJAX request succeeded.
 */
snapwebsites.inherits(snapwebsites.BookkeepingAddClient, snapwebsites.ServerAccessCallbacks);


/** \brief The BookkeepingAddClient instance.
 *
 * This class is a singleton and as such it makes use of a static
 * reference to itself. It gets created on load.
 *
 * \@type {snapwebsites.BookkeepingAddClient}
 */
snapwebsites.BookkeepingAddClientInstance = null; // static


/** \brief The server access object.
 *
 * This variable represents the server access used to send an AJAX
 * request once we create the new page.
 *
 * @type {snapwebsites.ServerAccess}
 * @private
 */
snapwebsites.BookkeepingAddClient.prototype.serverAccess_ = null;


/** \brief Close the Add Client popup window.
 *
 * This function is used to close the Add Client popup window.
 * The popup definition is found in the BookkeepingClient object.
 *
 * @private
 */
snapwebsites.BookkeepingAddClient.prototype.closeAddClientPopup_ = function()
{
    window.parent.snapwebsites.PopupInstance.hide(window.parent.snapwebsites.BookkeepingClient.addClientPopup_);
};



// auto-initialize
jQuery(document).ready(
    function()
    {
        snapwebsites.BookkeepingAddClientInstance = new snapwebsites.BookkeepingAddClient();
    }
);

// vim: ts=4 sw=4 et
