// Snap Websites Server -- snap manager CGI, daemon, library, plugins
// Copyright (c) 2016-2020  Made to Order Software Corp.  All Rights Reserved
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
#include    "snapmanager/manager.h"


// snapwebsites
//
#include    <snapwebsites/snap_exception.h>


// snaplogger
//
#include    <snaplogger/exception.h>
#include    <snaplogger/message.h>
#include    <snaplogger/options.h>


// advgetopt
//
#include    <advgetopt/exception.h>


// snapdev
//
#include    <snapdev/glob_to_list.h>
#include    <snapdev/mkdir_p.h>
#include    <snapdev/not_reached.h>
#include    <snapdev/not_used.h>
#include    <snapdev/qstring_extensions.h>


// addr
//
#include    <libaddr/addr_parser.h>


// Qt
//
#include    <QFile>


// C++
//
#include    <sstream>


// C
//
#include    <signal.h>


// last include
//
#include    <snapdev/poison.h>



/** \file
 * \brief This file represents the Snap! Manager library.
 *
 * The snapmanagercgi, snapmanagerdaemon, and snapmanager-plugins are
 * all linked against this common library which adds some functionality
 * not otherwise available in the libsnapwebsites core library.
 */


/** \mainpage
 * \brief Snap! Manager Documentation
 *
 * \section introduction Introduction
 *
 * The Snap! Manager is a CGI, a daemon and a set of plugins that both
 * of these binaries use to allow for an infinite number of capabilities
 * in terms of managing a Snap! websites cluster.
 */



namespace snap_manager
{

namespace
{

manager::pointer_t g_instance;


///** \brief List of configuration files one can create to define parameters.
// *
// * This feature is not used because the getopt does not yet give us a way
// * to specify a configuration file (i.e. --config \<path>/\<file>.conf).
// *
// * At this point, we load the configuration file using the snapwebsites
// * library.
// */
//std::vector<std::string> const g_configuration_files
//{
//    //"@snapwebsites@",  // project name
//    //"/etc/snapwebsites/snapmanager.conf" -- we use the snap f_config variable instead
//};

advgetopt::option const g_manager_options[] =
{
    //{
    //    '\0',
    //    advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::GETOPT_FLAG_CONFIGURATION_FILE | advgetopt::GETOPT_FLAG_REQUIRED,
    //    "config",
    //    nullptr,
    //    "Path and filename of the snapmanager.cgi and snapmanagerdaemon configuration file.",
    //    nullptr
    //},
    {
        '\0',
        advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::GETOPT_FLAG_CONFIGURATION_FILE | advgetopt::GETOPT_FLAG_REQUIRED,
        "data-path",
        "/var/lib/snapwebsites/cluster-status",
        "Path to this process data directory to save the cluster status.",
        nullptr
    },
    //{
    //    '\0',
    //    advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::GETOPT_FLAG_CONFIGURATION_FILE | advgetopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
    //    "log-config",
    //    "/etc/snapwebsites/logger/snapmanagerdaemon.properties",
    //    "Full path of log configuration file.",
    //    nullptr
    //},
    {
        '\0',
        advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::GETOPT_FLAG_CONFIGURATION_FILE | advgetopt::GETOPT_FLAG_REQUIRED,
        "stylesheet",
        "/etc/snapwebsites/snapmanagercgi-parser.xsl",
        "The stylesheet to use to transform the data before sending it to the client as HTML.",
        nullptr
    },
    {
        '\0',
        advgetopt::GETOPT_FLAG_END,
        nullptr,
        nullptr,
        nullptr,
        nullptr
    }
};



// until we have C++20 remove warnings this way
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
advgetopt::options_environment const g_manager_options_environment =
{
    .f_project_name = "snapwebsites",
    .f_group_name = nullptr,
    .f_options = g_manager_options,
    .f_options_files_directory = nullptr,
    .f_environment_variable_name = "SNAPMANAGER_OPTIONS",
    .f_environment_variable_intro = nullptr,
    .f_section_variables_name = nullptr,
    .f_configuration_files = nullptr,
    .f_configuration_filename = nullptr,
    .f_configuration_directories = nullptr,
    .f_environment_flags = advgetopt::GETOPT_ENVIRONMENT_FLAG_PROCESS_SYSTEM_PARAMETERS,
    .f_help_header = "Usage: %p [-<opt>]\n"
                     "where -<opt> is one or more of:",
    .f_help_footer = "%c",
    .f_version = SNAPMANAGER_VERSION_STRING,
    .f_license = "GNU GPL v2",
    .f_copyright = "Copyright (c) 2013-"
                   BOOST_PP_STRINGIZE(UTC_BUILD_YEAR)
                   " by Made to Order Software Corporation -- All Rights Reserved",
    //.f_build_date = UTC_BUILD_DATE,
    //.f_build_time = UTC_BUILD_TIME
};
#pragma GCC diagnostic pop




} // no name namespace


/** \brief Get a fixed watchdog plugin name.
 *
 * The watchdog plugin makes use of different fixed names. This function
 * ensures that you always get the right spelling for a given name.
 *
 * \param[in] name  The name to retrieve.
 *
 * \return A pointer to the name.
 */
char const * get_name(name_t name)
{
    switch(name)
    {
    case name_t::SNAP_NAME_MANAGER_STATUS_FILE_HEADER:
        return "header";

    default:
        // invalid index
        throw snap::snap_logic_error("Invalid SNAP_NAME_MANAGER_...");

    }
    snapdev::NOT_REACHED();
}







/** \brief Initialize the manager.
 *
 * The constructor parses the command ilne options in a symetrical way
 * for snapmanager.cgi and snapmanagerdaemon.
 *
 * \param[in] daemon  Whether the daemon (true) or CGI (false) process starts. 
 * \param[in] argc  The number of arguments in argv.
 * \param[in] argv  The list of command line arguments.
 */
void manager::init(bool daemon, int argc, char * argv[])
{
    f_daemon = daemon;

    f_signal_handler = ed::signal_handler::create_instance(
                              ed::signal_handler::EXTENDED_SIGNAL_TERMINAL);

    f_config = advgetopt::conf_file::get_conf_file(advgetopt::conf_file_setup("/etc/snapwebsites/snapmanager.conf"));

    g_instance = shared_from_this();

    // parse the arguments
    //
    f_opt = std::make_shared<advgetopt::getopt>(g_manager_options_environment);
    snaplogger::add_logger_options(*f_opt);
    f_opt->finish_parsing(argc, argv);
    if(!snaplogger::process_logger_options(*f_opt))
    {
        // exit on any error
        throw advgetopt::getopt_exit("logger options generated an error.", 0);
    }

    // get the server name using the library function
    //
    f_server_name = snap::server::get_server_name().c_str();

    // setup the logger -- the new logger does not yet have such a feature I think
    // the definition in the configuration file has priority...
    //
    //QString const log_config_filename(QString("log_config_%1").arg(f_daemon ? "daemon" : "cgi"));
    //if(f_config.has_parameter(log_config_filename))
    //{
    //    // use .conf definition when available
    //    f_log_conf = f_config[log_config_filename];
    //}
    //else
    //{
    //    f_log_conf = f_opt->get_string("log-config"));
    //}
    //snap::logging::configure_conffile( f_log_conf );

    // make sure there are no standalone parameters
    //
    if(f_opt->is_defined("--"))
    {
        std::cerr << "fatal error: unexpected parameter found on daemon command line." << std::endl;
        std::cerr << f_opt->usage(advgetopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR);
        exit(1);
    }

    // get the data path, we will be saving the status of each computer
    // in the cluster-status sub-directory and the bundles will be saved
    // under a sub-directory of that name.
    //
    f_data_path = "/var/lib/snapwebsites";
    if(f_config->has_parameter("data_path"))
    {
        // use .conf definition when available
        f_data_path = f_config->get_parameter("data_path");
    }

    // create the cluster-status path
    //
    f_cluster_status_path = f_data_path + "/cluster-status";
    if(snapdev::mkdir_p(f_cluster_status_path, false) != 0)
    {
        std::stringstream msg;
        msg << "manager::init(): mkdir_p(...): process could not create cluster-status sub-directory \""
            << f_cluster_status_path
            << "\".";
        throw std::runtime_error(msg.str());
    }

    // create the bundles path
    //
    f_bundles_path = f_data_path + "/bundles";
    if(snapdev::mkdir_p(f_bundles_path, false) != 0)
    {
        std::stringstream msg;
        msg << "manager::init(): mkdir_p(...): process could not create bundles sub-directory \""
            << f_bundles_path
            << "\".";
        throw std::runtime_error(msg.str());
    }

    // get the user defined path to plugins if set
    //
    if(f_config->has_parameter("plugins_path"))
    {
        f_plugins_path = f_config->get_parameter("plugins_path");
    }

    // get the user defined path to a folder used to cache data
    //
    if(f_config->has_parameter("cache_path"))
    {
        f_cache_path = f_config->get_parameter("cache_path");
    }

    // get the user defined path to a folder used to cache data
    //
    if(f_config->has_parameter("www_cache_path"))
    {
        f_www_cache_path = f_config->get_parameter("www_cache_path");
    }

    // get the path and filename to the apt-check tool
    //
    if(f_config->has_parameter("apt_check"))
    {
        f_apt_check = f_config->get_parameter("apt_check");
    }

    // get the path and filename to the reboot-required flag
    //
    if(f_config->has_parameter("reboot_required"))
    {
        f_reboot_required = f_config->get_parameter("reboot_required");
    }

    // get the path to a directory where we can create lock files
    //
    advgetopt::conf_file::pointer_t snapserver_config =
            advgetopt::conf_file::get_conf_file(advgetopt::conf_file_setup("/etc/snapwebsites/snapserver.conf"));
    if(snapserver_config->has_parameter("lock_path"))
    {
        f_lock_path = snapserver_config->get_parameter("lock_path");
    }

    // If not defined, keep the default of localhost:4041
    // TODO: make these "just in time" parameters, we nearly never need them
    //
    advgetopt::conf_file::pointer_t snapcommunicator_config =
            advgetopt::conf_file::get_conf_file(advgetopt::conf_file_setup("/etc/snapwebsites/snapserver.conf"));
    if(snapcommunicator_config->has_parameter("signal"))
    {
        addr::addr const a(addr::string_to_addr(
                  snapcommunicator_config->get_parameter("signal")
                , f_signal_address
                , f_signal_port
                , "udp"));
        f_signal_address = a.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_ONLY);
        f_signal_port = a.get_port();
    }
}


/** \brief Retrieve a pointer to the watchdog server.
 *
 * This function retrieve an instance pointer of the watchdog server.
 * If the instance does not exist yet, then it gets created. A
 * server is also a plugin which is named "server".
 *
 * \note
 * In the snapserver this function is static. Here it is useless...
 *
 * \return A manager pointer to the watchdog server.
 */
manager::pointer_t manager::instance()
{
    return g_instance;
}


QString manager::description() const
{
    return "Main manager plugin (\"server\")";
}


QString manager::dependencies() const
{
    return QString();
}


void manager::bootstrap(snap_child * snap)
{
    // virtual function stub
    snapdev::NOT_USED(snap);
}


bool manager::is_daemon() const
{
    return f_daemon;
}


std::string manager::server_type() const
{
    return "manager";
}


void manager::load_plugins()
{
    if(!f_plugins_loaded)
    {
        f_plugins_loaded = true;

        // we always want to load all the plugins
        //
        snap::snap_string_list all_plugins(snap::plugins::list_all(f_plugins_path));

        // the list_all() includes "server", but we cannot load the server
        // plugin (it's ourselves)
        //
        all_plugins.removeOne("server");

        if(!snap::plugins::load(f_plugins_path, this, std::static_pointer_cast<snap::plugins::plugin>(g_instance), all_plugins, QString()))
        {
            SNAP_LOG_FATAL
                << "loading of all the snapmanager plugins failed."
                << SNAP_LOG_SEND;
            throw snapmanager_exception_cannot_load_plugins("the snapmanager library could not load its plugins");
        }
    }
}


std::vector<std::string> manager::read_filenames(std::string const & pattern) const
{
    try
    {
        snapdev::glob_to_list<std::vector<std::string>> glob;
        if(glob.read_path<
              snapdev::glob_to_list_flag_t::GLOB_FLAG_NO_ESCAPE
            , snapdev::glob_to_list_flag_t::GLOB_FLAG_IGNORE_ERRORS>(pattern))
        {
            return glob;
        }
    }
    catch(std::exception const & x)
    {
        SNAP_LOG_ERROR
            << "could not read \""
            << pattern
            << "\" (what="
            << x.what()
            << ")!"
            << SNAP_LOG_SEND;
    }
    catch( ... )
    {
        SNAP_LOG_ERROR
            << "could not read \""
            << pattern
            << "\"!"
            << SNAP_LOG_SEND;
    }

    return std::vector<std::string>();
}


std::vector<std::string> manager::get_list_of_servers()
{
    std::string pattern(f_cluster_status_path.toUtf8().data());
    pattern += "/*.db";
    return read_filenames(pattern);
}


QString const & manager::get_server_name() const
{
    return f_server_name;
}


QString const & manager::get_public_ip() const
{
    return f_public_ip;
}


std::string const & manager::get_signal_address() const
{
    return f_signal_address;
}


int manager::get_signal_port() const
{
    return f_signal_port;
}


snap::snap_string_list const & manager::get_snapmanager_frontend() const
{
    // TODO: implement this one by reading the list of frontend names
    //       from "snapmanager.conf" config file in variable named
    //       "snapmanager_frontend" -- it is used in snapmanagerdaemon
    //       but I don't think it works 100% yet
    //
    static snap::snap_string_list empty;
    return empty;
}


std::string manager::get_parameter(std::string const & name) const
{
    return f_config[name];
}


std::vector<std::string> const & manager::get_bundle_uri() const
{
    return f_bundle_uri;
}


std::vector<std::string> manager::get_list_of_bundles() const
{
    std::string pattern(f_bundles_path);
    pattern += "/bundle-*.xml";
    return read_filenames(pattern);
}


bundle::vector_t manager::load_bundles()
{
    bundle::vector_t result;

    // get the list of bundle filenames
    //
    std::vector<std::string> bundle_list(get_list_of_bundles());

    // load  each bundle XML file and parse it
    //
    for(auto const & filename : bundle_list)
    {
        QDomDocument bundle_xml;
        QFile input(QString::fromUtf8(filename.c_str()));
        if(input.open(QIODevice::ReadOnly)
        && bundle_xml.setContent(&input, false))
        {
            bundle::pointer_t b(std::make_shared<bundle>(shared_from_this()));
            if(b->init(bundle_xml))
            {
                // bundle could be loaded successfully
                //
                auto it(std::find_if(
                          result.begin()
                        , result.end()
                        , [&b](auto const & eb)
                        {
                            return eb->get_name() == b->get_name();
                        }));
                if(it != result.end())
                {
                    SNAP_LOG_ERROR
                        << "bundle named \""
                        << b->get_name()
                        << "\" found twice, the second time was in \""
                        << filename
                        << "\"."
                        << SNAP_LOG_SEND;
                    return bundle::vector_t();
                }
                else
                {
                    result.push_back(b);
                }
            }
        }
        else
        {
            // got an error loading the XML file, possibly because a
            // tag is not closed correctly, etc.
            //
            SNAP_LOG_ERROR
                << "could not load bundle file \""
                << filename
                << "\". Check the file with xmllint and try again."
                << SNAP_LOG_SEND;
            return bundle::vector_t();
        }
    }

    for(auto b : result)
    {
        // transform prereq names to pointers
        //
        bundle::string_set_t const & prereq(b->get_prereq());
        for(auto p : prereq)
        {
            auto it(std::find_if(
                      result.begin()
                    , result.end()
                    , [&p](auto const & r)
                    {
                        return p == r->get_name();
                    }));
            if(it == result.end())
            {
                // this can happen if you do not have all the necessary
                // 3rd party bundles...
                //
                SNAP_LOG_WARNING
                    << "bundle \""
                    << b->get_name()
                    << "\" references a missing 'prereq' bundle named \""
                    << p
                    << "\"."
                    << SNAP_LOG_SEND;
            }
            else if(*it == b)
            {
                // need to install yourself to be able to install yourselves?
                //
                // TODO: actually check the whole chain, if A depends on B
                //       which depends on C and C depends on A, we've got
                //       a really bad error
                //
                SNAP_LOG_ERROR
                    << "you cannot be in a prereq of yourself (\""
                    << p
                    << "\")."
                    << SNAP_LOG_SEND;
                return bundle::vector_t();
            }
            else
            {
                b->add_prereq_bundle(*it);
                (*it)->add_locked_by_bundle(b);
            }
        }

        // transform conflicts names to pointers
        //
        bundle::string_set_t const & conflicts(b->get_conflicts());
        for(auto c : conflicts)
        {
            auto it(std::find_if(
                      result.begin()
                    , result.end()
                    , [&c](auto const & r)
                    {
                        return c == r->get_name();
                    }));
            if(it == result.end())
            {
                // this can happen if you do not have all the necessary
                // 3rd party bundles...
                //
                SNAP_LOG_WARNING
                    << "bundle \""
                    << b->get_name()
                    << "\" references a missing 'conflicts' bundle named \""
                    << c
                    << "\"."
                    << SNAP_LOG_SEND;
            }
            else if(*it == b)
            {
                // in conflict with yourselves?
                //
                SNAP_LOG_ERROR
                    << "you cannot be in conflict with yourself (\""
                    << c
                    << "\")."
                    << SNAP_LOG_SEND;
                return bundle::vector_t();
            }
            else
            {
                b->add_conflicts_bundle(*it);
                (*it)->add_conflicts_bundle(b);
            }
        }

        // also transform the suggestions in links, that way we can see whether
        // some of those suggestions are broken
        //
        bundle::string_set_t const & suggestions(b->get_suggestions());
        for(auto s : suggestions)
        {
            auto it(std::find_if(
                      result.begin()
                    , result.end()
                    , [&s](auto const & r)
                    {
                        return s == r->get_name();
                    }));
            if(it == result.end())
            {
                // this is not acceptable, prevent all bundles from being
                // added so the programmer notices quickly
                //
                SNAP_LOG_ERROR
                    << "missing suggested bundle \""
                    << s
                    << "\"."
                    << SNAP_LOG_SEND;
                return bundle::vector_t();
            }
            if(*it == b)
            {
                // suggesting yourself?
                //
                SNAP_LOG_ERROR
                    << "you cannot suggest \""
                    << s
                    << "\" to itself."
                    << SNAP_LOG_SEND;
                return bundle::vector_t();
            }
            b->add_suggestions_bundle(*it);
        }
    }

    return result;
}


QString const & manager::get_data_path() const
{
    return f_data_path;
}


QString const & manager::get_cache_path() const
{
    return f_cache_path;
}


QString const & manager::get_www_cache_path() const
{
    return f_www_cache_path;
}


QString const & manager::get_bundles_path() const
{
    return f_bundles_path;
}


QString const & manager::get_reboot_required_path() const
{
    return f_reboot_required;
}


bool manager::stop_now_prima() const
{
    return false;
}


void manager::forward_message(ed::message const & message)
{
    snapdev::NOT_USED(message);
    throw std::logic_error("forward_message() called on the wrong object (i.e. it is not implemented.)");
}


int manager::get_version_major()
{
    return SNAPMANAGER_VERSION_MAJOR;
}


int manager::get_version_minor()
{
    return SNAPMANAGER_VERSION_MINOR;
}


int manager::get_version_patch()
{
    return SNAPMANAGER_VERSION_PATCH;
}


char const * manager::get_version_string()
{
    return SNAPMANAGER_VERSION_STRING;
}



} // namespace snap_manager
// vim: ts=4 sw=4 et
