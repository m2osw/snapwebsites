// Snap Websites Servers -- glob a directory and enumerate the files
// Copyright (C) 2016-2017  Made to Order Software Corp.
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
//
#include "glob_dir.h"
#include "log.h"

namespace snap
{

/** \class Enumerate the contents of a directory (single level) using a wildcard.
 *
 * This class encapsulates and hides a Linux 'C' `glob_t` structure.
 * It allows enumeration of a single folder at the first level, using a path containing a wildcard.
 */

glob_dir::glob_dir()
{
    // Empty
}


glob_dir::glob_dir( QString const & path, int const flags )
{
    set_path( path, flags );
}


void glob_dir::set_path( QString const& path, int const flags )
{
    f_dir = glob_pointer_t( new glob_t );
    *f_dir = glob_t();
    int const r(glob(path.toUtf8().data(), flags, glob_err_callback, f_dir.get()));
    if(r != 0)
    {
        // do nothing when errors occur
        //
        QString err_msg;
        switch(r)
        {
            case GLOB_NOSPACE:
                err_msg = "glob() did not have enough memory to alllocate its buffers.";
                break;

            case GLOB_ABORTED:
                err_msg = "glob() was aborted after a read error.";
                break;

            case GLOB_NOMATCH:
                err_msg = "glob() could not find any status information.";
                break;

            default:
                err_msg = QString("unknown glob() error code: %1.").arg(r);
                break;
        }
        throw glob_dir_exception( r, err_msg );
    }
}


void glob_dir::enumerate_glob( std::function<void (QString path)> func )
{
    for(size_t idx(0); idx < f_dir->gl_pathc; ++idx)
    {
        func(f_dir->gl_pathv[idx]);
    }
}


int glob_dir::glob_err_callback(const char * epath, int eerrno)
{
    SNAP_LOG_ERROR("an error occurred while reading directory under \"")
        (epath)
        ("\". Got error: ")
        (eerrno)
        (", ")
        (strerror(eerrno))
        (".");

    // do not abort on a directory read error...
    return 0;
}


} // namespace snap

// vim: ts=4 sw=4 et
