// Snap Manager Daemon -- a snapmanager to run administrative commands on any computer
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
#pragma once

// ourselves
//
#include "snapmanager/manager.h"

// snapwebsites lib
#include <snapwebsites/addr.h>
#include <snapwebsites/log.h>
#include <snapwebsites/snap_exception.h>
#include <snapwebsites/tcp_client_server.h>
#include <snapwebsites/snapwebsites.h>

// advgetopt lib
//
#include <advgetopt/advgetopt.h>

// C++ lib
//
#include <string>
#include <iostream>


namespace snap_manager
{

class manager_daemon;
class manager_status;


class manager_interrupt
        : public snap::snap_communicator::snap_signal
{
public:
    typedef std::shared_ptr<manager_interrupt>  pointer_t;

                                manager_interrupt(manager_daemon * md);
    virtual                     ~manager_interrupt() override {}

    // snap::snap_communicator::snap_signal implementation
    virtual void                process_signal() override;

private:
    manager_daemon *            f_manager_daemon = nullptr;
};


class manager_messenger
        : public snap::snap_communicator::snap_tcp_client_permanent_message_connection
{
public:
    typedef std::shared_ptr<manager_messenger>    pointer_t;

                                manager_messenger(manager_daemon * md, std::string const & addr, int port);

    // snap::snap_communicator::snap_connection implementation
    //virtual void                process_timeout() override;

    // snap::snap_communicator::snap_tcp_client_permanent_message_connection implementation
    virtual void                process_message(snap::snap_communicator_message const & message) override;
    virtual void                process_connection_failed(std::string const & error_message) override;
    virtual void                process_connected() override;

protected:
    // these objects are owned by the manager_daemon objects so no need
    // for a smart pointer (and it would create a loop unless we use a weakptr)
    manager_daemon *            f_manager = nullptr;
};



class status_connection
        : public snap::snap_communicator::snap_inter_thread_message_connection
{
public:
    typedef std::shared_ptr<status_connection>   pointer_t;

                                status_connection(manager_daemon * md);

    void                        set_thread_b(manager_status * ms);
    void                        set_server_name(QString const & server_name);

    // snap::snap_communicator::snap_pipe_message_connection implementation
    virtual void                process_message_a(snap::snap_communicator_message const & message) override;
    virtual void                process_message_b(snap::snap_communicator_message const & message) override;

private:
    manager_daemon *            f_manager_daemon = nullptr;
    manager_status *            f_manager_status = nullptr;
    QString                     f_server_name;
};



class manager_status
    : public snap::snap_thread::snap_runner
{
public:
                                    manager_status(manager_daemon *md, status_connection::pointer_t sc);
    virtual                         ~manager_status();

    virtual void                    run() override;

    void                            resend_status(bool const kick_now);
    void                            set_snapmanager_frontend(QString const & snapmanager_frontend);
    bool                            is_snapmanager_frontend(QString const & server_name) const;
    snap::snap_string_list const &  get_snapmanager_frontend() const;
    bool                            stop_now_prima() const;

    void                            process_message(snap::snap_communicator_message const & message);

private:
    int                             package_status(QString const & package_name, bool add_info_only_if_present);

    manager_daemon *                f_manager_daemon = nullptr;
    status_connection::pointer_t    f_status_connection;
    bool                            f_running = true;
    bool                            f_resend_status = false;
    snap::snap_string_list          f_snapmanager_frontend;
};




class bundle_loader
    : public snap::snap_thread::snap_runner
{
public:
                                    bundle_loader();
    virtual                         ~bundle_loader() override;

    void                            set_bundle_uri(QString const & bundles_path, std::vector<std::string> const & bundle_uri);

    virtual void                    run() override;

private:
    bool                            load(std::string const & uri);
    bool                            wget(std::string const & uri, std::string const & filename);

    QString                         f_bundles_path;
    std::vector<std::string>        f_bundle_uri;
};




class manager_daemon
    : public manager
{
public:
    typedef std::shared_ptr<manager_daemon> pointer_t;

                                    manager_daemon();
    virtual                         ~manager_daemon() override;

    void                            init(int argc, char * argv[]);
    int                             run();
    void                            process_message(snap::snap_communicator_message const & message);
    virtual void                    forward_message(snap::snap_communicator_message const & message) override;
    void                            unreachable_message(snap::snap_communicator_message const & message);
    virtual snap::snap_string_list const & get_snapmanager_frontend() const override;
    virtual bool                    stop_now_prima() const override;
    void                            stop(bool quitting);

private:
    static void                     sighandler( int sig );

    // the MANAGE command and all of its sub-functions
    void                            manager_install(snap::snap_communicator_message const & message);
    void                            set_manager_status(snap::snap_communicator_message const & message);
    void                            modify_settings(snap::snap_communicator_message const & message);
    void                            send_ack( snap::snap_communicator_message const & message, bool const done = false );
    void                            send_nak( snap::snap_communicator_message const & message );

    //QString                                     f_server_types;                         // sent to use by snapcommunicator
    int                                         f_communicator_port = 4040;             // snap server port
    QString                                     f_communicator_address = "127.0.0.1";   // snap server address
    snap::snap_communicator::pointer_t          f_communicator;
    manager_interrupt::pointer_t                f_interrupt;
    manager_messenger::pointer_t                f_messenger;
    status_connection::pointer_t                f_status_connection;
    manager_status                              f_status_runner;
    snap::snap_thread                           f_status_thread;
    bundle_loader                               f_bundle_loader;
    snap::snap_thread                           f_bundle_thread;
    bool                                        f_force_restart = false;
    //server_status                               f_status;           // our status
    QString                                     f_output;           // commands output, to be sent to end user
};


} // namespace snap_manager
// vim: ts=4 sw=4 et
