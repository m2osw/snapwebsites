/** @preserve
 * Name: listener
 * Version: 0.0.1.17
 * Browsers: all
 * Depends: server-access (>= 0.0.1.16)
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
// @js plugins/output/output.js
// @js plugins/output/popup.js
// @js plugins/server_access/server-access.js
// ==/ClosureCompiler==
//

/*jslint nomen: true, todo: true, devel: true */
/*global snapwebsites: false, jQuery: false */



/** \brief A request object to add to the server access listener object.
 *
 * Whenever you want to request for something with your Snap! server,
 * you can create a request object, and attach that object to the
 * ListenerInstance object. Once the object is available,
 * your callback will be called and you can proceed with your work
 * on your requested object.
 *
 * This is useful, for example, to handle a wait after a successful
 * drag and drop which should result in a preview.
 *
 * The settings can be set to a string in which case it is viewed as
 * the URI by itself (no callbacks.)
 *
 * When the settings are set to an object, however, you can specify
 * a set of callbacks along the URI as in:
 *
 * \li "uri": the URI to listen on
 * \li "success": the function called once the resource is available
 * \li "error": the function called if an error occurs
 * \li "complete": the function called when the listener is complete
 *                 with your request
 *
 * Note that only one of "success" or "error" is called. "complete"
 * is always called, however.
 *
 * \code
 * class ListenerRequest
 * {
 * public:
 *      function ListenerRequest(settings);
 *      function getURI() : string;
 *      function setMaxRetry(max_retry: number);
 *      function setSpeeds(speeds: Array of number);
 *      function immediate() : boolean;
 *      function done() : boolean;
 *      function timeout() : boolean;
 *      function ready() : boolean;
 *      function timerTick();
 *      function setResult(result: Document);
 *      function getResult() : Document;
 *      function callCallback(result_status: boolean);
 *
 * private:
 *      uri_: String;
 *      resultXML_: Document;
 *      success_: function(ListenerRequest);
 *      error_: function(ListenerRequest);
 *      complete_: function(ListenerRequest);
 *      waitingPeriods_: Array of number;
 *      maxRetry_: number;
 *      processedCounter_: number;
 *      timerCounter_: number;
 * };
 * \endcode
 *
 * \note
 * The success and error functions are not mandatory, although they
 * are very strongly advised since otherwise you will not know what
 * happened (that being said, you may not care much about the failure
 * case... if it fails, then nothing happens anyway.)
 *
 * @param {string|Object.<string,string|function(snapwebsites.ListenerRequest)>} settings  The URI of the object being checked.
 *
 * @return {snapwebsites.ListenerRequest}  A reference to
 *              the newly created object.
 *
 * @constructor
 * @struct
 */
snapwebsites.ListenerRequest = function(settings)
{
    if(!settings)
    {
        throw new Error("the URI to the ListenerRequest() constructor cannot be empty.");
    }

    if(typeof settings === "string")
    {
        this.uri_ = settings;
    }
    else if(settings instanceof Object)
    {
        if(typeof settings.uri !== "string")
        {
            throw new Error("the ListenerRequest() constructor always expects a URI string.");
        }
        this.uri_ = settings.uri;
        this.success_ = settings.success;
        this.error_ = settings.error;
        this.complete_ = settings.complete;
    }
    else
    {
        throw new Error("the ListenerRequest() constructor expected a string or an object as input.");
    }

    return this;
};


/** \brief This class is a base class.
 *
 * This class is a base class, it does not inherit from any others.
 */
snapwebsites.base(snapwebsites.ListenerRequest);


/** \brief The URI of the object being checked.
 *
 * This URI represents the full URI of the object being checked on the
 * server.
 *
 * @type {string}
 * @private
 */
snapwebsites.ListenerRequest.prototype.uri_ = "";


/** \brief The XML data we received in the reply.
 *
 * This document represents the XML data received in as the result
 * of sending this request to the server.
 *
 * Note that as expected only the XML data for this request is available
 * here. Note that the document is never empty as it at least includes
 * a \<result> tag.
 *
 * @type {Document}
 * @private
 */
snapwebsites.ListenerRequest.prototype.resultXML_ = null;


/** \brief The function called on success.
 *
 * This variable member holds the function that is to be called in case
 * the request succeeds.
 *
 * @type {?function(snapwebsites.ListenerRequest)}
 * @private
 */
snapwebsites.ListenerRequest.prototype.success_ = null;


/** \brief The function called on error.
 *
 * This variable member holds a function that is to be called in case
 * the request definitively failed. This generally is known on the
 * very first AJAX request.
 *
 * There are two main reasons for the error callback to get called:
 *
 * \li The server refused the request (probably permission denied).
 * \li The client tried for too long and the system decides to timeout.
 *
 * @type {?function(snapwebsites.ListenerRequest)}
 * @private
 */
snapwebsites.ListenerRequest.prototype.error_ = null;


/** \brief The function called once everything is complete.
 *
 * This variable member holds a function that is to be called once
 * the listener request is done.
 *
 * @type {?function(snapwebsites.ListenerRequest)}
 * @private
 */
snapwebsites.ListenerRequest.prototype.complete_ = null;


/** \brief Poll speeds.
 *
 * This variable represents the number of seconds we have to wait
 * before we poll for this URI again.
 *
 * The value is an array. The count is used to know how many times
 * we should try for availability and this array can change over
 * time (the last number is used once the counter is larger than
 * the number of elements in this array.)
 *
 * If the array is not defined, it is viewed as { 1 } meaning
 * that the test is performed once per second.
 *
 * To get the effect of an immediate query at the start, use
 * zero as the very first entry of the array.
 *
 * @type {Array.<number>}
 * @private
 */
snapwebsites.ListenerRequest.prototype.waitingPeriods_ = null;


/** \brief Number of attempts before timing out a request.
 *
 * This variable member represents the maximum number of time
 * a request will be sent to the server. After that number of
 * times, it is considered that the request has timed out.
 *
 * Change this number using the setMaxRetry().
 *
 * Note that the amount of time spent, in seconds, to know
 * whether the request succeeded varies depending on the
 * speeds array (in case you setup that array, a good idea
 * if you can skip sending requests for the first few
 * seconds.)
 *
 * @type {number}
 * @private
 */
snapwebsites.ListenerRequest.prototype.maxRetry_ = 10;


/** \brief Number of times the request was sent to the server.
 *
 * This variable member represents the number of times the
 * ready() function returns true. This represents the
 * number of times the request is sent to the server.
 *
 * \note
 * It is considered an internal number and it is not accessible
 * from the outside.
 *
 * @type {number}
 * @private
 */
snapwebsites.ListenerRequest.prototype.processedCounter_ = 0;


/** \brief Internal timer counter.
 *
 * This variable counts the ticks of the timer, one at a time.
 * When this counter is == to the last waiting period
 * the URI is sent to the server.
 *
 * The counter starts at -1.
 *
 * @type {number}
 * @private
 */
snapwebsites.ListenerRequest.prototype.timerCounter_ = -1;


/** \brief Retrieve the URI of this request.
 *
 * This function returns the URI specified when constructing the
 * request object.
 *
 * @return {string}
 */
snapwebsites.ListenerRequest.prototype.getURI = function()
{
    return this.uri_;
};


/** \brief Set the maximum number of times the requests should be sent.
 *
 * This function is used to change the maximum number of times a request
 * is sent to the server.
 *
 * \note
 * The total amount of time (in seconds) that the request last also
 * depends on the speeds array defined with the setSpeeds() function.
 *
 * @param {number} max_retry  The maximum number of tries.
 */
snapwebsites.ListenerRequest.prototype.setMaxRetry = function(max_retry)
{
    this.maxRetry_ = max_retry;
};


/** \brief Setup the poll speeds.
 *
 * This function expects an array of numbers. Each number represents
 * the number of seconds to wait before performing that test again.
 *
 * \code
 * // Try as soon as possible, then once every 5 seconds
 * [0, 5]
 *
 * // Start trying after 10 seconds (every second after that)
 * [10, 1]
 *
 * // Wait 1 minute, then 30 sec., then every 10 seconds
 * [60, 30, 10]
 * \endcode
 *
 * \note
 * It is expected that these arrays will remain relatively small (i.e.
 * 2 to 5 elements.) Very large arrays here will make the isNext()
 * call slow.
 *
 * \warning
 * The array MUST be proper, meaning that indices must go from 0 to
 * speeds.length - 1. If not, then it won't work right (this function
 * will throw).
 *
 * @param {Array.<number>} speeds  The array of speeds used for this reueqst.
 */
snapwebsites.ListenerRequest.prototype.setSpeeds = function(speeds)
{
    var idx;

//#ifdef DEBUG
    if(!(speeds instanceof Array))
    {
        throw new Error("the speeds parameter must be an array");
    }
//#endif
    for(idx = 0; idx < speeds.length; ++idx)
    {
        // if an entry is undefined then err
        if(typeof speeds[idx] !== "number")
        {
            throw new Error("the speeds must be numbers");
        }
        // make sure we only have integers or our test in the function
        // below will fail
        speeds[idx] = Math.round(speeds[idx]);
        if(speeds[idx] < 0)
        {
            throw new Error("speeds must all be positive or null");
        }
    }

    this.waitingPeriods_ = speeds;
};


/** \brief Check whether this request should start immediately.
 *
 * This function returns true if the request was assigned an array of
 * speeds that starts with zero.
 *
 * \note
 * Using a zero at the start is really only effective if the request is
 * the only one at the time it gets added. In all other cases, this will
 * not happen.
 *
 * @return {boolean}  true if this request should be sent to the server.
 */
snapwebsites.ListenerRequest.prototype.immediate = function()
{
    return this.waitingPeriods_ !== null
        && this.waitingPeriods_.length > 0
        && this.waitingPeriods_[0] === 0;
};


/** \brief Check whether the request ends now.
 *
 * This function is used internally to know whether the request is
 * done.
 *
 * Being done means that "enough" requests were sent to the server
 * and doing more would not be useful.
 *
 * @return {boolean}  This function returns true if the request was
 *                    already sent too many times.
 */
snapwebsites.ListenerRequest.prototype.done = function()
{
    if(this.processedCounter_ >= this.maxRetry_)
    {
        this.processedCounter_ = this.maxRetry_ + 1; // allow for timedout() to return true
        if(this.error_)
        {
            this.error_(this);
        }
        if(this.complete_)
        {
            this.complete_(this);
        }
        return true;
    }

    return false;
};


/** \brief Check whether the request timed out.
 *
 * This function checks whether the number of times the request
 * was processed and if more than the maximum allowed, then it
 * returns true.
 *
 * When your failure callback gets called, it may call this function
 * to know whether the error was a timeout.
 *
 * @return {boolean}  Return true if the listener timed out.
 */
snapwebsites.ListenerRequest.prototype.timedout = function()
{
    return this.processedCounter_ > this.maxRetry_;
};


/** \brief Check whether this request should be sent on the next AJAX call.
 *
 * This function checks whether the request should be sent on this timer
 * tick. The timerTick() function should have been called BEFORE this
 * function gets called.
 *
 * \note
 * This function is considered to be an internal function (i.e. only
 * called internally by the listener scripts.)
 *
 * @return {boolean}  true if this request should be sent to the server.
 */
snapwebsites.ListenerRequest.prototype.ready = function()
{
    var counter,
        max,
        idx;

    // ignore empty arrays, always true in this case
    if(this.waitingPeriods_ === null
    || this.waitingPeriods_.length === 0)
    {
        ++this.processedCounter_;
        return true;
    }

    counter = this.timerCounter_;
    max = this.waitingPeriods_.length - 1;
    idx = 0;

    while(counter > 0)
    {
        counter -= this.waitingPeriods_[idx];
        if(counter === 0)
        {
            ++this.processedCounter_;
            return true;
        }
        if(idx < max) // note: max is the last offset (inclusive)
        {
            ++idx;
        }
    }

    return false;
};


/** \brief Call on each tick.
 *
 * Each time the timer times out, this function gets called on
 * all the requests. This increments the "internal timer" of
 * the request.
 *
 * \note
 * This function is considered to be an internal function (i.e. only
 * called internally by the listener scripts.) Yet it cannot be made
 * private since we do not support the `friend` keyword.
 */
snapwebsites.ListenerRequest.prototype.timerTick = function()
{
    // no need to check for overflows since +1 per second takes some
    // 90 years to overflow a 32 bit number
    ++this.timerCounter_;
};


/** \brief Set the resulting XML as returned by the server.
 *
 * This function is considered to be an internal function. It is used
 * by the Listener to save this request XML data as returned by
 * the server.
 *
 * In your code, you can retrieve the result with the use of the
 * getResult() function. The result can be used with jQuery to
 * retrieve the data available:
 *
 * \code
 * s = jQuery("result", request.getResult()).attr("status");
 * \endcode
 *
 * @param {Document} result  The XML document returned by the server.
 */
snapwebsites.ListenerRequest.prototype.setResult = function(result)
{
    this.resultXML_ = result;
};


/** \brief Retrieve the resulting XML returned by the server.
 *
 * This function allows you to retrieve the XML returned by the server.
 * jQuery can then be used to navigate the XML content and retrieve
 * the data that interests your script.
 *
 * \code
 * s = jQuery("result", request.getResult()).attr("status");
 * \endcode
 *
 * Note that in most cases just the fact that your callback gets
 * called means that you are having success and thus you do not
 * need more data from the XML.
 *
 * \warning
 * The document may still be 'null' if your failure callback function
 * is called. This is because failures may happen for reasons other
 * than the server. And in that case, the Listener does not create
 * an XML document. (It might at a later time so we can pass error
 * messages to your.)
 *
 * @return {Document} The XML document returned by the server.
 * @const
 */
snapwebsites.ListenerRequest.prototype.getResult = function()
{
    return this.resultXML_;
};


/** \brief Call one of the callbacks.
 *
 * This function is used to call the success (true) or failure (false)
 * callback.
 *
 * @param {boolean} result_status  Defines which callback should be called.
 */
snapwebsites.ListenerRequest.prototype.callCallback = function(result_status)
{
    if(result_status)
    {
        if(this.success_)
        {
            this.success_(this);
        }
    }
    else
    {
        if(this.error_)
        {
            this.error_(this);
        }
    }
    if(this.complete_)
    {
        this.complete_(this);
    }
};



/** \brief Snap Listener constructor.
 *
 * The Listener environment offers one listener object, which accepts any
 * number of requests:
 *
 * \code
 * snapwebsites.ListenerInstance.addRequest(my_request);
 * \endcode
 *
 * All those requests of URI to "listen" to are used to poll the server.
 * The client receives a callback once the server makes the resource
 * available.
 *
 * A listener request is:
 *
 * \li A URI representing the resource the client is waiting for,
 * \li An action, (defaults to "a=view",)
 * \li Additional parameters (i.e. to get an image of a given width x height, width=123&height=123.)
 *
 * which are used to know whether a resource is available. Once available
 * the server responds with a positive answer and lets the client actually
 * query the information (by using said URI directly, although that is the
 * responsibility of the object that created the listener request.)
 *
 * One instance of this class is created on load. You should use that
 * instance to add your requests to the Snap! server.
 *
 * \code
 * class Listener : public ServerAccessCallbacks
 * {
 * public:
 *      function Listener();
 *
 *      virtual function serverAccessSuccess(result: ResultData);
 *      virtual function serverAccessComplete(result: ResultData);
 *
 *      function addRequest(request: ListenerRequest);
 *
 * private:
 *      function processRequests_();
 *
 *      requests_: Array of ListenerRequest;
 *      serverAccess_: ServerAccess;
 *      processing_: boolean;
 *      timeoutID_: number;
 * };
 *
 * var ListenerInstance: Listener;
 * \endcode
 *
 * \note
 * We expect all plugins that need to listen to something from the server
 * to make use of this interface. This way we avoid duplication, special
 * handling on the server which could be non-secure, and should get things
 * done faster, overall.
 *
 * \todo
 * Implement a version with the new HTML5 connection type so we can avoid
 * polling (but make sure that this does not eat a connection "forever".)
 *
 * @constructor
 * @extends snapwebsites.ServerAccessCallbacks
 * @struct
 */
snapwebsites.Listener = function()
{
    this.requests_ = [];

    return this;
};


/** \brief Listener inherits from the ServerAccessCallbacks class.
 *
 * This class inherits from the snapwebsites.ServerAccessCallbacks because
 * it sends AJAX data to the server and it wants to handle the response.
 */
snapwebsites.inherits(snapwebsites.Listener, snapwebsites.ServerAccessCallbacks);


/** \brief The Listener instance.
 *
 * This class is a singleton and as such it makes use of a static
 * reference to itself. It gets created on load.
 *
 * \@type {snapwebsites.Listener}
 */
snapwebsites.ListenerInstance = null; // static


/** \brief Array of requests currently active.
 *
 * Requests are added using the addRequest() function. This allows for the
 * Listener to handle any number of requests.
 *
 * \note
 * The default is set to null otherwise in the event another
 * listener was created (should not be, we execpt only one of those per
 * client,) the array would be shared. It gets properly initialized as
 * an array in the constructor.
 *
 * @type {Array.<snapwebsites.ListenerRequest>}
 * @private
 */
snapwebsites.Listener.prototype.requests_ = null;


/** \brief A server access object.
 *
 * The listener object makes use of a server access object to handle its
 * requests. This object reference is saved in this variable member.
 *
 * It gets allocated only if the object needs to send data to the
 * server (so in most cases: never.)
 *
 * @type {?snapwebsites.ServerAccess}
 * @private
 */
snapwebsites.Listener.prototype.serverAccess_ = null;


/** \brief Whether the listener is currently waiting on the server.
 *
 * Whenever the listener object sends a message to the server, it sets
 * this parameter to true. If the parameter is already true, then the
 * object avoid sending yet another message (since that would just
 * jam the client/server communication port).
 *
 * Note that the client may send as many as one request per second
 * which could be more than the speed at which the client and
 * server currently communicate.
 *
 * \todo
 * Calculate the rate at which the client and server communicate to
 * make sure to slow it down if required.
 *
 * @type {boolean}
 * @private
 */
snapwebsites.Listener.prototype.processing_ = false;


/** \brief Timer used to pull the server.
 *
 * At this point we poll the server to know whether the requested URIs are
 * available or not.
 *
 * \todo
 * Implement a version that makes use of the HTML5 capability which
 * allows for a more permanent connection, which can then be used
 * to push the answer as soon as the data is ready, and push the
 * query only once.
 *
 * @type {number}
 * @private
 */
snapwebsites.Listener.prototype.timeoutID_ = NaN;


/** \brief Function called on AJAX success.
 *
 * This function is called if the remote access was successful. The
 * result object includes a reference to the XML document found in the
 * data sent back from the server.
 *
 * @param {snapwebsites.ServerAccessCallbacks.ResultData} result  The
 *          resulting data.
 */
snapwebsites.Listener.prototype.serverAccessSuccess = function(result) // virtual
{
    var data_tags = result.jqxhr.responseXML.getElementsByTagName("data"),
        idx,                    // index going through all the data tags
        name,                   // name of the data tag, should be "listener"
        uri,                    // the URI of the request
        r,                      // the key of the request
        request,                // the request object that matched uri
        request_status,         // the status of this request
        result_xml,             // the XML data returned for that request
        result_tag;             // the result tag in the XML data returned

    snapwebsites.Listener.superClass_.serverAccessSuccess.call(this, result);

    for(idx = 0; idx < data_tags.length; ++idx)
    {
        // make sure it is one of our parameters
        name = data_tags[idx].getAttribute("name");
        if(name === "listener")
        {
            result_xml = jQuery.parseXML(data_tags[idx].childNodes[0].nodeValue);
            result_tag = jQuery("result", result_xml);
            uri = result_tag.attr("href");
            request = null;
            for(r in this.requests_)
            {
                if(this.requests_.hasOwnProperty(r))
                {
                    // we save the data using a new index to make sure it goes
                    // from 0 to n
                    if(this.requests_[r].getURI() === uri)
                    {
                        request = this.requests_[r];
                        break;
                    }
                }
            }
            if(request)
            {
                request.setResult(result_xml);
                request_status = result_tag.attr("status");
                switch(request_status)
                {
                case "failed":
                    // status failed, forget about that request completely
                    this.requests_.splice(r, 1);
                    request.callCallback(false);
                    break;

                case "wait":
                    // in this case, nothing is available yet...
                    break;

                case "success":
                    // the resource is available, we are done with the request
                    this.requests_.splice(r, 1);
                    request.callCallback(true);
                    break;

                }
            }
            // else -- request not found ignore for now...
        }
    }
};


/** \brief Function called on AJAX completion.
 *
 * This function is called once the whole process is over. It is most
 * often used to do some cleanup.
 *
 * Here we mark that we are done with the processing of the last sent
 * request. This means a new request can be sent now.
 *
 * @param {snapwebsites.ServerAccessCallbacks.ResultData} result  The
 *          resulting data with information about the error(s).
 */
snapwebsites.Listener.prototype.serverAccessComplete = function(result) // virtual
{
    // done processing this set of requests
    this.processing_ = false;

    // manage messages if any
    snapwebsites.Listener.superClass_.serverAccessComplete.call(this, result);
};


/** \brief Attach a request to the listener.
 *
 * This function adds a request object to the listner. That request
 * will be sent the next time the timer wakes up.
 *
 * Requests added in this way stay in the Listener instance
 * until the server answers with a valid AJAX response. The response can
 * tell the client that whatever was being requested for availability
 * is now available or the request cannot ever be answered positively
 * (i.e. the item requested does not exist at all.)
 *
 * \todo
 * Allow for instant requests, at this time, a new request is likely
 * to be handled 1 second later (Especially the first one added.)
 *
 * @param {snapwebsites.ListenerRequest} request  The request
 *            data to save in the server access listener.
 */
snapwebsites.Listener.prototype.addRequest = function(request)
{
    var that = this;

    this.requests_.push(request);

    // only us? (if not we may be processing right now)
    if(isNaN(this.timeoutID_))
    {
        // note that we already know that no other request is currently
        // running, so we're good here
        if(request.immediate())
        {
            this.processRequests_();
        }
        else
        {
            // repeat once every second
            //
            // Note: I do not use a setInterval() so that way the time it
            //       takes to manage one request is taken in account
            //       (hopefully) up to the next request; this means the
            //       requests should not all happen at an even second
            this.timeoutID_ = setTimeout(
                function()
                {
                    that.processRequests_();
                },
                1000);
        }
    }
};


/** \brief Process all the requests once.
 *
 * This function is called once per second to process the existing
 * requests. If the server had not yet answered the last request,
 * then nothing happens.
 *
 * @private
 */
snapwebsites.Listener.prototype.processRequests_ = function()
{
    var that = this,
        idx = 0,
        obj = {},
        r,
        clean_requests = [];

    // ***********
    // *********** WARNING:
    // *********** DO NOT SET timerID_ TO -1 HERE!!!
    // *********** WARNING:
    // ***********
    // *********** Although the counter is officially "out", resetting the
    // *********** variable to -1 would break the code by allowing another
    // *********** request from being added and this function called when
    // *********** we are still managing another request!
    // ***********

    // last request ended?
    if(!this.processing_)
    {
        // first we remove the requests which reached they total delay
        // (some may go forever, but those we can remove, remove them!)
        for(r in this.requests_)
        {
            if(this.requests_.hasOwnProperty(r))
            {
                // we save the data using a new index to make sure it goes
                // from 0 to n
                this.requests_[r].timerTick();
                if(!this.requests_[r].done())
                {
                    clean_requests.push(this.requests_[r]);
                }
            }
        }
        this.requests_ = clean_requests;
        if(this.requests_.length === 0)
        {
            // no more requests, stop everything
            this.timeoutID_ = NaN;
            return;
        }

        // retrieve all the requests that are ready to be processed
        for(r in this.requests_)
        {
            if(this.requests_.hasOwnProperty(r))
            {
                // we save the data using a new index to make sure it goes
                // from 0 to n
                if(this.requests_[r].ready())
                {
                    obj["uri" + idx] = this.requests_[r].getURI();
                    ++idx;
                }
            }
        }

        // anything to process this time around?
        // (this happens if your requests are to be checked every
        // few seconds instead of every second.)
        if(idx !== 0)
        {
            // prepare the server access object and then call send()
            this.processing_ = true;

            if(!this.serverAccess_)
            {
                this.serverAccess_ = new snapwebsites.ServerAccess(this);
                this.serverAccess_.setURI(snapwebsites.castToString(
                            jQuery("link[rel='page-uri']").attr("href"),
                            "casting href of the page-uri link to a string in snapwebsites.Listener.processRequests_()"));
            }

            // the number of URI sent
            obj._listener_size = idx;

            this.serverAccess_.setData(obj);
            this.serverAccess_.send();
            // TODO: if the serverAccess send() function fails without
            //       ending up calling the complete() function below
            //       then we lose all processing capabilities...
        }
    }

    // we want to reset the time out by hand each time
    //
    // Note: I do not use a setInterval() so that way the time it
    //       takes to manage one request is taken in account
    //       (hopefully) up to the next request; this means the
    //       requests should not all happen at an even second
    this.timeoutID_ = setTimeout(
        function()
        {
            that.processRequests_();
        },
        1000);
};


// auto-initialize
jQuery(document).ready(
    function()
    {
        snapwebsites.ListenerInstance = new snapwebsites.Listener();
    }
);
// vim: ts=4 sw=4 et
