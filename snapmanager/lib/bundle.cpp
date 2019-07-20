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

// self
//
#include "snapmanager/bundle.h"


// snapmanager lib
//
#include "snapmanager/manager.h"


// snapwebsites lib
//
#include <snapwebsites/file_content.h>
#include <snapwebsites/log.h>
#include <snapwebsites/mkdir_p.h>
#include <snapwebsites/process.h>
#include <snapwebsites/qdomhelpers.h>
#include <snapwebsites/snap_string_list.h>


// snapdev lib
//
#include <snapdev/not_used.h>
#include <snapdev/tokenize_string.h>


// boost lib
//
#include <boost/algorithm/string.hpp>


// C++ lib
//
#include <algorithm>
#include <fstream>


// C lib
//
#include <sys/stat.h>


// last include
//
#include <snapdev/poison.h>



/** \file
 * \brief This file manages bundles in a set of easy to use C++ objects.
 *
 * The bundles define how to handle one set of packages. It includes setup
 * fields, pre and post scripts, conflicts, prerequisites, a name, and
 * a description. All of that is handled by this class.
 *
 * The load function makes sure to create a set of bundles by name, then
 * we load the rest of the data. Especially, we need the named bundle
 * objects to exist before we can link them together for conflicts
 * and prerequisites. Links are actually going both ways. For conflicts,
 * the exact same linking is used either way. For prerequisites we have
 * to sets of links:
 *
 * \li A depends on B, and
 * \li B is required by A.
 */




namespace snap_manager
{




template<typename T>
struct bundle_field
{
    typedef std::vector<bundle_field<T>>    vector_t;

    enum class type_t
    {
        BUNDLE_FIELD_TYPE_PLAIN,
        BUNDLE_FIELD_TYPE_SCRIPT,
        BUNDLE_FIELD_TYPE_HTML,
        BUNDLE_FIELD_TYPE_LIST,
        BUNDLE_FIELD_TYPE_FLAG,
        BUNDLE_FIELD_TYPE_ATTRIBUTE,
        BUNDLE_FIELD_TYPE_FIELDS
    };

    type_t                          f_type          = type_t::BUNDLE_FIELD_TYPE_PLAIN;
    char const *                    f_name          = nullptr;
    bool                            f_required      = false;
    std::string T::*                f_data_string   = nullptr;
    bool T::*                       f_data_flag     = nullptr;
    bundle::string_set_t T::*       f_data_list     = nullptr;
    bundle::field::vector_t T::*    f_data_fields   = nullptr;
};


bundle_field<bundle>::vector_t    bundle::g_bundle_fields =
{
    {
        bundle_field<bundle>::type_t::BUNDLE_FIELD_TYPE_ATTRIBUTE,
        "hide",
        false,
        &bundle::f_hide,
        nullptr,
        nullptr,
        nullptr
    },
    {
        bundle_field<bundle>::type_t::BUNDLE_FIELD_TYPE_PLAIN,
        "name",
        true,
        &bundle::f_name,
        nullptr,
        nullptr,
        nullptr
    },
    {
        bundle_field<bundle>::type_t::BUNDLE_FIELD_TYPE_HTML,
        "description",
        true,
        &bundle::f_description,
        nullptr,
        nullptr,
        nullptr
    },
    {
        bundle_field<bundle>::type_t::BUNDLE_FIELD_TYPE_FLAG,
        "expected",
        false,
        nullptr,
        &bundle::f_expected,
        nullptr,
        nullptr
    },
    {
        bundle_field<bundle>::type_t::BUNDLE_FIELD_TYPE_LIST,
        "affected-services",
        false,
        nullptr,
        nullptr,
        &bundle::f_affected_services,
        nullptr
    },
    {
        bundle_field<bundle>::type_t::BUNDLE_FIELD_TYPE_LIST,
        "packages",
        false,
        nullptr,
        nullptr,
        &bundle::f_packages,
        nullptr
    },
    {
        bundle_field<bundle>::type_t::BUNDLE_FIELD_TYPE_LIST,
        "prereq",
        false,
        nullptr,
        nullptr,
        &bundle::f_prereq,
        nullptr
    },
    {
        bundle_field<bundle>::type_t::BUNDLE_FIELD_TYPE_LIST,
        "conflicts",
        false,
        nullptr,
        nullptr,
        &bundle::f_conflicts,
        nullptr
    },
    {
        bundle_field<bundle>::type_t::BUNDLE_FIELD_TYPE_LIST,
        "suggestions",
        false,
        nullptr,
        nullptr,
        &bundle::f_suggestions,
        nullptr
    },
    {
        bundle_field<bundle>::type_t::BUNDLE_FIELD_TYPE_SCRIPT,
        "is-installed",
        false,
        &bundle::f_is_installed,
        nullptr,
        nullptr,
        nullptr
    },
    {
        bundle_field<bundle>::type_t::BUNDLE_FIELD_TYPE_SCRIPT,
        "preinst",
        false,
        &bundle::f_preinst,
        nullptr,
        nullptr,
        nullptr
    },
    {
        bundle_field<bundle>::type_t::BUNDLE_FIELD_TYPE_SCRIPT,
        "postinst",
        false,
        &bundle::f_postinst,
        nullptr,
        nullptr,
        nullptr
    },
    {
        bundle_field<bundle>::type_t::BUNDLE_FIELD_TYPE_SCRIPT,
        "prerm",
        false,
        &bundle::f_prerm,
        nullptr,
        nullptr,
        nullptr
    },
    {
        bundle_field<bundle>::type_t::BUNDLE_FIELD_TYPE_SCRIPT,
        "postrm",
        false,
        &bundle::f_postrm,
        nullptr,
        nullptr,
        nullptr
    },
    {
        bundle_field<bundle>::type_t::BUNDLE_FIELD_TYPE_FIELDS,
        "fields",
        false,
        nullptr,
        nullptr,
        nullptr,
        &bundle::f_fields
    }
};



bundle_field<bundle::field>::vector_t    bundle::field::g_bundle_field_fields =
{
    {
        bundle_field<bundle::field>::type_t::BUNDLE_FIELD_TYPE_ATTRIBUTE,
        "name",
        true,
        &bundle::field::f_name,
        nullptr,
        nullptr,
        nullptr
    },
    {
        bundle_field<bundle::field>::type_t::BUNDLE_FIELD_TYPE_ATTRIBUTE,
        "type",
        false,
        &bundle::field::f_type,
        nullptr,
        nullptr,
        nullptr
    },
    {
        bundle_field<bundle::field>::type_t::BUNDLE_FIELD_TYPE_HTML,
        "label",
        true,
        &bundle::field::f_label,
        nullptr,
        nullptr,
        nullptr
    },
    {
        bundle_field<bundle::field>::type_t::BUNDLE_FIELD_TYPE_HTML,
        "initial-value",
        false,
        &bundle::field::f_initial_value,
        nullptr,
        nullptr,
        nullptr
    },
    {
        bundle_field<bundle::field>::type_t::BUNDLE_FIELD_TYPE_LIST,
        "options",
        false,
        nullptr,
        nullptr,
        &bundle::field::f_options,
        nullptr
    },
    {
        bundle_field<bundle::field>::type_t::BUNDLE_FIELD_TYPE_HTML,
        "description",
        true,
        &bundle::field::f_description,
        nullptr,
        nullptr,
        nullptr
    }
};




namespace
{

template<typename T>
bool load_dom(T * b, QDomElement e, typename bundle_field<T>::vector_t const & fields)
{
    snap::snap_string_list loaded;

    for(auto const & f : fields)
    {
        if(f.f_type == bundle_field<T>::type_t::BUNDLE_FIELD_TYPE_ATTRIBUTE)
        {
            QString const qattribute_name(QString::fromUtf8(f.f_name));
            if(loaded.contains(qattribute_name))
            {
                SNAP_LOG_ERROR("attribute ")(qattribute_name)("=\"...\" found more than once in this bundle");
                return false;
            }
            loaded << qattribute_name;

            if(e.hasAttribute(f.f_name))
            {
                QString const value(e.attribute(f.f_name));
                b->*f.f_data_string = value.toUtf8().data();
            }
        }
    }

    QDomElement n(e.firstChildElement());
    while(!n.isNull())
    {
        // verify that any one tag is loaded only once
        //
        QString const qtag_name(n.tagName());
        if(loaded.contains(qtag_name))
        {
            SNAP_LOG_ERROR("tag <")(qtag_name)("> (and/or attribute) found more than once in this bundle");
            return false;
        }
        loaded << qtag_name;

        // make sure we understand that tag
        //
        // TODO: the fields should be sorted and searched using a binary
        //       search which is way more efficient
        //
        std::string const tag_name(qtag_name.toUtf8().data());
        auto it(std::find_if(
                  fields.begin()
                , fields.end()
                , [tag_name](auto const & f)
                {
                    return tag_name == f.f_name;
                }));
        if(it == fields.end())
        {
            SNAP_LOG_ERROR("unknown tag <")(tag_name)("> found in XML bundle declaration");
            return false;
        }

        // retrieve the data and put it in our structure
        //
        switch(it->f_type)
        {
        case bundle_field<T>::type_t::BUNDLE_FIELD_TYPE_PLAIN:
            // plain text drops the newlines and shrinks the spaces
            //
            b->*it->f_data_string = n.text().simplified().toUtf8().data();
            break;

        case bundle_field<T>::type_t::BUNDLE_FIELD_TYPE_SCRIPT:
            // scripts must keep the newline in place or it's not
            // likely to work
            //
            b->*it->f_data_string = n.text().trimmed().toUtf8().data();
            break;

        case bundle_field<T>::type_t::BUNDLE_FIELD_TYPE_HTML:
            b->*it->f_data_string = boost::algorithm::trim_copy(std::string(snap::snap_dom::xml_children_to_string(n).toUtf8().data()));
            break;

        case bundle_field<T>::type_t::BUNDLE_FIELD_TYPE_LIST:
            {
                std::string const list(n.text().toUtf8().data());
                snap::tokenize_string<bundle::string_set_t>(b->*it->f_data_list, list, ",", true, " ");
            }
            break;

        case bundle_field<T>::type_t::BUNDLE_FIELD_TYPE_FLAG:
            b->*it->f_data_flag = true;
            break;

        case bundle_field<T>::type_t::BUNDLE_FIELD_TYPE_ATTRIBUTE:
            // attributes are handled in a previous loop
            //
            SNAP_LOG_ERROR("bundle attribute cannot be specified using a tag (\"")
                          (it->f_name)
                          ("\" is a tag).");
            return false;

        case bundle_field<T>::type_t::BUNDLE_FIELD_TYPE_FIELDS:
            // 'e' is a 'fields' tag
            //
            if(std::is_same<T, bundle>::value)
            {
                QDomElement m(n.firstChildElement());
                while(!m.isNull())
                {
                    bundle::field::pointer_t f(std::make_shared<bundle::field>());
                    if(!f->init(m))
                    {
                        return false;
                    }

                    // make sure we don't have two fields with the same name
                    //
                    auto ff(std::find_if(
                              (b->*it->f_data_fields).cbegin()
                            , (b->*it->f_data_fields).cend()
                            , [&f](auto const & ef)
                            {
                                return f->get_name() == ef->get_name();
                            }));
                    if(ff != (b->*it->f_data_fields).cend())
                    {
                        SNAP_LOG_ERROR("found two fields with the same name (\"")
                                      ((*ff)->get_name())
                                      ("\" is a tag).");
                        return false;
                    }

                    (b->*it->f_data_fields).push_back(f);

                    m = m.nextSiblingElement();
                }
            }
            break;

        }

        n = n.nextSiblingElement();
    }

    for(auto const & f : fields)
    {
        if(f.f_required
        && !loaded.contains(f.f_name))
        {
            // a required field is missing
            //
            SNAP_LOG_ERROR("bundle required field named \"")(f.f_name)("\" is missing.");
            return false;
        }
    }

    return true;
}



bool is_valid_status(char const * s)
{
    // pointer must not be null
    //
    if(s == nullptr)
    {
        return false;
    }

    // status is exactly one letter
    //
    if(s[0] == '\0'
    || s[1] != '\0')
    {
        return false;
    }

    switch(static_cast<bundle::bundle_status_t>(s[0]))
    {
    case bundle::bundle_status_t::BUNDLE_STATUS_UNKNOWN:
    case bundle::bundle_status_t::BUNDLE_STATUS_ERROR:
    case bundle::bundle_status_t::BUNDLE_STATUS_HIDE:
    case bundle::bundle_status_t::BUNDLE_STATUS_INSTALLED:
    case bundle::bundle_status_t::BUNDLE_STATUS_LOCKED:
    case bundle::bundle_status_t::BUNDLE_STATUS_NOT_INSTALLED:
    case bundle::bundle_status_t::BUNDLE_STATUS_PREREQ_MISSING:
    case bundle::bundle_status_t::BUNDLE_STATUS_IN_CONFLICT:
        return true;

    default:
        return false;

    }
}



}
// no name namespace







bool bundle::field::init(QDomElement e)
{
    std::string const tag_name(e.tagName().toUtf8().data());
    if(tag_name != "field")
    {
        SNAP_LOG_ERROR("unsupported tag <")(tag_name)("> within the <fields> tag; we only support <field> at this time.");
        return false;
    }
    return load_dom<bundle::field>(this, e, g_bundle_field_fields);
}


std::string const & bundle::field::get_name() const
{
    return f_name;
}


std::string const & bundle::field::get_type() const
{
    return f_type;
}


std::string const & bundle::field::get_label() const
{
    return f_label;
}


std::string const & bundle::field::get_initial_value() const
{
    return f_initial_value;
}


bundle::string_set_t const & bundle::field::get_options() const
{
    return f_options;
}


std::string const & bundle::field::get_description() const
{
    return f_description;
}










bundle::package::package(manager::pointer_t m, std::string const & name)
    : f_snap(m)
    , f_name(name)
{
    // TBD: should we check each character to make sure we have a valid name?
    //      (i.e. something like `[-a-z0-9_.~]*`)
    //
    if(f_name.empty())
    {
        throw snapmanager_exception_undefined("package was not given a valid name.");
    }
}


bool bundle::package::is_installed() const
{
    const_cast<bundle::package *>(this)->check_status();
    return f_status.compare(0, 20, "install ok installed") == 0;
}


std::string const & bundle::package::get_name() const
{
    return f_name;
}


std::string const & bundle::package::get_status() const
{
    const_cast<bundle::package *>(this)->check_status();
    return f_status;
}


std::string const & bundle::package::get_version() const
{
    const_cast<bundle::package *>(this)->check_status();
    return f_version;
}


void bundle::package::check_status()
{
    // already checked on this run?
    //
    if(!f_status.empty())
    {
        return;
    }

    // first try to load the status from our cache
    //
    std::string cache_file(f_snap->get_data_path().toUtf8().data());
    cache_file += "/bundle-package-status/" + f_name + ".status";

    time_t const now(time(nullptr));

    std::ifstream in;
    in.open(cache_file, std::ios_base::in | std::ios_base::binary);
    if(in.is_open())
    {
        // these files include 3 lines:
        //
        //  . Unix timestamp for freshness
        //  . Version
        //  . status as returned by dpkg-query (see below)
        //
        char buf[256];
        in.getline(buf, sizeof(buf));
        if(in.good())
        {
            time_t const timestamp(std::atol(buf));
            if(timestamp + PACKAGE_CACHE_FILE_LIFETIME >= now
            || !f_snap->is_daemon()) // ignore timestamp in snapmanager.cgi
            {
                // still fresh
                //
                in.getline(buf, sizeof(buf));
                if(in.good())
                {
                    // got version
                    //
                    f_version = buf;
                    in.getline(buf, sizeof(buf));
                    if(!in.fail())
                    {
                        // got status
                        //
                        f_status = buf;

                        // cache worked
                        //
                        return;
                    }
                }
            }
        }

        in.close();

        // the file is not considered good or is out of date, so delete it
        //
        unlink(cache_file.c_str());
    }

    if(!f_snap->is_daemon())
    {
        // in snapmanager.cgi we can't do anything more at this point
        // use defaults in the version/status info
        //
        // (i.e. pkg-query is too slow for snapmanager.cgi, especially
        // against 20+ packages)
        //
        f_version = "-";
        f_status = "unknown";
        return;
    }

    // the status is not yet available, run the pkg-query command
    //
    std::string output;
    int const r(f_snap->package_status(f_name, output));

    // defaults in case of failure
    //
    f_version = "-";
    f_status = output.empty() ? "unknown" : boost::algorithm::trim_copy(output);

    if(r == 0)
    {
        // success, parse the output in a "<version> <status>" pair
        //
        std::string::size_type const pos(output.find(' '));

        // if there is a version, then pos will be 1 or more
        //
        if(pos > 0
        && output.compare(pos + 1, 20, "install ok installed") == 0)
        {
            f_version = output.substr(0, pos);
            f_status = boost::algorithm::trim_copy(output.substr(pos + 1));
        }
    }

    // whatever result we got, create a corresponding cache file
    //
    // first make sure the parent directories exist
    //
    snap::mkdir_p(cache_file, true, 0755, "snapwebsites", "snapwebsites");
    std::ofstream out;
    out.open(cache_file, std::ios_base::out | std::ios_base::trunc | std::ios_base::binary);
    if(out.is_open())
    {
        out << now << std::endl;
        out << f_version << std::endl;
        out << f_status << std::endl;
    }
}

















bundle::bundle(std::shared_ptr<manager> m)
    : f_snap(m)
{
}


bool bundle::init(QDomDocument doc)
{
    return load_dom<bundle>(this, doc.documentElement(), g_bundle_fields);
}


std::string const & bundle::get_name() const
{
    return f_name;
}


std::string const & bundle::get_description() const
{
    return f_description;
}


bool bundle::get_hide() const
{
    return !f_hide.empty();
}


bool bundle::get_expected() const
{
    return f_expected;
}


bundle::string_set_t const & bundle::get_affected_services() const
{
    return f_affected_services;
}


bundle::string_set_t const & bundle::get_packages() const
{
    return f_packages;
}


bundle::string_set_t const & bundle::get_prereq() const
{
    return f_prereq;
}


bundle::string_set_t const & bundle::get_conflicts() const
{
    return f_conflicts;
}


bundle::string_set_t const & bundle::get_suggestions() const
{
    return f_suggestions;
}


std::string const & bundle::get_is_installed() const
{
    return f_is_installed;
}


std::string const & bundle::get_preinst() const
{
    return f_preinst;
}


std::string const & bundle::get_postinst() const
{
    return f_postinst;
}


std::string const & bundle::get_prerm() const
{
    return f_prerm;
}


std::string const & bundle::get_postrm() const
{
    return f_postrm;
}


bundle::field::vector_t const & bundle::get_fields() const
{
    return f_fields;
}


void bundle::add_prereq_bundle(pointer_t b)
{
    if(std::find(f_prereq_bundles.begin(), f_prereq_bundles.end(), b) == f_prereq_bundles.end())
    {
        f_prereq_bundles.push_back(b);
    }
}


bundle::weak_vector_t const & bundle::get_prereq_bundles() const
{
    return f_prereq_bundles;
}


void bundle::add_locked_by_bundle(pointer_t b)
{
    if(std::find(f_locked_by_bundles.begin(), f_locked_by_bundles.end(), b) == f_locked_by_bundles.end())
    {
        f_locked_by_bundles.push_back(b);
    }
}


bundle::weak_vector_t const & bundle::get_locked_by_bundles() const
{
    return f_locked_by_bundles;
}


void bundle::add_conflicts_bundle(pointer_t b)
{
    if(std::find(f_conflicts_bundles.begin(), f_conflicts_bundles.end(), b) == f_conflicts_bundles.end())
    {
        f_conflicts_bundles.push_back(b);
    }
}


bundle::weak_vector_t const & bundle::get_conflicts_bundles() const
{
    return f_conflicts_bundles;
}


void bundle::add_suggestions_bundle(pointer_t b)
{
    if(std::find(f_suggestions_bundles.begin(), f_suggestions_bundles.end(), b) == f_suggestions_bundles.end())
    {
        f_suggestions_bundles.push_back(b);
    }
}


bundle::weak_vector_t const & bundle::get_suggestions_bundles() const
{
    return f_suggestions_bundles;
}


bundle::package::pointer_t bundle::get_package(std::string const & name) const
{
    auto const it(f_package_status.find(name));
    if(it == f_package_status.cend())
    {
        // make sure it is a legal package name
        //
        auto p(std::find(f_packages.begin(), f_packages.end(), name));
        if(p == f_packages.end())
        {
            throw snapmanager_exception_undefined(
                          "package \""
                        + name
                        + "\" is not one defined in bundle \""
                        + f_name
                        + "\"."
                    );
        }

        // not available yet, create it
        //
        package::pointer_t pp(std::make_shared<package>(f_snap, name));
        f_package_status[name] = pp;
        return pp;
    }

    return it->second;
}


/** \brief Determine the current status of this bundle.
 *
 * A bundle has mainly two statuses: installed or not installed. However,
 * there are a few more things that we determine and the final status
 * is one of the following enumeration:
 *
 * \li BUNDLE_STATUS_UNKNOWN
 *
 * The status was not yet determined. This is the default. The determination
 * of the status can be costly so by default we do not try to determine it.
 * Calling this function will transform that status. The function should
 * never return BUNDLE_STATUS_UNKNOWN. Instead, if it cannot determine the
 * status, it returns BUNDLE_STATUS_ERROR.
 *
 * The status will be cached too. The Refresh button, among others, allows
 * for all the statuses to be recalculated by deleting the caches.
 *
 * \li BUNDLE_STATUS_ERROR
 *
 * The status could not be calculated. Maybe a package name is incorrect
 * or the is-installed script cause a problem. This status is rather
 * unusual.
 *
 * In the previous version, we loaded a bundle even when required fields
 * were missing and used this status as a result. Now we do not load
 * bundles that are not valid.
 *
 * \li BUNDLE_STATUS_HIDE
 *
 * This bundle was installed successfully and now will remain hidden.
 *
 * This is used for bundles you want users to install and never remove.
 * We mark the firewall that way. This can be annoying while testing,
 * though (i.e. to test reinstalling the firewall bundle over and over
 * again.)
 *
 * \li BUNDLE_STATUS_INSTALLED
 *
 * The bundle is installed and can be uninstalled if the user wishes so.
 *
 * To be considered installed, a bundle must have all of the packages
 * specified in its \<packages> field installed and if it has an
 * \<is-installed> script, that script must return "install ok installed".
 * There should be no other errors either.
 *
 * \li BUNDLE_STATUS_LOCKED
 *
 * The bundle is installed, but it cannot be uninstalled. A bundle
 * gets locked when another bundle that depends on it is also installed.
 *
 * For example, mysql gets locked on snaplog is installed because snaplog
 * requires mysql.
 *
 * To determine that a bundle is locked, the function first checks whether
 * the bundle is installed. If so, it checks the list of
 * f_locked_by_bundles and if one of them is marked as installed, then
 * it decides to mark this bundle as installed.
 *
 * \li BUNDLE_STATUS_NOT_INSTALLED
 *
 * This bundle is not considered installed. The user is given the option
 * to install this bundle.
 *
 * If the check used to know whether a bundle is installed determines that
 * one or more of the package is not installed or the \<is-installed>
 * script does not return "install ok installed", then the bundle is
 * considered uninstalled.
 *
 * \li BUNDLE_STATUS_PREREQ_MISSING
 *
 * This bundle is not installed. The user is not given the option to
 * install it, though, because one or more pre-requisite are not yet
 * installed.
 *
 * To determine whether a bundle can be installed, it first has to determine
 * that it is not installed. If that is the case, it further checks whether
 * it has any bundles in the f_prereq_bundles list which is not installed.
 * If so, then it is marked with this status.
 *
 * \li BUNDLE_STATUS_IN_CONFLICT
 *
 * This bundle cannot be installed because it is in conflict with another
 * bundle. This is a normal situation where we offer two distinct
 * bundles but one or the other should be installed, not both.
 *
 * A good example of this situation is with the snapmta and mailserver.
 * You want to install at least one computer with a full mailserver,
 * all the other computers can just get the snapmta.
 *
 * \return The status representing the current status of this bundle.
 */
bundle::bundle_status_t bundle::get_bundle_status() const
{
    // already defined? if so just return that cached value
    //
    if(f_bundle_status != bundle_status_t::BUNDLE_STATUS_UNKNOWN)
    {
        return f_bundle_status;
    }

    // check the cache first
    //
    std::string cache_file(f_snap->get_data_path().toUtf8().data());
    cache_file += "/bundle-status/" + f_name + ".status";

    time_t const now(time(nullptr));

    std::ifstream in;
    in.open(cache_file, std::ios_base::in | std::ios_base::binary);
    if(in.is_open())
    {
        // these files include 2 lines:
        //
        //  . Unix timestamp for freshness
        //  . status as our BUNDLE_STATUS_... letter
        //
        char buf[256];
        in.getline(buf, sizeof(buf));
        if(in.good())
        {
            time_t const timestamp(std::atol(buf));
            if(timestamp + BUNDLE_CACHE_FILE_LIFETIME >= now
            || !f_snap->is_daemon()) // ignore timestamp in snapmanager.cgi
            {
                // still fresh
                //
                in.getline(buf, sizeof(buf));
                if(in.good()
                && is_valid_status(buf)) // avoid tainted data
                {
                    // got status
                    //
                    f_bundle_status = static_cast<bundle::bundle_status_t>(buf[0]);

                    // cache worked
                    //
                    return f_bundle_status;
                }
            }
        }

        in.close();

        // the file is not considered good or is out of date, so delete it
        //
        unlink(cache_file.c_str());
    }

    // cache did not work, if we are in snapmanager.cgi we can't do anything
    // more here, so we have to return BUNDLE_STATUS_UNKNOWN
    //
    if(!f_snap->is_daemon())
    {
        return f_bundle_status;
    }

    // the following determines the status, the function returns as soon
    // as the new status is known, so it's easier to call it and then
    // act on the final status
    //
    determine_bundle_status();

    // whatever result we got, create a corresponding cache file
    //
    // first make sure the parent directories exist
    //
    snap::mkdir_p(cache_file, true, 0755, "snapwebsites", "snapwebsites");
    std::ofstream out;
    out.open(cache_file, std::ios_base::out | std::ios_base::trunc | std::ios_base::binary);
    if(out.is_open())
    {
        out << now << std::endl;
        out << static_cast<char>(f_bundle_status) << std::endl;
    }

    return f_bundle_status;
}


void bundle::determine_bundle_status() const
{
    // check each package
    //
    // by default we are considered installed unless one package
    // (or the <is-installed> script, see next block) says otherwise
    //
    bool installed(true);
    for(auto const & name : f_packages)
    {
        bundle::package::pointer_t p(get_package(name));
        if(!p->is_installed())
        {
            installed = false;

            // we can stop short since `installed` won't become
            // true again once set to false
            //
            break;
        }
    }

    // a bundle may define a script in <is-installed>
    // running that script determines whether the bundle is installed
    // or not on top of packages (especially useful if you do not include
    // packages in this bundle)
    //
    // note if we already know that some packages are not installed
    // there is no need to check anything more
    //
    if(installed
    && !f_is_installed.empty())
    {
        // get a filename using this bundle's name
        //
        std::string path(std::const_pointer_cast<manager>(f_snap)->get_cache_path().toUtf8().data());
        path += "/bundle-scripts/";
        path += f_name;
        path += ".is-installed";

        // create the script in a file
        //
        snap::file_content script(path, true);
        script.set_content(
                    std::string(
                            "#!/bin/bash\n"
                            "# auto-generated by snapmanagerdaemon\n"
                            "# from bundle ")
                    + f_name
                    + "\n"
                    + f_is_installed);
        script.write_all();
        chmod(path.c_str(), 0755);

        // run the script
        //
        snap::process p("is-installed");
        p.set_mode(snap::process::mode_t::PROCESS_MODE_OUTPUT);
        p.set_command(QString::fromUtf8(path.c_str()));
        int const r(p.run());
        if(r != 0)
        {
            // the script failed badly
            //
            int const e(errno);
            SNAP_LOG_ERROR("is-installed script of bundle \"")
                          (f_name)
                          ("\" failed with ")
                          (r)
                          (" (errno: ")
                          (e)
                          (", ")
                          (strerror(e))
                          (")");

            f_errors.push_back("Bundle \""
                            + f_name
                            + "\" includes an <is-installed> script to test whether it is installed."
                              " That script FAILED.");

            f_bundle_status = bundle_status_t::BUNDLE_STATUS_ERROR;
            return;
        }

        // the script worked, check the output which tells us whether
        // the bundle is considered installed or not
        //
        QString const output(p.get_output(true));
        if(output.trimmed() != "install ok installed")
        {
            installed = false;
        }
    }

    if(installed)
    {
        // "hide" has priority over installed/locked because if hidden
        // we just cannot uninstalled anyway
        //
        if(!f_hide.empty())
        {
            f_bundle_status = bundle_status_t::BUNDLE_STATUS_HIDE;
            return;
        }

        // so it is considered installed, set this state early
        // because the next loop calls get_bundle_status()
        // recursively
        //
        f_bundle_status = bundle_status_t::BUNDLE_STATUS_INSTALLED;

        // check whether another bundle locks this one
        //
        for(auto lock_by_bundle : f_locked_by_bundles)
        {
            bundle::pointer_t l(lock_by_bundle.lock());
            if(l != nullptr)
            {
                switch(l->get_bundle_status())
                {
                case bundle_status_t::BUNDLE_STATUS_HIDE:
                case bundle_status_t::BUNDLE_STATUS_INSTALLED:
                case bundle_status_t::BUNDLE_STATUS_LOCKED:
                    f_bundle_status = bundle_status_t::BUNDLE_STATUS_LOCKED;
                    return;

                default:
                    break;

                }
            }
        }

        return;
    }

    // this is the default status in this case, the function further
    // checks for conflicts and prereqs, but it is important to
    // define a type because the get_bundle_status() function gets
    // called recursively and may need to know the status of this
    // bundle without generating a loop. The fact that it's not
    // exactly the correct status is okay. It still works as intended.
    //
    f_bundle_status = bundle_status_t::BUNDLE_STATUS_NOT_INSTALLED;

    // the bundle is not installed yet, let's see whether it is in
    // conflict (which has priority over missing pre-requisites)
    //
    for(auto conflict : f_conflicts_bundles)
    {
        bundle::pointer_t l(conflict.lock());
        if(l != nullptr)
        {
            switch(l->get_bundle_status())
            {
            case bundle_status_t::BUNDLE_STATUS_HIDE:
            case bundle_status_t::BUNDLE_STATUS_INSTALLED:
            case bundle_status_t::BUNDLE_STATUS_LOCKED:
                f_bundle_status = bundle_status_t::BUNDLE_STATUS_IN_CONFLICT;
                return;

            default:
                break;

            }
        }
    }

    // if another bundle must be installed first, then our status
    // is going to be that a prereq is missing
    //
    for(auto prereq : f_prereq_bundles)
    {
        bundle::pointer_t l(prereq.lock());
        if(l != nullptr)
        {
            switch(l->get_bundle_status())
            {
            case bundle_status_t::BUNDLE_STATUS_NOT_INSTALLED:
            case bundle_status_t::BUNDLE_STATUS_PREREQ_MISSING:
                f_bundle_status = bundle_status_t::BUNDLE_STATUS_PREREQ_MISSING;
                return;

            case bundle_status_t::BUNDLE_STATUS_IN_CONFLICT:
                // this is a special case, if we depend on a bundle which
                // itself is in conflict with another bundle, then we are
                // also in conflict
                //
                // Say A depends on B, B is in conflict with C, C is
                // installed so the status of B is currently set to
                // "in-conflict", then A can also be marked as "in-conflict"
                //
                f_bundle_status = bundle_status_t::BUNDLE_STATUS_IN_CONFLICT;
                return;

            default:
                break;

            }
        }
    }
}








} // namespace snap_manager
// vim: ts=4 sw=4 et
