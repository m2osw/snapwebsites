// Snap Websites Server -- generate lists of links and display them
// Copyright (C) 2014-2017  Made to Order Software Corp.
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

#include "menu.h"

#include "../content/content.h"
#include "../output/output.h"

#include <snapwebsites/log.h>
#include <snapwebsites/not_reached.h>
#include <snapwebsites/not_used.h>

#include <iostream>

#include <QtCore/QDebug>

#include <snapwebsites/poison.h>


SNAP_PLUGIN_START(menu, 1, 0)

/** \brief Get a fixed menu name.
 *
 * The menu plugin makes use of different names in the database. This
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
    case name_t::SNAP_NAME_MENU_NAMESPACE:
        return "menu";

    default:
        // invalid index
        throw snap_logic_exception("invalid name_t::SNAP_NAME_MENU_...");

    }
    NOTREACHED();
}







/** \brief Initialize the menu plugin.
 *
 * This function is used to initialize the menu plugin object.
 */
menu::menu()
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Clean up the menu plugin.
 *
 * Ensure the menu object is clean before it is gone.
 */
menu::~menu()
{
}


/** \brief Get a pointer to the menu plugin.
 *
 * This function returns an instance pointer to the menu plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the menu plugin.
 */
menu * menu::instance()
{
    return g_plugin_menu_factory.instance();
}


/** \brief Send users to the plugin settings.
 *
 * This path represents this plugin settings.
 */
QString menu::settings_path() const
{
    return "/admin/menu";
}


/** \brief A path or URI to a logo for this plugin.
 *
 * This function returns a 64x64 icons representing this plugin.
 *
 * \return A path to the logo.
 */
QString menu::icon() const
{
    return "/images/menu/menu-logo-64x64.png";
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
QString menu::description() const
{
    return "This plugin generates lists of pages used to form a menu."
          " It manages two different types of lists: automated lists,"
          " using the list plugin, and manually created lists where"
          " a user enters each item in the list.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString menu::dependencies() const
{
    return "|content|layout|output|";
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
int64_t menu::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2016, 1, 17, 0, 18, 0, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Update to content of the menu plugin.
 *
 * Send our content to the database so the system can find us when a
 * user references our administration pages, etc.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added
 *                        to the database by this update (in micro-seconds).
 */
void menu::content_update(int64_t variables_timestamp)
{
    NOTUSED(variables_timestamp);

    content::content::instance()->add_xml(get_plugin_name());
}


/** \brief Initialize the menu plugin.
 *
 * This function terminates the initialization of the menu plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void menu::bootstrap(snap_child * snap)
{
    f_snap = snap;
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
void menu::on_generate_main_content(content::path_info_t & ipath, QDomElement & page, QDomElement & body)
{
    output::output::instance()->on_generate_main_content(ipath, page, body);
}


SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
