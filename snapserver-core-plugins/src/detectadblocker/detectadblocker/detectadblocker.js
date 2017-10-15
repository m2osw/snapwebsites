/** @preserve
 * Name: detectadblocker
 * Version: 0.0.5
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
// @js plugins/server_access/server-access.js
// ==/ClosureCompiler==
//

/*jslint nomen: true, todo: true, devel: true */
/*global jQuery: false, Uint8Array: true */



/** \brief Detect Ad Blocker constructor.
 *
 * The DetectAdBlocker class is used as a singleton handling the detection
 * of ad blockers. It allows other plugins to detect whether the client
 * has an active ad blocker and if so to not bother showing any ads.
 *
 * Other plugins should call the hasAdBlocker() function as in:
 *
 * \code
 *  if(snapwebsites.DetectAdBlockerInstance.hasAdBlocker())
 *  {
 *    return; // do nothing
 *  }
 *  // otherwise run your ad code
 *  ...
 * \endcode
 *
 * @return {!snapwebsites.DetectAdBlocker}  This object reference.
 *
 * @constructor
 * @struct
 */
snapwebsites.DetectAdBlocker = function()
{
    var that = this;

    // administrator wants ad blocker to tell whether user blocked
    // the ads?
    //
    if(detectadblocker__inform_server)
    {
        setTimeout(function()
            {
                that.informAboutBlocker_();
            }, 1000);
    }

    return this;
};


/** \brief DetectAdBlocker derives from ServerAccessCallbacks.
 *
 * The DetectAdBlocker class inherit the ServerAccessCallbacks class
 * so it knows when an AJAX request succeeded.
 */
snapwebsites.inherits(snapwebsites.DetectAdBlocker, snapwebsites.ServerAccessCallbacks);


/** \brief The DetectAdBlocker instance.
 *
 * This class is a singleton and as such it makes use of a static
 * reference to itself. It gets created on load.
 *
 * @type {snapwebsites.DetectAdBlocker}
 */
snapwebsites.DetectAdBlockerInstance = null; // static


/** \brief Whether the ad blocker is present (true) or not (false).
 *
 * This variable is true by default, meaning that we think an ad blocker
 * is active on that client's website.
 *
 * The page will attempt to load the adframe.js script. If that fails,
 * then this variable remains set to true. If the load succeeds, then
 * the variable will be changed to false, meaning that we can display
 * ads on that page.
 *
 * @type {snapwebsites.boolean}
 */
snapwebsites.DetectAdBlocker.present = true;


/** \brief The server access object.
 *
 * This variable represents the server access object used to listen
 * for a reply to our AJAX call.
 *
 * If the administrator turns on the capability, the DetectAdBlocker
 * class sends a quick AJAX message to the server so it is able to
 * count the number of people who have an ad blocker and the number
 * without an ad blocker.
 *
 * @type {snapwebsites.ServerAccess}
 * @private
 */
snapwebsites.DetectAdBlocker.prototype.serverAccess_ = null;


/** \brief Force a reload of the calendar.
 *
 * After one closes the Day popup, we want to reload the calendar
 * so that way we can make sure that changes to the data are
 * reflected in the calendar.
 *
 * This function is static because it is used in the popup definition
 * which requires it to be static. It calls the non-static version.
 *
 * @private
 */
snapwebsites.DetectAdBlocker.prototype.informAboutBlocker_ = function()
{
    if(!this.serverAccess_)
    {
        this.serverAccess_ = new snapwebsites.ServerAccess(this);
    }

    // now we know whether the client browser has an ad blocker running
    // or not, send the information to the server with AJAX...
    //
    this.serverAccess_.setURI("/detectadblocker/" + (snapwebsites.DetectAdBlocker.present ? "true" : "false"));
    this.serverAccess_.setMethod("GET");
    this.serverAccess_.send();
};



// auto-initialize
jQuery(document).ready(
    function()
    {
        snapwebsites.DetectAdBlockerInstance = new snapwebsites.DetectAdBlocker();
    }
);

// vim: ts=4 sw=4 et
