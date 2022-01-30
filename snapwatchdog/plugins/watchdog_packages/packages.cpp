// Snap Websites Server -- watchdog packages
// Copyright (c) 2013-2021  Made to Order Software Corp.  All Rights Reserved
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
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA


// self
//
#include "packages.h"


// snapwebsites lib
//
#include <snapwebsites/file_content.h>
#include <snapwebsites/glob_dir.h>
#include <snapwebsites/log.h>
#include <snapwebsites/process.h>
#include <snapwebsites/qdomhelpers.h>


// snapdev lib
//
#include <snapdev/not_used.h>


// Qt lib
//
#include <QFile>
#include <QRegExp>


// boost lib
//
#include <boost/algorithm/string.hpp>


// C++ lib
//
#include <sstream>


// last include
//
#include <snapdev/poison.h>




/** \file
 * \brief Verify that packages are installed, not installed, not in conflicts.
 *
 * This Snap! Watchdog plugin checks packages for:
 *
 * \li Packages that are expected to be installed (necessary for Snap! or
 *     enhance security)
 * \li Packages that should not be installed (security issues)
 * \li Packages that are in conflict (i.e. ntpd vs ntpdate)
 *
 * The plugin generates errors in all those situations.
 *
 * For example, if you have ntpd and ntpdate both installed on the
 * same system, they can interfere. Especially, the ntpd daemon may
 * not get restarted while ntpdate is running. If that happens
 * _simultaneously_, then the ntpd can't be restarted and the clock
 * is going to be allowed to drift.
 *
 * This watchdog plugin expects a list of XML files with definitions
 * of packages as defined above: required, unwanted, in conflict.
 * It is just too hard to make sure invalid installations won't ever
 * happen without help from the computer.
 */


SNAP_PLUGIN_START(packages, 1, 0)



/** \brief Get a fixed processes plugin name.
 *
 * The processes plugin makes use of different names. This function ensures
 * that you get the right spelling for a given name.
 *
 * \param[in] name  The name to retrieve.
 *
 * \return A pointer to the name.
 */
char const * get_name(name_t name)
{
    switch(name)
    {
    case name_t::SNAP_NAME_WATCHDOG_PACKAGES_CACHE_FILENAME:
        return "package-statuses.txt";

    case name_t::SNAP_NAME_WATCHDOG_PACKAGES_PATH:
        return "watchdog_packages_path";

    default:
        // invalid index
        throw snap_logic_exception("Invalid SNAP_NAME_WATCHDOG_PACKAGES_...");

    }
    snapdev::NOT_REACHED();
}




namespace
{



/** \brief Class used to read the list of packages to check.
 *
 * The class understands the following XML format:
 *
 * \code
 * <watchdog-packages>
 *   <package name="name" priority="15" installation="optional|required|unwanted">
 *      <description>...</description>
 *      <conflict>...</conflict>
 *   </package>
 *   <package name="..." ...>
 *      ...
 *   </package>
 *   ...
 * </watchdog-packages>
 * \endcode
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
class watchdog_package_t
{
public:
    typedef std::vector<watchdog_package_t>             vector_t;
    typedef std::set<std::string>                       package_name_set_t;
    typedef std::map<std::string, bool>                 installed_packages_t;

    enum class installation_t
    {
        PACKAGE_INSTALLATION_OPTIONAL,
        PACKAGE_INSTALLATION_REQUIRED,
        PACKAGE_INSTALLATION_UNWANTED
    };

                                watchdog_package_t(watchdog_child * snap, std::string const & name, installation_t installation, int priority);

    void                        set_description(std::string const & description);
    void                        add_conflict(std::string const & package_name);

    std::string const &         get_name() const;
    installation_t              get_installation() const;
    std::string                 get_installation_as_string() const;
    std::string const &         get_description() const;
    package_name_set_t const &  get_conflicts() const;
    package_name_set_t const &  get_packages_in_conflict() const;
    int                         get_priority() const;

    bool                        is_package_installed(std::string const & package_name);
    bool                        is_in_conflict();

    static installation_t       installation_from_string(std::string const & installation);

private:
    watchdog_child *            f_snap = nullptr;
    std::string                 f_name = std::string();
    std::string                 f_description = std::string();
    package_name_set_t          f_conflicts = package_name_set_t();
    package_name_set_t          f_in_conflict = package_name_set_t();
    installation_t              f_installation = installation_t::PACKAGE_INSTALLATION_OPTIONAL;
    int                         f_priority = 15;
};
#pragma GCC diagnostic pop

watchdog_package_t::vector_t                g_packages = watchdog_package_t::vector_t();
watchdog_package_t::installed_packages_t    g_installed_packages = watchdog_package_t::installed_packages_t();
bool                                        g_cache_loaded = false;
bool                                        g_cache_modified = false;


/** \brief Initializes a watchdog_package_t object.
 *
 * This function initializes the watchdog_package_t making it
 * ready to run the match() command.
 *
 * \note
 * The \p name is like a brief _description_ of the conflict.
 *
 * \param[in] snap  The pointer back to the watchdog_child object.
 * \param[in] name  The name of the package conflict.
 * \param[in] priority  The priority to use in case of conflict.
 */
watchdog_package_t::watchdog_package_t(watchdog_child * snap, std::string const & name, installation_t installation, int priority)
    : f_snap(snap)
    , f_name(name)
    , f_installation(installation)
    , f_priority(priority)
{
}


/** \brief Set the description of the expected package.
 *
 * This function saves the description of the conflict in more details than
 * its name.
 *
 * \param[in] description  The description of this conflict.
 */
void watchdog_package_t::set_description(std::string const & description)
{
    f_description = boost::algorithm::trim_copy(description);
}


/** \brief Add the name of an package in conflict with this package.
 *
 * This function takes the name of a package that is in conflict with
 * this package (i.e. see get_name().)
 *
 * Any number of packages in conflict can be added. The only restriction
 * is that a package cannot be in conflict with itself or its dependencies
 * although we do not check all the dependencies (too much work/too slow)
 * as we expect that you will create sensible XML definitions that do not
 * create impossible situations for your users.
 *
 * \param[in] package_name  The name of the package to seek.
 */
void watchdog_package_t::add_conflict(std::string const & package_name)
{
    if(package_name == f_name)
    {
        throw packages_exception_invalid_argument("a package cannot be in conflict with itself");
    }

    f_conflicts.insert(package_name);
}


/** \brief Get the name of the package concerned.
 *
 * This function returns the name of the package concerned by this
 * definition. This is the exact name of a Debian package.
 *
 * \return The name this package.
 */
std::string const & watchdog_package_t::get_name() const
{
    return f_name;
}


/** \brief Get the name of this conflict.
 *
 * This function returns the name of the package conflict. This is expected
 * to be a really brief description of the conflict so we know what is
 * being tested.
 *
 * \return The name this package conflict.
 */
watchdog_package_t::installation_t watchdog_package_t::get_installation() const
{
    return f_installation;
}


/** \brief Get the installation check as a string.
 *
 * This function transforms the installation_t of this package definition
 * in a string for display or saving to file.
 *
 * \return A string representing this package installation mode.
 */
std::string watchdog_package_t::get_installation_as_string() const
{
    switch(f_installation)
    {
    case installation_t::PACKAGE_INSTALLATION_REQUIRED:
        return "required";

    case installation_t::PACKAGE_INSTALLATION_UNWANTED:
        return "unwanted";

    //case installation_t::PACKAGE_INSTALLATION_OPTIONAL:
    default:
        return "optional";

    }
}


/** \brief Get the description of this conflict.
 *
 * This function returns the description of the package error. This
 * is used whenever an error message is generated, along the priority
 * and list of packages in conflict if any.
 *
 * \return The description to generate errors.
 */
std::string const & watchdog_package_t::get_description() const
{
    return f_description;
}


/** \brief Get the set of conflicts.
 *
 * This function returns the list of package names as defined by the
 * \<conflict> tag in the source XML files.
 *
 * When this package is installed and any one of the conflict package
 * is installed, then an error is generated.
 *
 * \note
 * To declare a package that should never be installed, in conflict or
 * not, you should instead use the "unwanted" installation type and
 * not mark it as in conflict of another package.
 *
 * \return The set of package names considered in conflict with this package.
 */
watchdog_package_t::package_name_set_t const & watchdog_package_t::get_conflicts() const
{
    return f_conflicts;
}


/** \brief Get the set of packages that are in conflict.
 *
 * This function returns a reference to the set of packages that are in
 * conflict as determined by the is_in_conflict() function. If the
 * is_in_conflict() function was not called yet, then this function
 * always returns an empty set. If the is_in_conflict() function was
 * called and it returned true, then this function always return a
 * set with at least one element.
 *
 * \return The set of package in conflict with the expected package.
 */
watchdog_package_t::package_name_set_t const & watchdog_package_t::get_packages_in_conflict() const
{
    return f_in_conflict;
}


/** \brief Get the priority of a package conflict object.
 *
 * Whenever an error occurs, the package conflict defines a priority
 * which is to be used with the error generator. This allows various
 * conflicts to be given various level of importance.
 *
 * The default priorty is 15.
 *
 *
 * Remember that to generate an email, the priority needs to be at least
 * 50. Any priority under 50 will still generate an error in the
 * snapmanager.cgi output.
 *
 * \return The priority of this package conflict.
 */
int watchdog_package_t::get_priority() const
{
    return f_priority;
}


/** \brief Check whether the specified package is installed.
 *
 * This function returns true if the named package is installed.
 *
 * \return true if the named package is installed.
 */
bool watchdog_package_t::is_package_installed(std::string const & package_name)
{
    bool result(false);

    if(!g_cache_loaded)
    {
        g_cache_loaded = true;

        QString const packages_filename(f_snap->get_cache_path(snap::packages::get_name(name_t::SNAP_NAME_WATCHDOG_PACKAGES_CACHE_FILENAME)));
        QFile in(packages_filename);
        if(in.open(QIODevice::ReadOnly))
        {
            char buf[256];
            while(in.readLine(buf, sizeof(buf) - 1) != -1)
            {
                buf[sizeof(buf) - 1] = '\0';

                char const * equal(strchr(buf, '='));
                if(equal != nullptr)
                {
                    std::string const name(buf, equal - buf);
                    bool value(equal[1] == 't');
                    g_installed_packages[name] = value;
                }
            }
        }
    }

    auto it(g_installed_packages.find(package_name));
    if(it != g_installed_packages.end())
    {
        // already determined, return the cached result
        //
        result = it->second;
    }
    else
    {
        // get the system status now
        //
        snap::process p("query package status");
        p.set_mode(snap::process::mode_t::PROCESS_MODE_OUTPUT);
        p.set_command("dpkg-query");
        p.add_argument("--showformat='${Status}'");
        p.add_argument("--show");
        p.add_argument(QString::fromUtf8(package_name.c_str()));
        int const r(p.run());

SNAP_LOG_TRACE("output of dpkg-query is: ")(r)(" -> ")(p.get_output(false));
        if(r == 0)
        {
            std::string output(p.get_output(true).toUtf8().data());

            // make sure to remove the '\n' and '\r' (and eventual spaces too)
            // from the start and end of the output
            //
            boost::algorithm::trim(output);

            // if there is a version, then pos will be 1 or more
            //
            result = output == "install ok installed";
SNAP_LOG_TRACE("output: [")(output)("] -> ")(result ? "TRUE" : "FALSE");
        }

        // cache the result in case the same package is checked multiple
        // times...
        //
        g_installed_packages[package_name] = result;

        g_cache_modified = true;
    }

    return result;
}


/** \brief Wether duplicate definitions are allowed or not.
 *
 * If a process is required by more than one package, then it should
 * be defined in each one of them and it should be marked as a
 * possible duplicate.
 *
 * For example, the mysqld service is required by snaplog and snaplistd.
 * Both will have a defintion for mysqld (because one could be installed
 * on a backend and the other on another backend.) However, when they
 * both get installed on the same machine, you get two definitions with
 * the same process name. If this function returns false for either one,
 * then the setup throws.
 *
 * \return true if the process definitions can have duplicates for that process.
 */
bool watchdog_package_t::is_in_conflict()
{
    // if the expected package is not even installed, there cannot be
    // a conflict because of this definition so ignore the list of
    // unexpected packages
    //
    if(!is_package_installed(f_name))
    {
        return false;
    }

    f_in_conflict.clear();
    for(auto u : f_conflicts)
    {
        if(is_package_installed(u))
        {
            // we are in conflict at least with this package
            //
            f_in_conflict.insert(u);
        }
    }

    return !f_in_conflict.empty();
}


/** \brief Transform a string in an installation type.
 *
 * This function converts a string in an initialization_t enumeration type.
 *
 * \exception packages_exception_invalid_argument
 * If the string is not recognized or empty, then this exception is raised.
 *
 * \note
 * The exception should never occur if the file gets checked for validity
 * with the XSD before it is used by Snap!
 *
 * \param[in] installation  The installation to convert to an enumeration.
 *
 * \return The corresponding installation type.
 */
watchdog_package_t::installation_t watchdog_package_t::installation_from_string(std::string const & installation)
{
    if(installation.empty()
    || installation == "optional")
    {
        return installation_t::PACKAGE_INSTALLATION_OPTIONAL;
    }
    if(installation == "required")
    {
        return installation_t::PACKAGE_INSTALLATION_REQUIRED;
    }
    if(installation == "unwanted")
    {
        return installation_t::PACKAGE_INSTALLATION_UNWANTED;
    }

    throw packages_exception_invalid_argument("invalid installation name, cannot load your XML file");
}



/** \brief Save the cache if it was updated.
 *
 * This function updates the cache used to save the installed packages
 * in the event it was modified (i.e. a new package was listed somewhere.)
 *
 * The cache is used so we can still check for the conflicts once a minute
 * but not spend the time it takes (very long) to check whether each package
 * is or is not installed.
 *
 * The cache gets reset once a day so it can be redefined anew at that time
 * and a new status determined.
 *
 * \param[in] snap  A pointer to the child so we can get the path to the cache.
 */
void save_cache(watchdog_child * snap)
{
    if(g_cache_modified)
    {
        QString const packages_filename(snap->get_cache_path(get_name(name_t::SNAP_NAME_WATCHDOG_PACKAGES_CACHE_FILENAME)));
        QFile out(packages_filename);
        if(out.open(QIODevice::WriteOnly))
        {
            for(auto const & p : g_installed_packages)
            {
                out.write(p.first.c_str(), p.first.length());
                out.putChar('=');
                out.putChar(p.second ? 't' : 'f');
                out.putChar('\n');
            }
        }
    }
}




}
// no name namespace



/** \brief Initialize the processes plugin.
 *
 * This function is used to initialize the processes plugin object.
 */
packages::packages()
{
}


/** \brief Clean up the processes plugin.
 *
 * Ensure the processes object is clean before it is gone.
 */
packages::~packages()
{
}


/** \brief Get a pointer to the processes plugin.
 *
 * This function returns an instance pointer to the processes plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the processes plugin.
 */
packages * packages::instance()
{
    return g_plugin_packages_factory.instance();
}


/** \brief Return the description of this plugin.
 *
 * This function returns the English description of this plugin.
 * The system presents that description when the user is offered to
 * install or uninstall a plugin on his website. Translation may be
 * available in the database.
 *
 * \return The description in a QString.
 */
QString packages::description() const
{
    return "Check whether a some required packages are missing,"
          " some installed packages are unwanted (may cause problems"
          " with running Snap! or are known security risks,)"
          " or packages that are in conflict.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString packages::dependencies() const
{
    return "|server|";
}


/** \brief Check whether updates are necessary.
 *
 * This function is ignored in the watchdog.
 *
 * \param[in] last_updated  The UTC Unix date when the website was last updated (in micro seconds).
 *
 * \return The UTC Unix date of the last update of this plugin.
 */
int64_t packages::do_update(int64_t last_updated)
{
    snapdev::NOT_USED(last_updated);
    SNAP_PLUGIN_UPDATE_INIT();
    // no updating in watchdog
    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Initialize packages.
 *
 * This function terminates the initialization of the packages plugin
 * by registering for various events.
 *
 * \param[in] snap  The child handling this request.
 */
void packages::bootstrap(snap_child * snap)
{
    f_snap = static_cast<watchdog_child *>(snap);

    SNAP_LISTEN(packages, "server", watchdog_server, process_watch, boost::placeholders::_1);
}


/** \brief Process this watchdog data.
 *
 * This function runs this watchdog.
 *
 * \param[in] doc  The document.
 */
void packages::on_process_watch(QDomDocument doc)
{
    SNAP_LOG_DEBUG("packages::on_process_watch(): processing");

    load_packages();

    QDomElement parent(snap_dom::create_element(doc, "watchdog"));
    QDomElement e(snap_dom::create_element(parent, "packages"));

SNAP_LOG_TRACE("got ")(g_packages.size())(" packages to check...");
    for(auto pc : g_packages)
    {
        QDomElement package(doc.createElement("package"));
        e.appendChild(package);

        QString const name(QString::fromUtf8(pc.get_name().c_str()));
        package.setAttribute("name", name);
        package.setAttribute("installation", QString::fromUtf8(pc.get_installation_as_string().c_str()));
        auto const & possible_conflicts(pc.get_conflicts());
        if(!possible_conflicts.empty())
        {
            package.setAttribute("conflicts", QString::fromUtf8(boost::algorithm::join(possible_conflicts, ", ").c_str()));
        }

        watchdog_package_t::installation_t const installation(pc.get_installation());
        switch(installation)
        {
        case watchdog_package_t::installation_t::PACKAGE_INSTALLATION_REQUIRED:
            if(!pc.is_package_installed(pc.get_name()))
            {
                // package is required, so it is in error if not installed
                //
                package.setAttribute("error", "missing");

                f_snap->append_error(
                          doc
                        , "packages"
                        , "The \""
                          + name
                          + "\" package is required but not (yet) installed."
                            " Please install this package at your earliest convenience."
                        , pc.get_priority());

                // continue for loop
                //
                continue;
            }
            break;

        case watchdog_package_t::installation_t::PACKAGE_INSTALLATION_UNWANTED:
            if(pc.is_package_installed(pc.get_name()))
            {
                // package is unwanted, so it should not be installed
                //
                package.setAttribute("error", "unwanted package is installed");

                f_snap->append_error(
                          doc
                        , "packages"
                        , "The \""
                          + name
                          + "\" package is expected to NOT ever be installed."
                            " Please remove this package at your earliest convenience."
                        , pc.get_priority());

                // continue for loop
                //
                continue;
            }
            break;

        // optional means that it may or may not be installed
        //case watchdog_package::installation_t::PACKAGE_INSTALLATION_OPTIONAL:
        default:
            break;

        }

        if(pc.is_in_conflict())
        {
            // conflict discovered, generate an error
            //
            auto const & conflicts(pc.get_packages_in_conflict());
            std::string const conflicts_list(boost::algorithm::join(conflicts, "\", \""));

            package.setAttribute("error", "package with conflicts");

            std::stringstream ss;

            ss << pc.get_description()
               << " The \""
               << pc.get_name()
               << "\" package is in conflict with \""
               << conflicts_list
               << "\".";

            f_snap->append_error(
                      doc
                    , "packages"
                    , QString(QString::fromUtf8(ss.str().c_str()))
                    , pc.get_priority());
        }
        // else -- everything's fine
    }

    // the cache may have been modified, save it if so
    //
    save_cache(f_snap);
}


/**\brief Load the list of watchdog packages.
 *
 * This function loads the XML from the watchdog and other packages that
 * define packages that are to be reported to the administrator.
 */
void packages::load_packages()
{
    g_packages.clear();

    // get the path to the packages XML files
    //
    QString packages_path(f_snap->get_server_parameter(get_name(name_t::SNAP_NAME_WATCHDOG_PACKAGES_PATH)));
    if(packages_path.isEmpty())
    {
        packages_path = "/usr/share/snapwebsites/snapwatchdog/packages";
    }
SNAP_LOG_TRACE("load package files from ")(packages_path)("...");

    // parse every XML file
    //
    glob_dir const script_filenames(packages_path + "/*.xml", GLOB_NOSORT | GLOB_NOESCAPE, true);
    script_filenames.enumerate_glob(std::bind(&packages::load_xml, this, std::placeholders::_1));
}


/** \brief Load a package XML file.
 *
 * This function loads one XML file and transform it in a
 * watchdog_package_t object.
 *
 * \exception packages_exception_invalid_name
 * This exception is raised if the name from a package is empty
 * or undefined.
 *
 * \param[in] package_filename  The name of an XML file representing
 *                              required, unwanted, or conflicted packages.
 */
void packages::load_xml(QString package_filename)
{
    QFile input(package_filename);
    if(input.open(QIODevice::ReadOnly))
    {
        QDomDocument doc;
        if(doc.setContent(&input, false)) // TODO: add error handling for debug
        {
            // we got the XML loaded
            //
            QDomNodeList packages_tags(doc.elementsByTagName("package"));
            int const max(packages_tags.size());
SNAP_LOG_TRACE("got XML from ")(package_filename)("... with ")(max)(" package definitions");
            for(int idx(0); idx < max; ++idx)
            {
                QDomNode p(packages_tags.at(idx));
                if(!p.isElement())
                {
                    continue;
                }
                QDomElement package(p.toElement());
                QString const name(package.attribute("name"));
                if(name.isEmpty())
                {
                    throw packages_exception_invalid_name("the name of a package cannot be the empty string or go undefined");
                }

                int priority(15);
                if(package.hasAttribute("priority"))
                {
                    bool ok(false);
                    priority = package.attribute("priority").toInt(&ok, 10);
                    if(!ok)
                    {
                        throw packages_exception_invalid_priority("the error priority of a package must be a valid decimal number");
                    }
                }

                watchdog_package_t::installation_t installation(watchdog_package_t::installation_t::PACKAGE_INSTALLATION_OPTIONAL);
                if(package.hasAttribute("installation"))
                {
                    installation = watchdog_package_t::installation_from_string(package.attribute("installation").toUtf8().data());
                }

                watchdog_package_t wp(f_snap, name.toUtf8().data(), installation, priority);

                QDomNodeList description_tags(package.elementsByTagName("description"));
                if(description_tags.size() > 0)
                {
                    QDomNode description_node(description_tags.at(0));
                    if(description_node.isElement())
                    {
                        QDomElement description(description_node.toElement());
                        wp.set_description(description.text().toUtf8().data());
                    }
                }

                QDomNodeList conflict_tags(package.elementsByTagName("conflict"));
                int const conflict_max(conflict_tags.size());
                for(int t(0); t < conflict_max; ++t)
                {
                    QDomNode conflict_node(conflict_tags.at(t));
                    if(conflict_node.isElement())
                    {
                        QDomElement conflict(conflict_node.toElement());
                        wp.add_conflict(conflict.text().toUtf8().data());
                    }
                }

                g_packages.push_back(wp);
            }
        }
    }
}


SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
