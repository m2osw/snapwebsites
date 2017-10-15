// Snap Websites Server -- epayment_creditcard_info_t implementation
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
 * \brief The implementation of epayment_creditcard_info_t.
 *
 * This file contains the implementation of the epayment_creditcard_info_t
 * class.
 *
 * The class is used whenever a client sends his credit card information.
 * It is expected that the current credit card processing facility that
 * you offer be sent that information to actually charge the client's
 * credit card.
 */

#include "epayment_creditcard.h"

#include <snapwebsites/poison.h>


SNAP_PLUGIN_EXTENSION_START(epayment_creditcard)


void epayment_creditcard_info_t::set_user_name(QString const & user_name)
{
    f_user_name = user_name;
}


QString epayment_creditcard_info_t::get_user_name() const
{
    return f_user_name;
}


void epayment_creditcard_info_t::set_creditcard_number(QString const & creditcard_number)
{
    f_creditcard_number = creditcard_number;
}


QString epayment_creditcard_info_t::get_creditcard_number() const
{
    return f_creditcard_number;
}


void epayment_creditcard_info_t::set_security_code(QString const & security_code)
{
    f_security_code = security_code;
}


QString epayment_creditcard_info_t::get_security_code() const
{
    return f_security_code;
}


void epayment_creditcard_info_t::set_expiration_date_month(QString const & expiration_date_month)
{
    f_expiration_date_month = expiration_date_month;
}


QString epayment_creditcard_info_t::get_expiration_date_month() const
{
    return f_expiration_date_month;
}


void epayment_creditcard_info_t::set_expiration_date_year(QString const & expiration_date_year)
{
    f_expiration_date_year = expiration_date_year;
}


QString epayment_creditcard_info_t::get_expiration_date_year() const
{
    return f_expiration_date_year;
}


void epayment_creditcard_info_t::set_billing_business_name(QString const & business_name)
{
    f_billing_business_name = business_name;
}


QString epayment_creditcard_info_t::get_billing_business_name() const
{
    return f_billing_business_name;
}


void epayment_creditcard_info_t::set_billing_attention(QString const & attention)
{
    f_billing_attention = attention;
}


QString epayment_creditcard_info_t::get_billing_attention() const
{
    return f_billing_attention;
}


void epayment_creditcard_info_t::set_billing_address1(QString const & address1)
{
    f_billing_address1 = address1;
}


QString epayment_creditcard_info_t::get_billing_address1() const
{
    return f_billing_address1;
}


void epayment_creditcard_info_t::set_billing_address2(QString const & address2)
{
    f_billing_address2 = address2;
}


QString epayment_creditcard_info_t::get_billing_address2() const
{
    return f_billing_address2;
}


void epayment_creditcard_info_t::set_billing_city(QString const & city)
{
    f_billing_city = city;
}


QString epayment_creditcard_info_t::get_billing_city() const
{
    return f_billing_city;
}


void epayment_creditcard_info_t::set_billing_province(QString const & province)
{
    f_billing_province = province;
}


QString epayment_creditcard_info_t::get_billing_province() const
{
	return f_billing_province;
}


void epayment_creditcard_info_t::set_billing_postal_code(QString const & postal_code)
{
	f_billing_postal_code = postal_code;
}


QString epayment_creditcard_info_t::get_billing_postal_code() const
{
	return f_billing_postal_code;
}


void epayment_creditcard_info_t::set_billing_country(QString const & country)
{
	f_billing_country = country;
}


QString epayment_creditcard_info_t::get_billing_country() const
{
	return f_billing_country;
}


void epayment_creditcard_info_t::set_delivery_business_name(QString const & business_name)
{
    f_delivery_business_name = business_name;
}


QString epayment_creditcard_info_t::get_delivery_business_name() const
{
    return f_delivery_business_name;
}


void epayment_creditcard_info_t::set_delivery_attention(QString const & attention)
{
    f_delivery_attention = attention;
}


QString epayment_creditcard_info_t::get_delivery_attention() const
{
    return f_delivery_attention;
}


void epayment_creditcard_info_t::set_delivery_address1(QString const & address1)
{
    f_delivery_address1 = address1;
}


QString epayment_creditcard_info_t::get_delivery_address1() const
{
    return f_delivery_address1;
}


void epayment_creditcard_info_t::set_delivery_address2(QString const & address2)
{
    f_delivery_address2 = address2;
}


QString epayment_creditcard_info_t::get_delivery_address2() const
{
    return f_delivery_address2;
}


void epayment_creditcard_info_t::set_delivery_city(QString const & city)
{
    f_delivery_city = city;
}


QString epayment_creditcard_info_t::get_delivery_city() const
{
    return f_delivery_city;
}


void epayment_creditcard_info_t::set_delivery_province(QString const & province)
{
    f_delivery_province = province;
}


QString epayment_creditcard_info_t::get_delivery_province() const
{
	return f_delivery_province;
}


void epayment_creditcard_info_t::set_delivery_postal_code(QString const & postal_code)
{
	f_delivery_postal_code = postal_code;
}


QString epayment_creditcard_info_t::get_delivery_postal_code() const
{
	return f_delivery_postal_code;
}


void epayment_creditcard_info_t::set_delivery_country(QString const & country)
{
	f_delivery_country = country;
}


QString epayment_creditcard_info_t::get_delivery_country() const
{
	return f_delivery_country;
}


void epayment_creditcard_info_t::set_phone(QString const & phone)
{
	f_phone = phone;
}


QString epayment_creditcard_info_t::get_phone() const
{
	return f_phone;
}

void epayment_creditcard_info_t::set_subscription(bool subscription)
{
    f_subscription = subscription;
}


bool epayment_creditcard_info_t::get_subscription() const
{
    return f_subscription;
}


SNAP_PLUGIN_EXTENSION_END()

// vim: ts=4 sw=4 et
