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
 * \brief The implementation of the content plugin class cache control parts.
 *
 * This file contains the implementation of the various content cache
 * control functions of the content plugin.
 */

// self
//
#include "content.h"


// snaplogger lib
//
#include <snaplogger/message.h>


// snapdev lib
//
#include <snapdev/not_reached.h>


// last include
//
#include <snapdev/poison.h>



namespace snap
{
namespace content
{



void content::set_cache_control_page(path_info_t & ipath)
{
    libdbproxy::table::pointer_t content_table(get_content_table());

    QString const cache_control(content_table->getRow(ipath.get_key())->getCell(get_name(name_t::SNAP_NAME_CONTENT_CACHE_CONTROL))->getValue().stringValue());

    // setup the page, we first reset it in case another page was set
    // here earlier
    f_snap->page_cache_control().reset_cache_info();
    f_snap->page_cache_control().set_cache_info(cache_control, true);
}



} // namespace content
} // namespace snap
// vim: ts=4 sw=4 et
