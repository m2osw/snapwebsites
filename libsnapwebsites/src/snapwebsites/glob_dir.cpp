// Snap Websites Servers -- glob a directory and enumerate the files
// Copyright (c) 2016-2019  Made to Order Software Corp.  All Rights Reserved
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
//

// self
//
#include "glob_dir.h"

// snapwebsites lib
//
#include "log.h"


// last include
//
#include <snapdev/poison.h>



namespace snap
{


namespace
{


int glob_err_callback(char const * epath, int eerrno)
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


}
// no name namespace


/** \class glob_dir
 * \brief Enumerate the contents of a directory using a wildcard.
 *
 * This class encapsulates and hides a Unix 'C' `glob_t` structure.
 *
 * It allows enumeration of a single folder at the first level
 * using a path containing a Unix shell compatible wildcard.
 */


/** \brief Create an empty directory.
 *
 * The default constructor creates an empty directory. You are expected
 * to call the set_path() function at least once before attempting to
 * enumerate files.
 *
 * \sa set_path()
 */
glob_dir::glob_dir()
{
    // Empty
}


/** \brief Create a glob_dir from a path and flags.
 *
 * This constructor calls set_path() immediately using the two parameters
 * passed to it.
 *
 * \param[in] path  The path including the Unix shell wildcards.
 * \param[in] flags  A set of GLOB_... flags.
 * \param[in] allow_empty  Just return on an empty glob().
 *
 * \sa set_path()
 */
glob_dir::glob_dir( char const * path, int const flags, bool allow_empty )
{
    set_path( path, flags, allow_empty );
}


/** \brief Create a glob_dir from a path and flags.
 *
 * This constructor calls set_path() immediately using the two parameters
 * passed to it.
 *
 * \param[in] path  The path including the Unix shell wildcards.
 * \param[in] flags  A set of GLOB_... flags.
 * \param[in] allow_empty  Just return on an empty glob().
 *
 * \sa set_path()
 */
glob_dir::glob_dir( std::string const & path, int const flags, bool allow_empty )
{
    set_path( path, flags, allow_empty );
}


/** \brief Create a glob_dir from a path and flags.
 *
 * This constructor calls set_path() immediately using the two parameters
 * passed to it.
 *
 * \param[in] path  The path including the Unix shell wildcards.
 * \param[in] flags  A set of GLOB_... flags.
 * \param[in] allow_empty  Just return on an empty glob().
 *
 * \sa set_path()
 */
glob_dir::glob_dir( QString const & path, int const flags, bool allow_empty )
{
    set_path( path, flags, allow_empty );
}


/** \brief Set the path to read with glob().
 *
 * This function is an overload which accepts a bare pointer as input.
 *
 * \param[in] path  The path including the Unix shell wildcards.
 * \param[in] flags  A set of GLOB_... flags.
 * \param[in] allow_empty  Just return on an empty glob().
 */
void glob_dir::set_path( char const * path, int const flags, bool allow_empty )
{
    if(path != nullptr)
    {
        set_path(std::string(path), flags, allow_empty);
    }
}


/** \brief Set the path to read with glob().
 *
 * This function passes the \p path parameter to the glob() function and
 * saves the results in an internally managed glob_t structure.
 *
 * The flags are as specified in the glob(3) function (try `man glob`).
 *
 * The path is expected to already include a wildcard. Without a wildcard,
 * it probably won't work as expected.
 *
 * The function doesn't return anything. Instead it will memorize the
 * results and enumerate them once the enumerate_glob() function is called.
 *
 * \note
 * Do not worry about the globfree(), this class handles that part internally.
 *
 * \todo
 * Offer another function to retrieve a vector of strings instead of only an
 * enumeration function.
 *
 * \param[in] path  The path including the Unix shell wildcards.
 * \param[in] flags  A set of GLOB_... flags.
 * \param[in] allow_empty  Just return on an empty glob().
 */
void glob_dir::set_path( std::string const & path, int const flags, bool allow_empty )
{
    f_dir = glob_pointer_t( new glob_t );
    *f_dir = glob_t();
    int const r(glob(path.c_str(), flags, glob_err_callback, f_dir.get()));
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
            if(allow_empty)
            {
                return;
            }
            err_msg = QString("glob() could not find any files matching the specified glob pattern: \"%1\".")
                                .arg(QString::fromUtf8(path.c_str()));
            break;

        default:
            err_msg = QString("unknown glob() error code: %1.").arg(r);
            break;

        }
        throw glob_dir_exception( r, err_msg );
    }
}


/** \brief Set the path to read with glob().
 *
 * This function is an overload with a QString. See the set_path() function
 * with an std::string for more details.
 *
 * \param[in] path  The path including the Unix shell wildcards.
 * \param[in] flags  A set of GLOB_... flags.
 * \param[in] allow_empty  Just return on an empty glob().
 */
void glob_dir::set_path( QString const & path, int const flags, bool allow_empty )
{
    set_path(path.toUtf8().data(), flags, allow_empty);
}


/** \brief Enumerate full filenames with an std::string.
 *
 * This function enumerates all the filenames found in this glob
 * calling your callback once per file. This function expects
 * an std::string. You can also enumerate using a QString.
 *
 * \param[in] func  The function to call on each filename.
 */
void glob_dir::enumerate_glob( std::function<void (std::string path)> func ) const
{
    if(f_dir != nullptr)
    {
        for(size_t idx(0); idx < f_dir->gl_pathc; ++idx)
        {
            func(f_dir->gl_pathv[idx]);
        }
    }
}


/** \brief Enumerate full filenames with a QString.
 *
 * This function enumerates all the filenames found in this glob
 * calling your callback once per file. This function expects
 * a QString. You can also enumerate using an std::string.
 *
 * \param[in] func  The function to call on each filename.
 */
void glob_dir::enumerate_glob( std::function<void (QString path)> func ) const
{
    if(f_dir != nullptr)
    {
        for(size_t idx(0); idx < f_dir->gl_pathc; ++idx)
        {
            func(QString::fromUtf8(f_dir->gl_pathv[idx]));
        }
    }
}




} // namespace snap
// vim: ts=4 sw=4 et
