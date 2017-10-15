// Snap Websites Server -- Cassandra watchdog
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


namespace snap
{
namespace cassandra
{

enum class name_t
{
    SNAP_NAME_WATCHDOG_CASSANDRA_NAME
};
char const * get_name(name_t name) __attribute__ ((const));




class cassandra_exception : public snap_exception
{
public:
    cassandra_exception(char const *        what_msg) : snap_exception("cassandra", what_msg) {}
    cassandra_exception(std::string const & what_msg) : snap_exception("cassandra", what_msg) {}
    cassandra_exception(QString const &     what_msg) : snap_exception("cassandra", what_msg) {}
};

class cassandra_exception_invalid_argument : public cassandra_exception
{
public:
    cassandra_exception_invalid_argument(char const *        what_msg) : cassandra_exception(what_msg) {}
    cassandra_exception_invalid_argument(std::string const & what_msg) : cassandra_exception(what_msg) {}
    cassandra_exception_invalid_argument(QString const &     what_msg) : cassandra_exception(what_msg) {}
};





class cassandra
        : public plugins::plugin
{
public:
                        cassandra();
                        ~cassandra();

    // plugins::plugin implementation
    static cassandra *  instance();
    virtual QString     description() const;
    virtual QString     dependencies() const;
    virtual int64_t     do_update(int64_t last_updated);
    virtual void        bootstrap(snap_child * snap);

    // server signal
    void                on_process_watch(QDomDocument doc);

private:
    zpsnap_child_t      f_snap;
};

} // namespace cassandra
} // namespace snap
// vim: ts=4 sw=4 et
