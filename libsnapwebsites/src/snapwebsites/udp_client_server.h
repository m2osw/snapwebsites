// UDP Client Server -- send/receive UDP packets
// Copyright (C) 2013-2017  Made to Order Software Corp.
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

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <stdexcept>
#include <memory>

namespace udp_client_server
{

class udp_client_server_runtime_error : public std::runtime_error
{
public:
    udp_client_server_runtime_error(const std::string & errmsg) : std::runtime_error(errmsg) {}
};

class udp_client_server_parameter_error : public std::logic_error
{
public:
    udp_client_server_parameter_error(const std::string & errmsg) : std::logic_error(errmsg) {}
};


class udp_client
{
public:
    typedef std::shared_ptr<udp_client>     pointer_t;

                        udp_client(std::string const & addr, int port);
                        ~udp_client();

    int                 get_socket() const;
    int                 get_port() const;
    std::string         get_addr() const;

    int                 send(const char * msg, size_t size);

private:
    int                 f_socket;
    int                 f_port;
    std::string         f_addr;
    struct addrinfo *   f_addrinfo;
};


class udp_server
{
public:
    typedef std::shared_ptr<udp_server>     pointer_t;

                        udp_server(std::string const & addr, int port);
                        ~udp_server();

    int                 get_socket() const;
    int                 get_port() const;
    std::string         get_addr() const;

    int                 recv(char * msg, size_t max_size);
    int                 timed_recv( char * msg, size_t const max_size, int const max_wait_ms );
    std::string         timed_recv( int const bufsize, int const max_wait_ms );

private:
    int                 f_socket;
    int                 f_port;
    std::string         f_addr;
    struct addrinfo *   f_addrinfo;
};

} // namespace udp_client_server
// vim: ts=4 sw=4 et
