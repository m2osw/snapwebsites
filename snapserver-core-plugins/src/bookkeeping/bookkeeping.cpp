// Copyright (c) 2016-2019  Made to Order Software Corp.  All Rights Reserved
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
#include    "bookkeeping.h"


// other plugins
//
#include    "../output/output.h"
#include    "../permissions/permissions.h"
#include    "../messages/messages.h"


// snapwebsites
//
#include    <snapwebsites/qdomhelpers.h>
#include    <snapwebsites/snap_lock.h>


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
namespace bookkeeping
{



SERVERPLUGINS_START(bookkeeping, 1, 0)
    , ::serverplugins::description(
            "The bookkeeping plugin offers a basic set of functionality to"
            " manage your small business books: expensives, invoices, payroll,"
            " contractors, accounts, etc.")
    , ::serverplugins::icon("/images/bookkeeping/bookkeeping-logo-64x64.png")
    , ::serverplugins::settings_path("/admin/settings/bookkeeping")
    , ::serverplugins::dependency("editor")
    , ::serverplugins::dependency("messages")
    , ::serverplugins::dependency("output")
    , ::serverplugins::dependency("path")
    , ::serverplugins::dependency("permissions")
    , ::serverplugins::dependency("sendmail")
    , ::serverplugins::dependency("users")
    , ::serverplugins::help_uri("https://snapwebsites.org/help")
    , ::serverplugins::categorization_tag("finance")
SERVERPLUGINS_END(bookkeeping)


/** \class bookkeeping
 * \brief Support for a basic bookkeeping system.
 *
 * This plugin offers the following features:
 *
 * \li Recap of your day, week, month, quarter, year with graphs (balance sheets).
 * \li Expenses
 * \li Accounts (any positive accounts, bank accounts, paypal account, ...)
 * \li Account Payables (any negative accounts, i.e. credit cards)
 * \li Contractors
 * \li Payroll
 * \li Personal expenses
 * \li Invoices
 * \li Other Income (for special cases where no invoice is generated)
 * \li Quotes
 * \li Various Settings
 *
 * Categorization of each item allows you to determine various things
 * such as graphs of your expenses by category, and especially, lists
 * of items that correspond to a certain type of taxation such as
 * sales taxes or value added tax.
 */


/** \brief Get a fixed bookkeeping name.
 *
 * The bookkeeping plugin makes use of different names in the database. This
 * function ensures that you get the right spelling for a given name.
 *
 * \param[in] name  The name to retrieve.
 *
 * \return A pointer to the name.
 */
char const * get_name(name_t name)
{
    switch(name)
    {
    case name_t::SNAP_NAME_BOOKKEEPING_ADD_CLIENT_PATH:
        return "bookkeeping/client/add-client";

    case name_t::SNAP_NAME_BOOKKEEPING_CLIENT_ADDRESS1:
        return "bookkeeping::client_address1";

    case name_t::SNAP_NAME_BOOKKEEPING_CLIENT_ADDRESS2:
        return "bookkeeping::client_address2";

    case name_t::SNAP_NAME_BOOKKEEPING_CLIENT_CITY:
        return "bookkeeping::client_city";

    case name_t::SNAP_NAME_BOOKKEEPING_CLIENT_NAME:
        return "bookkeeping::client_name";

    case name_t::SNAP_NAME_BOOKKEEPING_CLIENT_PATH:
        return "bookkeeping/client";

    case name_t::SNAP_NAME_BOOKKEEPING_CLIENT_STATE:
        return "bookkeeping::client_state";

    case name_t::SNAP_NAME_BOOKKEEPING_CLIENT_ZIP:
        return "bookkeeping::client_zip";

    case name_t::SNAP_NAME_BOOKKEEPING_COUNTER:
        return "bookkeeping::counter";

    default:
        // invalid index
        throw snap_logic_exception("invalid name_t::SNAP_NAME_BOOKKEEPING_...");

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
time_t bookkeeping::do_update(time_t last_updated, unsigned int phase)
{
    SERVERPLUGINS_PLUGIN_UPDATE_INIT();

    if(phase == 0)
    {
        SERVERPLUGINS_PLUGIN_UPDATE(2016, 4, 7, 1, 45, 41, content_update);
    }

    SERVERPLUGINS_PLUGIN_UPDATE_EXIT();
}


/** \brief Update the database with our bookkeeping references.
 *
 * Send our bookkeeping to the database so the system can find us when a
 * user references our pages.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added to the database by this update (in micro-seconds).
 */
void bookkeeping::content_update(int64_t variables_timestamp)
{
    snapdev::NOT_USED(variables_timestamp);

    content::content::instance()->add_xml(get_plugin_name());
}


/** \brief Initialize the bookkeeping.
 *
 * This function terminates the initialization of the bookkeeping plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void bookkeeping::bootstrap()
{
    //SERVERPLUGINS_LISTEN(bookkeeping, "server", server, improve_signature, boost::placeholders::_1, boost::placeholders::_2, boost::placeholders::_3);
    //SERVERPLUGINS_LISTEN(bookkeeping, "path", path::path, can_handle_dynamic_path, boost::placeholders::_1, boost::placeholders::_2);
    //SERVERPLUGINS_LISTEN(bookkeeping, "layout", layout::layout, generate_page_content, boost::placeholders::_1, boost::placeholders::_2, boost::placeholders::_3);
    //SERVERPLUGINS_LISTEN(bookkeeping, "editor", editor::editor, finish_editor_form_processing, boost::placeholders::_1, boost::placeholders::_2);
    //SERVERPLUGINS_LISTEN(bookkeeping, "editor", editor::editor, init_editor_widget, boost::placeholders::_1, boost::placeholders::_2, boost::placeholders::_3, boost::placeholders::_4, boost::placeholders::_5);
}


/** \brief Check whether we are receiving a POST 
 *
 * This function displays the page that the user is trying to view. It is
 * supposed that the page permissions were already checked and thus that
 * its contents can be displayed to the current user.
 *
 * Note that the path was canonicalized by the path plugin and thus it does
 * not require any further corrections.
 *
 * \param[in] ipath  The canonicalized path being managed.
 *
 * \return true if the content is properly generated, false otherwise.
 */
bool bookkeeping::on_path_execute(content::path_info_t & ipath)
{
    QString const cpath(ipath.get_cpath());
    if(cpath == get_name(name_t::SNAP_NAME_BOOKKEEPING_ADD_CLIENT_PATH))
    {
SNAP_LOG_WARNING
<< "got inside add-client...?"
<< SNAP_LOG_SEND;
        if(f_snap->postenv_exists("client_name"))
        {
            // we are getting a request to create a new client
            //
SNAP_LOG_WARNING
<< "create new client..."
<< SNAP_LOG_SEND;
            return create_new_client(ipath);
        }
SNAP_LOG_WARNING
<< "bad post?! client..."
<< SNAP_LOG_SEND;
        if(!f_snap->all_postenv().empty())
        {
            messages::messages::instance()->set_error(
                "Invalid Post Data",
                "We could not understand this post.",
                "bookkeeping::on_path_execute(): there is POST data but it was not managed.",
                false
            );
            server_access::server_access * server_access_plugin(server_access::server_access::instance());
            server_access_plugin->create_ajax_result(ipath, false);
            server_access_plugin->ajax_output();
            return false;
        }
SNAP_LOG_WARNING
<< "that path, but no posts?..."
<< SNAP_LOG_SEND;
    }

    return output::output::instance()->on_path_execute(ipath);
}


bool bookkeeping::create_new_client(content::path_info_t & ipath)
{
    // TODO: add code to prevent re-adding the same client multiple
    //       times (i.e. search for a client with about the same name)

    server_access::server_access * server_access_plugin(server_access::server_access::instance());
    output::output * output_plugin(output::output::instance());
    content::content * content_plugin(content::content::instance());
    libdbproxy::table::pointer_t content_table(content_plugin->get_content_table());
    libdbproxy::table::pointer_t revision_table(content_plugin->get_revision_table());

    // assign a new number to this customer
    // 
    // TODO: allow data entry clerck to specify the new customer number
    //
    int64_t counter(0);
    content::path_info_t add_client_ipath;
    add_client_ipath.set_path("bookkeeping/client/add-client");
    libdbproxy::row::pointer_t add_client_row(content_table->getRow(add_client_ipath.get_key()));
    {
        // lock this page while we increase the counter
        //
        snap_lock lock(add_client_ipath.get_key());

        counter = add_client_row->getCell(QString(get_name(name_t::SNAP_NAME_BOOKKEEPING_COUNTER)))->getValue().safeInt64Value(0, 0) + 1;
        add_client_row->getCell(QString(get_name(name_t::SNAP_NAME_BOOKKEEPING_COUNTER)))->setValue(counter);
    }

    // TODO: properly setup the locale (use the User defined locale?)
    //
    QString const locale("xx");

    // we got the counter, create the new client
    content::path_info_t client_ipath;
    client_ipath.set_path(QString("bookkeeping/client/%1").arg(counter));
    client_ipath.force_branch(snap_version::SPECIAL_VERSION_USER_FIRST_BRANCH);
    client_ipath.force_revision(snap_version::SPECIAL_VERSION_FIRST_REVISION);
    client_ipath.force_locale(locale);
    content_plugin->create_content(client_ipath, output_plugin->get_plugin_name(), "bookkeeping/client");

    libdbproxy::row::pointer_t content_row(content_table->getRow(client_ipath.get_key()));
    content_row->getCell(layout::get_name(layout::name_t::SNAP_NAME_LAYOUT_LAYOUT))->setValue(QString("\"bookkeeping-client-parser\";"));
    content_row->getCell(editor::get_name(editor::name_t::SNAP_NAME_EDITOR_LAYOUT))->setValue(QString("\"bookkeeping-client-page\";"));

    libdbproxy::row::pointer_t revision_row(revision_table->getRow(client_ipath.get_revision_key()));
    int64_t const start_date(f_snap->get_start_date());
    revision_row->getCell(content::get_name(content::name_t::SNAP_NAME_CONTENT_CREATED))->setValue(start_date);

    // the title is used for the client's name (see below)
    //revision_row->getCell(content::get_name(content::name_t::SNAP_NAME_CONTENT_TITLE))->setValue(QString("...").arg(counter));
    // the body is empty by default, it is used for the description
    //revision_row->getCell(content::get_name(content::name_t::SNAP_NAME_CONTENT_BODY))->setValue(QString("<div>...</div>"));

    // this one is mandatory and was already checked and we know it is present
    QString const client_name(f_snap->postenv("client_name"));
    revision_row->getCell(content::get_name(content::name_t::SNAP_NAME_CONTENT_TITLE))->setValue(client_name);

    if(f_snap->postenv_exists("client_address1"))
    {
        QString const client_address1(f_snap->postenv("client_address1"));
        revision_row->getCell(get_name(name_t::SNAP_NAME_BOOKKEEPING_CLIENT_ADDRESS1))->setValue(client_address1);
    }

    if(f_snap->postenv_exists("client_city"))
    {
        QString const client_city(f_snap->postenv("client_city"));
        revision_row->getCell(get_name(name_t::SNAP_NAME_BOOKKEEPING_CLIENT_CITY))->setValue(client_city);
    }

    if(f_snap->postenv_exists("client_state"))
    {
        QString const client_state(f_snap->postenv("client_state"));
        revision_row->getCell(get_name(name_t::SNAP_NAME_BOOKKEEPING_CLIENT_STATE))->setValue(client_state);
    }

    if(f_snap->postenv_exists("client_zip"))
    {
        QString const client_zip(f_snap->postenv("client_zip"));
        revision_row->getCell(get_name(name_t::SNAP_NAME_BOOKKEEPING_CLIENT_ZIP))->setValue(client_zip);
    }

    {
        // assign the view permission
        //
        bool const source_unique(false);
        bool const destination_unique(false);

        content::path_info_t permission_ipath;
        permission_ipath.set_path("types/permissions/rights/view/bookkeeping/client");

        QString const user_back_group(permissions::get_name(permissions::name_t::SNAP_NAME_PERMISSIONS_LINK_BACK_VIEW));
        QString const direct_link_name(permissions::get_name(permissions::name_t::SNAP_NAME_PERMISSIONS_DIRECT_ACTION_VIEW));

        links::link_info source(user_back_group, source_unique, permission_ipath.get_key(), permission_ipath.get_branch());
        links::link_info destination(direct_link_name, destination_unique, client_ipath.get_key(), client_ipath.get_branch());
        links::links::instance()->create_link(source, destination);
    }

    {
        // assign the edit permission
        //
        bool const source_unique(false);
        bool const destination_unique(false);

        content::path_info_t permission_ipath;
        permission_ipath.set_path("types/permissions/rights/edit/bookkeeping/client");

        QString const user_back_group(permissions::get_name(permissions::name_t::SNAP_NAME_PERMISSIONS_LINK_BACK_EDIT));
        QString const direct_link_name(permissions::get_name(permissions::name_t::SNAP_NAME_PERMISSIONS_DIRECT_ACTION_EDIT));

        links::link_info source(user_back_group, source_unique, permission_ipath.get_key(), permission_ipath.get_branch());
        links::link_info destination(direct_link_name, destination_unique, client_ipath.get_key(), client_ipath.get_branch());
        links::links::instance()->create_link(source, destination);
    }

    {
        // assign the administer permission
        //
        bool const source_unique(false);
        bool const destination_unique(false);

        content::path_info_t permission_ipath;
        permission_ipath.set_path("types/permissions/rights/administer/bookkeeping/client");

        QString const user_back_group(permissions::get_name(permissions::name_t::SNAP_NAME_PERMISSIONS_LINK_BACK_ADMINISTER));
        QString const direct_link_name(permissions::get_name(permissions::name_t::SNAP_NAME_PERMISSIONS_DIRECT_ACTION_ADMINISTER));

        links::link_info source(user_back_group, source_unique, permission_ipath.get_key(), permission_ipath.get_branch());
        links::link_info destination(direct_link_name, destination_unique, client_ipath.get_key(), client_ipath.get_branch());
        links::links::instance()->create_link(source, destination);
    }

    // success, send the user to the new page
    server_access_plugin->create_ajax_result(ipath, true);
    server_access_plugin->ajax_redirect(client_ipath.get_key(), "_top");
    server_access_plugin->ajax_output();
    return true;
}




} // namespace bookkeeping
} // namespace snap
// vim: ts=4 sw=4 et
