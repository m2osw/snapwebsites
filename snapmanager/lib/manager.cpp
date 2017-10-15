// Snap Websites Server -- snap manager CGI, daemon, library, plugins
// Copyright (C) 2016-2017  Made to Order Software Corp.
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

// ourselves
//
#include "snapmanager/manager.h"

// snapwebsites lib
//
#include <snapwebsites/addr.h>
#include <snapwebsites/glob_dir.h>
#include <snapwebsites/log.h>
#include <snapwebsites/mkdir_p.h>
#include <snapwebsites/not_reached.h>
#include <snapwebsites/not_used.h>
#include <snapwebsites/qstring_stream.h>

// C lib
//
#include <signal.h>

// C++ lib
//
#include <sstream>

// last entry
//
#include <snapwebsites/poison.h>


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


/** \brief List of configuration files one can create to define parameters.
 *
 * This feature is not used because the getopt does not yet give us a way
 * to specify a configuration file (i.e. --config \<path>/\<file>.conf).
 *
 * At this point, we load the configuration file using the snapwebsites
 * library.
 */
std::vector<std::string> const g_configuration_files
{
    //"@snapwebsites@",  // project name
    //"/etc/snapwebsites/snapmanager.conf" -- we use the snap f_config variable instead
};

advgetopt::getopt::option const g_manager_options[] =
{
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        nullptr,
        nullptr,
        "Usage: %p [-<opt>]",
        advgetopt::getopt::argument_mode_t::help_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        nullptr,
        nullptr,
        "where -<opt> is one or more of:",
        advgetopt::getopt::argument_mode_t::help_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "config",
        nullptr,
        "Path and filename of the snapmanager.cgi and snapmanagerdaemon configuration file.",
        advgetopt::getopt::argument_mode_t::required_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "data-path",
        "/var/lib/snapwebsites/cluster-status",
        "Path to this process data directory to save the cluster status.",
        advgetopt::getopt::argument_mode_t::required_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "debug",
        nullptr,
        "Start in debug mode.",
        advgetopt::getopt::argument_mode_t::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE | advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "log-config",
        "/etc/snapwebsites/logger/snapmanagerdaemon.properties",
        "Full path of log configuration file.",
        advgetopt::getopt::argument_mode_t::optional_argument
    },
    {
        'h',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "help",
        nullptr,
        "Show this help screen.",
        advgetopt::getopt::argument_mode_t::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "stylesheet",
        "/etc/snapwebsites/snapmanagercgi-parser.xsl",
        "The stylesheet to use to transform the data before sending it to the client as HTML.",
        advgetopt::getopt::argument_mode_t::required_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "version",
        nullptr,
        "Show the version of %p and exit.",
        advgetopt::getopt::argument_mode_t::no_argument
    },
    {
        '\0',
        0,
        nullptr,
        nullptr,
        nullptr,
        advgetopt::getopt::argument_mode_t::end_of_options
    }
};





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
        throw snap::snap_logic_exception("Invalid SNAP_NAME_MANAGER_...");

    }
    snap::NOTREACHED();
}




manager::manager(bool daemon)
    : snap_child(server_pointer_t())
    , f_daemon(daemon)
    , f_config("snapmanager")
{
}



manager::~manager()
{
}



/** \brief Initialize the manager.
 *
 * The constructor parses the command ilne options in a symetrical way
 * for snapmanager.cgi and snapmanagerdaemon.
 *
 * \param[in] argc  The number of arguments in argv.
 * \param[in] argv  The list of command line arguments.
 * \param[in] daemon  Whether the daemon (true) or CGI (false) process starts. 
 */
void manager::init(int argc, char * argv[])
{
    // Stop on these signals, log them, then terminate.
    //
    signal( SIGSEGV, sighandler );
    signal( SIGBUS,  sighandler );
    signal( SIGFPE,  sighandler );
    signal( SIGILL,  sighandler );
    signal( SIGTERM, sighandler );
    signal( SIGINT,  sighandler );
    signal( SIGQUIT, sighandler );

    // ignore console signals
    //
    signal( SIGTSTP,  SIG_IGN );
    signal( SIGTTIN,  SIG_IGN );
    signal( SIGTTOU,  SIG_IGN );

    g_instance = shared_from_this();

    // parse the arguments
    //
    f_opt.reset(new advgetopt::getopt(argc, argv, g_manager_options, g_configuration_files, "SNAPMANAGER_OPTIONS"));

    // --help
    //
    if(f_opt->is_defined("help"))
    {
        f_opt->usage(advgetopt::getopt::status_t::no_error, "Usage: %s -<arg> ...\n", argv[0]);
        exit(1);
    }

    // --version
    //
    if(f_opt->is_defined("version"))
    {
        std::cout << SNAPMANAGERCGI_VERSION_STRING << std::endl;
        exit(0);
    }

    // read the configuration file
    //
    if(f_opt->is_defined( "config"))
    {
        f_config.set_configuration_path(f_opt->get_string("config"));
    }

    // get the server name using the library function
    //
    f_server_name = QString::fromUtf8(snap::server::get_server_name().c_str());

    // --debug
    //
    f_debug = f_opt->is_defined("debug");

    // setup the logger
    // the definition in the configuration file has priority...
    //
    QString const log_config_filename(QString("log_config_%1").arg(f_daemon ? "daemon" : "cgi"));
    if(f_config.has_parameter(log_config_filename))
    {
        // use .conf definition when available
        f_log_conf = f_config[log_config_filename];
    }
    else
    {
        f_log_conf = QString::fromUtf8(f_opt->get_string("log-config").c_str());
    }
    snap::logging::configure_conffile( f_log_conf );

    if(f_debug)
    {
        // Force the logger level to DEBUG
        // (unless already lower)
        //
        snap::logging::reduce_log_output_level( snap::logging::log_level_t::LOG_LEVEL_DEBUG );
    }

    // make sure there are no standalone parameters
    //
    if( f_opt->is_defined( "--" ) )
    {
        std::cerr << "fatal error: unexpected parameter found on daemon command line." << std::endl;
        f_opt->usage(advgetopt::getopt::status_t::error, "Usage: %s -<arg> ...\n", argv[0]);
        snap::NOTREACHED();
    }

    // get the data path, we will be saving the status of each computer
    // in the cluster-status sub-directory and the bundles will be saved
    // under a sub-directory of that name.
    //
    f_data_path = "/var/lib/snapwebsites";
    if(f_config.has_parameter("data_path"))
    {
        // use .conf definition when available
        f_data_path = f_config["data_path"];
    }

    // create the cluster-status path
    //
    f_cluster_status_path = f_data_path + "/cluster-status";
    if(snap::mkdir_p(f_cluster_status_path, false) != 0)
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
    if(snap::mkdir_p(f_bundles_path, false) != 0)
    {
        std::stringstream msg;
        msg << "manager::init(): mkdir_p(...): process could not create bundles sub-directory \""
            << f_bundles_path
            << "\".";
        throw std::runtime_error(msg.str());
    }

    // get the user defined path to plugins if set
    //
    if(f_config.has_parameter("plugins_path"))
    {
        f_plugins_path = f_config["plugins_path"];
    }

    // get the user defined path to a folder used to cache data
    //
    if(f_config.has_parameter("cache_path"))
    {
        f_cache_path = f_config["cache_path"];
    }

    // get the path and filename to the apt-check tool
    //
    if(f_config.has_parameter("apt_check"))
    {
        f_apt_check = f_config["apt_check"];
    }

    // get the path and filename to the reboot-required flag
    //
    if(f_config.has_parameter("reboot_required"))
    {
        f_reboot_required = f_config["reboot_required"];
    }

    // get the path to a directory where we can create lock files
    //
    if(f_config.has_parameter("snapserver", "lock_path"))
    {
        f_lock_path = f_config(QString("snapserver"), "lock_path");
    }

    // If not defined, keep the default of localhost:4041
    // TODO: make these "just in time" parameters, we nearly never need them
    //
    if(f_config.has_parameter("snapcommunicator", "signal"))
    {
        snap_addr::addr const a(f_config("snapcommunicator", "signal"), f_signal_address, f_signal_port, "udp");
        f_signal_address = a.get_ipv4or6_string(false, false);
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


/** \brief Handle caught signals
 *
 * Catch the signal, then log the signal, then terminate with 1 status.
 */
void manager::sighandler( int sig )
{
    QString signame;
    bool output_stack_trace(true);
    switch( sig )
    {
        case SIGSEGV : signame = "SIGSEGV"; break;
        case SIGBUS  : signame = "SIGBUS";  break;
        case SIGFPE  : signame = "SIGFPE";  break;
        case SIGILL  : signame = "SIGILL";  break;
        case SIGTERM : signame = "SIGTERM"; output_stack_trace = false; break;
        case SIGINT  : signame = "SIGINT";  output_stack_trace = false; break;
        case SIGQUIT : signame = "SIGQUIT"; output_stack_trace = false; break;
        default      : signame = "UNKNOWN"; break;
    }

    if( output_stack_trace )
    {
        snap::snap_exception_base::output_stack_trace();
    }
    //
    SNAP_LOG_FATAL("signal caught: ")(signame);

    // is server available?
    if(g_instance)
    {
        g_instance->exit(1);
    }

    // server not available, exit directly
    ::exit(1);
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
    NOTUSED(snap);
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

        if(!snap::plugins::load(f_plugins_path, this, std::static_pointer_cast<snap::plugins::plugin>(g_instance), all_plugins))
        {
            throw snapmanager_exception_cannot_load_plugins("the snapmanager library could not load its plugins");
        }
    }
}


std::vector<std::string> manager::read_filenames(std::string const & pattern) const
{
    std::vector<std::string> result;

    try
    {
        snap::glob_dir files;
        files.set_path( pattern.c_str(), GLOB_NOESCAPE );
        files.enumerate_glob( [&]( QString the_path )
        {
            result.push_back(the_path.toUtf8().data());
        });
    }
    catch( std::exception const & x)
    {
        SNAP_LOG_ERROR("could not read \"")(pattern)("\" (what=")(x.what())(")!");
    }
    catch( ... )
    {
        SNAP_LOG_ERROR("could not read \"")(pattern)("\"!");
    }

    return result;
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
    std::string pattern(f_bundles_path.toUtf8().data());
    pattern += "/bundle-*.xml";
    return read_filenames(pattern);
}


QString const & manager::get_data_path() const
{
    return f_data_path;
}


QString const & manager::get_cache_path() const
{
    return f_cache_path;
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


void manager::forward_message(snap::snap_communicator_message const & message)
{
    snap::NOTUSED(message);
    throw std::logic_error("forward_message() called on the wrong object (i.e. it is not implemented.)");
}


int manager::get_version_major()
{
    return SNAPMANAGERCGI_VERSION_MAJOR;
}


int manager::get_version_minor()
{
    return SNAPMANAGERCGI_VERSION_MINOR;
}


int manager::get_version_patch()
{
    return SNAPMANAGERCGI_VERSION_PATCH;
}


char const * manager::get_version_string()
{
    return SNAPMANAGERCGI_VERSION_STRING;
}



} // namespace snap_manager
// vim: ts=4 sw=4 et
