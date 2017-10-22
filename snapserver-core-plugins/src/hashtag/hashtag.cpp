// Snap Websites Server -- hashtag implementation
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

#include "hashtag.h"

#include <snapwebsites/not_used.h>
#include <snapwebsites/qdomhelpers.h>

#include <snapwebsites/poison.h>


SNAP_PLUGIN_START(hashtag, 1, 0)


/** \brief Get a fixed hashtag name.
 *
 * The hashtag plugin makes use of different names in the database. This
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
    case name_t::SNAP_NAME_HASHTAG_LINK:
        return "hashtag::link";

    case name_t::SNAP_NAME_HASHTAG_PATH:
        return "types/taxonomy/system/content-types/hashtag";

    case name_t::SNAP_NAME_HASHTAG_SETTINGS_PATH:
        return "admin/settings/hashtag";

    default:
        // invalid index
        throw snap_logic_exception("invalid name_t::SNAP_NAME_HASHTAG_...");

    }
    NOTREACHED();
}


/** \brief Initialize the hashtag plugin.
 *
 * This function is used to initialize the hashtag plugin object.
 */
hashtag::hashtag()
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Clean up the hashtag plugin.
 *
 * Ensure the hashtag object is clean before it is gone.
 */
hashtag::~hashtag()
{
}


/** \brief Get a pointer to the hashtag plugin.
 *
 * This function returns an instance pointer to the hashtag plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the hashtag plugin.
 */
hashtag * hashtag::instance()
{
    return g_plugin_hashtag_factory.instance();
}


/** \brief Send users to the plugin settings.
 *
 * This path represents this plugin settings.
 */
QString hashtag::settings_path() const
{
    return "/admin/settings/hashtag";
}


/** \brief A path or URI to a logo for this plugin.
 *
 * This function returns a 64x64 icons representing this plugin.
 *
 * \return A path to the logo.
 */
QString hashtag::icon() const
{
    return "/images/hashtag/hashtag-logo-64x64.png";
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
QString hashtag::description() const
{
    return "Plugin used to transform #hashtag entries into tags and links."
          " Because all the pages linked to a particular hashtags appear"
          " in the same list, in effect, you get all the pages grouped as"
          " with Twitter and other similar systems.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString hashtag::dependencies() const
{
    return "|filter|messages|output|users|";
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
int64_t hashtag::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2015, 12, 21, 0, 2, 42, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Update the database with our content references.
 *
 * Send our content to the database so the system can find us when a
 * user references our pages.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added
 *                                 to the database by this update
 *                                 (in micro-seconds).
 */
void hashtag::content_update(int64_t variables_timestamp)
{
    NOTUSED(variables_timestamp);

    content::content::instance()->add_xml(get_plugin_name());
}


/** \brief Initialize the hashtag.
 *
 * This function terminates the initialization of the hashtag plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void hashtag::bootstrap(snap_child * snap)
{
    f_snap = snap;

    SNAP_LISTEN(hashtag, "filter", filter::filter, filter_text, _1);
}


/** \brief Check the text data in the XML and replace hashtags.
 *
 * This function searches the text nodes and replace the hashtags it
 * finds in these with a link to a hashtag (a taxonomy tag.) This
 * allows one to find all the pages that were tagged one way or
 * the other.
 *
 * Hashtags are just the same as on Facebook and Twitter. For
 * example, "#plugin" would point to the tag named plugin.
 *
 * The system can be setup to hide the hash character (#), which
 * is useful if you want to create something such as a glossary.
 *
 * \param[in,out] txt_filt  The text filter parameters.
 */
void hashtag::on_filter_text(filter::filter::filter_text_t & txt_filt)
{
    // initialized only if needed
    content::content * content_plugin(nullptr);
    libdbproxy::table::pointer_t content_table;
    libdbproxy::table::pointer_t revision_table;
    QString link_settings;

    bool first(true);
    bool added_css(false);
    bool changed(false);

    // TODO: we still need to implement the functions to get each text
    //       separately instead of one large block that can include
    //       tags (because we already replaced some tokens, etc.)
    //
    QString result(txt_filt.get_text());
    int const max(result.length());
    for(int pos(result.indexOf('#')); pos != -1 && pos + 1 < max; pos = result.indexOf('#', pos + 1))
    {
        QChar c(result.at(pos + 1));
        if(c.isLetterOrNumber())
        {
            // found a #word
            int const start(pos); // start includes the '#'
            for(pos += 2; pos < max && result.at(pos).isLetterOrNumber(); ++pos);
            // now pos == max or represents a space
            // extract the hash word without the '#'
            //
            // TODO: the hash cannot include Unicode characters as is (I think)
            //       we would need to convert them to %XX codes
            QString const hash(result.mid(start + 1, pos - start - 1));
            content::path_info_t hash_ipath;
            hash_ipath.set_path(QString("%1/%2").arg(get_name(name_t::SNAP_NAME_HASHTAG_PATH)).arg(hash));
            if(first)
            {
                first = false;

                content_plugin = content::content::instance();
                content_table = content_plugin->get_content_table();
                revision_table = content_plugin->get_revision_table();

                content::path_info_t settings_ipath;
                settings_ipath.set_path(get_name(name_t::SNAP_NAME_HASHTAG_SETTINGS_PATH));
                link_settings = revision_table->getRow(settings_ipath.get_revision_key())->getCell(get_name(name_t::SNAP_NAME_HASHTAG_LINK))->getValue().stringValue();
            }
            if(content_table->exists(hash_ipath.get_key())
            && content_table->getRow(hash_ipath.get_key())->exists(content::get_name(content::name_t::SNAP_NAME_CONTENT_CREATED)))
            {
                // the tag exists, add the link
                QString const title(revision_table->getRow(hash_ipath.get_revision_key())->getCell(content::get_name(content::name_t::SNAP_NAME_CONTENT_TITLE))->getValue().stringValue());
                QString a;
                if(link_settings == "bottom")
                {
                    // we could have a "ref" (link to the bottom of the article)
                    // or even another link_settings choice
                    a = hash;
                }
                else
                {
                    a = QString("<a href=\"/%1\" title=\"%2\" class=\"hashtag-link hashtag-%3\">%4<b>%5</b></a>")
                            .arg(hash_ipath.get_cpath())
                            .arg(snap_dom::remove_tags(title)) // titles are HTML code
                            .arg(link_settings) // class "hashtag-hashtag", "hashtag-standard", "hashtag-invisible"
                            .arg(link_settings == "hashtag" ? "<s>#</s>" : "")
                            .arg(hash);

                    // add the CSS only if necessary!
                    if(!added_css)
                    {
                        added_css = true;
                        content_plugin->add_css(txt_filt.get_xml_document(), "hashtag");
                    }
                }
                result.replace(start, pos - start, a);
                changed = true;
            }
        }
    }

    if(changed)
    {
        txt_filt.set_text(result);
    }
}




//
// TODO:
// We actually want to take care of all those hashtags in the background
// for 2 reasons: (1) the body of the page will be generated possibly going
// through many filters [think of the list and how "slow" it is...] and
// (2) that way we do not waste time for the editor--although the editor,
// as a drawback, does not immediately see the results of his changes.
//
// This represents a major change to the editor functionality that I do not
// want to implement quite yet... i.e. editor saves body in editor::body
// field, when you re-edit a page, you use editor::body; when the editor
// processes a new editor::body through any number of filters, then it
// saves the result in the regular content::body. This will allow very
// heavy processes (especially if someone installs the hashtag plugin on
// an existing system with, say, 10,000 pages, it will generate quite
// a bit of a slow down since all those 10,000 pages will need to be 100%
// reprocessed.
//





SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
