// Snap Websites Server -- test_plugin_suite to list and run unit tests from the browser
// Copyright (C) 2013-2017  Made to Order Software Corp.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

#include "test_plugin_suite.h"

#include <snapwebsites/log.h>
#include <snapwebsites/not_used.h>

#include <snapwebsites/poison.h>

SNAP_PLUGIN_START(test_plugin_suite, 1, 0)


/** \class test_plugin_suite
 * \brief Support for Snap! Unit Tests to be run from the browser.
 *
 * This plugin is for debug purposes only. It should only be installed on
 * debug systems and not on a live system. It will run unit tests that
 * were programmed in various Snap! plugins.
 *
 * In order to setup test suites, make use of the following macros:
 *
 * \code
 * // in your header file (.h)
 * public:
 * SNAP_TEST_PLUGIN_SUITE_SIGNALS()
 *
 * private:
 * SNAP_TEST_PLUGIN_TEST_DECL(<test name 1>)
 * SNAP_TEST_PLUGIN_TEST_DECL(<test name 2>)
 *   ...
 * SNAP_TEST_PLUGIN_TEST_DECL(<test name n>)
 *
 * // in your implementation file (.cpp)
 * SNAP_TEST_PLUGIN_SUITE()
 *   SNAP_TEST_PLUGIN_TEST(<test name 1>)
 *   SNAP_TEST_PLUGIN_TEST(<test name 2>)
 *     ...
 *   SNAP_TEST_PLUGIN_TEST(<test name n>)
 * SNAP_TEST_PLUGIN_SUITE_END()
 *
 * SNAP_TEST_PLUGIN_TEST_IMPL(<test name 1>)
 * {
 *    ...implementation of test...
 * }
 *
 * SNAP_TEST_PLUGIN_TEST_IMPL(<test name 2>)
 * {
 *    ...implementation of test...
 * }
 *
 * ...
 *
 * SNAP_TEST_PLUGIN_TEST_IMPL(<test name n>)
 * {
 *    ...implementation of test...
 * }
 * \endcode
 *
 * To create sub-groups of tests, one can use the
 * SNAP_TEST_PLUGIN_GROUPED_TEST() macro instead of the
 * SNAP_TEST_PLUGIN_TEST() macro.
 */




/** \brief Test whether the list of test is empty.
 *
 * This function returns true if the list of tests is still empty.
 * The plugin caches the list so calling the get_test_list() function
 * is fast the second time.
 *
 * Note that the list may remain empty if none of the plugins that you
 * are using defined tests.
 *
 * \return true if the list of test is still empty.
 */
bool test_list_t::empty() const
{
    return f_tests.isEmpty();
}


/** \brief Add a test to this list of tests.
 *
 * This function adds the named test to this list.
 *
 * \exception test_plugin_suite_already_exists
 * If the same test gets added more than once, then this function raises
 * this exception.
 *
 * \param[in] name  The name of the test being added.
 * \param[in] func  A pointer to a static function to call to run the test.
 */
void test_list_t::add_test(QString const& name, func_t func)
{
    if(f_tests.contains(name))
    {
        throw test_plugin_suite_already_exists(QString("Test \"%1\" already exists in the list of tests.").arg(name));
    }

    f_tests[name] = func;
}


/** \brief Retrieve a reference to the tests.
 *
 * This function returns a reference to the list of tests which can be
 * used to generate HTML or run a test.
 *
 * The map is indexed by the name of the test so it will be in
 * alphabetical order (since we expect all names to be ASCII.)
 *
 * \return The map of the tests available in this instance.
 */
test_list_t::test_func_map_t const& test_list_t::get_tests() const
{
    return f_tests;
}




/** \brief Initialize the test_plugin_suite plugin.
 *
 * This function is used to initialize the test_plugin_suite plugin object.
 */
test_plugin_suite::test_plugin_suite()
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Clean up the test_plugin_suite plugin.
 *
 * Ensure the test_plugin_suite object is clean before it is gone.
 */
test_plugin_suite::~test_plugin_suite()
{
}


/** \brief Get a pointer to the test_plugin_suite plugin.
 *
 * This function returns an instance pointer to the test_plugin_suite plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the test_plugin_suite plugin.
 */
test_plugin_suite * test_plugin_suite::instance()
{
    return g_plugin_test_plugin_suite_factory.instance();
}


/** \brief Send users to the plugin settings.
 *
 * This path represents this plugin "settings". In case of the
 * test plugin suite, this is really the page that allows one
 * to run the tests.
 */
QString test_plugin_suite::settings_path() const
{
    return "/admin/test-plugin";
}


/** \brief A path or URI to a logo for this plugin.
 *
 * This function returns a 64x64 icons representing this plugin.
 *
 * \return A path to the logo.
 */
QString test_plugin_suite::icon() const
{
    return "/images/test-plugin/test-plugin-logo-64x64.jpg";
}


/** \brief Return the description of this plugin.
 *
 * This function returns the English description of this plugin.
 * The system presents that description when the user is offered to
 * install or uninstall a plugin on his website. Translation may be
 * available in the database.
 *
 * \return The description in a QString.
 */
QString test_plugin_suite::description() const
{
    return "The test_plugin_suite plugin is the low level test plugin"
          " capability, which gives you the ability to implement unit"
          " tests in your plugins. Use the test_plugin to run the tests.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString test_plugin_suite::dependencies() const
{
    return "|server|";
}


/** \brief Check whether updates are necessary.
 *
 * This function updates the database when a newer version is installed
 * and the corresponding updates where not run.
 *
 * This works for newly installed plugins and older plugins that were
 * updated.
 *
 * \param[in] last_updated  The UTC Unix date when the website was last updated (in micro seconds).
 *
 * \return The UTC Unix date of the last update of this plugin.
 */
int64_t test_plugin_suite::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    NOTUSED(last_updated);
    //SNAP_PLUGIN_UPDATE(2014, 4, 10, 22, 47, 40, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Initialize the test_plugin_suite.
 *
 * This function terminates the initialization of the test_plugin_suite plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void test_plugin_suite::bootstrap(snap_child * snap)
{
    f_snap = snap;
}


/** \brief Initialize the test_results table.
 *
 * This function creates the test_results table if it does not already
 * exist. Otherwise it simply initializes the f_test_results_table
 * variable member.
 *
 * If the function is not able to create the table an exception is raised.
 *
 * The test_results table is used to save whether a test passed or failed
 * when last run. It also keeps information about the dates when the test
 * started and ended. Note that since the plugin should NOT be installed
 * on a live system, we do not overly protect the results.
 *
 * \return The pointer to the test_results table.
 */
test_list_t const & test_plugin_suite::get_test_list() const
{
    if(f_tests.empty())
    {
        const_cast<test_plugin_suite *>(this)->list_tests(const_cast<test_list_t&>(f_tests));
    }
    return f_tests;
}





SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
