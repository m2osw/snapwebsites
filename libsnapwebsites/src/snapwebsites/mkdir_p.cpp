// Snap Websites Server -- like shell `mkdir -p ...`
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


// self
//
#include "snapwebsites/mkdir_p.h"


// snapwebsites lib
//
#include "snapwebsites/chownnm.h"
#include "snapwebsites/log.h"
#include "snapwebsites/qcompatibility.h"
#include "snapwebsites/snap_exception.h"
#include "snapwebsites/snap_string_list.h"


// snapdev lib
//
#include <snapdev/not_reached.h>
#include <snapdev/not_used.h>


// C++ lib
//
#include <sstream>


// C lib
//
#include <sys/stat.h>
#include <sys/types.h>


// last include
//
#include <snapdev/poison.h>



namespace snap
{


/** \brief Create directory as with `mkdir -p ...`.
 *
 * This function creates all the directories so one can create a file
 * under the deepest directory specified in \p path.
 *
 * If \p path includes the filename, then make sure to set
 * \p include_filename parameter to true. That way this function
 * ignores that name.
 *
 * You make also pass the mode and owner/group details for the new
 * or existing directory. The function ignores these parameters if
 * set to their default (0 for mode and an empty string for owner
 * and group.)
 *
 * The default mode of 0755 is used when creating a directory if you
 * used 0 as the mode. In most cases, acceptable modes are:
 *
 * \li 0700
 * \li 0770
 * \li 0775
 * \li 0750
 * \li 0755
 *
 * Other modes are likely to not be useful for a directory.
 *
 * The owner and group parameters can be set to specific user and group
 * names. These names must existing in /etc/passwd and /etc/group. When
 * both are empty strings, chown() is not called.
 *
 * The function accepts paths with double slashes as if there was
 * just one (i.e. "/etc//snapwebsites" is viewed as "/etc/snapwebsites".)
 * This is the standard Unix behavior.
 *
 * The function returns -1 if one or more of the directories cannot
 * be created or adjusted according to the parameters. It also logs
 * a message to your log file specifying which directory could not
 * be created.
 *
 * If the function returns -1, then errno is set to the error returned
 * by the last mkdir(), chmod(), or chown() that failed.
 *
 * \note
 * The two main errors when this function fails are: (1) the directory
 * cannot be created because you do not have enough permissions; and
 * (2) the named directory exists in the form of a file.
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
 * \param[in] mode  The new directories mode if not zero.
 * \param[in] owner  The new directories owner.
 * \param[in] group  The new directories group.
 *
 * \return 0 if the directory exists at the time the function returns,
 *         -1 if an error occurs (i.e. permissions denied)
 */
int mkdir_p(QString const & path
          , bool include_filename
          , int mode
          , QString const & owner
          , QString const & group)
{
    // we skip empty parts since "//" is the same as "/" in a Unix path.
    //
    snap::snap_string_list segments(snap::split_string(path, '/'));
    if(segments.empty())
    {
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
            //
            if(S_ISDIR(s.st_mode))
            {
                // make sure the last segment (directory we are really
                // expected to create) has the correct mode and ownership
                //
                if(idx + 1 == max_segments)
                {
                    if(mode != 0)
                    {
                        if(chmod(p.toUtf8().data(), mode) != 0)
                        {
                            int const e(errno);
                            std::stringstream m;
                            m << "0" << std::oct << mode;
                            SNAP_LOG_DEBUG("could not change directory \"")
                                          (p)
                                          ("\" permissions to \"")
                                          (m.str())
                                          ("\". (errno: ")
                                          (e)
                                          (" -- ")
                                          (strerror(e));
                        }
                    }
                    if(chownnm(p, owner, group) != 0)
                    {
                        int const e(errno);
                        SNAP_LOG_DEBUG("could not change directory \"")
                                      (p)
                                      ("\" ownership to \"")
                                      (owner)
                                      (":")
                                      (group)
                                      ("\". (errno: ")
                                      (e)
                                      (" -- ")
                                      (strerror(e));
                    }
                }
                continue;
            }

            // not a directory, that is an error
            //
            int const e(EEXIST);
            SNAP_LOG_ERROR("could not create directory \"")
                          (p)
                          ("\" since a file, which is not a directory, of the same name exists. (errno: ")
                          (e)
                          (" -- ")
                          (strerror(e));
            errno = e;
            return -1;
        }

        // attempt creating
        //
        if(mkdir(p.toUtf8().data(), mode == 0 ? 0755 : mode) != 0)
        {
            int const e(errno);
            SNAP_LOG_ERROR("could not create directory \"")
                          (p)
                          ("\". (errno: ")
                          (e)
                          (" -- ")
                          (strerror(e));
            errno = e;
            return -1;
        }

        // directories we create are also assigned owner/group
        //
        if(chownnm(p, owner, group) != 0)
        {
            int const e(errno);
            SNAP_LOG_DEBUG("could not change directory \"")
                          (p)
                          ("\" ownership to \"")
                          (owner)
                          (":")
                          (group)
                          ("\". (errno: ")
                          (e)
                          (" -- ")
                          (strerror(e));
        }
    }

    return 0;
}


int mkdir_p(std::string const & path
          , bool include_filename
          , int mode
          , std::string const & owner
          , std::string const & group)
{
    return mkdir_p(QString::fromUtf8(path.c_str())
                 , include_filename
                 , mode
                 , QString::fromUtf8(owner.c_str())
                 , QString::fromUtf8(group.c_str()));
}


int mkdir_p(char const * path
          , bool include_filename
          , int mode
          , char const * owner
          , char const * group)
{
    return mkdir_p(QString::fromUtf8(path)
                 , include_filename
                 , mode
                 , QString::fromUtf8(owner == nullptr ? "" : owner)
                 , QString::fromUtf8(group == nullptr ? "" : group));
}


} // snap namespace
// vim: ts=4 sw=4 et
