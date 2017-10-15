// Snap Websites Server -- to send SIGINT signal to stop a daemon
// Copyright (C) 2011-2017  Made to Order Software Corp.
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
#include "version.h"

// snapwebsites lib
//
#include <snapwebsites/snapwebsites.h>
#include <snapwebsites/snap_config.h>
#include <snapwebsites/not_reached.h>



namespace
{
    const std::vector<std::string> g_configuration_files; // Empty

    advgetopt::getopt::option const g_snapstop_options[] =
    {
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            nullptr,
            nullptr,
            "Usage: %p [-<opt>]",
            advgetopt::getopt::argument_mode_t::help_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            nullptr,
            nullptr,
            "where -<opt> is one or more of:",
            advgetopt::getopt::argument_mode_t::help_argument
        },
        {
            'h',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            "help",
            nullptr,
            "display this help screen.",
            advgetopt::getopt::argument_mode_t::no_argument
        },
        {
            's',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            "service",
            nullptr,
            "PID (only digits) or name of the service to stop.",
            advgetopt::getopt::argument_mode_t::required_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            "timeout",
            "60",
            "number of seconds to wait for the process to die, default is 60 seconds.",
            advgetopt::getopt::argument_mode_t::required_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            "version",
            nullptr,
            "show the version of %p and exit.",
            advgetopt::getopt::argument_mode_t::no_argument
        },
        {
            '\0',
            0,
            nullptr,
            nullptr,
            nullptr,
            advgetopt::getopt::argument_mode_t::end_of_options
        }
    };


}
//namespace




int main(int argc, char *argv[])
{
    try
    {
        advgetopt::getopt opt(argc, argv, g_snapstop_options, g_configuration_files, nullptr);

        if(opt.is_defined("help"))
        {
            opt.usage(advgetopt::getopt::status_t::no_error, "snapstop");
            exit(1);
        }

        if(opt.is_defined("version"))
        {
            std::cout << SNAPCOMMUNICATOR_VERSION_STRING << std::endl;
            exit(0);
        }

        // make sure it is defined
        //
        if(!opt.is_defined("service"))
        {
            std::cerr << "snapstop: error: --service parameter is mandatory." << std::endl;
            exit(1);
        }

        std::string const & service(opt.get_string("service"));
        pid_t service_pid(-1);
        for(char const * s(service.c_str()); *s != '\0'; ++s)
        {
            if(*s < '0' || *s > '9')
            {
                service_pid = -1;
                break;
            }
            if(service_pid == -1)
            {
                // we start with -1 so we have to have a special case...
                //
                service_pid = *s - '0';
            }
            else
            {
                service_pid = service_pid * 10 + *s - '0';
            }
        }

        if(service_pid <= 0)
        {
            if(service_pid == 0)
            {
                std::cerr << "snapstop: error: --service 0 is not valid." << std::endl;
                exit(1);
            }

            // TODO: load PID from a PID file for the named service...
            //
            std::cerr << "snapstop: error: --service <name> not supported yet, this will require us to create a corresponding pid." << std::endl;
            exit(1);
        }

        // verify that we have a process with that PID
        //
        if(kill(service_pid, 0) != 0)
        {
            std::cerr << "snapstop: error: --service " << service_pid << " is not running. Do nothing." << std::endl;
            exit(1);
        }

        // First try with a SIGINT which is a soft interruption; it will
        // not hurt whatever the process is currently doing and as soon as
        // possible it will be asked to stop as if it recieved the STOP
        // command in a message
        //
        {
            int const r(kill(service_pid, SIGINT));
            if(r != 0)
            {
                perror("snapstop: kill() failed: ");
                exit(1);
            }

            // the signal worked worked, now wait for some time for the process to die
            //
            long timeout(opt.get_long("timeout"));
            if(timeout < 10)
            {
                // enforce a minimum of 10 seconds
                //
                timeout = 10;
            }
            else if(timeout > 3600)
            {
                // wait at most 1 hour (wow!)
                //
                timeout = 3600;
            }
            time_t const time_limit(time(nullptr) + timeout);

            // TODO: once we have PID files that are locked by the process
            //       until it dies, we can actually do an flock(), once we
            //       obtain the lock we can quit, the process is dead and
            //       we can use a SIGALRM for the timeout and try SIGTERM.
            //
            do
            {
                // the kill() function returns immediately so we have to
                // sleep otherwise it would loop very quickly...
                //
                // (I do not know of a way to poll() on a dying process
                // unless it is your direct child or we have a lock...)
                //
                sleep(1);

                if(kill(service_pid, 0) != 0)
                {
                    // the process is dead now
                    //
                    exit(0);
                }
            }
            while(time_limit > time(nullptr));
        }

        // the SIGINT did not work, try again with SIGTERM
        //
        // this is not caught and transformed to a soft STOP, so it should
        // nearly never fail to stop the process very quickly...
        //
        // Note: we want to send SIGTERM ourselves because systemd really
        //       only offers two means of shutting down: (1) a signal of
        //       our choice, and (2) the SIGKILL after that;
        //
        //       although SIGTERM kills the process immediately, it still
        //       sends a message to the log file, which makes it useful
        //       for us to see how many times the SIGINT failed
        //
        {
            int const r(kill(service_pid, SIGTERM));
            if(r != 0)
            {
                perror("snapstop: kill() failed: ");
                exit(1);
            }

            // should we have another timeout option for this one?
            //
            // TODO: as with the other one we want to keep trying obtaining
            //       the flock() and have a SIGALRM for the timeout...
            //
            time_t const term_time_limit(time(nullptr) + 10);

            do
            {
                // the kill() function returns immediately so we have to
                // sleep otherwise it would loop very quickly...
                //
                // (I do not know of a way to poll() on a dying process
                // unless it is your direct child or we have a lock...)
                //
                sleep(1);

                if(kill(service_pid, 0) != 0)
                {
                    // the process is dead now
                    //
                    exit(0);
                }
            }
            while(term_time_limit > time(nullptr));
        }

        // it timed out!?
        //
        std::cerr << "snapstop: kill() had no effect within the timeout period." << std::endl;
        return 0;
    }
    catch(std::exception const & e)
    {
        // clean error on exception
        std::cerr << "snapstop: exception: " << e.what() << std::endl;
        return 1;
    }
}

// vim: ts=4 sw=4 et
