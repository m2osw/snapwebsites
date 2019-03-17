// Snap Websites Server -- server side javascript environment
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
#pragma once

// other plugins
//
#include "../content/content.h"


namespace snap
{
namespace javascript
{

enum class name_t
{
    SNAP_NAME_JAVASCRIPT_MINIMIZED,
    SNAP_NAME_JAVASCRIPT_MINIMIZED_COMPRESSED
};
char const * get_name(name_t name) __attribute__ ((const));


//class javascript_exception : public snap_exception {};
//class javascript_exception_circular_dependencies : public javascript_exception {};

class javascript_dynamic_plugin
{
public:
    virtual             ~javascript_dynamic_plugin() {}

    virtual int         js_property_count() const = 0;
    virtual QVariant    js_property_get(QString const & name) const = 0;
    virtual QString     js_property_name(int index) const = 0;
    virtual QVariant    js_property_get(int index) const = 0;
};



class javascript
    : public plugins::plugin
{
public:
                        javascript();
                        javascript(javascript const & rhs) = delete;
    virtual             ~javascript() override;

    javascript &        operator = (javascript const & rhs) = delete;

    static javascript * instance();

    // plugins::plugin implementation
    virtual QString     icon() const override;
    virtual QString     description() const override;
    virtual QString     dependencies() const override;
    virtual int64_t     do_update(int64_t last_updated) override;
    virtual void        bootstrap(snap_child * snap) override;

    // content signals
    void                on_process_attachment(libdbproxy::row::pointer_t file_row, content::attachment_file const & file);
    void                on_check_attachment_security(content::attachment_file const & file, content::permission_flag & secure, bool const fast);

    void                register_dynamic_plugin(javascript_dynamic_plugin *p);
    QVariant            evaluate_script(QString const & script);

private:
    friend class javascript_plugins_iterator;
    friend class plugins_class;

    void                initial_update(int64_t variables_timestamp);
    void                content_update(int64_t variables_timestamp);

    snap_child *                            f_snap = nullptr;
    QVector<javascript_dynamic_plugin *>    f_dynamic_plugins = {};
};



} // namespace javascript
} // namespace snap
// vim: ts=4 sw=4 et
