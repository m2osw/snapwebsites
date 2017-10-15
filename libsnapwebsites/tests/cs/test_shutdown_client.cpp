// Snap Websites Server -- test shutdown() on socket (client side)
// Copyright (C) 2014-2017  Made to Order Software Corp.
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

//
// This test works along the test_shutdown_server.cpp and implements the
// client's side. It connects to the server, sends a few messages (START,
// PAUSE x 4, STOP) and expects a HUP before quitting.
//
// To run the test, you first have to start the server otherwise the
// client won't be able to connect.
//

#include <snapwebsites/log.h>
#include <snapwebsites/not_reached.h>
#include <snapwebsites/qstring_stream.h>
#include <snapwebsites/snap_communicator.h>

#include <fcntl.h>
#include <netinet/tcp.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <iostream>

#include <QDir>



class messenger_connection
        : public snap::snap_communicator::snap_tcp_client_message_connection
{
public:
    messenger_connection()
        : snap_tcp_client_message_connection(
                    "127.0.0.1",
                    4030,
                    tcp_client_server::bio_client::mode_t::MODE_PLAIN)
    {
        set_name("messenger");
    }

    virtual void process_message(snap::snap_communicator_message const & message)
    {
        SNAP_LOG_INFO("process_message() client side: [")(message.to_message())("]");
    }

    virtual void process_timeout()
    {
        SNAP_LOG_INFO("process_timeout() called.");

        snap::snap_communicator_message msg;
        switch(f_state)
        {
        case 0:
            // sent to server
            msg.set_command("START");
            break;

        case 1:
        case 2:
        case 3:
        case 4:
            // sent "stuff" to client
            msg.set_command("PAUSE");
            break;

        case 5:
            // end everything (server closes socket)
            msg.set_command("STOP");

            // blocking for last message!
            // this does not make any difference, we really need the HUP
            //{
            //    int optval(0);
            //    ioctl(get_socket(), FIONBIO, &optval);

            //    int flags(fcntl(get_socket(), F_GETFL));
            //    flags &= ~O_NONBLOCK;
            //    fcntl(get_socket(), F_SETFL, flags);

            //}

            // stop the timer and just wait on HUP now
            set_timeout_delay(-1);
            break;

// having a case 6 instead of proper HUP works, but it is a hack with
// a timing that can be completely off either way (waiting for too much
// or too little time.)
//        case 6:
//            // now remove ourselves from the communicator, we are done
//            {
//                snap::snap_communicator::snap_connection::vector_t const & c(snap::snap_communicator::instance()->get_connections());
//                if(c.size() > 0)
//                {
//                    snap::snap_communicator::instance()->remove_connection(c[0]);
//                }
//            }
//            // we are not sending anything in this case
//            return;

        default:
            throw std::runtime_error("unknown f_state in client.");

        }
        SNAP_LOG_INFO("client sending message: [")(msg.to_message())("]");
        send_message(msg);

        // this is definitely wrong, if you ever call shutdown() it kills
        // the socket right there!
        //if(f_state == 5)
        //{
        //    shutdown(get_socket(), SHUT_WR);
        //}

        ++f_state;
    }

    int f_state = 0;
};



int main(int /*argc*/, char * /*argv*/[])
{
    try
    {
        snap::logging::set_progname("test_shutdown_server");
        snap::logging::configure_console();
        snap::logging::set_log_output_level(snap::logging::log_level_t::LOG_LEVEL_TRACE);

        messenger_connection::pointer_t mc(new messenger_connection);
        mc->set_timeout_delay(1LL * 1000000LL);

        int flag(1);
        if(setsockopt(mc->get_socket(), IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int)) < 0)
        {
            int const e(errno);
            SNAP_LOG_WARNING("setsockopt() with TCP_NODELAY failed. (errno: ")(e)(" -- ")(strerror(e))(")");
        }

        snap::snap_communicator::instance()->add_connection(mc);

        snap::snap_communicator::instance()->run();

        SNAP_LOG_INFO("exited run() loop...");

        return 0;
    }
    catch( snap::snap_exception const & e )
    {
        SNAP_LOG_FATAL("Caught a Snap! exception [")(e.what())("].");
    }
    catch( std::exception const & e )
    {
        SNAP_LOG_FATAL("Caught exception [")(e.what())("].");
    }

    return 1;
}

// vim: ts=4 sw=4 et
