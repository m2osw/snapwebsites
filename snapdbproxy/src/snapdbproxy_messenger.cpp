/*
 * Text:
 *      snapdbproxy/src/snapdbproxy_messenger.cpp
 *
 * Description:
 *      Messenger Class implementation. The messenger of the snapdbproxy
 *      is primarily used to handle the CASSANDRASTATUS message to know
 *      whether the `snapdbproxy` daemon is connected to Cassandra or not.
 *
 * License:
 *      Copyright (c) 2016-2019  Made to Order Software Corp.  All Rights Reserved
 *
 *      https://snapwebsites.org/
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




/** \class snapdbproxy_messenger
 * \brief Handle messages from the Snap Communicator server.
 *
 * This class is an implementation of the TCP client message connection
 * so we can handle incoming messages.
 */



/** \brief The messenger initialization.
 *
 * The messenger is a connection to the snapcommunicator server.
 *
 * In most cases we receive CASSANDRASTATUS, STOP, and LOG messages from it.
 * We implement a few other messages too (HELP, READY...)
 *
 * We use a permanent connection so if the snapcommunicator restarts
 * for whatever reason, we reconnect automatically.
 *
 * \note
 * The messenger connection used by the snapdbproxy tool makes use
 * of a thread. You will want to change this initialization function
 * if you intend to fork() and run direct children (i.e. not fork()
 * + execv() as we do to run iptables.)
 *
 * \param[in] proxy  The snapdbproxy server we are listening for.
 * \param[in] addr  The address to connect to. Most often it is 127.0.0.1.
 * \param[in] port  The port to connect to (4040).
 */
snapdbproxy_messenger::snapdbproxy_messenger(snapdbproxy * proxy, std::string const & addr, int port)
    : snap_tcp_client_permanent_message_connection(addr, port)
    , f_snapdbproxy(proxy)
{
    set_name("snapdbproxy messenger");
}


/** \brief Pass messages to the Snap Firewall.
 *
 * This callback is called whenever a message is received from
 * Snap! Communicator. The message is immediately forwarded to the
 * snapdbproxy object which is expected to process it and reply
 * if required.
 *
 * \param[in] message  The message we just received.
 */
void snapdbproxy_messenger::process_message(snap::snap_communicator_message const & message)
{
    f_snapdbproxy->process_message(message);
}


/** \brief The messenger could not connect to snapcommunicator.
 *
 * This function is called whenever the messengers fails to
 * connect to the snapcommunicator server. This could be
 * because snapcommunicator is not running or because the
 * configuration information for the snapdbproxy is wrong...
 *
 * Note that it is not abnormal as snapcommunicator may not
 * have been started yet when snapdbproxy is started. This
 * is okay because we have a messenger system that is resilient.
 * However, in normal circumstances, this error should very
 * rarely if ever happen.
 *
 * \param[in] error_message  An error message.
 */
void snapdbproxy_messenger::process_connection_failed(std::string const & error_message)
{
    SNAP_LOG_ERROR("connection to snapcommunicator failed (")(error_message)(")");

    // also call the default function, just in case
    snap_tcp_client_permanent_message_connection::process_connection_failed(error_message);
}


/** \brief The connection was established with Snap! Communicator.
 *
 * Whenever the connection is established with the Snap! Communicator,
 * this callback function is called.
 *
 * The messenger reacts by REGISTERing the snapdbproxy with the Snap!
 * Communicator.
 */
void snapdbproxy_messenger::process_connected()
{
    snap_tcp_client_permanent_message_connection::process_connected();

    snap::snap_communicator_message register_snapdbproxy;
    register_snapdbproxy.set_command("REGISTER");
    register_snapdbproxy.add_parameter("service", "snapdbproxy");
    register_snapdbproxy.add_parameter("version", snap::snap_communicator::VERSION);
    send_message(register_snapdbproxy);
}


// vim: ts=4 sw=4 et
