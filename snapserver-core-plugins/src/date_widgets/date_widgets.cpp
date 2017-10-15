// Snap Websites Server -- JavaScript WYSIWYG form widgets
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

#include "date_widgets.h"

#include "../locale/snap_locale.h"
#include "../messages/messages.h"

#include <snapwebsites/log.h>
#include <snapwebsites/mkgmtime.h>
#include <snapwebsites/not_reached.h>
#include <snapwebsites/not_used.h>
#include <snapwebsites/qdomxpath.h>

#include <snapwebsites/poison.h>


SNAP_PLUGIN_START(date_widgets, 1, 0)





/** \brief Get a fixed date_widgets plugin name.
 *
 * The date_widgets plugin makes use of different names in the database. This
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
    case name_t::SNAP_NAME_DATE_WIDGETS_DROPDOWN_TYPE:
        return "dropdown-date-edit";

    default:
        // invalid index
        throw snap_logic_exception("Invalid name_t::SNAP_NAME_DATE_WIDGETS_...");

    }
    NOTREACHED();
}








/** \brief Initialize the date_widgets plugin.
 *
 * This function is used to initialize the date_widgets plugin object.
 */
date_widgets::date_widgets()
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Clean up the date_widgets plugin.
 *
 * Ensure the date_widgets object is clean before it is gone.
 */
date_widgets::~date_widgets()
{
}


/** \brief Get a pointer to the date_widgets plugin.
 *
 * This function returns an instance pointer to the date_widgets plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the date_widgets plugin.
 */
date_widgets * date_widgets::instance()
{
    return g_plugin_date_widgets_factory.instance();
}


/** \brief A path or URI to a logo for this plugin.
 *
 * This function returns a 64x64 icons representing this plugin.
 *
 * \return A path to the logo.
 */
QString date_widgets::icon() const
{
    return "/images/editor/date-widgets-logo-64x64.png";
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
QString date_widgets::description() const
{
    return "This plugin offers several \"Date\" widgets for the Snap! editor."
        " By default, one can use a Line Edit widgets to let users type in a"
        " date. Only, it is often a lot faster to just click on the date in"
        " small calendar popup. The Date widget also offers a date range"
        " selection and a partial date selection (only one of the day, month"
        " or year; i.e. credit card expiration dates is only the year and the"
        " month.)";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString date_widgets::dependencies() const
{
    return "|editor|";
}


/** \brief Check whether updates are necessary.
 *
 * This function updates the database when a newer version is installed
 * and the corresponding updates where not run.
 *
 * This works for newly installed plugins and older plugins that were
 * updated.
 *
 * \param[in] last_updated  The UTC Unix date when the website was last
 *            updated (in micro seconds).
 *
 * \return The UTC Unix date of the last update of this plugin.
 */
int64_t date_widgets::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2017, 5, 28, 12, 46, 37, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Update the database with our content references.
 *
 * Send our content to the database so the system can find us when a
 * user references our administration pages, etc.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables
 *            added to the database by this update (in micro-seconds).
 */
void date_widgets::content_update(int64_t variables_timestamp)
{
    NOTUSED(variables_timestamp);

    content::content::instance()->add_xml(get_plugin_name());
}


/** \brief Initialize date_widgets.
 *
 * This function terminates the initialization of the date_widgets plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void date_widgets::bootstrap(snap_child * snap)
{
    f_snap = snap;

    SNAP_LISTEN(date_widgets, "editor", editor::editor, prepare_editor_form, _1);
    SNAP_LISTEN(date_widgets, "editor", editor::editor, value_to_string, _1);
    SNAP_LISTEN(date_widgets, "editor", editor::editor, string_to_value, _1);
    SNAP_LISTEN(date_widgets, "editor", editor::editor, init_editor_widget, _1, _2, _3, _4, _5);
    SNAP_LISTEN(date_widgets, "editor", editor::editor, validate_editor_post_for_widget, _1, _2, _3, _4, _5, _6, _7);
}


/** \brief Add the date widgets to the editor XSLT.
 *
 * The editor is extended by the locale plugin by adding a time zone
 * and other various widgets.
 *
 * \param[in] e  A pointer to the editor plugin.
 */
void date_widgets::on_prepare_editor_form(editor::editor * e)
{
    e->add_editor_widget_templates_from_file(":/xsl/date_widgets/date-form.xsl");
}


/** \brief Transform the dropdown-date value as required.
 *
 * This function transforms dropdown-date value to something more useable
 * than what the function returns by default.
 *
 * \param[in] value_info  A value_to_string_info_t object.
 *
 * \return true if the data_type is not known internally, false when the type
 *         was managed by this very function
 */
void date_widgets::on_value_to_string(editor::editor::value_to_string_info_t & value_info)
{
    if(value_info.is_done()
    || value_info.get_data_type() != "dropdown-date")
    {
        return;
    }

    value_info.set_type_name("date");

    // the value is an int64_t in microsecond; it will include a day,
    // a month and a year. The dropdown will know which do use and which
    // to ignore.
    //

    struct tm time_info;
    time_t seconds(value_info.get_value().safeInt64Value() / 1000000);
    gmtime_r(&seconds, &time_info);
    char buf[256];
    buf[0] = '\0';
    strftime(buf, sizeof(buf), "%Y/%m/%d", &time_info);
    value_info.result() = buf;

    value_info.set_status(editor::editor::value_to_string_info_t::status_t::DONE);
}


/** \brief Transform data to a QCassandraValue.
 *
 * This function transforms a value received from a POST into a
 * QCassandraValue to be saved in the database.
 *
 * \param[in] value_info  Information about the widget to be checked.
 *
 * \return false if the data_type is not known internally, true when the type
 *         was managed by this very function
 */
void date_widgets::on_string_to_value(editor::editor::string_to_value_info_t & value_info)
{
    if(value_info.is_done()
    || value_info.get_data_type() != "dropdown-date")
    {
        return;
    }

    value_info.set_type_name("date");

    // convert a YYYY/MM/DD date to 64 bit value in micro seconds
    //
    snap_string_list ymd(value_info.get_data().split('/'));

    // make sure we have exactly 3 entries
    if(ymd.size() != 3)
    {
        // that's invalid
        value_info.set_status(editor::editor::string_to_value_info_t::status_t::ERROR);
        return;
    }

    struct tm time_info;
    memset(&time_info, 0, sizeof(time_info));
    bool ok(false);

    // verify the year
    if(ymd[0] != "-")
    {
        // limit year between 1800 and 3000
        time_info.tm_year = ymd[0].toInt(&ok) - 1900;
        if(!ok || time_info.tm_year < (1800 - 1900) || time_info.tm_year > (3000 - 1900))
        {
            // that's invalid
            value_info.set_status(editor::editor::string_to_value_info_t::status_t::ERROR);
            return;
        }
    }
    else
    {
        // get current year as the default year
        time_t const now(time(nullptr));
        struct tm now_tm;
        localtime_r(&now, &now_tm);
        time_info.tm_year = now_tm.tm_year;
    }

    // verify the month
    if(ymd[1] != "-")
    {
        time_info.tm_mon = ymd[1].toInt(&ok) - 1;
        if(!ok || time_info.tm_mon < 0 || time_info.tm_mon > 11)
        {
            // that's invalid
            value_info.set_status(editor::editor::string_to_value_info_t::status_t::ERROR);
            return;
        }
    }
    // else time_info.tm_mon = 0; -- this is the default

    // verify the day
    if(ymd[2] != "-")
    {
        time_info.tm_mday = ymd[2].toInt(&ok);
        if(!ok || time_info.tm_mday < 1 || time_info.tm_mday > f_snap->last_day_of_month(time_info.tm_mon + 1, time_info.tm_year + 1900))
        {
            // that's invalid
            value_info.set_status(editor::editor::string_to_value_info_t::status_t::ERROR);
            return;
        }
    }
    else
    {
        time_info.tm_mday = 1;
    }

    time_t const t(mkgmtime(&time_info));
    value_info.result().setInt64Value(t * 1000000); // seconds to microseconds
    value_info.set_status(editor::editor::string_to_value_info_t::status_t::DONE);
}


/** \brief Finalize the dynamic part of the widget data.
 *
 * This function will transform the range defined in the \<include-year> tag
 * so it is easy to use in the XSLT parser.
 *
 * \param[in] ipath  The ipath to the page being worked on.
 * \param[in] field_id  The identifier of the field we are working on.
 * \param[in] field_type  The type of fields, we are interested in date widgets.
 * \param[in] widget  The DOM element representing this widget.
 * \param[in] row  The row where the user data is available.
 */
void date_widgets::on_init_editor_widget(content::path_info_t & ipath, QString const & field_id, QString const & field_type, QDomElement & widget, QtCassandra::QCassandraRow::pointer_t row)
{
    NOTUSED(ipath);
    NOTUSED(field_id);
    NOTUSED(row);

    if(field_type == get_name(name_t::SNAP_NAME_DATE_WIDGETS_DROPDOWN_TYPE))
    {
        QDomXPath dom_xpath;
        dom_xpath.setXPath("dropdown-date-edit/include-year");
        QDomXPath::node_vector_t include_year_tag(dom_xpath.apply(widget));
        if(include_year_tag.size() == 1)
        {
            // there is an <include-year> tag, check the attributes
            QDomElement e(include_year_tag[0].toElement());

            e.setAttribute("from", range_to_year(e.attribute("from")));
            e.setAttribute("to", range_to_year(e.attribute("to")));
        }
    }
}


QString date_widgets::range_to_year(QString const range_date)
{
    // to properly deal with a date, make sure the locale is
    // defined as expected
    //
    locale::locale * locale_plugin(locale::locale::instance());
    locale_plugin->set_locale();
    locale_plugin->set_timezone();

    // do we have a value number?
    bool ok(false);
    int const value(range_date.toInt(&ok, 10));
    if(ok
    && value >= 1
    && value <= 3000)
    {
        return QString("%1").arg(value);
    }

    // not a valid standalone number, try to convert as a date
    locale::locale::parse_error_t errcode;
    time_t const user_time(locale_plugin->parse_date(range_date, errcode));
    if(errcode == locale::locale::parse_error_t::PARSE_NO_ERROR)
    {
        // just return the year
        struct tm tm_user;
        localtime_r(&user_time, &tm_user);
        return QString("%1").arg(tm_user.tm_year + 1900);
    }

    // otherwise return the current year (i.e. "year(now)")
    time_t now(time(nullptr));
    struct tm tm_now;
    localtime_r(&now, &tm_now);
    return QString("%1").arg(tm_now.tm_year + 1900);
}



void date_widgets::on_validate_editor_post_for_widget(
            content::path_info_t & ipath,
            sessions::sessions::session_info & info,
            QDomElement const & widget,
            QString const & widget_name,
            QString const & widget_type,
            QString const & value,
            bool const is_secret)
{
    NOTUSED(ipath);
    NOTUSED(widget_type);

    // emptiness is checked with the system "required" feature
    if(!value.isEmpty())
    {
        QDomElement const filters(widget.firstChildElement("filters"));
        if(!filters.isNull())
        {
            // regular expression
            QDomElement const regex_tag(filters.firstChildElement("regex"));
            if(!regex_tag.isNull())
            {
                QString const regex_name(regex_tag.attribute("name"));
                if(regex_name == "partial-date")
                {
                    QString label(widget.firstChildElement("label").text());
                    if(label.isEmpty())
                    {
                        label = widget_name;
                    }

                    // partial date only--this means any one of the
                    // usual date parameters may be set to a dash
                    // instead of a number; we have to replace the
                    // dashes with a valid number first; for the
                    // month and day we use 1, for the year we
                    // use 2000; we expect the date to always be
                    // written as: YYYY/MM/DD
                    //
                    // The client is expected to properly build
                    // the date so any error in what we described
                    // earlier and we mark the date as invalid
                    //
                    snap_string_list date_parts(value.split('/'));
                    if(date_parts.size() != 3)
                    {
                        // this is incorrect
                        messages::messages * messages(messages::messages::instance());
                        messages->set_error(
                            "Invalid Value",
                            QString("\"%1\" is not a valid partial date for \"%2\".")
                                    .arg(form::form::html_64max(value, is_secret)).arg(label),
                            QString("the date did not validate for \"%1\"")
                                    .arg(widget_name),
                            is_secret
                        ).set_widget_name(widget_name);
                        info.set_session_type(sessions::sessions::session_info::session_info_type_t::SESSION_INFO_INCOMPATIBLE);
                    }
                    else
                    {
                        // we do our own parsing since the date is
                        // not specific to the current locale
                        //
                        // first we transform the "-" to 2000 (year) or 1
                        // (month/day of month) and then we parse the
                        // date using the snap_child parser which requires
                        // "-" as separators
                        //
                        if(date_parts[0] == "-")
                        {
                            date_parts[0] = "2000";
                        }
                        if(date_parts[1] == "-")
                        {
                            date_parts[1] = "1";
                        }
                        if(date_parts[2] == "-")
                        {
                            date_parts[2] = "1";
                        }
                        bool ok_year(false), ok_month(false), ok_day(false);
                        int const year(date_parts[0].toInt(&ok_year));
                        int const month(date_parts[1].toInt(&ok_month));
                        int const day(date_parts[2].toInt(&ok_day));
                        // since the data could be tainted, we check the
                        // values once here already...
                        if(!ok_year
                        || !ok_month
                        || !ok_day
                        || year < 1
                        || year > 3000
                        || month < 1
                        || month > 12
                        || day < 1
                        || day > f_snap->last_day_of_month(month, year))
                        {
                            messages::messages * messages(messages::messages::instance());
                            messages->set_error(
                                "Invalid Value",
                                QString("\"%1\" is not a valid partial date for \"%2\".")
                                        .arg(form::form::html_64max(value, is_secret)).arg(label),
                                QString("the date did not validate for \"%1\"")
                                        .arg(widget_name),
                                is_secret
                            ).set_widget_name(widget_name);
                            info.set_session_type(sessions::sessions::session_info::session_info_type_t::SESSION_INFO_INCOMPATIBLE);
                        }
                        else
                        {
                            // this checks the date yet again (probably not
                            // necessary?)
                            QString const us_date(QString("%1-%2-%3")
                                        .arg(year,  4, 10, QChar('0'))
                                        .arg(month, 2, 10, QChar('0'))
                                        .arg(day,   2, 10, QChar('0')));
                            time_t const date_value(f_snap->string_to_date(us_date));
                            if(date_value == -1)
                            {
                                messages::messages * messages(messages::messages::instance());
                                messages->set_error(
                                    "Invalid Value",
                                    QString("\"%1\" is not a valid partial date for \"%2\" (%3).")
                                            .arg(form::form::html_64max(value, is_secret)).arg(label).arg(us_date),
                                    QString("the date did not validate for \"%1\"")
                                            .arg(widget_name),
                                    is_secret
                                ).set_widget_name(widget_name);
                                info.set_session_type(sessions::sessions::session_info::session_info_type_t::SESSION_INFO_INCOMPATIBLE);
                            }
                            else
                            {
                                // Further the user may have defined a minimum and maximum
                                // (these will be ignored by the editor.cpp validation
                                // function because it will fail converting the partial date
                                // and assume that another validation will take over)
                                //
                                // minimum/maximum date
                                QDomElement min_date(filters.firstChildElement("min-date"));
                                QDomElement max_date(filters.firstChildElement("max-date"));
                                if(!min_date.isNull()
                                || !max_date.isNull())
                                {
                                    QString min_str("-1");
                                    QString max_str("-1");
                                    time_t min_time(-1);
                                    time_t max_time(-1);

                                    locale::locale * locale_plugin(locale::locale::instance());
                                    if(!min_date.isNull())
                                    {
                                        min_str = min_date.text();
                                        locale::locale::parse_error_t errcode;
                                        min_time = locale_plugin->parse_date(min_str, errcode);
                                        if(errcode != locale::locale::parse_error_t::PARSE_NO_ERROR)
                                        {
                                            throw editor::editor_exception_invalid_editor_form_xml(QString("the minimum date \"%1\" must be a valid date").arg(min_str));
                                        }
                                    }

                                    if(!max_date.isNull())
                                    {
                                        max_str = max_date.text();
                                        locale::locale::parse_error_t errcode;
                                        max_time = locale_plugin->parse_date(max_str, errcode);
                                        if(errcode != locale::locale::parse_error_t::PARSE_NO_ERROR)
                                        {
                                            throw editor::editor_exception_invalid_editor_form_xml(QString("the maximum date \"%1\" must be a valid date").arg(max_str));
                                        }
                                    }

                                    if(min_time != -1 && max_time != -1 && max_time < min_time)
                                    {
                                        throw editor::editor_exception_invalid_editor_form_xml(QString("the minimum date \"%1\" is not smaller than the maximum date \"%2\"").arg(min_str).arg(max_str));
                                    }

                                    // Note: if 'value' is not a valid date, we ignore the error
                                    //       at this point, we catch it below if the user asked
                                    //       for the format to be checked with a regex filter
                                    //       named 'date'.
                                    //  
                                    if(min_time != -1 && date_value < min_time)
                                    {
                                        // date is too small
                                        messages::messages * messages(messages::messages::instance());
                                        messages->set_error(
                                            "Too Old",
                                            QString("\"%1\" is too far in the past for \"%2\". The widget requires a date starting on \"%3\".")
                                                    .arg(form::form::html_64max(value, is_secret)).arg(label).arg(min_str),
                                            QString("unexpected date in \"%1\"").arg(widget_name),
                                            is_secret
                                        ).set_widget_name(widget_name);
                                        info.set_session_type(sessions::sessions::session_info::session_info_type_t::SESSION_INFO_INCOMPATIBLE);
                                    }

                                    if(max_time != -1 && date_value > max_time)
                                    {
                                        // date is too large
                                        messages::messages * messages(messages::messages::instance());
                                        messages->set_error(
                                            "Too Recent",
                                            QString("\"%1\" is too far in the future for \"%2\". The widget requires a date ending on \"%3\".")
                                                    .arg(form::form::html_64max(value, is_secret)).arg(label).arg(max_str),
                                            QString("unexpected date in \"%1\"").arg(widget_name),
                                            is_secret
                                        ).set_widget_name(widget_name);
                                        info.set_session_type(sessions::sessions::session_info::session_info_type_t::SESSION_INFO_INCOMPATIBLE);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}



SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
