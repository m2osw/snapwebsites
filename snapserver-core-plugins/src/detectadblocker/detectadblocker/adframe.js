/** @preserve
 * Name: adframe
 * Version: 0.0.1
 * Browsers: all
 * Copyright: Copyright 2016-2017 (c) Made to Order Software Corporation  All rights reverved.
 * Depends: detectadblocker (0.0.1)
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
// @js plugins/detectadblocker/detectadblocker.js
// ==/ClosureCompiler==
//

/*jslint nomen: true, todo: true, devel: true */
/*global jQuery: false, Uint8Array: true */


/** \brief Change the 'present' variable value to false.
 *
 * This variable gets redefined with the value false instead of the
 * default of true if this file gets loaded.
 *
 * This is the actual detection of the ad blocker. That is, this script
 * should be blocked (and thus not loaded) when ads are blocked by
 * an ad on. The side effect is that the 'present' variable value
 * does not get changed to false, meaning that the blocker is not
 * present (or at least not active against that website.)
 *
 * @type {boolean}
 */
snapwebsites.DetectAdBlocker.present = false;


// vim: ts=4 sw=4 et
