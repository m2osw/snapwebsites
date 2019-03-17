// Network Address -- classes used to handle table schemas.
// Copyright (c) 2016-2019  Made to Order Software Corp.  All Rights Reserved
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
#include "snapwebsites/dbutils.h"

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

class snap_table_unknown_table_exception : public snap_tables_exception
{
public:
    snap_table_unknown_table_exception(char const *        what_msg) : snap_tables_exception(what_msg) {}
    snap_table_unknown_table_exception(std::string const & what_msg) : snap_tables_exception(what_msg) {}
    snap_table_unknown_table_exception(QString const &     what_msg) : snap_tables_exception(what_msg) {}
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

    enum class kind_t
    {
        KIND_THRIFT,
        KIND_BLOB
    };

    typedef std::shared_ptr<snap_tables>    pointer_t;

    class column_t
    {
    public:
        typedef std::map<QString, column_t>    map_t;

        void                    set_name(QString const & name);
        QString const &         get_name() const;

        void                    set_type(dbutils::column_type_t type);
        dbutils::column_type_t  get_type() const;

        void                    set_required(bool required = true);
        bool                    get_required() const;

        void                    set_description(QString const & description);
        QString                 get_description() const;

        bool                    has_default_value() const;
        void                    set_default(QString const & default_value);
        QString                 get_default() const;

        void                    set_min_value(double const min);
        double                  get_min_value() const;

        void                    set_max_value(double const max);
        double                  get_max_value() const;

        void                    set_min_length(int64_t const min);
        int64_t                 get_min_length() const;

        void                    set_max_length(int64_t const max);
        int64_t                 get_max_length() const;

        void                    set_validation(QString const & validation);
        QString                 get_validation() const;

        void                    set_limited(bool limited = true);
        bool                    get_limited() const;

    private:
        QString                 f_name = QString();
        dbutils::column_type_t  f_type = dbutils::column_type_t::CT_string_value;
        QString                 f_description = QString("");
        QString                 f_default = QString();
        QString                 f_validation = QString();
        double                  f_min_value = std::numeric_limits<double>::min();
        double                  f_max_value = std::numeric_limits<double>::max();
        int64_t                 f_min_length = std::numeric_limits<int64_t>::min();
        int64_t                 f_max_length = std::numeric_limits<int64_t>::max();
        bool                    f_required = false;
        bool                    f_has_default = false;
        bool                    f_limited = false;
    };

    class secondary_index_t
    {
    public:
        typedef std::map<QString, secondary_index_t>    map_t;

        void                    set_name(QString const & name);
        QString const &         get_name() const;

        void                    set_column(QString const & column);
        QString const &         get_column() const;

    private:
        QString                 f_name = QString();
        QString                 f_column = QString();
    };

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

        void                    set_kind(kind_t kind);
        kind_t                  get_kind() const;

        void                    set_drop(bool drop = true);
        bool                    get_drop() const;

        void                    set_column(column_t const & column);
        column_t::map_t const & get_columns() const;

        void                    set_secondary_index(secondary_index_t const & index);
        secondary_index_t::map_t const &
                                get_secondary_indexes() const;

    private:
        QString                 f_name = QString();
        QString                 f_description = QString();
        model_t                 f_model = model_t::MODEL_CONTENT;
        kind_t                  f_kind = kind_t::KIND_THRIFT;
        column_t::map_t         f_columns = column_t::map_t();
        secondary_index_t::map_t
                                f_secondary_indexes = secondary_index_t::map_t();
        bool                    f_drop = false;
    };

    bool                            load(QString const & path);
    bool                            load_xml(QString const & filename);
    bool                            has_table(QString const & name) const;
    table_schema_t *                get_table(QString const & name) const;
    table_schema_t::map_t const &   get_schemas() const;

    static model_t                  string_to_model(QString const & model);
    static QString                  model_to_string(model_t model);

    static kind_t                   string_to_kind(QString const & kind);
    static QString                  kind_to_string(kind_t kind);

private:
    table_schema_t::map_t           f_schemas = table_schema_t::map_t();
};


}
// snap namespace
// vim: ts=4 sw=4 et
