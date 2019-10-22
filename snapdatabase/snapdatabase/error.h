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
 * \brief Database file header.
 *
 * The block base class handles the loading of the block in memory using
 * mmap() and gives information such as its type and location.
 */

// lib snapdatabase
//
#include    "snapdatabase/context.h"



namespace snapdatabase
{


enum error_code_t
{
    ERROR_CODE_NO_ERROR,
    ERROR_CODE_INVALID_XML,
};


class error
{
public:
    typedef std::shared_ptr<error>  pointer_t;

                                    error(
                                          error_code_t code
                                        , std::string const & message);

    error_code_t                    get_error_code() const;
    std::string                     get_error_message() const;

private:
    error_code_t                    f_error_code = error_code_t::ERROR_CODE_NO_ERROR;
    std::string                     f_message = std::string();
};



} // namespace snapdatabase
// vim: ts=4 sw=4 et
