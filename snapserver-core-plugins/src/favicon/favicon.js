/** @preserve
 * Name: favicon
 * Version: 0.0.1.16
 * Browsers: all
 * Copyright: Copyright 2014-2017 (c) Made to Order Software Corporation  All rights reverved.
 * Depends: output (0.1.5)
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
// @js plugins/output/popup.js
// @js plugins/server_access/server-access.js
// @js plugins/listener/listener.js
// @js plugins/editor/editor.js
// ==/ClosureCompiler==
//

/*jslint nomen: true, todo: true, devel: true */
/*global jQuery: false, Uint8Array: true */



/** \brief Plugin Selection constructor.
 *
 * The Plugin Selection is a singleton handling the Plugin Selection
 * window.
 *
 * @return {snapwebsites.FavIcon}  The newly created object.
 *
 * @constructor
 * @struct
 * @extends {snapwebsites.ServerAccessCallbacks}
 */
snapwebsites.FavIcon = function()
{
    this.initFavIcon_();

    return this;
};


/** \brief FavIcon derives from ServerAccessCallbacks.
 *
 * The FavIcon class inherit the ServerAccessCallbacks class
 * so it knows when an AJAX request succeeded.
 */
snapwebsites.inherits(snapwebsites.FavIcon, snapwebsites.ServerAccessCallbacks);


/** \brief The FavIcon instance.
 *
 * This class is a singleton and as such it makes use of a static
 * reference to itself. It gets created on load.
 *
 * \@type {snapwebsites.FavIcon}
 */
snapwebsites.FavIconInstance = null; // static


/** \brief The toolbar object.
 *
 * This variable represents the toolbar used by the editor.
 *
 * Note that this is the toolbar object, not the DOM. The DOM is
 * defined within the toolbar object and is considered private.
 *
 * @type {snapwebsites.ServerAccess}
 * @private
 */
snapwebsites.FavIcon.prototype.serverAccess_ = null;


/** \brief The Install or Remove button that was last clicked.
 *
 * This parameter holds the last Install or Remove button that
 * was last clicked.
 *
 * @type {jQuery}
 * @private
 */
snapwebsites.FavIcon.prototype.clickedButton_ = null;


/** \brief Attach to the buttons.
 *
 * This function attaches the FavIcon instance to the form buttons if
 * any.
 *
 * The function is expected to be called only once by the constructor.
 *
 * @private
 */
snapwebsites.FavIcon.prototype.initFavIcon_ = function()
{
    var that = this;

    // The "Install" buttons
    //
    jQuery("ul.default-icons li a")
        .makeButton()
        .click(function(e){
            var icon_name;

            e.preventDefault();
            e.stopPropagation();

            if(that.clickedButton_)
            {
                // already working
                return;
            }

            that.clickedButton_ = jQuery(this);

            icon_name = this.hash.substr(1);

            // user clicked the "Install" button, send the info to the
            // server with AJAX...
            //
            if(!that.serverAccess_)
            {
                that.serverAccess_ = new snapwebsites.ServerAccess(that);
            }
            that.serverAccess_.setURI("/admin/settings/favicon");
            that.serverAccess_.showWaitScreen(1);
            that.serverAccess_.setData(
                {
                    icon: icon_name
                });
            that.serverAccess_.send(e);
        });
};


/** \brief Function called on AJAX success.
 *
 * This function is called if the install or remove of a plugin succeeded.
 *
 * Here we make sure to change the interface accordingly.
 *
 * @param {snapwebsites.ServerAccessCallbacks.ResultData} result  The
 *          resulting data.
 */
snapwebsites.FavIcon.prototype.serverAccessSuccess = function(result) // virtual
{
    var editor = snapwebsites.EditorInstance,
        favicon_form = editor.getFormByName("favicon"),
        icon_widget = favicon_form.getWidgetByName("icon"),
        url;

    snapwebsites.FavIcon.superClass_.serverAccessSuccess.call(this, result);

    // TODO: should we not use the widget jQuery content instead?
    //
    jQuery("div.editor-form div.snap-editor.editable.icon .editor-content").html(snapwebsites.castToString(this.clickedButton_.html(), "HTML from clicked button is expected to be a string"));

    // this is a tad bit ugly, we may want to at least calculate
    // the positions...
    //
    jQuery("div.editor-form div.snap-editor.editable.icon .editor-content img")
            .css({
                    top: "88px",
                    left: "88px",
                    position: "relative"
                    // TODO: add the filename and width/height attributes?
                });

    // also change the <link> tag (some browsers will correctly update the
    // icon in the tab that way!)
    //
    jQuery("head link[rel='shortcut icon']").remove();
    url = this.clickedButton_.find("img").attr("src");
    jQuery("head").append("<link type='image/x-icon' rel='shortcut icon' href='" + url + "'/>");

    // this code modifies the widget and thus it is now viewed as
    // "it needs to be saved", which it already is, so we do not
    // want to get that message anymore
    //
    icon_widget.saved(icon_widget.saving());
};


/** \brief Function called on AJAX success.
 *
 * This function is called if the install or remove of a plugin succeeded.
 *
 * Here we make sure to change the interface accordingly.
 *
 * @param {snapwebsites.ServerAccessCallbacks.ResultData} result  The
 *          resulting data.
 */
snapwebsites.FavIcon.prototype.serverAccessComplete = function(result) // virtual
{
    snapwebsites.FavIcon.superClass_.serverAccessComplete.call(this, result);
    this.clickedButton_ = null;
};



// auto-initialize
jQuery(document).ready(
    function()
    {
        snapwebsites.FavIconInstance = new snapwebsites.FavIcon();
    }
);

// vim: ts=4 sw=4 et
