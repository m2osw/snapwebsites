// Snap Websites Server -- search capability
// Copyright (C) 2012-2017  Made to Order Software Corp.
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

#include "search.h"

#include <snapwebsites/not_reached.h>
#include <snapwebsites/not_used.h>
#include <snapwebsites/qdomhelpers.h>

#include <snapwebsites/poison.h>


SNAP_PLUGIN_START(search, 1, 0)

/** \brief Get a fixed path name.
 *
 * The path plugin makes use of different names in the database. This
 * function ensures that you get the right spelling for a given name.
 *
 * \param[in] name  The name to retrieve.
 *
 * \return A pointer to the name.
 */
const char * get_name(name_t name)
{
    switch(name) {
    case name_t::SNAP_NAME_SEARCH_STATUS:
        return "search::status";

    default:
        // invalid index
        throw snap_logic_exception("invalid name_t::SNAP_NAME_SEARCH_...");

    }
    NOTREACHED();
}


/** \brief Initialize the search plugin.
 *
 * This function initializes the search plugin.
 */
search::search()
    //: f_snap(NULL) -- auto-init
{
}


/** \brief Destroy the search plugin.
 *
 * This function cleans up the search plugin.
 */
search::~search()
{
}


/** \brief Get a pointer to the search plugin.
 *
 * This function returns an instance pointer to the search plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the search plugin.
 */
search * search::instance()
{
    return g_plugin_search_factory.instance();
}


/** \brief Send users to the plugin settings.
 *
 * This path represents this plugin settings.
 */
QString search::settings_path() const
{
    return "/admin/settings/search";
}


/** \brief A path or URI to a logo for this plugin.
 *
 * This function returns a 64x64 icons representing this plugin.
 *
 * \return A path to the logo.
 */
QString search::icon() const
{
    return "/images/search/search-logo-64x64.png";
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
QString search::description() const
{
    return "The search plugin index your website public pages in order to"
          " allow your users to search its content.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString search::dependencies() const
{
    return "|layout|";
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
int64_t search::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2015, 12, 20, 18, 1, 54, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Update the database with our search references.
 *
 * Do nothing at this point.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added to the database by this update (in micro-seconds).
 */
void search::content_update(int64_t variables_timestamp)
{
    NOTUSED(variables_timestamp);

    content::content::instance()->add_xml("search");
}


/** \brief Bootstrap the search.
 *
 * This function adds the events the search plugin is listening for.
 *
 * \param[in] snap  The child handling this request.
 */
void search::bootstrap(snap_child * snap)
{
    f_snap = snap;

    SNAP_LISTEN(search, "server", server, improve_signature, _1, _2, _3);
    SNAP_LISTEN(search, "layout", layout::layout, generate_page_content, _1, _2, _3);
}


/** \brief Improves the error signature.
 *
 * This function adds the search page to the brief signature of die()
 * errors.
 *
 * \param[in] path  The path to the page that generated the error.
 * \param[in] doc  The DOM document.
 * \param[in,out] signature_tag  The DOM element where signature anchors are added.
 */
void search::on_improve_signature(QString const & path, QDomDocument doc, QDomElement signature_tag)
{
    // use only the last section of the path (basename) because in most
    // cases the path is not as relevant (if relevant at all) to the search
    //
    QString query(path);
    int pos(query.lastIndexOf('/'));
    if(pos == query.length() - 1)
    {
        query.remove(pos, 1);
        pos = query.lastIndexOf('/');
    }
    if(pos >= 0)
    {
        query.remove(0, pos + 1);
    }
    query = query.simplified();

    // the query should never be empty since the home page should always work...
    //
    if(!query.isEmpty())
    {
        // add a space between the previous link and this one
        snap_dom::append_plain_text_to_node(signature_tag, " ");

        // add a link to the user account
        QDomElement a_tag(doc.createElement("a"));
        a_tag.setAttribute("class", "search");
        //a_tag.setAttribute("target", "_top"); -- I do not think _top makes sense in this case
        // TODO: we may want the save the language and not force a /search like this...
        a_tag.setAttribute("href", QString("/search?search=%1").arg(snap_uri::urlencode(query, "~")));
        // TODO: translate
        snap_dom::append_plain_text_to_node(a_tag, "Search Our Website");

        signature_tag.appendChild(a_tag);
    }
}


/** \brief Generate a link to the search page.
 *
 * This function generates a link to the search page so users with advanced
 * browsers such as Mozilla can go to our search page without having to search
 * for it (ha! ha!)
 *
 * \param[in,out] ipath  The path being managed.
 * \param[in,out] page  The page being generated.
 * \param[in,out] body  The body being generated.
 */
void search::on_generate_page_content(content::path_info_t & ipath, QDomElement & page, QDomElement & body)
{
    NOTUSED(ipath);

    QDomDocument doc(page.ownerDocument());

    QDomElement bookmarks;
    snap_dom::get_tag("bookmarks", body, bookmarks);

    QDomElement link(doc.createElement("link"));
    link.setAttribute("rel", "search");
    link.setAttribute("title", "Search"); // TODO: translate
    link.setAttribute("type", "text/html");
    link.setAttribute("href", f_snap->get_site_key_with_slash() + "search");
    bookmarks.appendChild(link);
}


SNAP_PLUGIN_END()
// vim: ts=4 sw=4 et
