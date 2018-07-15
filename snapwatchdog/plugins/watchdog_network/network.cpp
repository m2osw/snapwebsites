// Snap Websites Server -- Network watchdog
// Copyright (c) 2013-2018  Made to Order Software Corp.  All Rights Reserved
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

// ourselves
//
#include "network.h"

// snapwebsites lib
//
#include <snapwebsites/log.h>
#include <snapwebsites/not_used.h>
#include <snapwebsites/process.h>
#include <snapwebsites/qdomhelpers.h>


// last include
//
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
    f_snap = static_cast<watchdog_child *>(snap);

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
    SNAP_LOG_TRACE("network::on_process_watch(): processing");

    QDomElement parent(snap_dom::create_element(doc, "watchdog"));
    QDomElement e(snap_dom::create_element(parent, "network"));

    if(find_snapcommunicator(e))
    {
        // snapcommunicator is running, it should have been giving us
        // some information such as how many neighbors it is connected
        // with
        //

        // TODO: implement
    }
    else
    {
        // add tests for when snapcommunicator is not running
        // (maybe these only run after 5 min. to give other daemons
        // time to start)
        //

        // TODO: implement
    }
}


bool network::find_snapcommunicator(QDomElement e)
{
    process_list list;

    list.set_field(process_list::field_t::COMMAND_LINE);
    list.set_field(process_list::field_t::STATISTICS);
    for(;;)
    {
        process_list::proc_info::pointer_t info(list.next());
        if(info == nullptr)
        {
            // no cassandra process!?
            QDomElement proc(e.ownerDocument().createElement("process"));
            e.appendChild(proc);

            proc.setAttribute("name", "snapcommunicator");
            proc.setAttribute("error", "missing");

            f_snap->append_error(e.ownerDocument(), "snapcommunicator", "can't find mandatory processs \"napcommunicator\" in the list of processes. network health is not available.", 99);

            return false;
        }
        std::string name(info->get_process_name());
        std::string::size_type p(name.find_last_of('/'));
        if(p != std::string::npos)
        {
            name = name.substr(p + 1);
        }
        if(name == "snapcommunicator")
        {
            // got it! (well, one of them at least)
            //
            QDomElement proc(e.ownerDocument().createElement("process"));
            e.appendChild(proc);

            proc.setAttribute("name", "snapfirewall");

            //proc.setAttribute("cmdline", cmdline); -- TBD do we want to build the command line to save in the XML?
            proc.setAttribute("pcpu", QString("%1").arg(info->get_pcpu()));
            proc.setAttribute("total_size", QString("%1").arg(info->get_total_size()));
            proc.setAttribute("resident", QString("%1").arg(info->get_resident_size()));
            proc.setAttribute("tty", QString("%1").arg(info->get_tty()));

            unsigned long long utime;
            unsigned long long stime;
            unsigned long long cutime;
            unsigned long long cstime;
            info->get_times(utime, stime, cutime, cstime);

            proc.setAttribute("utime", QString("%1").arg(utime));
            proc.setAttribute("stime", QString("%1").arg(stime));
            proc.setAttribute("cutime", QString("%1").arg(cutime));
            proc.setAttribute("cstime", QString("%1").arg(cstime));
            return true;
        }
    }
}





SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
