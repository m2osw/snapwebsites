// Snap Websites Server -- handle versions and dependencies
// Copyright (C) 2014-2017  Made to Order Software Corp.
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
#include "snapwebsites/qstring_stream.h"

#include <QVector>


namespace snap
{
namespace snap_version
{

class snap_version_exception : public snap_exception
{
public:
    explicit snap_version_exception(char const *        whatmsg) : snap_exception("snap_version", whatmsg) {}
    explicit snap_version_exception(std::string const & whatmsg) : snap_exception("snap_version", whatmsg) {}
    explicit snap_version_exception(QString const &     whatmsg) : snap_exception("snap_version", whatmsg) {}
};

class snap_version_exception_invalid_extension : public snap_version_exception
{
public:
    explicit snap_version_exception_invalid_extension(char const *        whatmsg) : snap_version_exception(whatmsg) {}
    explicit snap_version_exception_invalid_extension(std::string const & whatmsg) : snap_version_exception(whatmsg) {}
    explicit snap_version_exception_invalid_extension(QString const &     whatmsg) : snap_version_exception(whatmsg) {}
};


enum class compare_t : signed int
{
    COMPARE_INVALID = -2, // i.e. unordered
    COMPARE_SMALLER = -1,
    COMPARE_EQUAL   =  0,
    COMPARE_LARGER  =  1
};

typedef uint32_t basic_version_number_t;

static basic_version_number_t const SPECIAL_VERSION_ALL               = static_cast<basic_version_number_t>(-4); // apply to all branches
static basic_version_number_t const SPECIAL_VERSION_EXTENDED          = static_cast<basic_version_number_t>(-3); // revision of .js/.css may be more than one number
static basic_version_number_t const SPECIAL_VERSION_INVALID           = static_cast<basic_version_number_t>(-2);
static basic_version_number_t const SPECIAL_VERSION_UNDEFINED         = static_cast<basic_version_number_t>(-1);
static basic_version_number_t const SPECIAL_VERSION_MIN               = 0;
static basic_version_number_t const SPECIAL_VERSION_SYSTEM_BRANCH     = 0;
static basic_version_number_t const SPECIAL_VERSION_USER_FIRST_BRANCH = 1;
static basic_version_number_t const SPECIAL_VERSION_FIRST_REVISION    = 0;
static basic_version_number_t const SPECIAL_VERSION_MAX_BRANCH_NUMBER = static_cast<basic_version_number_t>(-5);
static basic_version_number_t const SPECIAL_VERSION_MAX               = static_cast<basic_version_number_t>(-1);

static basic_version_number_t const SPECIAL_VERSION_DEFAULT           = SPECIAL_VERSION_UNDEFINED;

class version_number_t
{
public:
    version_number_t()
        //: f_version(SPECIAL_VERSION_UNDEFINED) -- auto-init
    {
    }

    version_number_t(version_number_t const & rhs)
        : f_version(rhs.f_version)
    {
    }

    version_number_t(basic_version_number_t const v)
        : f_version(v)
    {
    }

    version_number_t & operator = (version_number_t const & rhs)
    {
        f_version = rhs.f_version;
        return *this;
    }

    version_number_t & operator = (basic_version_number_t const v)
    {
        f_version = v;
        return *this;
    }

    /*explicit*/ operator basic_version_number_t () const
    {
        return f_version;
    }

    version_number_t & operator -- ()
    {
        --f_version;
        return *this;
    }

    version_number_t & operator ++ ()
    {
        ++f_version;
        return *this;
    }

    version_number_t operator -- (int)
    {
        version_number_t const copy(*this);
        --f_version;
        return copy;
    }

    version_number_t operator ++ (int)
    {
        version_number_t const copy(*this);
        ++f_version;
        return copy;
    }

private:
    basic_version_number_t          f_version = SPECIAL_VERSION_UNDEFINED;
};

typedef QVector<version_number_t> version_numbers_vector_t;

enum class operator_t
{
    /* ?? */ OPERATOR_UNORDERED,
    /* == */ OPERATOR_EQUAL,
    /* != */ OPERATOR_EXCEPT,
    /* <  */ OPERATOR_EARLIER,
    /* >  */ OPERATOR_LATER,
    /* <= */ OPERATOR_EARLIER_OR_EQUAL,
    /* >= */ OPERATOR_LATER_OR_EQUAL
};


char const * find_extension(QString const & filename, char const ** extensions);
bool validate_basic_name(QString const & name, QString & error);
bool validate_name(QString & name, QString & error, QString & namespace_string);
bool validate_version(QString const & version_string, version_numbers_vector_t & version, QString & error);
bool validate_operator(QString const & operator_string, operator_t & op, QString & error);


class name
{
public:
    typedef QVector<name>   vector_t;

    void                    clear() { f_name.clear(); f_error.clear(); }
    bool                    set_name(QString const & name_string);
    QString const &         get_name() const { return f_name; }
    QString const &         get_namespace() const { return f_namespace; }
    bool                    is_valid() const { return f_error.isEmpty(); }
    QString                 get_error() const { return f_error; }

    compare_t               compare(name const & rhs) const;

private:
    QString                 f_name;
    QString                 f_namespace;
    QString                 f_error;
};


class version_operator
{
public:
    bool                    set_operator_string(QString const & operator_string);
    bool                    set_operator(operator_t op);
    char const *            get_operator_string() const;
    operator_t              get_operator() const { return f_operator; }
    bool                    is_valid() const { return f_error.isEmpty(); }
    QString const &         get_error() const { return f_error; }

private:
    operator_t              f_operator = operator_t::OPERATOR_UNORDERED;
    QString                 f_error;
};


class version
{
public:
    typedef QVector<version>    vector_t;

    bool                        set_version_string(QString const & version_string);
    void                        set_version(version_numbers_vector_t const & version);
    void                        set_operator(version_operator const & op);
    version_numbers_vector_t const & get_version() const { return f_version; }
    QString const &             get_version_string() const;
    QString                     get_opversion_string() const;
    version_operator const &    get_operator() const { return f_operator; }
    bool                        is_valid() const { return f_error.isEmpty() && f_operator.is_valid(); }
    QString                     get_error() const { return f_error; }

    compare_t                   compare(version const & rhs) const;

private:
    mutable QString             f_version_string;
    version_numbers_vector_t    f_version;
    QString                     f_error;
    version_operator            f_operator;
};


class versioned_filename
{
public:

                                versioned_filename(QString const & extension);

    bool                        set_filename(QString const & filename);
    bool                        set_name(QString const & name);
    bool                        set_version(QString const & version_string);

    bool                        is_valid() const { return f_error.isEmpty() && f_name.is_valid() && f_version.is_valid() && f_browser.is_valid(); }

    QString                     get_error() const { return f_error; }
    QString                     get_filename(bool extension = false) const;
    QString const &             get_extension() const { return f_extension; }
    QString const &             get_name() const { return f_name.get_name(); }
    QString const &             get_version_string() const { return f_version.get_version_string(); } // this was canonicalized
    version_numbers_vector_t const & get_version() const { return f_version.get_version(); }
    QString const &             get_browser() const { return f_browser.get_name(); }

    compare_t                   compare(versioned_filename const & rhs) const;
    bool                        operator == (versioned_filename const & rhs) const;
    bool                        operator != (versioned_filename const & rhs) const;
    bool                        operator <  (versioned_filename const & rhs) const;
    bool                        operator <= (versioned_filename const & rhs) const;
    bool                        operator >  (versioned_filename const & rhs) const;
    bool                        operator >= (versioned_filename const & rhs) const;

                                operator bool () const { return is_valid(); }
    bool                        operator ! () const { return !is_valid(); }

private:
    QString                     f_error;
    QString                     f_extension;
    name                        f_name;
    version                     f_version;
    name                        f_browser;
};


class dependency
{
public:
    bool                        set_dependency(QString const & dependency_string);
    QString                     get_dependency_string() const;
    QString const &             get_name() const { return f_name.get_name(); }
    QString const &             get_namespace() const { return f_name.get_namespace(); }
    version::vector_t const &   get_versions() const { return f_versions; }
    name::vector_t const &      get_browsers() const { return f_browsers; }
    bool                        is_valid() const;
    QString const &             get_error() const { return f_error; }

private:
    QString                     f_error;
    name                        f_name;
    version::vector_t           f_versions;
    name::vector_t              f_browsers;
};
typedef QVector<dependency>     dependency_vector_t;


class quick_find_version_in_source
{
public:
                                quick_find_version_in_source();

    bool                        find_version(char const * data, int const size);

    void                        set_name(QString const & name) { f_name.set_name(name); }
    QString const &             get_name() const { return f_name.get_name(); }
    QString const &             get_layout() const { return f_layout.get_name(); }
    QString const &             get_version_string() const { return f_version.get_version_string(); } // this was canonicalized
    version_number_t            get_branch() const { if(f_version.get_version().empty()) return SPECIAL_VERSION_UNDEFINED; else return f_version.get_version()[0]; }
    version_numbers_vector_t const & get_version() const { return f_version.get_version(); }
    name::vector_t const &      get_browsers() const { return f_browsers; }
    QString const &             get_description() const { return f_description; }
    dependency_vector_t const & get_depends() const { return f_depends; }
    bool                        is_defined() const { return f_data != nullptr; }
    bool                        is_valid() const;
    QString const &             get_error() const { return f_error; }

private:
    int                         getc();
    QString                     get_line();

    char const *                f_data = nullptr;
    char const *                f_end = nullptr;

    name                        f_name;
    name                        f_layout;
    version                     f_version;
    name::vector_t              f_browsers;
    QString                     f_error;
    QString                     f_description;
    dependency_vector_t         f_depends;
};


} // namespace snap_version
} // namespace snap
// vim: ts=4 sw=4 et
