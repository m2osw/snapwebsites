// Snap Websites Server -- snap watchdog daemon
// Copyright (c) 2011-2018  Made to Order Software Corp.  All Rights Reserved
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

// C++ lib
//
#include <memory>
#include <set>


namespace snap
{

class flags_exception : public snap_exception
{
public:
    flags_exception(char const *        what_msg) : snap_exception("flags", what_msg) {}
    flags_exception(std::string const & what_msg) : snap_exception("flags", what_msg) {}
    flags_exception(QString const &     what_msg) : snap_exception("flags", what_msg) {}
};

class flags_exception_invalid_parameter : public flags_exception
{
public:
    flags_exception_invalid_parameter(char const *        what_msg) : flags_exception(what_msg) {}
    flags_exception_invalid_parameter(std::string const & what_msg) : flags_exception(what_msg) {}
    flags_exception_invalid_parameter(QString const &     what_msg) : flags_exception(what_msg) {}
};

class flags_exception_too_many_flags : public flags_exception
{
public:
    flags_exception_too_many_flags(char const *        what_msg) : flags_exception(what_msg) {}
    flags_exception_too_many_flags(std::string const & what_msg) : flags_exception(what_msg) {}
    flags_exception_too_many_flags(QString const &     what_msg) : flags_exception(what_msg) {}
};





class watchdog_flag
{
public:
    typedef std::shared_ptr<watchdog_flag>      pointer_t;
    typedef std::vector<pointer_t>              vector_t;

    typedef std::set<std::string>               tag_list_t;

    enum class state_t
    {
        STATE_UP,       // flag is an error
        STATE_DOWN      // flag file gets deleted
    };

                                watchdog_flag(std::string const & unit, std::string const & section, std::string const & name);
                                watchdog_flag(std::string const & filename);

    watchdog_flag &             set_state(state_t state);
    watchdog_flag &             set_source_file(std::string const & source_file);
    watchdog_flag &             set_function(std::string const & function);
    watchdog_flag &             set_line(int line);
    watchdog_flag &             set_message(std::string const & message);
    watchdog_flag &             set_message(QString const & message);
    watchdog_flag &             set_message(char const * message);
    watchdog_flag &             set_priority(int priority);
    watchdog_flag &             set_manual_down(bool manual);
    watchdog_flag &             add_tag(std::string const & tag);

    state_t                     get_state() const;
    std::string const &         get_unit() const;
    std::string const &         get_section() const;
    std::string const &         get_name() const;
    std::string const &         get_filename() const;
    std::string const &         get_source_file() const;
    std::string const &         get_function() const;
    int                         get_line() const;
    std::string const &         get_message() const;
    int                         get_priority() const;
    bool                        get_manual_down() const;
    time_t                      get_date() const;
    time_t                      get_modified() const;
    tag_list_t const &          get_tags() const;

    bool                        save();

    static vector_t             load_flags();

private:
    static void                 valid_name(std::string & name);
    static void                 load_flag(std::string const & filename, watchdog_flag::vector_t * result);

    state_t                     f_state             = state_t::STATE_UP;
    std::string                 f_unit              = std::string();
    std::string                 f_section           = std::string();
    std::string                 f_name              = std::string();
    mutable std::string         f_filename          = std::string();
    std::string                 f_source_file       = std::string();
    std::string                 f_function          = std::string();
    int                         f_line              = 0;
    std::string                 f_message           = std::string();
    int                         f_priority          = 5;
    bool                        f_manual_down       = false;
    time_t                      f_date              = -1;
    time_t                      f_modified          = -1;
    tag_list_t                  f_tags              = tag_list_t();
};



#define SNAPWATHCDOG_FLAG_UP(unit, section, name, message)   \
            std::make_shared<snap::watchdog_flag>( \
                snap::watchdog_flag(unit, section, name) \
                    .set_message(message) \
                    .set_source_file(__FILE__) \
                    .set_function(__func__) \
                    .set_line(__LINE__))

#define SNAPWATHCDOG_FLAG_DOWN(unit, section, name) \
            std::make_shared<snap::watchdog_flag>( \
                snap::watchdog_flag(unit, section, name) \
                    .set_state(snap::watchdog_flag::state_t::STATE_DOWN) \
                    .set_source_file(__FILE__) \
                    .set_function(__func__) \
                    .set_line(__LINE__))



}
// snap namespace
// vim: ts=4 sw=4 et
