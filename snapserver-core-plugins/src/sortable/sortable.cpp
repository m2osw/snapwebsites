// Snap Websites Server -- add a sortable widget so one can sort items in a list
// Copyright (C) 2016-2017  Made to Order Software Corp.
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

#include "sortable.h"

#include "../output/output.h"
#include "../messages/messages.h"
#include "../permissions/permissions.h"

#include <snapwebsites/log.h>
#include <snapwebsites/not_reached.h>
#include <snapwebsites/not_used.h>

#include <algorithm>
#include <iostream>

#include <openssl/rand.h>

#include <QChar>

#include <snapwebsites/poison.h>


/** \file
 * \brief The sortable plugin adds a widget one can use to sort list items.
 *
 * The sortable plugin is the integration of the Sortable.js library as
 * as Snap! editor widget giving the end user the ability to sort a list
 * of items by dragging and dropping those items.
 */

SNAP_PLUGIN_START(sortable, 1, 0)



/* \brief Get a fixed sortable name.
 *
 * The sortable plugin makes use of different names in the database. This
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
    case name_t::SNAP_NAME_SORTABLE_CHECK_BLACKLIST:
        return "sortable::check_blacklist";

    default:
        // invalid index
        throw snap_logic_exception(QString("invalid name_t::SNAP_NAME_SORTABLE_... (%1)").arg(static_cast<int>(name)));

    }
    NOTREACHED();
}









/** \brief Initialize the sortable plugin.
 *
 * This function is used to initialize the sortable plugin object.
 */
sortable::sortable()
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Clean up the sortable plugin.
 *
 * Ensure the sortable object is clean before it is gone.
 */
sortable::~sortable()
{
}


/** \brief Get a pointer to the sortable plugin.
 *
 * This function returns an instance pointer to the sortable plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the sortable plugin.
 */
sortable * sortable::instance()
{
    return g_plugin_sortable_factory.instance();
}


/** \brief A path or URI to a logo for this plugin.
 *
 * This function returns a 64x64 icons representing this plugin.
 *
 * \return A path to the logo.
 */
QString sortable::icon() const
{
    return "/images/sortable/sortable-logo-64x64.png";
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
QString sortable::description() const
{
    return "Gives the end users the ability to sort list items."
          " This plugin is very rarely added by itself. Instead, another"
          " plugin that needs the sort capability will depend on it.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString sortable::dependencies() const
{
    return "|editor|messages|output|permissions|users|";
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
int64_t sortable::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2016, 1, 24, 0, 33, 4, content_update);

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
void sortable::content_update(int64_t variables_timestamp)
{
    NOTUSED(variables_timestamp);

    content::content::instance()->add_xml(get_plugin_name());
}


/** \brief Initialize the sortable.
 *
 * This function terminates the initialization of the sortable plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void sortable::bootstrap(snap_child * snap)
{
    f_snap = snap;

    SNAP_LISTEN(sortable, "editor", editor::editor, prepare_editor_form, _1);
}


/** \brief Add the locale widget to the editor XSLT.
 *
 * The editor is extended by the locale plugin by adding a time zone
 * and other various widgets.
 *
 * \param[in] e  A pointer to the editor plugin.
 */
void sortable::on_prepare_editor_form(editor::editor * e)
{
    e->add_editor_widget_templates_from_file(":/xsl/sortable_widgets/sortable-form.xsl");
}


SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
