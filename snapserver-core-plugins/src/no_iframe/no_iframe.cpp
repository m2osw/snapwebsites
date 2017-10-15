// Snap Websites Server -- manager the No IFRAME JavaScript
// Copyright (C) 2017  Made to Order Software Corp.
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

#include "no_iframe.h"

#include "../output/output.h"
#include "../permissions/permissions.h"
//#include "../sendmail/sendmail.h"
//#include "../users/users.h"

#include <snapwebsites/log.h>
#include <snapwebsites/not_reached.h>
#include <snapwebsites/not_used.h>
#include <snapwebsites/qdomhelpers.h>

#include <snapwebsites/poison.h>

SNAP_PLUGIN_START(no_iframe, 1, 0)


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
    case name_t::SNAP_NAME_NO_IFRAME_ALLOW_PATH:
        return "types/taxonomy/system/no-iframe/allow";

    case name_t::SNAP_NAME_NO_IFRAME_DISALLOW_PATH:
        return "types/taxonomy/system/no-iframe/disallow";

    case name_t::SNAP_NAME_NO_IFRAME_MODE:
        return "no_iframe::mode";

    case name_t::SNAP_NAME_NO_IFRAME_MODE_PATH:
        return "admin/settings/no-iframe";

    case name_t::SNAP_NAME_NO_IFRAME_PAGE_MODE:
        return "no_iframe::page_mode";

    default:
        // invalid index
        throw snap_logic_exception("invalid name_t::SNAP_NAME_NO_IFRAME_...");

    }
    NOTREACHED();
}


/** \brief Initialize the no_iframe plugin.
 *
 * This function is used to initialize the no_iframe plugin object.
 */
no_iframe::no_iframe()
    //: f_snap(nullptr) -- auto-init
{
}

/** \brief Clean up the no_iframe plugin.
 *
 * Ensure the no_iframe object is clean before it is gone.
 */
no_iframe::~no_iframe()
{
}

/** \brief Get a pointer to the no_iframe plugin.
 *
 * This function returns an instance pointer to the no_iframe plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the no_iframe plugin.
 */
no_iframe * no_iframe::instance()
{
    return g_plugin_no_iframe_factory.instance();
}


/** \brief Send users to the no_iframe settings.
 *
 * This path represents the no_iframe settings.
 */
QString no_iframe::settings_path() const
{
    return "/admin/settings/no-iframe";
}


/** \brief A path or URI to a logo for this plugin.
 *
 * This function returns a 64x64 icons representing this plugin.
 *
 * \return A path to the logo.
 */
QString no_iframe::icon() const
{
    return "/images/no-iframe/no-iframe-logo-64x64.png";
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
QString no_iframe::description() const
{
    return "The no_iframe plugin gives administrators a way to prevent a"
          " website from being shown in another website iframe tag.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString no_iframe::dependencies() const
{
    return "|editor|output|";
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
int64_t no_iframe::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2017, 2, 1, 18, 26, 49, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Update the database with our no_iframe references.
 *
 * Send our no_iframe to the database so the system can find us when a
 * user references our pages.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added to the database by this update (in micro-seconds).
 */
void no_iframe::content_update(int64_t variables_timestamp)
{
    NOTUSED(variables_timestamp);

    content::content::instance()->add_xml(get_plugin_name());
}


/** \brief Initialize the no_iframe.
 *
 * This function terminates the initialization of the no_iframe plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void no_iframe::bootstrap(snap_child * snap)
{
    f_snap = snap;

    SNAP_LISTEN(no_iframe, "layout", layout::layout, generate_header_content, _1, _2, _3);
}


/** \brief Check whether this main page should remove itself from an IFrame.
 *
 * This function is the one that determines whether the page should check
 * whether it gets opened in an IFrame and if so, redirect the user browser
 * to show this page as a main page.
 *
 * The final test happens in JavaScript since we cannot know where we are
 * until then. This function includes the No IFrame JavaScript to the
 * final document if necessary for this page.
 *
 * \param[in] ipath  The path of the page being checked.
 * \param[in,out] header  The document header we are working on.
 * \param[in,out] metadata  The metadata we are working on.
 */
void no_iframe::on_generate_header_content(content::path_info_t & ipath, QDomElement & header, QDomElement & metadata)
{
    NOTUSED(metadata);

    snap_string_list const & segments(ipath.get_segments());
    if(segments.size() > 0
    && segments[0] == "admin")
    {
        // no need under /admin
        //
        return;
    }

    content::content * content_plugin(content::content::instance());
    QtCassandra::QCassandraTable::pointer_t content_table(content_plugin->get_content_table());
    QtCassandra::QCassandraTable::pointer_t revision_table(content_plugin->get_revision_table());

    content::path_info_t mode_ipath;
    mode_ipath.set_path(get_name(name_t::SNAP_NAME_NO_IFRAME_MODE_PATH));
    if(!content_table->exists(mode_ipath.get_key()))
    {
        // the content.xml was not yet installed?
        //
        return;
    }
    if(!revision_table->exists(mode_ipath.get_revision_key()))
    {
        // if the content.xml exists, then the revision should also exist?!
        //
        return;
    }

    QtCassandra::QCassandraRow::pointer_t mode_row(revision_table->row(mode_ipath.get_revision_key()));

    QString mode("always");
    if(mode_row->exists(get_name(name_t::SNAP_NAME_NO_IFRAME_MODE)))
    {
        mode = mode_row->cell(get_name(name_t::SNAP_NAME_NO_IFRAME_MODE))->value().stringValue();
    }

    if(mode == "never")
    {
        // never remove an IFrame, ignore completely
        //
        return;
    }

    if(mode == "select-pages")
    {
        // the page must be linked to the "allow" to be considered for no-iframe
        //
        links::link_info info(get_name(name_t::SNAP_NAME_NO_IFRAME_PAGE_MODE), true, ipath.get_key(), ipath.get_branch());
        QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(info));
        links::link_info child_info;
        if(!link_ctxt->next_link(child_info))
        {
            // not linked in regard to the No IFrame, ignore
            //
            return;
        }

        // check where we are linked to
        //
        if(!child_info.key().endsWith(get_name(name_t::SNAP_NAME_NO_IFRAME_ALLOW_PATH)))
        {
            // not the "No IFrame Allowed" link, so ignore
            //
            return;
        }
    }
    else if(mode == "except-select-pages")
    {
        // the page must NOT be linked to the "disallow" to be considered for no-iframe
        //
        links::link_info info(get_name(name_t::SNAP_NAME_NO_IFRAME_PAGE_MODE), true, ipath.get_key(), ipath.get_branch());
        QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(info));
        links::link_info child_info;
        if(!link_ctxt->next_link(child_info))
        {
            // not linked in regard to the No IFrame, ignore
            //
            return;
        }

        // check where we are linked to
        //
        if(child_info.key().endsWith(get_name(name_t::SNAP_NAME_NO_IFRAME_DISALLOW_PATH)))
        {
            // linked to "No IFrame Disallowed" link, so ignore
            //
            return;
        }
    }
    else if(mode != "always")
    {
        // unknown mode?!
        //
        SNAP_LOG_DEBUG("No IFrame mode \"")(mode)("\" no known. Pretend that page should not be taken out of an IFrame.");
        return;
    }

    QDomDocument doc(header.ownerDocument());
    content::content::instance()->add_javascript(doc, "no-iframe");
}


SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
