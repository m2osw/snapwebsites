// Snap Websites Server -- Sitemap XML
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
#include "../robotstxt/robotstxt.h"


namespace snap
{
namespace sitemapxml
{

enum class name_t
{
    SNAP_NAME_SITEMAPXML_COUNT,
    SNAP_NAME_SITEMAPXML_FILENAME_NUMBER_XML,
    SNAP_NAME_SITEMAPXML_INCLUDE,
    SNAP_NAME_SITEMAPXML_NAMESPACE,
    SNAP_NAME_SITEMAPXML_SITEMAP_NUMBER_XML,
    SNAP_NAME_SITEMAPXML_SITEMAP_XML
};
const char * get_name(name_t name) __attribute__ ((const));


class sitemapxml_exception : public snap_exception
{
public:
    explicit sitemapxml_exception(char const *        what_msg) : snap_exception("sitemap.xml", what_msg) {}
    explicit sitemapxml_exception(std::string const & what_msg) : snap_exception("sitemap.xml", what_msg) {}
    explicit sitemapxml_exception(QString const &     what_msg) : snap_exception("sitemap.xml", what_msg) {}
};

class sitemapxml_exception_missing_table : public sitemapxml_exception
{
public:
    explicit sitemapxml_exception_missing_table(char const *        what_msg) : sitemapxml_exception(what_msg) {}
    explicit sitemapxml_exception_missing_table(std::string const & what_msg) : sitemapxml_exception(what_msg) {}
    explicit sitemapxml_exception_missing_table(QString const &     what_msg) : sitemapxml_exception(what_msg) {}
};

class sitemapxml_exception_invalid_xslt_data : public sitemapxml_exception
{
public:
    explicit sitemapxml_exception_invalid_xslt_data(char const *        what_msg) : sitemapxml_exception(what_msg) {}
    explicit sitemapxml_exception_invalid_xslt_data(std::string const & what_msg) : sitemapxml_exception(what_msg) {}
    explicit sitemapxml_exception_invalid_xslt_data(QString const &     what_msg) : sitemapxml_exception(what_msg) {}
};

class sitemapxml_exception_missing_uri : public sitemapxml_exception
{
public:
    explicit sitemapxml_exception_missing_uri(char const *        what_msg) : sitemapxml_exception(what_msg) {}
    explicit sitemapxml_exception_missing_uri(std::string const & what_msg) : sitemapxml_exception(what_msg) {}
    explicit sitemapxml_exception_missing_uri(QString const &     what_msg) : sitemapxml_exception(what_msg) {}
};




class sitemapxml
        : public plugins::plugin
        , public path::path_execute
{
public:
    class url_image
    {
    public:
        typedef std::vector<url_image>      vector_t;

        void            set_uri(QString const & uri);
        void            set_title(QString const & title);
        void            set_caption(QString const & caption);
        void            set_geo_location(QString const & geo_location);
        void            set_license_uri(QString const & license_uri);

        QString const & get_uri() const;
        QString const & get_title() const;
        QString const & get_caption() const;
        QString const & get_geo_location() const;
        QString const & get_license_uri() const;

    private:
        QString         f_uri;                  // the image URI
        QString         f_title;                // the image title
        QString         f_caption;              // the image caption / description
        QString         f_geo_location;         // the location where the photo was taken (as a human name: Limerick, Ireland)
        QString         f_license_uri;          // a URI to the license assigned to this image
    };

    class url_info
    {
    public:
        static int const FREQUENCY_NONE = 0;
        static int const FREQUENCY_NEVER = -1;
        static int const FREQUENCY_MIN = 60; // 1 minute
        static int const FREQUENCY_MAX = 31536000; // 1 year

                                    url_info();

        void                        set_uri(QString const & uri);
        void                        set_priority(float priority);
        void                        set_last_modification(time_t last_modification);
        void                        set_frequency(int frequency);
        void                        add_image(url_image const & image);
        void                        reset_images();

        QString                     get_uri() const;
        float                       get_priority() const;
        time_t                      get_last_modification() const;
        int                         get_frequency() const;
        url_image::vector_t const & get_images() const;

        bool                        operator < (url_info const & rhs) const;

    private:
        QString                     f_uri;                      // the page URI
        float                       f_priority = 0.5f;          // 0.001 to 1.0, default 0.5
        time_t                      f_last_modification = 0;    // Unix date when it was last modified
        int                         f_frequency = 604800;       // number of seconds between modifications
        url_image::vector_t         f_images;                   // an array of images
    };
    typedef std::vector<url_info>        url_info_list_t;

                            sitemapxml();
                            ~sitemapxml();

    // plugins::plugin implementation
    static sitemapxml *     instance();
    virtual QString         settings_path() const;
    virtual QString         description() const;
    virtual QString         dependencies() const;
    virtual int64_t         do_update(int64_t last_updated);
    virtual void            bootstrap(snap_child * snap);

    // server signals
    void                    on_backend_process();

    // content signals
    void                    on_copy_branch_cells(QtCassandra::QCassandraCells & source_cells, QtCassandra::QCassandraRow::pointer_t destination_row, snap_version::version_number_t const destination_branch);

    // path::path_execute implementation
    virtual bool            on_path_execute(content::path_info_t & ipath);

    // robotstxt signals
    void                    on_generate_robotstxt(robotstxt::robotstxt * r);

    // shorturl signals
    void                    on_allow_shorturl(content::path_info_t & ipath, QString const & owner, QString const & type, bool & allow);

    SNAP_SIGNAL(generate_sitemapxml, (sitemapxml * sitemap), (sitemap));

    void                    add_url(url_info const & url);

private:
    void                    content_update(int64_t variables_timestamp);
    void                    generate_one_sitemap(int32_t const position, size_t & index);
    void                    generate_sitemap_index(int32_t position);

    snap_child *            f_snap = nullptr;
    url_info_list_t         f_url_info;
};

} // namespace sitemapxml
} // namespace snap
// vim: ts=4 sw=4 et
