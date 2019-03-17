/*
 * Text:
 *      snapdbproxy/src/snapdbproxy_initializer_thread.cpp
 *
 * Description:
 *      Handle a thread running a TCP/IP connection to Cassandra to proceed
 *      with various initializations (context and tables at the moment).
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



/** \brief Initialize the thread.
 *
 * This constructor initializes the thread runner with the specified
 * proxy, Cassandra host & port, and whether to use SSL to connect
 * to Cassandra.
 *
 * Once the constructor returns, the thread runner was created and
 * started.
 *
 * The thread is then started. If anything fails, except the start, you
 * are likely to get an exception. Otherwise the is_running() will
 * return false which means that you should not keep a copy of that
 * thread.
 */
snapdbproxy_initializer_thread::snapdbproxy_initializer_thread
    ( snapdbproxy * proxy
    , QString const & cassandra_host_list
    , int cassandra_port
    , bool use_ssl
    )
    : f_snapdbproxy(proxy)
    , f_initializer(proxy, cassandra_host_list, cassandra_port, use_ssl)
    , f_thread("initializer", &f_initializer)
{
    if(!f_thread.start())
    {
        SNAP_LOG_FATAL("could not start connection thread to handle CQL proxying");
        // do not throw so that way we do not just kill the snapdbproxy daemon
        // also that way we reach the destructor which closes the socket
    }
}


/** \brief Clean up the initializer thread.
 *
 * This function make sure to clean up the initializer thread.
 *
 * First it makes sure that the thread was terminated and then it closes
 * the socket associated with this thread.
 */
snapdbproxy_initializer_thread::~snapdbproxy_initializer_thread()
{
    // the kill() and stop() create a snap_lock which uses a snap_mutex and
    // the constructor and other functions eventually throw
    //

    // right now there is no much we can do "against" the initializer
    // however, it may be waiting for the status to change from STATUS_LOCK
    // to another status
    //
    if(f_snapdbproxy->get_status() == snapdbproxy::status_t::STATUS_LOCK)
    {
        SNAP_LOG_WARNING("stopping lock before it was obtained.");

        // the child process will throw on this one
        //
        f_snapdbproxy->set_status(snapdbproxy::status_t::STATUS_NO_LOCK);
    }

    try
    {
        f_thread.stop();
    }
    catch(snap::snap_thread_exception_mutex_failed_error const &)
    {
    }
    catch(snap::snap_thread_exception_invalid_error const &)
    {
    }
}


/** \brief Check whether the initializer thread is still running.
 *
 * A thread may die. We do not currently get a signal of any sort when
 * that happens. Instead we use this function to know whether the
 * thread is still running or not.
 *
 * \return true if the thread is still running.
 */
bool snapdbproxy_initializer_thread::is_running() const
{
    return f_thread.is_running();
}



// vim: ts=4 sw=4 et
