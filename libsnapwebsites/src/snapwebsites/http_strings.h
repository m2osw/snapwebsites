// Snap Servers -- HTTP string handling (splitting, etc.)
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

#include <QMap>
#include <QString>
#include <QVector>

namespace snap
{
namespace http_strings
{



// for HTTP_ACCEPT_ENCODING, HTTP_ACCEPT_LANGUAGE, HTTP_ACCEPT
class WeightedHttpString
{
public:
    class part_t
    {
    public:
        typedef float               level_t;
        typedef QVector<part_t>     vector_t; // do NOT use a map, we want to keep them in order!

        // an authoritative document at the IANA clearly says that
        // the default level (quality value) is 1.0f.
        //
        static level_t constexpr    DEFAULT_LEVEL() { return 1.0f; }

        static level_t constexpr    UNDEFINED_LEVEL() { return -1.0f; }

                                    part_t();
                                    part_t(QString const & name);

        QString const &             get_name() const;
        level_t                     get_level() const;
        void                        set_level(level_t const level);
        QString                     get_parameter(QString const & name) const;
        void                        add_parameter(QString const & name, QString const & value);
        QString                     to_string() const;

        bool                        operator < (part_t const & rhs) const;

    private:
        typedef QMap<QString, QString>      parameters_t;

        QString                     f_name;
        level_t                     f_level = DEFAULT_LEVEL(); // i.e.  q=0.8
        // TODO add support for any other parameter
        parameters_t                f_param;
    };

                            WeightedHttpString(QString const & str = QString());

    bool                    parse(QString const & str, bool reset = false);
    QString const &         get_string() const { return f_str; }
    part_t::level_t         get_level(QString const & name);
    void                    sort_by_level();
    part_t::vector_t &      get_parts() { return f_parts; }
    part_t::vector_t const &get_parts() const { return f_parts; }
    QString                 to_string() const;
    QString const &         error_messages() const { return f_error_messages; }

private:
    QString                 f_str;
    part_t::vector_t        f_parts; // do NOT use a map, we want to keep them in order
    QString                 f_error_messages;
};



} // namespace http_strings
} // namespace snap
// vim: ts=4 sw=4 et
