// Snap Websites Servers -- HTTP link handling (outgoing)
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
#pragma once

// libsnapwebsites
//
#include "snapwebsites/snap_exception.h"

// C++
//
#include <map>


namespace snap
{

class http_link_exception : public snap_exception
{
public:
    http_link_exception(char const *        what_msg) : snap_exception("http_link", what_msg) {}
    http_link_exception(std::string const & what_msg) : snap_exception("http_link", what_msg) {}
    http_link_exception(QString const &     what_msg) : snap_exception("http_link", what_msg) {}
};


class http_link_parse_exception : public http_link_exception
{
public:
    http_link_parse_exception(char const *        what_msg) : http_link_exception(what_msg) {}
    http_link_parse_exception(std::string const & what_msg) : http_link_exception(what_msg) {}
    http_link_parse_exception(QString const &     what_msg) : http_link_exception(what_msg) {}
};


class http_link_parameter_exception : public http_link_exception
{
public:
    http_link_parameter_exception(char const *        what_msg) : http_link_exception(what_msg) {}
    http_link_parameter_exception(std::string const & what_msg) : http_link_exception(what_msg) {}
    http_link_parameter_exception(QString const &     what_msg) : http_link_exception(what_msg) {}
};



class snap_child;

class http_link
{
public:
    typedef std::map<std::string, http_link>        map_t;
    typedef std::map<std::string, std::string>      param_t;

                        // define a default http_link() because of map_t
                        http_link();

                        http_link(snap_child * snap, std::string const & link, std::string const & rel);

    std::string         get_name() const;

    void                set_redirect(bool redirect = true);
    bool                get_redirect() const;

    void                add_param(std::string const & name, std::string const & value);

    bool                has_param(std::string const & name) const;
    std::string         get_param(std::string const & name) const;

    param_t const &     get_params() const;

    std::string         to_http_header() const;

private:
    snap_child *        f_snap = nullptr;   // the snap child that created this link
    std::string         f_link;             // this link URI
    std::string         f_rel;              // rel attribute, a.k.a. link name
    bool                f_redirect = false; // whether to include this link in a redirect header
    param_t             f_params;           // other attributes
};



} // namespace snap
// vim: ts=4 sw=4 et
