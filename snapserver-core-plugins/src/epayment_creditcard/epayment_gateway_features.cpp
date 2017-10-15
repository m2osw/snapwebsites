// Snap Websites Server -- epayment_gateway_features_t implementation
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

#include "epayment_creditcard.h"

#include <snapwebsites/poison.h>


SNAP_PLUGIN_EXTENSION_START(epayment_creditcard)


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



SNAP_PLUGIN_EXTENSION_END()

// vim: ts=4 sw=4 et
