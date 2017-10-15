/*
 * Text:
 *      snaplock_tool.cpp
 *
 * Description:
 *      A daemon to synchronize processes between any number of computers
 *      by blocking all of them but one.
 *
 *      The tool implementation is used to send messages to the running
 *      daemon to get information such as the list of tickets and
 *      statistics.
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
#include "snaplock.h"

// our lib
//
#include <snapwebsites/log.h>



/** \class snaplock_tool
 * \brief Handle snaplock command line commands.
 *
 * This class is an implementation of the TCP client message connection
 * so we can handle incoming messages.
 */



/** \brief The messenger initialization.
 *
 * The messenger is a connection to the snapcommunicator server.
 *
 * From the outside, we receive STOP and QUITTING messages.
 * We implement a few other generic messages too (HELP, READY...) Then
 * we support "internal" messages used to gather statistics from a
 * running snaplock daemon.
 *
 * We use a permanent connection so if the snapcommunicator restarts
 * for whatever reason, we reconnect automatically.
 *
 * \param[in] sl  The snaplock server we are listening for.
 * \param[in] addr  The address to connect to. Most often it is 127.0.0.1.
 * \param[in] port  The port to listen on (4040).
 */
snaplock_tool::snaplock_tool(snaplock * sl, std::string const & addr, int port)
    : snaplock_messenger(sl, addr, port)
{
    set_name("snaplock tool");
}


/** \brief Pass messages to Snap Lock.
 *
 * This callback is called whenever a message is received from
 * Snap! Communicator. The message is immediately forwarded to the
 * snaplock object which is expected to process it and reply
 * if required.
 *
 * \param[in] message  The message we just received.
 */
void snaplock_tool::process_message(snap::snap_communicator_message const & message)
{
    f_snaplock->tool_message(message);
}


/** \brief The connection failed, cancel everything.
 *
 * In case of the snaplock tools, we do not want to go on when the connection
 * fails. But since we derive from the snaplock_messenger, we inherit the
 * permanent connection which by default goes on "forever".
 *
 * This function reimplementation allows us to ignore the error.
 *
 * In terms of permanent connection, this is similar to having the f_done
 * flag set to true.
 *
 * \param[in] error_message  An error message about what happened.
 */
void snaplock_tool::process_connection_failed(std::string const & error_message)
{
    SNAP_LOG_ERROR("The connection to snapcommunicator and/or snaplock failed. ")(error_message);

    snap_timer::process_error();
}


/** \brief The connection was established with Snap! Communicator.
 *
 * Whenever the connection is established with the Snap! Communicator,
 * this callback function is called.
 *
 * The tool reacts by REGISTERing as snaplocktool with the Snap!
 * Communicator.
 */
void snaplock_tool::process_connected()
{
    snap_tcp_client_permanent_message_connection::process_connected();

    snap::snap_communicator_message register_snaplock;
    register_snaplock.set_command("REGISTER");
    register_snaplock.add_parameter("service", "snaplocktool");
    register_snaplock.add_parameter("version", snap::snap_communicator::VERSION);
    send_message(register_snaplock);
}


// vim: ts=4 sw=4 et
