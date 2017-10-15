/** @preserve
 * Name: users
 * Version: 0.0.1.19
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
// @externs plugins/users/externs/users-externs.js
// @js plugins/output/output.js
// @js plugins/output/popup.js
// @js plugins/server_access/server-access.js
// ==/ClosureCompiler==
//

/*jslint nomen: true, todo: true, devel: true */
/*global snapwebsites: false, jQuery: false */



/** \brief User session timeout.
 *
 * This class initialize a timer that allows the client (browser) to
 * "close" the window on a session timeout. That way the user does
 * not start using his account when it is already timed out (especially
 * by starting to type new data and not being able to save it.)
 *
 * The timer can be set to display a warning to the end user. By default
 * it just resets the current page if it is public, otherwise it sends
 * the user to another public page such as the home page.
 *
 * \todo
 * Add a timer that goes and check the server once in a while to make
 * sure that the session is not killed in between. Like for instance
 * once every 4h or so. Only that check must be such that we avoid
 * updating the dates in the existing session.
 *
 * @return {!snapwebsites.Users}  This object reference.
 *
 * @constructor
 * @struct
 */
snapwebsites.Users = function()
{
    // start the auto-logout timer
    //
    this.startAutoLogout();

    // see todo -- add another timer to periodically check the session
    //             on the server because it could get obliterated...

    return this;
};


/** \brief The ServerAccessCallbacks inherits ServerAccessCallbacks.
 *
 * This class inherits from the ServerAccessCallbacks so it can send
 * an AJAX command to the server to know where the user session is
 * at.
 */
snapwebsites.inherits(snapwebsites.Users, snapwebsites.ServerAccessCallbacks);


/** \brief The Users instance.
 *
 * This class is a singleton and as such it makes use of a static
 * reference to itself. It gets created on load.
 *
 * @type {snapwebsites.Users}
 */
snapwebsites.UsersInstance = null; // static


/** \brief The variable holding the timeout identifier.
 *
 * This variable holds the timer identifier which is eventually used to
 * cancel the timer when it was not triggered before some other event.
 *
 * @type {number}
 *
 * @private
 */
snapwebsites.Users.prototype.auto_logout_id_ = NaN;


/** \brief The server access object.
 *
 * The Users object creates a Server Access object so it can check the
 * session status with the server using AJAX.
 *
 * The Server Access object is held in this variable member so it can be
 * created just once.
 *
 * @type {snapwebsites.ServerAccess}
 * @private
 */
snapwebsites.Users.prototype.serverAccess_ = null;


/** \brief Whether we (already) detected that the time is out of whack.
 *
 * Whenever we setup the timeout timer interval, we calculate a number
 * which can end up being negative. When that happens and that negative
 * number is less than or equal to -3,600, the we emit a warning about
 * it. To avoid emitting many warnings, we use this flag. Once true, we
 * know we already emitted the warning and avoid doing it agian.
 *
 * @type {boolean}
 * @private
 */
snapwebsites.Users.prototype.warnedAboutTimeOutOfWhat_ = false;


/** \brief Start the timer of the auto-logout feature.
 *
 * This function starts or restarts the auto-logout timer. Although
 * there is no need to do so, it can be called multiple times in a
 * row. It will reset the timer each time it gets called, though.
 *
 * The function may call itself in case the timeout is too far in the
 * future (more than 24.8 days!) so as to make sure we time out on the
 * day we have to time out.
 *
 * In most cases, you call this function after you called the
 * preventAutoLogout().
 *
 * \note
 * Yes. We expect the user to close his browsers way before this timer
 * happens. But we are thourough...
 *
 * \sa preventAutoLogout()
 */
snapwebsites.Users.prototype.startAutoLogout = function()
{
    // TODO: remove the 20 seconds once we have a valid query string
    //       option to force a log out of the user on a timeout (actually
    //       we may even want to reduce the timeout by 1 minute or so...)
    //
    // we add 20 seconds to the time limit to make sure that we
    // timeout AFTER the session is dead;
    //
    var delay = (users__session_time_limit + 20) * 1000 - Date.now(),
        that = this;

    // force a delay of at least 60 seconds, otherwise we could have
    // a very intensive loop sending hit=check to the server over
    // and over for close to nothing...
    //
    if(delay < 60000)
    {
        // if we have a console, also show a warning if the delay is
        // more than 1h off
        //
        if(window.console
        && delay <= -3600000
        && !this.warnedAboutTimeOutOfWhat_)
        {
            this.warnedAboutTimeOutOfWhat_ = true;
            console.log("WARNING: the auto-logout delay is more than 1h off. A computer clock is probably out of whack.");
        }
        delay = 60000;
    }

    // we may get called again before the existing timeout was triggered
    // so if the ID is not NaN, we call the clearTimeout() on it
    //
    if(!isNaN(this.auto_logout_id_))
    {
        clearTimeout(this.auto_logout_id_);
        this.auto_logout_id_ = NaN;
    }

    // A JavaScript timer is limited to 2^31 - 1 which is about 24.8 days.
    // So here we want to make sure that we do not break the limit if
    // that ever happens.
    //
    // Note: I checked the Firefox implementation of window.setTimeout()
    //       and it takes the interval at he time of the call to calculate
    //       the date when it will be triggered, so even if 24.8 days later
    //       it will still be correct and execute within 50ms or so
    //
    if(delay > 0x7FFFFFFF)
    {
        this.auto_logout_id_ = setTimeout(function()
            {
                that.startAutoLogout();
            },
            0x7FFFFFFF);
    }
    else
    {
        this.auto_logout_id_ = setTimeout(function()
            {
                that.autoLogout_();
            },
            delay);
    }
};


/** \brief Stop the auto-logout timer.
 *
 * Once in a while, it may be useful (probably not, though) to
 * stop the auto-logout timer. This could help in avoiding the
 * redirect that happens on the timeout while we do some other
 * work.
 *
 * To restart the auto-logout timer, use the startAutoLogout()
 * function.
 *
 * \sa startAutoLogout()
 */
snapwebsites.Users.prototype.preventAutoLogout = function()
{
    if(!isNaN(this.auto_logout_id_))
    {
        clearTimeout(this.auto_logout_id_);
        this.auto_logout_id_ = NaN;
    }
};


/** \brief Function called once the auto-logout times out.
 *
 * After a certain amount of time, a user login session times out.
 * When that happens we can do one of several things
 *
 * \warning
 * Although we do not yet have it at the time of writing, we
 * expect that the logout timer will fire way after the last
 * auto-save of whatever changes the user has made so far.
 * This means that we do not have to worry about obliterating
 * the user's data.
 *
 * @private
 */
snapwebsites.Users.prototype.autoLogout_ = function()
{
    var doc = document,
        uri = doc.location.toString();

    // remove all query strings on that URI
    // (should we only remove the 'hit' and 'action' parameters?)
    //
    uri = uri.replace(/\?.*$/, "");

    // note: since the timer was triggered, the ID is not valid anymore
    //
    this.auto_logout_id_ = NaN;

    if(!this.serverAccess_)
    {
        this.serverAccess_ = new snapwebsites.ServerAccess(this);
    }
    this.serverAccess_.setURI(uri, { 'hit': 'check' });
    this.serverAccess_.send();
};


/** \brief Function called to reload the current page.
 *
 * Depending on how the AJAX code reacts, we will call this function.
 * This function is used to reloads the current page. Although we
 * are removing the 'action=edit' if present.
 *
 * @private
 */
snapwebsites.Users.prototype.reloadPage_ = function()
{
    var doc = document,
        redirect_uri;

    redirect_uri = doc.location.toString();

    // reload the page without the 'edit' action (XXX we probably should
    // remove any type of action, not just the edit action?)
    //
    // since the user is likely logged out, the reload will send actually
    // him to another page (in most cases the login page) if this current
    // page requires the user to be logged in to view this page
    //
    redirect_uri = redirect_uri.replace(/\?a=edit$/, "")
                               .replace(/\?a=edit&/, "?")
                               .replace(/&a=edit&/, "&");
    redirect_uri = snapwebsites.ServerAccess.appendQueryString(redirect_uri, { hit: "transparent" });
    doc.location = redirect_uri;
};


/** \brief Function called on AJAX success.
 *
 * This function is called when the client receives the answer from the server.
 *
 * The answer will include the current status of the session and if the
 * session is still considered valid, when it will end (i.e. the value to
 * replace the users__session_time_limit global variable).
 *
 * If the session has died, then this function generates a redirect.
 *
 * If the session has not died, we call the startAutoLogout() function
 * to restart the timer.
 *
 * @param {snapwebsites.ServerAccessCallbacks.ResultData} result  The
 *          resulting data with information about the error(s).
 */
snapwebsites.Users.prototype.serverAccessSuccess = function(result) // virtual
{
    var xml_data = jQuery(result.jqxhr.responseXML),
        session_status = xml_data.find("data[name='users__session_status']").text(),
        limit;

    // get the current user session time limit
    //
    limit = xml_data.find("data[name='users__session_time_limit']").text();
    if(limit && limit.length > 0)
    {
        users__session_time_limit = parseInt(limit, 10);
    }

    // get the current administrator session time limit
    //
    limit = xml_data.find("data[name='users__administrative_login_time_limit']").text();
    if(limit && limit.length > 0)
    {
        users__administrative_login_time_limit = parseInt(limit, 10);
    }

    // if session timed out, then we want to redirect the user
    // the system should automatically save this page in the user's
    // session, send the user to the login page, and reload it once
    // the user logs back in
    //
    if(session_status == "none")
    {
        this.reloadPage_();
        return;
    }

    // on a success with a session that did not yet time out, we want to
    // repeat the test so we restart the AJAX timer
    //
    this.startAutoLogout();

    // manage messages if any
    //
    snapwebsites.Users.superClass_.serverAccessComplete.call(this, result);
};


/** \brief Function called on AJAX error.
 *
 * This function is called when the client receives the answer from the server.
 *
 * The answer will include the current status of the session and if the
 * session is still considered valid, when it will end (i.e. the value to
 * replace the users__session_time_limit global variable).
 *
 * If the session has died, then this function generates a redirect.
 *
 * If the session has not died, we call the startAutoLogout() function
 * to restart the timer.
 *
 * \note
 * Unfortunately, if the reload fails the end user sees a "totally broken"
 * page. This is not great, but more secure than keeping a logged in user
 * page accessible to anyone who gains access to the client's computer.
 *
 * @param {snapwebsites.ServerAccessCallbacks.ResultData} result  The
 *          resulting data with information about the error(s).
 */
snapwebsites.Users.prototype.serverAccessError = function(result) // virtual
{
    // on an error we cannot be sure whether the session is still alive
    // or not so instead we have to force a redirect; since in most cases
    // the AJAX check is expected to get a timed out session, this makes
    // quite a lot of sense
    //
    // note that someone who opens multiple tabs to a Snap! website may
    // get "interesting behaviors" that his case does not handle well,
    // but we certainly hope that it won't happen to often...
    //
    this.reloadPage_();
};




// auto-initialize
jQuery(document).ready(
    function()
    {
        snapwebsites.UsersInstance = new snapwebsites.Users();
    }
);
// vim: ts=4 sw=4 et
