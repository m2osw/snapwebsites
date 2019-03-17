// Snap Websites Server -- AJAX response management
// Copyright (c) 2014-2019  Made to Order Software Corp.  All Rights Reserved
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
#include "../test_plugin_suite/test_plugin_suite.h"


namespace snap
{
namespace server_access
{

enum class name_t
{
    SNAP_NAME_SERVER_ACCESS_AJAX_FIELD
};
char const * get_name(name_t name) __attribute__ ((const));



class server_access_exception : public snap_exception
{
public:
    explicit server_access_exception(char const *        what_msg) : snap_exception("server-access", what_msg) {}
    explicit server_access_exception(std::string const & what_msg) : snap_exception("server-access", what_msg) {}
    explicit server_access_exception(QString const &     what_msg) : snap_exception("server-access", what_msg) {}
};

class server_access_exception_create_called_twice : public server_access_exception
{
public:
    explicit server_access_exception_create_called_twice(char const *        what_msg) : server_access_exception(what_msg) {}
    explicit server_access_exception_create_called_twice(std::string const & what_msg) : server_access_exception(what_msg) {}
    explicit server_access_exception_create_called_twice(QString const &     what_msg) : server_access_exception(what_msg) {}
};

class server_access_exception_success_with_errors : public server_access_exception
{
public:
    explicit server_access_exception_success_with_errors(char const *        what_msg) : server_access_exception(what_msg) {}
    explicit server_access_exception_success_with_errors(std::string const & what_msg) : server_access_exception(what_msg) {}
    explicit server_access_exception_success_with_errors(QString const &     what_msg) : server_access_exception(what_msg) {}
};

class server_access_exception_invalid_uri : public server_access_exception
{
public:
    explicit server_access_exception_invalid_uri(char const *        what_msg) : server_access_exception(what_msg) {}
    explicit server_access_exception_invalid_uri(std::string const & what_msg) : server_access_exception(what_msg) {}
    explicit server_access_exception_invalid_uri(QString const &     what_msg) : server_access_exception(what_msg) {}
};









class server_access
    : public plugins::plugin
{
public:
                                server_access();
                                server_access(server_access const & rhs) = delete;
    virtual                     ~server_access() override;

    server_access &             operator = (server_access const & rhs) = delete;

    static server_access *      instance();

    // plugins::plugin implementation
    virtual QString             icon() const override;
    virtual QString             description() const override;
    virtual QString             dependencies() const override;
    virtual int64_t             do_update(int64_t last_updated) override;
    virtual void                bootstrap(snap_child * snap) override;

    // server signals
    void                        on_output_result(QString const & uri_path, QByteArray & result);

    bool                        is_ajax_request() const;
    void                        create_ajax_result(content::path_info_t & ipath, bool const success);
    void                        ajax_output();

    void                        ajax_failure();
    void                        ajax_redirect(QString const & uri, QString const & target = "");
    void                        ajax_append_data(QString const & name, QByteArray const & data);

    SNAP_SIGNAL_WITH_MODE(process_ajax_result, (content::path_info_t & ipath, bool const succeeded), (ipath, succeeded), NEITHER);

    // links test suite
    SNAP_TEST_PLUGIN_SUITE_SIGNALS()

private:
    typedef QMap<QString, QByteArray>   data_map_t;

    void                        content_update(int64_t variables_timestamp);

    // tests
    SNAP_TEST_PLUGIN_TEST_DECL(test_ajax)

    snap_child *                f_snap = nullptr;
    QDomDocument                f_ajax = QDomDocument();
    bool                        f_ajax_initialized = false;
    bool                        f_ajax_output = false;
    bool                        f_success = false;
    QString                     f_ajax_redirect = QString();
    QString                     f_ajax_target = QString();
    data_map_t                  f_ajax_data = data_map_t();
};


} // namespace server_access
} // namespace snap
// vim: ts=4 sw=4 et
