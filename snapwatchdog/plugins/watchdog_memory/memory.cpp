// Snap Websites Server -- Memory watchdog: record memory usage over time.
// Copyright (c) 2013-2019  Made to Order Software Corp.  All Rights Reserved
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
#include "memory.h"


// snapwebsites lib
//
#include <snapwebsites/log.h>
#include <snapwebsites/qdomhelpers.h>
#include <snapwebsites/meminfo.h>


// snapdev lib
//
#include <snapdev/not_used.h>


// last include
//
#include <snapdev/poison.h>



SNAP_PLUGIN_START(memory, 1, 0)


/** \brief Get a fixed memory plugin name.
 *
 * The memory plugin makes use of different names in the database. This
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
    case name_t::SNAP_NAME_WATCHDOG_MEMORY_NAME:
        return "name";

    default:
        // invalid index
        throw snap_logic_exception("Invalid SNAP_NAME_WATCHDOG_MEMORY_...");

    }
    snapdev::NOT_REACHED();
}




/** \brief Initialize the memory plugin.
 *
 * This function is used to initialize the memory plugin object.
 */
memory::memory()
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Clean up the memory plugin.
 *
 * Ensure the memory object is clean before it is gone.
 */
memory::~memory()
{
}


/** \brief Get a pointer to the memory plugin.
 *
 * This function returns an instance pointer to the memory plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the memory plugin.
 */
memory * memory::instance()
{
    return g_plugin_memory_factory.instance();
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
QString memory::description() const
{
    return "Check current memory usage.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString memory::dependencies() const
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
int64_t memory::do_update(int64_t last_updated)
{
    snapdev::NOT_USED(last_updated);
    SNAP_PLUGIN_UPDATE_INIT();
    // no updating in watchdog
    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Initialize memory.
 *
 * This function terminates the initialization of the memory plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void memory::bootstrap(snap_child * snap)
{
    f_snap = static_cast<watchdog_child *>(snap);

    SNAP_LISTEN(memory, "server", watchdog_server, process_watch, boost::placeholders::_1);
}


/** \brief Process this watchdog data.
 *
 * This function runs this watchdog.
 *
 * \param[in] doc  The document.
 */
void memory::on_process_watch(QDomDocument doc)
{
    SNAP_LOG_DEBUG("memory::on_process_watch(): processing");

    QDomElement parent(snap_dom::create_element(doc, "watchdog"));
    QDomElement e(snap_dom::create_element(parent, "memory"));

    // read "/proc/meminfo"
    //
    meminfo_t const info(get_meminfo());

    // simple memory data should always be available
    e.setAttribute("mem_total",     QString("%1").arg(info.f_mem_total));
    e.setAttribute("mem_free",      QString("%1").arg(info.f_mem_free));
    e.setAttribute("mem_available", QString("%1").arg(info.f_mem_available));
    e.setAttribute("mem_buffers",   QString("%1").arg(info.f_buffers));
    e.setAttribute("mem_cached",    QString("%1").arg(info.f_cached));
    e.setAttribute("swap_cached",   QString("%1").arg(info.f_swap_cached));
    e.setAttribute("swap_total",    QString("%1").arg(info.f_swap_total));
    e.setAttribute("swap_free",     QString("%1").arg(info.f_swap_free));

    bool const high_memory_usage([info]()
            {
                // if we have at least 512MB don't generate an error
                //
                if(info.f_mem_available > 512.0 * 1024.0 * 1024.0)
                {
                    return false;
                }

                // otherwise see whether we use over 80% of the RAM
                //
                // NOTE: the 0.2 value is going to make it true immediatly
                //       if the total amount of memory is about 8Gb or more.
                //
                double const mem_left_percent(static_cast<double>(info.f_mem_available) / static_cast<double>(info.f_mem_total));
                return mem_left_percent < 0.2;
            }());
    if(high_memory_usage)
    {
        e.setAttribute("error", "high memory usage");
        f_snap->append_error(doc, "memory", "high memory usage", 75);
    }

    bool const high_swap_usage([info]()
            {
                // for swap, we generate errors once 50% is in use, it should
                // never be much more than 10% for a good system
                //
                double const swap_left_percent(static_cast<double>(info.f_swap_free) / static_cast<double>(info.f_swap_total));
                return swap_left_percent < 0.5;
            }());
    if(high_swap_usage)
    {
        e.setAttribute("error", "high swap usage");
        f_snap->append_error(doc, "memory", "high swap usage", 65);
    }

    // TODO: we should probably look into processing /proc/swaps
    //       as well, that way we would get details such as what
    //       files / partition is being used, etc.
}





SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
