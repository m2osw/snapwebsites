/*
 * Text:
 *      snapdbproxy/src/snaplock_nocassandra.cpp
 *
 * Description:
 *      Handling the SIGUSR1 whenever a child (thread) detects that it
 *      lost its connection.
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



/** \class snaplock_nocassandra
 * \brief Handle the SIGUSR1 Unix signal.
 *
 * This class is an implementation of the signalfd() specifically
 * listening for the SIGUSR1 signal.
 */



/** \brief The nocassandra initialization.
 *
 * The nocassandra uses the signalfd() function to obtain a way to listen on
 * incoming SIGUSR1 signals.
 *
 * \param[in] s  The snapdbproxy server we are listening for.
 */
snapdbproxy_nocassandra::snapdbproxy_nocassandra(snapdbproxy * s)
    : snap_signal(SIGUSR1)
    , f_snapdbproxy(s)
{
    unblock_signal_on_destruction();
    set_name("snapdbproxy nocassandra");
}


/** \brief Call the no_cassandra() function of the snapdbproxy object.
 *
 * When this function is called, the signal was received and thus we are
 * asked to reconnect to Cassandra as soon as convenient.
 */
void snapdbproxy_nocassandra::process_signal()
{
    f_snapdbproxy->no_cassandra();
}


// vim: ts=4 sw=4 et
