// Snap Websites Server -- antihammering plugin
// Copyright (c) 2013-2019  Made to Order Software Corp.  All Rights Reserved
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

// other plugins
//
#include "../path/path.h"


namespace snap
{
namespace antihammering
{


enum class name_t
{
    SNAP_NAME_ANTIHAMMERING_BLOCKED,
    SNAP_NAME_ANTIHAMMERING_FIRST_PAUSE,
    SNAP_NAME_ANTIHAMMERING_HIT_LIMIT,
    SNAP_NAME_ANTIHAMMERING_HIT_LIMIT_DURATION,
    SNAP_NAME_ANTIHAMMERING_SECOND_PAUSE,
    SNAP_NAME_ANTIHAMMERING_TABLE,
    SNAP_NAME_ANTIHAMMERING_THIRD_PAUSE
};
char const * get_name(name_t name) __attribute__ ((const));


class antihammering_exception : public snap_exception
{
public:
    explicit antihammering_exception(char const *        what_msg) : snap_exception("Antihammering URL", what_msg) {}
    explicit antihammering_exception(std::string const & what_msg) : snap_exception("Antihammering URL", what_msg) {}
    explicit antihammering_exception(QString const &     what_msg) : snap_exception("Antihammering URL", what_msg) {}
};



class antihammering
    : public plugins::plugin
{
public:
                            antihammering();
                            antihammering(antihammering const & rhs) = delete;
    virtual                 ~antihammering() override;

    antihammering &         operator = (antihammering const & rhs) = delete;

    static antihammering *  instance();

    // plugins::plugin implementation
    virtual QString         icon() const override;
    virtual QString         description() const override;
    virtual QString         settings_path() const override;
    virtual QString         dependencies() const override;
    virtual int64_t         do_update(int64_t last_updated) override;
    virtual void            bootstrap(snap_child * snap) override;

    libdbproxy::table::pointer_t get_antihammering_table();

    // server signals
    void                    on_output_result(QString const & uri_path, QByteArray & output);

    // path signals
    void                    on_check_for_redirect(content::path_info_t & ipath);

private:
    void                    content_update(int64_t variables_timestamp);

    snap_child *                    f_snap = nullptr;
    libdbproxy::table::pointer_t    f_antihammering_table = libdbproxy::table::pointer_t();
};


} // namespace antihammering
} // namespace snap
// vim: ts=4 sw=4 et
