// Snap Websites Server -- dynamically create forms from your website
// Copyright (c) 2016-2019  Made to Order Software Corp.  All Rights Reserved
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
#include <snapwebsites/snapwebsites.h>
#include <snapwebsites/plugins.h>


namespace snap
{
namespace webform
{


enum class name_t
{
    SNAP_NAME_WEBFORM_NAME
};
char const * get_name(name_t name) __attribute__ ((const));



class webform
    : public plugins::plugin
{
public:
                        webform();
                        webform(webform const & rhs) = delete;
    virtual             ~webform() override;

    webform &           operator = (webform const & rhs) = delete;

    static webform *    instance();

    // plugins::plugin implementation
    virtual QString     settings_path() const override;
    virtual QString     icon() const override;
    virtual QString     description() const override;
    virtual QString     dependencies() const override;
    virtual int64_t     do_update(int64_t last_updated) override;
    virtual void        bootstrap(snap_child * snap) override;

private:
    void                content_update(int64_t variables_timestamp);

    snap_child *        f_snap = nullptr;
};

} // namespace webform
} // namespace snap
// vim: ts=4 sw=4 et
