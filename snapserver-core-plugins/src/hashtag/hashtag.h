// Snap Websites Server -- hashtag implementation
// Copyright (c) 2013-2019  Made to Order Software Corp.  All Rights Reserved
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


//enum class name_t
//{
//    SNAP_NAME_HASHTAG_LINK,
//    SNAP_NAME_HASHTAG_PATH,
//    SNAP_NAME_HASHTAG_SETTINGS_PATH
//};
//char const *get_name(name_t name) __attribute__ ((const));


DECLARE_MAIN_EXCEPTION(hashtag_exception);



SERVERPLUGINS_VERSION(hashtag, 1, 0)


class hashtag
    : public serverplugins::plugin
{
public:
    SERVERPLUGINS_DEFAULTS(hashtag);

    // serverplugins::plugin implementation
    virtual void        bootstrap() override;
    virtual time_t      do_update(time_t last_updated, unsigned int phase) override;

    // filter signals
    void                on_filter_text(filter::filter::filter_text_t & txt_filt);

private:
    void                content_update(int64_t variables_timestamp);

    snap_child *        f_snap = nullptr;
};


} // namespace hashtag
} // namespace snap
// vim: ts=4 sw=4 et
