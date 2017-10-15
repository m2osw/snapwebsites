// Snap Websites Server -- load bundles from remote system
// Copyright (C) 2016-2017  Made to Order Software Corp.
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
#include "snapmanagerdaemon.h"

// snapwebsites lib
//
#include <snapwebsites/chownnm.h>
#include <snapwebsites/process.h>

// Qt lib
//
#include <QFile>
#include <QDomDocument>

// last entry
//
#include <snapwebsites/poison.h>

namespace snap_manager
{



bundle_loader::bundle_loader()
    : snap_runner("bundle_loader")
{
}


bundle_loader::~bundle_loader()
{
}


void bundle_loader::set_bundle_uri(QString const & bundles_path, std::vector<std::string> const & bundle_uri)
{
    f_bundles_path = bundles_path;
    f_bundle_uri = bundle_uri;
}


/** \brief Load all the bundles and exit.
 *
 * This thread is a worker thread which will load all the available bundle
 * files and then it exits.
 *
 * In order to avoid reloading the same files over and over again, we have
 * their MD5 sums evaluated and checked against the MD5 defined in
 * directory.xml files. If these did not change, then we do not reload
 * that one file.
 *
 * Also, after a successfully read of the entire  set of bundles, the
 * system saves a file named bundles.last-update-time with a Unix
 * timestamp in it. If that file exists, no reads will happen. Delete
 * the file, restart snapmanagerdaemon and you will get a new reload
 * of the bundles.
 */
void bundle_loader::run()
{
    {
        QString const reset_filename(QString("%1/bundles.reset").arg(f_bundles_path));
        QFile last_update_file(reset_filename);
        if(last_update_file.open(QIODevice::ReadOnly))
        {
            // the reset file exists, delete all the files under .../bundles/
            // including the reset file
            //
            std::string cmd("rm -rf ");
            cmd += f_bundles_path.toUtf8().data();
            cmd += "/*";
            int const r(system(cmd.c_str()));
            if(r != 0)
            {
                int const e(errno);
                SNAP_LOG_WARNING("the removal of old bundle files failed, command: \"")(cmd)("\" (errno: ")(e)(", ")(strerror(e))(")");
            }
        }
    }

    QString const date_filename(QString("%1/bundles.last-update-time").arg(f_bundles_path));

    {
        QFile last_update_file(date_filename);
        if(last_update_file.open(QIODevice::ReadOnly))
        {
            time_t last_updated;
            if(last_update_file.read(reinterpret_cast<char *>(&last_updated), sizeof(last_updated)) != sizeof(last_updated))
            {
                SNAP_LOG_WARNING("the bundles.last-update-time file could not be read. Assuming we need to read all the files.");
            }
            else
            {
                // for now, check once a month or so
                //
                // (Note: since the thread exists on a run, if you do not
                // restart your servers for months on end, then no check
                // is done anyway...)
                //
                if(time(nullptr) < last_updated + 86400 * 30)
                {
                    SNAP_LOG_DEBUG("the bundles are considered up to date.");
                    return;
                }
            }
        }
    }

    bool done(true);
    for(auto const & u : f_bundle_uri)
    {
        if(!continue_running())
        {
            done = false;
            break;
        }

        if(!load(u))
        {
            // This one failed, try the next one
            //
        }
    }

    if(done)
    {
        QFile last_update_file(date_filename);
        if(last_update_file.open(QIODevice::WriteOnly))
        {
            time_t const last_updated(time(nullptr));
            if(last_update_file.write(reinterpret_cast<char const *>(&last_updated), sizeof(last_updated)) != sizeof(last_updated))
            {
                SNAP_LOG_ERROR("the bundle_loader could not save the last time it updated the bundles.");
            }
        }
        else
        {
            SNAP_LOG_ERROR("the bundle_loader could not open the last time updated file \"")(date_filename)("\".");
        }
    }

    // delete the bundles.status or the front end will wait a day before it
    // updates this information!
    {
        QString const bundles_status_filename(QString("%1/bundles.status").arg(f_bundles_path));
        unlink(bundles_status_filename.toUtf8().data());
    }
}


/** \brief Load one URI.
 *
 * This function handles one URI by loading the directory and
 * then loading each bundle defined in the directory.
 *
 * \todo
 * Add some kind of security protection such as an MD5 sum of the file
 * so we can make more sure it was not tempered with.
 */
bool bundle_loader::load(std::string const & uri)
{
    if(!wget(uri, "directory.xml"))
    {
        // The load failed
        //
        return false;
    }

    // Read the file as an XML file
    //
    QString const directory_filename(QString("%1/directory.xml").arg(f_bundles_path));
    QDomDocument doc;
    QFile directory_file(directory_filename);
    if(!directory_file.open(QIODevice::ReadOnly))
    {
        // this can happen if the download failed...
        //
        return false;
    }
    if(!doc.setContent(&directory_file, false))
    {
        // this should never happen unless we loaded a partial version
        // of the file (or a hacker was trying to send us invalid data)
        //
        return false;
    }

    // Transform the XML file to a list of URIs and load each one of
    // these files
    //
    QDomNodeList bundles(doc.elementsByTagName("bundle"));
    int const max_bundles(bundles.size());
    for(int idx(0); idx < max_bundles; ++idx)
    {
        QDomElement e(bundles.at( idx ).toElement());
        if(!e.isNull())
        {
            // get the URI for that bundle
            //
            QString const bundle_filename(e.text());

            if(!wget(uri, bundle_filename.toUtf8().data()))
            {
                // The load failed
                //
                return false;
            }
        }
    }

    return true;
}


bool bundle_loader::wget(std::string const & uri, std::string const & filename)
{
    snap::process p("wget");
    p.set_mode(snap::process::mode_t::PROCESS_MODE_COMMAND);
    p.set_command("wget");
    p.add_argument("-a");
    p.add_argument("/var/log/snapwebsites/snapmanager-bundle.log"); // TODO: define a path in snapmanager.conf
    p.add_argument("-q");
    p.add_argument("-O");
    p.add_argument(QString("%1/%2").arg(f_bundles_path).arg(filename.c_str()));
    p.add_argument(QString::fromUtf8((uri + "/" + filename).c_str()));
    int const r(p.run());

    // fix the ownership of the log file--it should not matter unless we
    // want to give access to the website administrator (on the browser side)
    //
    snap::snap_config server_config("snapserver");
    QString username(server_config["user"]);
    if(username.isEmpty())
    {
        username = "snapwebsites";
    }
    QString groupname(server_config["group"]);
    if(groupname.isEmpty())
    {
        groupname = "snapwebsites";
    }
    snap::chownnm("/var/log/snapwebsites/snapmanager-bundle.log", username, groupname);
    if(chmod("/var/log/snapwebsites/snapmanager-bundle.log", 0640) != 0)
    {
        // we continue, this is just a log file, as long as we can write to it
        // we should be good, it's just a tad bit less secure
        //
        int const e(errno);
        SNAP_LOG_WARNING("Could not set mode of \"/var/log/snapwebsites/snapmanager-bundle.log\" to 0640. (errno: ")(e)(", ")(strerror(e));
    }

    if(r != 0)
    {
        SNAP_LOG_ERROR("wget \"")(uri)("\" returned an error (")(r)(").");
        return false;
    }

    return true;
}




} // snap_manager namespace
// vim: ts=4 sw=4 et
