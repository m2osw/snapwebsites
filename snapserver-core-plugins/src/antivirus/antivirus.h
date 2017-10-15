// Snap Websites Server -- anti-virus verifies uploaded files cleanliness
// Copyright (C) 2014-2017  Made to Order Software Corp.
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
#include "../layout/layout.h"
#include "../versions/versions.h"


namespace snap
{
namespace antivirus
{


enum class name_t
{
    SNAP_NAME_ANTIVIRUS_ENABLE,
    SNAP_NAME_ANTIVIRUS_SETTINGS_PATH,
    SNAP_NAME_ANTIVIRUS_VERSION
};
char const * get_name(name_t name) __attribute__ ((const));


class antivirus_exception : public snap_exception
{
public:
    explicit antivirus_exception(char const *        what_msg) : snap_exception("Anti-Virus", what_msg) {}
    explicit antivirus_exception(std::string const & what_msg) : snap_exception("Anti-Virus", what_msg) {}
    explicit antivirus_exception(QString const &     what_msg) : snap_exception("Anti-Virus", what_msg) {}
};



class antivirus
        : public plugins::plugin
        , public layout::layout_content
{
public:
                            antivirus();
                            ~antivirus();

    // plugins::plugin implementation
    static antivirus *      instance();
    virtual QString         icon() const;
    virtual QString         description() const;
    virtual QString         dependencies() const;
    virtual int64_t         do_update(int64_t last_updated);
    virtual void            bootstrap(snap_child * snap);

    // layout::layout_content implementation
    virtual void            on_generate_main_content(content::path_info_t & path, QDomElement & page, QDomElement & body);

    // content signals
    void                    on_check_attachment_security(content::attachment_file const & file, content::permission_flag & secure, bool const fast);

    // versions signals
    void                    on_versions_tools(filter::filter::token_info_t & token);

private:
    void                    content_update(int64_t variables_timestamp);
    bool                    has_clamscan();

    snap_child *            f_snap = nullptr;
};


} // namespace antivirus
} // namespace snap
// vim: ts=4 sw=4 et
