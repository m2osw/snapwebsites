// Snap Websites Server -- test_plugin_plugin to list and run unit tests from the browser
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
#pragma once

// snapwebsites lib
//
#include <snapwebsites/snapwebsites.h>
#include <snapwebsites/plugins.h>


namespace snap
{
namespace test_plugin_suite
{


class test_plugin_suite_exception : public snap_exception
{
public:
    explicit test_plugin_suite_exception(char const *        what_msg) : snap_exception("Test Plugin Suite", what_msg) {}
    explicit test_plugin_suite_exception(std::string const & what_msg) : snap_exception("Test Plugin Suite", what_msg) {}
    explicit test_plugin_suite_exception(QString const &     what_msg) : snap_exception("Test Plugin Suite", what_msg) {}
};

class test_plugin_suite_already_exists : public test_plugin_suite_exception
{
public:
    explicit test_plugin_suite_already_exists(char const *        what_msg) : test_plugin_suite_exception(what_msg) {}
    explicit test_plugin_suite_already_exists(std::string const & what_msg) : test_plugin_suite_exception(what_msg) {}
    explicit test_plugin_suite_already_exists(QString const &     what_msg) : test_plugin_suite_exception(what_msg) {}
};

class test_plugin_suite_assert_failed : public test_plugin_suite_exception
{
public:
    explicit test_plugin_suite_assert_failed(char const *        what_msg) : test_plugin_suite_exception(what_msg) {}
    explicit test_plugin_suite_assert_failed(std::string const & what_msg) : test_plugin_suite_exception(what_msg) {}
    explicit test_plugin_suite_assert_failed(QString const &     what_msg) : test_plugin_suite_exception(what_msg) {}
};




class test_list_t
{
public:
    // a static function
    typedef void                    (*func_t)();
    // a map of name / static functions
    typedef QMap<QString, func_t>   test_func_map_t;

    bool                            empty() const;
    void                            add_test(QString const & name, func_t func);
    test_func_map_t const &         get_tests() const;

private:
    mutable test_func_map_t         f_tests;
};


/** \brief Declare the test plugin suite signals.
 *
 * This macro should be used in your plugin class declaration in the
 * public area. It defines the signals used by the test_plugin_suite
 * and test_plugin plugins.
 */
#define SNAP_TEST_PLUGIN_SUITE_SIGNALS() \
        void on_list_tests(::snap::test_plugin_suite::test_list_t & tests);


/** \brief Declare a test from the test plugin suite.
 *
 * This macro shall be used to declare a test in the plugin suite.
 * At this time, this macro prepends "test_plugin_" to the name of the
 * test. Otherwise all tests return void and receive no parameters.
 *
 * It is expected that these macro will be used in the class declaration
 * of a plugin in the private area.
 *
 * \param test_name  The name of the test being declared.
 */
#define SNAP_TEST_PLUGIN_TEST_DECL(test_name) \
        void plugin_test_##test_name();


/** \brief Start the implementation of a test.
 *
 * This macro is used to create the plugin test signature. It is expected
 * to directly be followed by the body of the test.
 *
 * \param plugin_name  The name of the plugin class implementing this
 *                     test.
 * \param test_name  The name of the test being implemented.
 */
#define SNAP_TEST_PLUGIN_TEST_IMPL(plugin_name, test_name) \
        void plugin_name::plugin_test_##test_name()


/** \brief Start the list of test for this plugin.
 *
 * This is expected to be defined in the public part
 * of a class defining a plugin.
 *
 * The macro is expected to be followed by one or more
 * SNAP_TEST_PLUGIN_TEST() and then terminated with
 * the TEST_PLUGIN_SUITE_END() macro.
 *
 * The name of the test suite will be set to the name
 * of the plugin so there is no need to specify that
 * here.
 *
 * The use of these three macros is enough to declare
 * the on_list_tests() and the on_run_test() signals,
 * however, you want to connect those too. See the
 * SNAP_TEST_PLUGIN_SUITE_LISTEN() macro for that purpose.
 *
 * \param plugin_name  The name of the plugin class.
 */
#define SNAP_TEST_PLUGIN_SUITE(plugin_name) \
    void plugin_name::on_list_tests(::snap::test_plugin_suite::test_list_t & tests) \
    {


/** \brief Add one test to the test suite.
 *
 * This macro adds one test to your test suite.
 *
 * The \p test_name parameter cannot include the name of the class.
 *
 * For example, the links plugin defines a test as follow:
 *
 * \code
 *  SNAP_TEST_PLUGIN_TEST_DECL( test_unique_unique_create_delete )
 * \encode
 *
 * To add it to the test suite, one uses the following:
 *
 * \code
 *  SNAP_TEST_PLUGIN_TEST( links, test_unique_unique_create_delete )
 * \endcode
 *
 * \param plugin_name  The name of the plugin class implementing this test.
 * \param test_name  The name of the test being defined.
 */
#define SNAP_TEST_PLUGIN_TEST(plugin_name, test_name) \
        struct static_##test_name \
        { \
            static void static_plugin_test_##test_name() \
            { \
                plugin_name::plugin_name::instance()->plugin_test_##test_name(); \
            } \
        }; \
        tests.add_test(#plugin_name "::" #test_name, static_##test_name::static_plugin_test_##test_name);


/** \brief Add one test in a sub-group of the test suite.
 *
 * This macro adds one test to your test suite. It adds the test in the
 * specified sub-group.
 *
 * The \p test_name parameter cannot include the name of the class.
 *
 * For example, the links plugin defines a test as follow:
 *
 * \code
 *  SNAP_TEST_PLUGIN_TEST_DECL( test_unique_unique_create_delete )
 * \encode
 *
 * To add it to the test suite in a sub-group named "linking",
 * one uses the following:
 *
 * \code
 *  SNAP_TEST_PLUGIN_GROUPED_TEST( links, test_unique_unique_create_delete, "linking" )
 * \endcode
 *
 * The \p group paramater may include sub-sub-groups as in "linking::coverage"
 * or "linking::one_to_one".
 *
 * \warning
 * The \p group parameter cannot be an empty string. Otherwise the final
 * name will end up having two scope operators one after another, which is
 * invalid.
 *
 * \param plugin_name  The name of the plugin class implementing this test.
 * \param test_name  The name of the test being defined.
 * \param group  The name of a sub-group or multiple sub-groups separated by
 *               the scope operator.
 */
#define SNAP_TEST_PLUGIN_GROUPED_TEST(plugin_name, test_name, group) \
        struct static_##test_name \
        { \
            static void static_plugin_test_##test_name() \
            { \
                plugin_name::plugin_name::instance()->plugin_test_##test_name(); \
            } \
        }; \
        tests.add_test(#plugin_name "::" #group "::" #test_name, static_##test_name::static_plugin_test_##test_name);


/** \brief End the list of tests.
 *
 * This macro ends the SNAP_TEST_PLUGIN_SUITE() definitions.
 */
#define SNAP_TEST_PLUGIN_SUITE_END() \
    }


/** \brief Listen to the test suite signals.
 *
 * The test suite will not react unless you listen for the proper
 * signals. This is done by adding this macro to your
 * on_boostrap() function.
 *
 * The \p plugin_name parameter needs to be set to the name of your
 * plugin. So if you are programming in the links plugin, use
 * \em links. (no quotes)
 *
 * It is suggested that you add this at the very end of the list
 * to avoid potential problems with the order in which signal
 * functions get called.
 *
 * \param plugin_name  The name of the plugin being setup with
 *                     a test suite.
 */
#define SNAP_TEST_PLUGIN_SUITE_LISTEN(plugin_name) \
    SNAP_LISTEN(plugin_name, "test_plugin_suite", test_plugin_suite::test_plugin_suite, list_tests, _1);


#define SNAP_TEST_PLUGIN_SUITE_ASSERT(test) \
    if(!(test)) throw ::snap::test_plugin_suite::test_plugin_suite_assert_failed(QString("%1:%2:%3: %4").arg(__FILE__).arg(__func__).arg(__LINE__).arg(#test));


class test_plugin_suite
        : public plugins::plugin
{
public:
                                test_plugin_suite();
                                ~test_plugin_suite();

    // plugins::plugin implementation
    static test_plugin_suite *  instance();
    virtual QString             settings_path() const;
    virtual QString             icon() const;
    virtual QString             description() const;
    virtual QString             dependencies() const;
    virtual int64_t             do_update(int64_t last_updated);
    virtual void                bootstrap(snap_child * snap);

    test_list_t const &         get_test_list() const;

    SNAP_SIGNAL_WITH_MODE(list_tests, (test_list_t & tests), (tests), NEITHER);

private:
    void                        content_update(int64_t variables_timestamp);

    snap_child *                f_snap = nullptr;
    test_list_t                 f_tests;
};


} // namespace test_plugin_suite
} // namespace snap
// vim: ts=4 sw=4 et
