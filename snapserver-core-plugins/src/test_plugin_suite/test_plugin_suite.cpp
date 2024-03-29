// Copyright (c) 2013-2019  Made to Order Software Corp.  All Rights Reserved
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
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA


// self
//
#include    "test_plugin_suite.h"


// snaplogger
//
#include    <snaplogger/message.h>


// snapdev
//
#include    <snapdev/not_used.h>


// last include
//
#include    <snapdev/poison.h>



namespace snap
{
namespace test_plugin_suite
{


SERVERPLUGINS_START(test_plugin_suite)
    , ::serverplugins::description(
            "The test_plugin_suite plugin is the low level test plugin"
            " capability, which gives you the ability to implement unit"
            " tests in your plugins. Use the test_plugin to run the tests.")
    , ::serverplugins::icon("/images/test-plugin/test-plugin-logo-64x64.jpg")
    , ::serverplugins::settings_path("/admin/test-plugin")
    , ::serverplugins::dependency("server")
    , ::serverplugins::help_uri("https://snapwebsites.org/help")
    , ::serverplugins::categorization_tag("security")
    , ::serverplugins::categorization_tag("spam")
SERVERPLUGINS_END(test_plugin_suite)


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
time_t test_plugin_suite::do_update(time_t last_updated, unsigned int phase)
{
    SERVERPLUGINS_PLUGIN_UPDATE_INIT();

    if(phase == 0)
    {
        snapdev::NOT_USED(last_updated);
    }

    SERVERPLUGINS_PLUGIN_UPDATE_EXIT();
}


/** \brief Initialize the test_plugin_suite.
 *
 * This function terminates the initialization of the test_plugin_suite plugin
 * by registering for different events.
 */
void test_plugin_suite::bootstrap()
{
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



} // namespace test_plugin_suite
} // namespace snap
// vim: ts=4 sw=4 et
