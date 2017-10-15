// Snap Websites Server -- hashtag implementation
// Copyright (C) 2013-2017  Made to Order Software Corp.
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
namespace hashtag
{


enum class name_t
{
    SNAP_NAME_HASHTAG_LINK,
    SNAP_NAME_HASHTAG_PATH,
    SNAP_NAME_HASHTAG_SETTINGS_PATH
};
char const *get_name(name_t name) __attribute__ ((const));


class hashtag_exception : public snap_exception
{
public:
    explicit hashtag_exception(char const *        what_msg) : snap_exception("Hashtag", what_msg) {}
    explicit hashtag_exception(std::string const & what_msg) : snap_exception("Hashtag", what_msg) {}
    explicit hashtag_exception(QString const &     what_msg) : snap_exception("Hashtag", what_msg) {}
};



class hashtag
        : public plugins::plugin
{
public:
                        hashtag();
                        ~hashtag();

    // plugins::plugin implementation
    static hashtag *    instance();
    virtual QString     settings_path() const;
    virtual QString     icon() const;
    virtual QString     description() const;
    virtual QString     dependencies() const;
    virtual int64_t     do_update(int64_t last_updated);
    virtual void        bootstrap(snap_child * snap);

    // filter signals
    void                on_filter_text(filter::filter::filter_text_t & txt_filt);

private:
    void                content_update(int64_t variables_timestamp);

    snap_child *        f_snap = nullptr;
};


} // namespace hashtag
} // namespace snap
// vim: ts=4 sw=4 et
