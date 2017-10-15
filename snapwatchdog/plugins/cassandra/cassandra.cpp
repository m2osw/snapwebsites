// Snap Websites Server -- Cassandra watchdog
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

#include "cassandra.h"

#include "not_used.h"

#include "poison.h"


SNAP_PLUGIN_START(cassandra, 1, 0)


/** \brief Get a fixed cassandra plugin name.
 *
 * The cassamdra plugin makes use of different names in the database. This
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
    NOTREACHED();
}




/** \brief Initialize the cassandra plugin.
 *
 * This function is used to initialize the cassandra plugin object.
 */
cassandra::cassandra()
    //: f_snap(NULL) -- auto-init
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
    return "Check whether the Cassandra server is running.";
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
    f_snap = snap;

    SNAP_LISTEN(apache, "server", server, process_watch, _1);
}


/** \brief Process this watchdog data.
 *
 * This function runs this watchdog.
 *
 * \param[in] doc  The document.
 */
void cassandra::on_process_watch(QDomDocument doc)
{
    process_list list;

    QDomElement e(doc.createElement("cassandra"));

    list.set_field(list_process::process_info::field_t::COMMAND_LINE);
    for(;;)
    {
        process_list::process_info_pointer_t info(list.next());
        if(!info)
        {
            // no cassandra process!?
            e.setAttribute("error", "missing");
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
            int count_max(info->get_args_size());
            for(int c(0); c < count_max; ++c)
            {
                // is it Cassandra?
                if(info->get_arg(c) == "org.apache.cassandra.service.CassandraDaemon")
                {
                    // got it!
                    e.setAttribute("pcpu", QString("%1").arg(info->get_pcpu()));
                    e.setAttribute("total_size", QString("%1").arg(info->get_total_size()));
                    e.setAttribute("resident", QString("%1").arg(info->get_resident_size()));
                    e.setAttribute("tty", QString("%1").arg(info->get_tty()));

                    unsigned long long utime;
                    unsigned long long stime;
                    unsigned long long cutime;
                    unsigned long long cstime;
                    info->get_times(utime, stime, cutime, cstime);

                    e.setAttribute("utime", QString("%1").arg(utime));
                    e.setAttribute("stime", QString("%1").arg(stime));
                    e.setAttribute("cutime", QString("%1").arg(cutime));
                    e.setAttribute("cstime", QString("%1").arg(cstime));
                    return;
                }
            }
        }
    }
}





SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
