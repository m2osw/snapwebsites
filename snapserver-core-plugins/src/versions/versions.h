// Copyright (c) 2011-2022  Made to Order Software Corp.  All Rights Reserved
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
#include    "../filter/filter.h"


namespace snap
{
namespace versions
{


//enum class name_t
//{
//    SNAP_NAME_VERSIONS_VERSION
//};
//char const * get_name(name_t name) __attribute__ ((const));


DECLARE_MAIN_EXCEPTION(versions_exception);

DECLARE_EXCEPTION(versions_exception, versions_exception_invalid_content_xml);







class versions
    : public serverplugins::plugin
{
public:
    SERVERPLUGINS_DEFAULTS(versions);

    // serverplugins::plugin implementation
    virtual void        bootstrap() override;
    virtual time_t      do_update(time_t last_updated, unsigned int phase) override;

    // filter signals
    void                on_replace_token(content::path_info_t & ipath, QDomDocument & xml, filter::filter::token_info_t & token);
    void                on_token_help(filter::filter::token_help_t & help);

    PLUGIN_SIGNAL_WITH_MODE(versions_libraries, (filter::filter::token_info_t & token), (token), START_AND_DONE);
    PLUGIN_SIGNAL_WITH_MODE(versions_tools, (filter::filter::token_info_t & token), (token), START_AND_DONE);

private:
    void                content_update(int64_t variables_timestamp);

    snap_child *        f_snap = nullptr;
};


} // namespace versions
} // namespace snap
// vim: ts=4 sw=4 et
