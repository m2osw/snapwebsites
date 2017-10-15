// Snap Websites Server -- Snap Software Description handling
// Copyright (C) 2012-2017  Made to Order Software Corp.
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
#include "../robotstxt/robotstxt.h"


namespace snap
{
namespace snap_software_description
{

enum class name_t
{
    SNAP_NAME_SNAP_SOFTWARE_DESCRIPTION_CATEGORY,
    SNAP_NAME_SNAP_SOFTWARE_DESCRIPTION_ENABLE,
    SNAP_NAME_SNAP_SOFTWARE_DESCRIPTION_HTTP_HEADER,
    SNAP_NAME_SNAP_SOFTWARE_DESCRIPTION_LAST_UPDATE,
    SNAP_NAME_SNAP_SOFTWARE_DESCRIPTION_PUBLISHER_FIELD,
    SNAP_NAME_SNAP_SOFTWARE_DESCRIPTION_PUBLISHER_TYPE_PATH,
    SNAP_NAME_SNAP_SOFTWARE_DESCRIPTION_SETTINGS_MAX_FILES,
    SNAP_NAME_SNAP_SOFTWARE_DESCRIPTION_SETTINGS_PATH,
    SNAP_NAME_SNAP_SOFTWARE_DESCRIPTION_SETTINGS_TEASER_END_MARKER,
    SNAP_NAME_SNAP_SOFTWARE_DESCRIPTION_SETTINGS_TEASER_TAGS,
    SNAP_NAME_SNAP_SOFTWARE_DESCRIPTION_SETTINGS_TEASER_WORDS,
    SNAP_NAME_SNAP_SOFTWARE_DESCRIPTION_SUPPORT_FIELD,
    SNAP_NAME_SNAP_SOFTWARE_DESCRIPTION_SUPPORT_TYPE_PATH,
    SNAP_NAME_SNAP_SOFTWARE_DESCRIPTION_TABLE_OF_CONTENT
};
char const * get_name(name_t name) __attribute__ ((const));



class snap_software_description_exception : public snap_exception
{
public:
    explicit snap_software_description_exception(char const *        what_msg) : snap_exception("snap_software_description", what_msg) {}
    explicit snap_software_description_exception(std::string const & what_msg) : snap_exception("snap_software_description", what_msg) {}
    explicit snap_software_description_exception(QString const &     what_msg) : snap_exception("snap_software_description", what_msg) {}
};







class snap_software_description
        : public plugins::plugin
{
public:
                                            snap_software_description();
    virtual                                 ~snap_software_description();

    // plugins::plugin implementation
    static snap_software_description *      instance();
    virtual QString                         description() const;
    virtual QString                         dependencies() const;
    virtual int64_t                         do_update(int64_t last_updated);
    virtual void                            bootstrap(::snap::snap_child * snap);

    // server signal
    void                                    on_backend_process();

    // layout signals
    void                                    on_generate_header_content(content::path_info_t & path, QDomElement & header, QDomElement & metadata);
    void                                    on_generate_page_content(content::path_info_t & ipath, QDomElement & page, QDomElement & body);

    // robotstxt signal
    void                                    on_generate_robotstxt(robotstxt::robotstxt * r);

    // shorturl signal
    void                                    on_allow_shorturl(content::path_info_t & ipath, QString const & owner, QString const & type, bool & allow);

private:
    void                                    content_update(int64_t variables_timestamp);

    QString                                 get_root_path();
    void                                    save_pad_file_data();
    bool                                    create_publisher();
    bool                                    create_support();
    bool                                    create_catalog(content::path_info_t & catalog_ipath, int const depth);
    bool                                    create_file(content::path_info_t & file_ipath);
    bool                                    load_xsl_file(QString const & filename, QString & xsl);

    snap_child *                            f_snap = nullptr;
    QtCassandra::QCassandraRow::pointer_t   f_snap_software_description_settings_row;
    content::path_info_t::pointer_t         f_table_of_content_ipath;
    QString                                 f_snap_software_description_parser_catalog_xsl;
    QString                                 f_snap_software_description_parser_file_xsl;
    QString                                 f_snap_software_description_parser_publisher_xsl;
    QString                                 f_snap_software_description_parser_support_xsl;
    QString                                 f_padfile_xsl;
    QString                                 f_padmap_txt;
    QDomDocument                            f_padlist_xml;
};

} // namespace snap_software_description
} // namespace snap
// vim: ts=4 sw=4 et
