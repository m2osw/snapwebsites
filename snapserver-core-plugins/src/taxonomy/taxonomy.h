// Snap Websites Server -- taxonomy
// Copyright (C) 2012-2017  Made to Order Software Corp.
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
#include "../content/content.h"


namespace snap
{
namespace taxonomy
{

enum class name_t
{
    SNAP_NAME_TAXONOMY_NAME,
    SNAP_NAME_TAXONOMY_NAMESPACE
};
const char * get_name(name_t name) __attribute__ ((const));


class taxonomy
        : public plugins::plugin
{
public:
                        taxonomy();
                        ~taxonomy();

    // plugins::plugin implementation
    static taxonomy *   instance();
    virtual QString     icon() const;
    virtual QString     description() const;
    virtual QString     dependencies() const;
    virtual int64_t     do_update(int64_t last_updated);
    virtual int64_t     do_dynamic_update(int64_t last_updated);
    virtual void        bootstrap(snap_child * snap);

    // content signals implementation
    void                on_copy_branch_cells(libdbproxy::cells & source_cells, libdbproxy::row::pointer_t destination_row, snap_version::version_number_t const destination_branch);

    libdbproxy::value    find_type_with(content::path_info_t & cpath, const QString & taxonomy, const QString & col_name, const QString & limit_name);

private:
    void                content_update(int64_t variables_timestamp);
    void                owner_update(int64_t variables_timestamp);

    snap_child *        f_snap = nullptr;
};

} // namespace taxonomy
} // namespace snap
// vim: ts=4 sw=4 et
