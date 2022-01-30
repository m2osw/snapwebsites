// Snap Websites Server -- watchdog firewall
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
#include "firewall.h"


// snapwatchdog lib
//
#include "snapwatchdog/snapwatchdog.h"


// snapwebsites lib
//
#include <snapwebsites/log.h>
#include <snapwebsites/process.h>
#include <snapwebsites/qdomhelpers.h>


// snapdev lib
//
#include <snapdev/not_used.h>


// last include
//
#include <snapdev/poison.h>





SNAP_PLUGIN_START(firewall, 1, 0)


namespace
{




}


/** \brief Get a fixed firewall plugin name.
 *
 * The firewall plugin makes use of different names. This function ensures
 * that you get the right spelling for a given name.
 *
 * \param[in] name  The name to retrieve.
 *
 * \return A pointer to the name.
 */
char const * get_name(name_t name)
{
    switch(name)
    {
    case name_t::SNAP_NAME_WATCHDOG_FIREWALL_NAME:
        return "name";

    default:
        // invalid index
        throw snap_logic_exception("Invalid SNAP_NAME_WATCHDOG_FIREWALL_...");

    }
    snapdev::NOT_REACHED();
}




/** \brief Initialize the firewall plugin.
 *
 * This function is used to initialize the firewall plugin object.
 */
firewall::firewall()
{
}


/** \brief Clean up the firewall plugin.
 *
 * Ensure the firewall object is clean before it is gone.
 */
firewall::~firewall()
{
}


/** \brief Get a pointer to the firewall plugin.
 *
 * This function returns an instance pointer to the firewall plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the firewall plugin.
 */
firewall * firewall::instance()
{
    return g_plugin_firewall_factory.instance();
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
QString firewall::description() const
{
    return "Check whether the Apache server is running.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString firewall::dependencies() const
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
int64_t firewall::do_update(int64_t last_updated)
{
    snapdev::NOT_USED(last_updated);

    SNAP_PLUGIN_UPDATE_INIT();
    // no updating in watchdog
    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Initialize firewall.
 *
 * This function terminates the initialization of the firewall plugin
 * by registering for various events.
 *
 * \param[in] snap  The child handling this request.
 */
void firewall::bootstrap(snap_child * snap)
{
    f_snap = static_cast<watchdog_child *>(snap);

    SNAP_LISTEN(firewall, "server", watchdog_server, process_watch, boost::placeholders::_1);
}


/** \brief Process this watchdog data.
 *
 * This function runs this watchdog.
 *
 * \param[in] doc  The document.
 */
void firewall::on_process_watch(QDomDocument doc)
{
    SNAP_LOG_DEBUG("firewall::on_process_watch(): processing");

    QDomElement parent(snap_dom::create_element(doc, "watchdog"));
    QDomElement e(snap_dom::create_element(parent, "firewall"));

    // first we check that the snapfirewall daemon is running
    process_list list;

    list.set_field(process_list::field_t::COMMAND_LINE);
    list.set_field(process_list::field_t::STATISTICS);
    for(;;)
    {
        process_list::proc_info::pointer_t info(list.next());
        if(info == nullptr)
        {
            // no snapfirewall process!?
            //
            QDomElement proc(doc.createElement("process"));
            e.appendChild(proc);

            // TODO: check whether the snapfirewall service is active,
            //       if not then it is not an error that the service
            //       is down (although, I'm not too sure why one would
            //       turn snapfirewall down)

            proc.setAttribute("name", "snapfirewall");
            proc.setAttribute("error", "missing");

            f_snap->append_error(doc, "firewall", "cannot find \"snapfirewall\" in the list of processes.", 95);
            return;
        }
        std::string name(info->get_process_name());
        std::string::size_type p(name.find_last_of('/'));
        if(p != std::string::npos)
        {
            name = name.substr(p + 1);
        }
        if(name == "snapfirewall")
        {
            // got snapfirewall server, get the extra info
            //
            QDomElement proc(doc.createElement("process"));
            e.appendChild(proc);

            proc.setAttribute("name", "snapfirewall");

            //proc.setAttribute("cmdline", cmdline); -- TBD do we want to build the command line to save in the XML?
            proc.setAttribute("pcpu",       QString("%1").arg(info->get_pcpu()));
            proc.setAttribute("total_size", QString("%1").arg(info->get_total_size()));
            proc.setAttribute("resident",   QString("%1").arg(info->get_resident_size()));
            proc.setAttribute("tty",        QString("%1").arg(info->get_tty()));

            unsigned long long utime;
            unsigned long long stime;
            unsigned long long cutime;
            unsigned long long cstime;
            info->get_times(utime, stime, cutime, cstime);

            proc.setAttribute("utime",  QString("%1").arg(utime));
            proc.setAttribute("stime",  QString("%1").arg(stime));
            proc.setAttribute("cutime", QString("%1").arg(cutime));
            proc.setAttribute("cstime", QString("%1").arg(cstime));
            break;
        }
    }

    // TODO:
    // check that certain rules exist, that way we make sure that it's
    // really up (although we do not really have a good way to test
    // the system other than that...)
    //
    // long term it would be great to have a way to do a kind of nmap
    // to see what ports are open on what IP, that way we can make sure
    // that only ports that we generally allow are opened and all the
    // others are not, however, an nmap test is very long and slow so
    // it would required another tool we run once a day with a report
    // of the current firewall status
    //
}





SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
