/*
 * Text:
 *      snapwebsites/snaplog/src/snaplog_timer.h
 *
 * Description:
 *      Logger for the Snap! system. This service uses snapcommunicator to listen
 *      to all "SNAPLOG" messages. It records each message into a MySQL database for
 *      later retrieval, making reporting a lot easier for the admin.
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

/** \brief Provide a tick in can we cannot immediately connect to Cassandra.
 *
 * The snaplog tries to connect to Cassandra on startup. It is part
 * of its initialization procedure.
 *
 * If that fails, it needs to try again later. This timer is used for
 * that purpose.
 */
class snaplog_timer
        : public snap::snap_communicator::snap_timer
{
public:
    typedef std::shared_ptr<snaplog_timer>    pointer_t;

    /** \brief The timer initialization.
     *
     * The timer ticks once per second to retrieve the current load of the
     * system and forward it to whichever computer that requested the
     * information.
     *
     * \param[in] cs  The snap communicator server we are listening for.
     *
     * \sa process_timeout()
     */
    snaplog_timer(snaplog * proxy);

    // snap::snap_communicator::snap_timer implementation
    virtual void process_timeout() override;

private:
    // this is owned by a server function so no need for a smart pointer
    snaplog *               f_snaplog = nullptr;
};


// vim: ts=4 sw=4 et
