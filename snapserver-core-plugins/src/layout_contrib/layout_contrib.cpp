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



SERVERPLUGINS_START(layout_contrib)
    , ::serverplugins::description(
            "Offer additional files (JS, CSS, Fonts) for layouts.")
    , ::serverplugins::icon("/images/snap/layout_contrib-logo-64x64.png")
    , ::serverplugins::settings_path()
    , ::serverplugins::dependency("content")
    , ::serverplugins::dependency("links")
    , ::serverplugins::dependency("output")
    , ::serverplugins::dependency("path")
    , ::serverplugins::help_uri("https://snapwebsites.org/help")
    , ::serverplugins::categorization_tag("security")
    , ::serverplugins::categorization_tag("spam")
SERVERPLUGINS_END(layout_contrib)





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
time_t layout_contrib::do_update(time_t last_updated, unsigned int phase)
{
    SERVERPLUGINS_PLUGIN_UPDATE_INIT();

    if(phase == 0)
    {
        // first time, make sure the default theme is installed
        //
        SERVERPLUGINS_PLUGIN_UPDATE(2017, 5, 20, 0, 14, 30, content_update);
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
void layout_contrib::content_update(int64_t variables_timestamp)
{
    snapdev::NOT_USED(variables_timestamp);

    content::content::instance()->add_xml(name());
}



} // namespace layout_contrib
} // namespace snap
// vim: ts=4 sw=4 et
