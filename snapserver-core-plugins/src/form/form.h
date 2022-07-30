// Snap Websites Server -- form handling
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
#include "../sessions/sessions.h"
#include "../filter/filter.h"

// Qt
//
//#include <libdbproxy/table.h>
#include <QDomDocument>


namespace snap
{
namespace form
{

enum class name_t
{
    SNAP_NAME_FORM_FORM,
    SNAP_NAME_FORM_NAMESPACE,
    SNAP_NAME_FORM_PATH,
    SNAP_NAME_FORM_RESOURCE,
    SNAP_NAME_FORM_SETTINGS,
    SNAP_NAME_FORM_SOURCE
};
char const *get_name(name_t const name) __attribute__ ((const));


DECLARE_MAIN_EXCEPTION(form_exception);
DECLARE_EXCEPTION(form_exception, form_exception_invalid_form_xml);
DECLARE_EXCEPTION(form_exception, form_exception_invalid_xslt_data);




class form_post
{
public:
    virtual                 ~form_post() {}

    virtual void            on_process_form_post(content::path_info_t & cpath, sessions::sessions::session_info const & info) = 0;
};


class form
    : public serverplugins::plugin
{
public:
    SERVERPLUGINS_DEFAULTS(form);

    // serverplugins::plugin implementation
    virtual void                bootstrap() override;
    virtual time_t              do_update(time_t last_updated, unsigned int phase) override;

    QSharedPointer<libdbproxy::table> get_form_table();

    void                        on_process_post(QString const & uri_path);
    void                        on_replace_token(content::path_info_t & ipath, QDomDocument & xml, filter::filter::token_info_t & token);
    void                        on_filtered_content(content::path_info_t & path, QDomDocument & doc, QString const & xsl);
    void                        on_copy_branch_cells(libdbproxy::cells & source_cells, libdbproxy::row::pointer_t destination_row, snap_version::version_number_t const destination_branch);

    SNAP_SIGNAL_WITH_MODE(tweak_form, (form *f, content::path_info_t & ipath, QDomDocument form_doc), (f, ipath, form_doc), NEITHER);
    SNAP_SIGNAL_WITH_MODE(form_element, (form *f), (f), NEITHER);
    SNAP_SIGNAL_WITH_MODE(fill_form_widget, (form *f, QString const & owner, QString const & cpath, QDomDocument xml_form, QDomElement widget, QString const & id), (f, owner, cpath, xml_form, widget, id), NEITHER);
    SNAP_SIGNAL(validate_post_for_widget, (content::path_info_t & ipath, sessions::sessions::session_info & info, QDomElement const & widget, QString const & widget_name, QString const & widget_type, bool const is_secret), (ipath, info, widget, widget_name, widget_type, is_secret));

    QDomDocument const          load_form(content::path_info_t & cpath, QString const & source, QString & error);
    QDomDocument                form_to_html(sessions::sessions::session_info & info, QDomDocument & xml);
    void                        add_form_elements(QDomDocument & add);
    void                        add_form_elements(QString & filename);
    void                        fill_value(QDomElement widget, QString const & value);

    QString                     get_source(QString const & plugin_owner_name, content::path_info_t & cpath);
    bool                        is_auto_save(QString const & cpath);

    static QString              text_64max(QString const & text, bool const is_secret);
    static QString              html_64max(QString const & html, bool const is_secret);
    static int                  count_text_lines(QString const & text);
    static int                  count_html_lines(QString const & html);
    static bool                 parse_width_height(QString const & size, int & width, int & height);
    static int                  current_tab_id();
    static void                 used_tab_id(int used);

private:
    typedef QMap<QString, QString> auto_save_types_t;

    void                        content_update(int64_t variables_timestamp);
    void                        auto_save_form(QString const & owner, content::path_info_t & ipath, auto_save_types_t const & auto_save_type, QDomDocument xml_form);
    void                        auto_fill_form(QDomDocument xml_form);

    snap_child *                f_snap = nullptr;
    bool                        f_form_initialized = false;
    QDomDocument                f_form_elements = QDomDocument();
    QDomElement                 f_form_stylesheet = QDomElement();
    QString                     f_form_elements_string = QString();
    QString                     f_form_title = QString();
};

} // namespace form
} // namespace snap
// vim: ts=4 sw=4 et
