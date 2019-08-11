// Snap Websites Server -- firewall handling by snap
// Copyright (c) 2011-2019  Made to Order Software Corp.  All Rights Reserved
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
#include "version.h"


// snapwebsites lib
//
#include <snapwebsites/log.h>
#include <snapwebsites/process.h>
#include <snapwebsites/snap_cassandra.h>
#include <snapwebsites/snapwebsites.h>


// snapdev lib
//
#include <snapdev/not_used.h>


// Qt lib
//
#include <QtCore>
#include <QtSql>


// last include
//
#include <snapdev/poison.h>






namespace
{


advgetopt::option const g_options[] =
{
    {
        'c',
        advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::GETOPT_FLAG_REQUIRED | advgetopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "config",
        "/etc/fail2ban",
        "Path to the fail2ban.conf configuration file where 'dbfile' is defined.",
        nullptr
    },
    {
        'h',
        advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_FLAG | advgetopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "help",
        nullptr,
        "Show usage and exit.",
        nullptr
    },
    {
        '\0',
        advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_FLAG | advgetopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "version",
        nullptr,
        "Show the version of %p and exit.",
        nullptr
    },
    {
        '\0',
        advgetopt::GETOPT_FLAG_END,
        nullptr,
        nullptr,
        nullptr,
        nullptr
    }
};



// until we have C++20 remove warnings this way
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
advgetopt::options_environment const g_options_environment =
{
    .f_project_name = "snapwebsites",
    .f_options = g_options,
    .f_options_files_directory = nullptr,
    .f_environment_variable_name = "SNAPRESETFAIL2BAN_OPTIONS",
    .f_configuration_files = nullptr,
    .f_configuration_filename = nullptr,
    .f_configuration_directories = nullptr,
    .f_environment_flags = 0,
    .f_help_header = "Usage: %p [-<opt>]\n"
                     "where -<opt> is one or more of:",
    .f_help_footer = nullptr,
    .f_version = SNAPFIREWALL_VERSION_STRING,
    .f_license = nullptr,
    .f_copyright = nullptr,
    //.f_build_date = UTC_BUILD_DATE,
    //.f_build_time = UTC_BUILD_TIME
};
#pragma GCC diagnostic pop




class snap_resetfail2ban
{
public:
                snap_resetfail2ban(int argc, char * argv[]);

    void        run();

private:
    advgetopt::getopt                           f_opt;
    snap::snap_config                           f_config;
    QString                                     f_dbfile = "/var/lib/fail2ban/fail2ban.sqlite3";
};


snap_resetfail2ban::snap_resetfail2ban(int argc, char * argv[])
    : f_opt(g_options_environment, argc, argv)
    , f_config("fail2ban")
{
    if(f_opt.is_defined("help"))
    {
        std::cerr << f_opt.usage();
        exit(1);
        snap::NOTREACHED();
    }

    if(f_opt.is_defined("version"))
    {
        std::cout << SNAPFIREWALL_VERSION_STRING << std::endl;
        exit(0);
        snap::NOTREACHED();
    }

    // read the configuration file
    //
    f_config.set_configuration_path(f_opt.get_string("config"));

    // retrieve the dbfile parameter
    //
    if(f_config.has_parameter("Definition::dbfile"))
    {
        f_dbfile = f_config["Definition::dbfile"];
    }
}


void snap_resetfail2ban::run()
{
    if(getuid() != 0)
    {
        std::cerr << "snapresetfail2ban:error: tool must be run as root." << std::endl;
        exit(1);
    }

    // Stop fail2ban to avoid database conflicts
    {
        std::cout << "stopping fail2ban... " << std::flush;

        snap::process p("stop fail2ban");
        p.set_mode(snap::process::mode_t::PROCESS_MODE_OUTPUT);
        p.set_command("systemctl");
        p.add_argument("stop");
        p.add_argument("fail2ban");
        int const r(p.run());
        if(r != 0)
        {
            int const e(errno);
            QString const output(p.get_output(true));
            std::cerr << "snapresetfail2ban:error: could not stop fail2ban properly. (errno: "
                      << e
                      << " -- "
                      << strerror(e)
                      << ")"
                      << std::endl
                      << "Output:"
                      << std::endl
                      << output
                      << std::endl;
            throw std::runtime_error("snapresetfail2ban could not stop fail2ban");
        }

        std::cout << "stopped" << std::endl;
    }

    // clean the database "bans" table
    {
        std::cout << "deleting fail2ban data... " << std::flush;

        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
        if(!db.isValid())
        {
            const QString error("QSQLITE database is not valid for some reason!");
            std::cerr << "QSQLITE not valid!!!" << std::endl;
            throw std::runtime_error( error.toUtf8().data() );
        }

        db.setDatabaseName(f_dbfile);
        if(!db.open())
        {
            const QString error( QString("Cannot open SQLite database [%1]!").arg(f_dbfile) );
            std::cerr << "QSQLITE not open!!!" << std::endl;
            throw std::runtime_error( error.toUtf8().data() );
        }

        db.transaction();

        QSqlQuery q;
        q.prepare(QString("DELETE FROM bans;"));
        if(!q.exec())
        {
            std::cerr << "lastQuery=[" << q.lastQuery() << "]" << std::endl;
            std::cerr << "query error=[" << q.lastError().text() << "]" << std::endl;
            throw std::runtime_error(q.lastError().text().toUtf8().data());
        }

        db.commit();
    }

    // Restart fail2ban now that the database was reset
    {
        std::cout << "starting fail2ban... " << std::flush;

        snap::process p("start fail2ban");
        p.set_mode(snap::process::mode_t::PROCESS_MODE_OUTPUT);
        p.set_command("systemctl");
        p.add_argument("start");
        p.add_argument("fail2ban");
        int const r(p.run());
        if(r != 0)
        {
            int const e(errno);
            QString const output(p.get_output(true));
            std::cerr << "snapresetfail2ban:error: could not restart fail2ban properly after database reset. (errno: "
                      << e
                      << " -- "
                      << strerror(e)
                      << ")"
                      << std::endl
                      << "Output:"
                      << std::endl
                      << output
                      << std::endl;
            throw std::runtime_error("snapresetfail2ban could not start fail2ban");
        }

        std::cout << "started" << std::endl;
    }

}






} // no name namespace


int main(int argc, char * argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName   ( "snapresetfail2ban"         );
    app.setApplicationVersion( SNAPFIREWALL_VERSION_STRING );
    app.setOrganizationDomain( "snapwebsites.org"          );
    app.setOrganizationName  ( "M2OSW"                     );

    try
    {
        // create an instance of the snap_resetfail2ban object
        //
        snap_resetfail2ban resetf2b( argc, argv );

        // Now run!
        //
        resetf2b.run();

        // exit normally (i.e. we received a STOP message on our
        // connection with the Snap! Communicator service.)
        //
        return 0;
    }
    catch( snap::snap_exception const & e )
    {
        SNAP_LOG_FATAL("snap_exception caught! ")(e.what());
    }
    catch( std::invalid_argument const & e )
    {
        SNAP_LOG_FATAL("invalid argument: ")(e.what());
    }
    catch( std::exception const & e )
    {
        SNAP_LOG_FATAL("std::exception caught! ")(e.what());
    }
    catch( ... )
    {
        SNAP_LOG_FATAL("unknown exception caught!");
    }

    return 1;
}


// vim: ts=4 sw=4 et
