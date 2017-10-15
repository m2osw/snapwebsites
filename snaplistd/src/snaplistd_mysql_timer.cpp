/*
 * Text:
 *      snapwebsites/snaplistd/src/snaplistd_mysql_timer.cpp
 *
 * Description:
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
#include "snaplistd.h"

// our lib
//
#include <snapwebsites/log.h>


/** \class snaplistd_mysql_timer
 * \brief Timer used to handle reconnecting to MySQL.
 *
 * This timer is used to get a signal so the list daemon can reconnect
 * to MySQL. If any error occurs with MySQL, we disconnect and reconnect.
 */

snaplistd_mysql_timer::snaplistd_mysql_timer(snaplistd * listd)
    : snap_timer(0)  // run immediately
    , f_snaplistd(listd)
{
    set_name("snaplistd_mysql timer");
}


void snaplistd_mysql_timer::process_timeout()
{
    f_snaplistd->process_timeout();
}



// vim: ts=4 sw=4 et
