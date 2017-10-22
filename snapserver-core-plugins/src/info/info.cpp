// Snap Websites Server -- info plugin to control the core settings
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

#include "info.h"

#include "../output/output.h"
#include "../permissions/permissions.h"
#include "../sendmail/sendmail.h"
#include "../users/users.h"

#include <snapwebsites/log.h>
#include <snapwebsites/not_reached.h>
#include <snapwebsites/not_used.h>
#include <snapwebsites/qdomhelpers.h>

#include <snapwebsites/poison.h>

SNAP_PLUGIN_START(info, 1, 0)


/** \class info
 * \brief Support for the basic core information.
 *
 * The core information, such are your website name, are managed by the
 * plugin.
 *
 * It is a separate plugin because the content plugin (Which would probably
 * make more sense) is a dependency of the form plugin and the information
 * requires special handling which mean the content plugin would have to
 * include the form plugin (which is not possible since the form plugin
 * includes the content plugin.)
 */


/** \brief Get a fixed info name.
 *
 * The info plugin makes use of different names in the database. This
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
    case name_t::SNAP_NAME_INFO_PLUGIN_SELECTION:
        return "admin/plugins";

    default:
        // invalid index
        throw snap_logic_exception("invalid name_t::SNAP_NAME_INFO_...");

    }
    NOTREACHED();
}


/** \brief Initialize the info plugin.
 *
 * This function is used to initialize the info plugin object.
 */
info::info()
    //: f_snap(nullptr) -- auto-init
{
}

/** \brief Clean up the info plugin.
 *
 * Ensure the info object is clean before it is gone.
 */
info::~info()
{
}

/** \brief Get a pointer to the info plugin.
 *
 * This function returns an instance pointer to the info plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the info plugin.
 */
info * info::instance()
{
    return g_plugin_info_factory.instance();
}


/** \brief Send users to the info settings.
 *
 * This path represents the info settings.
 */
QString info::settings_path() const
{
    return "/admin/settings/info";
}


/** \brief A path or URI to a logo for this plugin.
 *
 * This function returns a 64x64 icons representing this plugin.
 *
 * \return A path to the logo.
 */
QString info::icon() const
{
    return "/images/info/info-logo-64x64.png";
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
QString info::description() const
{
    return "The info plugin offers handling of the core information of your"
           "system. It is opens a settings page where all that information"
           "can directly be edited online.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString info::dependencies() const
{
    return "|editor|messages|output|path|permissions|sendmail|users|";
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
int64_t info::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2016, 4, 7, 1, 45, 41, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Update the database with our info references.
 *
 * Send our info to the database so the system can find us when a
 * user references our pages.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added to the database by this update (in micro-seconds).
 */
void info::content_update(int64_t variables_timestamp)
{
    NOTUSED(variables_timestamp);

    content::content::instance()->add_xml(get_plugin_name());
}


/** \brief Initialize the info.
 *
 * This function terminates the initialization of the info plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void info::bootstrap(snap_child * snap)
{
    f_snap = snap;

    SNAP_LISTEN(info, "server", server, improve_signature, _1, _2, _3);
    SNAP_LISTEN(info, "path", path::path, can_handle_dynamic_path, _1, _2);
    SNAP_LISTEN(info, "layout", layout::layout, generate_page_content, _1, _2, _3);
    SNAP_LISTEN(info, "editor", editor::editor, finish_editor_form_processing, _1, _2);
    SNAP_LISTEN(info, "editor", editor::editor, init_editor_widget, _1, _2, _3, _4, _5);

    SNAP_TEST_PLUGIN_SUITE_LISTEN(info);
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
bool info::on_path_execute(content::path_info_t & ipath)
{
    // first check whether the unsubscribe implementation understands this path
    if(unsubscribe_on_path_execute(ipath))
    {
        return true;
    }

    // then check whether the plugin selection wants to deal with this hit
    if(plugin_selection_on_path_execute(ipath))
    {
        return true;
    }

    f_snap->output(layout::layout::instance()->apply_layout(ipath, this));

    return true;
}


/** \brief Generate a link to the administration page.
 *
 * This function generates a link to the main administration page
 * (i.e. /admin) so users with advanced browsers such as SeaMonkey
 * can go to their administration page without having to search
 * for it.
 *
 * \param[in,out] ipath  The path being managed.
 * \param[in,out] page  The page being generated.
 * \param[in,out] body  The body being generated.
 */
void info::on_generate_page_content(content::path_info_t & ipath, QDomElement & page, QDomElement & body)
{
    NOTUSED(ipath);

    // only check if user is logged in
    // (if user is not administratively logged in at the moment, try to
    // go to the administration page will require a relogin which is fine)
    //
    // XXX: we may want to show the bookmarks to returning users?
    //
    if(users::users::instance()->user_is_logged_in())
    {
        // only show the /admin link if the user can go there
        permissions::permissions * permissions_plugin(permissions::permissions::instance());
        QString const & login_status(permissions_plugin->get_login_status());
        content::path_info_t page_ipath;
        page_ipath.set_path("/admin");
        content::permission_flag allowed;
        path::path::instance()->access_allowed(permissions_plugin->get_user_path(), page_ipath, "administer", login_status, allowed);
        if(allowed.allowed())
        {
            QDomDocument doc(page.ownerDocument());

            QDomElement bookmarks;
            snap_dom::get_tag("bookmarks", body, bookmarks);

            QDomElement link(doc.createElement("link"));
            link.setAttribute("rel", "bookmark");
            link.setAttribute("title", "Administer Site"); // TODO: translate
            link.setAttribute("type", "text/html");
            link.setAttribute("href", f_snap->get_site_key_with_slash() + "admin");
            bookmarks.appendChild(link);
        }
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
void info::on_generate_main_content(content::path_info_t & ipath, QDomElement & page, QDomElement & body)
{
    // our settings pages are like any standard pages
    output::output::instance()->on_generate_main_content(ipath, page, body);
}


void info::on_finish_editor_form_processing(content::path_info_t & ipath, bool & succeeded)
{
    if(!succeeded)
    {
        return;
    }

    if(ipath.get_cpath() != "admin/settings/info")
    {
        unsubscribe_on_finish_editor_form_processing(ipath);

        return;
    }

    content::content * content_plugin(content::content::instance());
    libdbproxy::table::pointer_t revision_table(content_plugin->get_revision_table());
    libdbproxy::row::pointer_t settings_row(revision_table->getRow(ipath.get_revision_key()));

    libdbproxy::value value;

    value = settings_row->getCell(snap::get_name(snap::name_t::SNAP_NAME_CORE_SITE_NAME))->getValue();
    f_snap->set_site_parameter(snap::get_name(snap::name_t::SNAP_NAME_CORE_SITE_NAME), value);

    value = settings_row->getCell(snap::get_name(snap::name_t::SNAP_NAME_CORE_SITE_LONG_NAME))->getValue();
    f_snap->set_site_parameter(snap::get_name(snap::name_t::SNAP_NAME_CORE_SITE_LONG_NAME), value);

    value = settings_row->getCell(snap::get_name(snap::name_t::SNAP_NAME_CORE_SITE_SHORT_NAME))->getValue();
    f_snap->set_site_parameter(snap::get_name(snap::name_t::SNAP_NAME_CORE_SITE_SHORT_NAME), value);

    value = settings_row->getCell(snap::get_name(snap::name_t::SNAP_NAME_CORE_ADMINISTRATOR_EMAIL))->getValue();
    f_snap->set_site_parameter(snap::get_name(snap::name_t::SNAP_NAME_CORE_ADMINISTRATOR_EMAIL), value);
}


/** \brief Improves the error signature.
 *
 * This function adds a link to the administration page to the signature of
 * die() errors. This is done only if the user is logged in and has enough
 * rights to access administrative pages.
 *
 * \param[in] path  The path on which the error occurs.
 * \param[in] doc  The DOMDocument object.
 * \param[in,out] signature_tag  The signature tag to improve.
 */
void info::on_improve_signature(QString const & path, QDomDocument doc, QDomElement signature_tag)
{
    NOTUSED(path);

    // only check if user is logged in
    // (if user is not administratively logged in at the moment, try to
    // go to the administration page will require a relogin which is fine)
    //
    // XXX: we may want to show the Administration link to returning users?
    //      (i.e. just !f_user_key.isEmpty() instead of user_is_logged_in())
    //
    if(users::users::instance()->user_is_logged_in())
    {
        // only show the /admin link if the user can go there
        permissions::permissions * permissions_plugin(permissions::permissions::instance());
        QString const & login_status(permissions_plugin->get_login_status());
        content::path_info_t page_ipath;
        page_ipath.set_path("/admin");
        content::permission_flag allowed;
        path::path::instance()->access_allowed(permissions_plugin->get_user_path(), page_ipath, "administer", login_status, allowed);
        if(allowed.allowed())
        {
            // add a space between the previous link and this one
            snap_dom::append_plain_text_to_node(signature_tag, " ");

            // add a link to the user account
            QDomElement a_tag(doc.createElement("a"));
            a_tag.setAttribute("class", "administration");
            a_tag.setAttribute("target", "_top");
            a_tag.setAttribute("href", "/admin");
            // TODO: translate
            snap_dom::append_plain_text_to_node(a_tag, "Administration");

            signature_tag.appendChild(a_tag);
        }
    }
}


void info::on_can_handle_dynamic_path(content::path_info_t & ipath, path::dynamic_plugin_t & plugin_info)
{
    QString const cpath(ipath.get_cpath());
    if(cpath.startsWith(QString("%1/").arg(sendmail::get_name(sendmail::name_t::SNAP_NAME_SENDMAIL_UNSUBSCRIBE_PATH)))
    || cpath.startsWith(QString("%1/install/").arg(get_name(name_t::SNAP_NAME_INFO_PLUGIN_SELECTION)))
    || cpath.startsWith(QString("%1/remove/").arg(get_name(name_t::SNAP_NAME_INFO_PLUGIN_SELECTION))))
    {
        // tell the path plugin that this is ours
        plugin_info.set_plugin(this);
        return;
    }
}


void info::on_init_editor_widget(content::path_info_t & ipath, QString const & field_id, QString const & field_type, QDomElement & widget, libdbproxy::row::pointer_t row)
{
    NOTUSED(field_type);
    NOTUSED(row);

    QString const cpath(ipath.get_cpath());
    if(cpath == "unsubscribe")
    {
        init_unsubscribe_editor_widgets(ipath, field_id, widget);
    }
    else if(cpath == "admin/plugins")
    {
        init_plugin_selection_editor_widgets(ipath, field_id, widget);
    }
}



SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
