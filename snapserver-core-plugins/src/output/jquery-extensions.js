/*
 * Name: jquery-extensions
 * Version: 1.0.2
 * Browsers: all
 * Copyright: Copyright 2014-2017 (c) Made to Order Software Corporation  All rights reverved.
 * Depends: jquery (1.10)
 * License: GPL 2.0
 */

//
// Inline "command line" parameters for the Google Closure Compiler
// See output of:
//    java -jar .../google-js-compiler/compiler.jar --help
//
// ==ClosureCompiler==
// @compilation_level ADVANCED_OPTIMIZATIONS
// @js $CLOSURE_COMPILER/contrib/externs/jquery-1.9.js
// ==/ClosureCompiler==
//


/** \brief Objects that match any one of the selectors.
 *
 * Usage: $(":any(\<selectors\>)")
 *
 * This is particularly useful when you want to apply a function against
 * another, so for example you could apply a not to all the objects that
 * match the any:
 *
 * \code
 * $("span:any(\<selectors\>):not(\<selector\>)")
 * \endcode
 *
 * It makes it a little easier to handle the :not() without having to
 * repeat it many times over.
 *
 * \note
 * The :any(...) selector is an equivalent to an OR. By default selectors
 * must all be present (i.e. an AND function.)
 *
 * \source
 * http://stackoverflow.com/questions/14525758/how-to-use-a-not-selector-with-multiple-span-classes
 *
 * \param[in] selector  A list of selectors, all matches get returned.
 *
 * \return The list of matched DOM tags.
 */
jQuery.expr[':'].any = jQuery.expr.createPseudo(function(selector){
    return function(el){
        return jQuery.find.matches(selector, [el]).length > 0;
    }
});


jQuery.fn.extend({
    /** \brief Function to find the largest z-index in a group of div.
     *
     * This function applies a search on the current list of DOM items
     * and returns the maximum z-index value found.
     *
     * Usage:
     *
     * \code
     * var maxz;
     *
     * maxz = $("<selector>").maxZIndex();
     * \endcode
     *
     * \note
     * In Snap! the `<selector>` is always set to `".zordered"`.
     *
     * This function should be used to assign a z-index to your popup windows
     * instead of randomly assigning numbers and hoping everything appears in
     * the correct order.
     *
     * The function always returns a valid integer. If it finds no valid
     * z-index, it returns zero (0).
     *
     * Source: http://stackoverflow.com/questions/5680770/jquery-find-the-highest-z-index
     *
     * \return The largest z-index found within the DOM items found in
     *         \<selector\> elements.
     */
    maxZIndex: function()
    {
        return Math.max.apply(null, jQuery(this).map(
            function()
            {
                var z;

                z = jQuery(this).css("z-index");
                if(z == "auto")
                {
                    return 0;
                }
                return parseInt(z, 10);
            }));
    },

    /** \brief Check whether this jQuery object has items.
     *
     * This function returns true if the specified jQuery object has
     * items attached to it.
     *
     * For example:
     *
     * \code
     *      var items = jQuery("<selector>");
     *
     *      if(!items.exists())
     *      {
     *          alert("items missing...");
     *          return;
     *      }
     * \endcode
     *
     * @return {boolean}  true if the jQuery object references at least one element.
     */
    exists: function()
    {
        return this.length !== 0;
    }
});

// vim: ts=4 sw=4 et
