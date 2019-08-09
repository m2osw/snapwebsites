// Snap Websites Server -- snap exception handling
// Copyright (c) 2014-2019  Made to Order Software Corp.  All Rights Reserved
//
// https://snapwebsites.org/
// contact@m2osw.com
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA


// self
//
#include "snapwebsites/snap_exception.h"


// snapwebsites lib
//
#include "snapwebsites/log.h"


// libexcept lib
//
#include <libexcept/exception.h>


// C++ lib
//
#include <iostream>


// C lib
//
#include <execinfo.h>
#include <unistd.h>


// last include
//
#include <snapdev/poison.h>





namespace snap
{


/** \brief Initialize this Snap! exception.
 *
 * Initialize the base exception class. Output a stack trace to the error log.
 *
 * \param[in] what_msg  The exception message.
 *
 * \sa output_stack_trace()
 */
snap_exception_base::snap_exception_base(char const * what_msg)
{
    SNAP_LOG_ERROR("snap_exception: ")(what_msg);
    output_stack_trace();
}


/** \brief Initialize this Snap! exception.
 *
 * Initialize the base exception class. Output a stack trace to the error log.
 *
 * \param[in] what_msg  The exception message.
 *
 * \sa output_stack_trace()
 */
snap_exception_base::snap_exception_base(std::string const & what_msg)
{
    SNAP_LOG_ERROR("snap_exception: ")(what_msg);
    output_stack_trace();
}


/** \brief Initialize this Snap! exception.
 *
 * Initialize the base exception class. Output a stack trace to the error log.
 *
 * \param[in] what_msg  The exception message.
 *
 * \sa output_stack_trace()
 */
snap_exception_base::snap_exception_base(QString const & what_msg)
{
    SNAP_LOG_ERROR("snap_exception: ")(what_msg);
    output_stack_trace();
}


/** \brief Output stack trace to log as an error.
 *
 * This static method outputs the current stack as a trace to the log. If
 * compiled with DEBUG turned on, it will also output to the stderr.
 *
 * By default, the stack trace shows you a number of backtrace equal
 * to STACK_TRACE_DEPTH (which is 20 at time of writing). You may
 * specify another number to get more or less lines. Note that a
 * really large number will generally show you the entire stack since
 * a number larger than the number of function pointers on the stack
 * will return the entire stack.
 *
 * \param[in] stack_trace_depth  The number of lines to output in our stack track.
 */
void snap_exception_base::output_stack_trace( int const stack_trace_depth )
{
    libexcept::exception_base_t eb( stack_trace_depth );

    for( auto const & stack_line : eb.get_stack_trace() )
    {
        SNAP_LOG_ERROR("snap_exception_base(): backtrace=")( stack_line );
    }
}


} // namespace snap
// vim: ts=4 sw=4 et
