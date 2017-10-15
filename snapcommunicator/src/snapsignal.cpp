// Snap Websites Server -- to send UDP signals to backends
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

#include <snapwebsites/snapwebsites.h>
#include <snapwebsites/snap_config.h>
#include <snapwebsites/not_reached.h>


int main(int argc, char *argv[])
{
    try
    {
        // create a server object
        snap::server::pointer_t s( snap::server::instance() );
        s->setup_as_backend();

        // parse the command line arguments (this also brings in the .conf params)
        s->config(argc, argv);

        // Now create the qt application instance
        //
        s->prepare_qtapp( argc, argv );

        // get the message (Excuse the naming convension...)
        //
        QString const msg(s->get_parameter("__BACKEND_URI"));

        // the message is expected to be a complete message as defined in
        // our snap_communicator system, something like:
        //
        //    <service>/<COMMAND> param=value;...
        //
        snap::snap_communicator_message message;
        message.from_message(msg);

        // get the snap communicator signal IP and port (UDP)
        //
        QString addr("127.0.0.1");
        int port(4041);
        snap::snap_config config("snapcommunicator");
        tcp_client_server::get_addr_port(QString::fromUtf8(config.get_parameter("signal").c_str()), addr, port, "udp");

        // now send the message
        //
        snap::snap_communicator::snap_udp_server_message_connection::send_message(addr.toUtf8().data(), port, message);

        // exit via the server so the server can clean itself up properly
        s->exit(0);
        snap::NOTREACHED();

        return 0;
    }
    catch(std::exception const & e)
    {
        // clean error on exception
        std::cerr << "snapsignal: exception: " << e.what() << std::endl;
        return 1;
    }
}

// vim: ts=4 sw=4 et
