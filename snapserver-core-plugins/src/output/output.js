/** @preserve
 * Name: output
 * Version: 0.1.7.14
 * Browsers: all
 * Copyright: Copyright 2014-2017 (c) Made to Order Software Corporation  All rights reverved.
 * Depends: jquery-extensions (1.0.2)
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
// ==/ClosureCompiler==
//

/*jslint nomen: true, todo: true, devel: true */
/*global jQuery: false, Uint8Array: true */


/** \brief Defines the snapwebsites namespace in the JavaScript environment.
 *
 * All the JavaScript functions defined by Snap! plugins are defined inside
 * the snapwebsites namespace. For example, the WYSIWYG Snap! editor is
 * defined as:
 *
 * \code
 * snapwebsites.Editor
 * \endcode
 *
 * \note
 * Technically, this is an object.
 *
 * @type {Object}
 */
var snapwebsites = {};


/** \brief Helper function for base classes that can be inherited from.
 *
 * Base classes (those that do not inherit from other classes) can call
 * function function to get initialized so it works as expected.
 *
 * See the example in the snapwebsites.inherit() function.
 *
 * WARNING: the prototype is setup with this function so you cannot
 *          use blah.prototype = { ... };.
 *
 * @param {!function(...)} base_class  The class to initialize.
 *
 * @final
 */
snapwebsites.base = function(base_class)
{
//#ifdef DEBUG
    base_class.snapwebsitesBased_ = true;
//#endif
    base_class.prototype.constructor = base_class;
};


/** \brief Helper function for inheritance support.
 *
 * This function is used by classes that want to inherit from another
 * class. At this point we do not allow multiple inheritance though.
 *
 * The following shows you how it works:
 *
 * \code
 * // class A -- constructor
 * snapwebsites.A = function()
 * {
 *      ...
 * };
 *
 * snapwebsites.base(snapwebsites.A);
 *
 * // class A -- a method
 * snapwebsites.A.prototype.someMethod = function(q)
 * {
 *      ...
 * }
 *
 * // class B -- constructor
 * snapwebsites.B = function()
 * {
 *      snapwebsites.B.superClass_.constructor.call(this);
 *      // with parameters: snapwebsites.B.superClass_.constructor.call(this, a, b, c);
 *      ...
 * };
 *
 * // class B : public A -- define inheritance
 * snapwebsites.inherits(snapwebsites.B, snapwebsites.A);
 *
 * // class B : extend with more functions
 * snapwebsites.B.prototype.moreStuff = function()
 * {
 *      ...
 * };
 *
 * // class B : override function
 * snapwebsites.B.prototype.someMethod = function(p, q)
 * {
 *      ...
 *      // call super class method (optional)
 *      snapwebsites.B.superClass_.someMethod.call(this, q);
 *      ...
 * };
 * \endcode
 *
 * @param {!function(...)} child_class  The constructor of the child
 *                                      class (the on inheriting.)
 * @param {!function(...)} super_class  The constructor of the parent
 *                                      class (the to inherit from.)
 *
 * @final
 */
snapwebsites.inherits = function(child_class, super_class) // static
{
    /** \brief Intermediate constructor.
     *
     * In case Object.create() is not available (IE8 and older) we
     * want to have a function to hold the super class prototypes
     * and be able to do a new() without parameters. This is the
     * function we use for that purpose.
     *
     * @constructor
     */
    function C() {}

//#ifdef DEBUG
    if(!super_class.snapwebsitesBased_
    && !super_class.snapwebsitesInherited_)
    {
        throw new Error("Super class was not based or inherited");
    }
//#endif

    if(Object.create)
    {
        // modern inheriting is probably faster than
        // old browsers (and at some point we will
        // have the preprocessor which will select
        // one or the other part)
        child_class.prototype = Object.create(super_class.prototype);
    }
    else
    {
        // older browsers don't have Object.create and
        // this is how you inherit properly in that case
        C.prototype = super_class.prototype;
        child_class.prototype = new C();
    }
    child_class.prototype.constructor = child_class;
    child_class.superClass_ = super_class.prototype;
    child_class.snapwebsitesInherited_ = true;
};


/** \brief Checks a string and remove any spaces and new lines.
 *
 * This function removes starting and ending spaces and new lines
 * from a string. A newline may be represened by a \<br> tag
 * (especially with IE.)
 *
 * \todo
 * TBD: should we trim any \&nbsp; character too? The fact is that
 * the "no break spaces" are not totally removed so keep them probably
 * makes more sense.
 *
 * @param {string} s  The string to trim.
 *
 * @return {string}  The trimmed string.
 */
snapwebsites.trim = function(s)
{
    var i,
        start,
        end,
        sre = /^( |\t|\r|\n|<br *\/?>)/,
        ere = /( |\t|\r|\n|<br *\/?>)$/;

    for(i = s.length - 1; i > Math.max(0, s.length - 6); --i)
    {
        end = s.substr(i);
        if(end.match(ere))
        {
            s = s.substr(0, i);
            i = s.length;
        }
    }

    for(i = 1; i < Math.max(6, s.length); ++i)
    {
        start = s.substr(0, i);
        if(start.match(sre))
        {
            s = s.substr(i);
            i = 0;
        }
    }

    return s;
};


/** \brief Convert XML to a string.
 *
 * This function is used to convert an XML element and all of its children
 * in a string.
 *
 * \todo
 * Transform to a jQuery extension instead.
 *
 * @param {jQuery} xml_object  A jQuery object representing an XML tag.
 *
 * @return {string}  The string representation of the XML DOM passed in.
 */
snapwebsites.xmlToString = function(xml_object) // static
{
    var element,
        start,
        end,
        serializer,
        result;

    if(xml_object.length == 0)
    {
        return "";
    }

    try
    {
        // this works in case xml_object is an empty list
        return snapwebsites.castToString(xml_object.html(), "expected html() to return a string in xmlToString()");
    }
    catch(e)
    {
        // IE has a problem with .html() because XML documents
        // do not have an innerHTML parameter (which makes sense!)

        // older IE versions have an xml property
        element = xml_object.get()[0];
        if(element.xml)
        {
            // Internet Explorer
            return element.xml();
        }

        serializer = new XMLSerializer();
        result = serializer.serializeToString(element);

        // the serialization includes the parent tag
        // so here we manually remove it
        //
        // TBD: find a better (faster) way to handle this problem?
        start = result.indexOf(">");
        end = result.lastIndexOf("<");
        return result.substr(start + 1, end - start - 1);
    }
};


/** \brief This function returns the size of a map.
 *
 * This function computes the size of a map. This is considered a
 * very slow function so you should hold on the result.
 *
 * Since there is not much you can do with the result, other than
 * having a count, it is expected that the function will only be
 * used for debug purposes.
 *
 * \todo
 * Should this be only between '#ifdef DEBUG / #endif' preprocessor
 * entries?
 *
 * @param {Object} o  An object to size.
 *
 * @return {number}  The number of items in this map.
 */
snapwebsites.mapSize = function(o) // static
{
    var c,
        p;

    // Old FF
    if(o["__count__"] !== undefined)
    {
        return o["__count__"];
    }

    // ES5
    if(Object.keys)
    {
        return Object.keys(o).length;
    }

    // Otherwise
    c = 0;
    for(p in o)
    {
        if(o.hasOwnProperty(p))
        {
            ++c;
        }
    }

    return c;
};


/** \brief Helper function: generate hexadecimal number.
 *
 * This function transform byte \p c in a hexadecimal number of
 * exactly two digits.
 *
 * Note that \p c can be larger than a byte, only it should probably
 * not be negative.
 *
 * @param {number} c  The byte to transform (expected to be between 0 and 255)
 *
 * @return {string}  The hexadecimal representation of the number.
 */
snapwebsites.charToHex = function(c) // static
{
    var a, b;

    a = c & 15;
    b = (c >> 4) & 15;
    return String.fromCharCode(b + (b >= 10 ? 55 : 48))
         + String.fromCharCode(a + (a >= 10 ? 55 : 48));
};


/** \brief Replace certain characters by their identity.
 *
 * When adding a string to an HTML attribute, you want to escape a certain
 * number of characters. This function transforms those few characters as
 * follow:
 *
 * \li & -- changed to &amp;
 * \li " -- changed to &quot;
 * \li ' -- changed to &#39; -- &apos; is not considered a valid HTML entity...
 * \li < -- changed to &lt;
 * \li > -- changed to &gt;
 *
 * Other characters do not have to be converted even if they could be. This
 * way the function is simply faster.
 *
 * @param {string} str  The input string to convert.
 *
 * @return {string}  The converted string.
 */
snapwebsites.htmlEscape = function(str) // static
{
    return str.replace(/&/g, '&amp;')
              .replace(/"/g, '&quot;')  // " vim does not see regex's properly
              .replace(/'/g, '&#39;')
              .replace(/</g, '&lt;')
              .replace(/>/g, '&gt;');
};


/** \brief Remove HTML tags from a string.
 *
 * When you get a value of a widget or other similar data, it may include
 * HTML tags. This function removes such tags from the string and returns
 * the result.
 *
 * If you want to strip out certain tags and keep others, look at the
 * stripTag() function instead.
 *
 * @param {string} html_str  The input string to convert.
 *
 * @return {string}  The converted string.
 *
 * \sa stripTag()
 */
snapwebsites.stripAllTags = function(html_str) // static
{
    // Note that this regex removes all tags that are generall considered
    // valid; although frankly it does not check the tag per se, only the
    // syntax of such; it will keep lone < and unmatched > characters.
    //
    return html_str.replace(/<(?:\w|\/)[^>]*>/g, '');
};


/** \brief Remove HTML tags except those considered acceptable.
 *
 * The function removes all tags that are not defined in the allowed
 * string from the \p html_str input string. It makes heavy use
 * of regular expression so it can be a bit slow and should probably
 * not be used on really large buffers.
 *
 * The \p opt_allowed_tags parameter can be defined as a string or an array.
 * When defined as a string, it must be a list of tags written in lowercase
 * and separated by commas: 'a,b,h1,h2,h3,h4,h5,h6,span'. The function
 * does not verify the validity of the \p opt_allowed_tags string. However, if
 * undefined or empty, the function calls stripAllTags() instead.
 *
 * \code
 *      // called with an array
 *      stripTags("<b>string</b> with <em>HTML</em> tags", ["b","em"]);
 *
 *      // called with a string equivalent to the array
 *      stripTags("<b>string</b> with <em>HTML</em> tags", "b,em");
 * \endcode
 *
 * \note
 * This function is based on the code found on the PHP website:
 * http://phpjs.org/functions/strip_tags/
 *
 * @param {!string} html_str  The string where tags are to be stripped.
 * @param {string|Array.<string>} opt_allowed_tags  The tags that can stay in your string.
 *
 * @return {string}  The \p html_str string without tags except those
 *                   in the \p opt_allowed_tags string.
 */
snapwebsites.stripTags = function(html_str, opt_allowed_tags) // static
{
    if(!opt_allowed_tags || opt_allowed_tags === '')
    {
        return snapwebsites.stripAllTags(html_str);
    }
    // transform the string in an array
    if(!jQuery.isArray(opt_allowed_tags))
    {
        opt_allowed_tags = opt_allowed_tags.split(",");
    }
    return html_str.replace(
                /<\/?([a-z][a-z0-9:]*)\b[^>]*>/gi,
                function(full_match, first_match)
                    {
                        return opt_allowed_tags.indexOf(first_match.toLowerCase()) != -1 ? full_match : '';
                    });
};


/** \brief Make sure a parameter is a string.
 *
 * This function makes sure the parameter is a string, if not it
 * throws.
 *
 * This is useful in situations where a function may return something
 * else than a string.
 *
 * As you can see the function does not do anything to the parameter,
 * only the closure compiler sees a "string" coming out.
 *
 * @param {Object|string|number|boolean} s  Expects a string as input.
 * @param {string} e  An additional error message in case it fails.
 *
 * @return {string}  The input string after making sure it is a string.
 */
snapwebsites.castToString = function(s, e) // static
{
    if(typeof s !== "string")
    {
        throw new Error("a string was expected, got a \"" + (typeof s) + "\" instead (" + e + ")");
    }
    return s;
};


/** \brief Make sure a parameter is a number.
 *
 * This function makes sure the parameter is a number, if not it
 * throws.
 *
 * This is useful in situations where a function may return something
 * else than a number.
 *
 * As you can see the function doesn't do anything to the parameter,
 * only the closure compiler sees a "number" coming out.
 *
 * @param {Object|string|number|boolean} n  Expects a number as input.
 * @param {string} e  An additional error message in case it fails.
 *
 * @return {number}  The input number after making sure it is a number.
 */
snapwebsites.castToNumber = function(n, e) // static
{
    if(typeof n !== "number")
    {
        throw new Error("a number was expected, got a \"" + (typeof n) + "\" instead (" + e + ")");
    }
    return n;
};


/** \brief Compare two numbers for the array sort.
 *
 * By default, the JavaScript Array.sort() function sorts as if all
 * the elements were strings, no matter what. This function can be
 * used to sort an array of numbers as follow:
 *
 * \code
 *  var numbers = [ 10, 5, 1, 3, 20 ];
 *  numbers.sort();
 *  // [ 1, 10, 20, 3, 5 ]
 *  numbers.sort(snapwebsites.compareNumbers);
 *  // [ 1, 3, 5, 10, 20 ]
 * \endcode
 *
 * \note
 * The numbers get compared with the smallest possible epsilon available
 * for them at the time they get compared.
 *
 * @param {number} a  The left hand side number.
 * @param {number} b  The right hand side number.
 *
 * @return {number}  Zero when 'a' and 'b' are equal, a negative number
 *         when 'b' is larger, and a positive number when 'a' is larger.
 */
snapwebsites.compareNumbers = function(a, b) // static
{
    return a - b;
};



/** \brief A template used to define a set of buffer to MIME type scanners.
 *
 * This template class defines a function used by the Output.bufferToMIME()
 * function to determine the type of a file. The function expects a buffer
 * which is converts to a Uint8Array and sends to the
 * BufferToMIMETemplate.bufferToMIME() and overrides of that function to
 * determine the different file types supported (i.e. accepted) by the
 * system.
 *
 * The system inheritance is as follow:
 *
 * \code
 *   +---------------------------+
 *   |                           |
 *   |  BufferToMIMETemplate     |
 *   |                           |
 *   +---------------------------+
 *        ^
 *        | Inherit
 *        |
 *   +---------------------------+
 *   |                           |
 *   |  BufferToMIMESystemImage  |
 *   |                           |
 *   +---------------------------+
 * \endcode
 *
 * @return {!snapwebsites.BufferToMIMETemplate} A reference to this new object.
 *
 * @constructor
 * @struct
 */
snapwebsites.BufferToMIMETemplate = function()
{
    return this;
};


/** \brief Mark the template as a base.
 *
 * This call marks the BufferToMIMETempate a base. This means you
 * can inherit from it.
 */
snapwebsites.base(snapwebsites.BufferToMIMETemplate);


/*jslint unparam: true */
/** \brief Check a buffer magic codes.
 *
 * This function is expected to check the different MIME types supported
 * by your plugin. The function must return a string representing a MIME
 * type. If your function does not recognize that MIME type, then return
 * an empty string.
 *
 * The template function just returns "" so you do not need to call it.
 *
 * @param {!Uint8Array} buf  The buffer of data to be checked.
 *
 * @return {!string}  The MIME type you determined, or the empty string.
 */
snapwebsites.BufferToMIMETemplate.prototype.bufferToMIME = function(buf) // virtual
{
    return "";
};
/*jslint unparam: false */



/** \brief Check for "system" images.
 *
 * This function checks for well known images. The function is generally
 * very fast because it checks only the few very well known image file
 * formats.
 *
 * @return {!snapwebsites.BufferToMIMESystemImages} A reference to this new
 *                                                  object.
 *
 * @extends {snapwebsites.BufferToMIMETemplate}
 * @constructor
 */
snapwebsites.BufferToMIMESystemImages = function()
{
    snapwebsites.BufferToMIMESystemImages.superClass_.constructor.call(this);

    return this;
};


/** \brief Chain up the extension.
 *
 * This is the chain between this class and it's super.
 */
snapwebsites.inherits(snapwebsites.BufferToMIMESystemImages, snapwebsites.BufferToMIMETemplate);


/** \brief Check for most of the well known image file formats.
 *
 * This function checks for the well known image file formats that
 * we generally want the system to support. This includes:
 *
 * \li JPEG
 * \li PNG
 * \li GIF
 *
 * Other formats will be added with time:
 *
 * \li SVG
 * \li BMP/ICO
 * \li TIFF
 * \li ...
 *
 * @param {!Uint8Array} buf  The array of data to check for a known magic.
 *
 * @return {!string} The MIME type or the empty string if empty.
 *
 * @override
 */
snapwebsites.BufferToMIMESystemImages.prototype.bufferToMIME = function(buf)
{
    // Image JPEG
    if(buf[0] === 0xFF
    && buf[1] === 0xD8
    && buf[2] === 0xFF
    && buf[3] === 0xE0
    && buf[4] === 0x00
    && buf[5] === 0x10
    && buf[6] === 0x4A  // J
    && buf[7] === 0x46  // F
    && buf[8] === 0x49  // I
    && buf[9] === 0x46) // F
    {
        return "image/jpeg";
    }
    // Picassa has a comment before the JFIF
    if(buf[0] === 0xFF
    && buf[1] === 0xD8
    && buf[2] === 0xFF
    && buf[3] === 0xE0
    && buf[4] === 0x00
    && buf[5] === 0x08
    && buf[6] === 0x50  // P
    && buf[7] === 0x69  // i
    && buf[8] === 0x63  // c
    && buf[9] === 0x61  // a
    && buf[10] === 0x73  // s
    && buf[11] === 0x61) // a
    {
        return "image/jpeg";
    }

    // Image PNG
    if(buf[0] === 0x89
    && buf[1] === 0x50  // P
    && buf[2] === 0x4E  // N
    && buf[3] === 0x47  // G
    && buf[4] === 0x0D  // \r
    && buf[5] === 0x0A) // \n
    {
        return "image/png";
    }

    // Image GIF
    if(buf[0] === 0x47  // G
    && buf[1] === 0x49  // I
    && buf[2] === 0x46  // F
    && buf[3] === 0x38  // 8
    && buf[4] === 0x39  // 9
    && buf[5] === 0x61) // a
    {
        return "image/gif";
    }

    return "";
};



/** \brief Snap Output Manipulations.
 *
 * This class initializes and handles the different output objects.
 *
 * \note
 * The Snap! Output is a singleton and should never be created by you. It
 * gets initialized automatically when this output.js file gets included.
 *
 * @return {!snapwebsites.Output}  This object reference.
 *
 * @constructor
 * @struct
 */
snapwebsites.Output = function()
{
    this.registeredBufferToMIME_ = [];
    this.handleMessages_();

    // clearly mark the HTML tag with "js" so CSS knows that JavaScript
    // is supported
    //
    jQuery("html").removeClass("no-js");
    jQuery("html").addClass("js");

    return this;
};


/** \brief The output is a base class even though it is unlikely derived.
 *
 * This class is marked as a base class, although it is rather unlikely
 * that we'd need to derive from it.
 */
snapwebsites.base(snapwebsites.Output);


/** \brief The Output instance.
 *
 * This class is a singleton and as such it makes use of a static
 * reference to itself. It gets created on load.
 *
 * \@type {snapwebsites.Output}
 */
snapwebsites.OutputInstance = null; // static


/** \brief Holds the array of query string values if any.
 *
 * This variable member is an array of the query string values keyed on
 * their names.
 *
 * By default this parameter is undefined. It gets defined the first time
 * you call the qsParam() function.
 *
 * @type {Object}
 * @private
 */
snapwebsites.Output.prototype.queryString_ = null;


/** \brief Array of objects that know about magic codes.
 *
 * This variable member holds an array of objects that can convert a
 * buffer magic code in a MIME type. The system offers a limited number
 * of types recognition. In most cases that is enough, although at times
 * it would be more than useful to support many more formats and this
 * array is used for that purpose. To register additional supportive
 * classes, use the function:
 *
 * \code
 * registerBufferToMIME(MyBufferToMIMEExtension);
 * \endcode
 *
 * @type {Array.<snapwebsites.BufferToMIMETemplate>}
 * @private
 */
snapwebsites.Output.prototype.registeredBufferToMIME_; // = []; -- initialized in constructor to avoid potential problems


/** \brief Initialize the Query String parameters.
 *
 * This function is called to make sure that the Query String
 * parameters are properly initialized. Functions such as
 * the qsParam() one call this function the first time they
 * are called.
 *
 * Valid parameters are those that have an equal sign and
 * that have valid UTF-8 values (either as such or encoded).
 *
 * This function can be called multiple times, although really
 * it should be called only once since the query string is not
 * expected to change for the duration of a page lifetime.
 *
 * @private
 */
snapwebsites.Output.prototype.initQsParams_ = function()
{
    this.queryString_ = snapwebsites.Output.parseQueryString(document.location.search.replace(/^\?/, ""), false);
};


/** \brief Retrieve a parameter from the query string.
 *
 * This function reads the query string of the current page and retrieves
 * the named parameter.
 *
 * Note that parameters that are not followed by an equal sign or that
 * have "invalid" values (not valid UTF-8) will generally be ignored.
 *
 * @param {!string} name  A valid query string name.
 * @return {string}  The value of that query string if defined, otherwise
 *                   the "undefined" value.
 */
snapwebsites.Output.prototype.qsParam = function(name)
{
    if(this.queryString_ === null)
    {
        this.initQsParams_();
    }

    // if it was not defined, then this returns "undefined"
    return this.queryString_[name];
};


/** \brief Internal function used to display the error messages.
 *
 * This function is used to display the error messages that occured
 * "recently" (in most cases, this last access, or the one before.)
 *
 * This function is called by the init() function and shows the
 * messages if any were added to the DOM.
 *
 * \note
 * This is here because the messages plugin cannot handle the output
 * of its own messages (it is too low level a plugin.)
 *
 * @private
 */
snapwebsites.Output.prototype.handleMessages_ = function()
{
    var that = this;

    // put a little delay() so we see the fadeIn(), eventually
    //
    jQuery("div.user-messages")
        .each(function()
            {
                var z = jQuery(".zordered").maxZIndex() + 1;
                jQuery(this).css("z-index", z);
            })
        .delay(250)
        .fadeIn(300)
        .children("div.user-message-box")
            .click(function(e)
                {
                    // click to close, but do not react if the user clicked
                    // a link
                    //
                    if(!(jQuery(e.target).is("a")))
                    {
                        that.hideMessages();
                    }
                });
};


/** \brief Hide the message box if it is open.
 *
 * This function makes sure that the message box is closed.
 *
 * In most cases it is called whenever the system sent a
 * successful AJAX to the server and no messages were
 * displayed from it.
 */
snapwebsites.Output.prototype.hideMessages = function()
{
    jQuery("div.user-messages").fadeOut(300);
};


/** \brief Retrieve the number of messages of a certain type.
 *
 * This function can be used to know whether there are messages of a
 * certain type currently shown on the screen.
 *
 * You may specify the type of message. If not specified, 'error' is
 * assumed.
 *
 * We also support the special name "total" to get the total numebr
 * of messages, whatever their type.
 *
 * @param {string} opt_type  Whether this is an "info", "warning" or "error"
 *                 message (optional, defaults to "error"), or "total" for
 *                 a count of all the messages, whatever their type.
 *
 * @return {integer}  The number of message of the specified type.
 */
snapwebsites.Output.prototype.countMessages = function(opt_type)
{
    var msg = jQuery("div.user-messages"),
        msg_box,
        visible = false,
        errors = 0,
        warnings = 0,
        total = 0;

    // any messages at all?
    //
    if(!msg.exists())
    {
        return 0;
    }

    // if currently hidden, it is also considered that we have no messages
    //
    visible = msg.is(":visible");
    if(!visible)
    {
        return 0;
    }

    // setup a default if undefined
    //
    if(!opt_type)
    {
        opt_type = "error";
    }

    msg_box = msg.children("div.user-message-box");
    if(opt_type == "total")
    {
        // count all messages
        //
        return msg_box.children("div.message").length;
    }

    // only count messages of the specified type
    //
    return msg_box.children("div.message.message-" + opt_type).length;
};


/** \brief Display a set of messages.
 *
 * When working with AJAX, you may at times receive standard messages
 * as shown by the XSLT code and handleMessages_() function. To display
 * these messages, use this function.
 *
 * The function:
 *
 * \li Creates a div.user-messages if it is not available yet;
 * \li Deletes the existing messages if the div is currently hidden;
 * \li Appends the new messages at the end of list;
 * \li Fades the 'div' in.
 *
 * See snapwebsites.ServerAccess.onSuccess_() for the origin of the
 * \p xml object.
 *
 * \warning
 * If you have an XML string to call this function, make sure to parse
 * it first because otherwise it may be parsed as HTML and that has
 * side effects against the title and body tags of the message.
 *
 * @param {NodeList} xml  The XML object with the error messages.
 */
snapwebsites.Output.prototype.displayMessages = function(xml)
{
    var msg = jQuery("div.user-messages"),
        msg_box,
        visible = false,
        errors = 0,
        warnings = 0,
        call_handle = false,
        close_button = "<div class='close-button'><img src='/images/snap/close-button.png'/></div>",
        message_box = "<div class='user-message-box zordered'>" + close_button + "</div>";

    // if the list is empty, ignore
    //
    if(!xml || xml.length == 0)
    {
        return;
    }

    if(!msg.exists())
    {
        // that <div class="user-messages"> does not exist yet so create it
        //
        // the main <div> (user-messages) is there to make sure we can get
        // the correct dynamic size; the next <div> (user-message-box)
        // defines the actual message box
        //
        jQuery("body").append("<div class='user-messages'>" + message_box + "</div>");
        msg = jQuery("div.user-messages");
        msg_box = msg.children(".user-message-box");
        call_handle = true;
    }
    else
    {
        // still visible?
        //
        visible = msg.is(":visible");
        if(!visible)
        {
            // remove old errors
            //
            msg.empty();

            // TODO: we should look into only deleting the messages and not
            //       the close button?
            //
            msg.append(message_box);
            msg_box = msg.children(".user-message-box");

            call_handle = true;
        }
        else
        {
            // make sure to keep a tag on the existing number of errors and warnings
            //
            msg_box = msg.children(".user-message-box");
            errors = msg_box.children("div.message.message-error").length;
            warnings = msg_box.children("div.message.message-warning").length;
        }
    }

    if(window.frameElement)
    {
        // we are in an IFRAME, let CSS know that
        msg.addClass("in-iframe");
    }

    jQuery(xml).children("message").each(function()
        {
            // TODO: handle the id="<widget-name>" information by
            //       setting the class="errorneous" on that widget
            //
            // TODO: add debug to test that 'id' and 'type' are valid
            //       for how they get used (i.e. XML TOKEN).
            var m = jQuery(this),
                title = snapwebsites.xmlToString(m.children("title")),
                body = snapwebsites.xmlToString(m.children("body")),
                widget_id = m.attr("id"),
                id = m.attr("msg-id"),  // WARNING: the "id" attribute represents the name of a widget in the editor
                type = m.attr("type"),
                label,
                txt,
                re;

            if(widget_id)
            {
                // mark widget as erroneous when we have a widget identifier
                // (when no error occurs, no widget identifiers are returned)
                jQuery("div[field_name='" + widget_id + "']").addClass("erroneous");

                // TODO: move that part on the server side?
                //
                //       replace widget_id in the title and body with the
                //       widget label
                //
                label = jQuery("label[for='" + widget_id + "']").clone();
                if(label.exists())
                {
                    label.clone();
                    jQuery("span.required", label).remove();
                    label = snapwebsites.castToString(label.html(), "html was expected to return a string for the widget label");
                    label = snapwebsites.trim(label.replace(/[\s]*:[\s]*$/, ""));
                    label = "<strong>" + label + "</strong>";

                    // Note: The \b does not match many things such as the
                    //       double quotes we use in our messages...
                    //re = new RegExp("\b" + widget_id + "\b", "g");
                    re = new RegExp(widget_id, "g");
                    title = title.replace(re, label);
                    body = body.replace(re, label);
                }
            }
            txt = "<div id='" + id
                  + "' class='message message-" + type
                  + (body ? "" : " message-" + type + "-title-only")
                  + "'><h3>"
                  + title + "</h3>"
                  + (body ? "<p>" + body + "</p>" : "")
                  + "</div>";

            if(type == "error")
            {
                ++errors;
            }
            if(type == "warning")
            {
                ++warnings;
            }

            msg_box.append(txt);
        });

    // TBD: the location and if() for this statement looks wrong
    //      the erroneous status should probably always be cleared
    //      although maybe not? (i.e. if we receive a new (set of)
    //      error, does that mean the previous one is still in effect?)
    //
    if(warnings == 0 && errors == 0)
    {
        jQuery("div[field_name!='']").removeClass("erroneous");
    }

    msg_box.toggleClass("warning-messages", warnings > 0);
    msg_box.toggleClass("error-messages", errors > 0);

    if(!visible)
    {
        if(call_handle)
        {
            this.handleMessages_();
        }
        else
        {
            msg.fadeIn(500);
        }
    }
    if(!call_handle)
    {
        // z-index may need updating
        //
        msg.each(function()
            {
                var z;

                jQuery(this).css("z-index", 0);
                z = jQuery(".zordered").maxZIndex() + 1;
                jQuery(this).css("z-index", z);
            });
    }
};


/** \brief Display a message.
 *
 * This helper function can be used to display one message in the user
 * message popup.
 *
 * The message XML is created here from the few parametrs passed to
 * this function. Only the title is mandatory.
 *
 * \note
 * The type of message is not enforced so you can use any string
 * although if set to 'error' then the error counter is automatically
 * increased and if set to 'warning' then the warning counter is
 * automatically increased, and as a bonus those are displayed in
 * corresponding colors in the default version of the message box.
 *
 * @param {!string} title  The title for this error message.
 * @param {!string} message  The error message (optional, you may use null).
 * @param {string} opt_type  Whether this is an 'info', 'warning' or 'error' message
 *                  (optional, defaults to 'error')
 * @param {boolean} opt_message_is_html  Whether the message includes safe HTML
 *        that we want to be displayed (by default we call htmlEscape() on
 *        the message).
 */
snapwebsites.Output.prototype.displayOneMessage = function(title, message, opt_type, opt_message_is_html)
{
    // TODO: get a valid identifier from caller?
    var xml = "<?xml version='1.0'?><messages><message msg-id='no-id' type='"
                + (opt_type ? snapwebsites.htmlEscape(opt_type) : "error")
                + "'><title><span class='message-title'>" + snapwebsites.htmlEscape(title) + "</span></title>";
    if(message)
    {
        xml += "<body><span class='message-body'>";
        if(opt_message_is_html)
        {
            xml += message;
        }
        else
        {
            xml += snapwebsites.htmlEscape(message);
        }
        xml += "</span></body>";
    }
    xml += "</message></messages>";

    // Note that jQuery(xml) parses the string as HTML and thus
    // "mistreats" the <title> and <body> tags, hence the parseXML() call
    this.displayMessages(jQuery.parseXML(xml).getElementsByTagName("messages"));
};


/** \brief Determine the MIME type from a buffer of data.
 *
 * Assuming you got a buffer (generally from a file dropped over a widget)
 * you may want to determine what type of file it is. This function uses
 * the Magic information of the file to return the supposed type (it may
 * still be lying to us...)
 *
 * The function returns a MIME string such as "image/png".
 *
 * \todo
 * Add code to support all the magic as defined by the tool "file".
 * (once we find the source of magic, it will be easy, now the file
 * is compiled...) Source files are on ftp://ftp.astron.com/pub/file/
 * (the home page is http://www.darwinsys.com/file/ )
 *
 * @param {ArrayBuffer} buffer  The buffer to check for a Magic.
 *
 * @return {string}  The MIME type of the file or the empty string for any
 *                   unknown (unsupported) file.
 */
snapwebsites.Output.prototype.bufferToMIME = function(buffer)
{
    var buf = new Uint8Array(buffer),   // buffer to be checked
        i,                              // loop index
        max,                            // # of registered MIME parsers
        mime;                           // the resulting MIME type

    // Give other plugins a chance to determine the MIME type
    max = this.registeredBufferToMIME_.length;
    for(i = 0; i < max; ++i)
    {
        mime = this.registeredBufferToMIME_[i].bufferToMIME(buf);
        if(mime)
        {
            return mime;
        }
    }

    // unknown magic
    return "";
};


/** \brief Register an object which is capable of determine a MIME type.
 *
 * This function allows you to register an object that defines a
 * bufferToMIME() function:
 *
 * \code
 * myObject.prototype.bufferToMIME(buf)
 * {
 *    ...
 * }
 * \endcode
 *
 * That function is passed a Uint8Array buffer. The size of the buffer
 * must be checked. It may be very small or even empty.
 *
 * @param {snapwebsites.BufferToMIMETemplate} buffer_to_mime  An object
 *        that derives from the BufferToMIMETemplate definition.
 */
snapwebsites.Output.prototype.registerBufferToMIME = function(buffer_to_mime)
{
    this.registeredBufferToMIME_.push(buffer_to_mime);
};


/** \brief Parse a query string to an object.
 *
 * This function transforms a query string from a list of name/value
 * pairs to a JavaScript object.
 *
 * If you already have a query string extracted, set the
 * \p include_domain parameter to false. You have to be very careful
 * since in this case, setting the \p include_domain parameter to
 * true would likely make the function completely ignore your query
 * string (because it is not likely to include a question mark (?)
 * character, and when \p include_domain is true, the question mark
 * (?) is mandatory.)
 *
 * If you have a full href (as defined in an anchor) with a domain name
 * followed by a query string, then set the \p include_domain paramter
 * to true. That way the function first removes the domain part.
 *
 * @param {!string} query_string  The query string to parse, may include
 *                                a domain name.
 * @param {boolean} include_domain  Whether the query_string includes a
 *                                  domain name followed by a query string
 *                                  after a question mark (?).
 *
 * @return {Object}  An object representing the query string name/value
 *                   pairs of the query string; the object may be empty
 *                   (i.e. length === 0) although if no query string is
 *                   defined the function returns null instead.
 */
snapwebsites.Output.parseQueryString = function(query_string, include_domain) // static
{
    var v,
        value,
        variables,
        name_value,
        pos,
        result = {};

    if(include_domain)
    {
        pos = query_string.indexOf("?");
        if(pos === -1)
        {
            return null;
        }
        query_string = query_string.slice(pos + 1);
    }

    variables = query_string.replace(/\+/gm, "%20")
                            .split("&");
    for(v in variables)
    {
        if(variables.hasOwnProperty(v))
        {
            name_value = variables[v].split("=");
            if(name_value.length === 2)
            {
                try
                {
                    // do this on two lines to make sure we do not get
                    // spurious entries in the result object
                    value = decodeURIComponent(name_value[1]);
                    result[name_value[0]] = value;
                }
                catch(ignore)
                {
                    // totally ignore if invalid
                    // (happens if name_value[1] is not valid UTF-8)
                }
            }
        }
    }

    return result;
};


/** \brief Make an element react as a button.
 *
 * This function transforms an HTML element such as an anchor (in
 * most cases, an anchor) so it reacts to the keyboard the same
 * way as a button element.
 *
 * This means attaching the keydown event to that element and react
 * by triggering a click() event whenever the user hits enter or
 * the spacebar.
 *
 * @param {Object} element  The element to "transform to a button."
 */
snapwebsites.Output.makeButton = function(element) // static
{
    jQuery(element).keydown(function(e)
        {
            if(e.which == 13 || e.which == 32)
            {
                // prevent further propagation
                e.preventDefault();
                e.stopPropagation();

                // trigger a click instead
                jQuery(element).trigger("click");
            }
        });
};


jQuery.fn.extend({
    /** \brief Apply the makeButton() function to a jQuery() object.
     *
     * This function calls our snapwebsites.Output.makeButton() function
     * on all the objects in this jQuery and returns the necessary reference
     * to continue the jQuery chain.
     *
     * We use this function when we setup a click() handler on a button,
     * for example:
     *
     * \code
     *    jQuery(".add-user-button")
     *      .makeButton()
     *      .focus()
     *      .click(function(e)
     *          {
     *              ...snip...
     *          });
     * \endcode
     *
     * This gives users the possibility to use Enter, Space, or Click
     * with the Mouse on that button.
     *
     * @this {jQuery}
     */
    makeButton: function()
    {
        return this.each(function(){
                snapwebsites.Output.makeButton(this);
            });
    }
});



// auto-initialize
jQuery(document).ready(
    function()
    {
        snapwebsites.OutputInstance = new snapwebsites.Output();
        snapwebsites.OutputInstance.registerBufferToMIME(new snapwebsites.BufferToMIMESystemImages());
    }
);

// vim: ts=4 sw=4 et
