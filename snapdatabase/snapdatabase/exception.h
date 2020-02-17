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
 * \brief Snap! Database exceptions.
 *
 * This files declares a few exceptions that the database uses when a
 * parameter is wrong or something goes wrong (can't open a file, can't
 * create a lock for the context, etc.)
 *
 * The snapdatabase also makes use of the snaplogger so it will emit
 * a corresponding error to the log before throwing an exception.
 */


// libexcept lib
//
#include    <libexcept/exception.h>


namespace snapdatabase
{



DECLARE_LOGIC_ERROR(snapdatabase_logic_error);
DECLARE_LOGIC_ERROR(snapdatabase_not_yet_implemented);

DECLARE_OUT_OF_RANGE(snapdatabase_out_of_range);

DECLARE_MAIN_EXCEPTION(fatal_error);
DECLARE_MAIN_EXCEPTION(snapdatabase_error);

// uncomment as we use these
DECLARE_EXCEPTION(snapdatabase_error, block_full);
DECLARE_EXCEPTION(snapdatabase_error, block_not_found);
DECLARE_EXCEPTION(snapdatabase_error, column_not_found);
DECLARE_EXCEPTION(snapdatabase_error, conversion_unavailable);
DECLARE_EXCEPTION(snapdatabase_error, field_not_found);
DECLARE_EXCEPTION(snapdatabase_error, file_not_found);
DECLARE_EXCEPTION(snapdatabase_error, file_not_opened);
DECLARE_EXCEPTION(snapdatabase_error, id_already_assigned);
DECLARE_EXCEPTION(snapdatabase_error, id_missing);
DECLARE_EXCEPTION(snapdatabase_error, invalid_entity);
DECLARE_EXCEPTION(snapdatabase_error, invalid_name);
DECLARE_EXCEPTION(snapdatabase_error, invalid_number);
DECLARE_EXCEPTION(snapdatabase_error, invalid_parameter);
DECLARE_EXCEPTION(snapdatabase_error, invalid_size);
DECLARE_EXCEPTION(snapdatabase_error, invalid_token);
DECLARE_EXCEPTION(snapdatabase_error, invalid_xml);
DECLARE_EXCEPTION(snapdatabase_error, io_error);
DECLARE_EXCEPTION(snapdatabase_error, node_already_in_tree);
DECLARE_EXCEPTION(snapdatabase_error, page_not_found);
DECLARE_EXCEPTION(snapdatabase_error, row_already_exists);
DECLARE_EXCEPTION(snapdatabase_error, row_not_found);
DECLARE_EXCEPTION(snapdatabase_error, schema_not_found);
DECLARE_EXCEPTION(snapdatabase_error, string_not_terminated);
DECLARE_EXCEPTION(snapdatabase_error, type_mismatch);
DECLARE_EXCEPTION(snapdatabase_error, unexpected_eof);
DECLARE_EXCEPTION(snapdatabase_error, unexpected_token);
DECLARE_EXCEPTION(snapdatabase_error, out_of_bounds);



} // namespace snaplogger
// vim: ts=4 sw=4 et
