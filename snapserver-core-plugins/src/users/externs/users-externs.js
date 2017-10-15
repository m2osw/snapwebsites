/*
 * Name: users-externs
 * Version: 1.0.0
 * Browsers: all
 * Copyright: Copyright 2014-2017 (c) Made to Order Software Corporation  All rights reverved.
 * License: GPL 2.0
 */

/**
 *
 * @externs
 */


/** \brief Total time the user session survives.
 *
 * This variable represents the number of seconds the user session
 * is kept around. By default this is set to 1 year.
 *
 * @type {number}
 */
var users__session_time_to_live = 0;


/** \brief Time when the user session ends.
 *
 * This variable defines the time_t value when the user session
 * times out. At that time, the session will not be considered
 * valid anymore.
 *
 * The default is 5 days from the last user access to the website
 * (i.e. an access without "?hit=transparent" attached to the URI.)
 *
 * @type {number}
 */
var users__session_time_limit = 0;


/** \brief Time when the administrative login session ends.
 *
 * This variable defines the time_t value when the administrative
 * session times out. This is generally (way) smaller than the
 * users__session_time_limit and it is not updated just by hitting
 * the website. Instead you have to re-login later to increase this
 * limit.
 *
 * The default is 3h from the last time the user logged in.
 *
 * @type {number}
 */
var users__administrative_login_time_limit = 0;

