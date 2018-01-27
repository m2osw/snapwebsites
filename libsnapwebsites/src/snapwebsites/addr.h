// Network Address -- classes functions to ease handling IP addresses
// Copyright (c) 2012-2018  Made to Order Software Corp.  All Rights Reserved
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

#include "snapwebsites/snap_exception.h"

#include <arpa/inet.h>

#include <memory>
#include <string>
#include <vector>

namespace snap_addr
{


class addr_invalid_argument_exception : public snap::snap_exception
{
public:
    addr_invalid_argument_exception(char const *        what_msg) : snap_exception(what_msg) {}
    addr_invalid_argument_exception(std::string const & what_msg) : snap_exception(what_msg) {}
    addr_invalid_argument_exception(QString const &     what_msg) : snap_exception(what_msg) {}
};

class addr_invalid_structure_exception : public snap::snap_logic_exception
{
public:
    addr_invalid_structure_exception(char const *        what_msg) : snap_logic_exception(what_msg) {}
    addr_invalid_structure_exception(std::string const & what_msg) : snap_logic_exception(what_msg) {}
    addr_invalid_structure_exception(QString const &     what_msg) : snap_logic_exception(what_msg) {}
};

class addr_invalid_parameter_exception : public snap::snap_logic_exception
{
public:
    addr_invalid_parameter_exception(char const *        what_msg) : snap_logic_exception(what_msg) {}
    addr_invalid_parameter_exception(std::string const & what_msg) : snap_logic_exception(what_msg) {}
    addr_invalid_parameter_exception(QString const &     what_msg) : snap_logic_exception(what_msg) {}
};



class addr
{
public:
    enum class network_type_t
    {
        NETWORK_TYPE_UNDEFINED,
        NETWORK_TYPE_PRIVATE,
        NETWORK_TYPE_CARRIER,
        NETWORK_TYPE_LINK_LOCAL,
        NETWORK_TYPE_MULTICAST,
        NETWORK_TYPE_LOOPBACK,
        NETWORK_TYPE_ANY,
        NETWORK_TYPE_UNKNOWN,
        NETWORK_TYPE_PUBLIC = NETWORK_TYPE_UNKNOWN  // we currently do not distinguish public and unknown
    };

    enum class computer_interface_address_t
    {
        COMPUTER_INTERFACE_ADDRESS_ERROR = -1,
        COMPUTER_INTERFACE_ADDRESS_FALSE,
        COMPUTER_INTERFACE_ADDRESS_TRUE
    };

    typedef std::shared_ptr<addr>   pointer_t;
    typedef std::vector<addr>       vector_t;

                                    addr();
                                    addr(std::string const & ap, std::string const & default_address, int const default_port, char const * protocol);
                                    addr(std::string const & ap, char const * protocol);
                                    addr(std::string const & address, int const port, char const * protocol);
                                    addr(struct sockaddr_in const & in);
                                    addr(struct sockaddr_in6 const & in6);

    static vector_t                 get_local_addresses();

    void                            set_addr_port(std::string const & ap, std::string const & default_address, int const default_port, char const * protocol);
    void                            set_addr_port(std::string const & address, int const port, char const * protocol);
    void                            set_from_socket(int s);
    void                            set_ipv4(struct sockaddr_in const & in);
    void                            set_ipv6(struct sockaddr_in6 const & in6);
    void                            set_port(int port);
    void                            set_protocol(char const * protocol);

    bool                            is_ipv4() const;
    void                            get_ipv4(struct sockaddr_in & in) const;
    void                            get_ipv6(struct sockaddr_in6 & in6) const;
    std::string                     get_ipv4_string(bool include_port = false) const;
    std::string                     get_ipv6_string(bool include_port = false, bool include_brackets = true) const;
    std::string                     get_ipv4or6_string(bool include_port = false, bool include_brackets = true) const;

    network_type_t                  get_network_type() const;
    std::string                     get_network_type_string() const;
    computer_interface_address_t    is_computer_interface_address() const;

    std::string                     get_iface_name() const;
    std::string                     get_name() const;
    std::string                     get_service() const;
    int                             get_port() const;
    int                             get_protocol() const;

    bool                            operator == (addr const & rhs) const;
    bool                            operator != (addr const & rhs) const;
    bool                            operator < (addr const & rhs) const;

private:
    void                            address_changed();

    // either way, keep address in an IPv6 structure
    struct sockaddr_in6             f_address = sockaddr_in6();
    std::string                     f_iface_name;
    int                             f_protocol = IPPROTO_TCP;
    mutable network_type_t          f_private_network_defined = network_type_t::NETWORK_TYPE_UNDEFINED;
};


} // snap_addr namespace


inline bool operator == (struct sockaddr_in6 const & a, struct sockaddr_in6 const & b)
{
    return memcmp(&a, &b, sizeof(struct sockaddr_in6)) == 0;
}


inline bool operator != (struct sockaddr_in6 const & a, struct sockaddr_in6 const & b)
{
    return memcmp(&a, &b, sizeof(struct sockaddr_in6)) != 0;
}


inline bool operator < (struct sockaddr_in6 const & a, struct sockaddr_in6 const & b)
{
    return memcmp(&a, &b, sizeof(struct sockaddr_in6)) < 0;
}


inline bool operator == (in6_addr const & a, in6_addr const & b)
{
    return memcmp(&a, &b, sizeof(in6_addr)) == 0;
}


inline bool operator != (in6_addr const & a, in6_addr const & b)
{
    return memcmp(&a, &b, sizeof(in6_addr)) != 0;
}


inline bool operator < (in6_addr const & a, in6_addr const & b)
{
    return memcmp(&a, &b, sizeof(in6_addr)) < 0;
}


// vim: ts=4 sw=4 et
