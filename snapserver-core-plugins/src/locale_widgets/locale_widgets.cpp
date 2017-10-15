// Snap Websites Server -- offer a plethora of localized editor widgets
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

#include "locale_widgets.h"

#include "../locale/snap_locale.h"
#include "../messages/messages.h"

#include <snapwebsites/log.h>
#include <snapwebsites/not_reached.h>
#include <snapwebsites/not_used.h>

#include <iostream>

#include <snapwebsites/poison.h>


SNAP_PLUGIN_START(locale_widgets, 1, 0)


///* \brief Get a fixed locale_widgets name.
// *
// * The locale_widgets plugin makes use of different names in the database.
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
//    case SNAP_NAME_LOCALE_WIDGETS_NAME:
//        return "locale_widgets::name";
//
//    default:
//        // invalid index
//        throw snap_logic_exception("invalid SNAP_NAME_LOCALE_WIDGETS_...");
//
//    }
//    NOTREACHED();
//}









/** \brief Initialize the locale_widgets plugin.
 *
 * This function is used to initialize the locale_widgets plugin object.
 */
locale_widgets::locale_widgets()
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Clean up the locale_widgets plugin.
 *
 * Ensure the locale_widgets object is clean before it is gone.
 */
locale_widgets::~locale_widgets()
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
locale_widgets * locale_widgets::instance()
{
    return g_plugin_locale_widgets_factory.instance();
}


/** \brief Send users to the plugin settings.
 *
 * This path represents this plugin settings.
 */
QString locale_widgets::settings_path() const
{
    return "/admin/settings/locale";
}


/** \brief A path or URI to a logo for this plugin.
 *
 * This function returns a 64x64 icons representing this plugin.
 *
 * \return A path to the logo.
 */
QString locale_widgets::icon() const
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
QString locale_widgets::description() const
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
QString locale_widgets::help_uri() const
{
    // TBD: should we instead call the help_uri() of the locale plugin?
    //
    //      locale::locale::instance()->help_uri();
    //
    //      I am afraid that it would be a bad example because the pointer
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
QString locale_widgets::dependencies() const
{
    return "|editor|locale|";
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
 *                          updated (in micro seconds).
 *
 * \return The UTC Unix date of the last update of this plugin.
 */
int64_t locale_widgets::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2017, 5, 13, 18, 15, 30, content_update);

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
void locale_widgets::content_update(int64_t variables_timestamp)
{
    NOTUSED(variables_timestamp);

    content::content::instance()->add_xml(get_plugin_name());
}


/** \brief Initialize the locale_widgets.
 *
 * This function terminates the initialization of the locale_widgets plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void locale_widgets::bootstrap(snap_child * snap)
{
    f_snap = snap;

    SNAP_LISTEN(locale_widgets, "editor", editor::editor, init_editor_widget, _1, _2, _3, _4, _5);
    SNAP_LISTEN(locale_widgets, "editor", editor::editor, prepare_editor_form, _1);
    SNAP_LISTEN(locale_widgets, "editor", editor::editor, string_to_value, _1);
    SNAP_LISTEN(locale_widgets, "editor", editor::editor, value_to_string, _1);
    SNAP_LISTEN(locale_widgets, "editor", editor::editor, validate_editor_post_for_widget, _1, _2, _3, _4, _5, _6, _7);
}


/** \brief Add the locale widget to the editor XSLT.
 *
 * The editor is extended by the locale plugin by adding a time zone
 * and other various widgets.
 *
 * \param[in] e  A pointer to the editor plugin.
 */
void locale_widgets::on_prepare_editor_form(editor::editor * e)
{
    e->add_editor_widget_templates_from_file(":/xsl/locale_widgets/locale-form.xsl");
}


/** \brief Initialize the continent and city widgets.
 *
 * This function initializes continent and city widgets with timezone
 * information.
 *
 * \param[in] ipath  The path of the page being generated.
 * \param[in] field_id  The name of the field being initialized.
 * \param[in] field_type  The type of field being initialized.
 * \param[in] widget  The XML DOM widget.
 * \param[in] row  The row with the saved data.
 */
void locale_widgets::on_init_editor_widget(content::path_info_t & ipath, QString const & field_id, QString const & field_type, QDomElement & widget, QtCassandra::QCassandraRow::pointer_t row)
{
    NOTUSED(field_id);
    NOTUSED(row);

    QString const cpath(ipath.get_cpath());
    if(field_type == "locale_timezone")
    {
        QDomDocument doc(widget.ownerDocument());

        // we need script and CSS complements for timezones
        // but we do not have the right document (i.e. we need the -parser.xsl
        // and not the -page.xml file...) but we can put them in the form
        // defining the widget too
        //content::content::instance()->add_javascript(doc, "locale-timezone");
        //content::content::instance()->add_css(doc, "locale-timezone");

        // Was a default or current value defined?
        QString default_continent;
        QString default_city;

        QDomElement value_tag;
        value_tag = widget.firstChildElement("value");
        if(!value_tag.isNull())
        {
            // no tags in a timezone value, so we can just use text()
            QString const current_timezone(value_tag.text());
            snap_string_list const current_timezone_segments(current_timezone.split('/'));

            if(current_timezone_segments.size() == 2)
            {
                default_continent = current_timezone_segments[0];
                default_city = current_timezone_segments[1];
            }
        }

        // setup a dropdown preset list for continents and one for cities
        QDomElement preset_continent(doc.createElement("preset_continent"));
        widget.appendChild(preset_continent);

        QDomElement preset_city(doc.createElement("preset_city"));
        widget.appendChild(preset_city);

        // get the complete list
        locale::locale::timezone_list_t const & list(locale::locale::instance()->get_timezone_list());

        // extract the continents as we setup the cities
        QMap<QString, bool> continents;
        int const max(list.size());
        for(int idx(0); idx < max; ++idx)
        {
            // skip on a few "continent" which we really do not need
            QString const continent(list[idx].f_continent);
            if(continent == "Etc"
            || continent == "SystemV"
            || continent == "US")
            {
                continue;
            }

            continents[continent] = true;

            // create one item per city
            QDomElement item(doc.createElement("item"));
            preset_city.appendChild(item);
            QString const value_city(list[idx].f_city);
            if(value_city == default_city)
            {
                item.setAttribute("default", "default");
            }
            item.setAttribute("class", continent);
            QDomText text(doc.createTextNode(value_city));
            item.appendChild(text);
        }

        // now use the map of continents to add them to the list
        for(auto it(continents.begin());
                 it != continents.end();
                 ++it)
        {
            // create one item per continent
            QDomElement item(doc.createElement("item"));
            preset_continent.appendChild(item);
            QString const value_continent(it.key());
            if(value_continent == default_continent)
            {
                item.setAttribute("default", "default");
            }
            QDomText text(doc.createTextNode(value_continent));
            item.appendChild(text);
        }
    }
}


/** \brief Transform data to a QCassandraValue.
 *
 * This function transforms a value received from a POST into a
 * QCassandraValue to be saved in the database.
 *
 * \param[in] value_info  Information about the widget to be checked.
 */
void locale_widgets::on_string_to_value(editor::editor::string_to_value_info_t & value_info)
{
    if(!value_info.is_done()
    && value_info.get_data_type() == "locale_timezone")
    {
        value_info.set_type_name("locale timezone");

        value_info.result().setStringValue(value_info.get_data());
        value_info.set_status(editor::editor::string_to_value_info_t::status_t::DONE);
        return;
    }
}


/** \brief Transform a database value to a string for display.
 *
 * This function transforms a database value back to a string as displayed
 * to end users.
 *
 * The value must be valid. Invalid values do not make it in the result
 * string. In other words, the result string remains unchanged if the
 * input value is considered invalid.
 *
 * \param[in] value_info  A value_to_string_info_t object.
 *
 * \return true if the data_type is not known internally, false when the type
 *         was managed by this very function
 */
void locale_widgets::on_value_to_string(editor::editor::value_to_string_info_t & value_info)
{
    if(!value_info.is_done()
    && value_info.get_data_type() == "locale_timezone")
    {
        value_info.set_type_name("locale timezone");

        value_info.result() = value_info.get_value().stringValue();
        value_info.set_status(editor::editor::value_to_string_info_t::status_t::DONE);
        return;
    }
}


/** \brief Add some new validations.
 *
 * This function adds support for the following validations:
 *
 * \li \<filters>\<country/>\</filters> -- make sure that
 *     \p value represents a valid (known) country name.
 */
void locale_widgets::on_validate_editor_post_for_widget(
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

    messages::messages * messages(messages::messages::instance());

    // the label corresponding to that widget for better/cleaner error
    // messages
    //
    QString label(widget.firstChildElement("label").text());
    if(label.isEmpty())
    {
        label = widget_name;
    }

    // verify that the entry is a country
    QDomElement filters(widget.firstChildElement("filters"));
    if(!filters.isNull()
    && !value.isEmpty()) // emptiness was checked with the "required" test
    {
        QDomElement country_tag(filters.firstChildElement("country"));
        if(!country_tag.isNull())
        {
            QString const mode(country_tag.attribute("mode"));

            QString country(value);
            bool valid(false);
            if(mode == "2-letters")
            {
                if(country.length() == 2)
                {
                    valid = f_snap->verify_country_name(country);
                }
            }
            else if(mode == "full-name")
            {
                if(country.length() > 2)
                {
                    valid = f_snap->verify_country_name(country);
                }
            }
            else //if(mode == "any" or undefined)
            {
                valid = f_snap->verify_country_name(country);
            }
            //else -- TBD: should we err if invalid?

            if(!valid)
            {
                messages->set_error(
                    "Validation Failed",
                    QString("\"%1\" is not a valid country name.")
                            .arg(form::form::html_64max(value, is_secret)).arg(label),
                    QString("\"%1\" is not the name of a known country.")
                            .arg(widget_name),
                    is_secret
                ).set_widget_name(widget_name);
                info.set_session_type(sessions::sessions::session_info::session_info_type_t::SESSION_INFO_INCOMPATIBLE);
            }
        }
    }
}



//
// A reference of the library ICU library can be found here:
// /usr/include/x86_64-linux-gnu/unicode/timezone.h
// file:///usr/share/doc/icu-doc/html/index.html
//

SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
