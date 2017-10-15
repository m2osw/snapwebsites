// Snap Websites Server -- Disk watchdog: report disk usage over time.
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

// ourselves
//
#include "disk.h"

// snapwatchdog lib
//
#include "snapwatchdog.h"

// snapwebsites lib
//
#include <snapwebsites/mounts.h>
#include <snapwebsites/qdomhelpers.h>
#include <snapwebsites/not_used.h>

// C lib
//
#include <sys/statvfs.h>

// last entry
//
#include <snapwebsites/poison.h>


SNAP_PLUGIN_START(disk, 1, 0)


/** \brief Get a fixed disk plugin name.
 *
 * The disk plugin makes use of different names in the database. This
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
    case name_t::SNAP_NAME_WATCHDOG_DISK_NAME:
        return "admin/drafts";

    default:
        // invalid index
        throw snap_logic_exception("Invalid SNAP_NAME_WATCHDOG_DISK_...");

    }
    NOTREACHED();
}




/** \brief Initialize the disk plugin.
 *
 * This function is used to initialize the disk plugin object.
 */
disk::disk()
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Clean up the disk plugin.
 *
 * Ensure the disk object is clean before it is gone.
 */
disk::~disk()
{
}


/** \brief Get a pointer to the disk plugin.
 *
 * This function returns an instance pointer to the disk plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the disk plugin.
 */
disk * disk::instance()
{
    return g_plugin_disk_factory.instance();
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
QString disk::description() const
{
    return "Check disk space of all mounted drives.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString disk::dependencies() const
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
int64_t disk::do_update(int64_t last_updated)
{
    NOTUSED(last_updated);
    SNAP_PLUGIN_UPDATE_INIT();
    // no updating in watchdog
    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Initialize disk.
 *
 * This function terminates the initialization of the disk plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void disk::bootstrap(snap_child * snap)
{
    f_snap = snap;

    SNAP_LISTEN(disk, "server", watchdog_server, process_watch, _1);
}


/** \brief Process this watchdog data.
 *
 * This function runs this watchdog.
 *
 * \param[in] doc  The document.
 */
void disk::on_process_watch(QDomDocument doc)
{
    QDomElement parent(snap_dom::create_element(doc, "watchdog"));
    QDomElement e(snap_dom::create_element(parent, "disk"));

    // read the various mounts on this server
    //
    // TBD: instead of all mounts, we may want to look into definitions
    //      in our configuration file?
    mounts m("/proc/mounts");

    // check each disk
    size_t const max_mounts(m.size());
    for(size_t idx(0); idx < max_mounts; ++idx)
    {
        struct statvfs s;
        memset(&s, 0, sizeof(s));
        if(statvfs(m[idx].get_dir().c_str(), &s) == 0)
        {
            // got an entry, however, we ignore entries that have a number
            // of block equal to zero because those are virtual drives
            if(s.f_blocks != 0)
            {
                QDomElement p(doc.createElement("partition"));
                e.appendChild(p);

                p.setAttribute("dir", m[idx].get_dir().c_str());

                // we do not expect to get a server with blocks of 512 bytes
                // otherwise the following lose a bit of precision...
                p.setAttribute("blocks", QString("%1").arg(s.f_blocks * s.f_frsize / 1024));
                p.setAttribute("available", QString("%1").arg(s.f_bavail * s.f_frsize / 1024));
            }
        }
    }
}





SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
