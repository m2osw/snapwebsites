// Snap Websites Server -- layout management
// Copyright (C) 2011-2017  Made to Order Software Corp.
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
namespace layout
{

enum class name_t
{
    SNAP_NAME_LAYOUT_BODY_XSL,
    SNAP_NAME_LAYOUT_BOX,
    SNAP_NAME_LAYOUT_BOXES,
    SNAP_NAME_LAYOUT_CONTENT_XML,
    SNAP_NAME_LAYOUT_LAYOUT,
    SNAP_NAME_LAYOUT_LAYOUTS_PATH,
    SNAP_NAME_LAYOUT_NAMESPACE,
    SNAP_NAME_LAYOUT_REFERENCE,
    SNAP_NAME_LAYOUT_TABLE,
    SNAP_NAME_LAYOUT_THEME,
    SNAP_NAME_LAYOUT_THEME_XSL
};
char const * get_name(name_t name) __attribute__ ((const));


class layout_exception : public snap_exception
{
public:
    explicit layout_exception(char const *        what_msg) : snap_exception("layout", what_msg) {}
    explicit layout_exception(std::string const & what_msg) : snap_exception("layout", what_msg) {}
    explicit layout_exception(QString const &     what_msg) : snap_exception("layout", what_msg) {}
};

class layout_exception_invalid_xslt_data : public layout_exception
{
public:
    explicit layout_exception_invalid_xslt_data(char const *        what_msg) : layout_exception(what_msg) {}
    explicit layout_exception_invalid_xslt_data(std::string const & what_msg) : layout_exception(what_msg) {}
    explicit layout_exception_invalid_xslt_data(QString const &     what_msg) : layout_exception(what_msg) {}
};


class layout_content
{
public:
    virtual         ~layout_content() {} // ensure proper virtual tables
    virtual void    on_generate_main_content(content::path_info_t & ipath, QDomElement & page, QDomElement & body) = 0;
};


class layout_boxes
{
public:
    virtual         ~layout_boxes() {} // ensure proper virtual tables
    virtual void    on_generate_boxes_content(content::path_info_t & page_ipath, content::path_info_t & ipath, QDomElement & page, QDomElement & boxes) = 0;
};






class layout
        : public plugins::plugin
{
public:
                        layout();
                        ~layout();

    // plugins::plugin implementation
    static layout *     instance();
    virtual QString     icon() const override;
    virtual QString     description() const override;
    virtual QString     dependencies() const override;
    virtual int64_t     do_update(int64_t last_updated) override;
    virtual int64_t     do_dynamic_update(int64_t last_updated) override;
    virtual void        bootstrap(snap_child *snap) override;

    libdbproxy::table::pointer_t get_layout_table();

    // server signals
    void                on_load_file(snap_child::post_file_t & file, bool & found);
    bool                on_improve_signature(QString const & path, QDomDocument doc, QDomElement & signature_tag);

    // content signals
    void                on_copy_branch_cells(libdbproxy::cells & source_cells, libdbproxy::row::pointer_t destination_row, snap_version::version_number_t const destination_branch);

    QString             get_layout(content::path_info_t & ipath, const QString & column_name, bool use_qs_theme);
    QDomDocument        create_document(content::path_info_t & ipath, plugin * content_plugin);
    QString             apply_layout(content::path_info_t & ipath, layout_content * plugin);
    QString             define_layout(content::path_info_t & ipath, QString const & name, QString const & key, QString const & default_filename, QString const & theme_name, QString & layout_name);
    void                create_body(QDomDocument & doc, content::path_info_t & ipath, QString const & xsl, layout_content * content_plugin, bool const handle_boxes = false, QString const & layout_name = QString(), QString const & theme_name = QString());
    QString             create_body_string(QDomDocument & doc , content::path_info_t & ipath, layout_content * content_plugin);
    QString             apply_theme(QDomDocument doc, QString const & xsl, QString const & theme_name);
    void                replace_includes(QString & xsl);
    //void                add_layout_from_resources(QString const & name);
    void                extract_js_and_css(QDomDocument & doc, QDomDocument & doc_output);

    SNAP_SIGNAL(generate_header_content, (content::path_info_t & ipath, QDomElement & header, QDomElement & metadata), (ipath, header, metadata));
    SNAP_SIGNAL_WITH_MODE(add_layout_from_resources, (QString const & name), (name), START_AND_DONE);
    SNAP_SIGNAL_WITH_MODE(generate_page_content, (content::path_info_t & ipath, QDomElement & page, QDomElement & body), (ipath, page, body), NEITHER);
    SNAP_SIGNAL_WITH_MODE(filtered_content, (content::path_info_t & ipath, QDomDocument & doc, QString const & xsl), (ipath, doc, xsl), NEITHER);

private:
    void                content_update(int64_t variables_timestamp);
    void                do_layout_updates();
    void                install_layout(QString const & layout_name);
    void                finish_install_layout();

    void                generate_boxes(content::path_info_t & ipath, QString const & layout_name, QDomDocument doc);

    snap_child *                            f_snap = nullptr;
    libdbproxy::table::pointer_t f_content_table;
    std::vector<QString>                    f_initialized_layout;
};



} // namespace layout
} // namespace snap
// vim: ts=4 sw=4 et
