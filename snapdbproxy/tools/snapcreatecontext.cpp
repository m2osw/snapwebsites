// Snap Websites Server -- create the snap_websites context
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
#include <snapwebsites/snap_config.h>
#include <snapwebsites/snapwebsites.h>

#include <casswrapper/query.h>




int main(int argc, char * argv[])
{
    snap::NOTUSED(argc);

    try
    {
        snap::logging::set_progname(argv[0]);
        snap::logging::configure_console();

        QString const context_name(snap::get_name(snap::name_t::SNAP_NAME_CONTEXT));

        // initialize the reading of the configuration file
        //
        snap::snap_config config("snapdbproxy");

        // get the list of Cassandra hosts, "127.0.0.1" by default
        //
        QString cassandra_host_list("127.0.0.1");
        if(config.has_parameter("cassandra_host_list"))
        {
            cassandra_host_list = config[ "cassandra_host_list" ];
            if(cassandra_host_list.isEmpty())
            {
                throw snap::snapwebsites_exception_invalid_parameters("cassandra_host_list cannot be empty.");
            }
        }

        // get the Cassandra port, 9042 by default
        //
        int cassandra_port(9042);
        if(config.has_parameter("cassandra_port"))
        {
            std::size_t pos(0);
            std::string const port(config["cassandra_port"]);
            cassandra_port = std::stoi(port, &pos, 10);
            if(pos != port.length()
            || cassandra_port < 0
            || cassandra_port > 65535)
            {
                throw snap::snapwebsites_exception_invalid_parameters("cassandra_port to connect to Cassandra must be defined between 0 and 65535.");
            }
        }

        // create a new Cassandra session
        //
        auto session( casswrapper::Session::create() );

        // increase the request timeout "dramatically" because creating a
        // context is very slow
        //
        // note: we do not make use of the QCassandraRequestTimeout class
        //       because we will just create the context and be done with it
        //       so there is no real need for us to restore the timeout
        //       at a later time
        //
        session->setTimeout(5 * 60 * 1000); // timeout = 5 min.

        // connect to the Cassandra cluster
        //
        session->connect( cassandra_host_list, cassandra_port, config["cassandra_use_ssl"] == "true" ); // throws on failure!
        if(!session->isConnected())
        {
            // this error should not ever appear since the connect()
            // function throws on errors, but for completeness...
            //
            std::cerr << "error: could not connect to Cassandra cluster." << std::endl;
            exit(1);
        }

        // when called here we have f_session defined but no context yet
        //
        QString query_str( QString("CREATE KEYSPACE %1").arg(context_name) );

        // this is the default for contexts, but just in case we were
        // to change that default at a later time...
        //
        query_str += QString(" WITH durable_writes = true");

        // TODO: add support for simple strategy for developers
        //
        // for developers testing with a few nodes in a single data center,
        // SimpleStrategy is good enough; for anything larger ("a real
        // cluster",) it won't work right
        //
        //if(strategy == 0 /*"simple"*/)
        //{
        //    query_str += QString( " AND replication = { 'class': 'SimpleStrategy', 'replication_factor': '1' }" );
        //}
        //else
        //{
            // start with a replication factor of 1, we will have a field in
            // the snapdbproxy plugin to let admins change the replication
            // factor later
            //
            // TODO: add a field to the function that allows us to create
            //       the context
            //
            query_str += QString( " AND replication = { 'class': 'NetworkTopologyStrategy', 'dc1': '1' }" );
        //}

        auto query( casswrapper::Query::create( session ) );
        query->query( query_str, 0 );
        //query->setConsistencyLevel( ... );
        //query->setTimestamp(...);
        //query->setPagingSize(...);
        query->start();
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

