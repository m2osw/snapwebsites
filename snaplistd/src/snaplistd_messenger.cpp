/*
 * Text:
 *      snapwebsites/snaplistd/src/snaplistd_messenger.cpp
 *
 * Description:
 *      A daemon to collect all the changing website pages so the pagelist
 *      can manage them.
 *
 *      The messenger implementation listens for LISTDATA messages from
 *      other services.
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
#include "snaplistd.h"

// our lib
//
#include <snapwebsites/log.h>



/** \class snaplistd_messenger
 * \brief Handle messages from the Snap Communicator server.
 *
 * This class is an implementation of the TCP client message connection
 * so we can handle incoming messages.
 */



/** \brief The messenger initialization.
 *
 * The messenger is a connection to the snapcommunicator server.
 *
 * From the outside, we receive the LISTDATA to add an entry to the
 * journal.
 *
 * We use a permanent connection so if the snapcommunicator restarts
 * for whatever reason, we reconnect automatically.
 *
 * \param[in] sl  The snaplistd server we are listening for.
 * \param[in] addr  The address to connect to. Most often it is 127.0.0.1.
 * \param[in] port  The port to listen on (4040).
 */
snaplistd_messenger::snaplistd_messenger(snaplistd * sl, std::string const & addr, int port)
    : snap_tcp_client_permanent_message_connection(addr, port)
    , f_snaplistd(sl)
{
    set_name("snaplistd messenger");
}


/** \brief Pass messages to Snap listd.
 *
 * This callback is called whenever a message is received from
 * Snap! Communicator. The message is immediately forwarded to the
 * snaplistd object which is expected to process it and reply
 * if required.
 *
 * \param[in] message  The message we just received.
 */
void snaplistd_messenger::process_message(snap::snap_communicator_message const & message)
{
    f_snaplistd->process_message(message);
}


/** \brief The messenger could not connect to snapcommunicator.
 *
 * This function is called whenever the messengers fails to
 * connect to the snapcommunicator server. This could be
 * because snapcommunicator is not running or because the
 * configuration information for the snaplistd is wrong...
 *
 * \param[in] error_message  An error message.
 */
void snaplistd_messenger::process_connection_failed(std::string const & error_message)
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
 * The messenger reacts by REGISTERing the snaplistd with the Snap!
 * Communicator.
 */
void snaplistd_messenger::process_connected()
{
    snap_tcp_client_permanent_message_connection::process_connected();

    snap::snap_communicator_message register_snaplistd;
    register_snaplistd.set_command("REGISTER");
    register_snaplistd.add_parameter("service", "snaplistd");
    register_snaplistd.add_parameter("version", snap::snap_communicator::VERSION);
    send_message(register_snaplistd);
}


// vim: ts=4 sw=4 et
