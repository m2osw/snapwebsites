// Snap! Websites -- Test Suite for Snap Communicator
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
 * This test runs a battery of tests agains the snap_communicator.cpp
 * implementation to ensure that most everything works as expected.
 */

#include "snap_tests.h"

// snapwebsites lib
//
#include <snapwebsites/log.h>
#include <snapwebsites/snap_communicator.h>

// C lib
//
#include <string.h>
#include <signal.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <sys/wait.h>

// C++ lib
//
#include <sstream>


namespace
{

bool g_error_happened;

} // no name namespace




TEST_CASE("Message", "[snap-communicator] [message]")
{
    SECTION("baby steps first, make sure we can add/get/set properly")
    {
        snap::snap_communicator_message message;

        // no defaults
        REQUIRE(message.get_service() == "");
        REQUIRE(message.get_command() == "");
        REQUIRE_FALSE(message.has_parameter("p1"));
        REQUIRE_FALSE(message.has_parameter("p2"));

        // set and verify
        message.set_service("test-service");
        REQUIRE(message.get_service() == "test-service");

        message.set_command("test-command");
        REQUIRE(message.get_command() == "test-command");

        message.add_parameter("p1", "value one");
        REQUIRE(message.has_parameter("p1"));
        REQUIRE(message.get_parameter("p1") == "value one");
        REQUIRE(message.get_all_parameters().size() == 1);
        REQUIRE_FALSE(message.has_parameter("p2"));

        message.add_parameter("p2", 123);
        REQUIRE(message.has_parameter("p1"));
        REQUIRE(message.get_parameter("p1") == "value one");
        REQUIRE(message.has_parameter("p2"));
        REQUIRE(message.get_parameter("p2") == "123");
        REQUIRE(message.get_integer_parameter("p2") == 123);
        REQUIRE(message.get_all_parameters().size() == 2);
    }

    SECTION("test the from_message()/to_message() functions")
    {
        snap::snap_communicator_message message;

        REQUIRE(message.from_message("images/REPROCESS url=http://domain.name/this/one;priority=normal"));

        REQUIRE(message.get_service() == "images");
        REQUIRE(message.get_command() == "REPROCESS");
        REQUIRE_FALSE(message.has_parameter("p1"));
        REQUIRE_FALSE(message.has_parameter("p2"));
        REQUIRE(message.has_parameter("url"));
        REQUIRE(message.has_parameter("priority"));
        REQUIRE(message.get_parameter("url") == "http://domain.name/this/one");
        REQUIRE(message.get_parameter("priority") == "normal");
        // returned in "alphabetical" (binary really) order
        REQUIRE(message.to_message() == "images/REPROCESS priority=normal;url=http://domain.name/this/one");
    }

    SECTION("test the from_message()/to_message() functions with a stringized parameter")
    {
        snap::snap_communicator_message message;

        REQUIRE(message.from_message("pagelist/RESET url=\"http://domain.name/this;one;path\";priority=-58"));

        REQUIRE(message.get_service() == "pagelist");
        REQUIRE(message.get_command() == "RESET");
        REQUIRE_FALSE(message.has_parameter("p1"));
        REQUIRE_FALSE(message.has_parameter("p2"));
        REQUIRE(message.has_parameter("url"));
        REQUIRE(message.has_parameter("priority"));
        REQUIRE(message.get_parameter("url") == "http://domain.name/this;one;path");
        REQUIRE(message.get_parameter("priority") == "-58");
        REQUIRE(message.get_integer_parameter("priority") == -58);
        // returned in "alphabetical" (binary really) order
        REQUIRE(message.to_message() == "pagelist/RESET priority=-58;url=\"http://domain.name/this;one;path\"");
    }

    SECTION("test the from_message()/to_message() functions adding a couple of parameters")
    {
        snap::snap_communicator_message message;

        REQUIRE(message.from_message("PING"));

        REQUIRE(message.get_service() == "");
        REQUIRE(message.get_command() == "PING");
        REQUIRE_FALSE(message.has_parameter("p1"));
        REQUIRE_FALSE(message.has_parameter("p2"));
        REQUIRE_FALSE(message.has_parameter("url"));
        REQUIRE_FALSE(message.has_parameter("priority"));
        message.add_parameter("url", "\"not\naccessible\"");
        message.add_parameter("priority", 87);
        // returned in "alphabetical" (binary really) order
        REQUIRE(message.to_message() == "PING priority=87;url=\"\\\"not\\naccessible\\\"\"");
    }

    SECTION("test the from_message()/to_message() functions with special characters")
    {
        snap::snap_communicator_message message;

        REQUIRE(message.from_message("PING url=\"\\\"quoted URL\\\"\";zindex=\"3\\n-7\\r+5\""));

        REQUIRE(message.get_service() == "");
        REQUIRE(message.get_command() == "PING");
        REQUIRE_FALSE(message.has_parameter("p1"));
        REQUIRE_FALSE(message.has_parameter("p2"));
        REQUIRE(message.has_parameter("url"));
        REQUIRE(message.get_parameter("url") == "\"quoted URL\"");
        REQUIRE(message.has_parameter("zindex"));
        REQUIRE(message.get_parameter("zindex") == "3\n-7\r+5");
        message.set_service("images");
        message.add_parameter("priority", 87);
        // returned in "alphabetical" (binary really) order
        REQUIRE(message.to_message() == "images/PING priority=87;url=\"\\\"quoted URL\\\"\";zindex=3\\n-7\\r+5");
    }

    // TODO: add tests for all kinds of errors (for full coverage)

//        REQUIRE_THROWS_AS(func, expected exception);
}


TEST_CASE("Client/Server test", "[snap-communicator] [client-server]")
{
    // so much for integrated tests... here we want to have a separate
    // process to connect to us (we will be the TCP and UDP servers)
    // and we want a separate process because otherwise we could run
    // in some problems (i.e. the run() process blocking, etc.)
    //
    SECTION("create server, client and send some messsages over TCP and UDP, and test the timer")
    {
        class client_impl
            : public snap::snap_communicator::snap_tcp_server_client_message_connection
        {
        public:
            typedef std::shared_ptr<client_impl> pointer_t;

            client_impl(tcp_client_server::bio_client::pointer_t client)
                : snap_tcp_server_client_message_connection(client)
            {
            }

            virtual void process_timeout()
            {
                // remove the timer
                set_timeout_delay(-1);

                // done (we could grow all of that some more later...)
                snap::snap_communicator_message reply;
                reply.set_command("PINGME");
                reply.add_parameter("address", snap_test::host().c_str());
                reply.add_parameter("port", 4011);

                // for fun I create a message here and that's the message
                // we will have pinged back to us
                snap::snap_communicator_message expected_ping;
                expected_ping.set_service("pagelist");
                expected_ping.set_command("PING");
                expected_ping.add_parameter("madeup", "ping parameter");
                reply.add_parameter("message", expected_ping.to_message());

                send_message(reply);
            }

            virtual void process_hup()
            {
                snap_tcp_server_client_message_connection::process_hup();

                // force the listener to also go away once the client
                // is gone...
                //
                snap::snap_communicator::instance()->remove_connection(f_listener);
            }

            // snap::snap_communicator::snap_tcp_server_client_message_connection implementation
            virtual void process_message(snap::snap_communicator_message const & message)
            {
                snap::snap_communicator_message reply;

                QString const command(message.get_command());
std::cerr << QString("%1: SERVER: received command [%2]\n").arg(getpid()).arg(command);
                if(command == "VERSION")
                {
                    QString const version(message.get_parameter("version"));
                    if(version != SNAPWEBSITES_VERSION_STRING)
                    {
                        std::cerr << "REQUIRE( version == \"" SNAPWEBSITES_VERSION_STRING "\" ) failed (version = [" << version << "])\n";
                        g_error_happened = true;
                        reply.set_command("STOP");
                    }
                    else
                    {
                        // no reply on that one so we cannot go on, just return
                        return;
                    }
                }
                else if(command == "REGISTER")
                {
                    // REGISTER is sent as a reply to our START command
                    QString const service(message.get_parameter("service"));
                    if(service != "images")
                    {
                        std::cerr << "REQUIRE( service == \"images\" ) failed (service = [" << service << "])\n";
                        g_error_happened = true;
                        reply.set_command("STOP");
                    }
                    else
                    {
                        // next we test the timeout by not replying and
                        // in a second we should get a timeout which
                        // sends the STOP message...
                        //
                        // since we send no reply just return immediately
                        set_timeout_delay(1000000);
                        return;
                    }
                }
                else
                {
                    std::cerr << "REQUIRE( command == \"...\" ) failed (command = [" << command << "])\n";
                    g_error_happened = true;
                    reply.set_command("STOP");
                }

                send_message(reply);
            }

            void set_listener(snap::snap_communicator::snap_connection::pointer_t listener)
            {
                f_listener = listener;
            }

        private:
            snap::snap_communicator::snap_connection::pointer_t     f_listener;
        };

        class tcp_listener_impl
            : public snap::snap_communicator::snap_tcp_server_connection
        {
        public:
            typedef std::shared_ptr<tcp_listener_impl> pointer_t;

            tcp_listener_impl(std::string const & addr, int port, int max_connections = -1, bool reuse_addr = false)
                : snap_tcp_server_connection(addr, port, "", "", tcp_client_server::bio_server::mode_t::MODE_PLAIN, max_connections, reuse_addr)
            {
                //non_blocking();
            }

            virtual void process_accept()
            {
std::cerr << QString("%1: SERVER: received client connection\n").arg(getpid());
                // this is a new client connection
                tcp_client_server::bio_client::pointer_t new_client(accept());
                if(new_client == nullptr)
                {
                    // TBD: should we call process_error() instead? problem is this
                    //      listener would be removed from the list of connections...
                    //
                    int const e(errno);
                    // TODO: shouldn't this be a CATCH() instead?
                    SNAP_LOG_ERROR("accept() returned an error. (errno: ")(e)(" -- ")(strerror(e))("). No new connection will be created.");
                    return;
                }

                f_connection.reset(new client_impl(new_client));
                f_connection->set_name("SERVER: connection from client");
                f_connection->set_listener(shared_from_this());

                snap::snap_communicator::instance()->add_connection(f_connection);

                snap::snap_communicator_message start;
                start.set_command("START");
                f_connection->send_message(start);
            }

            void send_message(snap::snap_communicator_message const & message)
            {
                f_connection->send_message(message);
            }

        private:
            client_impl::pointer_t  f_connection;
        };

        class udp_listener_impl
            : public snap::snap_communicator::snap_udp_server_connection
        {
        public:
            typedef std::shared_ptr<udp_listener_impl> pointer_t;

            udp_listener_impl(std::string const & addr, int port)
                : snap_udp_server_connection(addr, port)
            {
                //non_blocking();
            }

            virtual void process_read()
            {
                snap::snap_communicator_message reply;
                reply.set_command("STOP");

                char buf[1024];
                ssize_t const r(recv(buf, sizeof(buf) - 1));
                if(r < 0)
                {
                    int const e(errno);
                    std::cerr << "recv() for UDP message failed (errno = " << e << ")\n";
                    g_error_happened = true;
                }
                else if(r < static_cast<ssize_t>(sizeof(buf)))
                {
                    buf[r] = '\0';
                    snap::snap_communicator_message ping;
                    ping.from_message(QString::fromUtf8(buf));
std::cerr << QString("%1: MESSAGE LISTENER: received UDP message \"%2\".\n").arg(getpid()).arg(ping.get_command());
                    if(ping.get_command() != "PING")
                    {
                        std::cerr << "REQUIRE( command == \"PING\" ) failed (command = [" << ping.get_command() << "])\n";
                        g_error_happened = true;
                    }
                    if(ping.get_service() != "pagelist")
                    {
                        std::cerr << "REQUIRE( service == \"pagelist\" ) failed (service = [" << ping.get_service() << "])\n";
                        g_error_happened = true;
                    }
                    if(ping.get_parameter("madeup") != "ping parameter")
                    {
                        std::cerr << "REQUIRE( parameter madeup == \"ping parameter\" ) failed (parameter = [" << ping.get_parameter("madeup") << "])\n";
                        g_error_happened = true;
                    }
                }
                else
                {
                    throw std::logic_error("this cannot happen");
                }

                // we remove ourselves so that way we don't have
                // to give the UDP pointer to the client connection
                // of the TCP server
                //
                snap::snap_communicator::instance()->remove_connection(shared_from_this());

                f_tcp_listener->send_message(reply);
                f_tcp_listener.reset(); // lose the shared pointer
            }

            void set_tcp_listener(tcp_listener_impl::pointer_t tcp_listener)
            {
                f_tcp_listener = tcp_listener;
            }

        private:
            tcp_listener_impl::pointer_t    f_tcp_listener;
        };

        snap::snap_communicator::pointer_t communicator(snap::snap_communicator::instance());

        tcp_listener_impl::pointer_t tcp_listener(new tcp_listener_impl(snap_test::host(), 4010, -1, true));
        tcp_listener->set_name("SERVER: tcp_listener_impl");
        REQUIRE(tcp_listener->is_listener()); // make sure this is true
        communicator->add_connection(tcp_listener);

        udp_listener_impl::pointer_t udp_listener(new udp_listener_impl(snap_test::host(), 4011));
        udp_listener->set_name("SERVER: udp_listener_impl");
        udp_listener->set_tcp_listener(tcp_listener);
        communicator->add_connection(udp_listener);

        // force a hang-up of the children upon our death
        prctl(PR_SET_PDEATHSIG, SIGHUP);

        // we are ready to start the client since we are now listening
        // for new connections
        //
        pid_t child(fork());
        if(child == 0)
        {
            // we are the child, run the client test
            std::string const cmd(snap_test::progdir() + "/test_snap_communicator_client --host " + snap_test::host() + " --port 4010");
            exit(system(cmd.c_str()));
        }

        // run until all our connections get removed
        communicator->run();

        // make sure the listeners are gone...
        // (this should be totally useless but the communicator could
        // have returned for a reason other than an empty list of
        // connections)
        //
        communicator->remove_connection(tcp_listener);
        communicator->remove_connection(udp_listener);

        // block until child returns (maybe we should have a timer on that one?)
        int status(0);
        waitpid(child, &status, 0);

        // make sure the child dies (can be useful while debugging)
        //kill(child, SIGKILL);

        REQUIRE_FALSE(g_error_happened);
    }
}



// Local Variables:
// mode: cpp
// indent-tabs-mode: nil
// c-basic-offset: 4
// tab-width: 4
// End:

// vim: ts=4 sw=4 et
