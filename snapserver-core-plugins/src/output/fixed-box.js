/** @preserve
 * Name: fixed-box
 * Version: 0.1.19
 * Browsers: all
 * Copyright: Copyright 2015-2017 (c) Made to Order Software Corporation  All rights reverved.
 * Depends: output (0.1.5.70)
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



/** \brief Handle one Fixed Box item.
 *
 * This class represents one Fixed Box item. A Fixed Box is defined as
 * a \<div> tag with class "fixed-box":
 *
 * \code
 *  <div class="fixed-box"
 *       data-fixed-box-name="ref"
 *       data-fixed-box-container="ref"
 *       data-fixed-box-orientation="vertical"
 *       data-fixed-box-margin="10"
 *       data-fixed-box-min-width="768">
 *    <div>This is the fixed content</div>
 *  </div>
 * \endcode
 *
 * If the height of the div.fixed-box may not be correct (i.e. a float
 * generally has the wrong height of 0px), then you can specify the
 * one div that will represent a valid height for this fixed-box in
 * the data-fixed-box-container attribute. The reference is a standard
 * CSS selector (it can be an identifier, a class, tag types, etc.)
 *
 * The name is used to give the user a way to retrieve a FixedBox
 * that was automatically initialized. Note that it is mandatory
 * even if you do not use it.
 *
 * The orientation defaults to "vertical" and also allows "horizontal".
 * If you scroll up/down, then the default is enough. If you scroll
 * left/right, then change the orientation to "horizontal".
 *
 * \note
 * A vertical fixed box only gets its "top" CSS field modified.
 * A horizontal fixed box only gets its "left" CSS field modified.
 *
 * The fixed box margin is used to make sure that the fixed box does not
 * get glued to the edge of the browser. It defaults to zero.
 *
 * A fixed box is usually created automatically when loading a page.
 * You may dynamically create fixed boxes if you'd like.
 *
 * \note
 * At some point, the "position: sticky" CSS value should be used
 * instead of the fixed-box.js script. However, this only works in
 * Chrome and Firefox at this point. It will probably never work
 * in IE if it gets 100% replaced by Edge.
 *
 * \todo
 * We need to add code to observe whether some of the widgets
 * concerned (container, this box, the child) change size. We
 * want to have a separate piece of code that does that observation
 * using the followgin code:
 * https://developer.mozilla.org/en-US/docs/Web/API/MutationObserver
 * see also the stackoverflow reference here:
 * http://stackoverflow.com/questions/9628450/jquery-how-to-determine-if-a-div-changes-its-height-or-any-css-attribute/29568586#answer-29568586
 *
 * @param {jQuery} box  The \<div> representing this box.
 *
 * @return {!snapwebsites.FixedBox}
 *
 * @constructor
 * @struct
 */
snapwebsites.FixedBox = function(box)
{
    var that = this,
        container = box.data("fixed-box-container"),
        m = box.data("fixed-box-margin"),
        min_width = box.data("fixed-box-min-width");

    this.box_ = box;

    // retrieve the fixed box data once on creation
    //

    // box name, needs to be unique between all boxes on a page
    //
    this.name_ = snapwebsites.castToString(box.data("fixed-box-name"), "data-fixed-box-name is expected to be defined and to be a string");

    // orientation of the box: vertical or horizontal
    //
    this.orientation_ = box.data("fixed-box-orientation") == "horizontal"
                        ? snapwebsites.FixedBox.ORIENTATION_HORIZONTAL
                        : snapwebsites.FixedBox.ORIENTATION_VERTICAL;

    // margin
    //
    if(m != undefined)
    {
        this.margin_ = snapwebsites.castToNumber(m, "data-fixed-box-margin is expected to be a valid number, no dimension, it is always taken as pixels");
    }
    //else -- keep default (0)

    // min-width
    //
    if(min_width != undefined)
    {
        this.minWidth_ = snapwebsites.castToNumber(min_width, "data-fixed-box-min-width is expected to be a valid number, no dimension, it is always taken as pixels");
    }
    //else -- keep default (-1)

    // verify some parameters
    //
    if(this.name_.length == 0
    || this.margin_ < 0)
    {
        throw new Error("A FixedBox must define a valid name attribute and a margin value which is not negative.");
    }

    // transform the container string to a jQuery object immediately
    // (so that way we do it once) -- if empty or it fails, use box_
    // as the default container
    //
    if(container)
    {
        this.container_ = jQuery(container);
        if(!this.container_.exists())
        {
            throw new Error("A FixedBox container selector must be valid (return at least one object) if specified.");
        }
    }
    else
    {
        this.container_ = this.box_;
    }

    // we must save the minimum position on entry, otherwise we
    // cannot really know what it is since it changes all the time
    //
    if(this.orientation_ == snapwebsites.FixedBox.ORIENTATION_VERTICAL)
    {
        this.minPosition_ = this.box_.offset().top;
    }
    else
    {
        this.minPosition_ = this.box_.offset().left;
    }

    // connect on the scroll and resize events
    //
    jQuery(window).bind("scroll resize",
            function()
            {
                that.adjustPosition();
            }
        );

    // run a first adjustment
    //
    this.adjustPosition();

    return this;
};


/** \brief Mark FixedBox as a base class.
 *
 * This class does not inherit from any other classes.
 */
snapwebsites.base(snapwebsites.FixedBox);


/** \brief The orientation_ values: Horizontal.
 *
 * Expect to fix the specified widget if one can scroll left and right.
 *
 * \sa snapwebsites.FixedBox.ORIENTATION_VERTICAL
 *
 * @type {number}
 * @const
 * @private
 */
snapwebsites.FixedBox.ORIENTATION_HORIZONTAL = 1; // static const


/** \brief The orientation_ values: Vertical.
 *
 * Expect to fix the specified widget if one can scroll up and down.
 *
 * \sa snapwebsites.FixedBox.ORIENTATION_HORIZONTAL
 *
 * @type {number}
 * @const
 * @private
 */
snapwebsites.FixedBox.ORIENTATION_VERTICAL = 0; // static const


/** \brief The current setup is the original.
 *
 * This value is used with the currentMode_ field. If set to
 * CURRENT_MODE_ORIGINAL then it is set to the default value
 * (i.e. we did not change the value yet.)
 *
 * \sa snapwebsites.FixedBox.CURRENT_MODE_STATIC
 * \sa snapwebsites.FixedBox.CURRENT_MODE_FIXED
 *
 * @type {number}
 * @const
 * @private
 */
snapwebsites.FixedBox.CURRENT_MODE_ORIGINAL = 0; // static const


/** \brief The current setup is the static one.
 *
 * This value is used with the currentMode_ field. If set to
 * CURRENT_MODE_STATIC then it is considered to be set to the
 * default value and the object scrolls with the scrollbar.
 *
 * \sa snapwebsites.FixedBox.CURRENT_MODE_STATIC
 * \sa snapwebsites.FixedBox.CURRENT_MODE_FIXED
 *
 * @type {number}
 * @const
 * @private
 */
snapwebsites.FixedBox.CURRENT_MODE_STATIC = 1; // static const


/** \brief The current setup is the fixed one.
 *
 * This value is used with the currentMode_ field. If set to
 * CURRENT_MODE_FIXED then it is considered to be set to the
 * fixed value, meaning that the scrollbar is at a position
 * which forces us to keep the object at a specific fixed
 * position.
 *
 * \sa snapwebsites.FixedBox.CURRENT_MODE_ORIGINAL
 * \sa snapwebsites.FixedBox.CURRENT_MODE_FIXED
 *
 * @type {number}
 * @const
 * @private
 */
snapwebsites.FixedBox.CURRENT_MODE_FIXED = 2; // static const


/** \brief The jQuery object representing this fixed box.
 *
 * This parameter represents this fixed box jQuery object. It is used
 * to handle the box.
 *
 * @type {jQuery}
 * @private
 */
snapwebsites.FixedBox.prototype.box_ = null;


/** \brief The name of the fixed box.
 *
 * This parameter is the name of the fixed box.
 *
 * The name is generally used by the FixedBoxes class to find FixedBox
 * objects as queried by users of the FixedBoxes object.
 *
 * @type {string}
 * @private
 */
snapwebsites.FixedBox.prototype.name_ = "";


/** \brief The selector to the container.
 *
 * At times (actually, probably quite often) the FixedBox.box_
 * element is not enough to handle the box because it won't
 * represent the correct height or width of the rail. The
 * container represents a \<div> element which represents
 * the full size of the area where the fixed box will be
 * scrolled.
 *
 * The container is a selector used to retrieve an object
 * as in:
 *
 * \code
 *      var height = jQuery(this.container_).height();
 * \endcode
 *
 * If the parameter is the empty string, then box_ is used
 * as the rail/container.
 *
 * @type {jQuery}
 * @private
 */
snapwebsites.FixedBox.prototype.container_ = null;


/** \brief The orientation of the fixed box.
 *
 * A fixed box can be handled when scroll up / down or left / right.
 *
 * The orientation is expected to be set to either: horizontal
 * or vertical. If not specified, it defaults to vertical.
 *
 * @type {number}
 * @private
 */
snapwebsites.FixedBox.prototype.orientation_ = snapwebsites.FixedBox.ORIENTATION_VERTICAL;


/** \brief The margin to keep the boxes from touching the edges of the viewport.
 *
 * By default, the margin is set to 0 meaning that your boxes will
 * be glued to the edges of the browser as you scroll. This parameter
 * can be used to push the boxes away from the edges.
 *
 * The margin is used for top and bottom margins when the orientation is
 * vertical and left and right margins when the orientation is
 * horizontal.
 *
 * @type {number}
 * @private
 */
snapwebsites.FixedBox.prototype.margin_ = 0;


/** \brief Ignore at that screen width or less.
 *
 * The FixedBox has to be compatible with SmartPhone JavaScripts. Those
 * generally kick in whenever the screen becomes too narrow. When that
 * happens, the FixedBox can be asked to stop doing anything. Since we
 * allow for our windows to be resized (on desktops/laptops, at least),
 * the size has to be specified and the code react dynamically whenever
 * it happens.
 *
 * By default the value is set to -1, which means FixedBox will kick
 * in whatever the width. It can be changed using the
 * data-fixed-box-min-width attribute.
 *
 * @type {number}
 * @private
 */
snapwebsites.FixedBox.prototype.minWidth_ = -1;


/** \brief The position of the \<div> at the start.
 *
 * Before ever moving the object anywhere, we retrieve its startup
 * position which also happens to be the minimum value one can use.
 *
 * @type {number}
 * @private
 */
snapwebsites.FixedBox.prototype.minPosition_ = 0;


/** \brief The current setup.
 *
 * The current mode of the managed object.
 *
 * @type {number}
 * @private
 */
snapwebsites.FixedBox.prototype.currentMode_ = snapwebsites.FixedBox.CURRENT_MODE_ORIGINAL;


/** \brief Retrieve the name of this FixedBox object.
 *
 * The name of a FixedBox is mandatory and expected to be setup in
 * the tag representing a FixedBox using the name attribute. If not
 * present, then the script throws an exception.
 *
 * @return {string}
 */
snapwebsites.FixedBox.prototype.getName = function()
{
    return this.name_;
}


/** \brief Handle the scrolling of the window.
 *
 * This function is called whenever the scroll position of the window
 * changes. It allows the FixedBox to adjust its own position before
 * returning.
 *
 * The function may be manually called for other reasons like the
 * resizing of of of the \<div> that compose the box.
 */
snapwebsites.FixedBox.prototype.adjustPosition = function()
{
    if(this.orientation_ == snapwebsites.FixedBox.ORIENTATION_HORIZONTAL)
    {
        this.scrollHorizontally_();
    }
    else
    {
        this.scrollVertically_();
    }
}


/** \brief Adjust the position with a vertical orientation.
 *
 * This function is called by adjustPosition() when the
 * orientation is set to vertical.
 *
 * \note
 * This function always recalculates the maximum position the
 * widget can be go to in order to adjust the position accordingly.
 *
 * \sa adjustPosition()
 *
 * @private
 */
snapwebsites.FixedBox.prototype.scrollVertically_ = function()
{
    // the following works with the ExDox navigation menu;
    // watchout if you want to modify the code!
    //
    var pos = jQuery(window).scrollTop(),
        screen_width = parseFloat(jQuery(window.top).innerWidth()),
        screen_height = parseFloat(jQuery(window.top).innerHeight()),
        max = this.minPosition_
                + this.box_.outerHeight()
                - screen_height;

    // on a narrow screen, the user may not want to have the FixedBox
    // functionality (i.e. smartphones)
    //
    if(screen_width <= this.minWidth_)
    {
        if(this.currentMode_ == snapwebsites.FixedBox.CURRENT_MODE_FIXED)
        {
            this.currentMode_ = snapwebsites.FixedBox.CURRENT_MODE_STATIC;
            this.box_.css("top", "auto");
            this.box_.css("position", "static");
        }
        return;
    }
//console.log("pos = " + pos + ", max = " + max + ", margin = " + this.margin_ + " (sum: " + (max + this.margin_) + "), height = " + screen_height);

    if(max < -this.margin_)
    {
        max = -this.margin_;
    }

    //if(pos <= this.minPosition_)
    //{
    //    // I don't see why this would be required, the fixed box should be
    //    // scrollable (static) by default...
    //    //
    //    this.box_.css("top", this.margin_ + "px");
    //    this.box_.css("position", "fixed");
    //}
    //else
    if(pos <= max + this.margin_)
    {
        // in this case the item is allowed to scroll, so let the position be
        // static (the supposed default -- TODO: make it work with relative too)
        //
        if(this.currentMode_ != snapwebsites.FixedBox.CURRENT_MODE_STATIC)
        {
            this.currentMode_ = snapwebsites.FixedBox.CURRENT_MODE_STATIC;
            this.box_.css("top", "auto");
            this.box_.css("position", "static");
        }
    }
    else
    {
        // in this case the position becomes fixed which
        // glues the item where it's at (once it reaches
        // its maximum, to be precise)
        //
        // Note: we have to use a fixed position because of IE,
        //       which otherwise bounces the box up and down like crazy
        //       (they scroll the box, then run the JS which fixes the
        //       position, then display the box with the new position,
        //       Mozilla runs the JS before doing any refresh.)
        //       In IE it still bounces a bit at the top and bottom
        //       when scrolling "too fast".
        //
        // We set the top value each time since it could change
        // (i.e. the window got resized...)
        //
        this.box_.css("top", (-max) + "px");
        if(this.currentMode_ != snapwebsites.FixedBox.CURRENT_MODE_FIXED)
        {
            this.currentMode_ = snapwebsites.FixedBox.CURRENT_MODE_FIXED;
            this.box_.css("position", "fixed");
        }
    }
};


/** \brief Adjust the position with a horizontal orientation.
 *
 * This function is called by adjustPosition() when the
 * orientation is set to horizontal.
 *
 * \sa adjustPosition()
 *
 * @private
 */
snapwebsites.FixedBox.prototype.scrollHorizontally_ = function()
{
    var pos = jQuery(window).scrollLeft() + this.margin_,
        max = this.minPosition_ + this.container_.width() - this.box_.outerWidth() - this.margin_;

    // TODO: the following was not yet tested in our Snap! enviroment...
    //
    if(pos < this.minPosition_)
    {
        pos = this.minPosition_;
    }
    else if(pos > max)
    {
        pos = max;
    }

    pos -= this.minPosition_;

    this.box_.css("left", pos + "px");
};


/** \brief Fixed Boxes prevents scrolling of boxes.
 *
 * Often it is neat to have a box that is "fixed" on the screen
 * while a window is scrolled up and down or left and right.
 *
 * The fixed box is a \em rail which itself includes a sub-\<div> element
 * which is expected to be fixed on the screen while the window gets
 * scrolled.
 *
 * In most cases, it is a box on the left or the right of the page's
 * main contents. The entire setup of such boxes is done in HTML. This
 * script handles all the other mechanics. This means the box will never
 * go at position 0 on the page because there is most certainly a header
 * there. It also won't go to the bottom if there is a footer in the way.
 *
 * \note
 * Note that boxes that should never scroll, you should use a fixed
 * position instead of this capability.
 *
 * @return {!snapwebsites.FixedBoxes}
 *
 * @constructor
 * @struct
 */
snapwebsites.FixedBoxes = function()
{
    this.fixedBoxes_ = {};

    this.initFixedBoxes_();

    return this;
};


/** \brief Mark FixedBox as a base class.
 *
 * This class does not inherit from any other classes.
 */
snapwebsites.base(snapwebsites.FixedBoxes);


/** \brief The FixedBoxes instance.
 *
 * This class is a singleton and as such it makes use of a static
 * reference to itself. It gets created on load.
 *
 * \@type {snapwebsites.FixedBoxes}
 */
snapwebsites.FixedBoxesInstance = null; // static


/** \brief The jQuery DOM object representing the popup.
 *
 * This parameter represents the popup DOM object used to create the
 * popup window. Since there is only one darken page popup, this is
 * created once and reused as many times as required.
 *
 * Note, however, that it is not "safe" to open a popup from a popup
 * as that will reuse that same darkenpagepopup object which will
 * obstruct the view.
 *
 * @type {Object}
 * @private
 */
snapwebsites.FixedBoxes.prototype.fixedBoxes_ = null;


/** \brief Initialize each FixedBox.
 *
 * This function runs against each FixedBox defined in your page
 * and attach the scroll() function, etc. so we make sure they
 * are properly managed.
 */
snapwebsites.FixedBoxes.prototype.initFixedBoxes_ = function()
{
    var that = this;

    jQuery(".fixed-box").each(function()
        {
            var fixed_box = new snapwebsites.FixedBox(jQuery(this));
            that.fixedBoxes_[fixed_box.getName()] = fixed_box;
        });
};


/** \brief Retrieve a FixedBox object by name.
 *
 * @param {string} name  The name of the FixedBox object to return.
 *
 * @return {snapwebsites.FixedBox}
 */
snapwebsites.FixedBoxes.prototype.findFixedBox = function(name)
{
    return this.fixedBoxes_[name];
};



// auto-initialize
jQuery(document).ready(
        function()
        {
            snapwebsites.FixedBoxesInstance = new snapwebsites.FixedBoxes();
        }
    );

// vim: ts=4 sw=4 et
