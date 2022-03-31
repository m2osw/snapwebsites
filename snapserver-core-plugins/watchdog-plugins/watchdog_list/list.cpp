// Snap Websites Server -- List watchdog: make sure the list processes work.
// Copyright (c) 2018-2019  Made to Order Software Corp.  All Rights Reserved
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
#include "watchdog_list/list.h"


// snapwebsites lib
//
#include <snapwebsites/glob_dir.h>
#include <snapwebsites/qdomhelpers.h>
#include <snapwebsites/process.h>


// snaplogger lib
//
#include <snaplogger/message.h>


// snapdev lib
//
#include <snapdev/not_used.h>


// Qt lib
//
#include <QFile>
#include <QtSql>


// C lib
//
#include <grp.h>
#include <pwd.h>
#include <unistd.h>
#include <sys/stat.h>


// last include
//
#include <snapdev/poison.h>



SNAP_PLUGIN_START(list, 1, 0)


/** \brief Get a fixed list plugin name.
 *
 * The list plugin makes use of different names in the database. This
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
    case name_t::SNAP_NAME_WATCHDOG_LIST_NAME:
        return "name";

    default:
        // invalid index
        throw snap_logic_exception("Invalid SNAP_NAME_WATCHDOG_LIST_...");

    }
    snapdev::NOT_REACHED();
}




/** \brief Initialize the list plugin.
 *
 * This function is used to initialize the list plugin object.
 */
list::list()
{
}


/** \brief Clean up the list plugin.
 *
 * Ensure the list object is clean before it is gone.
 */
list::~list()
{
}


/** \brief Get a pointer to the list plugin.
 *
 * This function returns an instance pointer to the list plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the list plugin.
 */
list * list::instance()
{
    return g_plugin_list_factory.instance();
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
QString list::description() const
{
    return "Check that the list process is working. That process uses many"
          " parts so it is fairly easy for one part to break and the whole"
          " process to fail. This watchdog verifies that the local list"
          " journal gets processed (files under"
          " /var/lib/snapwebsites/list/journal/... don't stick around for"
          " more than a day.) When such files exist, it also verifies that"
          " their permission and owner/group are properly set."
          " When installed on a computer that is to run the pagelist backend"
          " it also verifies that the MySQL journal table gets worked on as"
          " expected (i.e. that the number of rows changes and URL inside"
          " those rows change too.)"
          ;
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString list::dependencies() const
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
int64_t list::do_update(int64_t last_updated)
{
    snapdev::NOT_USED(last_updated);
    SNAP_PLUGIN_UPDATE_INIT();
    // no updating in watchdog
    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Initialize list.
 *
 * This function terminates the initialization of the list plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void list::bootstrap(snap_child * snap)
{
    f_snap = static_cast<watchdog_child *>(snap);

    SNAP_LISTEN(list, "server", watchdog_server, process_watch, boost::placeholders::_1);
}


/** \brief Process this watchdog data.
 *
 * This function runs this watchdog.
 *
 * \param[in] doc  The document.
 */
void list::on_process_watch(QDomDocument doc)
{
    SNAP_LOG_DEBUG("list::on_process_watch(): processing");

    QDomElement parent(snap_dom::create_element(doc, "watchdog"));
    QDomElement e(snap_dom::create_element(parent, "list"));

    local_journal(e);
    snaplist_database(e);
}


void list::local_journal(QDomElement e)
{
    snap_config server_config("snapserver");

    // try the most specific path first
    //
    QString path(server_config[snap::get_name(snap::name_t::SNAP_NAME_CORE_LIST_DATA_PATH)]);

    if(path.isEmpty())
    {
        // if the most specific is not defined, then maybe the basic
        // data_path is, we need to add "list" at the end, though
        //
        path = server_config[snap::get_name(snap::name_t::SNAP_NAME_CORE_DATA_PATH)];
        if(path.isEmpty())
        {
            path = "/var/lib/snapwebsites/list";
        }
        else
        {
            // get_data_path() never returns an empty path so no need to test
            //
            path += "/list";
        }
    }

    path += "/";
    path += snap::get_name(snap::name_t::SNAP_NAME_CORE_LIST_JOURNAL_PATH);

    // get the expected user and group names
    //
    f_username = "snapwebsites";
    f_groupname = "snapwebsites";
    QString const user_group(f_snap->get_server_parameter(snap::watchdog::get_name(snap::watchdog::name_t::SNAP_NAME_WATCHDOG_USER_GROUP)));
    if(!user_group.isEmpty())
    {
        snap_string_list ug_list(user_group.split(':'));
        if(ug_list.size() == 1)
        {
            ug_list = user_group.split('.');
        }
        f_username = ug_list[0];
        if(ug_list.size() > 1)
        {
            f_groupname = ug_list[1];
        }
    }

    struct passwd const * pwd(getpwnam(f_username.toUtf8().data()));
    if(pwd != nullptr)
    {
        f_uid = pwd->pw_uid;
    }
    else
    {
        SNAP_LOG_WARNING("could not find user \"")(f_username)("\" on this computer; user ownership won't be tested.");
    }

    struct group const * grp(getgrnam(f_groupname.toUtf8().data()));
    if(grp != nullptr)
    {
        f_gid = grp->gr_gid;
    }
    else
    {
        SNAP_LOG_WARNING("could not find group \"")(f_groupname)("\" on this computer; group onwership won't be tested.");
    }

    glob_dir const journal_filenames(path + "/*", GLOB_NOSORT | GLOB_NOESCAPE, true);
    journal_filenames.enumerate_glob(std::bind(&list::count_files, this, std::placeholders::_1, e));

    if(f_count > 2)
    {
        // 3 or more files means that we have a problem
        // We use one file per day and then they get deleted quickly
        //
        QString const msg("more than two journal files found, unless there is a problem, this should never happen");
        f_snap->append_error(e.ownerDocument(), "list", msg, 90);
    }
}


void list::count_files(QString const & filename, QDomElement e)
{
    QDomElement journal_tag(e.ownerDocument().createElement("journal"));
    e.appendChild(journal_tag);

    journal_tag.setAttribute("filename", filename);

    // count the number of files found
    //
    ++f_count;
    journal_tag.setAttribute("jid", f_count);

    snap_string_list err_msg;

    // check that the file does not have other W or X permissions
    //
    struct stat st;
    if(stat(filename.toUtf8().data(), &st) == 0)
    {
        journal_tag.setAttribute("size", static_cast<qlonglong>(st.st_size));
        journal_tag.setAttribute("uid", st.st_uid);
        journal_tag.setAttribute("gid", st.st_gid);
        journal_tag.setAttribute("mode", st.st_mode);
        journal_tag.setAttribute("mtime", static_cast<int>(st.st_mtime));

        if((st.st_mode & S_IWOTH) != 0)
        {
            QString const msg("the Other Write Permission (-------w-) is set on this file when it should not");
            err_msg << msg;
            f_snap->append_error(e.ownerDocument(), "list", msg, 60);
        }

        if((st.st_mode & (S_IXUSR | S_IXGRP | S_IWOTH)) != 0)
        {
            QString const msg("one or more of the Execution Permissions (--x--x--x) are set on this file when it should not");
            err_msg << msg;
            f_snap->append_error(e.ownerDocument(), "list", msg, 40);
        }

        if(f_uid != static_cast<uid_t>(-1)
        && st.st_uid != f_uid)
        {
            QString const msg(QString("the user ownership of this file is not \"%1\" as expected")
                                    .arg(f_username));
            err_msg << msg;
            f_snap->append_error(e.ownerDocument(), "list", msg, 73);
        }

        if(f_gid != static_cast<gid_t>(-1)
        && st.st_gid != f_gid)
        {
            QString const msg(QString("the group ownership of this file is not \"%1\" as expected")
                                    .arg(f_groupname));
            err_msg << msg;
            f_snap->append_error(e.ownerDocument(), "list", msg, 73);
        }

        // 12 Mb, it should really never grow that big ever
        // (a single front end should never have so many requests in a day)
        //
        if(st.st_size > 12 * 1024 * 1024)
        {
            QString const msg(QString("file is %1 bytes which is more than 12Mb").arg(st.st_size));
            err_msg << msg;
            f_snap->append_error(e.ownerDocument(), "list", msg, 60);
        }
    }
    else
    {
        // the file just got deleted?
        //
        journal_tag.setAttribute("warning", "could not stat() this file, just got removed?");
    }

    if(!err_msg.isEmpty())
    {
        journal_tag.setAttribute("error", err_msg.join("; "));
    }
}


void list::snaplist_database(QDomElement e)
{
    // if the computer does not run snaplistd, then we don't want to run
    // this test (because the MySQL database is not running here)
    //
    struct stat st;
    if(stat("/usr/sbin/snaplistd", &st) != 0 && 0)
    {
        // snaplistd is not even installed
        //
        SNAP_LOG_TRACE("/usr/sbin/snaplistd not found");
        return;
    }
    if(stat("/usr/bin/mysql", &st) != 0)
    {
        // we also need mysql to be present
        // (it comes along snaplistd so this one should never happen)
        //
        SNAP_LOG_TRACE("/usr/bin/mysql not found");
        return;
    }

    // now check that the snaplistd daemon is running
    // if not this is not an error here, we just ignore this check
    // (the processes will generate an error if necessary)
    //
    process_list plist;

    plist.set_field(process_list::field_t::COMMAND_LINE);
    //list.set_field(process_list::field_t::STATISTICS);
    for(;;)
    {
        process_list::proc_info::pointer_t info(plist.next());
        if(!info)
        {
            // snaplistd is not running at the moment
            //
            return;
        }

        // get basename
        //
        std::string name(info->get_process_name());
        std::string::size_type p(name.find_last_of('/'));
        if(p != std::string::npos)
        {
            name = name.substr(p + 1);
        }
        if(name == "snaplistd")
        {
            // found snaplistd server, move forward
            //
            break;
        }
    }

    // the snaplist database check uses a file with:
    //
    //   . Unix timestamp when we last ran the CHECKSUM TABLE command
    //   . Last result: 0 success, 1 checksum did not change for 1 day
    //   . Last MySQL checksum in hex or empty if the table is empty
    //
    // note that if the table remains empty when we run our test, then
    // everything is considered to be in order
    //
    QString cache_path(f_snap->get_server_parameter(snap::watchdog::get_name(snap::watchdog::name_t::SNAP_NAME_WATCHDOG_CACHE_PATH)));
    if(cache_path.isEmpty())
    {
        cache_path = "/var/cache/snapwebsites/snapwatchdog";
    }
    QString const snaplist_database_filename(cache_path + "/snaplist_database_last_check.txt");

    QDomElement journal_tag(e.ownerDocument().createElement("journal-checksum"));
    e.appendChild(journal_tag);

    QString err_msg;
    int priority(0);

    // if false, the MD5 is not available so we just create a new file
    // and do not generate any errors
    //
    bool has_old_checksum(false);
    bool has_new_checksum(false);
    QString old_checksum;
    QString new_checksum;

    QFile file(snaplist_database_filename);
    if(file.exists())
    {
        // if the file exists we may have tested the table not too long
        // ago, we want to test once per day only
        //
        if(file.open(QIODevice::ReadOnly))
        {
            char buf[1024];
            qint64 sz(file.readLine(buf, sizeof(buf) - 1));
            if(sz == -1
            || sz >= static_cast<qint64>(sizeof(buf) - 1))
            {
                file.remove();
            }
            else
            {
                // the first line is the time when it was saved
                //
                buf[sizeof(buf) - 1] = '\0';
                bool ok(false);
                time_t const last_check(QString::fromUtf8(buf).toLongLong(&ok, 10));
                if(!ok)
                {
                    file.remove();
                }
                else
                {
                    time_t const now(time(nullptr));
                    if(now < last_check + 86400)
                    {
                        // skip that check, it's too expensive to do all the
                        // time and we last tried 1 day ago
                        //
                        // TODO: make the 86400 a parameeter one can enter
                        //       in some .conf (but which .conf in this
                        //       case?)
                        //

                        // read line 2 with the previous result
                        //
                        sz = file.readLine(buf, sizeof(buf) - 1);
                        if(sz == -1
                        || sz >= static_cast<qint64>(sizeof(buf) - 1))
                        {
                            // we could not read the file correctly
                            //
                            file.remove();
                        }
                        else
                        {
                            buf[sizeof(buf) - 1] = '\0';
                            int const last_error(QString::fromUtf8(buf).toLongLong(&ok, 10));
                            if(last_error != 1)
                            {
                                // not in error, leave it
                                // we're done here
                                //
                                return;
                            }

                            // we're in error (no changes for 1 whole day
                            // and the list table was not empty)
                            //
                            err_msg = "the database is not empty and it did not change for at least one whole day (repeat)";
                            priority = 76;
                        }
                    }
                    else
                    {
                        // check timed out, we need to re-run it now
                        //
                        sz = file.readLine(buf, sizeof(buf) - 1);
                        if(sz == -1
                        || sz >= static_cast<qint64>(sizeof(buf) - 1))
                        {
                            // we could not read the file correctly
                            //
                            file.remove();
                        }
                        else
                        {
                            // ignore the error status, we're going to re-check

                            sz = file.readLine(buf, sizeof(buf) - 1);
                            if(sz == -1
                            || sz >= static_cast<qint64>(sizeof(buf) - 1))
                            {
                                // we could not read the file correctly
                                //
                                file.remove();
                            }
                            else
                            {
                                buf[sizeof(buf) - 1] = '\0';
                                old_checksum = QString::fromUtf8(buf);
                                has_old_checksum = true;
                            }
                        }
                    }
                }
            }

            // Note: if the file was removed it's already closed, it shouldn't
            // hurt to re-close() th same file.
            //
            file.close();
        }
    }

    // run the check unless we already got an error
    //
    if(err_msg.isEmpty())
    {
        // run the checksum on the journal table
        //
        for(;;)
        {
            try
            {
                SNAP_LOG_TRACE("Attempting to connect to MySQL database to run a table CHECKSUM");

#if 1
                // we run the following command; we could setup a .mylogin.cnf
                // but we may want to run other commands against other
                // database which would require different users
                //
                // mysql -u snaplist -psnaplist -sre 'CHECKSUM TABLE snaplist.journal' snaplist
                //
                process mysql("run CHECKSUM TABLE command line");
                mysql.set_mode(process::mode_t::PROCESS_MODE_OUTPUT);
                mysql.set_command("mysql");
                mysql.add_argument("-u");
                mysql.add_argument("snaplist");
                mysql.add_argument("-psnaplist");
                mysql.add_argument("-sre");
                mysql.add_argument("'CHECKSUM TABLE snaplist.journal'");
                mysql.add_argument("snaplist");

                int const r(mysql.run());
                if(r == 0)
                {
                    // the output is two lines, the first is the names of the
                    // columns and the second is the name of the table and
                    // its checksum
                    //
                    QString const output(mysql.get_output(true));
                    snap_string_list const rows(output.trimmed().split('\n'));
                    if(rows.size() == 1)
                    {
                        snap_string_list const columns(rows[0].split('\t'));
                        if(columns.size() == 2)
                        {
                            snapdev::NOT_USED(columns[1].toLongLong(&has_new_checksum, 10));
                            if(!has_new_checksum)
                            {
                                err_msg = "could not convert the column checksum to a number";
                                priority = 5;
                                break;
                            }
                            new_checksum = columns[1];
                        }
                        else
                        {
                            err_msg = "invalid number of columns in CHECKSUM TABLE output";
                            priority = 5;
                            break;
                        }
                    }
                    else
                    {
                        err_msg = "invalid number of rows in CHECKSUM TABLE output";
                        priority = 5;
                        break;
                    }
                }
                else
                {
                    err_msg = QString("got an error (exit code: %1) when running CHECKSUM TABLE output")
                                        .arg(r);
                    priority = 9;
                    break;
                }
#else
// the following is not working, somehow the q.next() returns false
// also the isSelect() should be returning true, but it's not
// so instead at this time we use a pipe amd the mysql client
//
// note: it may be because we have MyISAM tables and not InnoDB, the
//       InnoDB have a CHECKSUM field that is kept up to date and
//       so the result may work better in that situation
//
// See also: https://bugreports.qt.io/browse/QTBUG-9648?gerritIssueStatus=All
//
                QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL");
                if(!db.isValid())
                {
                    err_msg = "QMYSQL database is not valid for some reason!";
                    priority = 5;
                    break;
                }

                if(QSqlDatabase::database().isOpen())
                {
                    QSqlDatabase::database().close();
                }

                db.setHostName     ( "localhost" );
                db.setUserName     ( "snaplist" );
                db.setPassword     ( "snaplist" );
                db.setDatabaseName ( "snaplist" );
                if(!db.open())
                {
                    err_msg = "could not open MySQL database snaplist for table CHECKSUM";
                    priority = 5;
                    break;
                }

                QSqlQuery q(db);
                q.prepare("CHECKSUM TABLE snaplist.journal QUICK");
                if(!q.exec())
                {
                    err_msg = QString("the CHECKSUM TABLE command returned an error: %1 (Exact query: \"%2\")")
                                        .arg(q.lastError().text())
                                        .arg(q.lastQuery());
                    priority = 5;
                    break;
                }

SNAP_LOG_WARNING("query with QUICK is : ")(q.isSelect() ? "isSelect()!" : "not a select like thing?");

                // assuming it worked, the q.next() should read the
                // result that we can then grab
                //
                if(!q.next())
                {
                    err_msg = "could not read the CHECKSUM TABLE results";
                    priority = 5;
                    break;
                }

                int const checksum_field_no(q.record().indexOf("Checksum"));
                QVariant const value(q.value(checksum_field_no));
                if(value.isNull())
                {
                    err_msg = "the CHECKSUM TABLE command returned NULL (table missing?)";
                    priority = 51;
                }
                else
                {
                    int64_t const current_checksum(value.toLongLong());
                    new_checksum = QString("%1").arg(current_checksum);
                    has_new_checksum = true;

                }
#endif
                // 'checksum == 0' when the table is empty
                //
                if(has_new_checksum
                && new_checksum != "0"
                && has_old_checksum)
                {
                    if(new_checksum == old_checksum)
                    {
                        // this is the problem! The checksum did
                        // not change after more than a day
                        //
                        // priority is really high when it first happens,
                        // then it will drop to 76 for the following minutes
                        // until tomorrow when it either stops or restarts
                        //
                        err_msg = "the CHECKSUM TABLE has not changed in 24 hours";
                        priority = 92;
                    }
                }
            }
            catch(std::exception const & ex)
            {
                // something generated an error
                //
                err_msg = (std::string("Could not connect to MySQL database: retrying... (") + ex.what() + ")").c_str();
            }
            break;
        }
    }

    if(has_new_checksum)
    {
        if(file.open(QIODevice::WriteOnly))
        {
            // first line is 'now'
            //
            time_t const now(time(nullptr));
            QByteArray buf(QString("%1\n").arg(now).toUtf8());
            file.write(buf);

            // second line is 0, no error, or 1, emitted an error because
            // the database did not change (and the checksum was not 0
            //
            // TBD: we may want to record any error and repeat it instead?
            //      I think that most of the other errors are going to
            //      be repeated automatically because that's something
            //      wrong with the file or such
            //
            buf = QString("%1\n").arg(priority >= 90 ? 1 : 0).toUtf8();
            file.write(buf);

            // third line is the current checsum
            //
            buf = new_checksum.toUtf8();
            file.write(buf);
            file.write("\n", 1);

            file.close();
        }
    }

    if(!err_msg.isEmpty())
    {
        journal_tag.setAttribute("error", err_msg);
        f_snap->append_error(e.ownerDocument(), "list", err_msg, priority);
    }
}




SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
