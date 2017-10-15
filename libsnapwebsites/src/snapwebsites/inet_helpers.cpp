// Snap Websites Server -- inet helper functions
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

#include "snapwebsites/inet_helpers.h"

#include <iostream>

#include "snapwebsites/poison.h"


namespace snap
{
namespace snap_inet
{




/** \brief Convert a string to a in6_addr structure.
 *
 * This function is similar to inet_pton() only it always converts
 * the address to an IPv6 address. The input may be an IPv4 address
 * or an IPv6 address.
 *
 * If the input is neither an IPv4 nor an IPv6 address, the function
 * returns 0 and sets errno to EAFNOSUPPORT.
 *
 * \note
 * The function clears the IP address in dst if an error occurs.
 * In other words you end up with "IN6ADDR_ANY".
 *
 * \note
 * An IPv4 in an IPv6 address is just preceded by "::ffff:". So
 * in most cases you do not need such.
 *
 * \warning
 * IPv4 embedded in an IPv6 address should not be used over the wire.
 * In most cases this function is used to save IP address in binary
 * in one format rather than having to know which format the address
 * was being saved as (i.e. the row or column key of some data in
 * Cassandra.)
 *
 * \param[in] src  The string representing an IP address.
 * \param[out] dst  A pointer to a in6_addr buffer.
 *
 * \return 0 on error and errno set to EAFNOSUPPORT, otherwise 1
 *         and dst set to a valid IPv6 address.
 */
int inet_pton_v6(char const * src, struct in6_addr * dst)
{
    int const r6(inet_pton(AF_INET6, src, dst));
    if(r6 != 1)
    {
        // try again with IPv4
        struct in_addr addr;
        int const r4(inet_pton(AF_INET, src, &addr));
        if(r4 != 1)
        {
            // reset the address, it is not good
            for(size_t i(0); i < sizeof(in6_addr); ++i)
            {
                reinterpret_cast<char *>(dst)[i] = 0;
            }
            return 0;
        }

        // convert an IPv4 to an IPv6... (i.e. embedding)
        std::string ipv6("::ffff:");
        ipv6 += src;
        int const r46(inet_pton(AF_INET6, ipv6.c_str(), dst));
        if(r46 != 1)
        {
            // somehow it worked as IPv4, but not IPv6?!?
            for(size_t i(0); i < sizeof(in6_addr); ++i)
            {
                reinterpret_cast<char *>(dst)[i] = 0;
            }
            errno = EAFNOSUPPORT;
            return 0;
        }
    }

    // it work, user has result in 'dst'
    return 1;
}


} // snap_inet namespace
} // snap namespace
// vim: ts=4 sw=4 et
