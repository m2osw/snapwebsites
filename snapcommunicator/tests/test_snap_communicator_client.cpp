// Snap! Websites -- Test Suite for Snap Communicator; client implementation
// Copyright (C) 2015-2017  Made to Order Software Corp.
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

/** \file
 * \brief Test the lib/snap_communicator.cpp classes.
 *
 * This is a separate test which is used to create a client that connects
 * to a catch test (which starts this client in the first place.)
 *
 * This creates a client connection, and sends some messages and expects
 * specific answers then quits. The command line is used to ask which
 * messages to send.
 */

#include "snap_tests.h"

#include <snapwebsites/snap_communicator.h>
#include "snapwebsites/snapwebsites.h"

#include <sstream>

#include <string.h>

namespace
{



class connection_impl
        : public snap::snap_communicator::snap_tcp_client_message_connection
{
public:
    typedef std::shared_ptr<connection_impl> pointer_t;

                                connection_impl(std::string const & addr, int port, mode_t mode = mode_t::MODE_PLAIN);

    virtual void                process_message(snap::snap_communicator_message const & message);
};




connection_impl::connection_impl(std::string const & addr, int port, mode_t mode)
    : snap_tcp_client_message_connection(addr, port, mode)
{
    non_blocking();
}


void connection_impl::process_message(snap::snap_communicator_message const & message)
{
    QString const command(message.get_command());
std::cerr << QString("%1: CLIENT: note: process message [%2]\n").arg(getpid()).arg(command);
    if(command == "START")
    {
        snap::snap_communicator_message reply;
        reply.set_command("REGISTER");
        reply.add_parameter("service", "images");
        send_message(reply);
        return;
    }

    if(command == "PINGME")
    {
        // send a PING now
        QString const addr(message.get_parameter("address"));
        int const port(message.get_integer_parameter("port"));
        udp_client_server::udp_client client(addr.toUtf8().data(), port);
        QString const msg(message.get_parameter("message"));
        QByteArray m(msg.toUtf8());
        if(client.send(m.data(), m.size()) != m.size()) // we do not send the '\0'
        {
            // XXX: we need to determine whether we want to throw here
            throw std::runtime_error("failed sending PINGME reply");
        }
        return;
    }

    if(command == "STOP")
    {
        // this breaks the loop of the run() function since it is the
        // only connection defined in the communicator
        //

        snap::snap_communicator::instance()->remove_connection(shared_from_this());
        return;
    }
//std::cerr << QString("%1: CLIENT: processed message [%2], now we return back to normal?\n").arg(getpid()).arg(command);
}



void usage()
{
    std::cout << "Usage: test_snap_client [--opt]" << std::endl;
    std::cout << "where --opt are options as follow:" << std::endl;
    std::cout << "  --help | -h        print out this help screen" << std::endl;
    std::cout << "  --version          print out library version with which this test was compiled" << std::endl;
    std::cout << "  --host <address>   the IP address to connect to" << std::endl;
    std::cout << "  --port <port>      the port to connect to" << std::endl;
    exit(1);
}


} // no name namespace



int main(int argc, char * argv[])
{
    int port(4010);

    for(int i(1); i < argc; ++i)
    {
        const std::string arg(argv[i]);
        if( arg == "--help" )
        {
            usage();
            snap::NOTREACHED();
        }
        else if( arg == "--version" )
        {
            std::cout << SNAPWEBSITES_VERSION_STRING << std::endl;
            exit(0);
            snap::NOTREACHED();
        }
        else if( arg == "--host" )
        {
            ++i;
            if(i >= argc)
            {
                std::cerr << "error: --host must be followed by the IP address to connect to." << std::endl;
                exit(1);
                snap::NOTREACHED();
            }
            snap_test::host(argv[i]);
        }
        else if( arg == "--port" )
        {
            ++i;
            if(i >= argc)
            {
                std::cerr << "error: --host must be followed by the IP address to connect to." << std::endl;
                exit(1);
                snap::NOTREACHED();
            }
            port = atol(argv[i]);
        }
    }

    snap::snap_communicator::pointer_t communicator(snap::snap_communicator::instance());
    connection_impl::pointer_t connection(new connection_impl(snap_test::host(), port));
    connection->set_name("CLIENT: created connection");
    communicator->add_connection(connection);

    // always send a version message first
    // (since this gets cached, it will work as expected: i.e. the
    // communicator system will write that to the socket as soon as
    // possible)
    //
    snap::snap_communicator_message message;
    message.set_command("VERSION");
    message.add_parameter("version", SNAPWEBSITES_VERSION_STRING);
    connection->send_message(message);

    communicator->run();

    exit(0);
}



// Local Variables:
// mode: cpp
// indent-tabs-mode: nil
// c-basic-offset: 4
// tab-width: 4
// End:

// vim: ts=4 sw=4 et
