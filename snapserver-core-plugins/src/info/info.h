// Snap Websites Server -- website system info settings
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
#include "../editor/editor.h"


namespace snap
{
namespace info
{


enum class name_t
{
    SNAP_NAME_INFO_PLUGIN_SELECTION
};
char const * get_name(name_t name) __attribute__ ((const));


class info_exception : public snap_exception
{
public:
    explicit info_exception(char const *        what_msg) : snap_exception("Info", what_msg) {}
    explicit info_exception(std::string const & what_msg) : snap_exception("Info", what_msg) {}
    explicit info_exception(QString const &     what_msg) : snap_exception("Info", what_msg) {}
};

class info_exception_invalid_path : public info_exception
{
public:
    explicit info_exception_invalid_path(char const *        what_msg) : info_exception(what_msg) {}
    explicit info_exception_invalid_path(std::string const & what_msg) : info_exception(what_msg) {}
    explicit info_exception_invalid_path(QString const &     what_msg) : info_exception(what_msg) {}
};



class info
        : public plugins::plugin
        , public path::path_execute
        , public layout::layout_content
{
public:
                            info();
                            ~info();

    // plugin implementation
    static info *           instance();
    virtual QString         settings_path() const;
    virtual QString         icon() const;
    virtual QString         description() const;
    virtual QString         dependencies() const;
    virtual int64_t         do_update(int64_t last_updated);
    virtual void            bootstrap(snap_child * snap);

    // path_execute implementation
    virtual bool            on_path_execute(content::path_info_t & ipath);

    // path signals
    void                    on_can_handle_dynamic_path(content::path_info_t & ipath, path::dynamic_plugin_t & plugin_info);

    // layout_content implementation
    virtual void            on_generate_main_content(content::path_info_t & ipath, QDomElement & page, QDomElement & body);

    // layout signals
    void                    on_generate_page_content(content::path_info_t & ipath, QDomElement & page, QDomElement & body);

    // server signals
    void                    on_improve_signature(QString const & path, QDomDocument doc, QDomElement signature);

    // editor signals
    void                    on_finish_editor_form_processing(content::path_info_t & ipath, bool & succeeded);
    void                    on_init_editor_widget(content::path_info_t  & ipath, QString const  & field_id, QString const  & field_type, QDomElement  & widget, QtCassandra::QCassandraRow::pointer_t row);

    // links test suite
    SNAP_TEST_PLUGIN_SUITE_SIGNALS()

private:
    void                    content_update(int64_t variables_timestamp);

    void                    init_plugin_selection_editor_widgets(content::path_info_t & ipath, QString const & field_id, QDomElement & widget);
    bool                    plugin_selection_on_path_execute(content::path_info_t & ipath);
    bool                    install_plugin(snap_string_list & plugin_list, QString const & plugin_name);
    bool                    uninstall_plugin(snap_string_list & plugin_list, QString const & plugin_name);

    void                    init_unsubscribe_editor_widgets(content::path_info_t & ipath, QString const & field_id, QDomElement & widget);
    void                    unsubscribe_on_finish_editor_form_processing(content::path_info_t & ipath);
    bool                    unsubscribe_on_path_execute(content::path_info_t & ipath);

    // tests
    SNAP_TEST_PLUGIN_TEST_DECL(verify_core_dependencies)
    SNAP_TEST_PLUGIN_TEST_DECL(verify_all_dependencies)

    snap_child *            f_snap = nullptr;
};


} // namespace info
} // namespace snap
// vim: ts=4 sw=4 et
