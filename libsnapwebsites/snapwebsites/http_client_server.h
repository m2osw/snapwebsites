// HTTP Client & Server -- classes to ease handling HTTP protocol
// Copyright (c) 2012-2019  Made to Order Software Corp.  All Rights Reserved
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

#include "eventdispatcher/tcp_bio_client.h"

#include <map>
#include <vector>


namespace http_client_server
{



class http_client_server_logic_error : public std::logic_error
{
public:
    http_client_server_logic_error(std::string const& errmsg) : logic_error(errmsg) {}
};

class http_client_server_runtime_error : public std::runtime_error
{
public:
    http_client_server_runtime_error(std::string const& errmsg) : runtime_error(errmsg) {}
};

class http_client_exception_io_error : public http_client_server_runtime_error
{
public:
    http_client_exception_io_error(std::string const& errmsg) : http_client_server_runtime_error(errmsg) {}
};




// name / value pairs
typedef std::map<std::string, std::string>  header_t;

// attachment buffer
typedef std::vector<char>                   attachment_t;


class http_request
{
public:
    typedef std::shared_ptr<http_request>       pointer_t;

    std::string     get_host() const;
    int             get_port() const;
    std::string     get_command() const;
    std::string     get_path() const;
    std::string     get_header(std::string const& name) const;
    std::string     get_post(std::string const& name) const;
    std::string     get_body() const; // also returns data
    std::string     get_request(bool keep_alive) const;

    void            set_uri(std::string const& uri);
    void            set_host(std::string const& host);
    void            set_port(int port);
    void            set_command(std::string const& command);
    void            set_path(std::string const& path);
    void            set_header(std::string const& name, std::string const& value);
    void            set_post(std::string const& name, std::string const& value);
    void            set_basic_auth(std::string const& username, std::string const& secret);
    void            set_data(std::string const& data);
    void            set_body(std::string const& body);

private:
    std::string                 f_host = std::string();
    std::string                 f_command = std::string();
    std::string                 f_path = std::string();
    int32_t                     f_port = -1;
    header_t                    f_headers = header_t();
    header_t                    f_post = header_t();
    std::string                 f_body = std::string();
    std::vector<attachment_t>   f_attachments = std::vector<attachment_t>();  // not used yet (Also look in a way that allows us to avoid an extra copy)
    bool                        f_has_body = false;
    bool                        f_has_data = false;
    bool                        f_has_post = false;
    bool                        f_has_attachment = false; // not used yet
};


class http_client;


class http_response
{
public:
    typedef std::shared_ptr<http_response>      pointer_t;

    enum class protocol_t
    {
        UNKNOWN,
        HTTP_1_0,
        HTTP_1_1
    };

    std::string     get_original_header() const;
    protocol_t      get_protocol() const;
    int             get_response_code() const;
    std::string     get_http_message() const;
    bool            has_header(std::string const & name) const;
    std::string     get_header(std::string const & name) const;
    std::string     get_response() const;

    void            append_original_header(std::string const & header);
    void            set_protocol(protocol_t protocol);
    void            set_response_code(int code);
    void            set_http_message(std::string const& message);
    void            set_header(std::string const & name, std::string const & value);
    void            set_response(std::string const & response);

private:
    friend http_client;

    void            read_response(ed::tcp_bio_client::pointer_t connection);

    std::string                 f_original_header = std::string();
    protocol_t                  f_protocol = protocol_t::UNKNOWN;
    int32_t                     f_response_code = 0;
    std::string                 f_http_message = std::string();
    header_t                    f_header = header_t();
    std::string                 f_response = std::string();
};


class http_client
{
public:
                                http_client() {}

                                http_client(http_client const &) = delete;
    http_client &               operator = (http_client const &) = delete;

    bool                        get_keep_alive() const;

    void                        set_keep_alive(bool keep_alive);

    http_response::pointer_t    send_request(http_request const & request);

private:
    bool                            f_keep_alive = true;
    ed::tcp_bio_client::pointer_t   f_connection = ed::tcp_bio_client::pointer_t();
    std::string                     f_host = std::string();
    int32_t                         f_port = -1;
};


} // namespace http_client_server
// vim: ts=4 sw=4 et
