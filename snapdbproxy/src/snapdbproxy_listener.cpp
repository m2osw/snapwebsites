/*
 * Text:
 *      snapdbproxy_listener.cpp
 *
 * Description:
 *      Listen for connections on localhost.
 *
 * License:
 *      Copyright (c) 2016-2017 Made to Order Software Corp.
 *
 *      http://snapwebsites.org/
 *      contact@m2osw.com
 *
 *      Permission is hereby granted, free of charge, to any person obtaining a
 *      copy of this software and associated documentation files (the
 *      "Software"), to deal in the Software without restriction, including
 *      without limitation the rights to use, copy, modify, merge, publish,
 *      distribute, sublicense, and/or sell copies of the Software, and to
 *      permit persons to whom the Software is furnished to do so, subject to
 *      the following conditions:
 *
 *      The above copyright notice and this permission notice shall be included
 *      in all copies or substantial portions of the Software.
 *
 *      THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 *      OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *      MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *      IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 *      CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 *      TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 *      SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

// ourselves
//
#include "snapdbproxy.h"

// our lib
//
#include <snapwebsites/log.h>




/** \class snapdbproxy_listener
 * \brief Handle new connections from clients.
 *
 * This function is an implementation of the snap server so we can
 * handle new connections from various clients.
 */



/** \brief The listener initialization.
 *
 * The listener receives a pointer back to the snap::server object and
 * information on how to generate the new network connection to listen
 * on incoming connections from clients.
 *
 * The server listens to two types of messages:
 *
 * \li accept() -- a new connection is accepted from a client
 * \li recv() -- a UDP message was received
 *
 * \param[in] s  The server we are listening for.
 * \param[in] addr  The address to listen on. Most often it is 127.0.0.1.
 * \param[in] port  The port to listen on.
 * \param[in] max_connections  The maximum number of connections to keep
 *            waiting; if more arrive, refuse them until we are done with
 *            some existing connections.
 * \param[in] reuse_addr  Whether to let the OS reuse that socket immediately.
 */
snapdbproxy_listener::snapdbproxy_listener(snapdbproxy * proxy, std::string const & addr, int port, int max_connections, bool reuse_addr)
    : snap_tcp_server_connection(addr, port, "", "", tcp_client_server::bio_server::mode_t::MODE_PLAIN, max_connections, reuse_addr)
    , f_snapdbproxy(proxy)
{
    set_name("snapdbproxy listener");
    non_blocking();
    set_priority(30);
}


/** \brief This callback is called whenever a client tries to connect.
 *
 * This callback function is called whenever a new client tries to connect
 * to the server.
 *
 * The function retrieves the new connection socket, makes the socket
 * "keep alive" and then calls the process_connection() function of
 * the server.
 */
void snapdbproxy_listener::process_accept()
{
    // a new client just connected
    //
    tcp_client_server::bio_client::pointer_t new_client(accept());
    if(new_client == nullptr)
    {
        // TBD: should we call process_error() instead? problem is this
        //      listener would be removed from the list of connections...
        //
        int const e(errno);
        SNAP_LOG_ERROR("accept() returned an error. (errno: ")(e)(" -- ")(strerror(e))("). No new connection will be created.");
        return;
    }

    // process the new connection, which means create a thread
    // and let the thread handle database requests
    //
    f_snapdbproxy->process_connection(new_client);
}


// vim: ts=4 sw=4 et
