// Copyright (c) 2013-2019  Made to Order Software Corp.  All Rights Reserved
//
// https://snapwebsites.org/
// contact@m2osw.com
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
#include    "test_plugin.h"


// other plugins
//
#include    "../messages/messages.h"
#include    "../layout/layout.h"
#include    "../output/output.h"
#include    "../server_access/server_access.h"


// snapwebsites
//
#include    <snapwebsites/dbutils.h>
#include    <snapwebsites/qcompatibility.h>
#include    <snapwebsites/qhtmlserializer.h>
#include    <snapwebsites/qxmlmessagehandler.h>
#include    <snapwebsites/xslt.h>


// snaplogger
//
#include    <snaplogger/message.h>


// snapdev
//
#include    <snapdev/not_reached.h>
#include    <snapdev/not_used.h>


// last include
//
#include    <snapdev/poison.h>



namespace snap
{
namespace test_plugin
{



CPPTHREAD_PLUGIN_START(test_plugin, 1, 0)
    , ::cppthread::plugin_description(
            "The test_plugin plugin is capable of finding tests throughout"
            " all the plugins and run them one by one, per group,"
            " or all at once.")
    , ::cppthread::plugin_icon("/images/test-plugin/test-plugin-logo-64x64.jpg")
    , ::cppthread::plugin_settings("/admin/test-plugin")
    , ::cppthread::plugin_dependency("filter")
    , ::cppthread::plugin_dependency("layout")
    , ::cppthread::plugin_dependency("messages")
    , ::cppthread::plugin_dependency("output")
    , ::cppthread::plugin_dependency("path")
    , ::cppthread::plugin_dependency("server_access")
    , ::cppthread::plugin_dependency("test_plugin_suite")
    , ::cppthread::plugin_help_uri("https://snapwebsites.org/help/plugin/test_plugin_suite")
    , ::cppthread::plugin_categorization_tag("test")
CPPTHREAD_PLUGIN_END()



/** \class test_plugin
 * \brief Support for Snap! Unit Tests to be run from the browser.
 *
 * This plugin is for debug purposes only. It should only be installed on
 * debug systems and not on a live system. It will run unit tests that
 * were programmed in various Snap! plugins.
 */


/** \brief Get a fixed test_plugin name.
 *
 * The test_plugin plugin makes use of different names in the database. This
 * function ensures that you get the right spelling for a given name.
 *
 * Note that since this plugin is used to edit core and content data
 * more of the names come from those places.
 *
 * \param[in] name  The name to retrieve.
 *
 * \return A pointer to the name.
 */
char const * get_name(name_t name)
{
    switch(name)
    {
    case name_t::SNAP_NAME_TEST_PLUGIN_DURATION_FIELD:
        return "test_plugin__duration";

    case name_t::SNAP_NAME_TEST_PLUGIN_END_DATE:
        return "test_plugin::end_date";

    case name_t::SNAP_NAME_TEST_PLUGIN_END_DATE_FIELD:
        return "test_plugin__end_date";

    case name_t::SNAP_NAME_TEST_PLUGIN_RESULT_FIELD:
        return "test_plugin__result";

    case name_t::SNAP_NAME_TEST_PLUGIN_START_DATE:
        return "test_plugin::start_date";

    case name_t::SNAP_NAME_TEST_PLUGIN_START_DATE_FIELD:
        return "test_plugin__start_date";

    case name_t::SNAP_NAME_TEST_PLUGIN_SUCCESS:
        return "test_plugin::success";

    case name_t::SNAP_NAME_TEST_PLUGIN_TEST_NAME_FIELD:
        return "test_plugin__test_name";

    case name_t::SNAP_NAME_TEST_PLUGIN_TEST_RESULTS_TABLE:
        return "test_results";

    default:
        // invalid index
        throw snap_logic_exception("invalid name_t::SNAP_NAME_TEST_PLUGIN_...");

    }
    snapdev::NOT_REACHED();
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
int64_t test_plugin::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2015, 12, 20, 23, 29, 40, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Update the database with our info references.
 *
 * Send our info to the database so the system can find us when a
 * user references our pages.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added to the database by this update (in micro-seconds).
 */
void test_plugin::content_update(int64_t variables_timestamp)
{
    snapdev::NOT_USED(variables_timestamp);

    content::content::instance()->add_xml(get_plugin_name());
}


/** \brief Initialize the test_plugin.
 *
 * This function terminates the initialization of the test_plugin plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void test_plugin::bootstrap(snap_child * snap)
{
    f_snap = snap;

    SNAP_LISTEN(test_plugin, "server", server, process_post, boost::placeholders::_1);
    SNAP_LISTEN(test_plugin, "filter", filter::filter, replace_token, boost::placeholders::_1, boost::placeholders::_2, boost::placeholders::_3);
    SNAP_LISTEN(test_plugin, "filter", filter::filter, token_help, boost::placeholders::_1);
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
libdbproxy::table::pointer_t test_plugin::get_test_results_table()
{
    if(!f_test_results_table)
    {
        f_test_results_table = f_snap->get_table(get_name(name_t::SNAP_NAME_TEST_PLUGIN_TEST_RESULTS_TABLE));
    }
    return f_test_results_table;
}


/** \brief Replace the token used to show the list of tests.
 *
 * The name of the supported token is: [test_plugin::tests].
 *
 * We also support the [test_plugin::version] token which gets
 * replaced by the version of this plugin. To get the version
 * of all the plugins, send the user to the /admin/versions page.
 */
void test_plugin::on_replace_token(content::path_info_t & ipath, QDomDocument & xml, filter::filter::token_info_t & token)
{
    snapdev::NOT_USED(ipath);

    if(!token.is_namespace("test_plugin::"))
    {
        return;
    }

    if(token.is_token("test_plugin::version"))
    {
        token.f_replacement = QString("%1.%2").arg(get_major_version()).arg(get_minor_version());
    }
    else if(token.is_token("test_plugin::tests"))
    {
        libdbproxy::table::pointer_t test_results_table(get_test_results_table());

        test_plugin_suite::test_list_t const& test_list(test_plugin_suite::test_plugin_suite::instance()->get_test_list());
        QList<QString> keys(test_list.get_tests().keys());
        QDomDocument doc;
        QDomElement root_tag(doc.createElement("group"));
        root_tag.setAttribute("name", "all");
        doc.appendChild(root_tag);

        QString group; // current group
        snap_string_list group_segments;
        int const max_keys(keys.size());
        for(int idx(0), count(1); idx < max_keys; ++idx, ++count)
        {
            QString const name(keys.at(idx));
            snap_string_list segments(name.split("::"));
            QString const test_name(segments.last());
            segments.pop_back();
            QString const group_name(segments.join("::"));

            QDomElement tag(doc.documentElement());
            QDomElement parent_group_tag;
            QDomElement group_tag;
            while(!tag.isNull())
            {
                QDomNode next(tag.firstChild());
                if(next.isNull())
                {
                    next = tag;
                    while(!next.isNull())
                    {
                        QDomNode parent(next.parentNode());
                        next = next.nextSibling();
                        if(!next.isNull())
                        {
                            break;
                        }
                        next = parent;
                    }
                }
                if(next.isNull())
                {
                    // nothing found, we'll create a new set of groups
                    break;
                }
                tag = next.toElement();
                if(tag.isNull())
                {
                    // we are only interested by elements
                    continue;
                }

                if(tag.tagName() == "group")
                {
                    QString const tag_name(tag.attribute("name"));
                    if(tag_name == group_name)
                    {
                        // we found it!
                        group_tag = tag;
                        break;
                    }
                    else if(group_name.startsWith(tag_name + "::"))
                    {
                        // close... if we do not find it otherwise, we
                        // will create a child to this tag
                        if(parent_group_tag.isNull())
                        {
                            // first group that partially match, keep it
                            parent_group_tag = tag;
                        }
                        else
                        {
                            // another group tag partially match, keep the
                            // new one only if longer
                            int const l1(parent_group_tag.attribute("name").length());
                            int const l2(tag_name.length());
                            if(l2 > l1)
                            {
                                parent_group_tag = tag;
                            }
                        }
                    }
                }
            }

            if(group_tag.isNull())
            {
                if(parent_group_tag.isNull())
                {
                    group_tag = doc.documentElement();
                }
                else
                {
                    group_tag = parent_group_tag;
                }
                QString existing_group_name(group_tag.attribute("name"));
                if(existing_group_name == "all")
                {
                    // ignore the special name "all" (TBD)
                    existing_group_name = "";
                }
                snap_string_list const existing_segments(split_string(existing_group_name, "::"));

                int const max_count(segments.size());
                for(int j(existing_segments.size()); j < max_count; ++j)
                {
                    snap_string_list const new_group(segments.mid(0, j + 1));
                    QString const new_name(new_group.join("::"));
                    QDomElement new_group_tag(doc.createElement("group"));
                    new_group_tag.setAttribute("name", new_name);
                    group_tag.appendChild(new_group_tag);
                    group_tag = new_group_tag;
                }
            }

            QDomElement new_test_tag(doc.createElement("test"));
            new_test_tag.setAttribute("name", name);
            new_test_tag.setAttribute("count", count);

            // did that test run before?
            // note how tests are cross website!
            if(f_test_results_table->exists(name)
            && f_test_results_table->getRow(name)->exists(get_name(name_t::SNAP_NAME_TEST_PLUGIN_SUCCESS)))
            {
                new_test_tag.setAttribute("ran", "ran");
                libdbproxy::row::pointer_t row(f_test_results_table->getRow(name));
                int64_t const start_date(row->getCell(get_name(name_t::SNAP_NAME_TEST_PLUGIN_START_DATE))->getValue().safeInt64Value());
                new_test_tag.setAttribute("start_date", dbutils::microseconds_to_string(start_date, false));
                int64_t const end_date(row->getCell(get_name(name_t::SNAP_NAME_TEST_PLUGIN_END_DATE))->getValue().safeInt64Value());
                new_test_tag.setAttribute("end_date", dbutils::microseconds_to_string(end_date, false));
                int64_t const duration(end_date - start_date);
                new_test_tag.setAttribute("duration", QString("%1.%2").arg(duration / 1000000).arg(duration % 1000000, 6, 10, QChar('0')));
                new_test_tag.setAttribute("success", row->getCell(get_name(name_t::SNAP_NAME_TEST_PLUGIN_SUCCESS))->getValue().safeSignedCharValue());
            }
            else
            {
                new_test_tag.setAttribute("ran", "never-ran");
            }

            group_tag.appendChild(new_test_tag);
        }

        xslt x;
        x.set_xsl_from_file("qrc://xsl/test-plugin/test-plugin-parser.xsl");
        x.set_document(doc);
        token.f_replacement = x.evaluate_to_string();

        // the test plugin JavaScript takes over and generates the
        // client functionality
        content::content::instance()->add_css(xml, "test-plugin");
        content::content::instance()->add_javascript(xml, "test-plugin");
    }
}


void test_plugin::on_token_help(filter::filter::token_help_t & help)
{
    help.add_token("test_plugin::version",
        "Show the version of the test plugin.");

    help.add_token("test_plugin::tests",
        "Generate a list of all the available client-side tests"
        " including links to execute them. This token also adds"
        " CSS and JavaScript code so the output is fully functional.");
}




/** \brief Execute a path owned by test_plugin.
 *
 * This function executes a path owned by this plugin.
 *
 * We use this mechanism to capture the page when the user clicks on
 * a link and a corresponding test is expected to run.
 *
 * \param[in,out] ipath  The path of the page being executed.
 *
 * \return true meaning that it worked.
 */
bool test_plugin::on_path_execute(content::path_info_t& ipath)
{
    snapdev::NOT_USED(ipath);
    //f_snap->output(layout::layout::instance()->apply_layout(ipath, this));

    return false;
}


/** \brief Check the URL and process the POST data accordingly.
 *
 * This function captures POST events sent by the client whenever
 * a link is clicked and the client expects a test to run.
 *
 * \param[in] uri_path  The path received from the HTTP server.
 */
void test_plugin::on_process_post(QString const & uri_path)
{
    // make sure this is a cart post
    char const * clicked_test_name_field(get_name(name_t::SNAP_NAME_TEST_PLUGIN_TEST_NAME_FIELD));
    if(!f_snap->postenv_exists(clicked_test_name_field))
    {
        return;
    }

    // get the value to determine which button was clicked
    QString const test_name(f_snap->postenv(clicked_test_name_field));
    bool success(true);
    int64_t start_date(0);
    int64_t end_date(0);
    QString result;

    content::path_info_t ipath;
    ipath.set_path(uri_path);

    test_plugin_suite::test_list_t const& test_list(test_plugin_suite::test_plugin_suite::instance()->get_test_list());
    test_plugin_suite::test_list_t::test_func_map_t const& test_map(test_list.get_tests());
    if(test_map.contains(test_name))
    {
        // expects the test to fail by default
        result = "0";
        try
        {
            // call the test
            start_date = f_snap->get_current_date();
            (*test_map[test_name])();
            end_date = f_snap->get_current_date();
            result = "1";
        }
        catch(test_plugin_suite::test_plugin_suite_assert_failed const& e)
        {
            success = false;
            end_date = f_snap->get_current_date();
            messages::messages::instance()->set_error(
                "Test Assertion Failed",
                QString("Test \"%1\" failed an assertion with error: \"%2\".").arg(test_name).arg(e.what()),
                "Test did not pass as one of its assertions failed.",
                false
            );
        }
        catch(std::exception const& e)
        {
            success = false;
            end_date = f_snap->get_current_date();
            messages::messages::instance()->set_error(
                "Test Failed",
                QString("Test \"%1\" failed with error: \"%2\".").arg(test_name).arg(e.what()),
                "Test did not pass.",
                false
            );
        }
    }
    else
    {
        success = false;
        messages::messages::instance()->set_error(
            "Test Not Found",
            QString("We could not find test named \"%1\".").arg(test_name),
            "Somehow the name of a test is not valid, it could be that the plugin with that test was removed since you first loaded this page.",
            false
        );
        // processing error
        result = "-1";
    }

    libdbproxy::table::pointer_t test_results_table(get_test_results_table());
    libdbproxy::row::pointer_t test_results_row(test_results_table->getRow(test_name));
    test_results_row->getCell(get_name(name_t::SNAP_NAME_TEST_PLUGIN_START_DATE))->setValue(start_date);
    test_results_row->getCell(get_name(name_t::SNAP_NAME_TEST_PLUGIN_END_DATE))->setValue(end_date);
    int8_t const success_char(result == "1" ? 1 : 0);
    test_results_row->getCell(get_name(name_t::SNAP_NAME_TEST_PLUGIN_SUCCESS))->setValue(success_char);

    // create the AJAX response
    server_access::server_access * server_access_plugin(server_access::server_access::instance());
    server_access_plugin->create_ajax_result(ipath, success);
    server_access_plugin->ajax_append_data(get_name(name_t::SNAP_NAME_TEST_PLUGIN_RESULT_FIELD), result.toUtf8());
    QString const start_date_str(dbutils::microseconds_to_string(start_date, false));
    server_access_plugin->ajax_append_data(get_name(name_t::SNAP_NAME_TEST_PLUGIN_START_DATE_FIELD), start_date_str.toUtf8());
    QString const end_date_str(dbutils::microseconds_to_string(end_date, false));
    server_access_plugin->ajax_append_data(get_name(name_t::SNAP_NAME_TEST_PLUGIN_END_DATE_FIELD), end_date_str.toUtf8());
    int64_t duration(end_date - start_date);
    QString const duration_str(QString("%1.%2").arg(duration / 1000000).arg(duration % 1000000, 6, 10, QChar('0')));
    server_access_plugin->ajax_append_data(get_name(name_t::SNAP_NAME_TEST_PLUGIN_DURATION_FIELD), duration_str.toUtf8());
    server_access_plugin->ajax_output();
}



} // namespace test_plugin
} // namespace snap
// vim: ts=4 sw=4 et
