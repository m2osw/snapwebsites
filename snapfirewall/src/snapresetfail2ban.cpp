// Snap Websites Server -- firewall handling by snap
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
#include <snapwebsites/log.h>
#include <snapwebsites/not_used.h>
#include <snapwebsites/process.h>
#include <snapwebsites/snap_cassandra.h>
#include <snapwebsites/snapwebsites.h>

// Qt lib
//
#include <QtCore>
#include <QtSql>


namespace
{


std::vector<std::string> const g_configuration_files; // Empty

advgetopt::getopt::option const g_snapresetfail2ban_options[] =
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
        'c',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "config",
        "/etc/fail2ban",
        "Path to the fail2ban.conf configuration file where 'dbfile' is defined.",
        advgetopt::getopt::argument_mode_t::optional_argument
    },
    {
        'h',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "help",
        nullptr,
        "Show usage and exit.",
        advgetopt::getopt::argument_mode_t::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "version",
        nullptr,
        "Show the version of %p and exit.",
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
    : f_opt(argc, argv, g_snapresetfail2ban_options, g_configuration_files, "SNAPRESETFAIL2BAN_OPTIONS")
    , f_config("fail2ban")
{
    if(f_opt.is_defined("help"))
    {
        f_opt.usage( advgetopt::getopt::status_t::no_error, "snapresetfail2ban" );
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
    app.setApplicationName   ( "snapbackup"                );
    app.setApplicationVersion( SNAPFIREWALL_VERSION_STRING );
    app.setOrganizationDomain( "snapwebsites.org"          );
    app.setOrganizationName  ( "M2OSW"                     );

    try
    {
        // create an instance of the snap_firewall object
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
        SNAP_LOG_FATAL("snapfirewall: snap_exception caught! ")(e.what());
    }
    catch( std::invalid_argument const & e )
    {
        SNAP_LOG_FATAL("snapfirewall: invalid argument: ")(e.what());
    }
    catch( std::exception const & e )
    {
        SNAP_LOG_FATAL("snapfirewall: std::exception caught! ")(e.what());
    }
    catch( ... )
    {
        SNAP_LOG_FATAL("snapfirewall: unknown exception caught!");
    }

    return 1;
}


// vim: ts=4 sw=4 et
