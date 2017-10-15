/** @preserve
 * Name: list
 * Version: 0.0.5
 * Browsers: all
 * Depends: ...
 * Copyright: Copyright 2013-2017 (c) Made to Order Software Corporation  All rights reverved.
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
/*global snapwebsites: false, jQuery: false */



/** \brief Snap Output Manipulations.
 *
 * This class initializes and handles the different output objects.
 *
 * \note
 * The Snap! Output is a singleton and should never be created by you. It
 * gets initialized automatically when this output.js file gets included.
 *
 * @return {!snapwebsites.List}  This object once initialized.
 *
 * @constructor
 * @struct
 */
snapwebsites.List = function()
{
    this.constructor = snapwebsites.List;

    return this;
};


/** \brief Mark List as a base class.
 *
 * This class does not inherit from any other classes.
 */
snapwebsites.base(snapwebsites.List);


// auto-initialize
jQuery(document).ready(
    function()
    {
        snapwebsites.ListInstance = new snapwebsites.List();
    }
);

// vim: ts=4 sw=4 et
