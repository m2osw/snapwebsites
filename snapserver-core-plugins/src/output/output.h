// Snap Websites Server -- handle the basic display of the website content
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


DECLARE_MAIN_EXCEPTION(output_exception);

DECLARE_EXCEPTION(output_exception, output_exception_invalid_content_xml);



class output
    : public serverplugins::plugin
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

    SERVERPLUGINS_DEFAULTS(output);

    // serverplugins::plugin implementation
    virtual void        bootstrap() override;
    virtual time_t      do_update(time_t last_updated, unsigned int phase) override;

    // path::path_execute implementation
    virtual bool        on_path_execute(content::path_info_t & ipath) override;

    // layout::layout_content implementation
    virtual void        on_generate_main_content(content::path_info_t & ipath, QDomElement & page, QDomElement & body) override;

    // layout::layout_boxes implementation
    virtual void        on_generate_boxes_content(content::path_info_t & page_ipath, content::path_info_t & ipath, QDomElement & page, QDomElement & boxes) override;

    // server signals
    void                on_user_status(snap_child::user_status_t status, snap_child::user_identifier_t id);

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
    compression_extensions_map_t    f_compression_extensions = compression_extensions_map_t();
    snap_child::user_status_t       f_user_status = snap_child::user_status_t::USER_STATUS_UNKNOWN;
    snap_child::user_identifier_t   f_user_id = 0; // we do not have access to users::IDENTIFIER_ANONYMOUS
};


} // namespace output
} // namespace snap
// vim: ts=4 sw=4 et
