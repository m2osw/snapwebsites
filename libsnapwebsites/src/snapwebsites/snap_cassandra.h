// Snap Websites Server -- snap websites server
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

// libexcept
//
#include    <libexcept/exception.h>


// advgetopt
//
#include    <advgetopt/conf_file.h>


// libdbproxy
//
#include    <libdbproxy/libdbproxy.h>



namespace snap
{

DECLARE_MAIN_EXCEPTION(snap_cassandra_exception);

DECLARE_EXCEPTION(snap_cassandra_exception, snap_cassandra_not_available_exception);



class snap_cassandra
{
public:
    typedef std::shared_ptr<snap_cassandra>     pointer_t;

                                        snap_cassandra();
                                        ~snap_cassandra();

    void                                connect();
    void                                disconnect();
    libdbproxy::context::pointer_t      get_snap_context();
    libdbproxy::table::pointer_t        get_table(QString const & table_name);

    QString                             get_snapdbproxy_addr() const;
    int32_t                             get_snapdbproxy_port() const;
    bool                                is_connected() const;

private:
    libdbproxy::libdbproxy::pointer_t   f_cassandra = libdbproxy::libdbproxy::pointer_t();
    QString                             f_snapdbproxy_addr = "localhost";
    int                                 f_snapdbproxy_port = 4042;
    QMap<QString, bool>                 f_created_table = QMap<QString, bool>();
};


}
// namespace snap

// vim: ts=4 sw=4 et
