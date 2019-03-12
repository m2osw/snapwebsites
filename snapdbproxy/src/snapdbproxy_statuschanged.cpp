/*
 * Text:
 *      snapdbproxy/src/snaplock_statuschanged.cpp
 *
 * Description:
 *      Handling the SIGUSR2 whenever the initializer thread wants to get
 *      a LOCK or it is done with its work (see f_status).
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



/** \class snaplock_statuschanged
 * \brief Handle the SIGUSR2 Unix signal.
 *
 * This class is an implementation of the signalfd() specifically
 * listening for the SIGUSR2 signal.
 */



/** \brief The statuschanged initialization.
 *
 * The statuschanged uses the signalfd() function to obtain a way to listen on
 * incoming SIGUSR2 signals.
 *
 * \param[in] s  The snapdbproxy server we are listening for.
 */
snapdbproxy_statuschanged::snapdbproxy_statuschanged(snapdbproxy * s)
    : snap_signal(SIGUSR2)
    , f_snapdbproxy(s)
{
    unblock_signal_on_destruction();
    set_name("snapdbproxy statuschanged");
}


/** \brief Call the status_changed() function of the snapdbproxy object.
 *
 * When this function is called, the signal was received and thus we are
 * asked to either LOCK the database or start accepting proxy connections.
 */
void snapdbproxy_statuschanged::process_signal()
{
    f_snapdbproxy->status_changed();
}


// vim: ts=4 sw=4 et
