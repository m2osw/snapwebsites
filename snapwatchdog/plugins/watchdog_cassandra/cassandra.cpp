// Snap Websites Server -- Cassandra watchdog
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
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA


// self
//
#include "cassandra.h"


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




SNAP_PLUGIN_START(cassandra, 1, 0)


/** \brief Get a fixed cassandra plugin name.
 *
 * The cassandra plugin makes use of different names in the database. This
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
    case name_t::SNAP_NAME_WATCHDOG_CASSANDRA_NAME:
        return "name";

    default:
        // invalid index
        throw snap_logic_exception("Invalid SNAP_NAME_WATCHDOG_CASSANDRA_...");

    }
    snapdev::NOT_REACHED();
}




/** \brief Initialize the cassandra plugin.
 *
 * This function is used to initialize the cassandra plugin object.
 */
cassandra::cassandra()
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Clean up the cassandra plugin.
 *
 * Ensure the cassandra object is clean before it is gone.
 */
cassandra::~cassandra()
{
}


/** \brief Get a pointer to the cassandra plugin.
 *
 * This function returns an instance pointer to the cassandra plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the cassandra plugin.
 */
cassandra * cassandra::instance()
{
    return g_plugin_cassandra_factory.instance();
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
QString cassandra::description() const
{
    return "Check whether the Cassandra server is running on this very computer.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString cassandra::dependencies() const
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
int64_t cassandra::do_update(int64_t last_updated)
{
    snapdev::NOT_USED(last_updated);

    SNAP_PLUGIN_UPDATE_INIT();
    // no updating in watchdog
    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Initialize cassandra.
 *
 * This function terminates the initialization of the cassandra plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void cassandra::bootstrap(snap_child * snap)
{
    f_snap = static_cast<watchdog_child *>(snap);

    SNAP_LISTEN(cassandra, "server", watchdog_server, process_watch, boost::placeholders::_1);
}


/** \brief Process this watchdog data.
 *
 * This function runs this watchdog.
 *
 * \param[in] doc  The document.
 */
void cassandra::on_process_watch(QDomDocument doc)
{
    SNAP_LOG_DEBUG("cassandra::on_process_watch(): processing");

    process_list list;

    QDomElement parent(snap_dom::create_element(doc, "watchdog"));
    QDomElement e(snap_dom::create_element(parent, "cassandra"));

    list.set_field(process_list::field_t::COMMAND_LINE);
    list.set_field(process_list::field_t::STATISTICS);
    for(bool found(false); !found;)
    {
        process_list::proc_info::pointer_t info(list.next());
        if(info == nullptr)
        {
            // no cassandra process!?
            //
            QDomElement proc(doc.createElement("process"));
            e.appendChild(proc);

            // TODO: check whether the Cassandra service is active,
            //       if not then it is not an error that the service
            //       is down (although, I'm not too sure why one would
            //       turn cassandra down except to reboot or update this
            //       node?)

            proc.setAttribute("name", "cassandra"); // the name of the command is really "java" at the moment
            proc.setAttribute("error", "missing");

            f_snap->append_error(doc, "cassandra", "can't find \"cassandra\" in the list of processes.", 90);
            return;
        }
        std::string name(info->get_process_name());
        std::string::size_type p(name.find_last_of('/'));
        if(p != std::string::npos)
        {
            name = name.substr(p + 1);
        }
        if(name == "java")
        {
            // found a java entry, search for Cassandra
            int const count_max(info->get_args_size());
            for(int c(0); c < count_max; ++c)
            {
                // is it Cassandra?
                if(info->get_arg(c) == "org.apache.cassandra.service.CassandraDaemon")
                {
                    // got it! (well, one of them at least, they spawn
                    // many times and we just grab the first we find.)
                    //
                    QDomElement proc(doc.createElement("process"));
                    e.appendChild(proc);

                    proc.setAttribute("name", "cassandra");

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

                    found = true;
                    break;
                }
            }
        }
    }

    // TODO: do further checks to make sure everything is A-Okay
    //       although checks against the database itself are
    //       to be done once in a while, not every minute
    //
}





SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
