// Copyright (c) 2014-2022  Made to Order Software Corp.  All Rights Reserved
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


namespace snap
{
namespace attachment
{


enum class name_t
{
    SNAP_NAME_ATTACHMENT_ACTION_EXTRACTFILE
};
char const * get_name(name_t name) __attribute__ ((const));


DECLARE_MAIN_EXCEPTION(attachment_exception);

DECLARE_EXCEPTION(attachment_exception, attachment_exception_invalid_content_xml);
DECLARE_EXCEPTION(attachment_exception, attachment_exception_invalid_filename);






class attachment
    : public serverplugins::plugin
    , public path::path_execute
    , public server::backend_action
    , public permission_error_callback::error_by_mime_type
{
public:
    SERVERPLUGINS_DEFAULTS(attachment);

    // serverplugins::plugin implementation
    virtual void        bootstrap() override;
    virtual time_t      do_update(time_t last_updated, unsigned int phase) override;

    // server signals
    void                on_register_backend_action(server::backend_action_set & actions);

    // server::backend_action implementation
    virtual void        on_backend_action(QString const & action) override;

    // path signals
    void                on_can_handle_dynamic_path(content::path_info_t & ipath, path::dynamic_plugin_t & plugin_info);

    // path::path_execute
    virtual bool        on_path_execute(content::path_info_t & ipath) override;

    // content signal
    void                on_page_cloned(content::content::cloned_tree_t const & tree);
    void                on_copy_branch_cells(libdbproxy::cells & source_cells, libdbproxy::row::pointer_t destination_row, snap_version::version_number_t const destination_branch);

    // permissions signal
    void                on_permit_redirect_to_login_on_not_allowed(content::path_info_t & ipath, bool & redirect_to_login);

    // permission_error_callback::error_by_mime_type
    virtual void        on_handle_error_by_mime_type(snap_child::http_code_t err_code, QString const & err_name, QString const & err_description, QString const & path) override;

    int                 delete_all_attachments(content::path_info_t & ipath);

private:
    void                content_update(int64_t variables_timestamp);
    void                backend_action_extract_file();

    bool                check_for_uncompressed_file(content::path_info_t & ipath, path::dynamic_plugin_t & plugin_info);
    bool                check_for_minified_js_or_css(content::path_info_t & ipath, path::dynamic_plugin_t & plugin_info, QString const & extension);

    snap_child *        f_snap = nullptr;
};


} // namespace attachment
} // namespace snap
// vim: ts=4 sw=4 et
