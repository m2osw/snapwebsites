// Snap Websites Server -- handle various locale information such as timezone and date output, number formatting for display, etc.
// Copyright (c) 2011-2019  Made to Order Software Corp.  All Rights Reserved
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
#include <snapwebsites/snapwebsites.h>


/** \file
 * \brief Header of the locale plugin.
 *
 * This header file is named "snap_locale.h" and not "locale.h" because
 * there is a system file "locale.h" and having the same name prevents
 * the system file from being included properly.
 *
 * The file defines the various locale plugin classes.
 */

namespace snap
{
namespace locale
{


enum class name_t
{
    SNAP_NAME_LOCALE_SETTINGS_LOCALE,
    SNAP_NAME_LOCALE_SETTINGS_TIMEZONE,
    SNAP_NAME_LOCALE_SETTINGS_PATH
};
char const * get_name(name_t name) __attribute__ ((const));


class locale_exception : public snap_exception
{
public:
    explicit locale_exception(char const *        what_msg) : snap_exception("locale", what_msg) {}
    explicit locale_exception(std::string const & what_msg) : snap_exception("locale", what_msg) {}
    explicit locale_exception(QString const &     what_msg) : snap_exception("locale", what_msg) {}
};

class locale_exception_invalid_argument : public locale_exception
{
public:
    explicit locale_exception_invalid_argument(char const *        what_msg) : locale_exception(what_msg) {}
    explicit locale_exception_invalid_argument(std::string const & what_msg) : locale_exception(what_msg) {}
    explicit locale_exception_invalid_argument(QString const &     what_msg) : locale_exception(what_msg) {}
};







class locale
    : public plugins::plugin
{
public:
    enum class parse_error_t
    {
        PARSE_NO_ERROR = 0,
        PARSE_ERROR_OVERFLOW,
        PARSE_ERROR_DATE,
        PARSE_ERROR_UNDERFLOW
    };

    struct locale_info_t
    {
        struct locale_parameters_t
        {
            // the following is in the order it is defined in the
            // full name although all parts except the language are
            // optional; the script is rare, the variant is used
            // quite a bit
            QString                 f_language = QString();
            QString                 f_variant = QString();
            QString                 f_country = QString();
            QString                 f_script = QString();
        };

        QString                 f_locale = QString();                       // name to use to setup this locale
        locale_parameters_t     f_abbreviations = locale_parameters_t();
        locale_parameters_t     f_display_names = locale_parameters_t();    // all names in "current" locale
    };
    typedef QVector<locale_info_t>      locale_list_t;

    // the ICU library only gives us the timezone full name,
    // continent and city all the other parameters will be empty
    struct timezone_info_t
    {
        QString         f_2country = QString();         // 2 letter country code
        int64_t         f_longitude = 0;                // city longitude
        int64_t         f_latitude = 0;                 // city latitude
        QString         f_timezone_name = QString();    // the full name of the timezone as is
        QString         f_continent = QString();        // one of the 5 continents and a few other locations
        QString         f_country_or_state = QString(); // likely empty (Used for Argentina, Kentucky, Indiana...)
        QString         f_city = QString();             // The main city for that timezone
        QString         f_comment = QString();          // likely empty, a comment about this timezone
    };
    typedef QVector<timezone_info_t>    timezone_list_t;

                                locale();
                                locale(locale const & rhs) = delete;
    virtual                     ~locale() override;

    locale &                    operator = (locale const & rhs) = delete;

    static locale *             instance();

    // plugin implementation
    virtual QString             settings_path() const override;
    virtual QString             icon() const override;
    virtual QString             description() const override;
    virtual QString             dependencies() const override;
    virtual int64_t             do_update(int64_t last_updated) override;
    virtual void                bootstrap(snap_child * snap) override;

    locale_list_t const &       get_locale_list();
    timezone_list_t const &     get_timezone_list();

    void                        reset_locale();
    QString                     get_current_locale() const;
    void                        set_current_locale(QString const & new_locale);
    QString                     get_current_timezone() const;
    void                        set_current_timezone(QString const & new_timezone);

    SNAP_SIGNAL_WITH_MODE(set_locale, (), (), START_AND_DONE);
    SNAP_SIGNAL_WITH_MODE(set_timezone, (), (), START_AND_DONE);

    QString                     format_date(time_t d);
    QString                     format_date(time_t d, QString const & date_format, bool use_local);
    QString                     format_time(time_t d);
    time_t                      parse_date(QString const & date, parse_error_t & errcode);
    time_t                      parse_time(QString const & time_str, parse_error_t & errcode);

private:
    snap_child *                f_snap = nullptr;
    locale_list_t               f_locale_list = locale_list_t();
    timezone_list_t             f_timezone_list = timezone_list_t();
    QString                     f_current_locale = QString();
    QString                     f_current_timezone = QString();
};


class safe_timezone
{
public:
                    safe_timezone(QString const new_timezone);
                    safe_timezone(safe_timezone const & rhs) = delete;
                    ~safe_timezone();

    safe_timezone & operator = (safe_timezone const & rhs) = delete;

private:
    locale *        f_locale_plugin = nullptr;
    QString         f_old_timezone = QString();
};



} // namespace locale
} // namespace snap
// vim: ts=4 sw=4 et
