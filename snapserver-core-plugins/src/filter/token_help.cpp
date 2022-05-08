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
 * \brief The implementation of the filter::token_help_t class.
 *
 * This file contains the implementation of the token_help_t class defined
 * in the filter system. It is used to gather help information about all
 * the available tokens in the system.
 */


// self
//
#include    "filter.h"


// snapwebsites
//
#include    <snapwebsites/qdomhelpers.h>
#include    <snapwebsites/xslt.h>


// snaplogger
//
#include    <snaplogger/message.h>


// C++
//
#include    <iostream>


// last include
//
#include    <snapdev/poison.h>



namespace snap
{
namespace filter
{



filter::token_help_t::token_help_t()
{
    f_root_tag = f_doc.createElement("snap");
    f_doc.appendChild(f_root_tag);

    f_help_tag = f_doc.createElement("token-help");
    f_root_tag.appendChild(f_help_tag);
}


void filter::token_help_t::add_token(QString const & token, QString const & help)
{
    QDomElement token_tag(f_doc.createElement("token"));
    token_tag.setAttribute("name", token);
    f_help_tag.appendChild(token_tag);

    snap_dom::insert_html_string_to_xml_doc(token_tag, help);
}


QString filter::token_help_t::result()
{
    xslt x;
    x.set_xsl_from_file(":/xsl/filter/token-help.xsl");
    x.set_document(f_doc);
    return x.evaluate_to_string();
}



} // namespace filter
} // namespace snap
// vim: ts=4 sw=4 et
