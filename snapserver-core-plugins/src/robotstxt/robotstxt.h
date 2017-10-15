// Snap Websites Server -- robots.txt
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
#include "../layout/layout.h"
#include "../path/path.h"


namespace snap
{
namespace robotstxt
{

enum class name_t
{
    SNAP_NAME_ROBOTSTXT_FORBIDDEN_PATH,
    SNAP_NAME_ROBOTSTXT_FORBIDDEN,
    SNAP_NAME_ROBOTSTXT_NOARCHIVE,
    SNAP_NAME_ROBOTSTXT_NOFOLLOW,
    SNAP_NAME_ROBOTSTXT_NOIMAGEINDEX,
    SNAP_NAME_ROBOTSTXT_NOINDEX,
    SNAP_NAME_ROBOTSTXT_NOSNIPPET
};
char const * get_name(name_t name) __attribute__ ((const));



class robotstxt_exception : public snap_exception
{
public:
    explicit robotstxt_exception(const char *        what_msg) : snap_exception("robots.txt", what_msg) {}
    explicit robotstxt_exception(const std::string & what_msg) : snap_exception("robots.txt", what_msg) {}
    explicit robotstxt_exception(const QString &     what_msg) : snap_exception("robots.txt", what_msg) {}
};

class robotstxt_exception_invalid_field_name : public robotstxt_exception
{
public:
    explicit robotstxt_exception_invalid_field_name(const char *        what_msg) : robotstxt_exception(what_msg) {}
    explicit robotstxt_exception_invalid_field_name(const std::string & what_msg) : robotstxt_exception(what_msg) {}
    explicit robotstxt_exception_invalid_field_name(const QString &     what_msg) : robotstxt_exception(what_msg) {}
};

class robotstxt_exception_already_defined : public robotstxt_exception
{
public:
    explicit robotstxt_exception_already_defined(const char *        what_msg) : robotstxt_exception(what_msg) {}
    explicit robotstxt_exception_already_defined(const std::string & what_msg) : robotstxt_exception(what_msg) {}
    explicit robotstxt_exception_already_defined(const QString &     what_msg) : robotstxt_exception(what_msg) {}
};



class robotstxt
        : public plugins::plugin
        , public path::path_execute
{
public:
    static char const * ROBOT_NAME_ALL;
    static char const * ROBOT_NAME_GLOBAL;
    static char const * FIELD_NAME_DISALLOW;

                        robotstxt();
                        ~robotstxt();

    // plugins::plugin
    static robotstxt *  instance();
    //virtual QString     settings_path() const;
    virtual QString     icon() const;
    virtual QString     description() const;
    virtual QString     dependencies() const;
    virtual int64_t     do_update(int64_t last_updated);
    virtual void        bootstrap(snap_child * snap);

    // path::path_execute implementation
    virtual bool        on_path_execute(content::path_info_t & url);

    // layout signals
    void                on_generate_header_content(content::path_info_t & path, QDomElement & header, QDomElement & metadata);
    void                on_generate_page_content(content::path_info_t & path, QDomElement & page, QDomElement & body);

    SNAP_SIGNAL(generate_robotstxt, (robotstxt * r), (r));

    void        add_robots_txt_field(QString const & value,
                                     QString const & field = FIELD_NAME_DISALLOW,
                                     QString const & robot = ROBOT_NAME_ALL,
                                     bool unique = false);

    void        output() const;

private:
    void        content_update(int64_t variables_timestamp);
    void        define_robots(content::path_info_t & path);

    struct robots_field_t
    {
        QString     f_field;
        QString     f_value;
    };
    typedef std::vector<robots_field_t>                     robots_field_array_t;
    typedef std::map<const QString, robots_field_array_t>   robots_txt_t;

    snap_child *        f_snap = nullptr;
    robots_txt_t        f_robots_txt;

    QString             f_robots_path; // path that the cache represents
    QString             f_robots_cache;
};

} // namespace robotstxt
} // namespace snap
// vim: ts=4 sw=4 et
