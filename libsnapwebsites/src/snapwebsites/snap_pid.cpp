// Snap Websites Server -- snap websites server
// Copyright (c) 2011-2019  Made to Order Software Corp.  All Rights Reserved
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


// self
//
#include "snapwebsites/snap_pid.h"


// snapwebsites lib
//
#include "snapwebsites/log.h"
#include "snapwebsites/snapwebsites.h"


// C lib
//
#include <unistd.h>
#include <fcntl.h>
#include <sys/file.h>


// last include
//
#include <snapdev/poison.h>





/** \file
 * \brief Implementation of the snap_pid class.
 *
 * The snap_pid class is used to create a locked PID file which we keep
 * around while a service is running. It allows us to make sure only
 * one instance of a service is running. Attempting to run a second
 * instance will fail in the constructor.
 *
 * The destructor makes sure that the PID file gets unlocked and deleted.
 * It is generally not extremely useful, but it is not a bad thing to
 * clean up behind oneself.
 */


namespace snap
{


/** \brief Create a PID file.
 *
 * This function is going to create a PID file for the currently running
 * process.
 *
 * The function attempts to lock the file before writing to it. If the lock
 * fails, then the file is already held by another process so we can't
 * move forward (i.e. if another instance of the same service is already
 * running, the second isntance has to quit.)
 *
 * \note
 * We do not 100% guarantee that the current file always gets deleted on exit.
 * The lock tells us whether a process is gone or not. We are planning to
 * have a stronger rule for the delete to happen, though.
 *
 * \param[in] service_name  The name of this server.
 */
snap_pid::snap_pid(std::string const & service_name)
    : f_service_name(service_name)
{
    generate_filename(service_name);
    if(pipe2(f_pipes, O_CLOEXEC) != 0)
    {
        throw snapwebsites_exception_io_error("error trying to open pipe() to inform parent that the child PID file was created.");
    }
}


/** \brief Clean up the PID file.
 *
 * The destructor cleans up the PID file. Mainly, it attempts to delete
 * the file which has the side effect of removing the exclusive lock.
 *
 * If you hold a shared pointer to the snap_pid object, make sure to
 * reset it before calling exit() if such you do.
 */
snap_pid::~snap_pid()
{
    unlink_pid_file();
    close_pipes();
}


/** \brief Create the PID file.
 *
 * This function creates the PID file.
 *
 * The function returns success or failure to the parent using the pipes.
 *
 * On success, it sends true (0x01).
 *
 * On failure, it sends false (0x00).
 */
void snap_pid::create_pid_file()
{
    f_safe_fd.reset(open(f_pid_filename.c_str()
                       , O_CLOEXEC | O_CREAT | O_TRUNC | O_WRONLY
                       , S_IRUSR | S_IWUSR));

    // make sure the open() worked
    //
    if(!f_safe_fd)
    {
        SNAP_LOG_FATAL("Server \"")
                      (f_service_name)
                      ("\" could not create PID file \"")
                      (f_pid_filename)
                      ("\".");
        send_signal(false);
        throw snap_pid_exception_io_error(
                      "Could not open PID file \""
                    + f_pid_filename
                    + "\".");
    }

    // attempt an exclusive lock
    //
    if(flock(f_safe_fd.get(), LOCK_EX) != 0)
    {
        SNAP_LOG_FATAL("Server \"")
                      (f_service_name)
                      ("\"could not lock PID file \"")
                      (f_pid_filename)
                      ("\". Another instance is already running?");
        send_signal(false);
        throw snap_pid_exception_io_error(
                      "Could not lock PID file \""
                    + f_pid_filename
                    + "\". Another instance is already running?");
    }

    f_child_process = true;

    std::string const pid(std::to_string(getpid()) + "\n");
    if(write(f_safe_fd.get(), pid.c_str(), pid.length()) != static_cast<ssize_t>(pid.length()))
    {
        SNAP_LOG_FATAL("Server \"")
                      (f_service_name)
                      ("\"could not lock PID file \"")
                      (f_pid_filename)
                      ("\". Another instance is already running?");
        send_signal(false);
        throw snap_pid_exception_io_error(
                      "Could not lock PID file \""
                    + f_pid_filename
                    + "\". Another instance is already running?");
    }

    // PID file is all good
    //
    // TBD: we may want to allow the caller to send this signal at a
    //      later time assuming the child's initialization may not yet
    //      be complete and we may want to return with exit(1) from
    //      the parent if the child's initialization fails.
    //
    send_signal(true);
}


/** \brief Transforms the service name in a PID filename.
 *
 * This function retrieves the "run_path" parameter from the snapserver
 * configuration file (/etc/snapwebsites/[snapwebsites.d/]snapserver.conf)
 * and append the service name. It also adds a .pid extension.
 *
 * The \p service_name parameter is not expected to include a slash or
 * an extension.
 *
 * \param[in] service_name  The name of the service getting a PID file.
 */
void snap_pid::generate_filename(std::string const & service_name)
{
    if(service_name.find('/') != std::string::npos)
    {
        SNAP_LOG_FATAL("Service name \"")
                      (service_name)
                      ("\"cannot include a slash (/) character.");
        throw snap_pid_exception_invalid_parameter(
                      "Service name \""
                    + service_name
                    + "\"cannot include a slash (/) character.");
    }

    // get the "run_path=..." parameter from the snapserver.conf file
    //
    snap::snap_config config("snapserver");
    std::string run_path("/run/snapwebsites");
    if(config.has_parameter("run_path"))
    {
        run_path = config.get_parameter("run_path");
    }

    // generate the filename now
    //
    f_pid_filename = run_path + "/" + service_name + ".pid";
}


/** \brief Remove the PID file.
 *
 * This function removes the PID file from disk. This has the side effect
 * of unlocking the file too.
 *
 * \attention
 * Only the child process is allowed to delete the PID file. This function
 * will do nothing if called by the parent process (which happens in the
 * destructor.)
 */
void snap_pid::unlink_pid_file()
{
    if(f_child_process)
    {
        unlink(f_pid_filename.c_str());
    }
}


/** \brief Close the communication pipes.
 *
 * This function close the two pipes created by the constructor. This
 * ensures that the communication happens only once and that the
 * pipe resources get resolved.
 */
void snap_pid::close_pipes()
{
    if(f_pipes[0] != -1)
    {
        close(f_pipes[0]);
        f_pipes[0] = -1;
    }
    if(f_pipes[1] != -1)
    {
        close(f_pipes[1]);
        f_pipes[1] = -1;
    }
}


/** \brief Send the parent process a signal about the results.
 *
 * This function is used by the child to send a signal to the parent
 * letting it know that the PID file was properly created.
 *
 * The parent must call the wait_signal() function to make sure that
 * the PID gets created before it returns. In the snapserver is it
 * done in the server::detach() function.
 *
 * \param[in] result  true if the PID file was created successfully.
 */
void snap_pid::send_signal(bool result)
{
    char c[1] = { static_cast<char>(result) };
    if(write(f_pipes[1], c, 1) != 1)
    {
        throw snapwebsites_exception_io_error("error while writing to the pipe between parent and child, letting parent know that the PID file is ready.");
    }
    close_pipes();
}


/** \brief Wait for the child's signal.
 *
 * This function waits for the child to send us one byte to let us know
 * whether the creation of the PID file succeeded or not.
 *
 * If the function returns false, we can let systemd know that the
 * initialization was not a success.
 *
 * \note
 * The function can be called multiple times. It will wait for the child
 * process only the first time. Further calls will just return the same
 * result over and over again.
 *
 * \return true if the child created the PID successfully, false otherwise.
 */
bool snap_pid::wait_signal()
{
    if(f_pipes[0] != -1)
    {
        if(read(f_pipes[0], &f_result, 1) != 1)
        {
            throw snapwebsites_exception_io_error("error while reading from the pipe between parent and child, parent will never know whether the PID file is ready.");
        }

        close_pipes();
    }

    return f_result == 1;
}


} // namespace snap
// vim: ts=4 sw=4 et
