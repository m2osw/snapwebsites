// Snap Websites Server -- manage a PID file for a snap service
// Copyright (c) 2019  Made to Order Software Corp.  All Rights Reserved
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
#pragma once


// self
//
#include "snapwebsites/snap_exception.h"


// snapdev lib
//
#include <snapdev/raii_generic_deleter.h>



/** \file
 * \brief Declaration of the snap_pid class to manage a PID file.
 *
 * This header defines the snap_pid class which we use to manage a PID
 * file. This is quite practical to make sure only one instance of a
 * service is running, it also allows us to let systemd know once our
 * child process is ready and the parent is about to quit (i.e. when
 * we use detached processes.)
 *
 * See the .cpp file for more details.
 */

namespace snap
{



class snap_pid_exception : public snap_exception
{
public:
    explicit snap_pid_exception(char const *        whatmsg) : snap_exception("snap_pid", whatmsg) {}
    explicit snap_pid_exception(std::string const & whatmsg) : snap_exception("snap_pid", whatmsg) {}
    explicit snap_pid_exception(QString const &     whatmsg) : snap_exception("snap_pid", whatmsg) {}
};

class snap_pid_exception_io_error : public snap_pid_exception
{
public:
    explicit snap_pid_exception_io_error(char const *        whatmsg) : snap_pid_exception(whatmsg) {}
    explicit snap_pid_exception_io_error(std::string const & whatmsg) : snap_pid_exception(whatmsg) {}
    explicit snap_pid_exception_io_error(QString const &     whatmsg) : snap_pid_exception(whatmsg) {}
};

class snap_pid_exception_invalid_parameter : public snap_pid_exception
{
public:
    explicit snap_pid_exception_invalid_parameter(char const *        whatmsg) : snap_pid_exception(whatmsg) {}
    explicit snap_pid_exception_invalid_parameter(std::string const & whatmsg) : snap_pid_exception(whatmsg) {}
    explicit snap_pid_exception_invalid_parameter(QString const &     whatmsg) : snap_pid_exception(whatmsg) {}
};






class snap_pid
{
public:
    typedef std::shared_ptr<snap_pid>   pointer_t;

                    snap_pid(std::string const & service_name);
                    ~snap_pid();

    void            create_pid_file();
    bool            wait_signal();

private:
    void            generate_filename(std::string const & service_name);
    void            unlink_pid_file();
    void            close_pipes();
    void            send_signal(bool result);

    std::string     f_service_name = std::string();
    int             f_pipes[2];
    std::string     f_pid_filename = std::string();
    raii_fd_t       f_safe_fd = raii_fd_t();
    bool            f_child_process = false;
    char            f_result = static_cast<char>(false);
};

} // namespace snap
// vim: ts=4 sw=4 et
