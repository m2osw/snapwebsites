// Snap Websites Server -- handle the basic display of the website content
// Copyright (c) 2014-2019  Made to Order Software Corp.  All Rights Reserved
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
#include "../output/output.h"


namespace snap
{
namespace mimetype
{


//enum class name_t
//{
//    SNAP_NAME_MIMETYPE_ACCEPTED
//};
//char const * get_name(name_t name) __attribute__ ((const));


class mimetype_exception : public snap_exception
{
public:
    explicit mimetype_exception(char const *        what_msg) : snap_exception("mimetype", what_msg) {}
    explicit mimetype_exception(std::string const & what_msg) : snap_exception("mimetype", what_msg) {}
    explicit mimetype_exception(QString const &     what_msg) : snap_exception("mimetype", what_msg) {}
};

class mimetype_exception_invalid_data : public mimetype_exception
{
public:
    explicit mimetype_exception_invalid_data(char const *        what_msg) : mimetype_exception(what_msg) {}
    explicit mimetype_exception_invalid_data(std::string const & what_msg) : mimetype_exception(what_msg) {}
    explicit mimetype_exception_invalid_data(QString const &     what_msg) : mimetype_exception(what_msg) {}
};







class mimetype
    : public cppthread::plugin
{
public:
                        mimetype();
                        mimetype(mimetype const & rhs) = delete;
    virtual             ~mimetype() override;

    mimetype &          operator = (mimetype const & rhs) = delete;

    static mimetype *   instance();

    // plugins::plugin implementation
    virtual int64_t     do_update(int64_t last_updated) override;
    virtual void        bootstrap(snap_child * snap) override;

    QString             mimetype_to_icon(QString const & mime_type);
    QString             mimetype_to_extension(QString const & mime_type);

private:
    void                content_update(int64_t variables_timestamp);

    snap_child *        f_snap = nullptr;
};


} // namespace mimetype
} // namespace snap
// vim: ts=4 sw=4 et
