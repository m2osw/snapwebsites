/*
 * Text:
 *      snapwebsites/snaplog/src/snaplog_messenger.h
 *
 * Description:
 *      Logger for the Snap! system. This service uses snapcommunicator to
 *      listen to all "LOG" messages. It records each message into a MySQL
 *      database for later retrieval, making reporting a lot easier for
 *      the admin.
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

// snapwebsites lib
//
#include <snapwebsites/snapwebsites.h>
#include <snapwebsites/snap_communicator.h>

// advgetopt lib
//
#include <advgetopt/advgetopt.h>


class snaplog;


class snaplog_messenger
        : public snap::snap_communicator::snap_tcp_client_permanent_message_connection
{
public:
    typedef std::shared_ptr<snaplog_messenger>    pointer_t;

                                snaplog_messenger(snaplog * proxy, std::string const & addr, int port);

    // snap::snap_communicator::snap_tcp_client_permanent_message_connection implementation
    virtual void                process_message(snap::snap_communicator_message const & message);
    virtual void                process_connection_failed(std::string const & error_message);
    virtual void                process_connected();

private:
    // this is owned by a snaplog function so no need for a smart pointer
    // (and it would create a loop)
    snaplog *               f_snaplog = nullptr;
};


// vim: ts=4 sw=4 et
