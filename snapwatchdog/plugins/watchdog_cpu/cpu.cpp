// Snap Websites Server -- CPU watchdog: record CPU usage over time
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
#include "cpu.h"


// snapwebsites lib
//
#include <snapwebsites/log.h>
#include <snapwebsites/qdomhelpers.h>


// snapdev lib
//
#include <snapdev/not_used.h>


// Qt lib
//
#include <QFile>


// C lib
//
#include <proc/sysinfo.h>


// last include
//
#include <snapdev/poison.h>




SNAP_PLUGIN_START(cpu, 1, 0)


/** \brief Get a fixed cpu plugin name.
 *
 * The cpu plugin makes use of different fixed names. This function
 * ensures that you always get the right spelling for a given name.
 *
 * \param[in] name  The name to retrieve.
 *
 * \return A pointer to the name.
 */
char const * get_name(name_t name)
{
    switch(name)
    {
    case name_t::SNAP_NAME_WATCHDOG_CPU_NAME:
        return "name";

    default:
        // invalid index
        throw snap_logic_exception("Invalid SNAP_NAME_WATCHDOG_CPU_...");

    }
    snapdev::NOT_REACHED();
}




/** \brief Initialize the cpu plugin.
 *
 * This function is used to initialize the cpu plugin object.
 */
cpu::cpu()
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Clean up the cpu plugin.
 *
 * Ensure the cpu object is clean before it is gone.
 */
cpu::~cpu()
{
}


/** \brief Get a pointer to the cpu plugin.
 *
 * This function returns an instance pointer to the cpu plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the cpu plugin.
 */
cpu * cpu::instance()
{
    return g_plugin_cpu_factory.instance();
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
QString cpu::description() const
{
    return "Check the CPU load and instant usage.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString cpu::dependencies() const
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
int64_t cpu::do_update(int64_t last_updated)
{
    snapdev::NOT_USED(last_updated);

    SNAP_PLUGIN_UPDATE_INIT();
    // no updating in watchdog
    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Initialize cpu.
 *
 * This function terminates the initialization of the cpu plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void cpu::bootstrap(snap_child * snap)
{
    f_snap = static_cast<watchdog_child *>(snap);

    SNAP_LISTEN(cpu, "server", watchdog_server, process_watch, boost::placeholders::_1);
}


/** \brief Process this watchdog data.
 *
 * This function runs this watchdog.
 *
 * \param[in] doc  The document.
 */
void cpu::on_process_watch(QDomDocument doc)
{
    SNAP_LOG_DEBUG("cpu::on_process_watch(): processing");

    QDomElement parent(snap_dom::create_element(doc, "watchdog"));
    QDomElement e(snap_dom::create_element(parent, "cpu"));

    // automatically initialized when loading the procps library
    e.setAttribute("cpu_count", QString("%1").arg(smp_num_cpus));
    e.setAttribute("cpu_freq", QString("%1").arg(Hertz)); // in hz, so a large number

    // total uptime and total idle time since boot
    {
        double uptime_secs(0.0);
        double idle_secs(0.0);
        uptime(&uptime_secs, &idle_secs); // also returns uptime_secs
        e.setAttribute("uptime", QString("%1").arg(uptime_secs));
        e.setAttribute("idle", QString("%1").arg(idle_secs));
    }

    // average CPU usage in the last 1 minute, 5 minutes, 15 minutes
    {
        double avg1(0.0);
        double avg5(0.0);
        double avg15(0.0);
        loadavg(&avg1, &avg5, &avg15);
        e.setAttribute("avg1", QString("%1").arg(avg1));
        e.setAttribute("avg5", QString("%1").arg(avg5));
        e.setAttribute("avg15", QString("%1").arg(avg15));

        // we always need the path to this cache file, if the CPU is
        // okay (not overloaded) then we want to delete the file
        // which in effect resets the timer
        //
        QString cache_path(f_snap->get_server_parameter(snap::watchdog::get_name(snap::watchdog::name_t::SNAP_NAME_WATCHDOG_CACHE_PATH)));
        if(cache_path.isEmpty())
        {
            cache_path = "/var/cache/snapwebsites/snapwatchdog";
        }
        QString const high_cpu_usage_filename(cache_path + "/high_cpu_usage.txt");

        double max_avg1(smp_num_cpus);
        if(max_avg1 > 1.0) // with 1 CPU, go up to 100%
        {
            if(max_avg1 <= 2.0)
            {
                max_avg1 *= 0.95; // with 2 CPUs, go up to 95%
            }
            else
            {
                max_avg1 *= 0.8; // with 3+, go up to 80%
            }
        }
        if(static_cast<double>(avg1) >= max_avg1)
        {
            // using too much of the CPUs is considered a warning, however,
            // if it lasts for too long (15 min.) it becomes an error
            //
            bool add_warning(true);

            int64_t const start_date(f_snap->get_start_date());

            // to track the CPU over time, we create a file
            //
            // we use the cache location since the file should not stay
            // around for very long (a few minutes) or there is a problem
            // on the computer
            //
            QFile file(high_cpu_usage_filename);
            if(file.exists())
            {
                if(file.open(QIODevice::ReadOnly))
                {
                    QByteArray const value(file.readAll());
                    QString const high_cpu_start_date_str(value);
                    bool ok(false);
                    int64_t const high_cpu_start_date(high_cpu_start_date_str.toLongLong(&ok, 10));
                    if(ok)
                    {
                        // high CPU started more than 15 min. ago?
                        //
                        if(start_date - high_cpu_start_date > 15LL * 60LL * 1000000LL)
                        {
                            // processors are overloaded on this machine
                            //
                            e.setAttribute("error", "High CPU usage");
                            f_snap->append_error(doc, "cpu", "High CPU usage.", 100);
                            add_warning = false;
                        }
                    }
                }
            }
            else
            {
                if(file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
                {
                    QString const data_str(QString("%1").arg(start_date));
                    QByteArray const data(data_str.toUtf8());
                    file.write(data.data(), data.size());
                }
            }

            if(add_warning)
            {
                e.setAttribute("cpu_warning", "High CPU usage");
            }
        }
        else
        {
            // CPU usage is not that high right now, eliminate the file
            //
            unlink(high_cpu_usage_filename.toUtf8().data());
        }
    }

    // some additional statistics
    // (maybe one day they'll use a structure instead!)
    // typedef unsigned long jiff;
    // total CPU usage is in Jiffies (USER_HZ)
    {
        jiff cuse;                      // total CPU usage by user, normal processes
        jiff cice;                      // total CPU usage by user, niced processes
        jiff csys;                      // total CPU usage by system (kernel)
        jiff cide;                      // total CPU IDLE
        jiff ciow;                      // total CPU I/O wait
        jiff cxxx;                      // total CPU usage servicing interrupts
        jiff cyyy;                      // total CPU usage servicing softirqs
        jiff czzz;                      // total CPU usage running virtual hosts (steal time)
        unsigned long pin;              // page in (read back from cache)
        unsigned long pout;             // page out (freed from memory)
        unsigned long s_in;             // swap in (read back from swap)
        unsigned long sout;             // swap out (written to swap to free form memory)
        unsigned intr;                  // total number of interrupts so far
        unsigned ctxt;                  // total number of context switches
        unsigned int running;           // total number of processes currently running
        unsigned int blocked;           // total number of processes currently blcoked
        unsigned int btime;             // boot time (as a Unix time)
        unsigned int processes;         // total number of processes ever created

        getstat(&cuse, &cice, &csys, &cide, &ciow, &cxxx, &cyyy, &czzz,
             &pin, &pout, &s_in, &sout, &intr, &ctxt, &running, &blocked,
             &btime, &processes);

        e.setAttribute("total_cpu_user", QString("%1").arg(cuse + cice));
        e.setAttribute("total_cpu_system", QString("%1").arg(csys));
        e.setAttribute("total_cpu_wait", QString("%1").arg(cide + ciow));
        e.setAttribute("page_cache_in", QString("%1").arg(pin));
        e.setAttribute("page_cache_out", QString("%1").arg(pout));
        e.setAttribute("swap_cache_in", QString("%1").arg(s_in));
        e.setAttribute("swap_cache_out", QString("%1").arg(sout));
        e.setAttribute("time_of_boot", QString("%1").arg(btime));
        if(running > 1)
        {
            e.setAttribute("processes_running", QString("%1").arg(running));
        }
        if(blocked != 0)
        {
            e.setAttribute("processes_blocked", QString("%1").arg(blocked));
        }
        e.setAttribute("total_processes", QString("%1").arg(processes));
    }
}





SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
