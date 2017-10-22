// Snap Websites Server -- test_plugin to run plugin unit tests from the browser
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
#include "../path/path.h"
#include "../filter/filter.h"


namespace snap
{
namespace test_plugin
{


enum class name_t
{
    SNAP_NAME_TEST_PLUGIN_DURATION_FIELD,
    SNAP_NAME_TEST_PLUGIN_END_DATE,
    SNAP_NAME_TEST_PLUGIN_END_DATE_FIELD,
    SNAP_NAME_TEST_PLUGIN_RESULT_FIELD,
    SNAP_NAME_TEST_PLUGIN_START_DATE,
    SNAP_NAME_TEST_PLUGIN_START_DATE_FIELD,
    SNAP_NAME_TEST_PLUGIN_SUCCESS,
    SNAP_NAME_TEST_PLUGIN_TEST_NAME_FIELD,
    SNAP_NAME_TEST_PLUGIN_TEST_RESULTS_TABLE
};
char const * get_name(name_t name) __attribute__ ((const));


class test_plugin_exception : public snap_exception
{
public:
    explicit test_plugin_exception(char const *        what_msg) : snap_exception("Test Plugin", what_msg) {}
    explicit test_plugin_exception(std::string const & what_msg) : snap_exception("Test Plugin", what_msg) {}
    explicit test_plugin_exception(QString const &     what_msg) : snap_exception("Test Plugin", what_msg) {}
};





class test_plugin
        : public plugins::plugin
        , public path::path_execute
{
public:
                            test_plugin();
                            ~test_plugin();

    // plugins::plugin implementation
    static test_plugin *    instance();
    virtual QString         settings_path() const;
    virtual QString         icon() const;
    virtual QString         description() const;
    virtual QString         help_uri() const;
    virtual QString         dependencies() const;
    virtual int64_t         do_update(int64_t last_updated);
    virtual void            bootstrap(snap_child * snap);

    // server signals
    void                    on_process_post(QString const & uri_path);

    // path::path_execute implementation
    virtual bool            on_path_execute(content::path_info_t & ipath);

    // filter signals
    void                    on_replace_token(content::path_info_t & ipath, QDomDocument & xml, filter::filter::token_info_t & token);
    void                    on_token_help(filter::filter::token_help_t & help);

    libdbproxy::table::pointer_t get_test_results_table();

private:
    void                    content_update(int64_t variables_timestamp);

    snap_child *                            f_snap = nullptr;
    libdbproxy::table::pointer_t f_test_results_table;
};


} // namespace test_plugin
} // namespace snap
// vim: ts=4 sw=4 et
