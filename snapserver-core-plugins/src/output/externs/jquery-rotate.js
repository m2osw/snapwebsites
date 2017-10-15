/*
 * Name: jquery-rotate
 * Version: 1.0.1
 * Browsers: all
 * Copyright: Copyright 2014-2017 (c) Made to Order Software Corporation  All rights reverved.
 * Depends: jquery (1.11)
 * License: GPL 2.0
 */

/**
 *
 * @externs
 */

/**
 * Angle seems to be mandatory, although it says the default is zero so
 * we make it optional.
 *
 * Defaults:
 *
 * \li angle -- 0
 * \li animateTo -- not animated
 * \li callback -- function called once the animation is over
 * \li center -- ["50%", "50%"]
 * \li duration -- not animated
 * \li easing -- function used to calculate the next position
 * \li step -- a function called while animating to calculate the next step
 *             (new angle = old angle + step)
 *
 * @typedef {{angle: (number|null),
 *            animateTo: (number|null|undefined),
 *            callback: (function()|null|undefined),
 *            center: (Array|null|undefined),
 *            duration: (number|null|undefined),
 *            easing: (function(Object,number,number,number,number):number|null|undefined),
 *            step: (function(number):number|null|undefined)}}
 */
jQuery.rotate_settings;


/**
 * @param {number|jQuery.rotate_settings} settings
 * @return {jQuery}
 */
jQuery.prototype.rotate = function(settings) {};

/**
 * @return {number}
 */
jQuery.prototype.getRotateAngle = function() {};

/**
 * @return {number}
 */
jQuery.prototype.stopRotate = function() {};

