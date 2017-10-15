/*
 * Text:
 *      snapwebsites/snaplog/src/snaplog.h
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
#pragma once

#include "snaplog_interrupt.h"
#include "snaplog_messenger.h"
#include "snaplog_timer.h"

// snapwebsites lib
//
#include <snapwebsites/snapwebsites.h>
#include <snapwebsites/snap_communicator.h>

// advgetopt lib
//
#include <advgetopt/advgetopt.h>

// C++
//
#include <atomic>


class snaplog
{
public:
    typedef std::shared_ptr<snaplog>      pointer_t;

                                snaplog( int argc, char * argv[] );
                                ~snaplog();

    std::string                 server_name() const;

    void                        run();
    void                        process_message(snap::snap_communicator_message const & message);
    void                        process_timeout();
    void                        stop(bool quitting);

    static void                 sighandler( int sig );

private:
                                snaplog( snaplog const & ) = delete;
    snaplog &                   operator = ( snaplog const & ) = delete;

    void                        usage(advgetopt::getopt::status_t status);

    void                        add_message_to_db( snap::snap_communicator_message const & message );
    void                        mysql_ready();
    void                        no_mysql();

    static pointer_t                        g_instance;

    advgetopt::getopt                       f_opt;
    snap::snap_config                       f_config;
    QString                                 f_log_conf                  = "/etc/snapwebsites/logger/snaplog.properties";
    QString                                 f_communicator_addr         = "127.0.0.1";
    int                                     f_communicator_port         = 4040;
    std::string                             f_server_name;
    snap::snap_communicator::pointer_t      f_communicator;
    snaplog_interrupt::pointer_t            f_interrupt;
    snaplog_messenger::pointer_t            f_messenger;
    snaplog_timer::pointer_t                f_timer;
    bool                                    f_ready                     = false;
    bool                                    f_force_restart             = false;
    bool                                    f_stop_received             = false;
    bool                                    f_debug                     = false;
    float                                   f_mysql_connect_timer_index = 1.625f;
};



// vim: ts=4 sw=4 et
