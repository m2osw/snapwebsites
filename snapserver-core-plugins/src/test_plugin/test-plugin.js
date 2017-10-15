/** @preserve
 * Name: test-plugin
 * Version: 0.0.1.25
 * Browsers: all
 * Depends: popup (>= 0.1.0.1), server-access (>= 0.0.1.20)
 * Copyright: Copyright 2013-2017 (c) Made to Order Software Corporation  All rights reverved.
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
// ==/ClosureCompiler==
//

/*
 * JSLint options are defined online at:
 *    http://www.jshint.com/docs/options/
 */
/*jslint nomen: true, todo: true, devel: true */
/*global snapwebsites: false, jQuery: false, FileReader: true, Blob: true */



//
// This test-plugin.js is NOT yet offering unit tests for JavaScript.
// This is really only to test the C++ side for now. This allows us to
// do something similar to unitcpp but using the browser and a "real"
// client environment for the tests.
//

/** \file
 * \brief The client side of the test_plugin implementation.
 *
 * The test_plugin JavaScript includes two objects:
 *
 * \li The test object which handles running one test; and
 * \li The test environment object which knows how to manage the
 *     test objects.
 *
 * The manager registers all the tests available. The list is created
 * by the server as HTML. The manager knows how to get that list from
 * the HTML and manage it as expected (i.e. the 'run' links become
 * AJAX commands that send a 'run' command to the server and accept
 * a result.)
 *
 * The HTML code looks somewhat like this:
 *
 * \code
 *  <div class="test-plugin">
 *      <div class="group">
 *          <div class="group-title">Group: all <a class="run" href="#all">Run <em>all</em></a></div>
 *          <div class="group">
 *              <div class="group-title">Group: links <a class="run" href="links">Run <em>links</em></a></div>
 *              <div class="test never-ran">
 *                  1. <a class="run name" href="links::test_unique_unique_create_delete"
 *                        title="Click to run this specific test.">links::test_unique_unique_create_delete()</a>
 *                  <span class="status"></span>
 *              </div>
 *              <div class="test ran">
 *                  2. <a class="run name" href="links::test_unique_multiple_create_delete"
 *                        title="Click to run this specific test.">links::test_unique_multiple_create_delete()</a>
 *                  <span class="status">(last run on: ...date... for ...duration...)</span>
 *              </div>
 *              <div class="test never-ran">
 *                  3. <a class="run name" href="links::test_multiple_unique_create_delete"
 *                        title="Click to run this specific test.">links::test_multiple_unique_create_delete()</a>
 *                  <span class="status"></span>
 *              </div>
 *              <div class="test ran">
 *                  4. <a class="run name" href="links::test_multiple_multiple_create_delete"
 *                        title="Click to run this specific test.">links::test_multiple_multiple_create_delete()</a>
 *                  <span class="status">(last run on: ...date... for ...duration...)</span>
 *              </div>
 *          </div>
 *      </div>
 *  </div>
 * \endcode
 *
 * \code
 *   +-----------------------+
 *   |                       |
 *   | ServerAccessCallbacks |
 *   |                       |
 *   |                       |
 *   +-----------------------+
 *          ^
 *          | Inherit
 *          |
 *   +-----------------------+
 *   |                       |
 *   | TestPluginManagerBase |   Reference
 *   |                       +<-------+---------------+
 *   |                       |        |               |
 *   +-----------------------+        |               |
 *          ^                         |               |
 *          | Inherit                 |               |
 *          |                         |               |
 *          |      +------------------+----+   +------+----------------+
 *          |      |                       |   |                       |
 *          |      |  TestPluginTest       |   |  TestPluginGroup      |
 *          |      |                       |   |                       |
 *          |      |                       |   |                       |
 *          |      +-----------------------+   +-----------------------+
 *          |             ^                           ^
 *          |             | Create (1,n)              | Create (1,n)
 *          |             |                           |
 *   +------+-------------+--+                        |
 *   |                       |                        |
 *   |  TestPluginManager    +------------------------+
 *   |                       |
 *   |                       |
 *   +-----------------------+
 *          ^
 *          | Create (1,1)
 *          |
 *   +------+----------------+
 *   |                       |
 *   |   jQuery()            |
 *   |   Initialization      |
 *   |                       |
 *   +-----------------------+
 * \endcode
 */



/** \brief Snap TestPluginManagerBase constructor.
 *
 * The TestPluginManagerBase class is referenced by the TestPluginTest
 * and TestPluginGroup whenever one of these Run button is clicked.
 *
 * \code
 * class TestPluginManagerBase extends ServerAccessCallbacks
 * {
 * public:
 *      TestPluginManagerBase();
 *
 *      abstract function runGroup(group_name: string) : Void;
 *      abstract function runTest(test_name: string) : Void;
 * };
 * \endcode
 *
 * @return {snapwebsites.TestPluginManagerBase}  The newly created object.
 *
 * @constructor
 * @extends {snapwebsites.ServerAccessCallbacks}
 * @struct
 */
snapwebsites.TestPluginManagerBase = function()
{
    snapwebsites.TestPluginManagerBase.superClass_.constructor.call(this);

    return this;
};


/** \brief The ServerAccessCallbacks inherits ServerAccessCallbacks.
 *
 * This class inherits from the ServerAccessCallbacks so it can send
 * signals to the server so it starts running tests.
 */
snapwebsites.inherits(snapwebsites.TestPluginManagerBase, snapwebsites.ServerAccessCallbacks);


/** \brief Run all the tests in this group.
 *
 * This function is called whenever a group Run button gets clicked.
 * It runs all the tests defined in this group.
 *
 * @param {string} group_name  The name of the group to run.
 */
snapwebsites.TestPluginManagerBase.prototype.runGroup = function(group_name) // abstract
{
    throw new Error("the runGroup() function in TestPluginManagerBase is abstract");
};


/** \brief Run all the tests in this group.
 *
 * This function is called whenever a test Run button gets clicked.
 * It runs this specific test.
 *
 * @param {string} test_name  The name of the test to run.
 */
snapwebsites.TestPluginManagerBase.prototype.runTest = function(test_name) // abstract
{
    throw new Error("the runTest() function in TestPluginManagerBase is abstract");
};



/** \brief TestPluginGroup constructor.
 *
 * Some definitions are viewed as group of test. This object handles a
 * group by offering ways to retrieve group information (name, etc.)
 *
 * \code
 *  class TestPluginGroup
 *  {
 *  public:
 *      TestPluginGroup(group: jQuery, manager: TestPluginManagerBase);
 *
 *      function getName() : String;
 *  };
 * \endcode
 *
 * @param {jQuery} group  The jQuery group object.
 * @param {snapwebsites.TestPluginManagerBase} manager  A reference to the
 * test manager.
 *
 * @constructor
 * @struct
 */
snapwebsites.TestPluginGroup = function(group, manager)
{
    var that = this;

    this.group_ = group;

    this.group_.children(".group-title").find("a").click(function(e)
        {
            e.preventDefault();
            manager.runGroup(that.getName());
        });

    return this;
};


/** \brief TestPluginGroup inherits from the ServerAccessCallbacks class.
 *
 * This class inherits from the snapwebsites.ServerAccessCallbacks so that
 * way tests can send AJAX commands to the server to run the specified test.
 */
snapwebsites.base(snapwebsites.TestPluginGroup);


/** \brief The variable member that holds the group jQuery object.
 *
 * When a TestPluginGroup object is created, it receives a reference to
 * a jQuery object which represents the group in the HTML DOM.
 *
 * @type {jQuery}
 * @private
 */
snapwebsites.TestPluginGroup.prototype.group_ = null;


/** \brief Retrieve the name of this group.
 *
 * This function returns the group name.
 *
 * @return {string}  The name of this group.
 */
snapwebsites.TestPluginGroup.prototype.getName = function()
{
    return snapwebsites.castToString(this.group_.children(".group-title").find("a").attr("href"),
        "Group name is expected to be a string");
};



/** \brief TestPluginTest constructor.
 *
 * Each definition of a test in the screen defines a test that can be
 * run on the server with the test process running just as if a regular
 * user was using the system (Except maybe for permissions at least at
 * this point...).
 *
 * \code
 *  class TestPluginTest
 *  {
 *  public:
 *      TestPluginTest(test: jQuery, manager: TestPluginManagerBase);
 *
 *      function getName() const : String;
 *      function getGroupName() const : String;
 *      function isInGroup(group) const : Boolean;
 *      function setupServerAccess(server) const : Boolean;
 *
 *  private
 *      test_: jQuery;
 *  };
 * \endcode
 *
 * @param {jQuery} test  The jQuery test object.
 * @param {snapwebsites.TestPluginManagerBase} manager  A reference to the
 * test manager.
 *
 * @constructor
 * @struct
 */
snapwebsites.TestPluginTest = function(test, manager)
{
    var that = this;

    this.test_ = test;

    this.test_.find("a").click(function(e)
        {
            e.preventDefault();
            manager.runTest(that.getName());
        });

    return this;
};


/** \brief TestPluginTest is a base class.
 *
 * This class is primarily used to handle the parsing of the HTML
 * representing a test.
 */
snapwebsites.base(snapwebsites.TestPluginTest);


/** \brief The variable member that holds the test jQuery object.
 *
 * When a TestPluginTest object is created, it receives a reference to
 * a jQuery object which represents the test in the HTML DOM.
 *
 * @type {jQuery}
 * @private
 */
snapwebsites.TestPluginTest.prototype.test_ = null;


/** \brief Retrieve the name of this test.
 *
 * This function returns the test name.
 *
 * @return {string}  The name of this test.
 */
snapwebsites.TestPluginTest.prototype.getName = function()
{
    return snapwebsites.castToString(this.test_.find("a").attr("href"),
        "Test name is expected to be a string");
};


/** \brief Retrieve the name of the group of this test.
 *
 * This function returns the test name of the group this test is
 * part of.
 *
 * The group includes all the namespaces, not including the last
 * scope operator (::).
 *
 * @return {string}  The name of this test group.
 */
snapwebsites.TestPluginTest.prototype.getGroupName = function()
{
    var name = this.getName(),
        segments = name.split("::");

    // remove the test name, keep all the namespaces
    segments.pop();

    if(segments.length == 0)
    {
        throw new Error("somehow we found a test without a namespace");
    }

    return segments.join("::");
};


/** \brief Check whether this test is part of a group.
 *
 * This function gets this test group name, then it compares the
 * beginning of the group name with the specified \p group parameter.
 * If it is a match, then the function returns true which means
 * that this test is part of the specified \p group.
 *
 * @param {string} group  The group to check against.
 *
 * @return {boolean}  True if this test is part of the specified group.
 */
snapwebsites.TestPluginTest.prototype.isInGroup = function(group)
{
    var group_name = this.getGroupName();

    return group_name.indexOf(group) === 0;
};


/** \brief Run the test.
 *
 * This function is called in order to run this test once.
 * The run() function is called by the test when its link
 * gets clicked.
 *
 * When clicking on a group Run button, the manager takes care
 * of calling each test one by one. This is why it registers
 * a callback so on completion of a test, it can run its own
 * additional code.
 *
 * @param {snapwebsites.ServerAccess} server  The ServerAccess to setup for
 *                                            this test.
 */
snapwebsites.TestPluginTest.prototype.setupServerAccess = function(server)
{
    server.setURI("/admin/test-plugin/run");
    server.setData({test_plugin__test_name: this.getName()});

    this.test_.removeClass("ran")
              .removeClass("failed")
              .removeClass("succeeded")
              .removeClass("never-ran")
              .addClass("working");
};


/** \brief Mark that the test ran.
 *
 * This function is called once this test ran at least once.
 * It updates the DOM to reflect the fact that it ran.
 *
 * The \p xml_data parameter is also used to save the new
 * status in the DOM by create the corresponding HTML depending
 * on what happened.
 *
 * @param {jQuery} xml_data  The XML data returned by the server.
 */
snapwebsites.TestPluginTest.prototype.ran = function(xml_data)
{
    var result = parseInt(xml_data.find("data[name='test_plugin__result']").text(), 10),
        start_date = xml_data.find("data[name='test_plugin__start_date']").text(),
        end_date = xml_data.find("data[name='test_plugin__end_date']").text(),
        duration = xml_data.find("data[name='test_plugin__duration']").text(),
        status;

    // cleanup the classes
    this.test_.removeClass("never-ran")
              .removeClass("failed")
              .removeClass("suceeded")
              .removeClass("working")
              .addClass("ran")
              .addClass(result == 0 ? "failed" : "succeeded");

    // setup the status
    // TODO: translation / XSTL?
    status = " | Last run on <span title=\"It ran until "
            + end_date
            + "\">"
            + start_date
            + "</span>"
            + " for "
            + duration
            + " seconds and "
            + (result == 0 ? "failed" : "succeeded")
            + ".";
    this.test_.find(".status").html(status);
};



/** \brief Snap TestPluginManager constructor.
 *
 * The TestPluginManager class gets the list of tests and get them started.
 * It also handles the clicks on the test groups.
 *
 * \code
 * class TestPluginManager extends TestPluginManagerbase
 * {
 * public:
 *      TestPluginManager();
 *
 *      function runGroup(group_name: string);
 *      function runNextTest();
 *      function runTest(test_name: string);
 *
 *      virtual function serverAccessComplete(result: snapwebsites.ServerAccessCallbacks.ResultData) : Void;
 *
 * private:
 *      function setupTest_(item: jQuery);
 *
 *      groups_: Array of TestPluginGroup;
 *      testsMap_: TestPluginTest;
 *      tests_: Array of TestPluginTest;
 *      testsIndex_: number;
 *      groupName_: string;
 *      testName_: string;
 *      serverAccess_: ServerAccess;
 * };
 *
 * TestPluginManagerInstance: TestPluginManager;
 * \endcode
 *
 * @return {snapwebsites.TestPluginManager}  The newly created object.
 *
 * @constructor
 * @extends {snapwebsites.TestPluginManagerBase}
 * @struct
 */
snapwebsites.TestPluginManager = function()
{
    var that = this;

    snapwebsites.TestPluginManager.superClass_.constructor.call(this);

//#ifdef DEBUG
    if(jQuery("body").hasClass("snap-test-plugin-initialized"))
    {
        throw new Error("Only one test plugin singleton can be created.");
    }
    jQuery("body").addClass("snap-test-plugin-initialized");
//#endif

    // reset the list of groups and tests
    this.groups_ = {};
    this.testsMap_ = {};
    this.tests_ = [];

    jQuery(".test-plugin").children(".group")
        .each(function()
            {
                that.setupTest_(jQuery(this));
            });

    return this;
};


/** \brief Mark EditorBase as a base class.
 *
 * This class does not inherit from any other classes.
 */
snapwebsites.inherits(snapwebsites.TestPluginManager, snapwebsites.TestPluginManagerBase);


/** \brief The Editor instance.
 *
 * This class is a singleton and as such it makes use of a static
 * reference to itself. It gets created on load.
 *
 * @type {snapwebsites.TestPluginManager}
 */
snapwebsites.TestPluginManagerInstance = null; // static


/** \brief The map of group objects.
 *
 * The test plugin manager retrieves the lists of groups
 * defined in the DOM and saves them as JavaScript objects in
 * this map.
 *
 * The index is the name of the group.
 *
 * @type {Object}
 * @private
 */
snapwebsites.TestPluginManager.prototype.groups_ = null;


/** \brief The map of test objects.
 *
 * The test plugin manager retrieves the lists of tests defined in the
 * DOM and saves them as JavaScript objects in this map. The index is
 * the name of the test.
 *
 * @type {Object}
 * @private
 */
snapwebsites.TestPluginManager.prototype.testsMap_ = null;


/** \brief The array of test objects.
 *
 * The test plugin manager retrieves the lists of tests defined in the
 * DOM and saves them as JavaScript objects in this array.
 *
 * This array remains ordered. It is particularly useful to run() more
 * than one test in a row when the user clicks on a group button.
 *
 * @type {Array}
 * @private
 */
snapwebsites.TestPluginManager.prototype.tests_ = null;


/** \brief The index in the array of tests_.
 *
 * When we run a group, the process makes use of this index to know
 * which test to check next. If this index is larger or equal to
 * the length of the tests_ array, then the process is complete.
 *
 * @type {number}
 * @private
 */
snapwebsites.TestPluginManager.prototype.testsIndex_ = 0;


/** \brief The name of the group being processed.
 *
 * While we are running tests in a group, the name of the group is
 * saved here. This name represents the name we use to call the
 * test function isInGroup().
 *
 * @type {string}
 * @private
 */
snapwebsites.TestPluginManager.prototype.groupName_ = "";


/** \brief The name of the test being processed.
 *
 * While we ask the server to run a test, we save its name in this
 * variable member so when we get the signal back from the server
 * we know what to expect.
 *
 * We also use this parameter as a flag to know whether a test
 * is already being processed and prevent further clicks from
 * sending events simultaneously.
 *
 * @type {string}
 * @private
 */
snapwebsites.TestPluginManager.prototype.testName_ = "";


/** \brief The server access object.
 *
 * This object creates a server access so it can send the test commands
 * to the server via AJAX. The object is held in this variable member so
 * it can be created just once.
 *
 * @type {snapwebsites.ServerAccess}
 * @private
 */
snapwebsites.TestPluginManager.prototype.serverAccess_ = null;


/** \brief Initialize one of the test items.
 *
 * Test items include groups, sub-groups, and tests. This function is
 * called recursively up until all tests gets registered.
 *
 * @param {jQuery} item  The item to be worked on: a group or an items.
 *
 * @private
 */
snapwebsites.TestPluginManager.prototype.setupTest_ = function(item)
{
    var that = this,
        group,
        test;

    if(item.hasClass("group"))
    {
        // groups are handled by the manager
        group = new snapwebsites.TestPluginGroup(item, this);
        this.groups_[group.getName()] = group;

        // sub-groups
        item.children(".group").each(function()
            {
                that.setupTest_(jQuery(this));
            });

        // actual tests
        item.children(".group-tests").children(".test").each(function()
            {
                that.setupTest_(jQuery(this));
            });
    }
    else
    {
        // this is a test, create the corresponding object
        test = new snapwebsites.TestPluginTest(item, this);
        this.testsMap_[test.getName()] = test;
        this.tests_.push(test);

        // no children in this case.
    }
};


/** \brief Run all the tests in this group.
 *
 * This function is called whenever a group Run button gets clicked.
 * It runs all the tests defined in this group.
 *
 * @param {string} group_name  The name of the group to run.
 */
snapwebsites.TestPluginManager.prototype.runGroup = function(group_name) // abstract
{
    this.testsIndex_ = 0;
    this.groupName_ = group_name;
    this.runNextTest();
    // we return after we start the first group, it will restart once
    // we get a response from the server that the first test is done.
};


/** \brief Call the next test that matches the group name.
 *
 * This function goes through the list of tests and if one
 * matches the groupName_ parameter, then it gets run.
 *
 * The function memorize an index which is used to check for
 * the next test the next time it gets called.
 */
snapwebsites.TestPluginManager.prototype.runNextTest = function() // abstract
{
    var test,
        max_tests = this.tests_.length;

    if(this.groupName_ == "")
    {
        throw new Error("runNextText() called when groupName_ is an empty string");
    }

    for(; this.testsIndex_ < max_tests; ++this.testsIndex_)
    {
        test = this.tests_[this.testsIndex_];
        if(this.groupName_ == "#all" || test.isInGroup(this.groupName_))
        {
            this.runTest(test.getName());
            ++this.testsIndex_;
            // we have to wait for the server to reply before we can
            // process the next test so we return here
            return;
        }
    }

    // if we reach here the run over that group is over
    this.testsIndex_ = 0;
    this.groupName_ = "";
};


/** \brief Run all the tests in this group.
 *
 * This function is called whenever a test Run button gets clicked.
 * It runs this specific test.
 *
 * @param {string} test_name  The name of the test to run.
 */
snapwebsites.TestPluginManager.prototype.runTest = function(test_name) // abstract
{
    if(!this.serverAccess_)
    {
        this.serverAccess_ = new snapwebsites.ServerAccess(this);
    }
    this.testsMap_[test_name].setupServerAccess(this.serverAccess_);
    this.serverAccess_.send();

    // save the name so we have it in the serverAccessComplete() function
    this.testName_ = test_name;
};


/** \brief Function called on AJAX completion.
 *
 * This function is called once the whole process is over. It is most
 * often used to do some cleanup.
 *
 * By default this function does nothing.
 *
 * @param {snapwebsites.ServerAccessCallbacks.ResultData} result  The
 *          resulting data with information about the error(s).
 */
snapwebsites.TestPluginManager.prototype.serverAccessComplete = function(result) // virtual
{
    var xml_data = jQuery(result.jqxhr.responseXML),
        test = this.testsMap_[this.testName_];

    // save the results in there
    test.ran(xml_data);

    // if all tests where not checked yet, continue running them
    // (this happens when a group Run button was clicked)
    if(this.groupName_ != "")
    {
        this.runNextTest();
    }

    // manage messages if any
    snapwebsites.TestPluginManager.superClass_.serverAccessComplete.call(this, result);
};



// auto-initialize
jQuery(document).ready(function()
    {
        snapwebsites.TestPluginManagerInstance = new snapwebsites.TestPluginManager();
    });

// vim: ts=4 sw=4 et
