// Snap Websites Server -- sortable so end users can sort lists
// Copyright (C) 2016-2017  Made to Order Software Corp.
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
#include "../editor/editor.h"


namespace snap
{
namespace sortable
{


enum class name_t
{
    SNAP_NAME_SORTABLE_CHECK_BLACKLIST
};
char const * get_name(name_t name) __attribute__ ((const));


class sortable_exception : public snap_exception
{
public:
    explicit sortable_exception(char const *        what_msg) : snap_exception("versions", what_msg) {}
    explicit sortable_exception(std::string const & what_msg) : snap_exception("versions", what_msg) {}
    explicit sortable_exception(QString const &     what_msg) : snap_exception("versions", what_msg) {}
};

class sortable_exception_invalid_content_xml : public sortable_exception
{
public:
    explicit sortable_exception_invalid_content_xml(char const *        what_msg) : sortable_exception(what_msg) {}
    explicit sortable_exception_invalid_content_xml(std::string const & what_msg) : sortable_exception(what_msg) {}
    explicit sortable_exception_invalid_content_xml(QString const &     what_msg) : sortable_exception(what_msg) {}
};






class sortable
        : public plugins::plugin
{
public:
                        sortable();
                        ~sortable();

    // plugins::plugin implementation
    static sortable *   instance();
    virtual QString     icon() const;
    virtual QString     description() const;
    virtual QString     dependencies() const;
    virtual int64_t     do_update(int64_t last_updated);
    virtual void        bootstrap(snap_child * snap);

    // editor signals
    void                on_prepare_editor_form(editor::editor * e);

private:
    void                content_update(int64_t variables_timestamp);

    snap_child *        f_snap = nullptr;
};


} // namespace versions
} // namespace snap
// vim: ts=4 sw=4 et
