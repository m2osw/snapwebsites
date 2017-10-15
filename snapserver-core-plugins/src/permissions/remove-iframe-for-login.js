/** @preserve
 * Name: remove-iframe-for-login
 * Version: 0.0.1.8
 * Browsers: all
 * Copyright: Copyright 2014-2017 (c) Made to Order Software Corporation  All rights reverved.
 * Depends: output (0.1.5.70)
 * License: GPL 2.0
 */


//
// Inline "command line" parameters for the Google Closure Compiler
// See output of:
//    java -jar .../google-js-compiler/compiler.jar --help
//
// ==ClosureCompiler==
// @compilation_level ADVANCED_OPTIMIZATIONS
// @externs $CLOSURE_COMPILER/contrib/externs/jquery-1.9.js
// @externs plugins/output/externs/jquery-extensions.js
// @js plugins/output/output.js
// ==/ClosureCompiler==
//

/*jslint nomen: true, todo: true, devel: true */
/*global jQuery: false, Uint8Array: true */

// auto-initialize
jQuery(document).ready(
    function()
    {
        // TODO: should we use window.parent or window.top?
        var doc = window.parent.document;

        //snapwebsites.PopupInstance = new snapwebsites.Popup();
        //console.log("P = [" + jQuery("html").attr("class") + "]");
        //console.log("parent = [" + jQuery("html", doc).attr("class") + "]");
        doc.location = "/login";
//console.log("we got here already!");
    }
);

// vim: ts=4 sw=4 et
