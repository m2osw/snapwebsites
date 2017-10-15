/*
 * Text:
 *      snapwebsites/snapdbproxy/src/snapdbproxy_interrupt.cpp
 *
 * Description:
 *      The interrupt implementation listens for the Ctrl-C or SIGINT Unix
 *      signal. When the signal is received, it calls the stop function
 *      of the snapdbproxy object to simulate us receiving a STOP message.
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



/** \class snapdbproxy_interrupt
 * \brief Handle the SIGINT Unix signal.
 *
 * This class is an implementation of the signalfd() specifically
 * listening for the SIGINT signal.
 */



/** \brief The interrupt initialization.
 *
 * The interrupt uses the signalfd() function to obtain a way to listen on
 * incoming Unix signals.
 *
 * Specifically, it listens on the SIGINT signal, which is the equivalent
 * to the Ctrl-C.
 *
 * \param[in] s  The snapdbproxy server we are listening for.
 */
snapdbproxy_interrupt::snapdbproxy_interrupt(snapdbproxy * s)
    : snap_signal(SIGINT)
    , f_snapdbproxy(s)
{
    unblock_signal_on_destruction();
    set_name("snapdbproxy interrupt");
}


/** \brief Call the stop function of the snapdbproxy object.
 *
 * When this function is called, the signal was received and thus we are
 * asked to quit as soon as possible.
 */
void snapdbproxy_interrupt::process_signal()
{
    // we simulate the STOP, so pass 'false' (i.e. not quitting)
    //
    f_snapdbproxy->stop(false);
}


// vim: ts=4 sw=4 et
