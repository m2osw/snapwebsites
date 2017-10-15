// Snap Websites Server -- Network watchdog
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

#include "network.h"

#include "snapwatchdog.h"

#include <snapwebsites/not_used.h>
#include <snapwebsites/qdomhelpers.h>

#include <snapwebsites/poison.h>


SNAP_PLUGIN_START(network, 1, 0)






/** \brief Get a fixed network plugin name.
 *
 * The network plugin makes use of different names in the database. This
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
    case name_t::SNAP_NAME_WATCHDOG_NETWORK_NAME:
        return "name";

    default:
        // invalid index
        throw snap_logic_exception("Invalid SNAP_NAME_WATCHDOG_NETWORK_...");

    }
    NOTREACHED();
}




/** \brief Initialize the network plugin.
 *
 * This function is used to initialize the network plugin object.
 */
network::network()
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Clean up the network plugin.
 *
 * Ensure the network object is clean before it is gone.
 */
network::~network()
{
}


/** \brief Get a pointer to the network plugin.
 *
 * This function returns an instance pointer to the network plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the network plugin.
 */
network * network::instance()
{
    return g_plugin_network_factory.instance();
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
QString network::description() const
{
    return "Check that the network is up and running.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString network::dependencies() const
{
    return "|server|";
}


/** \brief Check whether updates are necessary.
 *
 * This function is ignored in the watchdog.
 *
 * \param[in] last_updated  The UTC Unix date when the website was last updated (in micro seconds).
 *
 * \return The UTC Unix date of the last update of this plugin.
 */
int64_t network::do_update(int64_t last_updated)
{
    NOTUSED(last_updated);

    SNAP_PLUGIN_UPDATE_INIT();
    // no updating in watchdog
    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Initialize network.
 *
 * This function terminates the initialization of the network plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void network::bootstrap(snap_child * snap)
{
    f_snap = snap;

    SNAP_LISTEN0(network, "server", watchdog_server, init);
    SNAP_LISTEN(network, "server", watchdog_server, process_watch, _1);
}


/** \brief Initialize the network plugin.
 *
 * At this time there is nothing for us to initialize for the network.
 */
void network::on_init()
{
}


/** \brief Process this watchdog data.
 *
 * This function runs this watchdog.
 *
 * \param[in] doc  The document.
 */
void network::on_process_watch(QDomDocument doc)
{
    QDomElement parent(snap_dom::create_element(doc, "watchdog"));
    QDomElement e(snap_dom::create_element(parent, "network"));

    // request status information from snapcommunicator which has all of
    // the data we can dream of in regard to the network status
    //

}





SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
