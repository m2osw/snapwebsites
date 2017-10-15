// Snap Websites Server -- manage permissions for users, forms, etc.
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

#include "content.h"

#include <snapwebsites/log.h>
#include <snapwebsites/not_reached.h>
#include <snapwebsites/not_used.h>

#include <snapwebsites/poison.h>


SNAP_PLUGIN_EXTENSION_START(content)


namespace details
{

void call_page_status(snap_expr::variable_t & result, snap_expr::variable_t::variable_vector_t const & sub_results)
{
    if(sub_results.size() != 1)
    {
        throw snap_expr::snap_expr_exception_invalid_number_of_parameters("invalid number of parameters to call page_status() expected 1 parameter");
    }
    QString const path(sub_results[0].get_string("page_status(1)"));

    // get the status of the specified path
    path_info_t ipath;
    ipath.set_path(path);
    path_info_t::status_t const status(ipath.get_status());
    std::string const named_status(path_info_t::status_t::status_name_to_string(status.get_state()));

    //SNAP_LOG_WARNING("got in page_status(")(ipath.get_key())(") = \"")(named_status)("\".");

    // save the result
    QtCassandra::QCassandraValue value;
    value.setStringValue(named_status.c_str());
    result.set_value(snap_expr::variable_t::variable_type_t::EXPR_VARIABLE_TYPE_STRING, value);
}


void call_website_uri(snap_expr::variable_t & result, snap_expr::variable_t::variable_vector_t const & sub_results)
{
    if(sub_results.size() != 0)
    {
        throw snap_expr::snap_expr_exception_invalid_number_of_parameters("invalid number of parameters to call website_uri() expected no parameters");
    }

    snap_child * snap(content::content::instance()->get_snap());
    snap_uri const main_uri(snap->get_uri());
    QString const website_uri(main_uri.get_website_uri(false));

    // save the result
    QtCassandra::QCassandraValue value;
    value.setStringValue(website_uri);
    result.set_value(snap_expr::variable_t::variable_type_t::EXPR_VARIABLE_TYPE_STRING, value);
}


snap_expr::functions_t::function_call_table_t const content_functions[] =
{
    { // check whether a user has permissions to access a page
        "page_status",
        call_page_status
    },
    { // return the website URL without path
        "website_uri",
        call_website_uri
    },
    {
        nullptr,
        nullptr
    }
};


} // namespace details


void content::on_add_snap_expr_functions(snap_expr::functions_t & functions)
{
    functions.add_functions(details::content_functions);
}




SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
