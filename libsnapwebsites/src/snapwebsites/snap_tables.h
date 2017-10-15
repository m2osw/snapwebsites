// Network Address -- classes used to handle table schemas.
// Copyright (C) 2016-2017  Made to Order Software Corp.
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

#include "snapwebsites/snap_exception.h"

//#include <string>
#include <map>
#include <memory>

namespace snap
{


class snap_tables_exception : public snap::snap_logic_exception
{
public:
    snap_tables_exception(char const *        what_msg) : snap_logic_exception(what_msg) {}
    snap_tables_exception(std::string const & what_msg) : snap_logic_exception(what_msg) {}
    snap_tables_exception(QString const &     what_msg) : snap_logic_exception(what_msg) {}
};

class snap_table_invalid_xml_exception : public snap_tables_exception
{
public:
    snap_table_invalid_xml_exception(char const *        what_msg) : snap_tables_exception(what_msg) {}
    snap_table_invalid_xml_exception(std::string const & what_msg) : snap_tables_exception(what_msg) {}
    snap_table_invalid_xml_exception(QString const &     what_msg) : snap_tables_exception(what_msg) {}
};



class snap_tables
{
public:
    enum class model_t
    {
        MODEL_CONTENT,
        MODEL_DATA,
        MODEL_QUEUE,
        MODEL_LOG,
        MODEL_SESSION
    };

    typedef std::shared_ptr<snap_tables>    pointer_t;

    class table_schema_t
    {
    public:
        typedef std::map<QString, table_schema_t>   map_t;

        void                    set_name(QString const & name);
        QString const &         get_name() const;

        void                    set_description(QString const & description);
        QString const &         get_description() const;

        void                    set_model(model_t model);
        model_t                 get_model() const;

        void                    set_drop(bool drop = true);
        bool                    get_drop() const;

    private:
        QString                 f_name;
        QString                 f_description;
        model_t                 f_model = model_t::MODEL_CONTENT;
        bool                    f_drop = false;
    };

    bool                            load(QString const & path);
    bool                            load_xml(QString const & filename);
    bool                            has_table(QString const & name) const;
    table_schema_t::map_t const &   get_schemas() const;

    static model_t                  string_to_model(QString const & model);

private:
    table_schema_t::map_t           f_schemas;
};


}
// snap_addr namespace
// vim: ts=4 sw=4 et
