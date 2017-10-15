// Network Address -- classes functions to ease handling IP addresses
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

#include "snapwebsites/addr.h"

#include "snapwebsites/log.h"
#include "snapwebsites/tcp_client_server.h"

#include <QString>

#include <sstream>

#include <ifaddrs.h>
#include <netdb.h>

#include "snapwebsites/poison.h"


namespace snap_addr
{

/*
 * Various sytem address structures

struct sockaddr {
   unsigned short    sa_family;    // address family, AF_xxx
   char              sa_data[14];  // 14 bytes of protocol address
};


struct sockaddr_in {
    short            sin_family;   // e.g. AF_INET, AF_INET6
    unsigned short   sin_port;     // e.g. htons(3490)
    struct in_addr   sin_addr;     // see struct in_addr, below
    char             sin_zero[8];  // zero this if you want to
};


struct sockaddr_in6 {
    u_int16_t       sin6_family;   // address family, AF_INET6
    u_int16_t       sin6_port;     // port number, Network Byte Order
    u_int32_t       sin6_flowinfo; // IPv6 flow information
    struct in6_addr sin6_addr;     // IPv6 address
    u_int32_t       sin6_scope_id; // Scope ID
};


struct sockaddr_storage {
    sa_family_t  ss_family;     // address family

    // all this is padding, implementation specific, ignore it:
    char      __ss_pad1[_SS_PAD1SIZE];
    int64_t   __ss_align;
    char      __ss_pad2[_SS_PAD2SIZE];
};

*/


namespace
{

/** \brief Delete an addrinfo structure.
 *
 * This deleter is used to make sure all the addinfo get released when
 * an exception occurs or the function using such exists.
 *
 * \param[in] ai  The addrinfo structure to free.
 */
void addrinfo_deleter(struct addrinfo * ai)
{
    freeaddrinfo(ai);
}


/** \brief Delete an ifaddrs structure.
 *
 * This deleter is used to make sure all the ifaddrs get released when
 * an exception occurs or the function using such exists.
 *
 * \param[in] ia  The ifaddrs structure to free.
 */
void ifaddrs_deleter(struct ifaddrs * ia)
{
    freeifaddrs(ia);
}


}
// no name namespace


/** \brief Create an addr object that represents an ANY address.
 *
 * This function initializes the addr object with the ANY address.
 * The port is set to 0 and the protocol to TCP.
 *
 * It is strongly suggested that you change those parameters
 * before really using this address since a port of zero and
 * the protocol may be wrong.
 */
addr::addr()
{
    memset(&f_address, 0, sizeof(f_address));
    // keep default protocol (TCP)
}


/** \brief Initialize the addr object with the specified address and protocol.
 *
 * This function parses the \p ap string as an IP address optionally
 * followed by a port, just a port (:123), or even the empty string.
 *
 * The protocol name is used to determine the port if the port is not just
 * a number (i.e. by default, "localhost:http" with protocol "tcp" or "udp"
 * return 80 as the port).
 *
 * \param[in] ap  The address and port specification.
 * \param[in] default_address  The default address if none specified.
 * \param[in] default_port  The default port if none specified.
 * \param[in] protocol  The name of the protocol ("tcp", "udp", or nullptr)
 */
addr::addr(std::string const & ap, std::string const & default_address, int const default_port, char const * protocol)
{
    if(ap.empty())
    {
        return;
    }

    set_addr_port(ap, default_address, default_port, protocol);
}


/** \brief Initialize a new addr object with an address and a port.
 *
 * If you already have an address and a port defined separatly, then
 * you can use this function to initialize the addr object.
 *
 * If the \p address string is empty, then the function saves the
 * port and returns immediately.
 *
 * \param[in] address  The address to save in this object or "".
 * \param[in] port  The port to save along this address.
 * \param[in] protocol  The name of the protocol ("tcp", "udp", or nullptr)
 */
addr::addr(std::string const & address, int const port, char const * protocol)
{
    if(address.empty())
    {
        f_address.sin6_port = htons(port);
        return;
    }

    set_addr_port(address, port, protocol);
}


/** \brief Initialize the addr object with the specified address and protocol.
 *
 * This function parses the \p ap string as an IP address optionally
 * followed by a port, just a port (:123), or even the empty string.
 *
 * The protocol name is used to determine the port if the port is not just
 * a number (i.e. by default, "localhost:http" with protocol "tcp" or "udp"
 * return 80 as the port).
 *
 * \param[in] ap  The address and port specification.
 * \param[in] protocol  The name of the protocol ("tcp", "udp", or nullptr)
 */
addr::addr(std::string const & ap, char const * protocol)
{
    if(ap.empty())
    {
        return;
    }

    set_addr_port(ap, "", -1, protocol);
}


/** \brief Create an addr object from a binary IPv4 address.
 *
 * This function initializes this addr object with the specified IPv4
 * address. The is_ipv4() function will return true.
 *
 * \param[in] in  The binary IPv4 address.
 */
addr::addr(struct sockaddr_in const & in)
{
    set_ipv4(in);
    // keep default protocol (TCP)
}


/** \brief Create an addr object from a binary IPv6 address.
 *
 * This function initializes this addr object with the specified IPv6
 * address. The is_ipv4() function will return false.
 *
 * \param[in] in6  The binary IPv6 address.
 */
addr::addr(struct sockaddr_in6 const & in6)
{
    set_ipv6(in6);
    // keep default protocol (TCP)
}


/** \brief The the address and port of this addr object.
 *
 * This function takes one string with an address and port specification
 * separated by a colon and an optional string representing a protocol.
 *
 * The address and port must be separated by a colon. The IPv6 string
 * must be defined in square bracket. For example: "[::]:80" represents
 * the ANY address in IPv6 on port 80.
 *
 * The protocol supported are defined in the set_protocol() function.
 *
 * \param[in] ap  The address and port specification.
 * \param[in] default_address  The default address if none specified.
 * \param[in] default_port  The default port if none specified.
 * \param[in] protocol  The name of the protocol ("tcp", "udp", or nullptr)
 */
void addr::set_addr_port(std::string const & ap, std::string const & default_address, int const default_port, char const * protocol)
{
    // break up the address and port
    //
    QString address(default_address.c_str());
    int port(default_port);
    tcp_client_server::get_addr_port(ap.c_str(), address, port, protocol);

    set_addr_port(address.toUtf8().data(), port, protocol);
}


/** \brief Set the address and port with the address defined as a string.
 *
 * This function saves the specified address as port to this addr object.
 *
 * \exception addr_invalid_argument_exception
 * This exception is raised if the address cannot be parsed by
 * the system getaddrinfo() function. IPv6 addresses cannot include
 * square brackets when calling this function. This exception is
 * also raised if the type of address is not recognized (i.e. we
 * only support IPv4 and IPv6 addresses.)
 *
 * \param[in] address  The address to convert to binary.
 * \param[in] port  The port to attach to the addr object.
 * \param[in] protocol  The name of the protocol ("tcp", "udp", or nullptr)
 */
void addr::set_addr_port(std::string const & address, int const port, char const * protocol)
{
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_flags = AI_NUMERICSERV | AI_ADDRCONFIG | AI_V4MAPPED;
    hints.ai_family = AF_UNSPEC;
    if(protocol != nullptr)
    {
        if(strcmp(protocol, "tcp") == 0)
        {
            hints.ai_socktype = SOCK_STREAM;
            hints.ai_protocol = IPPROTO_TCP;
        }
        else if(strcmp(protocol, "udp") == 0)
        {
            hints.ai_socktype = SOCK_DGRAM;
            hints.ai_protocol = IPPROTO_UDP;
        }
        else
        {
            throw addr_invalid_argument_exception(QString("unknown protocol \"%1\", expected \"tcp\" or \"udp\".").arg(protocol));
        }
    }
    // else -- should we have a default?

    QString port_str(QString("%1").arg(port));
    QByteArray port_str_utf8(port_str.toUtf8());

    // now addr is just the address and we can convert that to binary
    struct addrinfo * addrlist(nullptr);
    errno = 0;
    int const r(getaddrinfo(address.c_str(), port_str_utf8.data(), &hints, &addrlist));
    if(r != 0)
    {
        // break on invalid addresses
        //
        int const e(errno); // if r == EAI_SYSTEM, then 'errno' is consistent here
        throw addr_invalid_argument_exception(QString("invalid address in %1:%2, error %3 -- %4 (errno: %5 -- %6.")
                            .arg(QString::fromUtf8(address.c_str()))
                            .arg(port)
                            .arg(r)
                            .arg(gai_strerror(r))
                            .arg(e)
                            .arg(strerror(e)));
    }
    std::shared_ptr<struct addrinfo> ai(addrlist, addrinfo_deleter);

    if(addrlist->ai_family == AF_INET)
    {
        if(addrlist->ai_addrlen != sizeof(struct sockaddr_in))
        {
            throw addr_invalid_argument_exception(QString("Unsupported address size (%1, expected %2).").arg(addrlist->ai_addrlen).arg(sizeof(struct sockaddr_in)));
        }
        set_ipv4(*reinterpret_cast<struct sockaddr_in *>(addrlist->ai_addr));
    }
    else if(addrlist->ai_family == AF_INET6)
    {
        if(addrlist->ai_addrlen != sizeof(struct sockaddr_in6))
        {
            throw addr_invalid_argument_exception(QString("Unsupported address size (%1, expected %2).").arg(addrlist->ai_addrlen).arg(sizeof(struct sockaddr_in6)));
        }
        set_ipv6(*reinterpret_cast<struct sockaddr_in6 *>(addrlist->ai_addr));
    }
    else
    {
        throw addr_invalid_argument_exception(QString("Unsupported address family %1.").arg(addrlist->ai_family));
    }

    // save the protocol so we can create a socket if requested
    //
    f_protocol = addrlist->ai_protocol;

    address_changed();
}


/** \brief Save an IPv4 in this addr object.
 *
 * This function saves the specified IPv4 in this addr object.
 *
 * Since we save the data in an IPv6 structure, it is kept in
 * the addr as an IPv4 mapped in an IPv6 address. It can still
 * be retrieved right back as an IPv4 with the get_ipv4() function.
 *
 * \param[in] in  The IPv4 address to save in this addr object.
 */
void addr::set_ipv4(struct sockaddr_in const & in)
{
    // reset the address first
    memset(&f_address, 0, sizeof(f_address));

    // then transform the IPv4 to an IPv6
    //
    // Note: this is not an IPv6 per se, it is an IPv4 mapped within an
    //       IPv6 and your network anwway stack needs to support IPv4
    //       in order to use that IP...
    //
    f_address.sin6_family = AF_INET6;
    f_address.sin6_port = in.sin_port;
    f_address.sin6_addr.s6_addr16[5] = 0xFFFF;
    f_address.sin6_addr.s6_addr32[3] = in.sin_addr.s_addr;

    address_changed();
}


/** \brief Set the port of this address.
 *
 * This function changes the port of this address to \p port.
 *
 * \exception addr_invalid_argument_exception
 * This exception is raised whenever the \p port parameter is set to
 * an invalid number (negative or larger than 65535.)
 *
 * \param[in] port  The new port to save in this address.
 */
void addr::set_port(int port)
{
    if(port > 65535 
    || port < 0)
    {
        throw addr_invalid_argument_exception("port to set_port() cannot be out of the allowed range [0..65535].");
    }
    f_address.sin6_port = htons(port);
}


/** \brief Change the protocol.
 *
 * This function is used to change the current protocol defined in
 * this addr object.
 *
 * \exception addr_invalid_argument_exception
 * We currently support "tcp" and "udp". Any other protocol definition
 * generates this exception.
 *
 * \param[in] protocol  The name of the protocol ("tcp", "udp", or nullptr)
 */
void addr::set_protocol(char const * protocol)
{
    if(protocol == nullptr)
    {
        throw addr_invalid_argument_exception("protocol pointer to set_protocol() cannot be a nullptr.");
    }

    if(strcmp(protocol, "tcp") == 0)
    {
        f_protocol = IPPROTO_TCP;
    }
    else if(strcmp(protocol, "udp") == 0)
    {
        f_protocol = IPPROTO_UDP;
    }
    else
    {
        throw addr_invalid_argument_exception(QString("unknown protocol \"%1\", expected \"tcp\" or \"udp\".").arg(protocol));
    }

    address_changed();
}


/** \brief Check whether this address represents an IPv4 address.
 *
 * The IPv6 format supports embedding IPv4 addresses. This function
 * returns true if the embedded address is an IPv4. When this function
 * returns true, the get_ipv4() can be called. Otherwise, the get_ipv4()
 * function throws an error.
 *
 * \return true if this address represents an IPv4 address.
 */
bool addr::is_ipv4() const
{
    return f_address.sin6_addr.s6_addr32[0] == 0
        && f_address.sin6_addr.s6_addr32[1] == 0
        && f_address.sin6_addr.s6_addr16[4] == 0
        && f_address.sin6_addr.s6_addr16[5] == 0xFFFF;
}


/** \brief Retreive the IPv4 address.
 *
 * This function can be used to retrieve the IPv4 address of this addr
 * object. If the address is not an IPv4, then the function throws.
 *
 * \exception addr_invalid_structure_exception
 * This exception is raised if the address is not an IPv4 address.
 *
 * \param[out] in  The structure where the IPv4 Internet address gets saved.
 */
void addr::get_ipv4(struct sockaddr_in & in) const
{
    if(is_ipv4())
    {
        // this is an IPv4 mapped in an IPv6, "unmap" that IP
        //
        memset(&in, 0, sizeof(in));
        in.sin_family = AF_INET;
        in.sin_port = f_address.sin6_port;
        in.sin_addr.s_addr = f_address.sin6_addr.s6_addr32[3];
        return;
    }

    throw addr_invalid_argument_exception("Not an IPv4 compatible address.");
}


/** \brief Save the specified IPv6 address in this addr object.
 *
 * This function saves the specified IPv6 address in this addr object.
 * The function does not check the validity of the address. It is
 * expected to be valid.
 *
 * The address may be an embedded IPv4 address.
 *
 * \param[in] in6  The source IPv6 to save in the addr object.
 */
void addr::set_ipv6(struct sockaddr_in6 const & in6)
{
    memcpy(&f_address, &in6, sizeof(in6));

    address_changed();
}


/** \brief Retrieve a copy of this addr IP address.
 *
 * This function returns the current IP address saved in this
 * addr object. The IP may represent an IPv4 address in which
 * case the is_ipv4() returns true.
 *
 * \param[out] in6  The structure where the address gets saved.
 */
void addr::get_ipv6(struct sockaddr_in6 & in6) const
{
    memcpy(&in6, &f_address, sizeof(in6));
}


/** \brief Retrive the IPv4 as a string.
 *
 * This function returns a string representing the IP address
 * defined in this addr object.
 *
 * \exception addr_invalid_argument_exception
 * If the addr object does not currently represent an IPv4 then
 * this exception is raised.
 *
 * \param[in] include_port  Whether the port should be appended to the string.
 */
std::string addr::get_ipv4_string(bool include_port) const
{
    if(is_ipv4())
    {
        // this is an IPv4 mapped in an IPv6, "unmap" that IP
        // so the inet_ntop() can correctly generate an output IP
        //
        struct in_addr in;
        memset(&in, 0, sizeof(in));
        in.s_addr = f_address.sin6_addr.s6_addr32[3];
        char buf[INET_ADDRSTRLEN + 1];
        if(inet_ntop(AF_INET, &in, buf, sizeof(buf)) != nullptr)
        {
            if(include_port)
            {
                std::stringstream result;
                result << buf;
                result << ":";
                result << ntohs(f_address.sin6_port);
                return result.str();
            }
            return std::string(buf);
        }
        // IPv4 should never fail converting the address unless the
        // buffer was too small...
    }

    throw addr_invalid_argument_exception("Not an IPv4 compatible address.");
}


/** \brief Convert the addr object to a string.
 *
 * This function converts the addr object to a canonicalized string.
 * This can be used to compare two IPv6 together as strings, although
 * it is probably better to compare them using the < and == operators.
 *
 * By default the function returns with the IPv6 address defined
 * between square bracket, so the output of this function can be
 * used as the input of the set_addr_port() function. You may
 * also request the address without the brackets.
 *
 * \exception 
 * If include_brackets is false and include_port is true, this
 * exception is raised because we cannot furfill the request.
 *
 * \param[in] include_port  Whether the port should be added at the end of
 *            the string.
 * \param[in] include_brackets  Whether the square bracket characters
 *            should be included at all.
 *
 * \return The addr object converted to an IPv6 address.
 */
std::string addr::get_ipv6_string(bool include_port, bool include_brackets) const
{
    if(include_port && !include_brackets)
    {
        throw addr_invalid_parameter_exception("include_port cannot be true if include_brackets is false");
    }

    char buf[INET6_ADDRSTRLEN + 1];
    if(inet_ntop(AF_INET6, &f_address.sin6_addr, buf, sizeof(buf)) != nullptr)
    {
        std::stringstream result;
        if(include_brackets)
        {
            result << "[";
        }
        result << buf;
        if(include_brackets)
        {
            result << "]";
        }
        if(include_port)
        {
            result << ":";
            result << ntohs(f_address.sin6_port);
        }
        return result.str();
    }

    throw addr_invalid_argument_exception("The address from this addr could not be converted to a valid canonicalized IPv6 address.");
}


/** \brief Return the address as IPv4 or IPv6.
 *
 * Depending on whether the address represents an IPv4 or an IPv6,
 * this function returns the corresponding address. Since the format
 * of both types of addresses can always be distinguished, it poses
 * no concerns.
 *
 * \exception 
 * If include_brackets is false and include_port is true, this
 * exception is raised because we cannot furfill the request.
 *
 * \param[in] include_port  Whether the port should be added at the end of
 *            the string.
 * \param[in] include_brackets  Whether the square bracket characters
 *            should be included at all (ipv6 only).
 *
 * \return The addr object converted to an IPv4 or an IPv6 address.
 */
std::string addr::get_ipv4or6_string(bool include_port, bool include_brackets) const
{
    if(include_port && !include_brackets)
    {
        throw addr_invalid_parameter_exception("include_port cannot be true if include_brackets is false");
    }

    return is_ipv4() ? get_ipv4_string(include_port)
                     : get_ipv6_string(include_port, include_brackets);
}


/** \brief Determine the type of network this IP represents.
 *
 * The IP address may represent various type of networks. This
 * function returns that type.
 *
 * The function checks the address either as IPv4 when is_ipv4()
 * returns true, otherwise as IPv6.
 *
 * See
 *
 * \li https://en.wikipedia.org/wiki/Reserved_IP_addresses
 * \li https://tools.ietf.org/html/rfc3330
 * \li https://tools.ietf.org/html/rfc5735 (IPv4)
 * \li https://tools.ietf.org/html/rfc5156 (IPv6)
 *
 * \return One of the possible network types as defined in the
 *         network_type_t enumeration.
 */
addr::network_type_t addr::get_network_type() const
{
    if(f_private_network_defined == network_type_t::NETWORK_TYPE_UNDEFINED)
    {
        f_private_network_defined = network_type_t::NETWORK_TYPE_UNKNOWN;

        if(is_ipv4())
        {
            // get the address in host order
            //
            // we can use a simple mask + compare to know whether it is
            // this or that once in host order
            //
            uint32_t const host_ip(ntohl(f_address.sin6_addr.s6_addr32[3]));

            if((host_ip & 0xFF000000) == 0x0A000000         // 10.0.0.0/8
            || (host_ip & 0xFFF00000) == 0xAC100000         // 172.16.0.0/12
            || (host_ip & 0xFFFF0000) == 0xC0A80000)        // 192.168.0.0/16
            {
                f_private_network_defined = network_type_t::NETWORK_TYPE_PRIVATE;
            }
            else if((host_ip & 0xFFC00000) == 0x64400000)   // 100.64.0.0/10
            {
                f_private_network_defined = network_type_t::NETWORK_TYPE_CARRIER;
            }
            else if((host_ip & 0xFFFF0000) == 0xA9FE0000)   // 169.254.0.0/16
            {
                f_private_network_defined = network_type_t::NETWORK_TYPE_LINK_LOCAL; // i.e. DHCP
            }
            else if((host_ip & 0xF0000000) == 0xE0000000)   // 224.0.0.0/4
            {
                // there are many sub-groups on this one which are probably
                // still in use...
                f_private_network_defined = network_type_t::NETWORK_TYPE_MULTICAST;
            }
            else if((host_ip & 0xFF000000) == 0x7F000000)   // 127.0.0.0/8
            {
                f_private_network_defined = network_type_t::NETWORK_TYPE_LOOPBACK; // i.e. localhost
            }
            else if(host_ip == 0x00000000)
            {
                f_private_network_defined = network_type_t::NETWORK_TYPE_ANY; // i.e. 0.0.0.0
            }
        }
        else //if(is_ipv6()) -- if not IPv4, we have an IPv6
        {
            // for IPv6 it was simplified by using a prefix for
            // all types; really way easier than IPv4
            //
            if(f_address.sin6_addr.s6_addr32[0] == 0      // ::
            && f_address.sin6_addr.s6_addr32[1] == 0
            && f_address.sin6_addr.s6_addr32[2] == 0
            && f_address.sin6_addr.s6_addr32[3] == 0)
            {
                // this is the "any" IP address
                f_private_network_defined = network_type_t::NETWORK_TYPE_ANY;
            }
            else
            {
                uint16_t const prefix(ntohs(f_address.sin6_addr.s6_addr16[0]));

                if((prefix & 0xFF00) == 0xFD00)                 // fd00::/8
                {
                    f_private_network_defined = network_type_t::NETWORK_TYPE_PRIVATE;
                }
                else if((prefix & 0xFFC0) == 0xFE80    // fe80::/10
                     || (prefix & 0xFF0F) == 0xFF02)   // ffx2::/16
                {
                    f_private_network_defined = network_type_t::NETWORK_TYPE_LINK_LOCAL; // i.e. DHCP
                }
                else if((prefix & 0xFF0F) == 0xFF01    // ffx1::/16
                     || (f_address.sin6_addr.s6_addr32[0] == 0      // ::1
                      && f_address.sin6_addr.s6_addr32[1] == 0
                      && f_address.sin6_addr.s6_addr32[2] == 0
                      && f_address.sin6_addr.s6_addr16[6] == 0
                      && f_address.sin6_addr.s6_addr16[7] == htons(1)))
                {
                    f_private_network_defined = network_type_t::NETWORK_TYPE_LOOPBACK;
                }
                else if((prefix & 0xFF00) == 0xFF00)   // ff00::/8
                {
                    // this one must be after the link-local and loopback networks
                    f_private_network_defined = network_type_t::NETWORK_TYPE_MULTICAST;
                }
            }
        }
    }

    return f_private_network_defined;
}


/** \brief Get the network type string
 *
 * Translate the network type into a string.
 */
std::string addr::get_network_type_string() const
{
    std::string name;
    switch( get_network_type() )
    {
        case addr::network_type_t::NETWORK_TYPE_UNDEFINED  : name= "Undefined";  break;
        case addr::network_type_t::NETWORK_TYPE_PRIVATE    : name= "Private";    break;
        case addr::network_type_t::NETWORK_TYPE_CARRIER    : name= "Carrier";    break;
        case addr::network_type_t::NETWORK_TYPE_LINK_LOCAL : name= "Local Link"; break;
        case addr::network_type_t::NETWORK_TYPE_MULTICAST  : name= "Multicast";  break;
        case addr::network_type_t::NETWORK_TYPE_LOOPBACK   : name= "Loopback";   break;
        case addr::network_type_t::NETWORK_TYPE_ANY        : name= "Any";        break;
        case addr::network_type_t::NETWORK_TYPE_UNKNOWN    : name= "Unknown";    break;
    }
    return name;
}


/** \brief Retrieve the interface name
 *
 * This function retrieves the name of the interface of the address.
 * This is set using the get_local_addresses() static method.
 */
std::string addr::get_iface_name() const
{
    return f_iface_name;
}


/** \brief Transform the IP into a domain name.
 *
 * This function transforms the IP address in this `addr` object in a
 * name such as "snap.website".
 *
 * \note
 * The function does not cache the result because it is rarely used (at least
 * at this time). So you should cache the result and avoid calling this
 * function more than once as the process can be very slow.
 *
 * \todo
 * Speed enhancement can be achieved by using getaddrinfo_a(). That would
 * work with a vector of addr objects.
 *
 * \return The domain name. If not available, an empty string.
 */
std::string addr::get_name() const
{
    char host[NI_MAXHOST];

    int flags(NI_NAMEREQD);
    if(f_protocol == IPPROTO_UDP)
    {
        flags |= NI_DGRAM;
    }

    // TODO: test with the NI_IDN* flags and make sure we know what we get
    //       (i.e. we want UTF-8 as a result)
    //
    int const r(getnameinfo(reinterpret_cast<sockaddr const *>(&f_address), sizeof(f_address), host, sizeof(host), nullptr, 0, flags));

    // return value is 0, then it worked
    //
    return r == 0 ? host : std::string();
}


/** \brief Transform the port into a service name.
 *
 * This function transforms the port in this `addr` object in a
 * name such as "http".
 *
 * \note
 * The function does not cache the result because it is rarely used (at least
 * at this time). So you should cache the result and avoid calling this
 * function more than once as the process is somewhat slow.
 *
 * \return The service name. If not available, an empty string.
 */
std::string addr::get_service() const
{
    char service[NI_MAXSERV];

    int flags(NI_NAMEREQD);
    if(f_protocol == IPPROTO_UDP)
    {
        flags |= NI_DGRAM;
    }
    int const r(getnameinfo(reinterpret_cast<sockaddr const *>(&f_address), sizeof(f_address), nullptr, 0, service, sizeof(service), flags));

    // return value is 0, then it worked
    //
    return r == 0 ? service : std::string();
}


/** \brief Retrieve the port.
 *
 * This function retrieves the port of the IP address in host order.
 *
 * \return The port defined along this address.
 */
int addr::get_port() const
{
    return ntohs(f_address.sin6_port);
}


/** \brief Retrieve the protocol.
 *
 * This function retrieves the protocol as specified on the
 * set_addr_port() function or corresponding constructor.
 *
 * You may change the protocol with the set_protocol() function.
 *
 * \return protocol such as IPPROTO_TCP or IPPROTO_UDP.
 */
int addr::get_protocol() const
{
    return f_protocol;
}


/** \brief Check whether two addresses are equal.
 *
 * This function compares the left hand side (this) and the right
 * hand side (rhs) for equality. If both represent the same IP
 * address, then the function returns true.
 *
 * \warning
 * The function only compares the address itself. The family, port,
 * flow info, scope identifier, protocol are all ignored.
 *
 * \return true if \p this is equal to \p rhs.
 */
bool addr::operator == (addr const & rhs) const
{
    return f_address.sin6_addr == rhs.f_address.sin6_addr;
}


/** \brief Check whether two addresses are not equal.
 *
 * This function compares the left hand side (this) and the right
 * hand side (rhs) for inequality. If both represent the same IP
 * address, then the function returns false.
 *
 * \warning
 * The function only compares the address itself. The family, port,
 * flow info, scope identifier, protocol are all ignored.
 *
 * \return true if \p this is not equal to \p rhs.
 */
bool addr::operator != (addr const & rhs) const
{
    return f_address.sin6_addr != rhs.f_address.sin6_addr;
}


/** \brief Compare two addresses to know which one is smaller.
 *
 * This function compares the left hand side (this) and the right
 * hand side (rhs) to know which one is the smallest. If both
 * are equal or the left hand side is larger than the right hand
 * side, then it returns false, otherwise it returns true.
 *
 * \warning
 * The function only compares the address itself. The family, port,
 * flow info, scope identifier, protocol are all ignored.
 *
 * \return true if \p this is smaller than \p rhs.
 */
bool addr::operator < (addr const & rhs) const
{
    return f_address.sin6_addr < rhs.f_address.sin6_addr;
}


/** \brief Mark that the address changed.
 *
 * This functions makes sure that some of the parameters being cached
 * get reset in such a way that checking the cache will again return
 * the correct answer.
 *
 * \sa get_network_type()
 */
void addr::address_changed()
{
    f_private_network_defined = network_type_t::NETWORK_TYPE_UNDEFINED;
}


/** \brief Return a list of local addresses on this machine.
 *
 * Peruse the list of available interfaces, and return any detected ip addresses
 * in a vector.
 */
addr::vector_t addr::get_local_addresses()
{
    // get the list of interface addresses
    //
    struct ifaddrs * ifa_start(nullptr);
    if(getifaddrs(&ifa_start) != 0)
    {
        // TODO: Should this throw, or just return an empty list quietly?
        //
        return vector_t();
    }

    std::shared_ptr<struct ifaddrs> auto_free(ifa_start, ifaddrs_deleter);

    vector_t addr_list;
    for(struct ifaddrs * ifa(ifa_start); ifa != nullptr; ifa = ifa->ifa_next)
    {
        if( ifa->ifa_addr == nullptr ) continue;

        addr the_address;

        the_address.f_iface_name = ifa->ifa_name;
        uint16_t const family( ifa->ifa_addr->sa_family );
        if( family == AF_INET )
        {
            the_address.set_ipv4( *(reinterpret_cast<struct sockaddr_in *>(ifa->ifa_addr)) );
        }
        else if( family == AF_INET6 )
        {
            the_address.set_ipv6( *(reinterpret_cast<struct sockaddr_in6 *>(ifa->ifa_addr)) );
        }
        else
        {
            // TODO: can we just ignore invalid addresses?
            //throw addr_invalid_structure_exception( "Unknown address family!" );
            continue;
        }

        addr_list.push_back( the_address );
    }

    return addr_list;
}


/** \brief Check whether this address represents this computer.
 *
 * This function reads the addresses as given to us by the getifaddrs()
 * function. This is a system function that returns a complete list of
 * all the addresses this computer is managing / represents. In other
 * words, a list of address that other computers can use to connect
 * to this computer (assuming proper firewall, of course.)
 *
 * \warning
 * The list of addresses from getifaddrs() is not being cached. So you
 * probably do not want to call this function in a loop. That being
 * said, I still would imagine that retrieving that list is fast.
 *
 * \return a computer_interface_address_t enumeration: error, true, or
 *         false at this time; on error errno should be set to represent
 *         what the error was.
 */
addr::computer_interface_address_t addr::is_computer_interface_address() const
{
    // TBD: maybe we could cache the ifaddrs for a small amount of time
    //      (i.e. 1 minute) so additional calls within that time
    //      can go even faster?
    //

    // get the list of interface addresses
    //
    struct ifaddrs * ifa_start(nullptr);
    if(getifaddrs(&ifa_start) != 0)
    {
        return computer_interface_address_t::COMPUTER_INTERFACE_ADDRESS_ERROR;
    }
    std::shared_ptr<struct ifaddrs> auto_free(ifa_start, ifaddrs_deleter);

    bool const ipv4(is_ipv4());
    uint16_t const family(ipv4 ? AF_INET : AF_INET6);
    for(struct ifaddrs * ifa(ifa_start); ifa != nullptr; ifa = ifa->ifa_next)
    {
        if(ifa->ifa_addr != nullptr
        && ifa->ifa_addr->sa_family == family)
        {
            if(ipv4)
            {
                // the interface address structure is a 'struct sockaddr_in'
                //
                if(memcmp(&reinterpret_cast<struct sockaddr_in *>(ifa->ifa_addr)->sin_addr,
                            f_address.sin6_addr.s6_addr32 + 3, //&reinterpret_cast<struct sockaddr_in const *>(&f_address)->sin_addr,
                            sizeof(struct in_addr)) == 0)
                {
                    return computer_interface_address_t::COMPUTER_INTERFACE_ADDRESS_TRUE;
                }
            }
            else
            {
                // the interface address structure is a 'struct sockaddr_in6'
                //
                if(memcmp(&reinterpret_cast<struct sockaddr_in6 *>(ifa->ifa_addr)->sin6_addr, &f_address.sin6_addr, sizeof(f_address.sin6_addr)) == 0)
                {
                    return computer_interface_address_t::COMPUTER_INTERFACE_ADDRESS_TRUE;
                }
            }
        }
    }

    return computer_interface_address_t::COMPUTER_INTERFACE_ADDRESS_FALSE;
}


}
// snap_addr namespace
// vim: ts=4 sw=4 et
