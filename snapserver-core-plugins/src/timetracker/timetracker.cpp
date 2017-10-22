// Snap Websites Server -- timetracker plugin to track time and generate invoices
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

#include "timetracker.h"

#include "../bookkeeping/bookkeeping.h"
#include "../layout/layout.h"
#include "../list/list.h"
#include "../locale/snap_locale.h"
#include "../messages/messages.h"
#include "../output/output.h"
#include "../permissions/permissions.h"
#include "../users/users.h"

#include <snapwebsites/log.h>
#include <snapwebsites/not_reached.h>
#include <snapwebsites/not_used.h>
#include <snapwebsites/qdomhelpers.h>
#include <snapwebsites/xslt.h>

#include <snapwebsites/poison.h>


SNAP_PLUGIN_START(timetracker, 1, 0)


/** \class timetracker
 * \brief Offer a way to track time spent on a project and generate invoices.
 *
 * This is a simple way to track hours of work so you can invoice them later.
 */


/** \brief Get a fixed timetracker name.
 *
 * The timetracker plugin makes use of different names in the database. This
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
    case name_t::SNAP_NAME_TIMETRACKER_BILLING_DURATION:
        return "timetracker::billing_duration";

    case name_t::SNAP_NAME_TIMETRACKER_DATE_QUERY_STRING:
        return "date";

    case name_t::SNAP_NAME_TIMETRACKER_LOCATION:
        return "timetracker::location";

    case name_t::SNAP_NAME_TIMETRACKER_MAIN_PAGE:
        return "timetracker::main_page";

    case name_t::SNAP_NAME_TIMETRACKER_PATH:
        return "timetracker";

    case name_t::SNAP_NAME_TIMETRACKER_TRANSPORTATION:
        return "timetracker::transportation";

    default:
        // invalid index
        throw snap_logic_exception("invalid name_t::SNAP_NAME_TIMETRACKER_...");

    }
    NOTREACHED();
}


/** \brief Initialize the timetracker plugin.
 *
 * This function is used to initialize the timetracker plugin object.
 */
timetracker::timetracker()
    //: f_snap(nullptr) -- auto-init
{
}

/** \brief Clean up the timetracker plugin.
 *
 * Ensure the timetracker object is clean before it is gone.
 */
timetracker::~timetracker()
{
}

/** \brief Get a pointer to the timetracker plugin.
 *
 * This function returns an instance pointer to the timetracker plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the timetracker plugin.
 */
timetracker * timetracker::instance()
{
    return g_plugin_timetracker_factory.instance();
}


/** \brief Send users to the timetracker settings.
 *
 * This path represents the timetracker settings.
 */
QString timetracker::settings_path() const
{
    return "/admin/settings/timetracker";
}


/** \brief A path or URI to a logo for this plugin.
 *
 * This function returns a 64x64 icons representing this plugin.
 *
 * \return A path to the logo.
 */
QString timetracker::icon() const
{
    return "/images/timetracker/timetracker-logo-64x64.png";
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
QString timetracker::description() const
{
    return "The time tracker plugin lets you or your employees enter their"
          " hours in order to generate invoices to your clients."
          " The tracker includes notes to describe the work done.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString timetracker::dependencies() const
{
    return "|bookkeeping|editor|messages|output|path|permissions|users|";
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
int64_t timetracker::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2016, 4, 7, 1, 45, 41, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Update the database with our timetracker references.
 *
 * Send our timetracker to the database so the system can find us when a
 * user references our pages.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables
 *            added to the database by this update (in micro-seconds).
 */
void timetracker::content_update(int64_t variables_timestamp)
{
    NOTUSED(variables_timestamp);

    content::content::instance()->add_xml(get_plugin_name());
}


/** \brief Initialize the timetracker.
 *
 * This function terminates the initialization of the timetracker plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void timetracker::bootstrap(snap_child * snap)
{
    f_snap = snap;

    //SNAP_LISTEN(timetracker, "path", path::path, can_handle_dynamic_path, _1, _2);
    SNAP_LISTEN(timetracker, "path", path::path, check_for_redirect, _1);
    SNAP_LISTEN(timetracker, "layout", layout::layout, generate_header_content, _1, _2, _3);
    SNAP_LISTEN(timetracker, "filter", filter::filter, replace_token, _1, _2, _3);
    SNAP_LISTEN(timetracker, "filter", filter::filter, token_help, _1);
    SNAP_LISTEN(timetracker, "editor", editor::editor, init_editor_widget, _1, _2, _3, _4, _5);
}


void timetracker::on_generate_header_content(content::path_info_t & ipath, QDomElement & header, QDomElement & metadata)
{
    NOTUSED(header);
    NOTUSED(metadata);

    QString const cpath(ipath.get_cpath());
    if(cpath == get_name(name_t::SNAP_NAME_TIMETRACKER_PATH)
    || cpath.startsWith(QString("%1/").arg(get_name(name_t::SNAP_NAME_TIMETRACKER_PATH))))
    {
        content::content * content_plugin(content::content::instance());

        content_plugin->add_javascript(header.ownerDocument(), "timetracker");
        content_plugin->add_css(header.ownerDocument(), "timetracker");
    }
}


/** \brief Execute a page: generate the complete output of that page.
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
bool timetracker::on_path_execute(content::path_info_t & ipath)
{
    QString const cpath(ipath.get_cpath());
    if(cpath == get_name(name_t::SNAP_NAME_TIMETRACKER_PATH))
    {
        if(f_snap->postenv_exists("operation"))
        {
            server_access::server_access * server_access_plugin(server_access::server_access::instance());
            users::users * users_plugin(users::users::instance());
            users::users::user_info_t user_info(users_plugin->get_user_info());


            QString const operation(f_snap->postenv("operation"));
            if(operation == "add-self")
            {
                int64_t const identifier(user_info.get_identifier());
                add_calendar(identifier);
                server_access_plugin->create_ajax_result(ipath, true);
                server_access_plugin->ajax_redirect(QString("/%1").arg(get_name(name_t::SNAP_NAME_TIMETRACKER_PATH)));
            }
            else if(operation == "calendar")
            {
                int64_t const identifier(user_info.get_identifier());
                content::path_info_t calendar_ipath;
                ipath.get_child(calendar_ipath, QString("%1").arg(identifier));
                if(f_snap->postenv_exists("year")
                && f_snap->postenv_exists("month"))
                {
                    // convert the year/month in a date that the
                    // token_calendar() understands
                    bool ok(false);
                    int const year(f_snap->postenv("year").toInt(&ok, 10));
                    if(!ok || year < 2000 || year > 3000)
                    {
                        messages::messages::instance()->set_error(
                            "Invalid Year",
                            QString("The year (%1) is not a valid number or is out of bounds.").arg(f_snap->postenv("year")),
                            "timetracker::on_path_execute(): the year was not correct.",
                            false
                        );
                        goto invalid_data;
                    }
                    int const month(f_snap->postenv("month").toInt(&ok, 10));
                    if(!ok || month < 1 || month > 12)
                    {
                        messages::messages::instance()->set_error(
                            "Invalid Month",
                            QString("The month (%1) is not a valid number or is out of bounds.").arg(f_snap->postenv("month")),
                            "timetracker::on_path_execute(): the month was not correct.",
                            false
                        );
                        goto invalid_data;
                    }
                    calendar_ipath.set_parameter("date", QString("%1%0201").arg(year, 4, 10, QChar('0')).arg(month, 2, 10, QChar('0')));
                }
                QString const result(token_calendar(calendar_ipath));
                server_access_plugin->create_ajax_result(ipath, true);
                server_access_plugin->ajax_append_data("calendar", result.toUtf8());
            }
            else
            {
                messages::messages::instance()->set_error(
                    "Unknown Timetracker Operation",
                    QString("Timetracker received unknown operation \"%1\".").arg(operation),
                    "info::plugin_selection_on_path_execute(): the plugin is already installed so we should not have gotten this event.",
                    false
                );
invalid_data:
                server_access_plugin->create_ajax_result(ipath, false);
            }

            // create AJAX response
            server_access_plugin->ajax_output();
            return true;
        }
    }

    // let the output plugin take care of this otherwise
    //
    return output::output::instance()->on_path_execute(ipath);
}


/** \brief Add current user to timetracker.
 *
 * This function adds the current user to the list of users who can
 * use the timetracker.
 *
 * This adds a page with the calendar as:
 *
 * \code
 *      /timetracker/<user identifier>
 * \endcode
 *
 * \param[in] identifier  The identifier of the user to be added.
 */
void timetracker::add_calendar(int64_t const identifier)
{
    // basic setup
    output::output * output_plugin(output::output::instance());
    content::content * content_plugin(content::content::instance());
    //libdbproxy::table::pointer_t content_table(content_plugin->get_content_table());
    libdbproxy::table::pointer_t revision_table(content_plugin->get_revision_table());

    // setup locale
    QString const locale("xx"); // TODO: use the user defined locale by default, instead of "xx"

    // setup calendar info path
    content::path_info_t calendar_ipath;
    calendar_ipath.set_path(QString("%1/%2").arg(get_name(name_t::SNAP_NAME_TIMETRACKER_PATH)).arg(identifier));
    calendar_ipath.force_branch(snap_version::SPECIAL_VERSION_USER_FIRST_BRANCH);
    calendar_ipath.force_revision(snap_version::SPECIAL_VERSION_FIRST_REVISION);
    calendar_ipath.force_locale(locale);

    // create the actual page
    //
    // Note: if the page already exists, nothing happens;
    //       if the page was previously deleted, it gets "recreated"
    //       (reinstantiated as a NORMAL page)
    //
    content_plugin->create_content(calendar_ipath, output_plugin->get_plugin_name(), "timetracker/calendar");

    libdbproxy::row::pointer_t revision_row(revision_table->getRow(calendar_ipath.get_revision_key()));
    int64_t const start_date(f_snap->get_start_date());
    revision_row->getCell(content::get_name(content::name_t::SNAP_NAME_CONTENT_CREATED))->setValue(start_date);
    revision_row->getCell(content::get_name(content::name_t::SNAP_NAME_CONTENT_TITLE))->setValue(QString("Time Tracker Calendar"));
    revision_row->getCell(content::get_name(content::name_t::SNAP_NAME_CONTENT_BODY))->setValue(QString("<div>[timetracker::calendar]</div>"));

    // TODO: create a new permission for this user so he can access
    //       his calendar page (right now this is not required since
    //       the calendar is shown in the main page so we keep that
    //       work for later)

    {
        // assign the user with the permission of viewing his calendar
        //
        bool const source_unique(false);
        bool const destination_unique(false);

        content::path_info_t user_ipath;
        user_ipath.set_path(QString("user/%1").arg(identifier));

        QString const user_back_group(permissions::get_name(permissions::name_t::SNAP_NAME_PERMISSIONS_LINK_BACK_VIEW));
        QString const direct_link_name(permissions::get_name(permissions::name_t::SNAP_NAME_PERMISSIONS_DIRECT_ACTION_VIEW));

        links::link_info source(user_back_group, source_unique, user_ipath.get_key(), user_ipath.get_branch());
        links::link_info destination(direct_link_name, destination_unique, calendar_ipath.get_key(), calendar_ipath.get_branch());
        links::links::instance()->create_link(source, destination);
    }
}


/** \brief Generate the page main content.
 *
 * This function generates the main content of the page. Other
 * plugins will also have the event called if they subscribed and
 * thus will be given a chance to add their own content to the
 * main page. This part is the one that (in most cases) appears
 * as the main content on the page although the content of some
 * columns may be interleaved with this content.
 *
 * Note that this is NOT the HTML output. It is the \<page\> tag of
 * the snap XML file format. The theme layout XSLT will be used
 * to generate the final output.
 *
 * \param[in,out] ipath  The path being managed.
 * \param[in,out] page  The page being generated.
 * \param[in,out] body  The body being generated.
 */
void timetracker::on_generate_main_content(content::path_info_t & ipath, QDomElement & page, QDomElement & body)
{
    // our settings pages are like any standard pages
    output::output::instance()->on_generate_main_content(ipath, page, body);
}


//void timetracker::on_can_handle_dynamic_path(content::path_info_t & ipath, path::dynamic_plugin_t & plugin_info)
//{
//    QString const cpath(ipath.get_cpath());
//    if(cpath.startsWith(QString("%1/").arg(sendmail::get_name(sendmail::name_t::SNAP_NAME_SENDMAIL_UNSUBSCRIBE_PATH)))
//    || cpath.startsWith(QString("%1/install/").arg(get_name(name_t::SNAP_NAME_INFO_PLUGIN_SELECTION)))
//    || cpath.startsWith(QString("%1/remove/").arg(get_name(name_t::SNAP_NAME_INFO_PLUGIN_SELECTION))))
//    {
//        // tell the path plugin that this is ours
//        plugin_info.set_plugin(this);
//        return;
//    }
//}


void timetracker::on_check_for_redirect(content::path_info_t & ipath)
{
    QString const cpath(ipath.get_cpath());

    // we are only interested by timetracker pages
    snap_string_list const & segments(ipath.get_segments());
    if(segments.size() != 3
    || segments[0] != get_name(name_t::SNAP_NAME_TIMETRACKER_PATH)
    || segments[2].length() != 8)
    {
        // not /timetracker/<userid>/<date>
        return;
    }

    content::content * content_plugin(content::content::instance());
    libdbproxy::table::pointer_t content_table(content_plugin->get_content_table());

    bool ok(false);
    int const user_identifier(segments[1].toInt(&ok, 10));
    if(!ok)
    {
        // the <userid> segment is not a number
        return;
    }
    content::path_info_t calendar_ipath;
    calendar_ipath.set_path(QString("%1/%2").arg(get_name(name_t::SNAP_NAME_TIMETRACKER_PATH)).arg(user_identifier));

    if(!content_table->exists(calendar_ipath.get_key())
    || !content_table->getRow(calendar_ipath.get_key())->exists(content::get_name(content::name_t::SNAP_NAME_CONTENT_CREATED)))
    {
        // the user MUST exists
        return;
    }

    // 3rd segment must be YYYYMMDD
    //
    int const year(segments[2].mid(0, 4).toInt(&ok, 10));
    if(!ok
    || year < 2012
    || year > 3000)
    {
        // the <year> is not a number or is out of bounds
        return;
    }
    int const month(segments[2].mid(4, 2).toInt(&ok, 10));
    if(!ok
    || month <= 0
    || month > 12)
    {
        // the <month> is not a number or is out of bounds
        return;
    }
    int const max_day(f_snap->last_day_of_month(month, year));
    int const day(segments[2].mid(6, 2).toInt(&ok, 10));
    if(!ok
    || day <= 0
    || day > max_day)
    {
        // the <day> is not a number or is out of bounds
        return;
    }

    // okay all sections are valid, check whether it exists
    // if not, we can create that day now, the path is in "ipath"
    //
    if(content_table->exists(ipath.get_key())
    && content_table->getRow(ipath.get_key())->exists(content::get_name(content::name_t::SNAP_NAME_CONTENT_CREATED)))
    {
        // it exists, we do not need to do anything more
        return;
    }

    locale::locale * locale_plugin(locale::locale::instance());

    ipath.force_branch(snap_version::SPECIAL_VERSION_USER_FIRST_BRANCH);
    ipath.force_revision(static_cast<snap_version::basic_version_number_t>(snap_version::SPECIAL_VERSION_FIRST_REVISION));
    ipath.force_locale("xx");

    output::output * output_plugin(output::output::instance());
    content_plugin->create_content(ipath, output_plugin->get_plugin_name(), "timetracker/day");

    libdbproxy::row::pointer_t content_row(content_table->getRow(ipath.get_key()));
    content_row->getCell(layout::get_name(layout::name_t::SNAP_NAME_LAYOUT_LAYOUT))->setValue(QString("\"timetracker-parser\";"));
    //content_row->getCell(layout::get_name(layout::name_t::SNAP_NAME_LAYOUT_THEME))->setValue("\"...\";");
    content_row->getCell(editor::get_name(editor::name_t::SNAP_NAME_EDITOR_LAYOUT))->setValue(QString("\"timetracker-page\";"));

    libdbproxy::table::pointer_t revision_table(content_plugin->get_revision_table());
    libdbproxy::row::pointer_t revision_row(revision_table->getRow(ipath.get_revision_key()));
    int64_t const start_date(f_snap->get_start_date());
    revision_row->getCell(content::get_name(content::name_t::SNAP_NAME_CONTENT_CREATED))->setValue(start_date);
    // Note: we can hard code the date in the title since that specific page
    //       is for that specific day and it cannot be changed
    time_t const selected_date(SNAP_UNIX_TIMESTAMP(year, month, day, 0, 0, 0));
    QString const date(locale_plugin->format_date(selected_date));
    revision_row->getCell(content::get_name(content::name_t::SNAP_NAME_CONTENT_TITLE))->setValue(QString("Time Tracker: %1").arg(date));
    //revision_row->getCell(content::get_name(content::name_t::SNAP_NAME_CONTENT_BODY))->setValue("...using editor form...");

    {
        // assign the user with the permission of viewing his day
        //
        bool const source_unique(false);
        bool const destination_unique(false);

        content::path_info_t user_ipath;
        user_ipath.set_path(QString("user/%1").arg(user_identifier));

        QString const user_back_group(permissions::get_name(permissions::name_t::SNAP_NAME_PERMISSIONS_LINK_BACK_VIEW));
        QString const direct_link_name(permissions::get_name(permissions::name_t::SNAP_NAME_PERMISSIONS_DIRECT_ACTION_VIEW));

        links::link_info source(user_back_group, source_unique, user_ipath.get_key(), user_ipath.get_branch());
        links::link_info destination(direct_link_name, destination_unique, ipath.get_key(), ipath.get_branch());
        links::links::instance()->create_link(source, destination);
    }

    {
        // assign the user with the permission of editing his day
        //
        bool const source_unique(false);
        bool const destination_unique(false);

        content::path_info_t user_ipath;
        user_ipath.set_path(QString("user/%1").arg(user_identifier));

        QString const user_back_group(permissions::get_name(permissions::name_t::SNAP_NAME_PERMISSIONS_LINK_BACK_EDIT));
        QString const direct_link_name(permissions::get_name(permissions::name_t::SNAP_NAME_PERMISSIONS_DIRECT_ACTION_EDIT));

        links::link_info source(user_back_group, source_unique, user_ipath.get_key(), user_ipath.get_branch());
        links::link_info destination(direct_link_name, destination_unique, ipath.get_key(), ipath.get_branch());
        links::links::instance()->create_link(source, destination);
    }

    {
        // assign the user with the permission of adminitering his day
        //
        bool const source_unique(false);
        bool const destination_unique(false);

        content::path_info_t user_ipath;
        user_ipath.set_path(QString("user/%1").arg(user_identifier));

        QString const user_back_group(permissions::get_name(permissions::name_t::SNAP_NAME_PERMISSIONS_LINK_BACK_ADMINISTER));
        QString const direct_link_name(permissions::get_name(permissions::name_t::SNAP_NAME_PERMISSIONS_DIRECT_ACTION_ADMINISTER));

        links::link_info source(user_back_group, source_unique, user_ipath.get_key(), user_ipath.get_branch());
        links::link_info destination(direct_link_name, destination_unique, ipath.get_key(), ipath.get_branch());
        links::links::instance()->create_link(source, destination);
    }
}


void timetracker::on_replace_token(content::path_info_t & ipath, QDomDocument & xml, filter::filter::token_info_t & token)
{
    NOTUSED(xml);

    // we only support timetracker tokens
    //
    if(token.f_name.length() <= 13
    || !token.is_namespace("timetracker::"))
    {
        return;
    }

    switch(token.f_name[13].unicode())
    {
    case 'c':
        // [timetracker::calendar]
        if(token.is_token("timetracker::calendar"))
        {
            token.f_replacement = token_calendar(ipath);
        }
        break;

    case 'm':
        // [timetracker::main_page]
        if(token.is_token(get_name(name_t::SNAP_NAME_TIMETRACKER_MAIN_PAGE)))
        {
            token.f_replacement = token_main_page(ipath);
        }
        break;

    }
}


void timetracker::on_token_help(filter::filter::token_help_t & help)
{
    help.add_token("timetracker::calendar",
        "Display a calendar with work done and functionality so one"
        " can edit the data.");

    help.add_token("timetracker::main_page",
        "Display the timetracker main page.");
}


/** \brief Define the dynamic content of /timetracker.
 *
 * This function computes the content of the /timetracker page. There
 * are several possibilities:
 *
 * \li User is a Time Tracker Administrator
 *
 * In this case, the page is a list of all the existing Time Tracker
 * users plus a button to add another user. The /timetracker page is a list
 * and what is displayed is that list.
 */
QString timetracker::token_main_page(content::path_info_t & ipath)
{
    content::content * content_plugin(content::content::instance());
    users::users * users_plugin(users::users::instance());
    permissions::permissions * permissions_plugin(permissions::permissions::instance());
    list::list * list_plugin(list::list::instance());

    libdbproxy::table::pointer_t content_table(content_plugin->get_content_table());
    users::users::user_info_t user_info(users_plugin->get_user_info());

    QString result;

    // if we are an administrator, show the administrator view of this
    // page:
    //
    //  . our calendar or an Add Self button
    //  . an Add User button
    //  . list of users below
    //
    QString const & login_status(permissions_plugin->get_login_status());
    content::permission_flag allowed;

    // TODO: check whether the user is logged in? (or the page implies such
    //       already?)
    //
    path::path::instance()->access_allowed(permissions_plugin->get_user_path(), ipath, "administer", login_status, allowed);
    if(allowed.allowed())
    {
        // check whether the administrator has a calendar, if so, show it
        // otherwise show an "Add Self" button so the administrator can
        // create his own timetracker page but that is not mandatory
        //
        content::path_info_t calendar_ipath;
        ipath.get_child(calendar_ipath, QString("%1").arg(user_info.get_identifier()));
        calendar_ipath.set_parameter("date", ipath.get_parameter("date"));
        if(content_table->exists(calendar_ipath.get_key())
        && content_table->getRow(calendar_ipath.get_key())->exists(content::get_name(content::name_t::SNAP_NAME_CONTENT_CREATED)))
        {
            // calendar exists
            result += token_calendar(calendar_ipath);
        }
        else
        {
            // no calendar
            result +=
                    "<p>"
                        "You do no yet have a Time Tracker calendar. Click"
                        " <a class=\"button time-tracker add-self\""
                        " href=\"#add-self\">Add Self</a> button to add your"
                        " own calendar."
                    "</p>";
        }

        // as an administrator you can always add other users to the
        // Time Tracker system; users can be added as "User" only;
        // bookkeepers and other administrators cannot be added here
        // (at least not at this time.)
        //
        result +=
                "<div class=\"time-tracker-buttons\">"
                    "<a class=\"button timetracker-button time-tracker add-user\""
                    " href=\"#add-user\">Add User</a>"
                "</div>";

        // now show a list of users, we do not show their calendar because
        // that could be too much to generate here; the administrator can
        // click on a link to go see the calendar, though
        //
        result += QString("<div class=\"time-tracker-users\">%1</div>").arg(list_plugin->generate_list(ipath, ipath, 0, 30));
        return result;
    }

    {
        // regular users may have a timetracker page, defined as
        //
        //      /timetracker/<user-identifier>
        //
        // if that page exists, display that only (that is all what regular
        // users can do.)
        //
        content::path_info_t calendar_ipath;
        ipath.get_child(calendar_ipath, QString("%1").arg(user_info.get_identifier()));
        calendar_ipath.set_parameter("date", ipath.get_parameter("date"));
        if(content_table->exists(calendar_ipath.get_key())
        && content_table->getRow(calendar_ipath.get_key())->exists(content::get_name(content::name_t::SNAP_NAME_CONTENT_CREATED)))
        {
            // calendar exists
            return token_calendar(calendar_ipath);
        }

        return
            "<p>"
                "You do not yet have a Time Tracker page."
                " Please ask your administrator to create a page for you if you are"
                " allowed to use the Time Tracker system."
            "</p>";
    }

    return result;
}


QString timetracker::token_calendar(content::path_info_t & ipath)
{
    NOTUSED(ipath);

    locale::locale * locale_plugin(locale::locale::instance());
    users::users * users_plugin(users::users::instance());
    content::content * content_plugin(content::content::instance());
    libdbproxy::table::pointer_t content_table(content_plugin->get_content_table());
    libdbproxy::table::pointer_t revision_table(content_plugin->get_revision_table());

    bool ok(false);

    // validate the user identifier which we get in the path as
    // in "/timetracker/3"
    //
    snap_string_list const & segments(ipath.get_segments());
    if(segments.size() != 2)
    {
        // not a valid path
        throw snap_logic_exception(QString("the token_calendar() ipath is \"%1\" instead of exactly 2 segments").arg(ipath.get_cpath()));
    }
    int const user_identifier(segments[1].toInt(&ok, 10));
    if(!ok || user_identifier <= 0)
    {
        // not a valid number or zero or negative
        throw snap_logic_exception(QString("invalid user identifier in the token_calendar() ipath \"%1\" (not a number or out of range).").arg(ipath.get_cpath()));
    }
    QString ignore_status_key;
    users::users::status_t status(users_plugin->user_status_from_identifier(user_identifier, ignore_status_key));
    if(status == users::users::status_t::STATUS_UNKNOWN
    || status == users::users::status_t::STATUS_UNDEFINED
    || status == users::users::status_t::STATUS_NOT_FOUND)
    {
        // user does not seem to even exist on this website
        messages::messages::instance()->set_error(
            "Unknown User",
            "The specified user does not exist on this website.",
            QString("timetracker::token_calendar(): with with wrong user.").arg(user_identifier),
            false
        );
        return QString("<p class=\"bad-user\">Calendar not available.</p>");
    }
    if(!content_table->exists(ipath.get_key())
    || !content_table->getRow(ipath.get_key())->exists(content::get_name(content::name_t::SNAP_NAME_CONTENT_CREATED)))
    {
        // user does not have a calendar in this timetracker instance
        messages::messages::instance()->set_error(
            "Unassigned User",
            QString("The specified user (%1) was not added to this timetracker instance.").arg(user_identifier),
            "timetracker::token_calendar(): user is valid, but has no calendar in timetracker.",
            false
        );
        return QString("<p class=\"bad-user\">Calendar not available.</p>");
    }

    // by default we want to create the calendar for the current month,
    // if the main URI includes a query string, we may switch to a different
    // month or even year
    //
    time_t const now(f_snap->get_start_time());
    QString const date(locale_plugin->format_date(now, "%Y%m%d", true));
    int year (date.mid(0, 4).toInt(&ok));
    int month(date.mid(4, 2).toInt(&ok));
    int day  (date.mid(6, 2).toInt(&ok));
    int const today_year (year);
    int const today_month(month);
    int const today_day  (day);

    time_t selected_day(now);

    // optionally we expect a full date with format: %Y%m%d
    //
    // the date may come from the ipath or the query string
    //
    QString when(ipath.get_parameter("date"));
    if(when.isEmpty())
    {
        snap_uri const & uri(f_snap->get_uri());
        when = uri.query_option(get_name(name_t::SNAP_NAME_TIMETRACKER_DATE_QUERY_STRING));
    }
    if(!when.isEmpty())
    {
        int const when_year(when.mid(0, 4).toInt(&ok, 10));
        if(ok)
        {
            int const when_month(when.mid(4, 2).toInt(&ok, 10));
            if(ok)
            {
                int const when_day(when.mid(6, 2).toInt(&ok, 10));
                if(ok)
                {
                    int const max_when_day(f_snap->last_day_of_month(when_month, when_year));
                    if(when_year  >= 2000 && when_year  <= 3000
                    && when_month >= 1    && when_month <= 12
                    && when_day   >= 1    && when_day   <= max_when_day)
                    {
                        // an acceptable date, use it instead of 'now'
                        //
                        year  = when_year;
                        month = when_month;
                        day   = when_day;

                        // adjust the selected day
                        //
                        selected_day = SNAP_UNIX_TIMESTAMP(year, month, day, 0, 0, 0);
                    }
                }
            }
        }
    }

    QDomDocument doc;
    QDomElement root(doc.createElement("snap"));
    doc.appendChild(root);

    QDomElement month_tag(doc.createElement("month"));
    snap_dom::append_plain_text_to_node(month_tag, locale_plugin->format_date(selected_day, "%B", true));
    month_tag.setAttribute("mm", month);
    root.appendChild(month_tag);

    QDomElement year_tag(doc.createElement("year"));
    snap_dom::append_integer_to_node(year_tag, year);
    root.appendChild(year_tag);

    QDomElement days_tag(doc.createElement("days"));
    //days_tag.setAttribute("user-identifier", QString("%1").arg(user_identifier));
    days_tag.setAttribute("user-identifier", user_identifier);
    root.appendChild(days_tag);

    int const max_day(f_snap->last_day_of_month(month, year));

    // this part of the path to the day data does not change over time
    //
    QString const pre_defined_day_path(QString("%1/%2/%3%4")
                                .arg(get_name(name_t::SNAP_NAME_TIMETRACKER_PATH))
                                .arg(user_identifier)
                                .arg(year)
                                .arg(month, 2, 10, QChar('0')));

    for(int line(1); line <= max_day; )
    {
        QDomElement line_tag(doc.createElement("line"));
        days_tag.appendChild(line_tag);

        time_t const day_one(SNAP_UNIX_TIMESTAMP(year, month, line, 0, 0, 0));

        int const week_number(locale_plugin->format_date(day_one, "%U", true).toInt(&ok)); // user should be in control of which number to use, valid formats are: %U, %V, %W
        line_tag.setAttribute("week", week_number);

        int const week_day(locale_plugin->format_date(day_one, "%w", true).toInt(&ok));
        if(line == 1)
        {
            days_tag.setAttribute("first-week-day", week_day);
        }
#ifdef DEBUG
// this debug will work as long as Sunday is the first day, of course...
// later we may have to change the week_day test
        if(line != 1 && week_day != 0)
        {
            throw snap_logic_exception(QString("line = %1 and week_day = %2 when it should be zero.").arg(line).arg(week_day));
        }
#endif

        for(int w(0); w <= 6; ++w)
        {
            if(w < week_day
            || line > max_day)
            {
                // this is a day in the previous or next month
                // (a.k.a. out of range)
                //
                QDomElement no_day_tag(doc.createElement("no-day"));
                line_tag.appendChild(no_day_tag);
            }
            else
            {
                QDomElement day_tag(doc.createElement("day"));
                day_tag.setAttribute("day", line);
                line_tag.appendChild(day_tag);
                //snap_dom::append_plain_text_to_node(day_tag, QString("%1").arg(line)); -- using @day for now

                // does this day represent today?
                //
                if(line  == today_day
                && month == today_month
                && year  == today_year)
                {
                    day_tag.setAttribute("today", "today");
                }

                // we want to get the data to show directly in the calendar
                // first we have to make sure data exists
                //
                content::path_info_t day_ipath;
                day_ipath.set_path(QString("%1%2").arg(pre_defined_day_path).arg(line, 2, 10, QChar('0')));
                if(content_table->exists(day_ipath.get_key())
                && content_table->getRow(day_ipath.get_key())->exists(content::get_name(content::name_t::SNAP_NAME_CONTENT_CREATED)))
                {
                    libdbproxy::row::pointer_t row(revision_table->getRow(day_ipath.get_revision_key()));

                    // billing duration
                    QString const duration(row->getCell(get_name(name_t::SNAP_NAME_TIMETRACKER_BILLING_DURATION))->getValue().stringValue());
                    day_tag.setAttribute("billing_duration", duration);

                    // location
                    QString const location(row->getCell(get_name(name_t::SNAP_NAME_TIMETRACKER_LOCATION))->getValue().stringValue());
                    day_tag.setAttribute("location", location);

                    // transportation
                    QString const transportation(row->getCell(get_name(name_t::SNAP_NAME_TIMETRACKER_TRANSPORTATION))->getValue().stringValue());
                    day_tag.setAttribute("transportation", transportation);
                }

                ++line;
            }
        }
    }

//SNAP_LOG_WARNING("creating the calendar... [")(doc.toString())("]");

    QString result;

    xslt x;
    x.set_xsl_from_file("qrc:/xsl/layout/calendar-parser.xsl");
    x.set_document(doc);
    QString const output(x.evaluate_to_string());
    // "<output>"
    return output.mid(8, output.size() - 17);
}


/** \brief Initializes various dynamic widgets.
 *
 * This function gets called any time a field is initialized for use
 * in the editor.
 */
void timetracker::on_init_editor_widget(content::path_info_t & ipath, QString const & field_id, QString const & field_type, QDomElement & widget, libdbproxy::row::pointer_t row)
{
    NOTUSED(field_type);
    NOTUSED(row);

    QString const cpath(ipath.get_cpath());
    snap_string_list const & segments(ipath.get_segments());
    if(segments.size() == 3
    && segments[0] == "timetracker")
    {
        // we assume timetracker/<user id>/<day>
        //
        init_day_editor_widgets(field_id, widget);
    }
}


void timetracker::init_day_editor_widgets(QString const & field_id, QDomElement & widget)
{
    if(field_id == "client")
    {
        // the client dropdown is filled with the list of bookkeeping clients
        // this will be this way until we get a dynamic dropdown that let
        // you start typing and show only part of the list
        //
        list::list * list_plugin(list::list::instance());
        content::content * content_plugin(content::content::instance());
        libdbproxy::table::pointer_t revision_table(content_plugin->get_revision_table());

        QDomDocument doc(widget.ownerDocument());
        QDomElement preset(snap_dom::create_element(widget, "preset"));

        content::path_info_t client_list_ipath;
        client_list_ipath.set_path(bookkeeping::get_name(bookkeeping::name_t::SNAP_NAME_BOOKKEEPING_CLIENT_PATH));
        list::list_item_vector_t client_list(list_plugin->read_list(client_list_ipath, 0, 20));
        int const client_max_items(client_list.size());
        for(int idx(0); idx < client_max_items; ++idx)
        {
            QDomElement item(doc.createElement("item"));
            preset.appendChild(item);
            if(client_max_items == 1)
            {
                // for businesses which have a single client
                item.setAttribute("default", "default");
            }
            content::path_info_t value_ipath;
            value_ipath.set_path(client_list[idx].get_uri());
            item.setAttribute("value", client_list[idx].get_uri());
            QString const client_name(revision_table->getRow(value_ipath.get_revision_key())->getCell(content::get_name(content::name_t::SNAP_NAME_CONTENT_TITLE))->getValue().stringValue());
            snap::snap_dom::insert_html_string_to_xml_doc(item, client_name);
        }
    }
}


SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
