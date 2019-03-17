// Snap Websites Server -- taxonomy
// Copyright (c) 2012-2019  Made to Order Software Corp.  All Rights Reserved
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
                        taxonomy(taxonomy const & rhs) = delete;
    virtual             ~taxonomy() override;

    taxonomy &          operator = (taxonomy const & rhs) = delete;

    static taxonomy *   instance();

    // plugins::plugin implementation
    virtual QString     icon() const override;
    virtual QString     description() const override;
    virtual QString     dependencies() const override;
    virtual int64_t     do_update(int64_t last_updated) override;
    virtual int64_t     do_dynamic_update(int64_t last_updated) override;
    virtual void        bootstrap(snap_child * snap) override;

    // content signals implementation
    void                on_copy_branch_cells(libdbproxy::cells & source_cells, libdbproxy::row::pointer_t destination_row, snap_version::version_number_t const destination_branch);

    libdbproxy::value   find_type_with(content::path_info_t & cpath, const QString & taxonomy, const QString & col_name, const QString & limit_name);
    content::path_info_t const &
                        get_type_ipath() const;

private:
    void                content_update(int64_t variables_timestamp);
    void                owner_update(int64_t variables_timestamp);

    snap_child *                    f_snap = nullptr;
    content::path_info_t::pointer_t f_tpath = content::path_info_t::pointer_t();
};

} // namespace taxonomy
} // namespace snap
// vim: ts=4 sw=4 et
