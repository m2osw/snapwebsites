// Snap Websites Server -- run apt-get to upgrade a computer
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

// our lib
//
#include "snapmanager/manager.h"

// snapwebsites lib
//
#include <snapwebsites/log.h>
#include <snapwebsites/lockfile.h>
#include <snapwebsites/process.h>

// last entry
//
#include <snapwebsites/poison.h>


namespace snap
{

// definitions from the plugins so we can define the name and filename of
// the server plugin
namespace plugins
{

extern QString g_next_register_name;
extern QString g_next_register_filename;

} // plugin namespace

} // snap namespace



bool upgrade(snap_manager::manager::pointer_t upgrader)
{
    SNAP_LOG_INFO("snapupgrader started the upgrade process.");

    // TODO: move this command to the installer instead?
    //
    // make sure we are in a relatively sane state in case some
    // configuration failed/did not occur on a prior upgrade
    //
    {
        snap::process p("configure pending");
        p.set_mode(snap::process::mode_t::PROCESS_MODE_OUTPUT);
        p.set_command("dpkg");
        p.add_argument("--configure");
        p.add_argument("--pending");
        p.add_argument("--force-confold");
        p.add_environ("DEBIAN_FRONTEND", "noninteractive");
        int const r(p.run());

        // the output is saved so we can send it to the user and log it...
        QString const output(p.get_output(true));
        SNAP_LOG_INFO("dpkg --configure --pending returned:\n")(output);

        if(r != 0)
        {
            SNAP_LOG_ERROR("\"dpkg --configure --pending --force-confold\" failed.");
            return false;
        }
    }

    if(upgrader->update_packages("update") != 0)
    {
        // at times, the update fails because some old configuration
        // failed earlier, tries to fix the problem--this is likely
        // to fix most common problems and let us upgrade the computer;
        // if that fails, the user is on his own!
        //
        // TODO: actually we really need to have one process running at
        //       a time, be it for the status gathering, or the upgrader
        //       or the installer... right now we do not have good
        //       synchronization so things can break in between...
        //
        sleep(10);
        if(upgrader->update_packages("update") != 0)
        {
            SNAP_LOG_ERROR("\"apt-get update\" failed.");
            return false;
        }
    }

    if(upgrader->update_packages("upgrade") != 0)
    {
        SNAP_LOG_ERROR("\"apt-get upgrade\" failed.");
        return false;
    }

    if(upgrader->update_packages("dist-upgrade") != 0)
    {
        SNAP_LOG_ERROR("\"apt-get dist-upgrade\" failed.");
        return false;
    }

    if(upgrader->update_packages("autoremove") != 0)
    {
        SNAP_LOG_ERROR("\"apt-get autoremove\" failed.");
        return false;
    }

    return true;
}


int main(int argc, char * argv[])
{
    try
    {
        // we need these globals to "properly" initializes the first
        // "plugin" (the core system or server) even though the
        // upgrader does not make use of them at all
        //
        snap::plugins::g_next_register_name = "server";
        snap::plugins::g_next_register_filename = "snapmanagercgi.cpp";

        snap_manager::manager::pointer_t upgrader(new snap_manager::manager(true));

        snap::plugins::g_next_register_name.clear();
        snap::plugins::g_next_register_filename.clear();

        upgrader->init(argc, argv);

        // mark that we started properly now that the logger is on
        //
        SNAP_LOG_INFO("--------------------------------- snapupgrader v" SNAPMANAGERCGI_VERSION_STRING " started on ")(upgrader->get_server_name());

        // detach from the parent now, this allows for --version and
        // --help to work as expected (i.e. before the detach)
        //
        pid_t const child_pid(fork());
        if(child_pid != 0)
        {
            if(child_pid == -1)
            {
                int const e(errno);
                SNAP_LOG_ERROR("snapupgrader failed to detach itself (")(e)(", ")(strerror(e))(").");
                return 1;
            }
            // snapupgrader detached itself, return 0
            return 0;
        }

        // leave my parents session
        //
        setsid();

        // TODO: add support for handlers too
        //

        // ignore HUP signals
        //
        signal(SIGHUP, SIG_IGN);

        // always reconfigure the logger in the child
        //
        snap::logging::reconfigure();

        // make sure we do not start an upgrade while an installation is
        // still going (and vice versa)
        //
        snap::lockfile lf(upgrader->lock_filename(), snap::lockfile::mode_t::LOCKFILE_EXCLUSIVE);
        if(lf.try_lock())
        {
            bool const r(upgrade(upgrader));

            // things are likely changed, make sure to reset the apt-check counters
            //
            // Note: we should also automatically receive a DPKGUPDATE message
            //
            upgrader->reset_aptcheck();

            if(r)
            {
                return 0;
            }
        }
        else
        {
            SNAP_LOG_ERROR("snapupgrader could not lock the upgrading.lock file.");
        }
    }
    catch(std::runtime_error const & e)
    {
        // this should rarely happen!
        std::cerr << "snapupgrader: caught a runtime exception: " << e.what() << std::endl;
    }
    catch(std::logic_error const & e)
    {
        // this should never happen!
        std::cerr << "snapupgrader: caught a logic exception: " << e.what() << std::endl;
    }
    catch(std::exception const & e)
    {
        // we are in trouble, we cannot even answer!
        std::cerr << "snapupgrader: standard exception: " << e.what() << std::endl;
    }
    catch(...)
    {
        std::cerr << "snapupgrader: caught an unknown exception.";
    }

    return 1;

/*
bool manager::upgrader()
{
    // make sure we do not start an upgrade while an installation is
    // still going (and vice versa)
    //
    snap::lockfile lf(lock_filename(), snap::lockfile::mode_t::LOCKFILE_EXCLUSIVE);
    if(!lf.try_lock())
    {
        return false;
    }

    // detach ourselves from our parent so the upgrader does not die
    // even if it upgrades snapinit
    //
    pid_t const pid(fork());
    if(pid != 0)
    {
        // we are the parent, just stay around as normal
        //
        return true;
    }

    // TODO: apply some fixes for the logger

    // always reconfigure the logger in the child
    snap::logging::reconfigure();

    // if the parent dies, then we generally receive a SIGHUP which is
    // a big problem if we want to go on with the update... so here I
    // make sure we ignore the HUP signal.
    //
    // (this should really only happen if you have a terminal attached
    // to the process, but just in case...)
    //
    signal(SIGHUP, SIG_IGN);

    // setsid() moves us in the group instead of being viewed as a child
    //
    setsid();

    //pid_t const sub_pid(fork());
    //if(sub_pid != 0)
    //{
    //    // we are the sub-parent, by leaving now the child has
    //    // a parent PID equal to 1 meaning that it cannot be
    //    // killed just because snapmanagerdaemon gets killed
    //    //
    //    exit(0);
    //}

    bool const success(update_packages("update")       == 0
                    && update_packages("upgrade")      == 0
                    && update_packages("dist-upgrade") == 0
                    && update_packages("autoremove")   == 0);

    // we have to do this one here now
    //
    reset_aptcheck();

    exit(success ? 0 : 1);

    snap::NOTREACHED();
    return true;
}
*/

}

// vim: ts=4 sw=4 et
