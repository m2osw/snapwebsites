// Copyright (c) 2019  Made to Order Software Corp.  All Rights Reserved
//
// https://snapwebsites.org/project/snapdatabase
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
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#pragma once


/** \file
 * \brief Script handling file header.
 *
 * The script feature is used to handle data transformation and filtering
 * for secondary filters (primarily).
 */

// self
//
#include    "snapdatabase/database/row.h"
#include    "snapdatabase/data/virtual_buffer.h"



namespace snapdatabase
{



buffer_t        compile_script(std::string const & script);
buffer_t        execute_script(buffer_t compiled_script, row::pointer_t row);



} // namespace snapdatabase
// vim: ts=4 sw=4 et
