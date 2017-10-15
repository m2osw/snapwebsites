// Snap Websites Server -- offer a small window to accept/refuse use of cookies
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
//
// Using 3rd party library from silktide:
// https://silktide.com/tools/cookie-consent/

#include "cookie_consent_silktide.h"

#include "../attachment/attachment.h"

#include <snapwebsites/log.h>
#include <snapwebsites/not_reached.h>
#include <snapwebsites/not_used.h>

#include <as2js/json.h>

#include <iostream>

#include <snapwebsites/poison.h>


SNAP_PLUGIN_START(cookie_consent_silktide, 1, 0)


/* \brief Get a fixed cookie_consent_silktide name.
 *
 * The cookie_consent_silktide plugin makes use of different names
 * in the database. This function ensures that you get the right
 * spelling for a given name.
 *
 * \param[in] name  The name to retrieve.
 *
 * \return A pointer to the name.
 */
char const * get_name(name_t name)
{
    switch(name)
    {
    case name_t::SNAP_NAME_COOKIE_CONSENT_SILKTIDE_CONSENT_DURATION:
        return "cookie_consent_silktide::consent_duration";

    case name_t::SNAP_NAME_COOKIE_CONSENT_SILKTIDE_DISMISS:
        return "cookie_consent_silktide::dismiss";

    case name_t::SNAP_NAME_COOKIE_CONSENT_SILKTIDE_DOMAIN:
        return "cookie_consent_silktide::domain";

    case name_t::SNAP_NAME_COOKIE_CONSENT_SILKTIDE_JAVASCRIPT_FILENAME:
        return "cookie-consent-silktide-options";

    case name_t::SNAP_NAME_COOKIE_CONSENT_SILKTIDE_JAVASCRIPT_OPTIONS:
        return "js/cookie-consent-silktide/cookie-consent-silktide-options.js";

    case name_t::SNAP_NAME_COOKIE_CONSENT_SILKTIDE_JAVASCRIPT_OPTIONS_DEPENDENCY:
        return "cookie-consent-silktide (>= 2)";

    case name_t::SNAP_NAME_COOKIE_CONSENT_SILKTIDE_JAVASCRIPT_OPTIONS_PARENT_PATH:
        return "js/cookie-consent-silktide";

    case name_t::SNAP_NAME_COOKIE_CONSENT_SILKTIDE_JAVASCRIPT_PLUGIN: // the author of cookieconsent called his file plugin.js for a while...
        return "cookie-consent-silktide";

    case name_t::SNAP_NAME_COOKIE_CONSENT_SILKTIDE_JAVASCRIPT_TYPE:
        return "attachment/public";

    case name_t::SNAP_NAME_COOKIE_CONSENT_SILKTIDE_JAVASCRIPT_VERSION:
        return "cookie_consent_silktide::javascript_version";

    case name_t::SNAP_NAME_COOKIE_CONSENT_SILKTIDE_LEARN_MORE_LABEL:
        return "cookie_consent_silktide::learn_more_label";

    case name_t::SNAP_NAME_COOKIE_CONSENT_SILKTIDE_LEARN_MORE_URI:
        return "cookie_consent_silktide::learn_more_uri";

    case name_t::SNAP_NAME_COOKIE_CONSENT_SILKTIDE_MESSAGE:
        return "cookie_consent_silktide::message";

    case name_t::SNAP_NAME_COOKIE_CONSENT_SILKTIDE_PATH:
        return "admin/settings/cookie-consent-silktide";

    case name_t::SNAP_NAME_COOKIE_CONSENT_SILKTIDE_THEME:
        return "cookie_consent_silktide::theme";

    default:
        // invalid index
        throw snap_logic_exception("invalid name_t::SNAP_NAME_COOKIE_CONTENT_SILKTIDE_...");

    }
    NOTREACHED();
}









/** \brief Initialize the locale plugin.
 *
 * This function is used to initialize the locale plugin object.
 */
cookie_consent_silktide::cookie_consent_silktide()
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Clean up the locale plugin.
 *
 * Ensure the locale object is clean before it is gone.
 */
cookie_consent_silktide::~cookie_consent_silktide()
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
cookie_consent_silktide * cookie_consent_silktide::instance()
{
    return g_plugin_cookie_consent_silktide_factory.instance();
}


/** \brief Send users to the plugin settings.
 *
 * This path represents this plugin settings.
 */
QString cookie_consent_silktide::settings_path() const
{
    return "/admin/settings/cookie-consent-silktide";
}


/** \brief A path or URI to a logo for this plugin.
 *
 * This function returns a 64x64 icons representing this plugin.
 *
 * \return A path to the logo.
 */
QString cookie_consent_silktide::icon() const
{
    return "/images/cookie-consent-silktide/cookie-consent-silktide-logo-64x64.png";
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
QString cookie_consent_silktide::description() const
{
    return "Show an in-page popup allowing users to agree on use of cookies."
        " This plugin makes use the third party silktide cookie-consent tool.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString cookie_consent_silktide::dependencies() const
{
    return "|attachment|editor|layout|";
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
int64_t cookie_consent_silktide::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2016, 3, 27, 15, 30, 34, content_update);

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
void cookie_consent_silktide::content_update(int64_t variables_timestamp)
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
void cookie_consent_silktide::bootstrap(snap_child * snap)
{
    f_snap = snap;

    SNAP_LISTEN(cookie_consent_silktide, "layout", layout::layout, generate_header_content, _1, _2, _3);
    SNAP_LISTEN(cookie_consent_silktide, "editor", editor::editor, save_editor_fields, _1);
}


/** \brief Setup page for the editor.
 *
 * The editor has a set of dynamic parameters that the users are offered
 * to setup. These parameters need to be sent to the user and we use this
 * function for that purpose.
 *
 * \todo
 * Look for a way to generate the editor data only if necessary (too
 * complex for now.)
 *
 * \param[in,out] ipath  The path being managed.
 * \param[in,out] header  The header being generated.
 * \param[in,out] metadata  The metadata being generated.
 */
void cookie_consent_silktide::on_generate_header_content(content::path_info_t & ipath, QDomElement & header, QDomElement & metadata)
{
    NOTUSED(ipath);
    NOTUSED(metadata);

    snap_uri const & main_uri(f_snap->get_uri());
    if(main_uri.has_query_option("iframe"))
    {
        if(main_uri.query_option("iframe") == "true")
        {
            // avoid the cookie consent from appearing in iframes
            // (assuming the developers properly setup the URIs with
            // the iframe query string...)
            return;
        }
    }

    QDomDocument doc(header.ownerDocument());

    // check whether the adminstrator defined options for this plugin;
    // if so we have a JavaScript with a small JSON file...
    //
    bool has_options(false);
    content::content * content_plugin(content::content::instance());
    QtCassandra::QCassandraTable::pointer_t content_table(content_plugin->get_content_table());
    content::path_info_t options_ipath;
    options_ipath.set_path(get_name(name_t::SNAP_NAME_COOKIE_CONSENT_SILKTIDE_JAVASCRIPT_OPTIONS));
    if(content_table->exists(options_ipath.get_key()))
    {
        content::path_info_t::status_t status(options_ipath.get_status());
        has_options = status.get_state() == content::path_info_t::status_t::state_t::NORMAL;
    }

    if(has_options)
    {
        content::content::instance()->add_javascript(doc, get_name(name_t::SNAP_NAME_COOKIE_CONSENT_SILKTIDE_JAVASCRIPT_FILENAME));
    }
    else
    {
        content::content::instance()->add_javascript(doc, get_name(name_t::SNAP_NAME_COOKIE_CONSENT_SILKTIDE_JAVASCRIPT_PLUGIN));
    }
}


/** \brief Generate JavaScript code with user defined settings.
 *
 * This function generates the JavaScript to use with the
 * cookie-consent-silktide.js
 *
 * \param[in,out] save_info  
 */
void cookie_consent_silktide::on_save_editor_fields(editor::save_info_t & save_info)
{
    if(save_info.ipath().get_cpath() != "admin/settings/cookie-consent-silktide")
    {
        return;
    }

    SNAP_LOG_INFO("saving silktide options to \"")(get_name(name_t::SNAP_NAME_COOKIE_CONSENT_SILKTIDE_JAVASCRIPT_OPTIONS))("\".");

    as2js::String temp_str;
    as2js::Position pos;
    as2js::JSON::JSONValue::object_t empty_object;
    as2js::JSON::JSONValue::array_t empty_array;
    as2js::JSON::JSONValue::pointer_t field;
    as2js::JSON::JSONValue::pointer_t body(new as2js::JSON::JSONValue(pos, empty_object));

    // read the data from the database (it could already be in memory
    // for those we verify the format of which as far as I know is all
    // of them!)
    {
        temp_str = save_info.revision_row()->cell(get_name(name_t::SNAP_NAME_COOKIE_CONSENT_SILKTIDE_MESSAGE))->value().stringValue().toUtf8().data();
        if(!temp_str.empty())
        {
            field.reset(new as2js::JSON::JSONValue(pos, temp_str));
            body->set_member("message", field);
        }
    }

    {
        temp_str = save_info.revision_row()->cell(get_name(name_t::SNAP_NAME_COOKIE_CONSENT_SILKTIDE_LEARN_MORE_LABEL))->value().stringValue().toUtf8().data();
        if(!temp_str.empty())
        {
            field.reset(new as2js::JSON::JSONValue(pos, temp_str));
            body->set_member("learnMore", field);
        }
    }

    {
        temp_str = save_info.revision_row()->cell(get_name(name_t::SNAP_NAME_COOKIE_CONSENT_SILKTIDE_LEARN_MORE_URI))->value().stringValue().toUtf8().data();
        if(!temp_str.empty())
        {
            field.reset(new as2js::JSON::JSONValue(pos, temp_str));
            body->set_member("link", field);
        }
    }

    {
        temp_str = save_info.revision_row()->cell(get_name(name_t::SNAP_NAME_COOKIE_CONSENT_SILKTIDE_DISMISS))->value().stringValue().toUtf8().data();
        if(!temp_str.empty())
        {
            field.reset(new as2js::JSON::JSONValue(pos, temp_str));
            body->set_member("dismiss", field);
        }
    }

    {
        temp_str = save_info.revision_row()->cell(get_name(name_t::SNAP_NAME_COOKIE_CONSENT_SILKTIDE_DOMAIN))->value().stringValue().toUtf8().data();
        if(!temp_str.empty())
        {
            field.reset(new as2js::JSON::JSONValue(pos, temp_str));
            body->set_member("domain", field);
        }
    }

    {
        int64_t consent_duration(save_info.revision_row()->cell(get_name(name_t::SNAP_NAME_COOKIE_CONSENT_SILKTIDE_CONSENT_DURATION))->value().safeInt64Value());
        if(consent_duration > 0)
        {
            as2js::Int64 integer(consent_duration);
            field.reset(new as2js::JSON::JSONValue(pos, integer));
            body->set_member("expiryDays", field);
        }
    }

    {
        temp_str = save_info.revision_row()->cell(get_name(name_t::SNAP_NAME_COOKIE_CONSENT_SILKTIDE_THEME))->value().stringValue().toUtf8().data();
        if(!temp_str.empty())
        {
            field.reset(new as2js::JSON::JSONValue(pos, temp_str));
            body->set_member("theme", field);
        }
    }

    content::path_info_t js_file;
    js_file.set_path(get_name(name_t::SNAP_NAME_COOKIE_CONSENT_SILKTIDE_JAVASCRIPT_OPTIONS));
    int64_t const version(save_info.revision_row()->cell(get_name(name_t::SNAP_NAME_COOKIE_CONSENT_SILKTIDE_JAVASCRIPT_VERSION))->value().safeInt64Value() + 1);
    save_info.revision_row()->cell(get_name(name_t::SNAP_NAME_COOKIE_CONSENT_SILKTIDE_JAVASCRIPT_VERSION))->setValue(version);

    QDateTime date;
    date.setMSecsSinceEpoch(f_snap->get_start_date() / 1000); // us to ms

    QString const js_options(QString(
                "/*!\n"
                " * Name: cookie-consent-silktide-options\n"
                " * Version: 1.%1\n"
                " * Browsers: all\n"
                " * Depends: %2\n"
                " * Description: Silktide Cookie Consent User Defined Options\n"
                " * License: Public Domain\n"
                " * Date: %3\n"
                " */\n"
                "window.cookieconsent_options=%4"
            )
                .arg(version)
                .arg(get_name(name_t::SNAP_NAME_COOKIE_CONSENT_SILKTIDE_JAVASCRIPT_OPTIONS_DEPENDENCY))
                .arg(date.toString("yyyy/MM/dd hh:mm:ss"))
                .arg(body->to_string().to_utf8().c_str())
        );

    // We could have a copy in the revision table,
    // but I don't think that's useful
    //save_info.revision_row()->cell(get_name())->setValue(js_options);

    {
        content::attachment_file file(f_snap);

        // attachment specific fields
        file.set_multiple(false);
        file.set_parent_cpath(get_name(name_t::SNAP_NAME_COOKIE_CONSENT_SILKTIDE_JAVASCRIPT_OPTIONS_PARENT_PATH));
        file.set_field_name(get_name(name_t::SNAP_NAME_COOKIE_CONSENT_SILKTIDE_JAVASCRIPT_FILENAME));
        file.set_attachment_owner(attachment::attachment::instance()->get_plugin_name());
        file.set_attachment_type(get_name(name_t::SNAP_NAME_COOKIE_CONSENT_SILKTIDE_JAVASCRIPT_TYPE));
        file.set_creation_time(f_snap->get_start_date());
        file.set_update_time(f_snap->get_start_date());
        content::dependency_list_t js_dependencies;
        js_dependencies << get_name(name_t::SNAP_NAME_COOKIE_CONSENT_SILKTIDE_JAVASCRIPT_OPTIONS_DEPENDENCY);
        file.set_dependencies(js_dependencies);

        // post file fields
        file.set_file_name(get_name(name_t::SNAP_NAME_COOKIE_CONSENT_SILKTIDE_JAVASCRIPT_FILENAME));
        file.set_file_filename(QString("%1.js").arg(get_name(name_t::SNAP_NAME_COOKIE_CONSENT_SILKTIDE_JAVASCRIPT_FILENAME)));
        file.set_file_data(js_options.toUtf8());
        file.set_file_original_mime_type("text/javascript");
        file.set_file_mime_type("text/javascript");
        file.set_file_creation_time(f_snap->get_start_date());
        file.set_file_modification_time(f_snap->get_start_date());
        file.set_file_index(1); // there is only one such file

        // ready, create the attachment
        content::content::instance()->create_attachment(file, snap_version::SPECIAL_VERSION_SYSTEM_BRANCH, "");

        // here the data buffer gets freed!
    }
}



SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
