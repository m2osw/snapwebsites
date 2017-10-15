// Snap Websites Server -- offer a website global locale settings page
// Copyright (C) 2011-2017  Made to Order Software Corp.
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

#include "locale_settings.h"

#include "../locale/snap_locale.h"

#include <snapwebsites/log.h>
#include <snapwebsites/not_reached.h>
#include <snapwebsites/not_used.h>

#include <unicode/uversion.h>

#include <iostream>

#include <snapwebsites/poison.h>


SNAP_PLUGIN_START(locale_settings, 1, 0)


///* \brief Get a fixed locale_settings name.
// *
// * The locale_settings plugin makes use of different names in the database.
// * This function ensures that you get the right spelling for a given name.
// *
// * \param[in] name  The name to retrieve.
// *
// * \return A pointer to the name.
// */
//char const * get_name(name_t name)
//{
//    switch(name)
//    {
//    default:
//        // invalid index
//        throw snap_logic_exception("invalid name_t::SNAP_NAME_LOCALE_SETTINGS_...");
//
//    }
//    NOTREACHED();
//}









/** \brief Initialize the locale plugin.
 *
 * This function is used to initialize the locale plugin object.
 */
locale_settings::locale_settings()
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Clean up the locale plugin.
 *
 * Ensure the locale object is clean before it is gone.
 */
locale_settings::~locale_settings()
{
}


/** \brief Get a pointer to the locale plugin.
 *
 * This function returns an instance pointer to the locale plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the locale plugin.
 */
locale_settings * locale_settings::instance()
{
    return g_plugin_locale_settings_factory.instance();
}


/** \brief Send users to the plugin settings.
 *
 * This path represents this plugin settings.
 */
QString locale_settings::settings_path() const
{
    return "/admin/settings/locale";
}


/** \brief A path or URI to a logo for this plugin.
 *
 * This function returns a 64x64 icons representing this plugin.
 *
 * \return A path to the logo.
 */
QString locale_settings::icon() const
{
    return "/images/locale/locale-logo-64x64.png";
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
QString locale_settings::description() const
{
    return "Define locale functions to be used throughout all the plugins."
        " It handles time and date, timezone, numbers, currency, etc.";
}


/** \brief Change the help URI to the base plugin.
 *
 * This help_uri() function returns the URI to the base plugin URI
 * since this plugin is just an extension and does not need to have
 * a separate help page.
 *
 * \return The URI to the locale plugin help page.
 */
QString locale_settings::help_uri() const
{
    // TBD: should we instead call the help_uri() of the locale plugin?
    //
    //      locale::locale::instance()->help_uri();
    //
    //      I'm afraid that it would be a bad example because the pointer
    //      may not be a good pointer anymore at this time (once we
    //      properly remove plugins that we loaded just to get their info.)
    //
    return "http://snapwebsites.org/help/plugin/locale";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString locale_settings::dependencies() const
{
    return "|editor|locale_widgets|";
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
int64_t locale_settings::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2015, 12, 20, 20, 32, 8, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Update the database with our content references.
 *
 * Send our content to the database so the system can find us when a
 * user references our pages.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added
 *                        to the database by this update (in micro-seconds).
 */
void locale_settings::content_update(int64_t variables_timestamp)
{
    NOTUSED(variables_timestamp);

    content::content::instance()->add_xml(get_plugin_name());
}


/** \brief Initialize the locale.
 *
 * This function terminates the initialization of the locale plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void locale_settings::bootstrap(snap_child * snap)
{
    f_snap = snap;

    SNAP_LISTEN(locale_settings, "filter", filter::filter, replace_token, _1, _2, _3);
    SNAP_LISTEN(locale_settings, "filter", filter::filter, token_help, _1);
}


/** \brief Allows one to display the current locale information.
 *
 * This function replace the following tokens:
 *
 * \li [locale::library] -- the name of the library used to support locale
 *                          specialization (i.e. ICU)
 * \li [locale::version] -- the version of the locale library in use
 * \li [locale::timezone-list] -- create an HTML table with the list of
 *                                timezones available on this system
 * \li [locale::locale-list] -- create an HTML table with the list of
 *                              countries available on this system
 */
void locale_settings::on_replace_token(content::path_info_t & ipath, QDomDocument & xml, filter::filter::token_info_t & token)
{
    NOTUSED(ipath);
    NOTUSED(xml);

    if(!token.is_namespace("locale::"))
    {
        return;
    }

    if(token.is_token("locale::library"))
    {
        // at this time we use the ICU exclusively
        token.f_replacement = "ICU";
        return;
    }

    if(token.is_token("locale::version"))
    {
        UVersionInfo icu_version;
        u_getVersion(icu_version);
        char buffer[U_MAX_VERSION_STRING_LENGTH];
        u_versionToString(icu_version, buffer);
        token.f_replacement = buffer;
        return;
    }

    if(token.is_token("locale::timezone_list"))
    {
        // create the table header
        QDomDocument doc("list");
        QDomElement style(doc.createElement("style"));
            QDomText stylesheet(doc.createCDATASection(
                    // Qt bug? the first period gets doubled
                    "first.period.gets.doubled.here{}"
                    "table.timezone-list"
                    "{"
                      "border-spacing: 0;"
                    "}"
                    "table.timezone-list th, table.timezone-list td"
                    "{"
                      "border-right: 1px solid black;"
                      "border-bottom: 1px solid black;"
                      "padding: 5px;"
                    "}"
                    "table.timezone-list tr th"
                    "{"
                      "border-top: 1px solid black;"
                    "}"
                    "table.timezone-list tr th:first-child, table.timezone-list tr td:first-child"
                    "{"
                      "border-left: 1px solid black;"
                    "}"
                ));
            style.appendChild(stylesheet);
        doc.appendChild(style);
        QDomElement table(doc.createElement("table"));
        table.setAttribute("class", "timezone-list");
        doc.appendChild(table);
            QDomElement thead(doc.createElement("thead"));
            table.appendChild(thead);
                QDomElement tr(doc.createElement("tr"));
                thead.appendChild(tr);
                    QDomElement th(doc.createElement("th"));
                    tr.appendChild(th);
                        QDomText text(doc.createTextNode("Name"));
                        th.appendChild(text);
                    th = doc.createElement("th");
                    tr.appendChild(th);
                        text = doc.createTextNode("Continent");
                        th.appendChild(text);
                    th = doc.createElement("th");
                    tr.appendChild(th);
                        text = doc.createTextNode("Country");
                        th.appendChild(text);
                    th = doc.createElement("th");
                    tr.appendChild(th);
                        text = doc.createTextNode("City");
                        th.appendChild(text);
                    th = doc.createElement("th");
                    tr.appendChild(th);
                        text = doc.createTextNode("Longitude");
                        th.appendChild(text);
                    th = doc.createElement("th");
                    tr.appendChild(th);
                        text = doc.createTextNode("Latitude");
                        th.appendChild(text);
                    th = doc.createElement("th");
                    tr.appendChild(th);
                        text = doc.createTextNode("Comment");
                        th.appendChild(text);
            QDomElement tbody(doc.createElement("tbody"));
            table.appendChild(tbody);

        // add the table content
        locale::locale::timezone_list_t const & list(locale::locale::instance()->get_timezone_list());
        int const max_items(list.size());
        for(int idx(0); idx < max_items; ++idx)
        {
                tr = doc.createElement("tr");
                tbody.appendChild(tr);
                    QDomElement td(doc.createElement("td"));
                    tr.appendChild(td);
                        text = doc.createTextNode(list[idx].f_timezone_name);
                        td.appendChild(text);
                    td = doc.createElement("td");
                    tr.appendChild(td);
                        text = doc.createTextNode(list[idx].f_continent);
                        td.appendChild(text);
                    td = doc.createElement("td");
                    tr.appendChild(td);
                        text = doc.createTextNode(list[idx].f_country_or_state.isEmpty() ? list[idx].f_2country : list[idx].f_country_or_state);
                        td.appendChild(text);
                    td = doc.createElement("td");
                    tr.appendChild(td);
                        text = doc.createTextNode(list[idx].f_city);
                        td.appendChild(text);
                    td = doc.createElement("td");
                    tr.appendChild(td);
                        text = doc.createTextNode(QString("%1").arg(list[idx].f_longitude));
                        td.appendChild(text);
                    td = doc.createElement("td");
                    tr.appendChild(td);
                        text = doc.createTextNode(QString("%1").arg(list[idx].f_latitude));
                        td.appendChild(text);
                    td = doc.createElement("td");
                    tr.appendChild(td);
                        text = doc.createTextNode(list[idx].f_comment);
                        td.appendChild(text);
        }

        token.f_replacement = doc.toString(-1);
        return;
    }

    if(token.is_token("locale::locale_list"))
    {
        // create the table header
        QDomDocument doc("list");
        QDomElement style(doc.createElement("style"));
            QDomText stylesheet(doc.createCDATASection(
                    // Qt bug? the first period gets doubled
                    "first.period.gets.doubled.here{}"
                    "table.locale-list"
                    "{"
                      "border-spacing: 0;"
                    "}"
                    "table.locale-list th, table.locale-list td"
                    "{"
                      "border-right: 1px solid black;"
                      "border-bottom: 1px solid black;"
                      "padding: 5px;"
                    "}"
                    "table.locale-list tr th"
                    "{"
                      "border-top: 1px solid black;"
                    "}"
                    "table.locale-list tr th:first-child, table.locale-list tr td:first-child"
                    "{"
                      "border-left: 1px solid black;"
                    "}"
                ));
            style.appendChild(stylesheet);
        doc.appendChild(style);
        QDomElement table(doc.createElement("table"));
        table.setAttribute("class", "locale-list");
        doc.appendChild(table);
            QDomElement thead(doc.createElement("thead"));
            table.appendChild(thead);
                QDomElement tr(doc.createElement("tr"));
                thead.appendChild(tr);
                    QDomElement th(doc.createElement("th"));
                    tr.appendChild(th);
                        QDomText text(doc.createTextNode("Name"));
                        th.appendChild(text);
                    th = doc.createElement("th");
                    th.setAttribute("colspan", 4);
                    tr.appendChild(th);
                        text = doc.createTextNode("Abbreviation");
                        th.appendChild(text);
                    th = doc.createElement("th");
                    th.setAttribute("colspan", 4);
                    tr.appendChild(th);
                        text = doc.createTextNode("Display Names");
                        th.appendChild(text);
            QDomElement tbody(doc.createElement("tbody"));
            table.appendChild(tbody);

        // add the table content
        locale::locale::locale_list_t const & list(locale::locale::instance()->get_locale_list());
        int const max_items(list.size());
        for(int idx(0); idx < max_items; ++idx)
        {
                tr = doc.createElement("tr");
                tbody.appendChild(tr);
                    QDomElement td(doc.createElement("td"));
                    tr.appendChild(td);
                        text = doc.createTextNode(list[idx].f_locale);
                        td.appendChild(text);
        struct locale_parameters_t
        {
            // the following is in the order it is defined in the
            // full name although all parts except the language are
            // optional; the script is rare, the variant is used
            // quite a bit
            QString                 f_language;
            QString                 f_variant;
            QString                 f_country;
            QString                 f_script;
        };

        QString                 f_locale;           // name to use to setup this locale
        locale_parameters_t     f_abbreviations;
        locale_parameters_t     f_display_names;    // all names in "current" locale
                    td = doc.createElement("td");
                    tr.appendChild(td);
                        text = doc.createTextNode(list[idx].f_abbreviations.f_language);
                        td.appendChild(text);
                    td = doc.createElement("td");
                    tr.appendChild(td);
                        text = doc.createTextNode(list[idx].f_abbreviations.f_variant);
                        td.appendChild(text);
                    td = doc.createElement("td");
                    tr.appendChild(td);
                        text = doc.createTextNode(list[idx].f_abbreviations.f_country);
                        td.appendChild(text);
                    td = doc.createElement("td");
                    tr.appendChild(td);
                        text = doc.createTextNode(list[idx].f_abbreviations.f_script);
                        td.appendChild(text);
                    td = doc.createElement("td");
                    tr.appendChild(td);
                        text = doc.createTextNode(list[idx].f_display_names.f_language);
                        td.appendChild(text);
                    td = doc.createElement("td");
                    tr.appendChild(td);
                        text = doc.createTextNode(list[idx].f_display_names.f_variant);
                        td.appendChild(text);
                    td = doc.createElement("td");
                    tr.appendChild(td);
                        text = doc.createTextNode(list[idx].f_display_names.f_country);
                        td.appendChild(text);
                    td = doc.createElement("td");
                    tr.appendChild(td);
                        text = doc.createTextNode(list[idx].f_display_names.f_script);
                        td.appendChild(text);
        }

        token.f_replacement = doc.toString(-1);
        return;
    }
}


void locale_settings::on_token_help(filter::filter::token_help_t & help)
{
    help.add_token("locale::library",
        "Output the name of the library used to handle the locale data.");

    help.add_token("locale::timezone_list",
        "Output an HTML table with the completely list of all the"
        " available timezones.");

    help.add_token("locale::locale_list",
        "Output an HTML table with the completely list of all the"
        " available locales.");
}


//
// A reference of the library ICU library can be found here:
// /usr/include/x86_64-linux-gnu/unicode/timezone.h
// file:///usr/share/doc/icu-doc/html/index.html
//

SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
