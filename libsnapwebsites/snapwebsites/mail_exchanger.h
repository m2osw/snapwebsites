// Snap Websites Servers -- snap websites mail exchangers loading
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
#pragma once

// snapwebsites lib
//
#include "snapwebsites/snap_exception.h"

// C++ lib
//
#include <vector>



namespace snap
{





class mail_exchanger
{
public:
    typedef std::vector<mail_exchanger>     mail_exchange_vector_t;

                    mail_exchanger(int priority, std::string const & domain);

    int             get_priority() const;
    std::string     get_domain() const;

    bool            operator < (mail_exchanger const & rhs) const;

private:
    int             f_priority = 0;
    std::string     f_domain = std::string();
};






class mail_exchangers
{
public:

                    mail_exchangers(std::string const & domain);

    bool            domain_found() const;
    size_t          size() const;
    mail_exchanger::mail_exchange_vector_t get_mail_exchangers() const;

private:
    bool            f_domain_found = false;
    mail_exchanger::mail_exchange_vector_t f_mail_exchangers = mail_exchanger::mail_exchange_vector_t();
};



} // namespace snap
// vim: ts=4 sw=4 et
