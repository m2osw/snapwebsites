/*
 * Text:
 *      snaplock/src/snaplock_debug_info.cpp
 *
 * Description:
 *      A daemon to synchronize processes between any number of computers
 *      by blocking all of them but one.
 *
 *      The interrupt implementation listens for the Ctrl-C or SIGINT Unix
 *      signal. When the signal is received, it calls the stop function
 *      of the snaplock object to simulate us receiving a STOP message.
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
#include "snaplock.h"

// our lib
//
#include <snapwebsites/log.h>


namespace snaplock
{

/** \class snaplock_debug_info
 * \brief Handle the SIGUSR2 Unix signal.
 *
 * This class is an implementation of the signalfd() specifically
 * listening for the SIGUSR2 signal.
 *
 * The signal is used to ask snaplock to do some debugging tasks.
 * This is used by developers as they test the tool and it is not
 * actually used in non-debug versions.
 */




/** \brief The snaplock info initialization.
 *
 * The snaplock information object uses the signalfd() function to
 * obtain a way to listen on incoming Unix signals.
 *
 * Specifically, it listens on the SIGUSR1 signal. This is used to
 * request snaplock to print out its current state. This is mainly
 * for debug purposes.
 *
 * \param[in] sl  The snaplock we are listening for.
 */
snaplock_debug_info::snaplock_debug_info(snaplock * sl)
    : snap_signal(SIGUSR2)
    , f_snaplock(sl)
{
    unblock_signal_on_destruction();
    set_name("snap debug info");
}


/** \brief Call the info function of the snaplock object.
 *
 * When this function is called, the debug_info() function of
 * snaplock gets called and it can process whatever the programmer
 * wants to debug at the time.
 */
void snaplock_debug_info::process_signal()
{
    // we simulate the STOP, so pass 'false' (i.e. not quitting)
    //
    f_snaplock->debug_info();
}






}
// snaplock namespace
// vim: ts=4 sw=4 et
