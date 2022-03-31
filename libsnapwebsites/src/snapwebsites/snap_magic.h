// Snap Websites Servers -- check the MIME type of a file using the magic database
// Copyright (c) 2013-2019  Made to Order Software Corp.  All Rights Reserved
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
#pragma once

// libexcept
//
#include    <libexcept/exception.h>



namespace snap
{

DECLARE_MAIN_EXCEPTION(snap_magic_exception);

DECLARE_EXCEPTION(snap_magic_exception, snap_magic_exception_no_magic);


// the actual function that generates a MIME type from a byte array
//
QString     get_mime_type(const QByteArray & data);

}
// vim: ts=4 sw=4 et
