// Snap Websites Server -- watchdog firewall
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

#include "firewall.h"

#include "not_used.h"

#include "poison.h"


SNAP_PLUGIN_START(firewall, 1, 0)


namespace
{

struct apache_data_t
{
    uint32_t    f_pcpu;
    int32_t     f_tty;
    uint64_t    f_utime;
    uint64_t    f_stime;
    uint64_t    f_cutime;
    uint64_t    f_cstime;
    int64_t     f_total_size;
    int64_t     f_resident_size;
};



}


/** \brief Get a fixed apache plugin name.
 *
 * The apache plugin makes use of different names. This function ensures
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
    case name_t::SNAP_NAME_WATCHDOG_APACHE_NAME:
        return "name";

    default:
        // invalid index
        throw snap_logic_exception("Invalid SNAP_NAME_WATCHDOG_APACHE_...");

    }
    NOTREACHED();
}




/** \brief Initialize the apache plugin.
 *
 * This function is used to initialize the apache plugin object.
 */
firewall::firewall()
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Clean up the apache plugin.
 *
 * Ensure the apache object is clean before it is gone.
 */
firewall::~firewall()
{
}


/** \brief Get a pointer to the apache plugin.
 *
 * This function returns an instance pointer to the apache plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the apache plugin.
 */
apache *apache::instance()
{
    return g_plugin_apache_factory.instance();
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
    NOTUSED(last_updated);

    SNAP_PLUGIN_UPDATE_INIT();
    // no updating in watchdog
    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Initialize apache.
 *
 * This function terminates the initialization of the apache plugin
 * by registering for various events.
 *
 * \param[in] snap  The child handling this request.
 */
void firewall::bootstrap(snap_child * snap)
{
    f_snap = snap;

    SNAP_LISTEN(apache, "server", server, process_watch, _1);
}


/** \brief Process this watchdog data.
 *
 * This function runs this watchdog.
 *
 * \param[in] doc  The document.
 * \param[in] e  This element to record this watchdog information.
 */
void firewall::on_process_watch(QDomDocument doc, QDomElement e)
{
    process_list list;

    list.set_field(list_process::process_info::field_t::COMMAND_LINE);
    for(;;)
    {
        process_list::process_info_pointer_t info(list.next());
        if(!info)
        {
            // no apache process!?
            e.setAttribute("error", "missing");
            break;
        }
        std::string name(info->get_process_name());
        std::string::size_type p(name.find_last_of('/'));
        if(p != std::string::npos)
        {
            name = name.substr(p + 1);
        }
        if(name == "apache2")
        {
            // got apache2 server, get the extra info

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
            break;
        }
    }
}





SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
