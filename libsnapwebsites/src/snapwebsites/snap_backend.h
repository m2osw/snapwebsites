// Snap Websites Servers -- snap websites child process hanlding
// Copyright (C) 2011-2017  Made to Order Software Corp.
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

#include "snapwebsites/snap_child.h"
#include "snapwebsites/snap_communicator.h"
#include "snapwebsites/snap_thread.h"

namespace snap
{

class snap_backend
        : public snap_child
{
public:
    typedef std::string         message_t;

                                snap_backend( server_pointer_t s );
    virtual                     ~snap_backend();

    void                        create_signal( std::string const & name );

    bool                        stop_received() const;
    bool                        add_uri_for_processing(QString const & cron_action, int64_t date, QString const & website_uri);
    bool                        remove_processed_uri(QString const & action, QByteArray const & key, QString const & website_uri);

    void                        run_backend();
    void                        stop(bool quitting);
    void                        process_connection_failed();

    // internal functions that need to be public...
    // (we could create friends, too)
    //
    void                        process_tick();
    bool                        process_timeout();
    void                        process_message(snap::snap_communicator_message const & message);
    void                        process_child_message(snap::snap_communicator_message const & message);
    void                        process_reconnect();
    void                        capture_zombies(pid_t pid);

private:
    void                        process_action();
    bool                        process_backend_uri(QString const & uri);
    void                        disconnect();
    virtual void                disconnect_cassandra() override;
    void                        request_cassandra_status();
    std::string                 get_signal_name_from_action();
    bool                        is_cron_action(QString const & action);
    bool                        is_ready(QString const & uri);

    pid_t                                   f_parent_pid = -1;
    libdbproxy::table::pointer_t f_sites_table;
    libdbproxy::table::pointer_t f_backend_table;
    QString                                 f_action;
    QString                                 f_website;
    int                                     f_not_ready_counter = 0;
    int                                     f_error_count = 0;
    bool                                    f_cron_action = false;
    bool                                    f_stop_received = false;
    bool                                    f_auto_retry_cassandra = false;
    bool                                    f_emit_warning_about_missing_sites = true;
    bool                                    f_pinged = false;
    bool                                    f_global_lock = false;
    bool                                    f_snaplock = false;
};


} // namespace snap
// vim: ts=4 sw=4 et
