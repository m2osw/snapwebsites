// Snap Websites Server -- handle the JavaScript WYSIWYG editor
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
#include "../form/form.h"
#include "../users/users.h"
#include "../server_access/server_access.h"
#include "../javascript/javascript.h"


namespace snap
{
namespace editor
{

class editor_exception : public snap_exception
{
public:
    explicit editor_exception(char const *        what_msg) : snap_exception("editor", what_msg) {}
    explicit editor_exception(std::string const & what_msg) : snap_exception("editor", what_msg) {}
    explicit editor_exception(QString const &     what_msg) : snap_exception("editor", what_msg) {}
};

class editor_exception_invalid_argument : public editor_exception
{
public:
    explicit editor_exception_invalid_argument(char const *        what_msg) : editor_exception(what_msg) {}
    explicit editor_exception_invalid_argument(std::string const & what_msg) : editor_exception(what_msg) {}
    explicit editor_exception_invalid_argument(QString const &     what_msg) : editor_exception(what_msg) {}
};

class editor_exception_invalid_path : public editor_exception
{
public:
    explicit editor_exception_invalid_path(char const *        what_msg) : editor_exception(what_msg) {}
    explicit editor_exception_invalid_path(std::string const & what_msg) : editor_exception(what_msg) {}
    explicit editor_exception_invalid_path(QString const &     what_msg) : editor_exception(what_msg) {}
};

class editor_exception_invalid_editor_form_xml : public editor_exception
{
public:
    explicit editor_exception_invalid_editor_form_xml(char const *        what_msg) : editor_exception(what_msg) {}
    explicit editor_exception_invalid_editor_form_xml(std::string const & what_msg) : editor_exception(what_msg) {}
    explicit editor_exception_invalid_editor_form_xml(QString const &     what_msg) : editor_exception(what_msg) {}
};

class editor_exception_too_many_tags : public editor_exception
{
public:
    explicit editor_exception_too_many_tags(char const *        what_msg) : editor_exception(what_msg) {}
    explicit editor_exception_too_many_tags(std::string const & what_msg) : editor_exception(what_msg) {}
    explicit editor_exception_too_many_tags(QString const &     what_msg) : editor_exception(what_msg) {}
};

class editor_exception_invalid_xslt_data : public editor_exception
{
public:
    explicit editor_exception_invalid_xslt_data(char const *        what_msg) : editor_exception(what_msg) {}
    explicit editor_exception_invalid_xslt_data(std::string const & what_msg) : editor_exception(what_msg) {}
    explicit editor_exception_invalid_xslt_data(QString const &     what_msg) : editor_exception(what_msg) {}
};

class editor_exception_locked : public editor_exception
{
public:
    explicit editor_exception_locked(char const *        what_msg) : editor_exception(what_msg) {}
    explicit editor_exception_locked(std::string const & what_msg) : editor_exception(what_msg) {}
    explicit editor_exception_locked(QString const &     what_msg) : editor_exception(what_msg) {}
};




enum class name_t
{
    SNAP_NAME_EDITOR_AUTO_RESET,
    SNAP_NAME_EDITOR_DRAFTS_PATH,
    SNAP_NAME_EDITOR_LAYOUT,
    SNAP_NAME_EDITOR_PAGE,
    SNAP_NAME_EDITOR_PAGE_TYPE,
    SNAP_NAME_EDITOR_SESSION,
    SNAP_NAME_EDITOR_TIMEOUT,
    SNAP_NAME_EDITOR_TYPE_EXTENDED_FORMAT_PATH,
    SNAP_NAME_EDITOR_TYPE_FORMAT_PATH
};
char const * get_name(name_t name) __attribute__ ((const));



class save_info_t
{
public:
                                            save_info_t(content::path_info_t & p_ipath,
                                                        QDomDocument & p_editor_widgets,
                                                        libdbproxy::row::pointer_t p_revision_row,
                                                        libdbproxy::row::pointer_t p_secret_row,
                                                        libdbproxy::row::pointer_t p_draft_row);

    content::path_info_t &                  ipath();

    QDomDocument &                          editor_widgets();

    libdbproxy::row::pointer_t   revision_row() const;
    libdbproxy::row::pointer_t   secret_row() const;
    libdbproxy::row::pointer_t   draft_row() const;

    void                                    lock();

    void                                    mark_as_modified();
    bool                                    modified() const;
    void                                    mark_as_having_errors();
    bool                                    has_errors() const;

private:
    content::path_info_t &                  f_ipath;
    QDomDocument                            f_editor_widgets;
    libdbproxy::row::pointer_t   f_revision_row;
    libdbproxy::row::pointer_t   f_secret_row;
    libdbproxy::row::pointer_t   f_draft_row;
    bool                                    f_locked = false;
    bool                                    f_modified = false;
    bool                                    f_has_errors = false;
};



class editor
        : public plugins::plugin
        , public links::links_cloned
        , public path::path_execute
        , public layout::layout_content
        , public form::form_post
        , public layout::layout_boxes
        , public javascript::javascript_dynamic_plugin
{
public:
    static int const    EDITOR_SESSION_ID_EDIT = 1;

    enum class save_mode_t
    {
        EDITOR_SAVE_MODE_UNKNOWN = -1,
        EDITOR_SAVE_MODE_DRAFT,
        EDITOR_SAVE_MODE_PUBLISH,
        EDITOR_SAVE_MODE_SAVE,
        EDITOR_SAVE_MODE_NEW_BRANCH,
        EDITOR_SAVE_MODE_AUTO_DRAFT,
        EDITOR_SAVE_MODE_ATTACHMENT,
        EDITOR_SAVE_MODE_AUTO_ATTACHMENT
    };

    typedef QMap<QString, QString> params_map_t;

    struct editor_uri_token
    {
        editor_uri_token(content::path_info_t & ipath, QString const & page_name, params_map_t const & params)
            : f_ipath(ipath)
            , f_page_name(page_name)
            , f_params(params)
            //, f_token("") -- auto-init
            //, f_result("") -- auto-init
        {
        }

        content::path_info_t &  f_ipath;
        QString const &         f_page_name;
        params_map_t const &    f_params;
        QString                 f_token;
        QString                 f_result;
    };

    class value_to_string_info_t
    {
    public:
        enum class status_t
        {
            WORKING,
            DONE,
            ERROR
        };

                                        value_to_string_info_t(content::path_info_t & ipath, QDomElement widget, libdbproxy::value const & value);

        bool                            is_done() const;
        bool                            is_valid() const;
        void                            set_status(status_t const status);
        content::path_info_t &          get_ipath() const;
        QDomElement                     get_widget() const;
        libdbproxy::value const & get_value() const;
        QString const &                 get_widget_type() const;
        QString const &                 get_data_type() const;
        QString const &                 get_type_name() const;
        void                            set_type_name(QString const & new_type_name);
        QString &                       result();

    private:
        status_t                        f_status = status_t::WORKING;

        content::path_info_t &          f_ipath;
        QDomElement                     f_widget;
        libdbproxy::value const & f_value;

        mutable QString                 f_widget_type;
        mutable QString                 f_data_type;
        QString                         f_type_name = "unknown";

        QString                         f_result;
    };

    class string_to_value_info_t
    {
    public:
        enum class status_t
        {
            WORKING,
            DONE,
            ERROR
        };

                                        string_to_value_info_t(content::path_info_t & ipath, QDomElement widget, QString const & data);

        bool                            is_done() const;
        bool                            is_valid() const;
        void                            set_status(status_t const status);
        content::path_info_t &          get_ipath() const;
        QDomElement                     get_widget() const;
        QString const &                 get_data() const;
        QString const &                 get_widget_type() const;
        QString const &                 get_data_type() const;
        QString const &                 get_type_name() const;
        void                            set_type_name(QString const & new_type_name);
        libdbproxy::value &  result();

    private:
        status_t                        f_status = status_t::WORKING;

        content::path_info_t &          f_ipath;
        QDomElement                     f_widget;
        QString const &                 f_data;

        mutable QString                 f_widget_type;
        mutable QString                 f_data_type;
        QString                         f_type_name = "unknown";

        libdbproxy::value    f_result;
    };

                        editor();
                        ~editor();

    // plugins::plugin implementation
    static editor *     instance();
    virtual QString     settings_path() const;
    virtual QString     icon() const;
    virtual QString     description() const;
    virtual QString     dependencies() const;
    virtual int64_t     do_update(int64_t last_updated);
    virtual void        bootstrap(snap_child * snap);

    // server signals
    void                on_process_post(QString const & uri_path);

    // path::path_execute
    virtual bool        on_path_execute(content::path_info_t & ipath);

    // path signals
    void                on_can_handle_dynamic_path(content::path_info_t & ipath, path::dynamic_plugin_t & plugin_info);

    // layout::layout_content
    virtual void        on_generate_main_content(content::path_info_t & path, QDomElement & page, QDomElement & body);

    // layout::layout_boxes
    virtual void        on_generate_boxes_content(content::path_info_t & page_cpath, content::path_info_t & ipath, QDomElement & page, QDomElement & box);

    // layout signals
    void                on_generate_header_content(content::path_info_t & path, QDomElement & header, QDomElement & metadata);
    void                on_generate_page_content(content::path_info_t & ipath, QDomElement & page, QDomElement & body);
    void                on_add_layout_from_resources(QString const & name);

    // links::links_cloned
    virtual void        repair_link_of_cloned_page(QString const & clone, snap_version::version_number_t branch_number, links::link_info const & source, links::link_info const & destination, bool const cloning);

    // form::form_post
    virtual void        on_process_form_post(content::path_info_t & ipath, sessions::sessions::session_info const & info);

    // form signals
    void                on_validate_post_for_widget(content::path_info_t & ipath, sessions::sessions::session_info & info,
                                         QDomElement const & widget, QString const & widget_name,
                                         QString const & widget_type, bool is_secret);

    QString             format_uri(QString const & format, content::path_info_t & ipath, QString const & page_name, params_map_t const & params);
    static save_mode_t  string_to_save_mode(QString const & mode);
    static QString      clean_post_value(QString const & widget_type, QString value);
    void                parse_out_inline_img(content::path_info_t & ipath, QString & body, QDomElement widget);
    QDomDocument        get_editor_widgets(content::path_info_t & ipath, bool const saving = false);
    void                add_editor_widget_templates(QDomDocument doc);
    void                add_editor_widget_templates(QString const & doc);
    void                add_editor_widget_templates_from_file(QString const & filename);

    SNAP_SIGNAL(prepare_editor_form, (editor * e), (e));
    SNAP_SIGNAL(save_editor_fields, (save_info_t & info), (info));
    SNAP_SIGNAL(validate_editor_post_for_widget, (content::path_info_t & ipath, sessions::sessions::session_info & info, QDomElement const & widget, QString const & widget_name, QString const & widget_type, QString const & value, bool const is_secret), (ipath, info, widget, widget_name, widget_type, value, is_secret));
    SNAP_SIGNAL(replace_uri_token, (editor_uri_token & token_info), (token_info));
    SNAP_SIGNAL_WITH_MODE(dynamic_editor_widget, (content::path_info_t & cpath, QString const & name, QDomDocument & editor_widgets), (cpath, name, editor_widgets), NEITHER);
    SNAP_SIGNAL_WITH_MODE(init_editor_widget, (content::path_info_t & ipath, QString const & field_id, QString const & field_type, QDomElement & widget, libdbproxy::row::pointer_t row), (ipath, field_id, field_type, widget, row), NEITHER);
    SNAP_SIGNAL_WITH_MODE(new_attachment_saved, (content::attachment_file & the_attachment, QDomElement const & widget, QDomElement const & attachment_tag), (the_attachment, widget, attachment_tag), NEITHER);
    SNAP_SIGNAL_WITH_MODE(finish_editor_form_processing, (content::path_info_t & ipath, bool & succeeded), (ipath, succeeded), NEITHER);
    SNAP_SIGNAL(string_to_value, (string_to_value_info_t & value_info), (value_info));
    SNAP_SIGNAL(value_to_string, (value_to_string_info_t & value_info), (value_info));
    SNAP_SIGNAL(editor_widget_type_is_secret, (QDomElement widget, content::permission_flag & is_public), (widget, is_public));

    // dynamic javascript property support
    virtual int         js_property_count() const;
    virtual QVariant    js_property_get(QString const & name) const;
    virtual QString     js_property_name(int index) const;
    virtual QVariant    js_property_get(int index) const;

    bool                has_post_value( QString const & name ) const;
    QString             get_post_value( QString const & name ) const;

    bool 							has_value( QString const & name ) const;
    libdbproxy::value	get_value( QString const & name ) const;

private:
    typedef QMap<QString, QString>      value_map_t;
    typedef QMap<QString, libdbproxy::value>     cassandra_value_map_t;

    void                content_update(int64_t variables_timestamp);
    void                process_new_draft();
    void                editor_save(content::path_info_t & ipath, sessions::sessions::session_info & info);
    void                editor_save_attachment(content::path_info_t & ipath, sessions::sessions::session_info & info, server_access::server_access * server_access_plugin);
    void                editor_create_new_branch(content::path_info_t & ipath);
    bool                save_inline_image(content::path_info_t & ipath, QDomElement img, QString const & src, QString filename, QDomElement widget);
    QString             verify_html_validity(QString body);
    bool                widget_is_secret(QDomElement widget);
    void                retrieve_original_field(content::path_info_t & ipath);
    void                on_check_for_redirect(content::path_info_t & ipath);

    snap_child *            f_snap = nullptr;
    QDomDocument            f_editor_form;          // XSL from editor-form.xsl + other plugin extensions
    QString                 f_value_to_validate;    // for the JavaScript, the value of the field being checked right now (from either the POST, Draft, or Database)
    value_map_t             f_post_values;          // in part for JavaScript, also caches all the values sent by the user
    value_map_t             f_current_values;       // in part for JavaScript, also caches all the current values in the database
    value_map_t             f_draft_values;         // in part for JavaScript, also caches all the values last saved along an error or an auto-save
    value_map_t             f_default_values;       // validation fails if we do not have the default value
    cassandra_value_map_t   f_converted_values;     // to avoid converting the values twice
};

} // namespace editor
} // namespace snap
// vim: ts=4 sw=4 et
