/** @preserve
 * Name: sortable-widgets
 * Version: 0.0.1.25
 * Browsers: all
 * Depends: editor (>= 0.0.3.468), sortable (>= 1.4.2)
 * Copyright: Copyright 2016-2017 (c) Made to Order Software Corporation  All rights reverved.
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
// @externs plugins/sortable/externs/sortable.js
// @js plugins/output/output.js
// @js plugins/output/popup.js
// @js plugins/server_access/server-access.js
// @js plugins/listener/listener.js
// @js plugins/editor/editor.js
// ==/ClosureCompiler==
//
// This is not required and it may not exist at the time you run the
// JS compiler against this file (it gets generated)
// --js plugins/mimetype/mimetype-basics.js
//

/*
 * JSLint options are defined online at:
 *    http://www.jshint.com/docs/options/
 */
/*jslint nomen: true, todo: true, devel: true */
/*global snapwebsites: false, jQuery: false, FileReader: true, Blob: true */


/** \file
 * \brief Extension to the editor: Sortable
 *
 * This file is used to extend the editor with a sortable which gives
 * the end user the ability to sort a list of items, tabs, etc.
 *
 * On the screen, the result is sorted immediately. Database wise,
 * however, nothing gets sorted. The result of the widget is a set
 * of widget names (the items of the list are expected to be widgets
 * themselves) which are defined in the order which the user chose.
 *
 * \code
 *      .
 *      .
 *      .
 *      |
 *  +---+--------------------+
 *  |                        |
 *  |  EditorWidgetType      |
 *  |  (cannot instantiate)  |
 *  +------------------------+
 *      ^
 *      |  Inherit
 *      |
 *  +---+-----------------------+
 *  |                           |
 *  | EditorWidgetTypeSortable  |
 *  |                           |
 *  +---------------------------+
 * \endcode
 */



/** \brief Editor widget type for Sortable widgets.
 *
 * This widget defines the sortable edit in the editor forms.
 *
 * @return {!snapwebsites.EditorWidgetTypeSortable}
 *
 * @constructor
 * @extends {snapwebsites.EditorWidgetType}
 * @struct
 */
snapwebsites.EditorWidgetTypeSortable = function()
{
    snapwebsites.EditorWidgetTypeSortable.superClass_.constructor.call(this);

    return this;
};


/** \brief Chain up the extension.
 *
 * This is the chain between this class and its super.
 */
snapwebsites.inherits(snapwebsites.EditorWidgetTypeSortable, snapwebsites.EditorWidgetType);


/** \brief Return "sortable".
 *
 * Return the name of the widget type: "sortable".
 *
 * @return {string} The name of the widget type.
 * @override
 */
snapwebsites.EditorWidgetTypeSortable.prototype.getType = function()
{
    return "sortable";
};


/** \brief Initialize the widget.
 *
 * This function initializes the sortable widget.
 *
 * The sortable widget allows for the end user to sort a set of items in
 * a list. The container and the items are to be defined in your parser.xsl
 * file and connected with using the right name in the widget.
 *
 * @param {!Object} widget  The widget being initialized.
 * @override
 */
snapwebsites.EditorWidgetTypeSortable.prototype.initializeWidget = function(widget) // virtual
{
    var that = this,
        editor_widget = /** @type {snapwebsites.EditorWidget} */ (widget),
        c = editor_widget.getWidgetContent(),
        container = jQuery("." + c.data("container"));

    snapwebsites.EditorWidgetTypeSortable.superClass_.initializeWidget.call(this, widget);

    // we want a class that we know of within the sortable plugin
    container
        .addClass("snap-sortable-container")
        .children()
        .addClass("snap-sortable-items");

    Sortable.create(
            /** @type {Element} */ (container.get(0)),
            {
                onSort: function(e)
                    {
                        var list = container.children().map(function()
                            {
                                return jQuery(this).data("item-name");
                            }).get().join(",");

                        editor_widget.setValue(list, true);
                    }
            });
};


/** \brief Save a new value in the specified editor widget.
 *
 * This function offers a way for programmers to dynamically change the
 * value of a dropdown widget. You should never call the editor widget
 * type function, instead use the setValue() function of the widget you
 * want to change the value of (it will make sure that the modified
 * flag is properly set.)
 *
 * For a dropdown, the value parameter is expected to be one of the strings
 * found in the list of items or a number. If it is a number, then that
 * item gets selected (we use floor() to round any number down). If it
 * is a string it searches the list of items for it and selects that item.
 * If two or more items have the same label, then the first one is selected.
 *
 * If the index number is too large or the specified string is not found
 * in the existing items, then nothing happens.
 *
 * \todo
 * Add support for disabled value which cannot be selected.
 *
 * @param {!Object} widget  The concerned widget.
 * @param {!Object|string|number} value  The value to be saved.
 *
 * @return {boolean}  true if the value gets changed.
 *
 * \sa resetValue()
 */
snapwebsites.EditorWidgetTypeSortable.prototype.setValue = function(widget, value)
{
    var editor_widget = /** @type {snapwebsites.EditorWidget} */ (widget),
        w = editor_widget.getWidget(),
        c = editor_widget.getWidgetContent();

    if(c.attr("value") != value)
    {
        c.attr("value", snapwebsites.castToString(value, "sortable value attribute"));

        editor_widget.getEditorBase().checkModified(editor_widget);

        editor_widget.getEditorForm().changed();

        return true;
    }

    return false;
};



// auto-initialize
jQuery(document).ready(function()
    {
        snapwebsites.EditorInstance.registerWidgetType(new snapwebsites.EditorWidgetTypeSortable());
    });

// vim: ts=4 sw=4 et
