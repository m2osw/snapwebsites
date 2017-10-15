/** @preserve
 * Name: password
 * Version: 0.0.1.25
 * Browsers: all
 * Depends: editor (>= 0.0.3.468), javascript-unicode (>= 0.0.1)
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
// @js plugins/password/externs/password.js
// @js plugins/output/externs/javascript-unicode.js
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
 * \brief Extension to the editor: Password and Password Confrim Widget
 *
 * This file is used to extend the editor with a password widget and a
 * password plug confirm widget used to let the clients enter passwords.
 *
 * \code
 *      .
 *      .
 *      .
 *      |
 *  +---+--------------------+  Inherit
 *  |                        |<---------------+
 *  |  EditorWidgetType      |                |
 *  |  (cannot instantiate)  |                |
 *  +------------------------+                |
 *      ^                                     |
 *      |  Inherit                            |
 *      .                                     |
 *      .                                     |
 *      .                                     |
 *      |                                     |
 *  +---------------------------+             |
 *  |                           |             |
 *  | EditorWidgetTypeLineEdit  |             |
 *  |                           |             |
 *  +---------------------------+             |
 *      ^                                     |
 *      | Inherit                             |
 *      |                                     |
 *  +---+-----------------------+         +---+------------------------------+
 *  |                           |  Ref.   |                                  |
 *  | EditorWidgetTypePassword  |<--------| EditorWidgetTypePasswordConfirm  |
 *  |                           |         |                                  |
 *  +---------------------------+         +----------------------------------+
 * \endcode
 */



/** \brief Editor widget type for Password widgets.
 *
 * This widget defines the password edit in the editor forms. This is
 * an equivalent to the input of type password of a standard form.
 *
 * The class extends the line edit as we expect that password will
 * not be required in a full text area.
 *
 * @return {!snapwebsites.EditorWidgetTypePassword}
 *
 * @constructor
 * @extends {snapwebsites.EditorWidgetTypeLineEdit}
 * @struct
 */
snapwebsites.EditorWidgetTypePassword = function()
{
    snapwebsites.EditorWidgetTypePassword.superClass_.constructor.call(this);

    return this;
};


/** \brief Chain up the extension.
 *
 * This is the chain between this class and its super.
 */
snapwebsites.inherits(snapwebsites.EditorWidgetTypePassword, snapwebsites.EditorWidgetTypeLineEdit);


/** \brief Return "password".
 *
 * Return the name of the widget type: "password".
 *
 * @return {string} The name of the widget type.
 * @override
 */
snapwebsites.EditorWidgetTypePassword.prototype.getType = function()
{
    return "password";
};


/** \brief Initialize the widget.
 *
 * This function initializes the password widget.
 *
 * The password widget hijack the keyboard to make sure that characters
 * do not show up in the widget output. The output may be updated with
 * bullets or some other character or not updated at all.
 *
 * @param {!Object} widget  The widget being initialized.
 * @override
 */
snapwebsites.EditorWidgetTypePassword.prototype.initializeWidget = function(widget) // virtual
{
    var that = this,
        editor_widget = /** @type {snapwebsites.EditorWidget} */ (widget),
        c = editor_widget.getWidgetContent();

    snapwebsites.EditorWidgetTypePassword.superClass_.initializeWidget.call(this, widget);

    c.keydown(function(e)
        {
            // ignore controls and Alt-Key, but any other key needs to be
            // transformed in a "bullet" before it gets added to the widget
            // or even add nothing to the widget output (only keep the data in
            // the internal hidden buffer.)
            //
//console.log("e.which = " + e.which);
            if(!e.ctrlKey
            && !e.altKey
            && e.which != 144                     // NumLock
            && (e.which == 32 || e.which >= 48)
            && (e.which < 112 || e.which > 123))  // avoid F1 to F12
            {
                // prevent the default behavior
                //
                e.preventDefault();

                that.showBullets_(editor_widget, e.key, 0);
            }
            else if(e.which == 8 || e.which == 46)
            {
                // prevent the default behavior whatever the control
                // keys held!
                //
                e.preventDefault();

                // 8 is backspace
                // 46 is delete
                that.deleteBullets_(editor_widget, e.which == 8, e.ctrlKey);
            }
        });

    c.on("paste", function(e)
        {
            var clipboard,
                paste_text;

            // make sure we prevent the default otherwise people watching
            // the screen could have a glimps of the password being
            // pasted...
            //
            e.preventDefault();

            // allow the paste?
            if(!c.hasClass("no-paste"))
            {
                // there are three possible locations for the clipboard
                // data so we check them all in a specific order which
                // seems to be the one most people use
                //
                clipboard = e.originalEvent && e.originalEvent.clipboardData
                                    ? e.originalEvent.clipboardData
                                    : e.clipboardData
                                        ? e.clipboardData
                                        : window.clipboardData;

                if(clipboard)
                {
                    // TODO: add support for HTML clipboard? (or is the clipboard
                    //       smart enough to offer plain text when the data is HTML?)
                    //
                    paste_text = clipboard.getData("text/plain");

                    that.showBullets_(editor_widget, paste_text, 0);
                }
            }
        });
};


/** \brief Show the proper number of bullets.
 *
 * This function counts the number of characters in the password and
 * reflects that number in the edited widget using bullets.
 *
 * @param {snapwebsites.EditorWidget} editor_widget  The widget to update.
 * @param {string} text  The text to insert.
 * @param {number} adjust_position  Added to position (helps with Backspace.)
 * @private
 */
snapwebsites.EditorWidgetTypePassword.prototype.showBullets_ = function(editor_widget, text, adjust_position)
{
    var c = editor_widget.getWidgetContent(),
        i,
        sel,
        range,
        container,
        password,
        len,
        hidden_password = "",
        bullets = "\u2B24",
        new_pos;

    if(document.selection)
    {
        sel = document.selection;
        range = sel.createRange();
    }
    else
    {
        sel = window.getSelection();
        if(sel.getRangeAt)
        {
            range = sel.getRangeAt(0);
        }
        else
        {
            // else... what to do, really?!?
            return;
        }
    }

    // get the new position before we change the box HTML
    new_pos = range.startOffset + text.length + adjust_position;

    password = c.attr("value");
    if(password)
    {
        password = password.substr(0, range.startOffset) + text + password.substr(range.endOffset);
    }
    else
    {
        // when the value is undefined we get here
        password = text;
    }
    c.attr("value", snapwebsites.castToString(password, "password value attribute"));

    // repeat bullets password.length times
    // TODO: add other options (i.e. show a "length bar")
    //       do not show anything other than the fact that something is being typed
    len = password.length;
    for(i = 1; i <= len; i *= 2)
    {
        if((i & len) === i)
        {
            hidden_password += bullets;
        }
        bullets += bullets;
    }
    c.html(hidden_password);

    //new_range = document.createRange();
    container = c.get(0);
    if(container.firstChild)
    {
        container = container.firstChild;

        range.setEnd(container, new_pos);
        range.setStart(container, new_pos);

        sel.removeAllRanges();
        sel.addRange( /** @type {Range} */ (range));
    }

// for debug purposes, to make sure the password is as expected:
//console.log("password = [" + password + "] -> len " + len + " new pos still = " + new_pos);
};


/** \brief Delete a set of characters.
 *
 * This function is called whenever the Delete or Backspace keys
 * are pressed. We completely rewrite these because otherwise
 * the system default would take over and we could easilly get
 * unsynchronized.
 *
 * @param {snapwebsites.EditorWidget} editor_widget  The widget to update.
 * @param {boolean} backspace  If true, this was backspace, otherwise it was delete.
 * @param {boolean} control  If true the control key was being held down.
 * @private
 */
snapwebsites.EditorWidgetTypePassword.prototype.deleteBullets_ = function(editor_widget, backspace, control)
{
    var c = editor_widget.getWidgetContent(),
        sel,
        range,
        password,
        original_password,
        adjust_position = 0;

    if(document.selection)
    {
        sel = document.selection;
        range = sel.createRange();
    }
    else
    {
        sel = window.getSelection();
        if(sel.getRangeAt)
        {
            range = sel.getRangeAt(0);
        }
        else
        {
            // else... what to do, really?!?
            return;
        }
    }

    // since we only have plain text, if equal there is no selection
    password = /** @type {string} */ (c.attr("value"));
    if(password)
    {
        // there is password so there may be something to delete
        //
        if(range.startOffset == range.endOffset)
        {
            original_password = password;
            if(backspace)
            {
                if(range.startOffset > 0)
                {
                    if(control)
                    {
                        adjust_position = -range.endOffset;
                        password = password.substr(range.endOffset);
                    }
                    else
                    {
                        adjust_position = -1;
                        password = password.substr(0, range.startOffset - 1) + password.substr(range.endOffset);
                    }
                }
            }
            else
            {
                if(range.endOffset < password.length)
                {
                    if(control)
                    {
                        password = password.substr(0, range.startOffset);
                    }
                    else
                    {
                        password = password.substr(0, range.startOffset) + password.substr(range.endOffset + 1);
                    }
                }
            }

            // was it modified?
            if(original_password != password)
            {
                // yes, then update the result and view
                c.attr("value", password);
                this.showBullets_(editor_widget, "", adjust_position);
            }
        }
        else
        {
            // there is a range selection, the showBullets_() function
            // already removes that selection by itself so we have
            // nothing to here here!
            //
            this.showBullets_(editor_widget, "", adjust_position);
        }
    }
};



/** \brief Editor widget type for the password/confirm.
 *
 * This class handles the password/confirm widget. Mainly it ensures
 * that the password and confirmation fields are both equal because
 * the widget only sends one password to the server (no need to send
 * both, really!)
 *
 * \code
 *  class EditorWidgetTypePasswordConfirm extends EditorWidgetType
 *  {
 *  public:
 *      EditorWidgetTypePasswordConfirm() : snapwebsites.EditorWidget;
 *
 *      virtual function getType() : string;
 *      virtual function initializeWidget(widget: Object) : void;
 *      virtual function validate(editor_widget: Object) : boolean;
 *
 *  private:
 *  };
 * \endcode
 *
 * @constructor
 * @extends {snapwebsites.EditorWidgetType}
 * @struct
 */
snapwebsites.EditorWidgetTypePasswordConfirm = function()
{
    snapwebsites.EditorWidgetTypePasswordConfirm.superClass_.constructor.call(this);

    return this;
};


/** \brief Chain up the extension.
 *
 * This is the chain between this class and its super.
 */
snapwebsites.inherits(snapwebsites.EditorWidgetTypePasswordConfirm, snapwebsites.EditorWidgetType);


/** \brief Return "password_confirm".
 *
 * Return the name of the password/confirm widget type.
 *
 * @return {string} The name of the password confirm widget type.
 * @override
 */
snapwebsites.EditorWidgetTypePasswordConfirm.prototype.getType = function() // virtual
{
    return "password_confirm";
};


/** \brief Initialize the widget.
 *
 * This function initializes the password/confirm widget.
 *
 * @param {!Object} widget  The widget being initialized.
 * @override
 */
snapwebsites.EditorWidgetTypePasswordConfirm.prototype.initializeWidget = function(widget) // virtual
{
    var that = this,
        editor_widget = /** @type {snapwebsites.EditorWidget} */ (widget),
        editor_form = editor_widget.getEditorForm(),
        name = editor_widget.getName(),
        password_widget = editor_form.getWidgetByName(name + "_password"),
        password = password_widget.getWidget(),
        confirm_widget = editor_form.getWidgetByName(name + "_confirm"),
        confirm = confirm_widget.getWidget();

    snapwebsites.EditorWidgetTypePasswordConfirm.superClass_.initializeWidget.call(this, widget);

    // if the password changes, we need to compare against the confirmation
    password.bind("widgetchange", function(e)
        {
            that.comparePasswords_(editor_widget);
        });

    // if the confirmation changes, we need to compare against the password
    confirm.bind("widgetchange", function(e)
        {
            that.comparePasswords_(editor_widget);
        });
};


/** \brief Check whether the password and confirmation are equal.
 *
 * This function compares the password and confirmation to see whether
 * they are equal and if so let the user know.
 *
 * @param {snapwebsites.EditorWidget} editor_widget  The password/confirm widget.
 *
 * @private
 */
snapwebsites.EditorWidgetTypePasswordConfirm.prototype.comparePasswords_ = function(editor_widget)
{
    var editor_form = editor_widget.getEditorForm(),
        c = editor_widget.getWidgetContent(),
        name = editor_widget.getName(),
        password_widget = editor_form.getWidgetByName(name + "_password"),
        password = password_widget.getValue(),
        confirm_widget = editor_form.getWidgetByName(name + "_confirm"),
        confirm = confirm_widget.getValue(),
        msg = "",
        good = false;

    // show a message about the current status of the password + confirm
    //
    if(password.length == 0)
    {
        msg = "<li>Please enter a password twice.</li>";
        good = confirm.length == 0;
    }
    else if(confirm.length == 0)
    {
        msg = this.checkAgainstPolicy(editor_widget);
        good = msg.length == 0;
        if(good)
        {
            msg = "<li>Please make sure to enter the password confirmation.</li>";
        }
    }
    else if(password != confirm)
    {
        msg = "<li>The password and confirmation are not equal.</li>";
    }
    else
    {
        // TODO: implement the policy tests
        //
        msg = this.checkAgainstPolicy(editor_widget);
        good = msg.length == 0;
        if(good)
        {
            msg = "<li>The password is acceptable and the confirmation is equal.</li>";
        }
    }

    c.find(".password-status-details").toggleClass("good", good);
    c.find(".password-status").html(msg);

    // save the value in the main password widget
    c.attr("value", snapwebsites.castToString(password, "password value attribute"));
};


/** \brief Make sure the password and its confirmation are the same.
 *
 * This function verifies that the password and its confirmation are equal.
 * This is done before you can send the form to the server so as to prevent
 * sending an invalid password.
 *
 * @param {Object} widget  The editor widget.
 *
 * @return {boolean}  true if the passwords are both equal.
 */
snapwebsites.EditorWidgetTypePasswordConfirm.prototype.validate = function(widget) // virtual
{
    var editor_widget = /** @type {snapwebsites.EditorWidget} */ (widget),
        w = editor_widget.getWidget(),
        c = editor_widget.getWidgetContent(),
        label = "<strong>" + editor_widget.getLabel(true) + "</strong>",
        valid = c.find(".password-status-details").hasClass("good");

    if(!valid)
    {
        snapwebsites.OutputInstance.displayOneMessage(
                "Invalid Password",
                "The password and confirmation in " + label + " are not yet acceptable.",
                "error",
                true);
    }

    // make sure the "erroneous" class is as expected
    w.toggleClass("erroneous", !valid);

    return valid;
};


/** \brief Make sure the password and its confirmation are the same.
 *
 * This function verifies that the password and its confirmation are equal.
 * This is done before you can send the form to the server so as to prevent
 * sending an invalid password.
 *
 * @param {snapwebsites.EditorWidget} editor_widget  The editor widget.
 *
 * @return {string}  The message to display, empty if the password is valid.
 */
snapwebsites.EditorWidgetTypePasswordConfirm.prototype.checkAgainstPolicy = function(editor_widget) // virtual
{
    var editor_form = editor_widget.getEditorForm(),
        name = editor_widget.getName(),
        password_widget = editor_form.getWidgetByName(name + "_password"),
        password = password_widget.getValue(),
        count_lowercase_letters = 0,
        count_uppercase_letters = 0,
        count_letters = 0,
        count_digits = 0,
        count_spaces = 0,
        count_specials = 0,
        count_unicode = 0,
        i,
        c,
        msg = "",
        vc = [],
        max_variation,
        code_points = snapwebsites.stringToUnicodeCodePoints(password),
        categories = snapwebsites.stringToUnicodeCategories(password);

    // Setup variable defined by password.cpp as inline JavaScript:
    //
    // password__policy__minimum_length=...;
    // password__policy__minimum_lowercase_letters=...;
    // password__policy__minimum_uppercase_letters=...;
    // password__policy__minimum_letters=...;
    // password__policy__minimum_digits=...;
    // password__policy__minimum_spaces=...;
    // password__policy__minimum_specials=...;
    // password__policy__minimum_unicode=...;
    // password__policy__minimum_variation=...;
    // password__policy__minimum_length_of_variations=...;
    //

    if(password.length < password__policy__minimum_length)
    {
        msg += "<li>Password is still too short.</li>";
    }

    for(i = 0; i < categories.length; ++i)
    {
        switch(categories[i])
        {
        case "Ll":
        case "Lo":
            ++count_lowercase_letters;
            ++count_letters;
            break;

        case "Lu":
        case "Lt":
            ++count_uppercase_letters;
            ++count_letters;
            break;

        case "Nd":
        case "Nl":
        case "No":
            ++count_digits;
            break;

        case "Mc":
        case "Zs":
        case "Zl":
        case "Zp":
            ++count_spaces;
            ++count_specials;
            break;

        default:
            if(code_points[i] < 256)
            {
                ++count_specials;
            }
            break;

        }

        if(code_points[i] > 255)
        {
            ++count_unicode;
        }
    }

    if(count_lowercase_letters < password__policy__minimum_lowercase_letters)
    {
        msg += "<li>Password does not yet contain enough lowercase letters.</li>";
    }

    if(count_uppercase_letters < password__policy__minimum_uppercase_letters)
    {
        msg += "<li>Password does not yet contain enough uppercase letters.</li>";
    }

    if(count_letters < password__policy__minimum_letters)
    {
        msg += "<li>Password does not yet contain enough letters.</li>";
    }

    if(count_digits < password__policy__minimum_digits)
    {
        msg += "<li>Password does not yet contain enough digits.</li>";
    }

    if(count_spaces < password__policy__minimum_spaces)
    {
        msg += "<li>Password does not yet contain enough spaces.</li>";
    }

    if(count_specials < password__policy__minimum_specials)
    {
        msg += "<li>Password does not yet contain enough special characters such as punctuation.</li>";
    }

    if(count_unicode < password__policy__minimum_unicode)
    {
        msg += "<li>Password does not yet contain enough unicode characters (characters with code pointer larger to 255).</li>";
    }

    if(password__policy__minimum_variation > 0)
    {
        if(count_lowercase_letters > 0)
        {
            vc.push(count_lowercase_letters);
        }
        if(count_uppercase_letters > 0)
        {
            vc.push(count_uppercase_letters);
        }
        if(count_digits > 0)
        {
            vc.push(count_digits);
        }
        if(count_spaces > 0)
        {
            vc.push(count_spaces);
        }
        if(count_specials > 0)
        {
            vc.push(count_specials);
        }
        if(count_unicode > 0)
        {
            vc.push(count_unicode);
        }
        max_variation = password__policy__minimum_variation;
        if(vc.length < password__policy__minimum_variation)
        {
            msg += "<li>Password does not yet contain enough character variations (try entering uppercase letters, digits, punctuation...).</li>";
            max_variation = vc.length;
        }
        vc.sort(snapwebsites.compareNumbers);
        for(i = 0; i < max_variation; ++i)
        {
            if(vc[i] < password__policy__minimum_length_of_variations)
            {
                msg += "<li>Password does not yet contain enough character in each category (try entering more or each type of letters, digits, punctuation...).</li>";
                break;
            }
        }
    }

    return msg;
};





// auto-initialize
jQuery(document).ready(function()
    {
        snapwebsites.EditorInstance.registerWidgetType(new snapwebsites.EditorWidgetTypePassword());
        snapwebsites.EditorInstance.registerWidgetType(new snapwebsites.EditorWidgetTypePasswordConfirm());
    });

// vim: ts=4 sw=4 et
