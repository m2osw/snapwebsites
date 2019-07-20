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


// self
//
#include "snapwebsites/snap_cassandra.h"


// snapwebsites lib
//
#include "snapwebsites/snapwebsites.h"
#include "snapwebsites/snap_tables.h"
#include "snapwebsites/log.h"


// casswrapper lib
//
#include <casswrapper/schema.h>


// C lib
//
#include <unistd.h>


// last include
//
#include <snapdev/poison.h>





namespace snap
{


snap_cassandra::snap_cassandra()
{
    // empty
}


snap_cassandra::~snap_cassandra()
{
}


void snap_cassandra::connect()
{
    // We now connect to our proxy instead. This allows us to have many
    // permanent connections to Cassandra (or some other data store) and
    // not have to have threads (at least the C/C++ driver forces us to
    // have threads for asynchronous and timeout handling...)
    //
    snap_config config("snapdbproxy");
    tcp_client_server::get_addr_port(config["listen"], f_snapdbproxy_addr, f_snapdbproxy_port, "tcp");

//std::cerr << "snap proxy info: " << f_snapdbproxy_addr << " and " << f_snapdbproxy_port << "\n";
    f_cassandra = libdbproxy::libdbproxy::create();
    if(!f_cassandra)
    {
        QString const msg("could not create the libdbproxy instance.");
        SNAP_LOG_FATAL(msg);
        throw snap_cassandra_not_available_exception(msg);
    }

    if( !f_cassandra->connect(f_snapdbproxy_addr, f_snapdbproxy_port) )
    {
        QString const msg("could not connect libdbproxy to snapdbproxy.");
        SNAP_LOG_FATAL(msg);
        throw snap_cassandra_not_available_exception(msg);
    }

    // everything setup to QUORUM or we get really strange errors when under
    // load (without much load, it works like a charm with ONE).
    //
    // Note: the low level library forces everything to QUORUM anyway so
    //       this call is not really useful as it stands.
    //
    f_cassandra->setDefaultConsistencyLevel(libdbproxy::CONSISTENCY_LEVEL_QUORUM);
}


void snap_cassandra::disconnect()
{
    f_cassandra.reset();
}


libdbproxy::context::pointer_t snap_cassandra::get_snap_context()
{
    if( f_cassandra == nullptr )
    {
        QString msg("You must connect to cassandra first!");
        SNAP_LOG_FATAL(msg);
        throw snap_cassandra_not_available_exception(msg);
    }

    // we need to read all the contexts in order to make sure the
    // findContext() works properly
    //
    f_cassandra->getContexts();
    QString const context_name(snap::get_name(snap::name_t::SNAP_NAME_CONTEXT));
    return f_cassandra->findContext(context_name);
}


QString snap_cassandra::get_snapdbproxy_addr() const
{
    return f_snapdbproxy_addr;
}


int32_t snap_cassandra::get_snapdbproxy_port() const
{
    return f_snapdbproxy_port;
}


bool snap_cassandra::is_connected() const
{
    if(!f_cassandra)
    {
        return false;
    }
    return f_cassandra->isConnected();
}


libdbproxy::table::pointer_t snap_cassandra::get_table(QString const & table_name)
{
    libdbproxy::context::pointer_t context(get_snap_context());
    if(!context)
    {
        throw snap_cassandra_not_available_exception("The snap_websites context is not available in this Cassandra database.");
    }

    // does table exist?
    libdbproxy::table::pointer_t table(context->findTable(table_name));
    if(!table)
    {
        SNAP_LOG_FATAL("could not find table \"")(table_name)("\" in Cassandra.");
        throw snap_cassandra_not_available_exception(QString("Table \"%1\" does not exist. Did you install a *-tables.xml file for it?").arg(table_name));
    }

    return table;
}





}
// namespace snap
// vim: ts=4 sw=4 et
