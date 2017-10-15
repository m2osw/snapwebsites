// Snap Websites Server -- create the snap websites tables
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

#include <snapwebsites/log.h>
#include <snapwebsites/not_used.h>
#include <snapwebsites/snap_cassandra.h>
#include <snapwebsites/snapwebsites.h>




/** \brief Send a CASSANDRAREADY message to all listeners.
 *
 * After we created all the tables, give various daemons another chance
 * to check for the viability of Cassandra.
 *
 * This function broadcasts a CASSANDRAREADY message. The message never
 * gets cached.
 *
 * This is an equivalent to:
 *
 * \code
 *     snapsignal "*&#x2F;CASSANDRAREADY cache=no"
 * \endcode
 */
void send_cassandra_ready()
{
    snap::snap_communicator_message cassandra_ready;
    cassandra_ready.set_command("CASSANDRAREADY");
    cassandra_ready.set_service("*");
    cassandra_ready.add_parameter("cache", "no");

    // TBD: we may want to cache that information in case we call
    //      this function more than once
    //
    QString addr("127.0.0.1");
    int port(4041);
    snap::snap_config config("snapcommunicator");
    QString const communicator_addr_port(config["signal"]);
    tcp_client_server::get_addr_port(communicator_addr_port, addr, port, "udp");

    snap::snap_communicator::snap_udp_server_message_connection::send_message(addr.toUtf8().data(), port, cassandra_ready);
}




int main(int argc, char * argv[])
{
    snap::NOTUSED(argc);

    try
    {
        // TODO: get a function in the library so we can support a common
        //       way to setup the logger (and always support the various
        //       command line options, the logging server, etc.)
        //
        snap::logging::set_progname(argv[0]);
        if(isatty(STDERR_FILENO))
        {
            snap::logging::configure_console();
        }
        else
        {
            // as a background process use the snapserver setup
            // (it is always available because it is in snapbase)
            //
            snap::snap_config config("snapserver");
            QString const log_config(config["log_config"]);
            if(log_config.isEmpty())
            {
                snap::logging::configure_console();
            }
            else
            {
                snap::logging::configure_conffile(log_config);
            }
        }

        snap::snap_cassandra cassandra;
        cassandra.connect();

        // Create all the missing tables from all the plugins which
        // packages are currently installed
        //
        cassandra.create_table_list();

        // the tables were created, send a CASSANDRAREADY message to wake
        // up any daemon that was expecting such and checked for said
        // table(s) too soon.
        //
        send_cassandra_ready();
    }
    catch(std::exception const & e)
    {
        std::cerr << "error: an exception was raised: \"" << e.what() << "\"" << std::endl;
        exit(1);
    }
    catch(...)
    {
        std::cerr << "error: an unknown exception was raised." << std::endl;
        exit(1);
    }

    return 0;
}


// vim: ts=4 sw=4 et
