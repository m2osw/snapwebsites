// Snap Websites Server -- configuration reader
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

// ourselves
//
#include "snapwebsites/snap_exception.h"

// C++ lib
//
#include <map>
#include <memory>

namespace snap
{

class snap_configurations_exception : public snap_exception
{
public:
    explicit snap_configurations_exception(char const *        what_msg) : snap_exception("snap_configurations", what_msg) {}
    explicit snap_configurations_exception(std::string const & what_msg) : snap_exception("snap_configurations", what_msg) {}
    explicit snap_configurations_exception(QString const &     what_msg) : snap_exception("snap_configurations", what_msg) {}
};

class snap_configurations_exception_too_late : public snap_configurations_exception
{
public:
    explicit snap_configurations_exception_too_late(char const *        what_msg) : snap_configurations_exception(what_msg) {}
    explicit snap_configurations_exception_too_late(std::string const & what_msg) : snap_configurations_exception(what_msg) {}
    explicit snap_configurations_exception_too_late(QString const &     what_msg) : snap_configurations_exception(what_msg) {}
};

class snap_configurations_exception_config_error : public snap_configurations_exception
{
public:
    explicit snap_configurations_exception_config_error(char const *        what_msg) : snap_configurations_exception(what_msg) {}
    explicit snap_configurations_exception_config_error(std::string const & what_msg) : snap_configurations_exception(what_msg) {}
    explicit snap_configurations_exception_config_error(QString const &     what_msg) : snap_configurations_exception(what_msg) {}
};




class snap_configurations
{
public:
    typedef std::shared_ptr<snap_configurations>    pointer_t;
    typedef std::map<std::string, std::string>      parameter_map_t;

    static pointer_t        get_instance();

    std::string const &     get_configuration_path() const;
    void                    set_configuration_path(std::string const & path);

    bool                    configuration_file_exists( std::string const & configuration_filename, std::string const &override_filename );

    parameter_map_t const & get_parameters(std::string const & configuration_filename, std::string const & override_filename) const;
    void                    set_parameters(std::string const & configuration_filename, std::string const & override_filename, parameter_map_t const & params);

    std::string             get_parameter(std::string const & configuration_filename, std::string const & override_filename, std::string const & parameter_name) const;
    bool                    has_parameter(std::string const & configuration_filename, std::string const & override_filename, std::string const & parameter_name) const;
    void                    set_parameter(std::string const & configuration_filename, std::string const & override_filename, std::string const & parameter_name, std::string const & value);

    bool                    save(std::string const & configuration_filename, std::string const & override_filename, bool override_file);

private:
                            snap_configurations();
                            snap_configurations(snap_configurations const &) = delete;
    snap_configurations &   operator = (snap_configurations const &) = delete;
};


class snap_config
{
public:
    class snap_config_parameter_ref
    {
    public:
                                snap_config_parameter_ref(std::string const & configuration_filename, std::string const & override_filename, std::string const & parameter_name)
                                    : f_config(snap_configurations::get_instance())
                                    , f_configuration_filename(configuration_filename)
                                    , f_override_filename(override_filename)
                                    , f_parameter_name(parameter_name)
                                {
                                }

        snap_config_parameter_ref & operator = (char const * value)
                                {
                                    f_config->set_parameter(f_configuration_filename, f_override_filename, f_parameter_name, value);
                                    return *this;
                                }

        snap_config_parameter_ref & operator = (std::string const & value)
                                {
                                    f_config->set_parameter(f_configuration_filename, f_override_filename, f_parameter_name, value);
                                    return *this;
                                }

        snap_config_parameter_ref & operator = (QString const & value)
                                {
                                    f_config->set_parameter(f_configuration_filename, f_override_filename, f_parameter_name, value.toUtf8().data());
                                    return *this;
                                }

        snap_config_parameter_ref & operator = (snap_config_parameter_ref const & rhs)
                                {
                                    // get the value from the rhs
                                    //
                                    std::string const value(rhs.f_config->get_parameter(rhs.f_configuration_filename, f_override_filename, rhs.f_parameter_name));

                                    // save the value in the lhs
                                    //
                                    f_config->set_parameter(f_configuration_filename, f_override_filename, f_parameter_name, value);

                                    return *this;
                                }

        bool                    operator == (char const * value)
                                {
                                    return f_config->get_parameter(f_configuration_filename, f_override_filename, f_parameter_name) == value;
                                }

        bool                    operator == (std::string const & value)
                                {
                                    return f_config->get_parameter(f_configuration_filename, f_override_filename, f_parameter_name) == value;
                                }

        bool                    operator == (QString const & value)
                                {
                                    return f_config->get_parameter(f_configuration_filename, f_override_filename, f_parameter_name) == value.toUtf8().data();
                                }

        bool                    operator != (char const * value)
                                {
                                    return f_config->get_parameter(f_configuration_filename, f_override_filename, f_parameter_name) != value;
                                }

        bool                    operator != (std::string const & value)
                                {
                                    return f_config->get_parameter(f_configuration_filename, f_override_filename, f_parameter_name) != value;
                                }

        bool                    operator != (QString const & value)
                                {
                                    return f_config->get_parameter(f_configuration_filename, f_override_filename, f_parameter_name) != value.toUtf8().data();
                                }

                                operator std::string () const
                                {
                                    return f_config->get_parameter(f_configuration_filename, f_override_filename, f_parameter_name);
                                }

                                operator QString () const
                                {
                                    return QString::fromUtf8(f_config->get_parameter(f_configuration_filename, f_override_filename, f_parameter_name).c_str());
                                }

        bool                    empty() const
                                {
                                    return f_config->get_parameter(f_configuration_filename, f_override_filename, f_parameter_name).empty();
                                }

        size_t                  length() const
                                {
                                    return f_config->get_parameter(f_configuration_filename, f_override_filename, f_parameter_name).length();
                                }

    private:
        snap_configurations::pointer_t  f_config = snap_configurations::pointer_t();
        std::string const               f_configuration_filename = std::string();
        std::string const               f_override_filename = std::string();
        std::string const               f_parameter_name = std::string();
    };

                            snap_config()
                                : f_config(snap_configurations::get_instance())
                            {
                            }

                            snap_config(std::string const & configuration_filename)
                                : f_config(snap_configurations::get_instance())
                                , f_configuration_filename(configuration_filename)
                            {
                            }

                            snap_config(std::string const & configuration_filename, std::string const & override_filename)
                                : f_config(snap_configurations::get_instance())
                                , f_configuration_filename(configuration_filename)
                                , f_override_filename(override_filename)
                            {
                            }

    // get/set_configuration_filename()
    std::string const &     get_configuration_filename() const
                            {
                                return f_configuration_filename;
                            }

    void                    set_configuration_filename(std::string const & configuration_filename)
                            {
                                f_configuration_filename = configuration_filename;
                            }

    bool                    configuration_file_exists() const
                            {
                                return f_config->configuration_file_exists(f_configuration_filename, f_override_filename);
                            }

    // get/set_override_filename()
    std::string const &     get_override_filename() const
                            {
                                return f_override_filename;
                            }

    void                    set_override_filename(std::string const & override_filename)
                            {
                                f_override_filename = override_filename;
                            }

    // set_configuration_path() -- set path where configuration file(s) are installed
    std::string const &     get_configuration_path() const
                            {
                                return f_config->get_configuration_path();
                            }

    void                    set_configuration_path(std::string const & path)
                            {
                                f_config->set_configuration_path(path);
                            }

    // get_parameters() -- retrieve map to all parameters
    snap_configurations::parameter_map_t const & get_parameters(std::string const & configuration_filename, std::string const & override_filename) const
                            {
                                return f_config->get_parameters(configuration_filename, override_filename);
                            }

    snap_configurations::parameter_map_t const & get_parameters(std::string const & configuration_filename) const
                            {
                                return f_config->get_parameters(configuration_filename, f_override_filename);
                            }

    snap_configurations::parameter_map_t const & get_parameters() const
                            {
                                return f_config->get_parameters(f_configuration_filename, f_override_filename);
                            }

    // set_parameters() -- replace specified parameters
    void                    set_parameters(std::string const & configuration_filename, std::string const & override_filename, snap_configurations::parameter_map_t const & params)
                            {
                                f_config->set_parameters(configuration_filename, override_filename, params);
                            }

    void                    set_parameters(std::string const & configuration_filename, snap_configurations::parameter_map_t const & params)
                            {
                                f_config->set_parameters(configuration_filename, f_override_filename, params);
                            }

    void                    set_parameters(snap_configurations::parameter_map_t const & params)
                            {
                                f_config->set_parameters(f_configuration_filename, f_override_filename, params);
                            }

    // get_parameter() -- retrieve parameter value, specifying the configuration filename or not
    std::string             get_parameter(std::string const & configuration_filename, std::string const & override_filename, std::string const & parameter_name) const
                            {
                                return f_config->get_parameter(configuration_filename, override_filename, parameter_name);
                            }

    std::string             get_parameter(std::string const & configuration_filename, std::string const & parameter_name) const
                            {
                                return f_config->get_parameter(configuration_filename, f_override_filename, parameter_name);
                            }

    std::string             get_parameter(std::string const & parameter_name) const
                            {
                                return f_config->get_parameter(f_configuration_filename, f_override_filename, parameter_name);
                            }

    // has_parameter() -- check whether parameter is defined, specifying the configuration filename or not
    bool                    has_parameter(char const * configuration_filename, char const * parameter_name) const
                            {
                                return f_config->has_parameter(configuration_filename, f_override_filename, parameter_name);
                            }

    bool                    has_parameter(char const * configuration_filename, std::string const & parameter_name) const
                            {
                                return f_config->has_parameter(configuration_filename, f_override_filename, parameter_name);
                            }

    bool                    has_parameter(std::string const & configuration_filename, char const * parameter_name) const
                            {
                                return f_config->has_parameter(configuration_filename, f_override_filename, parameter_name);
                            }

    bool                    has_parameter(std::string const & configuration_filename, std::string const & parameter_name) const
                            {
                                return f_config->has_parameter(configuration_filename, f_override_filename, parameter_name);
                            }

    bool                    has_parameter(QString const & configuration_filename, QString const & parameter_name) const
                            {
                                return f_config->has_parameter(configuration_filename.toUtf8().data(), f_override_filename, parameter_name.toUtf8().data());
                            }

    bool                    has_parameter(char const * parameter_name) const
                            {
                                return f_config->has_parameter(f_configuration_filename, f_override_filename, parameter_name);
                            }

    bool                    has_parameter(std::string const & parameter_name) const
                            {
                                return f_config->has_parameter(f_configuration_filename, f_override_filename, parameter_name);
                            }

    bool                    has_parameter(QString const & parameter_name) const
                            {
                                return f_config->has_parameter(f_configuration_filename, f_override_filename, parameter_name.toUtf8().data());
                            }

    // set_parameter() -- set parameter value, specifying the configuration filename or not
    void                    set_parameter(std::string const & configuration_filename, std::string const & parameter_name, std::string const & value)
                            {
                                f_config->set_parameter(configuration_filename, f_override_filename, parameter_name, value);
                            }

    void                    set_parameter(std::string const & parameter_name, std::string const & value)
                            {
                                f_config->set_parameter(f_configuration_filename, f_override_filename, parameter_name, value);
                            }

    // set_parameter_default() -- set parameter value if still undefined, specifying the configuration filename or not
    void                    set_parameter_default(std::string const & configuration_filename, std::string const & parameter_name, std::string const & value)
                            {
                                if(!f_config->has_parameter(configuration_filename, f_override_filename, parameter_name))
                                {
                                    f_config->set_parameter(configuration_filename, f_override_filename, parameter_name, value);
                                }
                            }

    void                    set_parameter_default(std::string const & parameter_name, std::string const & value)
                            {
                                if(!f_config->has_parameter(f_configuration_filename, f_override_filename, parameter_name))
                                {
                                    f_config->set_parameter(f_configuration_filename, f_override_filename, parameter_name, value);
                                }
                            }

    // f_config() -- get parameter, specifying the configuration filename or not
    std::string             operator () (char const * configuration_filename, char const * parameter_name) const
                            {
                                return f_config->get_parameter(configuration_filename, f_override_filename, parameter_name);
                            }

    std::string             operator () (char const * configuration_filename, std::string const & parameter_name) const
                            {
                                return f_config->get_parameter(configuration_filename, f_override_filename, parameter_name);
                            }

    std::string             operator () (std::string const & configuration_filename, char const * parameter_name) const
                            {
                                return f_config->get_parameter(configuration_filename, f_override_filename, parameter_name);
                            }

    std::string             operator () (std::string const & configuration_filename, std::string const & parameter_name) const
                            {
                                return f_config->get_parameter(configuration_filename, f_override_filename, parameter_name);
                            }

    QString                 operator () (QString const & configuration_filename, QString const & parameter_name) const
                            {
                                return QString::fromUtf8(f_config->get_parameter(configuration_filename.toUtf8().data(), f_override_filename, parameter_name.toUtf8().data()).c_str());
                            }

    std::string             operator () (char const * parameter_name) const
                            {
                                return f_config->get_parameter(f_configuration_filename, f_override_filename, parameter_name);
                            }

    std::string             operator () (std::string const & parameter_name) const
                            {
                                return f_config->get_parameter(f_configuration_filename, f_override_filename, parameter_name);
                            }

    QString                 operator () (QString const & parameter_name) const
                            {
                                return QString::fromUtf8(f_config->get_parameter(f_configuration_filename, f_override_filename, parameter_name.toUtf8().data()).c_str());
                            }

    // f_config[] -- get parameter, only from context configuration
    std::string             operator [] (char const * parameter_name) const
                            {
                                return f_config->get_parameter(f_configuration_filename, f_override_filename, parameter_name);
                            }

    std::string             operator [] (std::string const & parameter_name) const
                            {
                                return f_config->get_parameter(f_configuration_filename, f_override_filename, parameter_name);
                            }

    QString                 operator [] (QString const & parameter_name) const
                            {
                                return QString::fromUtf8(f_config->get_parameter(f_configuration_filename, f_override_filename, parameter_name.toUtf8().data()).c_str());
                            }

    // f_config[] = ... -- set parameter, only to context configuration
    snap_config_parameter_ref operator [] (char const * parameter_name)
                            {
                                return snap_config_parameter_ref(f_configuration_filename, f_override_filename, parameter_name);
                            }

    snap_config_parameter_ref operator [] (std::string const & parameter_name)
                            {
                                return snap_config_parameter_ref(f_configuration_filename, f_override_filename, parameter_name);
                            }

    snap_config_parameter_ref operator [] (QString const & parameter_name)
                            {
                                return snap_config_parameter_ref(f_configuration_filename, f_override_filename, parameter_name.toUtf8().data());
                            }

    bool                    save(bool override_file = true)
                            {
                                return f_config->save(f_configuration_filename, f_override_filename, override_file);
                            }

private:
    snap_configurations::pointer_t  f_config = snap_configurations::pointer_t();
    std::string                     f_configuration_filename = std::string();
    std::string                     f_override_filename = std::string();
};



}
// namespace snap
// vim: ts=4 sw=4 et
