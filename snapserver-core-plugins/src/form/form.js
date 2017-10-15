/** @preserve
 * Name: form
 * Version: 0.0.2.42
 * Browsers: all
 * Depends: output (>= 0.1.5)
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
// ==/ClosureCompiler==
//

/*jslint nomen: true, todo: true, devel: true */
/*global snapwebsites: false, jQuery: false */



/** \brief Snap Form Manipulations.
 *
 * This class initializes and handles the different form inputs.
 *
 * \note
 * The Snap! Form is a singleton and should never be created by you. It
 * gets initialized automatically when this form.js file gets included.
 *
 * @return {!snapwebsites.Form}  This object reference.
 *
 * @constructor
 * @struct
 */
snapwebsites.Form = function()
{
    var focused = false,
        that = this;

    jQuery("form input[type='text']:not([data-background-value='']), form input[type='password']:not([data-background-value=''])")
        .focus(function()
            {
                that.focus_(this);
            })
        .change(function()
            {
                that.change_(this);
            })
        .blur(function()
            {
                that.blur_(this);
            })
        .on("keyup cut paste", function()
            {
                that.restartAutoReset_(this);
            })
        .each(function(i, e)
            {
                that.blur_(e);
            });

    // At least in Firefox, checkboxes do not reflect the checked="checked"
    // attribute properly on a reload (reload which can happen after a
    // standard POST!); so here we fix the problem in JavaScript
    jQuery("form input[type='checkbox']")
        .each(function(i, e)
            {
                var w = jQuery(e),
                    status = w.attr("status-on-load");

                if(status == "checked")
                {
                    w.attr("checked", "checked");
                }
                else
                {
                    w.removeAttr("checked");
                }
            });

    // TODO: check on how to handle the case of multiple forms and one Enter key
    //       if there is more than one form, we've got a problem on this one...
    //       since it looks like we're going to do the POST on a single form
    //       which may not be the right one.
    jQuery("form")
        .keydown(function(e)
            {
                if(e.which == 13)
                {
                    // got a Return/Enter, so force the default button
                    // action now
                    e.preventDefault();
                    e.stopPropagation();

                    // simulate a click on the default button
                    jQuery(this).find("input.default-button").click();
                }
            })
        .submit(function(e)
            {
                // TODO: verify the data as much as possible before we allow a submit
                //       also change the default data with "" otherwise we will be
                //       sending the background-value to the server!?
                if(that.submitted_)
                {
                    // a form is being processed, somehow we got two calls
                    // to the submit() function, prevent the second call
                    e.preventDefault();
                    e.stopPropagation();
                    return;
                }
                that.submitted_ = true;

                jQuery("form input[type='text']:not([data-background-value='']), form input[type='password']:not([data-background-value=''])")
                    .each(function(i, widget)
                        {
                            that.focus_(widget);
                        });
            });

    jQuery("form")
        .each(function(i, widget)
            {
                var w = jQuery(widget),
                    focus_id = w.attr("focus"),
                    timeout = parseFloat(w.attr("timeout")),
                    timeout_id;

                // this is very problematic, we can really only have one widget with
                // the focus; at this time we do not know which form is in the main
                // page and whether it would have a higher priority...
                if(!focused)
                {
                    focused = true;
                    jQuery("#" + focus_id)
                        .focus()
                        .select();
                }
                else
                {
                    // the .focus() has the side effect of calling the
                    // restartAutoReset_() function so this way we avoid
                    // doing it twice
                    that.restartAutoReset_(w.find("input,textarea").first());
                }

                // the timeout should always be set; it represents the
                // amount of time until a session ends; it is a fixed
                // time and therefore never gets reset once setup
                if(timeout > 0) // form times out?
                {
                    // we remove a few minutes because the clocks may
                    // not be properly synchronized
                    //
                    // (also we expect timeout to be at least 3 minutes)
                    timeout -= 3;
                    if(timeout < 3)
                    {
                        timeout = 3;
                    }
                    timeout_id = setTimeout(function()
                        {
                            that.timedOut_(w);
                        },
                        timeout * 60000); // minutes to ms
                    w.attr("timeout-id", timeout_id);
                }
            });

    // after we connected the .submit() callback, we finally enable
    // the submit buttons
    jQuery("form input[type='submit'][enable-on-load='enable-on-load']")
        .each(function(i, e)
            {
                jQuery(e).removeAttr("disabled");
            });

    return this;
};


/** \brief Mark Form as a base class.
 *
 * This class does not inherit from any other classes.
 */
snapwebsites.base(snapwebsites.Form);


/** \brief The Form instance.
 *
 * This class is a singleton and as such it makes use of a static
 * reference to itself. It gets created on load.
 *
 * @type {snapwebsites.Form}
 */
snapwebsites.FormInstance = null; // static


/** \brief A form was submitted once this is true.
 *
 * With a standard form and no AJAX, only one form can be submitted and
 * it can only be submitted once. The Enter key and buttons may be double
 * clicked and we want to avoid a possible second call so we use this
 * flag to prevent that from happening.
 *
 * @type {boolean}
 *
 * @private
 */
snapwebsites.Form.prototype.submitted_ = false;


/** \brief This function is called whenever a widget gets the focus.
 *
 * The function makes sure that the background value is removed
 * when the widget gets focused, if the background value was there.
 *
 * Also the function sets the type attribute to "password" on
 * password widgets whenever the background value is removed.
 *
 * @param {Element} widget  The widget that was just focused.
 *
 * @private
 */
snapwebsites.Form.prototype.focus_ = function(widget)
{
    var w = jQuery(widget);

    // TODO: ameliorate with 2 colors until change() happens
    if(w.val() === w.data('background-value')
    && !w.data('editor'))
    {
        w.val('');
    }

    // we force the removal of the class
    // (just in case something else went wrong)
    w.removeClass('input-with-background-value');

    if(w.hasClass('password-input'))
    {
        w.attr('type', 'password');
    }

    this.restartAutoReset_(widget);
};


/** \brief This function is called whenever a widget is modified.
 *
 * The function is called whenever the widget is changed in some
 * way. The function is used to set the value of the edited
 * data so we know whether to show the background value when
 * the widget gets blurred.
 *
 * @param {Element} widget  The widget that just changed.
 *
 * @private
 */
snapwebsites.Form.prototype.change_ = function(widget)
{
    var w = jQuery(widget);

    w.data("edited", w.val() !== "");

    this.restartAutoReset_(widget);
};


/** \brief This function is called whenever a widget is blurred.
 *
 * The function is an event executed when the blur event is received.
 * The function restores the background value if there is one and
 * the widget is empty.
 *
 * Note that in case of a password the type of the widget is changed
 * to "text" when it is empty and we want to show the background
 * value (otherwise the background value would show up as stars.)
 *
 * @param {Element} widget  The widget that just lost focus.
 *
 * @private
 */
snapwebsites.Form.prototype.blur_ = function(widget)
{
    var w = jQuery(widget),
        d = w.prop("disabled"),
        l;

    if(!w.is(":focus"))
    {
        l = w.val().length;
        if(l === 0 && !d)
        {
            w.val( /** @type string */ (w.data("background-value")))
                .addClass('input-with-background-value');

            if(w.hasClass('password-input'))
            {
                w.attr('type', 'text');
            }
        }
        else if(l != 0 && d)
        {
            w.val('');
        }
    }
};


/** \brief Get the current value of the widget.
 *
 * It is important for you to use this function to retrieve the
 * value of a widget because if it is the background value, this
 * function makes sure to return the empty string.
 *
 * @param {Array.<Element>|Element} widget  The widget element from which you want to
 *                          retrieve the value.
 *
 * @return {string}  The current value as a string.
 */
snapwebsites.Form.prototype.getVal = function(widget)
{
    var w = jQuery(widget);

    if(w.hasClass('input-with-background-value'))
    {
        return '';
    }
    return snapwebsites.castToString(w.val(), "Form.getVal() casting the .val() result");
};


/** \brief Set the value of the widget.
 *
 * It is important for you to use this function to set the value
 * of a widget because if it is the empty string, the background
 * value is used instead.
 *
 * @param {Array.<Element>|Element} widget  The widget element from which
 *                                          you want to retrieve the value.
 * @param {string|number} value  The value of the text to set the
 *                               input widget.
 */
snapwebsites.Form.prototype.setVal = function(widget, value)
{
    var w = jQuery(widget);

    if(value || w.is(":focus") || w.prop("disabled"))
    {
        if(w.hasClass('password-input'))
        {
            w.attr('type', 'password');
        }
        w.val( /** @type {string} */ (value))
            .removeClass('input-with-background-value');
    }
    else
    {
        w.val( /** @type string */ (w.data("background-value")))
            .addClass('input-with-background-value');
        if(w.hasClass('password-input'))
        {
            w.attr('type', 'text');
        }
    }
};


/** \brief Restart the timer of this form.
 *
 * Each time the user enters a key or clicks a checkbox or radio button,
 * the corresponding form timer has to be restarted (i.e. we extend the
 * timeout back to the full amount.)
 *
 * @param {Element|jQuery|null} widget  The widget that just got activated.
 *
 * @private
 */
snapwebsites.Form.prototype.restartAutoReset_ = function(widget)
{
    var w = jQuery(widget),
        f = w.parents("form"),
        auto_reset_id = parseFloat(f.attr("auto-reset-id")),
        auto_reset = parseFloat(f.attr("auto-reset")),
        that = this;

    // clear the old one
    if(!isNaN(auto_reset_id))
    {
        clearTimeout(auto_reset_id);
    }

    if(auto_reset > 0)
    {
        // setup a new one
        auto_reset_id = setTimeout(function()
            {
                that.autoReset_(f);
            },
            auto_reset * 60000); // minutes to ms

        // save the new id back in the form
        f.attr("auto-reset-id", auto_reset_id);
    }
};


/** \brief Function called once the auto-reset times out.
 *
 * After a certain amount of time, this function gets called to reset
 * the form. This is quite important for forms that hold data such
 * a user name, password, credit card number, etc.
 *
 * Note that the form does not time out in this case. That is, the user
 * can continue to use the form for a while, only they will have to
 * re-enter the information.
 *
 * @param {jQuery} w  The from which has to be reset.
 *
 * @private
 */
snapwebsites.Form.prototype.autoReset_ = function(w)
{
    var that = this,
        form_auto_reset = jQuery.Event("form_auto_reset",
        {
            form: w,
            minutes: w.attr("auto-reset")
        });

    // it triggered, so the ID is not valid anymore
    w.removeAttr("auto-reset-id");

    // TBD: should we send the existing data to a "draft" like
    //      store, so we can restore it if the user logs back in?
    //      (but not if the data is considered secret)

    // reset so possibly secret data (user name, address, telephone,
    // password, etc.) get removed
    w[0].reset();

    // finally give other scripts a chance to do something about
    // the form such as hide it or put a button to reload the page
    // and thus give the user a way to "restore" the form
    w.trigger(form_auto_reset);
};


/** \brief Function called once the form times out.
 *
 * After a certain amount of time, this function gets called to reset
 * the form. This is quite important for forms that hold data such
 * a user name, password, credit card number, etc.
 *
 * \note
 * Contrary to the auto-reset feature, the timeout counter is never
 * reset since the time on a timeout is fixed. Also there should
 * always be a timeout call for all forms since all forms have a
 * timed session.
 *
 * @param {jQuery} w  The from which timed out.
 *
 * @private
 */
snapwebsites.Form.prototype.timedOut_ = function(w)
{
    var that = this,
        form_timeout = jQuery.Event("form_timeout",
        {
            form: w,
            minutes: w.attr("timeout")
        });

    // TBD: should we send the existing data to a "draft" like
    //      store, so we can restore it if the user logs back in?

    // reset so possibly secret data (user name, address, telephone,
    // password, etc.) get removed
    w[0].reset();

    // now also disable the form so it cannot be submitted anymore
    jQuery(w).find(":input").attr("disabled", "disabled");

    jQuery(w)
        .find("input[type='text']:not([data-background-value='']), input[type='password']:not([data-background-value=''])")
        .each(function(i, widget)
            {
                that.blur_(widget);
            });

    // finally give other scripts a chance to do something about
    // the form such as hide it or put a button to reload the page
    // and thus give the user a way to "restore" the form
    w.trigger(form_timeout);

    // TODO: support a system reload button using AJAX
};



// auto-initialize
jQuery(document).ready(
    function()
    {
        snapwebsites.FormInstance = new snapwebsites.Form();
    }
);
// vim: ts=4 sw=4 et
