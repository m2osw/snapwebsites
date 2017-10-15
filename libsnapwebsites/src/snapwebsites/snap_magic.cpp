// Snap Websites Servers -- check the MIME type of a file using the magic database
// Copyright (C) 2013-2017  Made to Order Software Corp.
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

#include "snapwebsites/snap_magic.h"

#include <magic.h>

#include "snapwebsites/poison.h"


namespace snap
{

namespace
{
magic_t         g_magic = NULL;
}


/** \brief Generate a MIME type from a QByteArray.
 *
 * This static function transforms the content of a QByteArray in a
 * MIME type using the magic library. The magic library is likely to
 * understand all the files that a user is to upload on a Snap!
 * website (within reason, of course.)
 *
 * This function runs against the specified buffer (\p data) from
 * memory since in Snap! we do pretty much everything in memory.
 *
 * The function returns computer MIME types such as text/html or
 * image/png.
 *
 * \note
 * Compressed files get their type determined after decompression.
 *
 * \exception snap_magic_exception_no_magic
 * The function generates an exception if it cannot access the
 * magic library (the magic_open() function fails.)
 *
 * \param[in] data  The buffer to be transformed in a MIME type.
 */
QString get_mime_type(const QByteArray& data)
{
    if(g_magic == NULL)
    {
        g_magic = magic_open(MAGIC_COMPRESS | MAGIC_MIME);
        if(g_magic == NULL)
        {
            throw snap_magic_exception_no_magic("Magic MIME type cannot be opened (magic_open() failed)");
        }
        // load the default magic database
        magic_load(g_magic, NULL);
    }

    return magic_buffer(g_magic, data.data(), data.length());
}

}
// vim: ts=4 sw=4 et
