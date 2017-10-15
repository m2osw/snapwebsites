/*
 * Text:
 *      status_connection.cpp
 *
 * Description:
 *      The implementation of the status connection between the main
 *      application and the status thread (an inter-thread connection).
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
#include "snapmanagerdaemon.h"

// snapwebsites lib
//
#include <snapwebsites/log.h>



namespace snap_manager
{


status_connection::status_connection(manager_daemon * md)
    : f_manager_daemon(md)
{
    set_name("snapmanagerdaemon status connection");
}


void status_connection::set_thread_b(manager_status * ms)
{
    f_manager_status = ms;

    // The thread gets started "way" later so we do not need such a message
    //snap::snap_communicator_message thread_ready;
    //thread_ready.set_command("THREADREADY");
    //send_message(thread_ready);
}


/** \brief Save the server name in the status connection.
 *
 * In order for the status connection to know whether a message it is
 * processing needs to go to the manager daemon or needs to be
 * forwarded, it needs to know the name of the server.
 *
 * So, if a message has its server name field defined and it is equal
 * to the server name defined by this function, the message is expected
 * to be processed by this snapmanagerdaemon. Otherwise it gets forwarded
 * to with all or one specific service on that other server.
 *
 * \param[in] server_name
 */
void status_connection::set_server_name(QString const & server_name)
{
    f_server_name = server_name;
}


void status_connection::process_message_a(snap::snap_communicator_message const & message)
{
    // here we just received a message from the thread...
    //
    // if that message is MANAGERSTATUS, then it is expected to
    // be sent to all the computers in the cluster, not just this
    // computer, only the inter-thread connection does not allow
    // for broadcasting (i.e. the message never leaves the
    // snapmanagerdaemon process with that type of connection!)
    //
    // so here we check for the name of the service to where the
    // message is expected to arrive; if not empty, we instead
    // send the message to snapcommunicator
    //
    if((message.get_server().isEmpty() || message.get_server() == f_server_name)
    && (message.get_service().isEmpty() || message.get_service() == "snapmanagerdaemon"))
    {
        // in this case the message will not automatically get the
        // sent from server and service parameters defined
        //
        snap::snap_communicator_message copy(message);
        copy.set_sent_from_server(f_server_name);
        copy.set_sent_from_service("snapmanagerdaemon");
        f_manager_daemon->process_message(copy);
    }
    else
    {
        // forward to snapcommunicator instead
        //
        f_manager_daemon->forward_message(message);
    }
}


void status_connection::process_message_b(snap::snap_communicator_message const & message)
{
    f_manager_status->process_message(message);
}



} // namespace snap_manager
// vim: ts=4 sw=4 et
