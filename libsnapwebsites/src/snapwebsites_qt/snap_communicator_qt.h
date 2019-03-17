// Snap Communicator -- connection for Qt

// Copyright (c) 2018-2019  Made to Order Software Corp.  All Rights Reserved
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

#include "snapwebsites/snap_communicator.h"



namespace snap
{

class snap_communicator_no_connection_found : public snap_communicator_exception
{
public:
    snap_communicator_no_connection_found(char const *        what_msg) : snap_communicator_exception(what_msg) {}
    snap_communicator_no_connection_found(std::string const & what_msg) : snap_communicator_exception(what_msg) {}
    snap_communicator_no_connection_found(QString const &     what_msg) : snap_communicator_exception(what_msg) {}
};



class snap_qt_connection
    : public snap_communicator::snap_connection
{
public:
    typedef std::shared_ptr<snap_qt_connection> pointer_t;

                                snap_qt_connection();
    virtual                     ~snap_qt_connection() override;

    // implements snap_connection
    virtual int                 get_socket() const override;
    virtual bool                is_reader() const override;
    virtual void                process_read() override;

private:
    int                         f_fd = -1;
};


} // namespace snap
// vim: ts=4 sw=4 et
