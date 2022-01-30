// Snap Websites Server -- Log watchdog: report large logs and various errors.
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
#include "./log.h"


// snapwebsites lib
//
#include <snapwebsites/glob_dir.h>
#include <snapwebsites/log.h>
#include <snapwebsites/mounts.h>
#include <snapwebsites/qdomhelpers.h>


// snapdev lib
//
#include <snapdev/not_used.h>


// C lib
//
#include <sys/stat.h>


// last include
//
#include <snapdev/poison.h>



SNAP_PLUGIN_START(log, 1, 0)


/** \brief Get a fixed log plugin name.
 *
 * The log plugin makes use of different names in the database. This
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
    case name_t::SNAP_NAME_WATCHDOG_LOG_IGNORE:
        return "log_ignore";

    default:
        // invalid index
        throw snap_logic_exception("Invalid SNAP_NAME_WATCHDOG_LOG_...");

    }
    snapdev::NOT_REACHED();
}







namespace
{





}
// no name namespace








/** \brief Initialize the log plugin.
 *
 * This function is used to initialize the log plugin object.
 */
log::log()
{
}


/** \brief Clean up the log plugin.
 *
 * Ensure the log object is clean before it is gone.
 */
log::~log()
{
}


/** \brief Get a pointer to the log plugin.
 *
 * This function returns an instance pointer to the log plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the log plugin.
 */
log * log::instance()
{
    return g_plugin_log_factory.instance();
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
QString log::description() const
{
    // IMPORTANT NOTE: the plugin does NOT check the contents of the
    //                 log files, this is done by another tool which
    //                 permanently listens for changes to log files
    //
    return "Check log files existance, size, ownership, and permissions.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString log::dependencies() const
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
int64_t log::do_update(int64_t last_updated)
{
    snapdev::NOT_USED(last_updated);
    SNAP_PLUGIN_UPDATE_INIT();
    // no updating in watchdog
    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Initialize log.
 *
 * This function terminates the initialization of the log plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void log::bootstrap(snap_child * snap)
{
    f_snap = static_cast<watchdog_child *>(snap);

    SNAP_LISTEN(log, "server", watchdog_server, process_watch, boost::placeholders::_1);
}


/** \brief Process this watchdog data.
 *
 * This function runs this watchdog.
 *
 * \param[in] doc  The document.
 */
void log::on_process_watch(QDomDocument doc)
{
    SNAP_LOG_DEBUG("log::on_process_watch(): processing");

    QString const log_path(f_snap->get_server_parameter(watchdog::get_name(watchdog::name_t::SNAP_NAME_WATCHDOG_LOG_DEFINITIONS_PATH)));
    watchdog_log_t::vector_t log_defs(watchdog_log_t::load(log_path));

    QDomElement parent(snap_dom::create_element(doc, "watchdog"));
    QDomElement e(snap_dom::create_element(parent, "logs"));

    // check each log
    //
    size_t const max_logs(log_defs.size());
    for(size_t idx(0); idx < max_logs; ++idx)
    {
        watchdog_log_t const & l(log_defs[idx]);
        QString const & path(l.get_path());
        snap::snap_string_list const & patterns(l.get_patterns());
        f_found = false;
        for(auto const & p : patterns)
        {
            glob_dir const log_filenames(path + "/" + p, GLOB_ERR | GLOB_NOSORT | GLOB_NOESCAPE);
            log_filenames.enumerate_glob(std::bind(&log::check_log, this, std::placeholders::_1, l, e));
        }
        if(!f_found)
        {
            QDomElement log_tag(doc.createElement("log"));
            e.appendChild(log_tag);

            QString const err_msg(QString("no logs found for %1 which says it is mandatory to have at least one log file")
                                                .arg(l.get_name()));
            log_tag.setAttribute("error", err_msg);

            f_snap->append_error(doc
                               , "log"
                               , err_msg
                               , 85); // priority
        }
    }
}


void log::check_log(QString filename, watchdog_log_t const & l, QDomElement e)
{
    QDomDocument doc(e.ownerDocument());

    off_t const max_size(l.get_max_size());
    uid_t const uid(l.get_uid());
    gid_t const gid(l.get_gid());
    mode_t const mode(l.get_mode());
    mode_t const mode_mask(l.get_mode_mask());
    struct stat st;
    if(stat(filename.toUtf8().data(), &st) == 0)
    {
        // found at least one log under that directory with that pattern
        //
        f_found = true;

        QDomElement log_tag(doc.createElement("log"));
        e.appendChild(log_tag);

        log_tag.setAttribute("name", l.get_name());
        log_tag.setAttribute("filename", filename);
        log_tag.setAttribute("size", static_cast<qlonglong>(st.st_size));
        log_tag.setAttribute("mode", st.st_mode);
        log_tag.setAttribute("uid", st.st_uid);
        log_tag.setAttribute("gid", st.st_gid);
        log_tag.setAttribute("mtime", static_cast<qlonglong>(st.st_mtime)); // we could look into showing the timespec instead?

        if(st.st_size > max_size)
        {
            // file is too big, generate an error about it!
            //
            QString const err_msg(QString("size of log file  %1 (%2) is %3, which is more than the maximum size of %4")
                                                .arg(l.get_name())
                                                .arg(filename)
                                                .arg(st.st_size)
                                                .arg(max_size));
            log_tag.setAttribute("error", err_msg);

            f_snap->append_error(doc
                               , "log"
                               , err_msg
                               , st.st_size > max_size * 2 ? 73 : 58); // priority
        }

        if(uid != static_cast<uid_t>(-1)
        && uid != st.st_uid)
        {
            // file ownership mismatch
            //
            QString const err_msg(QString("log file owner mismatched for %1 (%2), found %3 expected %4")
                                                .arg(l.get_name())
                                                .arg(filename)
                                                .arg(st.st_uid)
                                                .arg(uid));
            log_tag.setAttribute("error", err_msg);

            f_snap->append_error(doc
                               , "log"
                               , err_msg
                               , 63); // priority
        }

        if(gid != static_cast<gid_t>(-1)
        && gid != st.st_gid)
        {
            // file ownership mismatch
            //
            QString const err_msg(QString("log file group mismatched for %1 (%2), found %3 expected %4")
                                                .arg(l.get_name())
                                                .arg(filename)
                                                .arg(st.st_gid)
                                                .arg(gid));
            log_tag.setAttribute("error", err_msg);

            f_snap->append_error(doc
                               , "log"
                               , err_msg
                               , 63); // priority
        }

        if(mode != 0
        && (st.st_mode & mode_mask) != mode)
        {
            // file ownership mismatch
            //
            QString const err_msg(QString("log file mode mismatched %1 (%2), found %3 expected %4")
                                                .arg(l.get_name())
                                                .arg(filename)
                                                .arg(st.st_mode, 0, 8)
                                                .arg(mode, 0, 8));
            log_tag.setAttribute("error", err_msg);

            f_snap->append_error(doc
                               , "log"
                               , err_msg
                               , 63); // priority
        }
    }
    else
    {
        // file does not exist anymore or we have a permission problem?
    }
}





SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
