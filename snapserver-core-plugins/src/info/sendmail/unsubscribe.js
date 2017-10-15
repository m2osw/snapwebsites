/** @preserve
 * Name: unsubscribe
 * Version: 0.0.14
 * Browsers: all
 * Depends: editor (>= 0.0.3.107)
 * Copyright: Copyright 2012-2017 (c) Made to Order Software Corporation  All rights reverved.
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
// @js plugins/output/popup.js
// @js plugins/server_access/server-access.js
// @js plugins/listener/listener.js
// @js plugins/editor/editor.js
// ==/ClosureCompiler==
//

/*jslint nomen: true, todo: true, devel: true */
/*global snapwebsites: false, jQuery: false */



/** \brief Unsubscribe form specialized handling.
 *
 * This class adds functionality to the unsubscribe form such as
 * skipping on the default Editor Save popup dialog.
 *
 * @return {!snapwebsites.Unsubscribe}  This object reference.
 *
 * @constructor
 * @struct
 */
snapwebsites.Unsubscribe = function()
{
    var editor = snapwebsites.EditorInstance,
        unsubscribe_form = editor.getFormByName("unsubscribe"),
        save_dialog = unsubscribe_form.getSaveDialog(),
        save_button = jQuery(".buttons a.save-button"),
        that = this;

    // replace Save Dialog with our Save Button
    save_dialog.setPopup(save_button, false);
    unsubscribe_form.setSaveFunctionOnSuccess(
            function(unsubscribe_form, result)
            {
                that.onSaved(unsubscribe_form, result);
            }
        );
    save_button
        .makeButton()
        .click(function(e)
            {
                // avoid the '#' from appearing in the URI
                e.preventDefault();

                // Action is set to "submit" since this data is very likely
                // sent by an anoymous user
                //
                // TODO: the name of the object parameter for the action
                //       ("a" at this time) should be dynamically defined
                unsubscribe_form.saveData("save", {a: "submit"});
            });

    // also mark the cancel button as such
    jQuery(".buttons a.cancel-button")
        .makeButton();

    jQuery(".buttons a.unsubscribe-button")
        .makeButton()
        .click(function(e)
            {
                var editor_tab = jQuery("div.editor-form"),
                    success_tab = jQuery("div.success");

                // avoid the '#' from appearing in the URI
                e.preventDefault();

                success_tab.hide();
                editor_tab.show();
            });

    return this;
};


/** \brief Mark Unsubscirbe as a base class.
 *
 * This class does not inherit from any other classes.
 */
snapwebsites.base(snapwebsites.Unsubscribe);


/** \brief The Form instance.
 *
 * This class is a singleton and as such it makes use of a static
 * reference to itself. It gets created on load.
 *
 * @type {snapwebsites.Unsubscribe}
 */
snapwebsites.UnsubscribeInstance = null; // static


/** \brief Function called on a successful save.
 *
 * Whenever we save the form, the server replies positively whenever the
 * save succeeds and then this function gets called. This gives us the
 * opportunity to update the output.
 *
 * @param {snapwebsites.EditorForm} settings_form  The form that was
 *                                             successfully saved.
 * @param {snapwebsites.ServerAccessCallbacks.ResultData} result  The form
 *                                                                result.
 */
snapwebsites.Unsubscribe.prototype.onSaved = function(settings_form, result)
{
    var editor_tab = jQuery("div.editor-form"),
        success_tab = jQuery("div.success"),
        editor = snapwebsites.EditorInstance,
        unsubscribe_form = editor.getFormByName("unsubscribe"),
        email_widget = unsubscribe_form.getWidgetByName("email");

    editor_tab.hide();
    success_tab.show();
    email_widget.setValue("", false);
};



// auto-initialize
jQuery(document).ready(
    function()
    {
        snapwebsites.UnsubscribeInstance = new snapwebsites.Unsubscribe();
    }
);
// vim: ts=4 sw=4 et
