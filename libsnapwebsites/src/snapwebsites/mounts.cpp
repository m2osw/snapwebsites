// Snap Websites Server -- read mount points in Linux
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

#include "snapwebsites/mounts.h"

#include "snapwebsites/poison.h"


namespace snap
{



/** \brief Read all the mount points defined in file defined by \p path.
 *
 * This function reads all the mount points found in the file pointed by
 * \p path and adds them to this mounts object which is a vector.
 *
 * You have full access to the vector once the mounts constructor returns
 * so you can add or delete entries if you want to.
 *
 * \param[in] path  The path to a fstab file such as "/etc/fstab" or
 *                  "/proc/mounts".
 */
mounts::mounts(std::string const& path)
    : f_path(path)
{
    struct mntent   *m;
    FILE            *in;

    in = setmntent(path.c_str(), "r");
    if(in == nullptr)
    {
        throw snap_mounts_exception_io_error("mounts() cannot open \"/proc/mounts\"");
    }

    while(m = getmntent(in))
    {
        push_back(mount_entry(m));
    }

    endmntent(in);
}




} // namespace snap

// vim: ts=4 sw=4 et
