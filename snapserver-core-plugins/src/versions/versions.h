// Snap Websites Server -- generate versions of all the parts used by snap
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


namespace snap
{
namespace versions
{


enum class name_t
{
    SNAP_NAME_VERSIONS_VERSION
};
char const * get_name(name_t name) __attribute__ ((const));


class versions_exception : public snap_exception
{
public:
    explicit versions_exception(char const *        what_msg) : snap_exception("versions", what_msg) {}
    explicit versions_exception(std::string const & what_msg) : snap_exception("versions", what_msg) {}
    explicit versions_exception(QString const &     what_msg) : snap_exception("versions", what_msg) {}
};

class versions_exception_invalid_content_xml : public versions_exception
{
public:
    explicit versions_exception_invalid_content_xml(char const *        what_msg) : versions_exception(what_msg) {}
    explicit versions_exception_invalid_content_xml(std::string const & what_msg) : versions_exception(what_msg) {}
    explicit versions_exception_invalid_content_xml(QString const &     what_msg) : versions_exception(what_msg) {}
};







class versions : public plugins::plugin
{
public:
                        versions();
                        ~versions();

    // plugins::plugin implementation
    static versions *   instance();
    virtual QString     settings_path() const;
    virtual QString     icon() const;
    virtual QString     description() const;
    virtual QString     dependencies() const;
    virtual int64_t     do_update(int64_t last_updated);
    virtual void        bootstrap(snap_child * snap);

    // filter signals
    void                on_replace_token(content::path_info_t & ipath, QDomDocument & xml, filter::filter::token_info_t & token);
    void                on_token_help(filter::filter::token_help_t & help);

    SNAP_SIGNAL_WITH_MODE(versions_libraries, (filter::filter::token_info_t & token), (token), START_AND_DONE);
    SNAP_SIGNAL_WITH_MODE(versions_tools, (filter::filter::token_info_t & token), (token), START_AND_DONE);

private:
    void                content_update(int64_t variables_timestamp);

    snap_child *        f_snap = nullptr;
};


} // namespace versions
} // namespace snap
// vim: ts=4 sw=4 et
