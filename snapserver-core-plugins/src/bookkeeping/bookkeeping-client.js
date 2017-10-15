/** @preserve
 * Name: bookkeeping-client
 * Version: 0.0.1.3
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
// ==/ClosureCompiler==
//

/*jslint nomen: true, todo: true, devel: true */
/*global jQuery: false, Uint8Array: true */



/** \brief Bookkeeping constructor.
 *
 * The Bookkeeping Client is a singleton handling the Bookkeeping
 * Client window which has an Add Client button.
 *
 * @return {snapwebsites.BookkeepingClient}  The newly created object.
 *
 * @constructor
 * @struct
 */
snapwebsites.BookkeepingClient = function()
{
    jQuery(".bookkeeping-buttons .bookkeeping-button.add-client")
        .makeButton()
        .click(function(e){
            e.preventDefault();
            e.stopPropagation();

            snapwebsites.PopupInstance.open(snapwebsites.BookkeepingClient.addClientPopup_);
            snapwebsites.PopupInstance.show(snapwebsites.BookkeepingClient.addClientPopup_);
        });

    return this;
};


/** \brief BookkeepingClient is a base class.
 *
 * The BookkeepingClient class is a base class.
 */
snapwebsites.base(snapwebsites.BookkeepingClient);


/** \brief The BookkeepingClient instance.
 *
 * This class is a singleton and as such it makes use of a static
 * reference to itself. It gets created on load.
 *
 * \@type {snapwebsites.BookkeepingClient}
 */
snapwebsites.BookkeepingClientInstance = null; // static


/** \brief The Time Tracker "Add User" popup window.
 *
 * This variable is used to describe the Time Tracker popup used when
 * the administrator wants to add a user to the Time Tracker system.
 *
 * \note
 * We have exactly ONE instance of this variable (i.e. it is static).
 * This means we cannot open two such popups simultaneously.
 *
 * @type {Object}
 * @private
 */
snapwebsites.BookkeepingClient.addClientPopup_ = // static
{
    id: "bookkeeping-add-client-popup",
    title: "New Client",
    path: "/bookkeeping/client/add-client?theme=notheme",
    darken: 150,
    width: 500,
    beforeHide: snapwebsites.EditorForm.beforeHide,
    editorFormName: "bookkeeping_add_client"
};



// auto-initialize
jQuery(document).ready(
    function()
    {
        snapwebsites.BookkeepingClientInstance = new snapwebsites.BookkeepingClient();
    }
);

// vim: ts=4 sw=4 et
