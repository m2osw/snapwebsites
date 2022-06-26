// Snap Websites Server -- read mount points in Linux
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


// C++
//
#include    <vector>


// C
//
#include    <mntent.h>



namespace snap
{

DECLARE_MAIN_EXCEPTION(snap_mounts_exception);

DECLARE_EXCEPTION(snap_mounts_exception, snap_mounts_exception_io_error);






class mount_entry
{
public:
    mount_entry(struct mntent * e)
        : f_fsname(e->mnt_fsname)
        , f_dir(e->mnt_dir)
        , f_type(e->mnt_type)
        , f_options(e->mnt_opts)
        , f_freq(e->mnt_freq)
        , f_passno(e->mnt_passno)
    {
    }

    std::string const & get_fsname()  const { return f_fsname;  }
    std::string const & get_dir()     const { return f_dir;     }
    std::string const & get_type()    const { return f_type;    }
    std::string const & get_options() const { return f_options; }
    int                 get_freq()    const { return f_freq;    }
    int                 get_passno()  const { return f_passno;  }

private:
    std::string         f_fsname;
    std::string         f_dir;
    std::string         f_type;
    std::string         f_options;
    int                 f_freq;
    int                 f_passno;
};



class mounts : public std::vector<mount_entry>
{
public:
                            mounts(std::string const & path);

    std::string const &     get_path() const { return f_path; }

private:
    std::string             f_path;
};




} // namespace snap
// vim: ts=4 sw=4 et
