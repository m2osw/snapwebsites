// Snap Websites Server -- epayment_gateway_features_t implementation
// Copyright (c) 2011-2019  Made to Order Software Corp.  All Rights Reserved
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


/** \file
 * \brief The implementation of epayment_gateway_features_t.
 *
 * This file contains the implementation of the epayment_gateway_features_t
 * class.
 *
 * The class is used to gather detailed information of what a gateway
 * supports. For example, a gateway may support varying currencies,
 * repeat payments for subscriptions, checks, etc.
 */


// self
//
#include    "epayment_creditcard.h"


// last include
//
#include    <snapdev/poison.h>



namespace snap
{
namespace epayment_creditcard
{



epayment_gateway_features_t::epayment_gateway_features_t(QString const & gateway)
    : f_gateway(gateway)
{
}


QString epayment_gateway_features_t::get_gateway() const
{
    return f_gateway;
}


void epayment_gateway_features_t::set_name(QString const & name)
{
    f_name = name;
}


QString epayment_gateway_features_t::get_name() const
{
    return f_name;
}



} // namespace epayment_creditcard
} // namespace snap
// vim: ts=4 sw=4 et
