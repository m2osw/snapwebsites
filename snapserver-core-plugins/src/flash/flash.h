// Snap Websites Server -- flash implementation
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

// other plugins
//
#include "../filter/filter.h"


namespace snap
{
namespace flash
{


enum class name_t
{
    SNAP_NAME_FLASH_ANIMATION_PATH
};

char const * get_name(name_t name) __attribute__ ((const));


class flash_exception : public snap_exception
{
public:
    explicit flash_exception(char const *        what_msg) : snap_exception("Flash", what_msg) {}
    explicit flash_exception(std::string const & what_msg) : snap_exception("Flash", what_msg) {}
    explicit flash_exception(QString const &     what_msg) : snap_exception("Flash", what_msg) {}
};



class flash : public plugins::plugin
{
public:
                        flash();
                        ~flash();

    // plugins::plugin implementation
    static flash *      instance();
    virtual QString     settings_path() const;
    virtual QString     icon() const;
    virtual QString     description() const;
    virtual QString     dependencies() const;
    virtual int64_t     do_update(int64_t last_updated);
    virtual void        bootstrap(snap_child * snap);

private:
    void                content_update(int64_t variables_timestamp);

    snap_child *        f_snap = nullptr;
};


} // namespace flash
} // namespace snap
// vim: ts=4 sw=4 et
