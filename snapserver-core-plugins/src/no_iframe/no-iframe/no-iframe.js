/** @preserve
 * Name: no-iframe
 * Version: 0.0.1.3
 * Browsers: all
 * Copyright: Copyright 2017 (c) Made to Order Software Corporation  All rights reverved.
 * Depends: jquery (1.11)
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
// ==/ClosureCompiler==
//

/*jslint nomen: true, todo: true, devel: true */
/*global jQuery: false, Uint8Array: true */




// auto-initialize
jQuery(document).ready(
    function()
    {
        if(window.self != window.top)
        {
            window.top.location = window.location;
        }
    }
);

// vim: ts=4 sw=4 et
