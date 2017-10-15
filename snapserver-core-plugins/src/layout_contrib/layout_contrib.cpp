// Snap Websites Server -- handle the contrib files for your layouts
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

// self
//
#include "layout_contrib.h"

// libsnapwebsites library
//
//#include <snapwebsites/log.h>
//#include <snapwebsites/not_reached.h>
#include <snapwebsites/not_used.h>

// always last
//
#include <snapwebsites/poison.h>


SNAP_PLUGIN_START(layout_contrib, 1, 0)


/** \brief Get a fixed layout name.
 *
 * The layout plugin makes use of different names in the database. This
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
    case name_t::SNAP_NAME_LAYOUT_CONTRIB_BOOTSTRAP:
        return "bootstrap";

    default:
        // invalid index
        throw snap_logic_exception("invalid name_t::SNAP_NAME_LAYOUT_CONTRIB_...");

    }
    NOTREACHED();
}


/** \brief Initialize the layout_contrib plugin.
 *
 * This function is used to initialize the layout_contrib plugin object.
 */
layout_contrib::layout_contrib()
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Clean up the layout_contrib plugin.
 *
 * Ensure the layout_contrib object is clean before it is gone.
 */
layout_contrib::~layout_contrib()
{
}


/** \brief Initialize the layout_contrib.
 *
 * This function terminates the initialization of the layout_contrib plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void layout_contrib::bootstrap(snap_child * snap)
{
    f_snap = snap;
}


/** \brief Get a pointer to the layout_contrib plugin.
 *
 * This function returns an instance pointer to the layout_contrib plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the layout_contrib plugin.
 */
layout_contrib * layout_contrib::instance()
{
    return g_plugin_layout_contrib_factory.instance();
}


/** \brief A path or URI to a logo for this plugin.
 *
 * This function returns a 64x64 icons representing this plugin.
 *
 * \return A path to the logo.
 */
QString layout_contrib::icon() const
{
    return "/images/snap/layout_contrib-logo-64x64.png";
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
QString layout_contrib::description() const
{
    return "Offer additional files (JS, CSS, Fonts) for layouts.";
}


/** \brief Return our dependencies
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString layout_contrib::dependencies() const
{
    return "|content|links|output|path|";
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
 * \return The UTC Unix date of the last update of this plugin or a layout.
 */
int64_t layout_contrib::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    // first time, make sure the default theme is installed
    //
    SNAP_PLUGIN_UPDATE(2017, 5, 20, 0, 14, 30, content_update);

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
void layout_contrib::content_update(int64_t variables_timestamp)
{
    NOTUSED(variables_timestamp);

    content::content::instance()->add_xml(get_plugin_name());
}


SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
