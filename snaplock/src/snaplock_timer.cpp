/*
 * Text:
 *      snaplock/src/snaplock_timer.cpp
 *
 * Description:
 *      A daemon to synchronize processes between any number of computers
 *      by blocking all of them but one.
 *
 *      The timer implementation is used to know when a lock times out. The
 *      cleanup() function determines when the next timeout happens and saves
 *      that in this timer. When a timeout happens, the process calls cleanup()
 *      which removes all the old locks and makes sure the next ones get
 *      handled as expected.
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

/** \class snaplock_timer
 * \brief Handle the locks timeout.
 *
 * This class is used to time out locks. Whenever we receive a new LOCK
 * message or enter a lock the timer is reset with the next lock that is
 * going to time out. When that happens, the cleanup() function gets
 * called. Any lock which timed out is removed and the user on the other
 * end is told about the problem with an UNLOCKED or LOCKFAILED message
 * as the case may be.
 */



/** \brief The timer initialization.
 *
 * The timer is always enabled, however by default there is nothing to
 * timeout. In other words, the timer is keep off.
 *
 * \param[in] sl  The snaplock server we are hadnling time outs for.
 */
snaplock_timer::snaplock_timer(snaplock * sl)
    : snap_timer(-1)
    , f_snaplock(sl)
{
    set_name("snaplock timer");
}


/** \brief Call the cleanup() function of the snaplock object.
 *
 * A timeout happened, call the snaplock cleanup() function which takes
 * care of cleaning up the list of lock requests and existing locks.
 */
void snaplock_timer::process_timeout()
{
    // we simulate the STOP, so pass 'false' (i.e. not quitting)
    //
    f_snaplock->cleanup();
}


}
// snaplock namespace
// vim: ts=4 sw=4 et
