/** @preserve
 * Name: rotate-animation
 * Version: 0.0.1.3
 * Browsers: all
 * Depends: jquery-rotate (2.3)
 * Copyright: Copyright 2014-2017 (c) Made to Order Software Corporation  All rights reverved.
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
// @externs plugins/output/externs/jquery-rotate.js
// @js plugins/output/output.js
// ==/ClosureCompiler==
//

/*jslint nomen: true, todo: true, devel: true */
/*global snapwebsites: false, jQuery: false */



/** \brief Create an object to apply a rotation to a widget.
 *
 * This constructor is used to create a never ending anmation that rotates
 * the specified widget around its center.
 *
 * Some parameters can be changed using other functions before you start
 * the animation (see start() to kickstart the animation.)
 *
 * To rotate in the other direction negate your angle.
 *
 * @param {jQuery} widget  A jQuery widget to rotate and the specified speed and angle.
 * @param {number} angle_increment  The angle to turn the object on each tick.
 * @param {number=} opt_delay  The delay between ticks in milliseconds. Default is 100.
 *
 * @constructor
 * @struct
 */
snapwebsites.RotateAnimation = function(widget, angle_increment, opt_delay)
{
    // the rotate code supports an animateTo() but no way to make something
    // rotate forever (from what I can see)
    this.widget_ = widget;
    this.angleIncrement_ = angle_increment;
    if(typeof opt_delay === "number" && opt_delay > 10)
    {
        this.delay_ = opt_delay;
    }

    this.angle_ = widget.getRotateAngle();
    if(!this.angle_)
    {
        // in case it undefined or null
        this.angle_ = 0;
    }
    this.center_ = ["50%", "50%"];
};


/** \brief The widget being rotated.
 *
 * This object is the one the RotateAnimation object is rotating.
 *
 * @type {?jQuery}
 * @private
 */
snapwebsites.RotateAnimation.prototype.widget_ = null;


/** \brief The delay between ticks of the animation.
 *
 * The object is rotated by angleIncrement_ every delay_ milliseconds.
 *
 * By default we set this parameter to 100.
 *
 * @type {number}
 * @private
 */
snapwebsites.RotateAnimation.prototype.delay_ = 100;


/** \brief The identifier of the timer used to animation the widget.
 *
 * This parameter is the timer identifier used to get the rotation
 * going. It is also used to stop the animation. It is important
 * to stop the animation if you want to get rid of the
 * RotateAnimation() object.
 *
 * @type {number}
 * @private
 */
snapwebsites.RotateAnimation.prototype.timerID_ = -1;


/** \brief The center of rotation of the object.
 *
 * By default the object is rotated around its center. This function can
 * be used to rotate the object around any other point.
 *
 * @type {?Array}
 * @private
 */
snapwebsites.RotateAnimation.prototype.center_ = null;


/** \brief The rotation current angle.
 *
 * This angle at this which the rotation starts. The constructor
 * defines this angle using the getRotateAngle() function.
 *
 * @type {number}
 * @private
 */
snapwebsites.RotateAnimation.prototype.angle_ = 0;


/** \brief The rotation angle increment.
 *
 * This angle used to increment the angle_ parameter determining the
 * next rotation position of the object.
 *
 * This is the value specified when creating the rotation animation object.
 *
 * @type {number}
 * @private
 */
snapwebsites.RotateAnimation.prototype.angleIncrement_ = 0;


/** \brief Set the center of rotation of the widget.
 *
 * This function sets the rotation center as percent of the object
 * position. By default this is set to ["50%", "50%"] which is the
 * exact center of the object.
 *
 * If the object is already being animated, then it will jump to
 * the new location on the next tick.
 *
 * The center can be set to a pixel position. However, the position
 * is expected to be limited to within the image [0..width, 0..height]
 * as the rotation capability makes use of a canvas and a position outside
 * of that would make the canvas really big.
 *
 * @param {Array.<number|string>} center  The new center to use for the rotation.
 */
snapwebsites.RotateAnimation.prototype.setCenter = function(center)
{
    this.center_ = center;
};


/** \brief Start the rotation.
 *
 * This function starts the rotation animation if not yet started.
 *
 * If the animation is already going, then the start function does
 * nothing.
 *
 * \warning
 * The RotateAnimation object is locked in place after a start even
 * if you drop all the references you have because it appears in
 * the setInterval() function. To completely release such an object
 * you want to make sure to stop() the animation at some point.
 */
snapwebsites.RotateAnimation.prototype.start = function()
{
    var that = this;

    if(this.timerID_ === -1)
    {
        this.timerID_ = setInterval(
            function()
            {
                that.rotate();
            },
            this.delay_);
    }
};


/** \brief Stop the rotation.
 *
 * This function stops the rotation animation.
 *
 * Note that the number of times the start() function is called does
 * not need to match the number of times the stop() function is called.
 * Calling the stop() function once stops the animation immediately.
 */
snapwebsites.RotateAnimation.prototype.stop = function()
{
    if(this.timerID_ !== -1)
    {
        clearTimeout(this.timerID_);
        this.timerID_ = -1;
    }
};


/** \brief Check whether this object is currently animated.
 *
 * This function returns true if the animation is currently playing.
 *
 * @return {boolean}  true if the animation was started and is still playing.
 */
snapwebsites.RotateAnimation.prototype.isAnimated = function()
{
    return this.timerID_ !== -1;
};


/** \brief Rotate the object by one increment.
 *
 * This function rotates the object once. This is called once per tick.
 * Ticks are defined by the speed parameter used when creating the
 * object.
 *
 * The function rotates the object to the next specified angle (i.e.
 * the current angle plus the angle_increment specified in the
 * constructor.)
 */
snapwebsites.RotateAnimation.prototype.rotate = function()
{
    // make sure we keep a reasonable angle amount (-359 to +359)
    this.angle_ = (this.angle_ + this.angleIncrement_) % 360;

    this.widget_.rotate(
        {
            angle: this.angle_,
            center: this.center_
        });
};


// vim: ts=4 sw=4 et
