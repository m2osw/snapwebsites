// TCP Client & Server -- classes to ease handling sockets
// Copyright (C) 2012-2017  Made to Order Software Corp.
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

// make sure we use OpenSSL with multi-thread support
// (TODO: move to .cpp once we have the impl!)
#define OPENSSL_THREAD_DEFINES

#include <snapwebsites/addr.h>

#include <QString>

#include <stdexcept>
#include <memory>

#include <arpa/inet.h>

// BIO versions of the TCP client/server
// TODO: move to an impl
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/ssl.h>

namespace tcp_client_server
{

class tcp_client_server_logic_error : public std::logic_error
{
public:
    tcp_client_server_logic_error(std::string const & errmsg) : logic_error(errmsg) {}
};

class tcp_client_server_runtime_error : public std::runtime_error
{
public:
    tcp_client_server_runtime_error(std::string const & errmsg) : runtime_error(errmsg) {}
};

class tcp_client_server_parameter_error : public tcp_client_server_logic_error
{
public:
    tcp_client_server_parameter_error(std::string const & errmsg) : tcp_client_server_logic_error(errmsg) {}
};

class tcp_client_server_initialization_error : public tcp_client_server_runtime_error
{
public:
    tcp_client_server_initialization_error(std::string const & errmsg) : tcp_client_server_runtime_error(errmsg) {}
};

class tcp_client_server_initialization_missing_error : public tcp_client_server_runtime_error
{
public:
    tcp_client_server_initialization_missing_error(std::string const & errmsg) : tcp_client_server_runtime_error(errmsg) {}
};



// TODO: assuming that bio_client with MODE_PLAIN works the same way
//       as a basic tcp_client, we should remove this class
class tcp_client
{
public:
    typedef std::shared_ptr<tcp_client>     pointer_t;

                        tcp_client(std::string const & addr, int port);
                        tcp_client(tcp_client const & src) = delete;
    tcp_client &        operator = (tcp_client const & rhs) = delete;
                        ~tcp_client();

    int                 get_socket() const;
    int                 get_port() const;
    int                 get_client_port() const;
    std::string         get_addr() const;
    std::string         get_client_addr() const;

    int                 read(char * buf, size_t size);
    int                 read_line(std::string & line);
    int                 write(char const * buf, size_t size);

private:
    int                 f_socket;
    int                 f_port;
    std::string         f_addr;
};


// TODO: implement a bio_server then like with the client remove
//       this basic tcp_server if it was like the bio version
class tcp_server
{
public:
    typedef std::shared_ptr<tcp_server>     pointer_t;

    static int const    MAX_CONNECTIONS = 50;

                        tcp_server(std::string const & addr, int port, int max_connections = MAX_CONNECTIONS, bool reuse_addr = false, bool auto_close = false);
                        tcp_server(tcp_server const & src) = delete;
    tcp_server &        operator = (tcp_server const & rhs) = delete;
                        ~tcp_server();

    int                 get_socket() const;
    int                 get_max_connections() const;
    int                 get_port() const;
    std::string         get_addr() const;
    bool                get_keepalive() const;
    void                set_keepalive(bool yes = true);

    int                 accept( int const max_wait_ms = -1 );
    int                 get_last_accepted_socket() const;

private:
    int                 f_max_connections = MAX_CONNECTIONS;
    int                 f_socket = -1;
    int                 f_port = -1;
    std::string         f_addr;
    int                 f_accepted_socket = -1;
    bool                f_keepalive = true;
    bool                f_auto_close = false;
};


class bio_server;

// Create/manage certificates details:
// https://help.ubuntu.com/lts/serverguide/certificates-and-security.html
class bio_client
{
public:
    typedef std::shared_ptr<bio_client>     pointer_t;

    enum class mode_t
    {
        MODE_PLAIN,             // avoid SSL/TLS
        MODE_SECURE,            // WARNING: may return a non-verified connection
        MODE_ALWAYS_SECURE      // fails if cannot be 100% secure
    };

                        bio_client(std::string const & addr, int port, mode_t mode = mode_t::MODE_PLAIN);
                        bio_client(bio_client const & src) = delete;
    bio_client &        operator = (bio_client const & rhs) = delete;
                        ~bio_client();

    void                close();

    int                 get_socket() const;
    int                 get_port() const;
    int                 get_client_port() const;
    std::string         get_addr() const;
    std::string         get_client_addr() const;

    int                 read(char * buf, size_t size);
    int                 read_line(std::string & line);
    int                 write(char const * buf, size_t size);

private:
    friend bio_server;

                        bio_client(std::shared_ptr<BIO> bio);

    std::shared_ptr<SSL_CTX>    f_ssl_ctx;
    std::shared_ptr<BIO>        f_bio;
};


// try `man BIO_f_ssl` or go to:
// https://www.openssl.org/docs/manmaster/crypto/BIO_f_ssl.html
class bio_server
{
public:
    typedef std::shared_ptr<bio_server>     pointer_t;

    static int const    MAX_CONNECTIONS = 50;

    enum class mode_t
    {
        MODE_PLAIN,             // no encryption
        MODE_SECURE             // use TLS encryption
    };

                            bio_server(snap_addr::addr const & addr_port, int max_connections, bool reuse_addr, std::string const & certificate, std::string const & private_key, mode_t mode);

    bool                    is_secure() const;
    int                     get_socket() const;
    bio_client::pointer_t   accept();

private:
    int                         f_max_connections = MAX_CONNECTIONS;
    std::shared_ptr<SSL_CTX>    f_ssl_ctx;
    std::shared_ptr<BIO>        f_listen;
    bool                        f_keepalive = true;
};


bool is_ipv4(char const * ip);
bool is_ipv6(char const * ip);
void get_addr_port(QString const & addr_port, QString & addr, int & port, char const * protocol);


} // namespace tcp_client_server
// vim: ts=4 sw=4 et
