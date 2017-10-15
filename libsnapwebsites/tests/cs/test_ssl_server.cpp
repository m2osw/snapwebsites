// Snap Websites Server -- test SSL on socket (server side)
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
// This test is used to prove that using SSL on the sockets works as
// expected with bio_server and bio_client.
//
// The test makes use of the test_ssl_client.cpp as well. To run the
// test, start the test_ssl_server binary first, then start the
// test_ssl_client. Then both exit cleanly (althouh you get a HUP
// in the client, which is normal, as in: part of the protocol used in
// this test).
//

#include <snapwebsites/log.h>
#include <snapwebsites/not_reached.h>
#include <snapwebsites/qstring_stream.h>
#include <snapwebsites/snap_communicator.h>

#include <unistd.h>

#include <iostream>

#include <QDir>


pid_t   g_parent_pid = 0;
pid_t   g_child_pid = 0;


class sigchld_impl
        : public snap::snap_communicator::snap_signal
{
public:
    typedef std::shared_ptr<sigchld_impl>    pointer_t;

    /** \brief The SIGCHLD signal initialization.
     *
     * The constructor defines this signal connection as a listener for
     * the SIGCHLD signal.
     *
     * \param[in] si  The snap init server we are listening for.
     * \param[in] addr  The address to listen on. Most often it is 127.0.0.1.
     *                  for the UDP because we currently only allow for
     *                  local messages.
     * \param[in] port  The port to listen on.
     */
    sigchld_impl()
        : snap_signal(SIGCHLD)
    {
        set_name("signal child death");
    }

    // snap::snap_communicator::snap_signal implementation
    virtual void process_signal()
    {
        // kill all connections
        snap::snap_communicator::snap_connection::vector_t const c(snap::snap_communicator::instance()->get_connections());
        for(size_t idx(0); idx < c.size(); ++idx)
        {
            snap::snap_communicator::instance()->remove_connection(c[idx]);
        }
    }
};


class client_connection
        : public snap::snap_communicator::snap_tcp_server_client_message_connection
{
public:
    client_connection(tcp_client_server::bio_client::pointer_t client)
        : snap_tcp_server_client_message_connection(client)
    {
        set_name("client");
    }

    virtual void process_message(snap::snap_communicator_message const & message)
    {
        QString const & command(message.get_command());

        if(getpid() == g_parent_pid)
        {
            SNAP_LOG_INFO("process_message() server/parent -- [")(message.to_message())("]");

            if(g_child_pid != 0)
            {
                throw std::runtime_error("child process already created.");
            }

            if(command == "START")
            {
                g_child_pid = fork();
                if(g_child_pid != 0)
                {
                    if(g_child_pid == -1)
                    {
                        throw std::runtime_error("could not create child process.");
                    }

                    // we assume we do not need the client's connection anymore
                    // (although the connection is 'this' we need a shared
                    // pointer so I just loop since I could do a fast copy/paste)
                    //
                    snap::snap_communicator::snap_connection::vector_t const & c(snap::snap_communicator::instance()->get_connections());
                    for(size_t idx(0); idx < c.size(); ++idx)
                    {
                        if(c[idx]->get_name() == "client")
                        {
                            snap::snap_communicator::instance()->remove_connection(c[idx]);
                            break;
                        }
                    }

                    return;
                }

                // here we are in the child
                snap::snap_communicator::snap_connection::vector_t const c(snap::snap_communicator::instance()->get_connections());
                for(size_t idx(0); idx < c.size(); ++idx)
                {
                    if(c[idx]->get_name() != "client")
                    {
                        snap::snap_communicator::instance()->remove_connection(c[idx]);
                    }
                }

                // return to the run() loop in the child!
                return;
            }

            // any other message is an error in the parent
            throw std::runtime_error("could not create child process.");
        }

        SNAP_LOG_INFO("process_message() server/child -- [")(message.to_message())("]");

        // here we handle commands received by client
        if(command == "PAUSE")
        {
            SNAP_LOG_INFO("PAUSE received");
            return;
        }

        if(command == "STOP")
        {
            // remove client from run loop, then we will exit
            snap::snap_communicator::snap_connection::vector_t const & c(snap::snap_communicator::instance()->get_connections());
            for(size_t idx(0); idx < c.size(); ++idx)
            {
                if(c[idx]->get_name() == "client")
                {
                    snap::snap_communicator::instance()->remove_connection(c[idx]);
                    break;
                }
            }
            return;
        }

        SNAP_LOG_ERROR("unknown command [")(command)("] received.");
    }
};

class listener
        : public snap::snap_communicator::snap_tcp_server_connection
{
public:
    listener()
        : snap_tcp_server_connection("127.0.0.1", 4030, "ssl-test.crt", "ssl-test.key", tcp_client_server::bio_server::mode_t::MODE_SECURE, 10, true)
    {
        set_name("listener");
    }

    virtual void process_accept()
    {
        SNAP_LOG_INFO("server received accept");

        tcp_client_server::bio_client::pointer_t new_client(accept());
        if(!new_client)
        {
            throw std::runtime_error("accept failed");
        }
        client_connection::pointer_t connection(new client_connection(new_client));
        if(!snap::snap_communicator::instance()->add_connection(connection))
        {
            throw std::runtime_error("could not add connection");
        }
    }
};


int main(int /*argc*/, char * /*argv*/[])
{
    try
    {
        snap::logging::set_progname("test_ssl_server");
        snap::logging::configure_console();
        snap::logging::set_log_output_level(snap::logging::log_level_t::LOG_LEVEL_TRACE);

        g_parent_pid = getpid();

        listener::pointer_t l(new listener);
        snap::snap_communicator::instance()->add_connection(l);

        sigchld_impl::pointer_t chld(new sigchld_impl);
        snap::snap_communicator::instance()->add_connection(chld);

        SNAP_LOG_INFO("server ready");
        snap::snap_communicator::instance()->run();

        return 0;
    }
    catch( std::exception & e )
    {
        SNAP_LOG_FATAL("Caught exception: \"")(e.what())("\".");
    }

    return 1;
}

// vim: ts=4 sw=4 et
