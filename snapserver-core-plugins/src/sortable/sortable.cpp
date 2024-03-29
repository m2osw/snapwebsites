// Copyright (c) 2016-2019  Made to Order Software Corp.  All Rights Reserved
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
#include    "sortable.h"


// other plugins
//
#include    "../output/output.h"
#include    "../messages/messages.h"
#include    "../permissions/permissions.h"


// snaplogger
//
#include    <snaplogger/message.h>


// snapdev
//
#include    <snapdev/not_reached.h>
#include    <snapdev/not_used.h>


// C++
//
#include    <algorithm>
#include    <iostream>


// OpenSSL
//
#include    <openssl/rand.h>


// Qt
//
#include    <QChar>


// last include
//
#include    <snapdev/poison.h>



/** \file
 * \brief The sortable plugin adds a widget one can use to sort list items.
 *
 * The sortable plugin is the integration of the Sortable.js library as
 * as Snap! editor widget giving the end user the ability to sort a list
 * of items by dragging and dropping those items.
 */

namespace snap
{
namespace sortable
{


SERVERPLUGINS_START(sortable)
    , ::serverplugins::description(
            "Gives the end users the ability to sort list items."
            " This plugin is very rarely added by itself. Instead, another"
            " plugin that needs the sort capability will depend on it.")
    , ::serverplugins::icon("/images/sortable/sortable-logo-64x64.png")
    , ::serverplugins::settings_path()
    , ::serverplugins::dependency("editor")
    , ::serverplugins::dependency("messages")
    , ::serverplugins::dependency("output")
    , ::serverplugins::dependency("permissions")
    , ::serverplugins::dependency("users")
    , ::serverplugins::help_uri("https://snapwebsites.org/help")
    , ::serverplugins::categorization_tag("sortable")
SERVERPLUGINS_END(sortable)



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
time_t sortable::do_update(time_t last_updated, unsigned int phase)
{
    SERVERPLUGINS_PLUGIN_UPDATE_INIT();

    if(phase == 0)
    {
        SERVERPLUGINS_PLUGIN_UPDATE(2016, 1, 24, 0, 33, 4, content_update);
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
void sortable::content_update(int64_t variables_timestamp)
{
    snapdev::NOT_USED(variables_timestamp);

    content::content::instance()->add_xml(get_plugin_name());
}


/** \brief Initialize the sortable.
 *
 * This function terminates the initialization of the sortable plugin
 * by registering for different events.
 */
void sortable::bootstrap()
{
    SERVERPLUGINS_LISTEN(sortable, "editor", editor::editor, prepare_editor_form, boost::placeholders::_1);
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



} // namespace sortable
} // namespace snap
// vim: ts=4 sw=4 et
