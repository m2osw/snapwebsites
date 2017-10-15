// Snap Manager CGI -- a snapmanager that works through Apache
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

// our lib
//
#include "snapmanager/manager.h"

// snapwebsites lib
//
#include <snapwebsites/addr.h>
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
    snap::snap_communicator_message f_message;
    QString                         f_result;
};





class manager_cgi
    : public manager
{
public:
    typedef std::shared_ptr<manager_cgi>    pointer_t;

                                manager_cgi();
    virtual                     ~manager_cgi();

    int                         error(char const * code, char const * msg, char const * details);
    void                        forbidden(std::string details);
    bool                        verify();
    int                         process();

private:
    typedef std::map<std::string, std::string>      post_variables_t;
    typedef std::vector<snap_manager::status_t>     status_list_t;
    //typedef std::vector<status_list_t>              statuses_list_t;
    typedef std::map<QString, status_list_t>        status_map_t;

    int                         read_post_variables();
    int                         process_post();
    void                        generate_content(QDomDocument doc, QDomElement root, QDomElement menu);
    QDomElement                 create_table_header( QDomDocument& doc );
    void                        generate_self_refresh_plugin_entry( QDomDocument& doc, QDomElement& table );
    void                        generate_plugin_entry( snap_manager::status_t status, QDomDocument& doc, QDomElement& table );
    void                        generate_plugin_status
                                    ( QDomDocument& doc
                                    , QDomElement& output
                                    , QString const & plugin_name
                                    , status_list_t const & status_list
                                    , bool const parent_div = true
                                    );
    void                        get_status_map( QString const & doc, status_map_t& map );
    void                        get_host_status(QDomDocument doc, QDomElement output, QString const & host);
    void                        get_cluster_status(QDomDocument doc, QDomElement output);
    int                         is_logged_in(std::string & request_method);

    snap::snap_uri              f_uri;
    std::string                 f_communicator_address;
    std::string                 f_cookie;
    std::string                 f_user_name;
    int                         f_communicator_port = -1;
    post_variables_t            f_post_variables;
};


} // namespace snap_manager
// vim: ts=4 sw=4 et
