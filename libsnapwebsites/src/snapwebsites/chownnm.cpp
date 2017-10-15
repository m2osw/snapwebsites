// Snap Websites Server -- like shell `chown <user>:<group> <path+file>`
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

#include "snapwebsites/chownnm.h"

#include "snapwebsites/log.h"

#include <grp.h>
#include <pwd.h>
#include <unistd.h>

#include "snapwebsites/poison.h"


namespace snap
{


/** \brief Set the owner and group of a file or directory.
 *
 * This function determines the user identifier and group identifier
 * from the specified names and use them to call chown().
 *
 * \param[in] path  The path to the file or directory to change.
 * \param[in] user_name  The name of the user.
 * \param[in] group_name  The name of the group.
 *
 * \return 0 if chown succeeded,
 *         -1 if an error occurs (i.e. permissions denied) and errno is set
 *         accordingly.
 */
int chownnm(QString const & path, QString const & user_name, QString const & group_name)
{
    uid_t uid(-1);
    gid_t gid(-1);

    // user name specified?
    //
    if(!user_name.isEmpty())
    {
        struct passwd const * pwd(getpwnam(user_name.toUtf8().data()));
        if(pwd == nullptr)
        {
            return -1;
        }
        uid = pwd->pw_uid;
    }

    // group name specified?
    //
    if(!group_name.isEmpty())
    {
        struct group const * grp(getgrnam(group_name.toUtf8().data()));
        if(grp == nullptr)
        {
            return -1;
        }
        gid = grp->gr_gid;
    }

    return chown(path.toUtf8().data(), uid, gid);
}


} // snap namespace
// vim: ts=4 sw=4 et
