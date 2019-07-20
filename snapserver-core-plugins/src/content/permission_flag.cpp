// Snap Websites Server -- all the user content and much of the system content
// Copyright (c) 2011-2019  Made to Order Software Corp.  All Rights Reserved
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


/** \file
 * \brief The implementation of the content plugin permissions_flag class.
 *
 * This file contains the permissions_flag class implementation.
 */


// self
//
#include "content.h"


// snapwebsites lib
//
#include <snapwebsites/log.h>


// last include
//
#include <snapdev/poison.h>




SNAP_PLUGIN_EXTENSION_START(content)


/** \brief Set the permission and reason for refusal.
 *
 * This function marks the permission flag as not permitted (i.e. it
 * sets it to false.) The default value of the permission flag is
 * true. Note that once this function was called once it is not possible
 * to set the flag back to true.
 *
 * \param[in] new_reason  The reason for the refusal, can be set to "".
 */
void permission_flag::not_permitted(QString const & new_reason)
{
    f_allowed = false;

    if(!new_reason.isEmpty())
    {
        if(!f_reason.isEmpty())
        {
            f_reason += "\n";
        }
        // TBD: should we prevent "\n" in "new_reason"?
        f_reason += new_reason;
    }
}





SNAP_PLUGIN_EXTENSION_END()

// vim: ts=4 sw=4 et
