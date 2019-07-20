// Snap Websites Server -- like shell `chown <user>:<group> <path+file>`
// Copyright (c) 2016-2019  Made to Order Software Corp.  All Rights Reserved
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
#include "snapwebsites/chownnm.h"


// snapwebsite lib
//
#include "snapwebsites/log.h"


// C lib
//
#include <grp.h>
#include <pwd.h>
#include <unistd.h>


// last include
//
#include <snapdev/poison.h>


namespace snap
{


/** \brief Set the owner and group of a file or directory.
 *
 * This function determines the user identifier and group identifier
 * from the specified names and use them to call chown().
 *
 * The function can fail if an identifier for the user or group names
 * cannot be determined.
 *
 * The function may fail if the path is invalid or permissions do not
 * allow for ownership modifications.
 *
 * \param[in] path  The path to the file or directory to change.
 * \param[in] user_name  The name of the user.
 * \param[in] group_name  The name of the group.
 *
 * \return 0 if chown succeeded,
 *         -1 if an error occurs (i.e. permissions denied, unknown user/group)
 *         and errno is set accordingly.
 */
int chownnm(std::string const & path
          , std::string const & user_name
          , std::string const & group_name)
{
    uid_t uid(-1);
    gid_t gid(-1);

    // user name specified?
    //
    if(!user_name.empty())
    {
        struct passwd const * pwd(getpwnam(user_name.c_str()));
        if(pwd == nullptr)
        {
            return -1;
        }
        uid = pwd->pw_uid;
    }

    // group name specified?
    //
    if(!group_name.empty())
    {
        struct group const * grp(getgrnam(group_name.c_str()));
        if(grp == nullptr)
        {
            return -1;
        }
        gid = grp->gr_gid;
    }

    if(uid != static_cast<uid_t>(-1)
    || gid != static_cast<gid_t>(-1))
    {
        return chown(path.c_str(), uid, gid);
    }

    // in case both are undefined (it happens in the mkdir_p() function)
    //
    return 0;
}


int chownnm(QString const & path, QString const & user_name, QString const & group_name)
{
    return chownnm(path.toUtf8().data()
                 , user_name.toUtf8().data()
                 , group_name.toUtf8().data());
}


int chownnm(char const * path, char const * user_name, char const * group_name)
{
    return chownnm(path == nullptr ? std::string() : std::string(path)
                 , user_name == nullptr ? std::string() : std::string(user_name)
                 , group_name == nullptr ? std::string() : std::string(group_name));
}


} // snap namespace
// vim: ts=4 sw=4 et
