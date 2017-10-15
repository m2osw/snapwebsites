// Snap Websites Server -- handle the basic display of the website content
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
#include "../filter/filter.h"
#include "../layout/layout.h"
#include "../path/path.h"
//#include "../javascript/javascript.h"


namespace snap
{
namespace output
{


//enum class name_t
//{
//    SNAP_NAME_OUTPUT_ACCEPTED
//};
//char const * get_name(name_t name) __attribute__ ((const));


class output_exception : public snap_exception
{
public:
    explicit output_exception(char const *        what_msg) : snap_exception("output", what_msg) {}
    explicit output_exception(std::string const & what_msg) : snap_exception("output", what_msg) {}
    explicit output_exception(QString const &     what_msg) : snap_exception("output", what_msg) {}
};

class output_exception_invalid_content_xml : public output_exception
{
public:
    explicit output_exception_invalid_content_xml(char const *        what_msg) : output_exception(what_msg) {}
    explicit output_exception_invalid_content_xml(std::string const & what_msg) : output_exception(what_msg) {}
    explicit output_exception_invalid_content_xml(QString const &     what_msg) : output_exception(what_msg) {}
};







class output
        : public plugins::plugin
        , public path::path_execute
        , public layout::layout_content
        , public layout::layout_boxes
        //, public javascript::javascript_dynamic_plugin
{
public:
    enum class phone_number_type_t
    {
        PHONE_NUMBER_TYPE_FAX,
        PHONE_NUMBER_TYPE_SKYPE,
        PHONE_NUMBER_TYPE_TELEPHONE
    };

                        output();
                        ~output();

    // plugins::plugin implementation
    static output *     instance();
    virtual QString     settings_path() const;
    virtual QString     icon() const;
    virtual QString     description() const;
    virtual QString     dependencies() const;
    virtual int64_t     do_update(int64_t last_updated);
    virtual void        bootstrap(snap_child * snap);

    // path::path_execute implementation
    virtual bool        on_path_execute(content::path_info_t & ipath);

    // layout::layout_content implementation
    virtual void        on_generate_main_content(content::path_info_t & ipath, QDomElement & page, QDomElement & body);

    // layout::layout_boxes implementation
    virtual void        on_generate_boxes_content(content::path_info_t & page_ipath, content::path_info_t & ipath, QDomElement & page, QDomElement & boxes);

    // layout signals
    void                on_generate_page_content(content::path_info_t & ipath, QDomElement & page, QDomElement & body);

    // filter signals
    void                on_replace_token(content::path_info_t & ipath, QDomDocument & xml, filter::filter::token_info_t & token);
    void                on_token_help(filter::filter::token_help_t & help);

    // javascript::javascript_dynamic_plugin implementation
    //virtual int         js_property_count() const;
    //virtual QVariant    js_property_get(QString const & name) const;
    //virtual QString     js_property_name(int index) const;
    //virtual QVariant    js_property_get(int index) const;

    static QString      phone_to_uri(QString const phone, phone_number_type_t const type);
    void                breadcrumb(content::path_info_t & ipath, QDomElement parent);

private:
    void                content_update(int64_t variables_timestamp);

    typedef std::map<QString, bool> compression_extensions_map_t;

    snap_child *                    f_snap = nullptr;
    compression_extensions_map_t    f_compression_extensions;
};


} // namespace output
} // namespace snap
// vim: ts=4 sw=4 et
