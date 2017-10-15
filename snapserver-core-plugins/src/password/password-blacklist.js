/** @preserve
 * Name: password-blacklist
 * Version: 0.0.1.6
 * Browsers: all
 * Depends: editor (>= 0.0.3.468)
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
 * \brief Handling of the Blacklist Manager.
 *
 * This script handles the Blacklist Manager window.
 */



/** \brief Handling of the password blacklist manager widgets.
 *
 * This class registers the various buttons to apply their function
 * with an AJAX call. All the functions are AJAX and we do not save
 * this form in any way as a usual editor form.
 *
 * @return {!snapwebsites.PasswordBlacklist}
 *
 * @constructor
 * @extends {snapwebsites.ServerAccessCallbacks}
 * @struct
 */
snapwebsites.PasswordBlacklist = function()
{
    var that = this,
        editor = snapwebsites.EditorInstance,
        blacklist_form = editor.getFormByName("blacklist"),
        save_dialog = blacklist_form.getSaveDialog(),
        save_button,
        check_password_button = jQuery(".check-password"),
        add_passwords_button = jQuery(".add-passwords"),
        remove_passwords_button = jQuery(".remove-passwords");

    snapwebsites.PasswordBlacklist.superClass_.constructor.call(this);

    this.serverAccess_ = new snapwebsites.ServerAccess(this);

    // disable the default Save functionality, we do not want it here
    // TBD: at this time I think the only way I have to do that is to
    //      create a hidden button...
    jQuery("body").append("<div class='hidden-save-dialog' style='display: none;'><a class='unused-save-button'>Save</a></div>");
    save_button = jQuery("a.unused-save-button");
    save_dialog.setPopup(save_button);

    check_password_button
        .makeButton()
        .click(function(e)
            {
                // avoid the '#' from appearing in the URI
                e.preventDefault();

                that.CheckPassword_();
            });

    add_passwords_button
        .makeButton()
        .click(function(e)
            {
                // avoid the '#' from appearing in the URI
                e.preventDefault();

                that.AddPasswords_();
            });

    remove_passwords_button
        .makeButton()
        .click(function(e)
            {
                // avoid the '#' from appearing in the URI
                e.preventDefault();

                that.RemovePasswords_();
            });

    return this;
};


/** \brief The PasswordBlacklist inherits from ServerAccessCallbacks.
 *
 * The password blacklist makes use of AJAX so it inherits
 * from the ServerAccessCallbacks interface.
 */
snapwebsites.inherits(snapwebsites.PasswordBlacklist, snapwebsites.ServerAccessCallbacks);


/** \brief The PasswordBlacklist instance.
 *
 * This variable holds the instance of the PasswordBlacklist we create
 * on startup. The PasswordBlacklist is a singleton and as such is
 * created only once and saved here.
 */
snapwebsites.PasswordBlacklistInstance_ = null


/** \brief The ServerAccess object used to send data to the server.
 *
 * This variable holds the ServerAccess object used by the
 * PasswordBlacklist object. It is created once in the constructor.
 *
 * @type {snapwebsites.ServerAccess}
 * @private
 */
snapwebsites.PasswordBlacklist.prototype.serverAccess_ = null;


/** \brief Check a password against the exiting blacklisted passwords.
 *
 * This function sends an AJAX command to the server to see whether
 * the password specified in the box above the button is a password
 * that was previously blacklisted.
 *
 * @private
 */
snapwebsites.PasswordBlacklist.prototype.CheckPassword_ = function()
{
    var editor = snapwebsites.EditorInstance,
        blacklist_form = editor.getFormByName("blacklist"),
        check_password_widget = blacklist_form.getWidgetByName("check_password");

    this.serverAccess_.setURI("/admin/settings/password/blacklist");
    this.serverAccess_.setData(
        {
            password_function: "is_password_blacklisted",
            password: check_password_widget.getValue()
        });
    this.serverAccess_.send();
};


/** \brief User clicked "Add Passwords"
 *
 * This function sends passwords to be added to the list of
 * blacklisted passwords.
 *
 * @private
 */
snapwebsites.PasswordBlacklist.prototype.AddPasswords_ = function()
{
    var editor = snapwebsites.EditorInstance,
        blacklist_form = editor.getFormByName("blacklist"),
        new_passwords_widget = blacklist_form.getWidgetByName("new_passwords");

    this.serverAccess_.setURI("/admin/settings/password/blacklist");
    this.serverAccess_.setData(
        {
            password_function: "blacklist_new_passwords",
            password: new_passwords_widget.getValue()
        });
    this.serverAccess_.send();
};


/** \brief User clicked "Remove Passwords"
 *
 * This function sends passwords to be removed from the list of
 * blacklisted passwords.
 *
 * @private
 */
snapwebsites.PasswordBlacklist.prototype.RemovePasswords_ = function()
{
    var editor = snapwebsites.EditorInstance,
        blacklist_form = editor.getFormByName("blacklist"),
        remove_passwords_widget = blacklist_form.getWidgetByName("remove_passwords");

    this.serverAccess_.setURI("/admin/settings/password/blacklist");
    this.serverAccess_.setData(
        {
            password_function: "blacklist_remove_passwords",
            password: remove_passwords_widget.getValue()
        });
    this.serverAccess_.send();
};



// auto-initialize
jQuery(document).ready(function()
    {
        snapwebsites.PasswordBlacklistInstance = new snapwebsites.PasswordBlacklist();
    });

// vim: ts=4 sw=4 et

