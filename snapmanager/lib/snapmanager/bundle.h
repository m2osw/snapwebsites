// Snap Websites Server -- handle snapmanager bundles
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

// Qt lib
//
#include <QDomDocument>

// C++ lib
//
#include <map>
#include <memory>
#include <set>
#include <vector>


namespace snap_manager
{


class manager;

template<typename T>
struct bundle_field;

class bundle
{
public:
    typedef std::shared_ptr<bundle>     pointer_t;
    typedef std::weak_ptr<bundle>       weak_t;
    typedef std::vector<pointer_t>      vector_t;
    typedef std::vector<weak_t>         weak_vector_t;
    typedef std::vector<std::string>    string_set_t; // our tokenize_string doesn't work with std::set<>...
    typedef std::vector<std::string>    string_vector_t;
    typedef std::map<std::string, std::string>  string_map_t;

    friend bool operator == (weak_t const & lhs, pointer_t const & rhs)
    {
        return lhs.lock() == rhs;
    }

    static constexpr time_t             BUNDLE_CACHE_FILE_LIFETIME  = 86400;    // 1 day in seconds
    static constexpr time_t             PACKAGE_CACHE_FILE_LIFETIME = 86400;    // 1 day in seconds

    // keep the status as a letter so we can easily save it to a text file
    // (we use a text file to cache the status to avoid running dpkg-query
    // too often, it's rather slow otherwise!)
    //
    // WARNING: these letters get saved in a status file, DO NOT MODIFY
    //          or all the caches will be invalid
    //
    enum class bundle_status_t
    {
        // unknown status
        //
        BUNDLE_STATUS_UNKNOWN = 'U',        // unknown status (not yet checked)
        BUNDLE_STATUS_ERROR = 'E',          // can't get status

        // bundle is installed
        //
        BUNDLE_STATUS_HIDE = 'H',           // installed, now hide (so can't be removed)
        BUNDLE_STATUS_INSTALLED = 'I',      // installed
        BUNDLE_STATUS_LOCKED = 'L',         // installed, locked by another bundle (prereq)

        // bundle is not installed
        //
        BUNDLE_STATUS_NOT_INSTALLED = 'W',  // not yet installed (originally "Warning")
        BUNDLE_STATUS_PREREQ_MISSING = 'R', // not yet installed, but require another bundle (prereq)
        BUNDLE_STATUS_IN_CONFLICT = 'C'     // not installed, but in conflict with an installed package
    };

    class field
    {
    public:
        typedef std::shared_ptr<field>  pointer_t;
        typedef std::vector<pointer_t>  vector_t;

        bool                    init(QDomElement e);

        std::string const &     get_name() const;
        std::string const &     get_type() const;
        std::string const &     get_label() const;
        std::string const &     get_initial_value() const;
        string_set_t const &    get_options() const;
        std::string const &     get_description() const;

    private:
        static std::vector<bundle_field<bundle::field>>
                                g_bundle_field_fields;

        std::string             f_name = std::string();
        std::string             f_type = std::string("input");
        std::string             f_label = std::string();
        std::string             f_initial_value = std::string();
        string_set_t            f_options = string_set_t();
        std::string             f_description = std::string();
    };

    class package
    {
    public:
        typedef std::shared_ptr<package>            pointer_t;
        typedef std::map<std::string, pointer_t>    map_t;

                                package(std::shared_ptr<manager> m, std::string const & name);

        bool                    is_installed() const;
        std::string const &     get_name() const;
        std::string const &     get_status() const;
        std::string const &     get_version() const;

    private:
        void                    check_status();

        std::shared_ptr<manager>
                                f_snap;

        std::string             f_name = std::string();
        std::string             f_status = std::string();
        std::string             f_version = std::string();
    };

                            bundle(std::shared_ptr<manager> m);

    bool                    init(QDomDocument doc);

    std::string const &     get_name() const;
    std::string const &     get_description() const;
    bool                    get_hide() const;
    bool                    get_expected() const;
    string_set_t const &    get_affected_services() const;
    string_set_t const &    get_packages() const;
    string_set_t const &    get_prereq() const;
    string_set_t const &    get_conflicts() const;
    string_set_t const &    get_suggestions() const;
    std::string const &     get_is_installed() const;
    std::string const &     get_preinst() const;
    std::string const &     get_postinst() const;
    std::string const &     get_prerm() const;
    std::string const &     get_postrm() const;
    field::vector_t const & get_fields() const;

    void                    add_prereq_bundle(pointer_t b);
    weak_vector_t const &   get_prereq_bundles() const;
    void                    add_locked_by_bundle(pointer_t b);
    weak_vector_t const &   get_locked_by_bundles() const;
    void                    add_conflicts_bundle(pointer_t b);
    weak_vector_t const &   get_conflicts_bundles() const;
    void                    add_suggestions_bundle(pointer_t b);
    weak_vector_t const &   get_suggestions_bundles() const;

    package::pointer_t      get_package(std::string const & name) const;
    bundle_status_t         get_bundle_status() const;

    string_vector_t         get_errors() const;

private:
    void                    determine_bundle_status() const;

    static std::vector<bundle_field<bundle>>
                            g_bundle_fields;

    std::shared_ptr<manager>
                            f_snap;                                     // pointer back to manager

    std::string             f_name = std::string();                     // the name of the bundle
    std::string             f_description = std::string();              // a human description
    std::string             f_hide = std::string();                     // whether this bundle gets hidden once installed (i.e. prevent Uninstall)
    bool                    f_expected = false;                         // whether this bundle should always be installed ("expected" to be installed)
    string_set_t            f_affected_services = string_set_t();       // list of affected services which need to be restarted after this bundle was installed
    string_set_t            f_packages = string_set_t();                // list of packages that will be installed by this bundle
    string_set_t            f_prereq = string_set_t();                  // list of bundles that must be installed before this one can be installed
    string_set_t            f_conflicts = string_set_t();               // list of bundles that prevent this one from being installed (and vice versa)
    string_set_t            f_suggestions = string_set_t();             // list of bundles that prevent this one from being installed (and vice versa)
    std::string             f_is_installed = std::string();             // script used to check whether the package is considered installed
    std::string             f_preinst = std::string();                  // script run before proceeding with the installation of the packages
    std::string             f_postinst = std::string();                 // script run after installation of the packages
    std::string             f_prerm = std::string();                    // script run before proceeding with the removal of the packages
    std::string             f_postrm = std::string();                   // script run after removal of the packages
    field::vector_t         f_fields = field::vector_t();               // list of fields to add to the form, values are used by the installer

    weak_vector_t           f_prereq_bundles = weak_vector_t();         // list of bundles that are prereqs
    weak_vector_t           f_locked_by_bundles = weak_vector_t();      // list of bundles that prevent removal of this bundle
    weak_vector_t           f_conflicts_bundles = weak_vector_t();      // list of bundles that are conflicts
    weak_vector_t           f_suggestions_bundles = weak_vector_t();    // list of bundles that are suggestions

    mutable package::map_t  f_package_status = package::map_t();        // [package-name] = <package-object>
    mutable bundle_status_t f_bundle_status = bundle_status_t::BUNDLE_STATUS_UNKNOWN;   // status of bundle

    mutable string_vector_t f_errors = string_vector_t();               // a list of errors that happened while loading/determining data
};






} // namespace snap_manager
// vim: ts=4 sw=4 et
