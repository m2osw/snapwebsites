// Snap Websites Server -- snap watchdog daemon
// Copyright (c) 2011-2019  Made to Order Software Corp.  All Rights Reserved
//
// https://snapwebsites.org/
// contact@m2osw.com
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

// lib snapwebsites
//
#include <snapwebsites/snap_exception.h>
#include <snapwebsites/snap_string_list.h>



namespace snap
{

class log_definitions_exception : public snap_exception
{
public:
    log_definitions_exception(char const *        what_msg) : snap_exception("log_definitions", what_msg) {}
    log_definitions_exception(std::string const & what_msg) : snap_exception("log_definitions", what_msg) {}
    log_definitions_exception(QString const &     what_msg) : snap_exception("log_definitions", what_msg) {}
};

class log_definitions_exception_invalid_parameter : public log_definitions_exception
{
public:
    log_definitions_exception_invalid_parameter(char const *        what_msg) : log_definitions_exception(what_msg) {}
    log_definitions_exception_invalid_parameter(std::string const & what_msg) : log_definitions_exception(what_msg) {}
    log_definitions_exception_invalid_parameter(QString const &     what_msg) : log_definitions_exception(what_msg) {}
};


class watchdog_log_t
{
public:
    typedef std::vector<watchdog_log_t>     vector_t;

    static size_t const         MAX_SIZE_UNDEFINED = 0;

    class search_t
    {
    public:
        typedef std::vector<search_t>       vector_t;

                                    search_t(QString const & regex, QString const & report_as);

        QString const &             get_regex() const;
        QString const &             get_report_as() const;

    private:
        QString                     f_regex = QString();
        QString                     f_report_as = QString("error");
    };

                                watchdog_log_t(QString const & name, bool mandatory);

    void                        set_mandatory(bool mandatory);
    void                        set_secure(bool secure);
    void                        set_path(QString const & path);
    void                        set_user_name(QString const & user_name);
    void                        set_group_name(QString const & group_name);
    void                        set_mode(int mode);
    void                        set_mode_mask(int mode_mask);
    void                        add_pattern(QString const & pattern);
    void                        set_max_size(size_t size);
    void                        add_search(search_t const & search);

    QString const &             get_name() const;
    bool                        is_mandatory() const;
    bool                        is_secure() const;
    QString const &             get_path() const;
    uid_t                       get_uid() const;
    gid_t                       get_gid() const;
    mode_t                      get_mode() const;
    mode_t                      get_mode_mask() const;
    snap::snap_string_list const &
                                get_patterns() const;
    size_t                      get_max_size() const;
    search_t::vector_t          get_searches() const;

    static watchdog_log_t::vector_t
                                load(QString log_definitions_path);

private:
    QString                     f_name          = QString();
    QString                     f_path          = QString("/var/log/snapwebsites");
    snap::snap_string_list      f_patterns      = { QString("*.log") };
    size_t                      f_max_size      = MAX_SIZE_UNDEFINED;
    uid_t                       f_uid           = -1;
    gid_t                       f_gid           = -1;
    int                         f_mode          = 0;
    int                         f_mode_mask     = 07777; // i.e. no masking
    search_t::vector_t          f_searches      = search_t::vector_t();
    bool                        f_mandatory     = false;
    bool                        f_secure        = false;
    bool                        f_first_pattern = true;
};


}
// snap namespace
// vim: ts=4 sw=4 et
