// Snap Websites Server -- like shell `mkdir -p ...`
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

#include "snapwebsites/mkdir_p.h"

#include "snapwebsites/log.h"
#include "snapwebsites/not_reached.h"
#include "snapwebsites/not_used.h"
#include "snapwebsites/snap_exception.h"
#include "snapwebsites/snap_string_list.h"

#include <sys/stat.h>
#include <sys/types.h>

#include "snapwebsites/poison.h"


namespace snap
{


/** \brief Create directory as with `mkdir -p ...`.
 *
 * This function creates all the directory so one can create a file
 * under the deepest directory specified in \p path.
 *
 * If \p path includes the filename, then make sure to set
 * \p include_filename to true. That way the function ignores
 * that name.
 *
 * The function accepts paths with double slashes as if there was
 * just one (i.e. "/etc//snapwebsites" is viewed as "/etc/snapwebsites".)
 * This is the standard Unix behavior.
 *
 * The function returns -1 if one or more of the directories cannot
 * be created. It also logs a message to your log file specifying which
 * directory could not be created.
 *
 * If the function returns -1, then errno is set to the error returned
 * by the last mkdir() that failed.
 *
 * \note
 * The two main errors when this function fails are: (1) the directory
 * cannot be created because you do not have enough permissions; and
 * (2) the named directory exists in the form of a file.
 *
 * \todo
 * The function should offer a way to set the ownership and permissions
 * of each file.
 *
 * \bug
 * Many of the default directories that we need to have to run our
 * servers are to be created in directories that are owned by root.
 * This causes problems when attempting to run Snap! executables
 * as a developer.
 *
 * \param[in] path  The path to create.
 * \param[in] include_filename  If true, the \p path parameter also
 *            includes a filename.
 *
 * \return 0 if the directory exists at the time the function returns,
 *         -1 if an error occurs (i.e. permissions denied)
 */
int mkdir_p(QString const & path, bool include_filename)
{
    // we skip empty parts since "//" is the same as "/" in a Unix path.
    //
    snap::snap_string_list segments(path.split('/', QString::SkipEmptyParts));
    if(segments.empty())
    {
        // root always exists
        return 0;
    }

    if(include_filename)
    {
        segments.pop_back();
    }

    QString p;
    int const max_segments(segments.size());
    for(int idx(0); idx < max_segments; ++idx)
    {
        // compute path
        p += "/";
        p += segments[idx];

        // already exists?
        struct stat s;
        if(stat(p.toUtf8().data(), &s) == 0)
        {
            // the file exists, it is a directory?
            if(S_ISDIR(s.st_mode))
            {
                // TODO: check access rights over this directory...
                continue;
            }
            // not a directory, that is an error
            int const e(EEXIST);
            SNAP_LOG_ERROR("could not create directory \"")(p)("\" since a file, which is not a directory, of the same name exists. (errno: ")(e)(" -- ")(strerror(e));
            errno = e;
            return -1;
        }

        // attempt creating
        if(mkdir(p.toUtf8().data(), 0755) != 0)
        {
            int const e(errno);
            SNAP_LOG_ERROR("could not create directory \"")(p)("\". (errno: ")(e)(" -- ")(strerror(e));
            errno = e;
            return -1;
        }
    }

    return 0;
}


} // snap namespace
// vim: ts=4 sw=4 et
