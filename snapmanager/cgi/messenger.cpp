//
// File:        messenger.cpp
// Object:      Allow for managing a Snap! Cluster.
//
// Copyright:   Copyright (c) 2016-2017 Made to Order Software Corp.
//              All Rights Reserved.
//
// http://snapwebsites.org/
// contact@m2osw.com
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "snapmanagercgi.h"


namespace snap_manager
{


/** \class messenger
 * \brief The actual implementation of messenger.
 *
 * This class is the implementation of a messenger used to send / receive
 * messages to the snapmanagerdaemon running on all computers.
 */



/** \brief Initiate a messenger to send a request to all snapmanagerdaemon.
 *
 * This function initializes a messenger that connects to the snapcommunicator
 * and then sends the specified \p message to snapmanagerdaemon.
 *
 * Once you constructed this messenger, you can wait for the message to
 * get sent by calling the run() function. Once the run() function returns,
 * the message was sent.
 *
 * \code
 * snap_communicator_message my_message;
 * my_message.set_command("EXPLODE");
 * ...
 * messenger msg(f_communicator_address, f_communicator_port, my_message);
 * msg.run();
 * \endcode
 *
 * \param[in] address  The address of snapcommunicator.
 * \param[in] port  The port of snapcommunicator.
 * \param[in] message  The message to send once we are connected.
 */
messenger::messenger(std::string const & address, int port, snap::snap_communicator_message const & message)
    : snap_tcp_blocking_client_message_connection(address, port, tcp_client_server::bio_client::mode_t::MODE_PLAIN)
    , f_message(message)
{
    int const fd(get_socket());
    if(fd < 0)
    {
        f_result = "could not connect to snapmanagerdaemon on this server.";
        return;
    }

    // wait for 1 second for replies
    //
    set_timeout_date(snap::snap_communicator::get_current_date() + 1000000L);

    // need to register with snap communicator
    //
    snap::snap_communicator_message register_message;
    register_message.set_command("REGISTER");
    register_message.add_parameter("service", QString("snapmanagercgi%1").arg(getpid()));
    register_message.add_parameter("version", snap::snap_communicator::VERSION);
    if(!send_message(register_message))
    {
        // this could happen if the socket does not have enough buffering
        // space of the register_message, which is probably unlikely
        //
        throw std::runtime_error("snapmanagercgi messenger could not send its REGISTER message to snapcommunicator");
    }

    // now wait for the READY and HELP replies, send the message, and
    // then wait for a while
    //
    // it's not a good idea to have the run() loop in the constructor...
    //
    //run();
}


/** \brief Send the UNLOCK message to snaplock to terminate the lock.
 *
 * The destructor makes sure that the lock is released.
 */
messenger::~messenger()
{
}


/** \brief Retrieve the resulting string.
 *
 * This function returns a copy of the result that was sent to use
 * by the snapmanagerdaemon service.
 *
 * \return The result we received in the message.
 */
QString const & messenger::result()
{
    return f_result;
}


/** \brief We waited much already, forget the next answers.
 *
 * After a little time we still want to return with whatever output
 * we already received.
 */
void messenger::process_timeout()
{
    mark_done();
}


/** \brief Process results as we receive them.
 *
 * This function is called whenever a complete message is read from
 * the snapcommunicator.
 *
 * This function gets called whenever a reply from a snapmanagerdaemon
 * is received. It also handles communication between us and the
 * snapcommunicator.
 *
 * \param[in] message  The message we just received.
 */
void messenger::process_message(snap::snap_communicator_message const & message)
{
    SNAP_LOG_TRACE("received message [")(message.to_message())("] for snapmanager.cgi");

    QString const command(message.get_command());

    switch(command[0].unicode())
    {
    case 'H':
        if(command == "HELP")
        {
            // snapcommunicator wants us to tell it what commands
            // we accept
            //
            snap::snap_communicator_message commands_message;
            commands_message.set_command("COMMANDS");
            commands_message.add_parameter("list", "HELP,INVALID,MANAGERACKNOWLEDGE,QUITTING,READY,SERVERSTATUS,STOP,UNKNOWN");
            send_message(commands_message);

            // now that we are fully registered, send the user message
            //
            send_message(f_message);

            return;
        }
        break;

    case 'M':
        if(command == "MANAGERACKNOWLEDGE")
        {
            // the snapmanagerdaemon tells us his server name, but it is
            // not really useful...
            //
            f_result = message.get_parameter("who");

            // we got at least one acknowledgement so the message was sent...
            // whether it worked on all computers (if broadcast to all) is a
            // different story!
            //
            if( message.has_parameter("done") || message.has_parameter("failed") )
            {
                mark_done();
            }
            return;
        }
        break;

    case 'Q':
        if(command == "QUITTING")
        {
            SNAP_LOG_WARNING("we received the QUITTING command while waiting for responses for snapmanager.cgi.");

            mark_done();
            return;
        }
        break;

    case 'R':
        if(command == "READY")
        {
            // the REGISTER worked, wait for the HELP message
            return;
        }
        break;

    case 'S':
        if(command == "STOP")
        {
            SNAP_LOG_WARNING("we received the STOP command while waiting for responses for snapmanager.cgi.");

            mark_done();
            return;
        }
        break;

    case 'U':
        if(command == "UNKNOWN")
        {
            // we sent a command that Snap! Communicator did not understand
            //
            SNAP_LOG_ERROR("we sent unknown command \"")(message.get_parameter("command"))("\" and probably did not get the expected result.");
            return;
        }
        break;

    }

    // unknown command is reported and process goes on
    //
    {
        SNAP_LOG_ERROR("unsupported command \"")(command)("\" was received by snapmanager.cgi on the connection with Snap! Communicator.");

        snap::snap_communicator_message unknown_message;
        unknown_message.set_command("UNKNOWN");
        unknown_message.add_parameter("command", command);
        send_message(unknown_message);
    }
}


}
// snap_manager namespace
// vim: ts=4 sw=4 et
