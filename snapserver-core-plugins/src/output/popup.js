/** @preserve
 * Name: popup
 * Version: 0.1.0.62
 * Browsers: all
 * Copyright: Copyright 2014-2017 (c) Made to Order Software Corporation  All rights reverved.
 * Depends: output (0.1.5.71)
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



/** \brief Snap DarkenScreen.
 *
 * This class is used by to darken the whole screen behind a popup or
 * for a "Please Wait ..." effect (i.e. a way to prevent further user
 * interactions until we are done with an AJAX message.)
 *
 * To remove a DarkenScreen once done with it, call its close()
 * function and forget its reference (see close() for an example.)
 *
 * @param {number} show  A positive number defining the amount of ms used
 *                       to show the darken page (as in jQuery.fadeIn())
 * @param {boolean} wait  Whether this is opened to represent a wait.
 *
 * @return {!snapwebsites.DarkenScreen}
 *
 * @constructor
 * @struct
 *
 * \sa close()
 */
snapwebsites.DarkenScreen = function(show, wait)
{
//#ifdef DEBUG
    var idx;

    for(idx = snapwebsites.DarkenScreen.darkenScreenStack_.length - 1; idx >= 0; --idx)
    {
        if(snapwebsites.DarkenScreen.darkenScreenStack_[idx].wait_)
        {
            // we cannot wait more than once... (we certainly could
            // and may be we will allow such at a later time, but right
            // now I think this is bug if it happens.)
            //
            throw new Error("adding a DarkenScreen over another that has the 'wait' parameter set to true is not allowed.");
        }
    }
//#endif

    // if it is the first time we get called, we still have to create the
    // actual object used to darken the whole screen
    //
    if(!snapwebsites.DarkenScreen.darkenScreenPopup_)
    {
        jQuery("<div id='darkenPage' class='zordered'></div>").appendTo("body");
        snapwebsites.DarkenScreen.darkenScreenPopup_ = jQuery("#darkenPage");
    }

    // we create this DarkenScreen object to darken the current screen,
    // so we can compute the z-index as it stands right now...
    //
    snapwebsites.DarkenScreen.darkenScreenPopup_.css("z-index", 0);
    this.zIndex_ = jQuery(".zordered").maxZIndex() + 1;
    snapwebsites.DarkenScreen.darkenScreenPopup_.css("z-index", this.zIndex_);

    // setup the 'wait' class appropriately
    //
    this.wait_ = wait;
    snapwebsites.DarkenScreen.darkenScreenPopup_.toggleClass("wait", wait);

//#ifdef DEBUG
    for(idx = snapwebsites.DarkenScreen.darkenScreenStack_.length - 1; idx >= 0; --idx)
    {
        if(this.zIndex_ < snapwebsites.DarkenScreen.darkenScreenStack_[idx].zIndex_)
        {
            // if this happens, it means our maxZIndex did not work
            // correctly because there should be some new items below
            // the new DarkenScreen
            //
            // Note that for a "double wait" (which we do not currently
            // allow) we would end up with an equality here, but still
            // not one index smaller than the other
            //
            throw new Error("somehow a newer DarkenScreen object has z-index "
                    + this.zIndex_
                    + ", which is smaller than an older DarkScreen object which has a z-index of "
                    + snapwebsites.DarkenScreen.darkenScreenStack_[idx].zIndex_);
        }
    }
//#endif

    // fade it in now
    //
    if(show < 10)
    {
        // timing is so small, just show instantly
        //
        snapwebsites.DarkenScreen.darkenScreenPopup_.show();
    }
    else
    {
        snapwebsites.DarkenScreen.darkenScreenPopup_.fadeIn(show);
    }

    // add this very item to the stack of darken pages
    //
    snapwebsites.DarkenScreen.darkenScreenStack_.push(this);

    return this;
};


/** \brief Mark DarkenScreen as a base class.
 *
 * This class does not inherit from any other classes.
 */
snapwebsites.base(snapwebsites.DarkenScreen);


/** \brief The darken "page" (a div that covers the whole screen).
 *
 * This variable holds the jQuery DOM object representing the darken
 * page popup.
 *
 * We use only one such DOM object, so it is marked as static. Whether
 * to show or hide this object depends on our current stack, which is
 * a static object too. More or less, if the stack is not empty, the
 * darken page is visible. If the stack is empty, then the darken
 * page is made invisible (display: none; so as to not interfere with
 * anything else.)
 *
 * @type {?jQuery}
 * @private
 */
snapwebsites.DarkenScreen.darkenScreenPopup_ = null; // static


/** \brief The stack of darken pages.
 *
 * Whenever a popup darkens the screen, it adds one DarkenScreen item
 * to this stack. When a popup undarkens the screen, its corresponding
 * item gets removed from the stack.
 *
 * The DarkenScreen item at the top of the stack defines the current
 * z-index of the darkenScreenPopup_ DOM object.
 *
 * @type {Array.<snapwebsites.DarkenScreen>}
 * @private
 */
snapwebsites.DarkenScreen.darkenScreenStack_ = []; // static


/** \brief The z-index where this darken screen is.
 *
 * Each DarkenScreen has a z-index. The darkenScreenStack_ parameter
 * has a stack of DarkenScreen object order by zIndex_ (the largest
 * is last.)
 *
 * @type {number}
 * @private
 */
snapwebsites.DarkenScreen.prototype.zIndex_ = 0;


/** \brief Whether this darken page is used as a "please wait" screen.
 *
 * This flag is used to know whether the "wait" class should be set
 * or not.
 *
 * Note that it is considered an error to open yet another DarkenScreen
 * over one that has this flag set to true. In debug mode, it throws
 * in such a special case.
 *
 * @type {boolean}
 * @private
 */
snapwebsites.DarkenScreen.prototype.wait_ = false;


/** \brief Get rid of a darken screen.
 *
 * This function is used to undarken the screen. If other darken
 * screen objects exist in the stack of darken screens, then
 * the screen remains darken, only the z-index parameter changes
 * back to the now largest z-index still in the stack. The wait
 * class may also be added or removed.
 *
 * This DarkenScreen reference should then be nullified since the
 * object is considered gone and it cannot be revived.
 *
 * \code
 *      this.myDarkenObject_.close();
 *      this.myDarkenObject_ = null;
 * \endcode
 */
snapwebsites.DarkenScreen.prototype.close = function()
{
    var idx;

    // 99% of the time, it will be the last one although we allow
    // intermediate popups being closed before to top-most popup
    // so we could end up removing another DarkenScreen than the
    // last
    //
    for(idx = snapwebsites.DarkenScreen.darkenScreenStack_.length - 1; idx >= 0; --idx)
    {
        if(snapwebsites.DarkenScreen.darkenScreenStack_[idx] == this)
        {
            snapwebsites.DarkenScreen.darkenScreenStack_.splice(idx, 1);
            if(idx == snapwebsites.DarkenScreen.darkenScreenStack_.length)
            {
                if(snapwebsites.DarkenScreen.darkenScreenStack_.length == 0)
                {
                    // Since the fading may or may not happen, having the user passing
                    // a timing for it is not sensible
                    //
                    // At this point we use 150ms, although we may look into a way for
                    // users to enter an amount in some settings (but I am not so sure
                    // it would be that useful, 150ms is so short no one should complain
                    // about such duration, right?)
                    //
                    snapwebsites.DarkenScreen.darkenScreenPopup_.fadeOut(150);
                }
                else
                {
                    // we removed the last darken page on the stack and there is
                    // still some darken pages... adjust the z-index and wait flag
                    //
                    --idx;
                    snapwebsites.DarkenScreen.darkenScreenPopup_.css("z-index", snapwebsites.DarkenScreen.darkenScreenStack_[idx].zIndex_);
                    snapwebsites.DarkenScreen.darkenScreenPopup_.toggleClass("wait", snapwebsites.DarkenScreen.darkenScreenStack_[idx].wait_);
                }
            }
            // else ...
            //
            // we did not remove the last item so we have nothing more to
            // do (i.e. the top item still has the correct z-index and
            // we cannot be the last if idx is smaller than length)
            //

            return;
        }
    }

    // otherwise we have got an error
    //
    throw new Error("attempt to close this DarkenScreen failed: could not find that object in the existing stack (closing more than once?)");
};



/** \brief Snap Popup.
 *
 * This class initializes and handles the different popup features.
 *
 * \note
 * The Snap! Output handles a popup capability that can be used to open
 * any number of popups.
 *
 * By default popups automatically include a close button.
 *
 * @return {!snapwebsites.Popup}
 *
 * @constructor
 * @struct
 */
snapwebsites.Popup = function()
{
    return this;
};


/** \brief Mark Popup as a base class.
 *
 * This class does not inherit from any other classes.
 */
snapwebsites.base(snapwebsites.Popup);


/** \brief The type definition for the popup structure.
 *
 * The Popup class makes use of a popup structure called PopupData.
 * This is an object passed to most of the Popup object functions
 * that holds the current state of your popup.
 *
 * The open() function set the \p id parameter if it is not defined
 * when the function is called.
 *
 * The open() function sets the \p widget parameter to a jQuery()
 * reference to the top-most DOM element used to create the popup.
 *
 * \li id -- the identifier of the popup DOM object; it is expected to be
 *           unique for each popup; i.e. if you call open() with the same
 *           popup.id parameter, then that very popup gets reopened.
 *           If undefined, the Popup library generates an identifier
 *           automatically ("snapPopup\<number>").
 * \li path -- the path to a page to display in the popup; in this case
 *             the Popup object uses an IFRAME to display the content.
 * \li html -- The HTML to display in the IFRAME, this is exclusive from
 *             the path usage (i.e. either path or hatml, not both).
 * \li noClose -- do not create a close button for the popup; this is
 *                particularly useful for popup windows that ask a question
 *                and cannot really safely have a close button (because the
 *                meaning of that close button is not known).
 * \li top -- the top position; if undefined, the popup will be vertically
 *            centered.
 * \li left -- the left position; if undefined, the popup will be horizontally
 *             centered.
 * \li width -- the width to use for the popup; defaults to 400 pixels.
 * \li height -- the height to use for the popup; defaults to 300 pixels.
 * \li darken -- the amount of time it will take to darken the screen behind
 *               the popup; zero, a negative value, null, or undefined
 *               prevents any darkening; use 1 to get the darkening instantly.
 * \li position -- "absolute" to get the popup to scroll with the window; this
 *                 is useful if you are to have very tall popups; you may also
 *                 use "fixed" to get a popup that does not scroll at all;
 *                 the default is "fixed".
 * \li title -- a title for the popup; it may include HTML.
 * \li open -- a callback called once the popup is open, it gets called with
 *             a reference to this popup object. Note that opened does not
 *             mean visible (see 'show' as well.)
 * \li show -- a callback called once the popup is fully visible (i.e. if
 *             fading in, this callback is called after the fade in is
 *             over); it has no parameters.
 * \li beforeHide -- a callback called before the popup gets hidden; this
 *                   gives your code a chance to cancel a click on the close
 *                   button or equivalent; the callback is called with the
 *                   PopupData object; the editor offers such a function:
 *                   snapwebsites.EditorForm.beforeHide().
 * \li hide -- a callback called once the popup is fully hidden; which means
 *             after the fade out is over; it is called without any
 *             parameters.
 * \li hideNow -- a callback defined by the Popup class in case the
 *                beforeHide is used; that function should be called by
 *                the beforeHide callback in the event the user decided
 *                to anyway close the popup; it takes no parameters.
 * \li widget -- this parameter is defined by the system; it is set to the
 *               jQuery widget using the id parameter.
 * \li editorFormName -- the name of the form attached to this popup;
 *                       this is used by the editor beforeHide() function.
 * \li darkenScreen_ -- this is a private member which you do not have access
 *                      to; it is used to save the DarkenScreen created by
 *                      the popup as required by the darken parameter.
 *
 * \note
 * The width and height could be calculated with different euristic math
 * and propbably will at some point. The current version requires these
 * parameters to be defined if you want to get a valid popup.
 *
 * \todo
 * Create an object to make it a lot safer (i.e. so as to not allow
 * contradictory parameters or invalid values at the time we set them.)
 * At the same time, we currently have static definitions and that cannot
 * be used with a class.
 *
 * @typedef {{id: ?string,
 *            path: ?string,
 *            html: ?string,
 *            noClose: ?boolean,
 *            top: ?number,
 *            left: ?number,
 *            width: number,
 *            height: number,
 *            darken: ?number,
 *            position: string,
 *            title: ?string,
 *            open: ?function(Object),
 *            show: ?function(),
 *            beforeHide: ?function(Object),
 *            hide: ?function(),
 *            hideNow: ?function(),
 *            widget: ?Object,
 *            editorFormName: ?string,
 *            darkenScreen_: snapwebsites.DarkenScreen}}
 */
snapwebsites.Popup.PopupData;


/** \brief The type used to create a message box.
 *
 * This type definition is used by the snapwebsites.Popup.messageBox()
 * function.
 *
 * \li id -- the identifier for the message box popup.
 *
 * @typedef {{id: string,
 *          title: string,
 *          message: string,
 *          buttons: Object.<{name: string, label: string}>,
 *          top: !number,
 *          left: !number,
 *          width: !number,
 *          height: !number,
 *          html: !string,
 *          callback: function(string)}}
 */
snapwebsites.Popup.PopupMessage;


/** \brief The Popup instance.
 *
 * This class is a singleton and as such it makes use of a static
 * reference to itself. It gets created on load.
 *
 * \@type {snapwebsites.Popup}
 */
snapwebsites.PopupInstance = null; // static


/** \brief A unique identifier.
 *
 * Each time a new popup is created it is given a new unique
 * identifier. This parameter is used to compute that new
 * identifier. The value is saved in the settings passed to
 * the open() function, so on the next call with the same
 * settings, the identifier will already be defined and thus
 * it won't change.
 *
 * @type {number}
 * @private
 */
snapwebsites.Popup.prototype.uniqueID_ = 0;


/** \brief Create a new popup "window".
 *
 * This function creates a \<div\> tag with the settings as defined
 * in the \c popup object. Once created, the popup needs to be shown
 * with the show() function (see below).
 *
 * All popups will have an identifier. If you do not specify one, then
 * the next available identifier will be used. That identifier starts
 * with the name "snapPopup" and ends with a unique number.
 *
 * Note that the identifier cannot be anything other than a non-empty
 * string or a non-zero number since those are viewed as false and thus
 * the automatically generated unique identifier will be used in that
 * case.
 *
 * To get rid of a popup, make sure to call the forget() function with
 * the same \p popup parameter. However, popups that can be shared or
 * reused should not be forgotten.
 *
 * The \p popup parameters are defined in the PopupData structure.
 *
 * The open() function creates the following few parameters as
 * required:
 *
 * \li id -- only if not defined by the caller
 * \li widget -- overwritten every time
 * \li darkenScreen_ -- set if darken > 0
 *
 * @param {snapwebsites.Popup.PopupData} popup  The settings to create
 *                                              the popup.
 * @return {jQuery}
 */
snapwebsites.Popup.prototype.open = function(popup)
{
    var i, b, f, p, t, w, y, that = this;

    if(!popup.id)
    {
        // user did not specify an id, generate a new one
        ++this.uniqueID_;
        popup.id = "snapPopup" + this.uniqueID_;
    }
    // popup already exists?
    popup.widget = jQuery("#" + popup.id);
    if(popup.widget.length === 0)
    {
        jQuery("<div class='snap-popup zordered' id='" + popup.id + "' style='position:fixed;visibility:hidden;'>"
             + (popup.noClose ? "" : "<div class='close-popup'></div>")
             + "<div class='inside-popup'><div class='popup-title'></div><div class='popup-body'></div></div></div>")
                .appendTo("body");
        popup.widget = jQuery("#" + popup.id);
    }
    popup.widget.show();

    // allow for a closePopup() directly from the iframe whether we have
    // a close-popup or not
    //
    popup.widget.bind("close_popup", function()
        {
            that.hide(popup);
        });

    // here we cannot be sure whether the click() event was still here
    // so we unbind() it first
    popup.widget.children(".close-popup").unbind('click').click(function()
        {
            that.hide(popup);
        });

    i = popup.widget.children(".inside-popup");
    t = i.children(".popup-title");
    b = i.children(".popup-body");
    if(popup.title)
    {
        t.empty();
        t.prepend(popup.title);
    }
    if(popup.width)
    {
        popup.widget.css("width", popup.width);
    }
    else
    {
        popup.widget.css("width", 400);
    }
    if(popup.height)
    {
        popup.widget.css("height", popup.height);
    }
    else
    {
        popup.widget.css("height", 300);
    }
    if(popup.top)
    {
        popup.widget.css("top", popup.top);
    }
    else
    {
        // Using jQuery("body") would be better to take any padding in
        // account, unfortunately, it gives us the whole document size
        // and not just the window size.
        //
        // TODO: retrieve the "body" padding to reduce the window height
        //
        y = Math.floor((jQuery(window).height() - popup.widget.outerHeight()) / 2);
        if(y < 0)
        {
            y = 0;
        }
        popup.widget.css("top", y);
    }
    if(popup.left)
    {
        popup.widget.css("left", popup.left);
    }
    else
    {
        w = Math.floor((jQuery("body").width() - popup.widget.outerWidth()) / 2);
        if(w < 0)
        {
            w = 0;
        }
        popup.widget.css("left", w);
    }
    if(popup.position && popup.position === "absolute")
    {
        popup.widget.css("position", "absolute");
    }
    if(popup.path)
    {
        // make sure to mark this popup window as coming from an IFRAME
        p = popup.path + (popup.path.indexOf("?") === -1 ? "?" : "&") + "iframe=true";
        b.empty();
        b.append("<iframe name='" + popup.id + "' class='popup-iframe' src='"
                    + p + "' frameborder='0' marginheight='0' marginwidth='0'></iframe>");
        f = b.children(".popup-iframe");

        // 'b.width()' may return a jQuery object so we have to force the
        // type to a number to call f.attr().
        f.attr("width", snapwebsites.castToNumber(b.width(), "snapwebsites.Popup.open() retrieving popup body height"));

        // the height of the body matches the height of the IFRAME so we
        // cannot use it here

//console.log("WxH = "+b.width()+"x"+b.height()
//          +" widget top = "+popup.widget.offset().top
//          +", widget height = "+popup.widget.height()
//          +", body top = "+b.offset().top);

        f.attr("height", popup.widget.offset().top
                       + snapwebsites.castToNumber(popup.widget.height(), "snapwebsites.Popup.open() retrieving popup window height")
                       - b.offset().top);
    }
    else if(popup.html)
    {
        b.empty();
        b.append(popup.html);
    }
    //else -- leave the user's body alone (on a second call, it may not be
    //        empty and already have what the user expects.)
    if(popup.open)
    {
        popup.open(popup.widget);
    }
    popup.widget.hide();

    // we start with "visibility: hidden" so switch to visible now that we
    // have a hidden popup
    popup.widget.css("visibility", "visible");

    return popup.widget;
};


/** \brief Show the popup.
 *
 * This function shows a popup that was just created with the open()
 * function or got hidden with the hide() function.
 *
 * @param {snapwebsites.Popup.PopupData} popup  The settings to create
 *                                              the popup.
 */
snapwebsites.Popup.prototype.show = function(popup)
{
    if(popup.id)
    {
        popup.widget = jQuery("#" + popup.id);
        if(popup.widget && !popup.widget.is(":visible"))
        {
            popup.widget.fadeIn(
                {
                    duration: 150,
                    complete: popup.show
                });
            if(popup.darken > 0 && !popup.darkenScreen_)
            {
                popup.darkenScreen_ = new snapwebsites.DarkenScreen(/** @type {number} */ (popup.darken), false);
            }
            popup.widget.css("z-index", 0);
            popup.widget.css("z-index", jQuery(".zordered").maxZIndex() + 1);

            // user make have asked for one of the widget to get focus
            // in most cases this is used for anchors (forms have their
            // own auto-focus feature)
            popup.widget.find(".popup-auto-focus").focus();
        }
    }
};


/** \brief Hide the popup.
 *
 * This function hides a popup that was previously opened with the
 * open() function and shown with the show() function.
 *
 * This is often referenced as a soft-close. After this call the popup
 * is still available in the DOM, it is just hidden (display: none).
 *
 * If your popup gets hidden, then the hide() function gets called.
 * Note that the hide() function is called after the popup is completely
 * hidden so depending on the animation timing it may take more or less
 * time.
 *
 * Note that the hide() function is called if the popup is already
 * hidden to make sure that your code knows that it is indeed hidden.
 * However, if the popup object defines a beforeHide() function which
 * never calls the call back (the hideNow() function) then the
 * hide() function will never be called.
 *
 * @param {snapwebsites.Popup.PopupData} popup  The settings to create
 *                                              the popup.
 */
snapwebsites.Popup.prototype.hide = function(popup)
{
    var that = this;

    if(popup.id)
    {
        popup.widget = jQuery("#" + popup.id);
        if(popup.widget && popup.widget.is(":visible"))
        {
            if(popup.beforeHide)
            {
                // the beforeHide() function does not return immediately
                // as it may open a confirmation popup and thus need to
                // offer a callback to process the confirmation
                popup.hideNow = function() { that.hideNow_(popup); };
                popup.beforeHide(popup);
                return;
            }
            // no beforeHide(), immediate close
            this.hideNow_(popup);
            return;
        }
    }

    // it was already hidden, make sure the hide() function gets called
    if(popup.hide)
    {
        popup.hide();
    }
};


/** \brief Close the popup.
 *
 * This function is expected to be called from within a popup iframe.
 * It will close the popup by triggering the "close_popup" event
 * on the main popup widget.
 */
snapwebsites.Popup.prototype.closePopup = function()
{
    window.parent.jQuery(window.frameElement, window.parent.document)
            .parents(".snap-popup")
            .trigger("close_popup");
};


/** \brief Actual hides the popup.
 *
 * This function actually hides the Popup. It is separate from the hide()
 * function so one can make use of a beforeHide() which requires a callback
 * If you have a beforeHide() you must call the popup.hideNow() function
 * once your are done and if you want to proceed with hiding
 * the popup.
 *
 * @param {snapwebsites.Popup.PopupData} popup  The settings to create
 *                                              the popup.
 * @private
 */
snapwebsites.Popup.prototype.hideNow_ = function(popup)
{
    popup.widget.fadeOut(
        {
            duration: 150,
            complete: popup.hide
        });
    if(popup.darkenScreen_)
    {
        popup.darkenScreen_.close();
        popup.darkenScreen_ = null;
    }
};


/** \brief Remove a popup from the DOM.
 *
 * This function removes the popup from the DOM. It does not first
 * fade out the popup. This function immediately removes the elements
 * from the DOM. You may want to first hide then forget.
 *
 * @param {snapwebsites.Popup.PopupData} popup  The settings to create
 *                                              the popup.
 *
 * \sa hide()
 */
snapwebsites.Popup.prototype.forget = function(popup)
{
    if(popup.widget)
    {
        popup.widget.remove();
        popup.widget = null;
    }
};


/** \brief Create a message box.
 *
 * This function is used to create an HTML message box. It expects a
 * structure that includes a message (in HTML) and any number of
 * buttons in an array. A click on any button closes the box and
 * calls your callback with the name of that button.
 *
 * A button has a label and a name.
 *
 * The callback is a function that should accept the name of the button
 * as its parameter. If you have a single button, it will generally not
 * be necessary to accept the name.
 *
 * The popup has a Close button which is always viewed as a button in
 * that dialog. When clicked, the callback function is called with the
 * string parameter set to "close_button".
 *
 * @param {snapwebsites.Popup.PopupMessage} msg  The message object with
 *    the message itself, buttons, and a callback function.
 */
snapwebsites.Popup.prototype.messageBox = function(msg)
{
    var popup = /** @type {snapwebsites.Popup.PopupData} */ ({}),
        that = this,
        key;

    popup.id = msg.id;
    popup.noClose = true;
    popup.top = msg.top > 0 ? msg.top : 10;
    if(msg.left > 0)
    {
        popup.left = msg.left;
    }
    popup.width = msg.width > 0 ? msg.width : 450;
    popup.height = msg.height > 0 ? msg.height : 250;
    popup.darken = 150;
    popup.title = msg.title;
    popup.html = "<div class=\"message-box\">" + msg.message + "</div><div class=\"message-box-buttons\">";
    for(key in msg.buttons)
    {
        if(msg.buttons.hasOwnProperty(key))
        {
            popup.html += "<a class=\"message-button\" name=\""
                    + msg.buttons[key].name
                    + "\" href=\"#" + msg.buttons[key].name + "\">"
                    + msg.buttons[key].label + "</a>";
        }
    }
    popup.html += "</div>";

    this.open(popup);
    this.show(popup);

    popup.widget.find(".message-button").click(function()
        {
            var button = jQuery(this),
                name = /** @type {string} */ (button.attr("name"));

            // always hide that popup once a button was clicked
            that.hide(popup);
            msg.callback(name);
        });
};


// auto-initialize
jQuery(document).ready(
    function()
    {
        snapwebsites.PopupInstance = new snapwebsites.Popup();
    }
);

// vim: ts=4 sw=4 et
