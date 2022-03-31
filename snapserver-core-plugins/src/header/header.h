// Snap Websites Server -- header management (HEAD tags and HTTP headers)
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
#include "../layout/layout.h"


namespace snap
{
namespace header
{


enum class name_t
{
    SNAP_NAME_HEADER_GENERATOR,
    SNAP_NAME_HEADER_INTERNAL
};
const char *get_name(name_t name) __attribute__ ((const));


class header_exception : public snap_exception
{
public:
    explicit header_exception(char const *        what_msg) : snap_exception("Header", what_msg) {}
    explicit header_exception(std::string const & what_msg) : snap_exception("Header", what_msg) {}
    explicit header_exception(QString const &     what_msg) : snap_exception("Header", what_msg) {}
};




class header
    : public cppthread::plugin
    , public path::path_execute
    , public layout::layout_content
{
public:
                        header();
                        header(header const & rhs) = delete;
    virtual             ~header() override;

    header &            operator = (header const & rhs) = delete;

    static header *     instance();

    // plugins::plugin implementation
    virtual int64_t     do_update(int64_t last_updated) override;
    virtual void        bootstrap(snap_child * snap) override;

    // path::path_execute implementation
    virtual bool        on_path_execute(content::path_info_t & ipath) override;

    // layout::layout_content implementation
    virtual void        on_generate_main_content(content::path_info_t & ipath, QDomElement & page, QDomElement & body) override;

    // layout signals
    void                on_generate_header_content(content::path_info_t & ipath, QDomElement & header_dom, QDomElement & metadata);

private:
    void                content_update(int64_t variables_timestamp);

    snap_child *        f_snap = nullptr;
};


} // namespace header
} // namespace snap
// vim: ts=4 sw=4 et
