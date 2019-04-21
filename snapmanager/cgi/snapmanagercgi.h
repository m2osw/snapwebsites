// Snap Manager CGI -- a snapmanager that works through Apache
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
#pragma once

// our lib
//
#include "snapmanager/manager.h"

// snapwebsites lib
//
#include <snapwebsites/log.h>
#include <snapwebsites/snap_exception.h>
#include <snapwebsites/snap_communicator.h>
#include <snapwebsites/snap_uri.h>
#include <snapwebsites/xslt.h>

// C++ lib
//
#include <string>
#include <iostream>



namespace snap_manager
{





class messenger
    : public snap::snap_communicator::snap_tcp_blocking_client_message_connection
{
public:
                            messenger(std::string const & address, int port, snap::snap_communicator_message const & message);
    virtual                 ~messenger() override;

    QString const &         result();

    // implementation of snap_communicator::snap_tcp_blocking_client_message_connection
    void                    process_timeout();
    void                    process_message(snap::snap_communicator_message const & message);

private:
    snap::snap_communicator_message f_message = snap::snap_communicator_message();
    QString                         f_result = QString();
};





class manager_cgi
    : public manager
{
public:
    typedef std::shared_ptr<manager_cgi>    pointer_t;

                                manager_cgi();
    virtual                     ~manager_cgi();

    static pointer_t            instance();
    virtual std::string         server_type() const override;

    int                         error(char const * code, char const * msg, char const * details);
    void                        forbidden(std::string details, bool allow_redirect = true);
    std::string                 get_session_path(bool create = false);
    bool                        verify();
    int                         process();
    snap::snap_uri const &      get_uri() const;

    SNAP_SIGNAL_WITH_MODE(
                  generate_content
                , (QDomDocument doc, QDomElement root, QDomElement output, QDomElement menu, snap::snap_uri const & uri)
                , (doc, root, output, menu, uri)
                , START_AND_DONE);

private:
    typedef std::map<std::string, std::string>      post_variables_t;
    typedef std::vector<snap_manager::status_t>     status_list_t;
    //typedef std::vector<status_list_t>              statuses_list_t;
    typedef std::map<QString, status_list_t>        status_map_t;

    int                         read_post_variables();
    int                         process_post();
    QDomElement                 create_table_header( QDomDocument& doc );
    void                        generate_self_refresh_plugin_entry( QDomDocument& doc, QDomElement& table );
    void                        generate_plugin_entry( snap_manager::status_t status, QDomDocument& doc, QDomElement& table );
    void                        generate_plugin_status
                                    ( QDomDocument& doc
                                    , QDomElement& output
                                    , QString const & plugin_name
                                    , status_list_t const & status_list
                                    , QString const & alerts
                                    );
    void                        generate_plugin_status
                                    ( QDomDocument& doc
                                    , QDomElement& output
                                    , QString const & plugin_name
                                    , status_list_t const & status_list
                                    );
    void                        get_status_map( QString const & doc, status_map_t& map );
    void                        get_host_status(QDomDocument doc, QDomElement output, QString const & host);
    void                        get_cluster_status(QDomDocument doc, QDomElement output);
    std::string                 get_hit_filename();
    int                         is_ip_blocked();
    void                        increase_hit_count();
    void                        delete_hit_file();
    int                         is_logged_in(std::string & request_method);

    snap::snap_uri              f_uri = snap::snap_uri();
    std::string                 f_communicator_address = std::string();
    std::string                 f_cookie = std::string();
    std::string                 f_user_name = std::string();
    int                         f_communicator_port = -1;
    int                         f_max_login_attempts = 5;
    int                         f_login_attempts = -1;
    post_variables_t            f_post_variables = post_variables_t();
};


} // namespace snap_manager
// vim: ts=4 sw=4 et
