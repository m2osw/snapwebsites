// Copyright (c) 2011-2019  Made to Order Software Corp.  All Rights Reserved
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
//
// Using 3rd party library from silktide:
// https://silktide.com/tools/cookie-consent/


// self
//
#include    "cookie_consent_silktide.h"


// other plugins
//
#include    "../attachment/attachment.h"


// snaplogger
//
#include    <snaplogger/message.h>


// snapdev
//
#include    <snapdev/not_reached.h>
#include    <snapdev/not_used.h>


// as2js
//
#include    <as2js/json.h>


// C++
//
#include    <iostream>


// last include
//
#include    <snapdev/poison.h>



namespace snap
{
namespace cookie_consent_silktide
{


SERVERPLUGINS_START(cookie_consent_silktide, 1, 0)
    , ::serverplugins::description(
            "Show an in-page popup allowing users to agree on use of cookies."
            " This plugin makes use the third party silktide cookie-consent tool.")
    , ::serverplugins::icon("/images/cookie-consent-silktide/cookie-consent-silktide-logo-64x64.png")
    , ::serverplugins::settings_path("/admin/settings/cookie-consent-silktide")
    , ::serverplugins::dependency("attachment")
    , ::serverplugins::dependency("editor")
    , ::serverplugins::dependency("layout")
    , ::serverplugins::help_uri("https://snapwebsites.org/help")
    , ::serverplugins::categorization_tag("security")
    , ::serverplugins::categorization_tag("spam")
SERVERPLUGINS_END(cookie_consent_silktide)


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
time_t cookie_consent_silktide::do_update(time_t last_updated, unsigned int phase)
{
    SERVERPLUGINS_PLUGIN_UPDATE_INIT();

    if(phase == 0)
    {
        SERVERPLUGINS_PLUGIN_UPDATE(2016, 3, 27, 15, 30, 34, content_update);
    }

    SERVERPLUGINS_PLUGIN_UPDATE_EXIT();
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
    snapdev::NOT_USED(variables_timestamp);

    content::content::instance()->add_xml(get_plugin_name());
}


/** \brief Initialize the locale.
 *
 * This function terminates the initialization of the locale plugin
 * by registering for different events.
 */
void cookie_consent_silktide::bootstrap()
{
    SERVERPLUGINS_LISTEN(cookie_consent_silktide, "layout", layout::layout, generate_header_content, boost::placeholders::_1, boost::placeholders::_2, boost::placeholders::_3);
    SERVERPLUGINS_LISTEN(cookie_consent_silktide, "editor", editor::editor, save_editor_fields, boost::placeholders::_1);
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
    snapdev::NOT_USED(ipath, metadata);

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
    libdbproxy::table::pointer_t content_table(content_plugin->get_content_table());
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

    SNAP_LOG_INFO
        << "saving silktide options to \""
        << get_name(name_t::SNAP_NAME_COOKIE_CONSENT_SILKTIDE_JAVASCRIPT_OPTIONS)
        << "\"."
        << SNAP_LOG_SEND;

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
        temp_str = save_info.revision_row()->getCell(get_name(name_t::SNAP_NAME_COOKIE_CONSENT_SILKTIDE_MESSAGE))->getValue().stringValue().toUtf8().data();
        if(!temp_str.empty())
        {
            field.reset(new as2js::JSON::JSONValue(pos, temp_str));
            body->set_member("message", field);
        }
    }

    {
        temp_str = save_info.revision_row()->getCell(get_name(name_t::SNAP_NAME_COOKIE_CONSENT_SILKTIDE_LEARN_MORE_LABEL))->getValue().stringValue().toUtf8().data();
        if(!temp_str.empty())
        {
            field.reset(new as2js::JSON::JSONValue(pos, temp_str));
            body->set_member("learnMore", field);
        }
    }

    {
        temp_str = save_info.revision_row()->getCell(get_name(name_t::SNAP_NAME_COOKIE_CONSENT_SILKTIDE_LEARN_MORE_URI))->getValue().stringValue().toUtf8().data();
        if(!temp_str.empty())
        {
            field.reset(new as2js::JSON::JSONValue(pos, temp_str));
            body->set_member("link", field);
        }
    }

    {
        temp_str = save_info.revision_row()->getCell(get_name(name_t::SNAP_NAME_COOKIE_CONSENT_SILKTIDE_DISMISS))->getValue().stringValue().toUtf8().data();
        if(!temp_str.empty())
        {
            field.reset(new as2js::JSON::JSONValue(pos, temp_str));
            body->set_member("dismiss", field);
        }
    }

    {
        temp_str = save_info.revision_row()->getCell(get_name(name_t::SNAP_NAME_COOKIE_CONSENT_SILKTIDE_DOMAIN))->getValue().stringValue().toUtf8().data();
        if(!temp_str.empty())
        {
            field.reset(new as2js::JSON::JSONValue(pos, temp_str));
            body->set_member("domain", field);
        }
    }

    {
        int64_t consent_duration(save_info.revision_row()->getCell(get_name(name_t::SNAP_NAME_COOKIE_CONSENT_SILKTIDE_CONSENT_DURATION))->getValue().safeInt64Value());
        if(consent_duration > 0)
        {
            as2js::Int64 integer(consent_duration);
            field.reset(new as2js::JSON::JSONValue(pos, integer));
            body->set_member("expiryDays", field);
        }
    }

    {
        temp_str = save_info.revision_row()->getCell(get_name(name_t::SNAP_NAME_COOKIE_CONSENT_SILKTIDE_THEME))->getValue().stringValue().toUtf8().data();
        if(!temp_str.empty())
        {
            field.reset(new as2js::JSON::JSONValue(pos, temp_str));
            body->set_member("theme", field);
        }
    }

    content::path_info_t js_file;
    js_file.set_path(get_name(name_t::SNAP_NAME_COOKIE_CONSENT_SILKTIDE_JAVASCRIPT_OPTIONS));
    int64_t const version(save_info.revision_row()->getCell(get_name(name_t::SNAP_NAME_COOKIE_CONSENT_SILKTIDE_JAVASCRIPT_VERSION))->getValue().safeInt64Value() + 1);
    save_info.revision_row()->getCell(get_name(name_t::SNAP_NAME_COOKIE_CONSENT_SILKTIDE_JAVASCRIPT_VERSION))->setValue(version);

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
    //save_info.revision_row()->getCell(get_name())->setValue(js_options);

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



} // namespace cookie_content_silktide
} // namespace snap
// vim: ts=4 sw=4 et
