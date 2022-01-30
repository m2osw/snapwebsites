// Snap Websites Server -- Disk watchdog: report disk usage over time.
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
#include "disk.h"


// snapwebsites lib
//
#include <snapwebsites/log.h>
#include <snapwebsites/mounts.h>
#include <snapwebsites/qdomhelpers.h>


// snapdev lib
//
#include <snapdev/not_used.h>


// C lib
//
#include <sys/statvfs.h>


// last include
//
#include <snapdev/poison.h>


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
    case name_t::SNAP_NAME_WATCHDOG_DISK_IGNORE:
        return "disk_ignore";

    default:
        // invalid index
        throw snap_logic_exception("Invalid SNAP_NAME_WATCHDOG_DISK_...");

    }
    snapdev::NOT_REACHED();
}



namespace
{


std::vector<QString> const g_ignore_filled_partitions =
{
    "^/snap/core/"
};



/** \brief The alarm handler we use to create a statvfs_try() function.
 *
 * This function is a hanlder we use to sound the alarm and prevent
 * the statvfs() from holding us up forever.
 */
void statvfs_alarm_handler(int sig)
{
    snapdev::NOT_USED(sig);
}


/** \brief A statvfs() that times out in case a drive locks us up.
 *
 * On Feb 10, 2018, I was testing snapwatchdog and it was getting
 * stuck on statvfs(). I have keybase installed on my system and
 * it failed restarting properly. Once restarted, everything worked
 * as expected.
 *
 * The `df` command would also lock up.
 *
 * The statvfs() is therefore the culprit. This function is used
 * in order to time out if the function doesn't return in a speedy
 * enough period.
 *
 * \note
 * If the function times out, it returns -1 and sets the errno
 * to EINTR. In other cases, a different errno is set.
 *
 * \param[in] path  The name of the file to stat.
 * \param[in] s  The structure were the results are saved on success.
 * \param[in] seconds  The number of seconds to wait before we time out.
 *
 * \return 0 on success, -1 on error and errno set appropriately.
 */
int statvfs_try(char const * path, struct statvfs * s, unsigned int seconds)
{
    struct sigaction alarm_action;
    struct sigaction saved_action;

    memset(&alarm_action, 0, sizeof(alarm_action));
    memset(&saved_action, 0, sizeof(saved_action));

    // note that the flags do not include SA_RESTART, so
    // statvfs() should be interrupted on the SIGALRM signal
    // and not restarted
    //
    alarm_action.sa_flags = 0;
    sigemptyset(&alarm_action.sa_mask);
    alarm_action.sa_handler = statvfs_alarm_handler;

    // first we setup the alarm handler as setting the alarm before
    // would mean that we don't get our handler called
    //
    if(sigaction(SIGALRM, &alarm_action, &saved_action) != 0)
    {
        return -1;
    }

    // alarm() does not return an errors
    //
    unsigned int old_alarm(alarm(seconds));
    time_t const start_time(time(nullptr));

    // clear errno
    //
    errno = 0;

    // do the statvfs() now
    //
    int const rc(statvfs(path, s));

    // save the errno value as alarm() and sigaction() might change it
    //
    int const saved_errno(errno);

    // make sure our or someone else handler does not get called
    // (this is if the alarm did not happen)
    //
    alarm(0);

    // reset the signal handler
    //
    // we have to ignore the error in this case because there is
    // pretty much nothing we can do about it (throw?!)
    //
    snapdev::NOT_USED(sigaction(SIGALRM, &saved_action, nullptr));

    // reset the alarm if required (if 0, avoid the system call)
    //
    if(old_alarm != 0)
    {
        // adjust the number of seconds with the number of seconds
        // that elapsed since we started our own alarm
        //
        time_t const elapsed(time(nullptr) - start_time);
        if(static_cast<unsigned int>(elapsed) >= old_alarm)
        {
            // we use this condition in part because the old_alarm
            // variable is an 'unsigned int'
            //
            old_alarm = 1;
        }
        else
        {
            old_alarm -= elapsed;
        }
        alarm(old_alarm);
    }

    // restore the errno that statvfs() generated
    //
    errno = saved_errno;

    // the statvfs() return code
    //
    return rc;
}


}
// no-name namespace




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
    snapdev::NOT_USED(last_updated);
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
    f_snap = static_cast<watchdog_child *>(snap);

    SNAP_LISTEN(disk, "server", watchdog_server, process_watch, boost::placeholders::_1);
}


/** \brief Process this watchdog data.
 *
 * This function runs this watchdog.
 *
 * \param[in] doc  The document.
 */
void disk::on_process_watch(QDomDocument doc)
{
    SNAP_LOG_DEBUG("disk::on_process_watch(): processing");

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
        if(statvfs_try(m[idx].get_dir().c_str(), &s, 3) == 0)
        {
            // got an entry, however, we ignore entries that have a number
            // of block equal to zero because those are virtual drives
            //
            if(s.f_blocks != 0)
            {
                // we can't use create_element() since we want a new partition
                // each time we create an entry
                //QDomElement p(snap_dom::create_element(e, "partition"));
                QDomElement p(doc.createElement("partition"));
                e.appendChild(p);

                // directory where this partition is attached
                //
                QString const dir(QString::fromUtf8(m[idx].get_dir().c_str()));
                p.setAttribute("dir", dir);

                // we do not expect to get a server with blocks of 512 bytes
                // otherwise the following lose one bit of precision...
                //
                p.setAttribute("blocks", QString("%1").arg(s.f_blocks * s.f_frsize / 1024));
                p.setAttribute("bfree", QString("%1").arg(s.f_bfree * s.f_frsize / 1024));
                p.setAttribute("available", QString("%1").arg(s.f_bavail * s.f_frsize / 1024));
                p.setAttribute("ffree", QString("%1").arg(s.f_ffree));
                p.setAttribute("favailable", QString("%1").arg(s.f_favail));
                p.setAttribute("flags", QString("%1").arg(s.f_flag));

                // is that partition full at 90% or more?
                //
                double const usage(1.0 - static_cast<double>(s.f_bavail) / static_cast<double>(s.f_blocks));
                if(usage >= 0.9)
                {
                    // we mark the partition as quite full even if the user
                    // marks it as "ignore that one"
                    //
                    p.setAttribute("error", "partition is uses over 90%");

                    // if we find it in the list of partitions to be
                    // ignored then we 
                    auto it1(std::find_if(
                              g_ignore_filled_partitions.begin()
                            , g_ignore_filled_partitions.end()
                            , [dir](auto pattern)
                            {
                                QRegExp const pat(pattern);
                                return pat.indexIn(dir) != -1;
                            }));
                    if(it1 == g_ignore_filled_partitions.end())
                    {
                        // the user can also define a list of QRegExp which
                        // we test now to ignore further partitions
                        //
                        QString const disk_ignore(f_snap->get_server_parameter(get_name(name_t::SNAP_NAME_WATCHDOG_DISK_IGNORE)));
                        QStringList const disk_ignore_patterns(disk_ignore.split(':'));
                        auto it2(std::find_if(
                                  disk_ignore_patterns.begin()
                                , disk_ignore_patterns.end()
                                , [dir](auto pattern)
                                {
                                    QRegExp const pat(pattern);
                                    return pat.indexIn(dir) != -1;
                                }));
                        if(it2 == disk_ignore_patterns.end())
                        {
                            // get the name of the host for the error message
                            //
                            char hostname[HOST_NAME_MAX + 1];
                            if(gethostname(hostname, sizeof(hostname)) != 0)
                            {
                                strncpy(hostname, "<unknown>", sizeof(hostname));
                            }

                            // priority increases as the disk gets filled up more
                            //
                            int const priority(usage >= 99.9
                                                    ? 100
                                                    : (usage >= 95
                                                        ? 80
                                                        : 55));
                            f_snap->append_error(doc
                                               , "disk"
                                               , QString("partition \"%1\" on \"%2\" is close to full (%3%)")
                                                        .arg(m[idx].get_dir().c_str())
                                                        .arg(hostname)
                                                        .arg(usage * 100.0)
                                               , priority);
                        }
                    }
                }
            }
        }
    }
}





SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
