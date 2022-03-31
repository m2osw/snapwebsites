// Snap Websites Server -- handle the contrib files for your layouts
// Copyright (c) 2017-2019  Made to Order Software Corp.  All Rights Reserved
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


// self
//
#include    "layout_contrib.h"


// snapdev
//
#include    <snapdev/not_used.h>


// last include
//
#include    <snapdev/poison.h>



namespace snap
{
namespace layout_contrib
{



CPPTHREAD_PLUGIN_START(layout_contrib, 1, 0)
    , ::cppthread::plugin_description(
            "Offer additional files (JS, CSS, Fonts) for layouts.")
    , ::cppthread::plugin_icon("/images/snap/layout_contrib-logo-64x64.png")
    , ::cppthread::plugin_settings()
    , ::cppthread::plugin_dependency("content")
    , ::cppthread::plugin_dependency("links")
    , ::cppthread::plugin_dependency("output")
    , ::cppthread::plugin_dependency("path")
    , ::cppthread::plugin_help_uri("https://snapwebsites.org/help")
    , ::cppthread::plugin_categorization_tag("security")
    , ::cppthread::plugin_categorization_tag("spam")
CPPTHREAD_PLUGIN_END()


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
        throw snap_logic_error("invalid name_t::SNAP_NAME_LAYOUT_CONTRIB_...");

    }
    snapdev::NOT_REACHED();
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
    snapdev::NOT_USED(variables_timestamp);

    content::content::instance()->add_xml(get_plugin_name());
}



} // namespace layout_contrib
} // namespace snap
// vim: ts=4 sw=4 et
