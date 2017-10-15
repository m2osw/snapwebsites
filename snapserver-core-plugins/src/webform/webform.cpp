// Snap Websites Server -- let end users dynamically create forms
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

#include "webform.h"

#include "../content/content.h"

#include <snapwebsites/not_reached.h>
#include <snapwebsites/not_used.h>

#include <snapwebsites/poison.h>


SNAP_PLUGIN_START(webform, 1, 0)


/** \brief Get a fixed webform plugin name.
 *
 * The webform plugin makes use of different names in the database. This
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
    case name_t::SNAP_NAME_WEBFORM_NAME:
        return "webform";

    default:
        // invalid index
        throw snap_logic_exception("invalid SNAP_NAME_WEBFORM_...");

    }
    NOTREACHED();
}






/** \brief Initialize the webform plugin.
 *
 * This function is used to initialize the webform plugin object.
 */
webform::webform()
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Clean up the webform plugin.
 *
 * Ensure the webform object is clean before it is gone.
 */
webform::~webform()
{
}


/** \brief Get a pointer to the webform plugin.
 *
 * This function returns an instance pointer to the webform plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the webform plugin.
 */
webform * webform::instance()
{
    return g_plugin_webform_factory.instance();
}


/** \brief Send users to the plugin settings.
 *
 * This path represents this plugin settings.
 */
QString webform::settings_path() const
{
    return "/admin/settings/webform";
}


/** \brief A path or URI to a logo for this plugin.
 *
 * This function returns a 64x64 icons representing this plugin.
 *
 * \return A path to the logo.
 */
QString webform::icon() const
{
    return "/images/webform/webform-logo-64x64.png";
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
QString webform::description() const
{
    return "Allows end users to dynamically create their own forms."
        " This is an extension of our Snap! editor that allows you"
        " to create forms directly from your website and reuse them"
        " on any page you'd like to reuse them.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString webform::dependencies() const
{
    return "|content|editor|";
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
int64_t webform::do_update(int64_t last_updated)
{
    NOTUSED(last_updated);

    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2012, 1, 1, 0, 0, 0, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Update the database with our content references.
 *
 * Send our content to the database so the system can find us when a
 * user references our administration pages, etc.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables
 *            added to the database by this update (in micro-seconds).
 */
void webform::content_update(int64_t variables_timestamp)
{
    NOTUSED(variables_timestamp);

    content::content::instance()->add_xml(get_plugin_name());
}


/** \brief Initialize webform.
 *
 * This function terminates the initialization of the webform plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void webform::bootstrap(snap_child * snap)
{
    f_snap = snap;
}




SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
