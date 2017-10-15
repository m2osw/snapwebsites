// Snap Websites Server -- manage HTTP link to be sent to the browser
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

#include "snapwebsites/http_link.h"

#include "snapwebsites/log.h"
#include "snapwebsites/snapwebsites.h"

#include <QtCassandra/QCassandra.h>

#include <sys/time.h>

#include "snapwebsites/poison.h"


namespace snap
{


namespace
{



} // no name namespace


http_link::http_link()
    : f_snap(nullptr)
    //, f_link() -- auto-init
    //, f_rel() -- auto-init
    //, f_redirect(false) -- auto-init
    //, f_params() -- auto-init
{
}


/** \brief Initializes the link.
 *
 * This function initializes the link.
 *
 * \param[in] snap  The snap child creating this link.
 * \param[in] link  The URI for this link.
 * \param[in] rel  What the link represents (relative).
 *
 * \sa add_param()
 * \sa has_param()
 * \sa get_param()
 * \sa get_params()
 * \sa to_http_header()
 */
http_link::http_link(snap_child * snap, std::string const & link, std::string const & rel)
    : f_snap(snap)
    , f_link(link)
    , f_rel(rel)
    //, f_redirect(false) -- auto-init
    //, f_params() -- auto-init
{
    if(f_link.empty())
    {
        throw http_link_parse_exception("the URI of a link cannot be empty");
    }
    snap_uri uri;
    if(!uri.set_uri(QString::fromUtf8(f_link.c_str())))
    {
        // the snap_uri uses libtld so we arrive here if the TLD is not
        // considered valid
        //
        throw http_link_parse_exception("link URI is not valid");
    }
}


/** \brief Retrieve the link "name".
 *
 * This function returns the relation string for this link. This is most
 * often viewed as the link name. Links are saved in snap_child by name.
 * You cannot have two links with the same name.
 *
 * \return The "rel" string as passed to the constructor.
 */
std::string http_link::get_name() const
{
    return f_rel;
}


/** \brief Set whether to include this link on a redirect or not.
 *
 * Whenever Snap! generates a 301 or a 302, links do not get added to
 * the header (there are generally useless there in that situation.)
 *
 * By calling this function with true you indicate that it should be
 * added whether the process is about to redirect the client to another
 * page or not.
 *
 * See the shortcut implementation for an example where this is used.
 *
 * \param[in] redirect  Whether to include the link on a redirect.
 */
void http_link::set_redirect(bool redirect)
{
    f_redirect = redirect;
}


/** \brief Check whether to add this link on a redirect.
 *
 * By default links do not get added to the header if the request
 * ends up in a redirect.
 *
 * If this function returns true, then it will be added to the header.
 *
 * \return true if link is expected to be added on normal or redirect calls.
 */
bool http_link::get_redirect() const
{
    return f_redirect;
}


/** \brief Add a parameter to this link.
 *
 * Each link accept any number of parameter, although usually it will be
 * limited to just 2 or 3. The "rel" parameter is defined on construction
 * and cannot be re-added or modified with this function.
 *
 * The name of the parameter cannot be the empty string.
 *
 * If that parameter already exists, its value gets replaced.
 *
 * \param[in] name  The parameter name.
 * \param[in] value  The value for the named parameter.
 */
void http_link::add_param(std::string const & name, std::string const & value)
{
    if(name.empty())
    {
        throw http_link_parameter_exception("the name of a link parameter cannot be empty");
    }
    if(name == "rel")
    {
        throw http_link_parameter_exception("the rel link parameter cannot be modified, it is set on construction");
    }
    for(char const * s(name.c_str()); *s != '\0'; ++s)
    {
        if(*s < 'a' || *s > 'z')
        {
            // this is probably wrong, but right now that's all we need
            // extend as required
            throw http_link_parameter_exception("the name of a link parameter must be defined with lowercase letters only (a-z)");
        }
    }

    f_params[name] = value;
}


/** \brief Check whether a named parameter exists.
 *
 * This function checks whether a parameter with the specified name exists
 * in the list of parameters.
 *
 * \param[in] name  The new value of the cookie.
 *
 * \return true if the named parameter is defined.
 */
bool http_link::has_param(std::string const & name) const
{
    return f_params.find(name) != f_params.end();
}


/** \brief Retrieve the value of a parameter.
 *
 * This function returns the value of the specified parameter. If the
 * parameter is not defined, the empty string is returned.
 *
 * \param[in] name  The name of the parameter to read.
 *
 * \sa add_param();
 */
std::string http_link::get_param(std::string const & name) const
{
    if(has_param(name))
    {
        auto const it(f_params.find(name));
        return it->second;
    }
    return std::string();
}


/** \brief Get the complete list of parameters.
 *
 * This function can be used to retrieve the entire list of parameters from
 * this link.
 *
 * \warning
 * By default the function returns a reference meaning that a call to
 * add_param() will change the map you are dealing with here.
 *
 * \return The list of parameters, a map.
 */
http_link::param_t const & http_link::get_params() const
{
    return f_params;
}


/** \brief Transform the link for the HTTP header.
 *
 * This function transforms the link so it works as an HTTP header.
 *
 * \warning
 * This generates that one link string. Not the actual header. The
 * header requires all the links to be added in one "Link: ..." entry.
 *
 * See https://tools.ietf.org/html/rfc5988
 *
 * \return A valid HTTP link header.
 */
std::string http_link::to_http_header() const
{
    // Note: the name was already checked for invalid characters
    std::string result;

    result += "<";
    result += f_link;
    result += ">; rel=";
    result += f_rel;

    for(auto p : f_params)
    {
        // Note: if we test the value of each parameter, then we could
        //       already know whether it needs quoting or not when we
        //       reach here
        //
        result += "; ";
        result += p.first;
        result += "=\"";
        result += p.second;
        result += "\"";
    }

    return result;
}



} // namespace snap

// vim: ts=4 sw=4 et
