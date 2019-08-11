// Snap Websites Server -- snap websites CGI function
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

// at this point this is just a passwthrough process, at some point we may
// want to implement a (complex) cache system that works here

//
// The following is a sample environment from Apache2.
//
// # arguments
// argv[0] = "/usr/clients/www/alexis.m2osw.com/cgi-bin/env_n_args.cgi"
//
// # See also: http://www.cgi101.com/book/ch3/text.html
//
// # environment
// UNIQUE_ID=VjAW4H8AAAEAAC7d0YIAAAAE
// SCRIPT_URL=/images/finball/20130711-lightning-by-Karl-Gehring.png
// SCRIPT_URI=http://csnap.m2osw.com/images/finball/20130711-lightning-by-Karl-Gehring.png
// CLEAN_SNAP_URL=1
// HTTP_HOST=csnap.m2osw.com
// HTTP_USER_AGENT=Mozilla/5.0 (X11; Linux i686 on x86_64; rv:41.0) Gecko/20100101 Firefox/41.0 SeaMonkey/2.38
// HTTP_ACCEPT=image/png,image/*;q=0.8,*/*;q=0.5
// HTTP_ACCEPT_LANGUAGE=en-US,en;q=0.8,fr-FR;q=0.5,fr;q=0.3
// HTTP_ACCEPT_ENCODING=gzip, deflate
// HTTP_REFERER=http://csnap.m2osw.com/css/finball/finball_0.0.127.min.css
// HTTP_COOKIE=cookieconsent_dismissed=yes; xUVt9AD6G4xKO_AU=036d371e8c10f340/2034695214
// HTTP_CONNECTION=keep-alive
// HTTP_CACHE_CONTROL=max-age=0
// PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
// SERVER_SIGNATURE=
// SERVER_SOFTWARE=Apache
// SERVER_NAME=csnap.m2osw.com
// SERVER_ADDR=162.226.130.121
// SERVER_PORT=80
// REMOTE_HOST=halk.m2osw.com
// REMOTE_ADDR=162.226.130.121
// DOCUMENT_ROOT=/usr/clients/www/csnap.m2osw.com/public_html/
// REQUEST_SCHEME=http
// CONTEXT_PREFIX=/cgi-bin/
// CONTEXT_DOCUMENT_ROOT=/usr/clients/www/csnap.m2osw.com/cgi-bin/
// SERVER_ADMIN=webmaster@m2osw.com
// SCRIPT_FILENAME=/usr/clients/www/csnap.m2osw.com/cgi-bin/snap.cgi
// REMOTE_PORT=51596
// GATEWAY_INTERFACE=CGI/1.1
// SERVER_PROTOCOL=HTTP/1.1
// REQUEST_METHOD=GET
// QUERY_STRING=
// REQUEST_URI=/images/finball/20130711-lightning-by-Karl-Gehring.png
// SCRIPT_NAME=/cgi-bin/snap.cgi
//

// self (server version)
//
#include "version.h"

// snapwebsites lib
//
#include <snapwebsites/log.h>
#include <snapwebsites/mkdir_p.h>
#include <snapwebsites/snap_uri.h>
#include <snapwebsites/snapwebsites.h>
#include <snapwebsites/tcp_client_server.h>


// snapdev lib
//
#include <snapdev/not_reached.h>


// libaddr lib
//
#include <libaddr/addr_parser.h>


// advgetopt lib
//
#include <advgetopt/advgetopt.h>


// boost lib
//
#include <boost/algorithm/string.hpp>


// C++ lib
//
#include <fstream>


// last include
//
#include <snapdev/poison.h>




// avoid leak detection from the -fsanitize option
// (who cares, we run then exit right away)
//
extern "C" {
char const * __asan_default_options()
{
    return "detect_leaks=0";
}
}




namespace
{


// WARNING: Do not forget that the snap.cgi does NOT accept command line
//          options, not even --version or --help; these are dangerous
//          in a CGI so we only have a few options we support from the
//          configuration file.
//
const advgetopt::option g_snapcgi_options[] =
{
    {
        '\0',
        advgetopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::GETOPT_FLAG_CONFIGURATION_FILE | advgetopt::GETOPT_FLAG_REQUIRED,
        "snapserver",
        nullptr,
        "IP address on which the snapserver is running, it may include a port (i.e. 192.168.0.1:4004)",
        nullptr
    },
    {
        '\0',
        advgetopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::GETOPT_FLAG_CONFIGURATION_FILE | advgetopt::GETOPT_FLAG_REQUIRED,
        "log-config",
        "/etc/snapwebsites/logger/snapcgi.properties",
        "Full path of log configuration file",
        nullptr
    },
    {
        '\0',
        advgetopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::GETOPT_FLAG_CONFIGURATION_FILE | advgetopt::GETOPT_FLAG_REQUIRED,
        "permanent-cache-path",
        nullptr,
        "Define a path to a folder were permanent files are saved while caching a page. Usually under /var/lib.",
        nullptr
    },
    {
        '\0',
        advgetopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::GETOPT_FLAG_CONFIGURATION_FILE | advgetopt::GETOPT_FLAG_REQUIRED,
        "temporary-cache-path",
        nullptr,
        "Define a path to a folder were temporary files are saved while attempting to cache a page. This could be under /run.",
        nullptr
    },
    {
        '\0',
        advgetopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::GETOPT_FLAG_CONFIGURATION_FILE | advgetopt::GETOPT_FLAG_REQUIRED,
        "use-ssl",
        nullptr,
        "Whether SSL should be used to connect to snapserver. Set to \"true\" or \"false\".",
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


constexpr char const * const g_configuration_files[]
{
    "/etc/snapwebsites/snapcgi.conf",
    nullptr
};




// until we have C++20 remove warnings this way
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
advgetopt::options_environment const g_snapcgi_options_environment =
{
    .f_project_name = "snapwebsites",
    .f_options = g_snapcgi_options,
    .f_options_files_directory = nullptr,
    .f_environment_variable_name = "SNAPCGI_OPTIONS",
    .f_configuration_files = g_configuration_files,
    .f_configuration_filename = nullptr,
    .f_configuration_directories = nullptr,
    .f_environment_flags = 0,
    .f_help_header = nullptr,
    .f_help_footer = nullptr,
    .f_version = SNAPWEBSITES_VERSION_STRING,
    .f_license = nullptr,
    .f_copyright = nullptr,
    //.f_build_date = UTC_BUILD_DATE,
    //.f_build_time = UTC_BUILD_TIME
};
#pragma GCC diagnostic pop




}
// no name namespace


class snap_cgi
{
public:
                        snap_cgi( int argc, char * argv[] );
                        ~snap_cgi();

    int                 error(char const * code, char const * msg, char const * details);
    bool                verify();
    int                 process();

private:
    typedef std::map<std::string, std::string>      field_map_t;
    enum class cache_state_t
    {
        CACHE_STATE_FIELD_NAME,          // text up to space or ':', if '\n' header end reached
        CACHE_STATE_FIELD_SEPARATOR,     // the ':' (accept spaces too)
        CACHE_STATE_FIELD_DATA_START,    // skip spaces until something else appears
        CACHE_STATE_FIELD_DATA,          // field data
        CACHE_STATE_FIELD_CONTINUE,      // '\n' found in field data, if '\t' or space(s) continue field data otherwise new FIELD_NAME

        CACHE_STATE_FIELD_CACHE,         // we found a header that tells us caching is possible
        CACHE_STATE_FIELD_NO_CACHE       // we found a header that tells us no cache can be created by snap.cgi
    };

    int                 check_permanent_cache();
    void                cache_data(char const * data, size_t size);
    void                check_headers();
    void                temporary_to_permanent_cache();
    int                 delete_cache_file(std::string const & filename);
    int                 rename_cache_file(std::string const & from_filename, std::string const & to_filename);

    advgetopt::getopt   f_opt;
    int                 f_port = 4004;                              // snap server port
    std::string         f_address = std::string("127.0.0.1");       // snap server address
    std::vector<char>   f_cache = {};   // to save outgoing data to see whether to cache it on disk or not
    cache_state_t       f_cache_state = cache_state_t::CACHE_STATE_FIELD_NAME;
    size_t              f_cache_pos = 0;
    std::string         f_cache_field_name = std::string();
    std::string         f_cache_field_data = std::string();
    field_map_t         f_cache_fields = {};
    std::string         f_cache_permanent_filename = std::string();
    std::string         f_cache_temporary_filename = std::string();
    std::shared_ptr<std::ofstream>
                        f_cache_file = std::shared_ptr<std::ofstream>();
    snap::cache_control_settings
                        f_client_ccs = snap::cache_control_settings();
};


snap_cgi::snap_cgi( int argc, char * argv[] )
    : f_opt(g_snapcgi_options_environment)
{
    snap::NOTUSED(argc);

    f_opt.parse_program_name(argv);
    f_opt.parse_configuration_files();
    f_opt.parse_environment_variable();
    // -- no parsing of the command line arguments, it's too dangerous in a CGI --

    // most requests are under 64Kb, larger ones are often images, JS, CSS
    // files that we want to cache if allowed
    //
    f_cache.reserve(64 * 1024);

    // max-age defaults to 0 which is not correct for client's cache
    // information (although with the current cache implementation here
    // it works the same as IGNORE_VALUE, later versions may change)
    //
    f_client_ccs.set_max_age(snap::cache_control_settings::IGNORE_VALUE);

    // read log-config and setup the logger
    //
    std::string logconfig(f_opt.get_string("log-config"));
    snap::logging::configure_conffile( logconfig.c_str() );
}


snap_cgi::~snap_cgi()
{
}


int snap_cgi::error(char const * code, char const * msg, char const * details)
{
    if(details == nullptr)
    {
        details = "No details.";
    }

    SNAP_LOG_ERROR("error(\"")(code)("\", \"")(msg)("\", \"")(details)("\")");

    std::string body("<h1>");
    body += code;
    body += "</h1><p>";
    body += (msg == nullptr ? "Sorry! We found an invalid server configuration or some other error occurred." : msg);
    body += "</p>";

    std::cout   << "Status: " << code                       << std::endl
                << "Expires: Sun, 19 Nov 1978 05:00:00 GMT" << std::endl
                << "Connection: close"                      << std::endl
                << "Content-Type: text/html; charset=utf-8" << std::endl
                << "Content-Length: " << body.length()      << std::endl
                << "X-Powered-By: snap.cgi"                 << std::endl
                << std::endl
                << body
                ;

    return 1;
}


bool snap_cgi::verify()
{
    // If not defined, keep the default of 127.0.0.1:4004
    if(f_opt.is_defined("snapserver"))
    {
        addr::addr a(addr::string_to_addr(f_opt.get_string("snapserver"), f_address, f_port, "tcp"));
        f_address = a.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_ONLY);
        f_port = a.get_port();
    }

    // catch "invalid" methods early so we do not waste
    // any time with methods we do not support at all
    //
    // later we want to add support for PUT, PATCH and DELETE though
    {
        // WARNING: do not use std::string because nullptr will crash
        //
        char const * request_method(getenv("REQUEST_METHOD"));
        if(request_method == nullptr)
        {
            SNAP_LOG_FATAL("Request method is not defined.");
            std::string body("<html><head><title>Method Not Defined</title></head><body><p>Sorry. We only support GET, HEAD, and POST.</p></body></html>");
            std::cout   << "Status: 405 Method Not Defined"         << std::endl
                        << "Expires: Sat, 1 Jan 2000 00:00:00 GMT"  << std::endl
                        << "Allow: GET, HEAD, POST"                 << std::endl
                        << "Connection: close"                      << std::endl
                        << "Content-Type: text/html; charset=utf-8" << std::endl
                        << "Content-Length: " << body.length()      << std::endl
                        << "X-Powered-By: snap.cgi"                 << std::endl
                        << std::endl
                        << body;
            return false;
        }
        if(strcmp(request_method, "GET") != 0
        && strcmp(request_method, "HEAD") != 0
        && strcmp(request_method, "POST") != 0)
        {
            SNAP_LOG_FATAL("Request method is \"")(request_method)("\", which we currently refuse.");
            if(strcmp(request_method, "BREW") == 0)
            {
                // see http://tools.ietf.org/html/rfc2324
                std::cout << "Status: 418 I'm a teapot" << std::endl;
            }
            else
            {
                std::cout << "Status: 405 Method Not Allowed" << std::endl;
            }
            //
            std::string body("<html><head><title>Method Not Allowed</title></head><body><p>Sorry. We only support GET, HEAD, and POST.</p></body></html>");
            std::cout   << "Expires: Sat, 1 Jan 2000 00:00:00 GMT"  << std::endl
                        << "Allow: GET, HEAD, POST"                 << std::endl
                        << "Connection: close"                      << std::endl
                        << "Content-Type: text/html; charset=utf-8" << std::endl
                        << "Content-Length: " << body.length()      << std::endl
                        << "X-Powered-By: snap.cgi"                 << std::endl
                        << std::endl
                        << body;
            return false;
        }
    }

    // catch "invalid" protocols early so we do not waste
    // any time with protocols we do not support at all
    //
    {
        // WARNING: do not use std::string because nullptr will crash
        //
        char const * server_protocol(getenv("SERVER_PROTOCOL"));
        if(server_protocol == nullptr)
        {
            // Frankly this should never happen here, Apache2 should refuse
            // such early on.
            //
            error("400 Bad Request", nullptr, "The SERVER_PROTOCOL parameter is not available.");
            return false;
        }
        if(server_protocol[0] != 'H'
        || server_protocol[1] != 'T'
        || server_protocol[2] != 'T'
        || server_protocol[3] != 'P'
        || server_protocol[4] != '/')
        {
            // Again, I would hope that Apache refuses anything that does not
            // say HTTP in the server protocol without sending it to us
            //
            error(
                "400 Bad Request",
                "We only support the HTTP protocol.",
                ("Unexpected protocol in \"" + std::string(server_protocol) + "\", not supported.").c_str());
            return false;
        }
        // we only support "[0-9].[0-9]" at the moment
        //
        if((server_protocol[5] < '0' || server_protocol[5] > '9')
        || server_protocol[6] != '.'
        || (server_protocol[7] < '0' || server_protocol[7] > '9')
        || server_protocol[8] != '\0')
        {
            // Again, I would hope that Apache refuses anything that does not
            // say HTTP in the server protocol without sending it to us
            //
            error(
                "400 Bad Request",
                "Protocol must be followed by a valid version.",
                ("Unexpected protocol version in \"" + std::string(server_protocol) + "\", not supported.").c_str());
            return false;
        }
        uint32_t const version(((server_protocol[5] - '0') << 16) + server_protocol[7] - '0');
        switch(version)
        {
        case 0x00010000:
        case 0x00010001:
            // we understand those
            break;

        default:
            // In this case, Apache may let it through... we only
            // support version 1.0 and 1.1 at the moment
            //
            error(
                "400 Bad Request",
                "Protocol version not supported.",
                ("Protocol version is not 1.0 or 1.1, \"" + std::string(server_protocol) + "\" is not supported.").c_str());
            return false;

        }
    }

    char const * remote_addr(getenv("REMOTE_ADDR"));
    if(remote_addr == nullptr)
    {
        error("400 Bad Request", nullptr, "The REMOTE_ADDR parameter is not available.");
        return false;
    }

    {
        // WARNING: do not use std::string because nullptr will crash
        //
        char const * http_host(getenv("HTTP_HOST"));
        if(http_host == nullptr)
        {
            error("400 Bad Request", "The host you want to connect to must be specified.", nullptr);
            return false;
        }
#ifdef _DEBUG
        //SNAP_LOG_DEBUG("HTTP_HOST=")(http_host);
#endif
        if(tcp_client_server::is_ipv4(http_host))
        {
            SNAP_LOG_ERROR("The host cannot be an IPv4 address.");
            std::cout   << "Status: 444 No Response" << std::endl
                        << "Connection: close" << std::endl
                        << "X-Powered-By: snap.cgi" << std::endl
                        << std::endl
                        ;
            snap::server::block_ip(remote_addr, "week", "user tried to access snap.cgi with a bare IPv4 adress");
            return false;
        }
        if(tcp_client_server::is_ipv6(http_host))
        {
            SNAP_LOG_ERROR("The host cannot be an IPv6 address.");
            std::cout   << "Status: 444 No Response" << std::endl
                        << "Connection: close" << std::endl
                        << "X-Powered-By: snap.cgi" << std::endl
                        << std::endl
                        ;
            snap::server::block_ip(remote_addr, "week", "user tried to access snap.cgi with a bare IPv6 adress");
            return false;
        }
    }

    {
        // WARNING: do not use std::string because nullptr will crash
        //
        char const * request_uri(getenv(snap::get_name(snap::name_t::SNAP_NAME_CORE_REQUEST_URI)));
        if(request_uri == nullptr)
        {
            // this should NEVER happen because without a path after the method
            // we probably do not have our snap.cgi run anyway...
            //
            error("400 Bad Request", "The path to the page you want to read must be specified.", nullptr);
            return false;
        }
#ifdef _DEBUG
        //SNAP_LOG_DEBUG("REQUEST_URI=")(request_uri);
#endif

        // if we receive this, someone tried to directly access our snap.cgi
        // which will not work right so better err immediately
        //
        if(strncasecmp(request_uri, "/cgi-bin/", 9) == 0)
        {
            error("404 Page Not Found", "We could not find the page you were looking for.", "The REQUEST_URI cannot start with \"/cgi-bin/\".");
            snap::server::block_ip(remote_addr, QString(), "user tried to access \"/cgi-bin/\" through snap.cgi");
            return false;
        }

        // if we receive this, someone is trying to log in through the XMLRPC
        // interface, but ours uses a different URL
        //
        if(strncasecmp(request_uri, "/xmlrpc.php", 11) == 0)
        {
            error("404 Page Not Found", "We could not find the page you were looking for.", "Our XMLRPC is not under /xmlrpc.php, wrong REQUEST_URI.");
            snap::server::block_ip(remote_addr, "year", "user tried to access \"/xmlrpc.php\" through snap.cgi");
            return false;
        }

        // We do not allow any kind of proxy
        //
        if(*request_uri != '/')
        {
            // avoid proxy accesses
            std::string msg("The REQUEST_URI cannot represent a proxy access (");
            msg += request_uri;
            msg += ").";
            error("404 Page Not Found", nullptr, msg.c_str());
            snap::server::block_ip(remote_addr, "year", "user tried to access snap.cgi with a proxy access");
            return false;
        }

        // TODO: move to snapserver because this could be the name of a legal page...
        if(strcasestr(request_uri, "phpmyadmin") != nullptr)
        {
            // block myPhpAdmin accessors
            error("410 Gone", "MySQL left.", nullptr);
            snap::server::block_ip(remote_addr, "year", "user is trying to access phpmyadmin through snap.cgi");
            return false;
        }

        // TODO: move to snapserver because this could be the name of a legal page...
        if(strcasestr(request_uri, "GponForm/diag_Form?images") != nullptr)
        {
            // block CVE-2018-10561 accessors
            error("410 Gone", "You were nearly logged in.", nullptr);
            snap::server::block_ip(remote_addr, "year", "user is trying to access GPON router");
            return false;
        }

        // TODO: move to snapserver because this could be the name of a legal page...
        if(strcasestr(request_uri, "wp-login.php") != nullptr)
        {
            // block attempt to login as if we were a WordPress site
            error("410 Gone", "Form not found.", nullptr);
            snap::server::block_ip(remote_addr, "year", "user is trying to log in as if this was a WordPress website");
            return false;
        }

        // TODO: move to snapserver because this could be the name of a legal page...
        if(strcasestr(request_uri, "w00tw00t") != nullptr)
        {
            // block attempt to login as if we were a WordPress site
            error("410 Gone", "Form not found.", nullptr);
            snap::server::block_ip(remote_addr, "year", "w00tw00t scanner detected.");
            return false;
        }
    }

    {
        // WARNING: do not use std::string because nullptr will crash
        //
        char const * user_agent(getenv(snap::get_name(snap::name_t::SNAP_NAME_CORE_HTTP_USER_AGENT)));
        if(user_agent == nullptr)
        {
            // the Agent: ... field is required
            //
            error("400 Bad Request", "The accessing agent must be specified.", nullptr);
            snap::server::block_ip(remote_addr, "month", "the User-Agent header is mandatory");
            return false;
        }
#ifdef _DEBUG
        //SNAP_LOG_DEBUG("HTTP_USER_AGENT=")(request_uri);
#endif

        // left trim
        while(isspace(*user_agent))
        {
            ++user_agent;
        }

        // if we receive this, someone tried to directly access our
        // snap.cgi, which will not work right so better err immediately
        //
        if(*user_agent == '\0'
        || (*user_agent == '-' && user_agent[1] == '\0')
        || strcasestr(user_agent, "ZmEu") != nullptr
        || strcasestr(user_agent, "libwww-perl") != nullptr)
        {
            // note that we consider "-" as empty for this test
            error("400 Bad Request", nullptr, "The agent string cannot be empty.");
            snap::server::block_ip(remote_addr, "month", "the User-Agent header is empty or \"-\", represents ZmEu or libwww-perl, which are all not allowed");
            return false;
        }
    }

    // success
    return true;
}


int snap_cgi::process()
{
    // WARNING: do not use std::string because nullptr will crash
    char const * request_method( getenv("REQUEST_METHOD") );
    if(request_method == nullptr)
    {
        // the method was already checked in verify(), before this
        // call so it should always be defined here...
        //
        SNAP_LOG_FATAL("Method not defined in REQUEST_METHOD.");
        std::string body("<html><head><title>Method Not Defined</title></head><body><p>Sorry. We only support GET, HEAD, and POST.</p></body></html>");
        std::cout   << "Status: 405 Method Not Defined"         << std::endl
                    << "Expires: Sat, 1 Jan 2000 00:00:00 GMT"  << std::endl
                    << "Connection: close"                      << std::endl
                    << "Allow: GET, HEAD, POST"                 << std::endl
                    << "Content-Type: text/html; charset=utf-8" << std::endl
                    << "Content-Length: " << body.length()      << std::endl
                    << "X-Powered-By: snap.cgi"                 << std::endl
                    << std::endl
                    << body;
        return false;
    }

    // check whether the user set use-ssl to false, if so we want to use
    // a plain connection to snapserver
    //
    bool secure(true);
    if(f_opt.is_defined("use-ssl"))
    {
        std::string const & use_ssl(f_opt.get_string("use-ssl"));
        if(use_ssl == "false")
        {
            secure = false;
        }
        else if(use_ssl != "true")
        {
            SNAP_LOG_WARNING("\"use_ssl\" parameter is set to unknown value \"")(use_ssl)("\". Using \"true\" instead.");
        }
    }
    if(secure
    && f_address == "127.0.0.1")
    {
        // avoid SSL if we are connecting locally ("lo" interface is secure)
        //
        secure = false;
    }

#ifdef _DEBUG
    SNAP_LOG_DEBUG("processing request_method=")(request_method)
                  (" request_uri=")(getenv("REQUEST_URI"));
#endif
    SNAP_LOG_DEBUG("f_address=")(f_address)(", f_port=")(f_port)(", secure=")(secure ? "true" : "false");

    {
        sigset_t set;
        sigemptyset(&set);
        sigaddset(&set, SIGPIPE);
        sigprocmask(SIG_BLOCK, &set, nullptr);
    }

    if(check_permanent_cache() == 0)
    {
        // it succeeded, we returned the cache data, no need to go further
        // (and we avoided hitting the snapserver / cassandra combo!)
        //
        return 0;
    }
    f_cache_fields.clear();
    if(f_client_ccs.get_only_if_cached())
    {
        // the client wanted something from the cache and avoid hitting
        // the server, we can't do that so we have to reply with a 504
        //
        return error("504 Gateway Timeout"
                   , "This page is not currently cached. Please verify that the URI is valid."
                   , "The user request included the \"Cache-Control: only-if-cached\" parameter.");
    }
    if(f_cache_permanent_filename.empty())
    {
        f_cache_state = cache_state_t::CACHE_STATE_FIELD_NO_CACHE;
    }

    tcp_client_server::bio_client socket(
                  f_address
                , f_port
                , secure
                        ? tcp_client_server::bio_client::mode_t::MODE_SECURE
                        : tcp_client_server::bio_client::mode_t::MODE_PLAIN);

    std::string var;
    auto send_data([this, &var, &socket, request_method]()
        {
#ifdef _DEBUG
            SNAP_LOG_DEBUG("writing #START=" SNAPWEBSITES_VERSION_STRING);
#endif

#define START_COMMAND "#START=" SNAPWEBSITES_VERSION_STRING
            if(socket.write(START_COMMAND "\n", sizeof(START_COMMAND)) != sizeof(START_COMMAND))
#undef START_COMMAND
            {
#ifdef _DEBUG
                SNAP_LOG_DEBUG("socket.write() of START_COMMAND failed!");
#endif
                return 1;
            }
            for(char ** e(environ); *e; ++e)
            {
                std::string env(*e);
                int const len(static_cast<int>(env.size()));

                // Prevent the HTTP_PROXY variable from going through, although
                // apparently Apache2 prevents such on its own but at this point
                // it is not clear to me whether it really does or not.
                //
                // (see https://httpoxy.org/)
                //
                if(static_cast<size_t>(len) >= sizeof("HTTP_PROXY")) // sizeof() includes the '\0', we test the '=' below
                {
                    // env.startsWith("HTTP_PROXY=")?
                    if(env[ 0] == 'H'
                    && env[ 1] == 'T'
                    && env[ 2] == 'T'
                    && env[ 3] == 'P'
                    && env[ 4] == '_'
                    && env[ 5] == 'P'
                    && env[ 6] == 'R'
                    && env[ 7] == 'O'
                    && env[ 8] == 'X'
                    && env[ 9] == 'Y'
                    && env[10] == '=')
                    {
                        continue;
                    }
                }

                //
                // Replacing all '\n' in the env variables to '|'
                // to prevent snap_child from complaining and die.
                //
                std::replace( env.begin(), env.end(), '\n', '|' );
#ifdef _DEBUG
                //SNAP_LOG_DEBUG("Writing environment '")(env.c_str())("', len=")(len);
#endif
                if(socket.write(env.c_str(), len) != len)
                {
#ifdef _DEBUG
                    SNAP_LOG_DEBUG("socket.write() of env failed!");
#endif
                    return 2;
                }
#ifdef _DEBUG
                //SNAP_LOG_DEBUG("Writing newline");
#endif
                if(socket.write("\n", 1) != 1)
                {
#ifdef _DEBUG
                    SNAP_LOG_DEBUG("socket.write() of '\\n' failed!");
#endif
                    return 3;
                }
            }
#ifdef _DEBUG
            //SNAP_LOG_DEBUG("Done with writing env");
#endif
            if( strcmp(request_method, "POST") == 0 )
            {
#ifdef _DEBUG
                SNAP_LOG_DEBUG("writing #POST");
#endif
                if(socket.write("#POST\n", 6) != 6)
                {
                    return 4;
                }
                // we also want to send the POST variables
                // http://httpd.apache.org/docs/2.4/howto/cgi.html
                // note that in case of a non-multipart post variables are separated
                // by & and the variable names and content cannot include the & since
                // that would break the whole scheme so we can safely break (add \n)
                // at that location
                char const * content_type(getenv("CONTENT_TYPE"));
                if(content_type == nullptr)
                {
                    return 5;
                }
                bool const is_multipart(QString(content_type).startsWith("multipart/form-data"));
                int const break_char(is_multipart ? '\n' : '&');
                for(;;)
                {
                    // TODO: handle potential read problems...
                    //       (here we read from Apache, our stdin, a pipe)
                    //
                    int const c(getchar());
                    if(c == break_char || c == EOF)
                    {
                        // Note: a POST without variables most often ends up with
                        //       one empty line
                        //
                        if(!is_multipart || c != EOF)
                        {
                            // WARNING: This \n MUST exist if the POST includes
                            //          a binary file!
                            var += "\n";
                        }
                        if(!var.empty())
                        {
                            if(socket.write(var.c_str(), var.length()) != static_cast<int>(var.length()))
                            {
                                return 6;
                            }
                        }
#ifdef _DEBUG
                        //SNAP_LOG_DEBUG("wrote var=")(var.c_str());
#endif
                        var.clear();
                        if(c == EOF)
                        {
                            // this was the last variable
                            break;
                        }
                    }
                    else
                    {
                        var += c;
                    }
                }
            }
#ifdef _DEBUG
            SNAP_LOG_DEBUG("writing #END");
#endif
            if(socket.write("#END\n", 5) != 5)
            {
                return 7;
            }

            return 0; // success
        });
    int const send_error(send_data());

    if(send_error == 5)
    {
        return error("500 Internal Server Error", "the CONTENT_TYPE variable was not defined along a POST.", nullptr);
    }

    if(send_error != 0)
    {
        SNAP_LOG_FATAL("Ready to send a 504 Gateway Timeout to client (")(send_error)(") but check for a reply from snapserver first...");

        // on error the server may have sent us a reply that we are
        // expected to send to the client
        //
    }

    // if we get here then we can just copy the output of the child to Apache2
    // the wait will flush all the writes as necessary
    //
    // XXX   buffer the entire data? it is definitively faster to pass it
    //       through as it comes in, but in order to be able to return an
    //       error instead of a broken page, we may want to consider
    //       buffering first?
    //
    int wrote(0);
    for(;;)
    {
        char buf[64 * 1024];
        int const r(socket.read(buf, sizeof(buf)));
        if(r > 0)
        {
#ifdef _DEBUG
            //SNAP_LOG_DEBUG("writing buf=")(buf);
#endif
            if(fwrite(buf, r, 1, stdout) != 1)
            {
                // there is not point in calling error() from here because
                // the connection is probably broken anyway, just report
                // the problem in to the logger
                //
                SNAP_LOG_FATAL("an I/O error occurred while sending the response to the client");
                return 1;
            }
            wrote += r;

            try
            {
                cache_data(buf, r);
            }
            catch(std::exception const & e)
            {
                SNAP_LOG_ERROR("cache_data() generated an exception: ")(e.what());
            }
            catch(...)
            {
                SNAP_LOG_ERROR("cache_data() generated an unknown exception.");
            }

// to get a "verbose" CGI (For debug purposes)
#if 0
            {
                FILE *f = fopen("/tmp/snap.output", "a");
                if(f != nullptr)
                {
                    fwrite(buf, r, 1, f);
                    fclose(f);
                }
            }
#endif
        }
        else if(r == -1)
        {
            int const e(errno);
            char const * request_uri(getenv(snap::get_name(snap::name_t::SNAP_NAME_CORE_REQUEST_URI)));
            SNAP_LOG_FATAL("an I/O error occurred while reading the response from the server, the REQUEST_URI was: ")
                          (request_uri)
                          (" (errno: ")
                          (e)
                          (" -- ")
                          (strerror(e))
                          (")");
            break;
        }
        else if(r == 0)
        {
            // normal exit
            break;
        }
    }

    // although the server was not happy with us, it may have sent us a
    // reply that we transmitted to the client (i.e. wrote != 0)
    //
    if(send_error != 0 && wrote == 0)
    {
        if(send_error == 6)
        {
            return error("504 Gateway Timeout", ("error while writing POST variable \"" + var + "\" to the child process.").c_str(), nullptr);
        }
        else
        {
            return error("504 Gateway Timeout", (std::string("error while writing to the child process (") + std::to_string(send_error) + ").").c_str(), nullptr);
        }
    }

    // so, everything worked, if we have a cache file, now is the time to
    // close it and save it in the cache area
    //
    // the f_cache_file pointer is set back to nullptr in case of failure
    //
    temporary_to_permanent_cache();

#ifdef _DEBUG
    SNAP_LOG_DEBUG("Closing connection...");
#endif
    return 0;
}


int snap_cgi::check_permanent_cache()
{
    // get the client's request cache info because this is important
    // to eventually skip the cached data, accept stale data, etc.
    //
    char const * const client_cache_control(getenv("HTTP_CACHE_CONTROL"));
    if(client_cache_control != nullptr
    && *client_cache_control != '\0')
    {
        f_client_ccs.set_cache_info(QString::fromUtf8(client_cache_control), false);

        // if the client requests a "no-store" in terms of caches, we
        // comply by not defining the permanent cache filename
        //
        // I'm not so sure this makes sense here, but I prefer to follow
        // the client's wishes for this one
        //
        if(f_client_ccs.get_no_store())
        {
            return -1;
        }
    }

    // our permanent cache is only for a GET
    //
    char const * const request_method(getenv("REQUEST_METHOD"));
    if(request_method == nullptr
    || strcmp(request_method, "GET") != 0)
    {
        // don't actually attempt to cache anything if method is not a GET
        //
        return -1;
    }

    // okay, we have a valid GET, let's see the request path
    //
    // WARNING: in reality, this cache MUST use a one to one parsing of the
    //          URI information as the snapserver does in snap_child; the
    //          following is a very rough approximation; very advanced
    //          features just won't work right with this cache that you
    //          should turn off by setting the permanent_cache_path to
    //          the empty string
    //

    snap::snap_uri uri;

    // protocol
    {
        char const * protocol("http");
        char const * const https(getenv("HTTPS"));
        if(https != nullptr)
        {
            if(strcmp(https, "on") == 0)
            {
                protocol = "https";
            }
        }
        uri.set_protocol(protocol);
    }

    // host
    {
        char const * const http_host(getenv("HTTP_HOST"));
        if(http_host == nullptr)
        {
            return -1;
        }
        std::string host;

        // the HTTP_HOST parameter may include the port after a colon
        // make sure to remove it otherwise snap_uri gets "confused"
        //
        char const * const port(strchr(http_host, ':'));
        if(port != nullptr)
        {
            // ignore port in the host part
            //
            host = std::string(http_host, port - http_host);
        }
        else
        {
            // no port, use full host info
            //
            host = http_host;
        }
        if(host.empty())
        {
            // this can probably not happen, but this is tainted data
            //
            return -1;
        }

        uri.set_domain(QString::fromUtf8(host.c_str()));
    }

    // port
    {
        char const * const port(getenv("SERVER_PORT"));
        if(port == nullptr)
        {
            // the port is mandatory, 80 and 443 are defaults but still
            // need to be specified by Apache
            //
            return -1;
        }
        uri.set_port(port);
    }

    // query string
    {
        char const * const query_string(getenv("QUERY_STRING"));
        if(query_string != nullptr)
        {
            uri.set_query_string(query_string);
        }
    }

    // path
    {
        char const * const request_uri(getenv("REQUEST_URI"));
        if(request_uri == nullptr)
        {
            return -1;
        }
        std::string path;
        char const * const query_string(strchr(request_uri, '?'));
        if(query_string != nullptr)
        {
            // there is a repeat of the query string in the REQUEST_URI
            //
            path = std::string(request_uri, query_string - request_uri);
        }
        else
        {
            // the whole URI is the path
            //
            path = request_uri;
        }
        if(path.length() > 2048)
        {
            return -1;
        }
        uri.set_path(QString::fromUtf8(path.c_str()));
    }

    // from the URI, calculate the permanent cache path and filename
    {
        QString canonicalized(uri.get_uri());

        // we want to change the protocol separators (://) to "_"
        // but the rest of the path has to remain as is so it can be
        // really long (i.e. 2048 is used above)
        //
        // WARNING: QString::replace() will replace all instances, we only
        //          want the first one to be converted
        //
        int const pos(canonicalized.indexOf("://"));
        if(pos > 0)
        {
            // replace just that one instance
            //
            canonicalized = canonicalized.mid(0, pos)
                          + "_"
                          + canonicalized.mid(pos + 3);
        }

        // the urlencode() function is good enough for us here and quite
        // sensible since it uses the same characters as what the browser
        // uses; also we leave a few characters alone as they can appear
        // as is in a filename anyway; especially, we keep all slashes
        // as is because filenames are limited to a length much smaller
        // than what a URI path can be in Snap! C++
        //
        canonicalized = snap::snap_uri::urlencode(canonicalized, ",/=~");

        // get the user defined path to the permanent folder
        //
        std::string permanent_cache_path("/var/lib/snapwebsites/www/permanent");
        if(f_opt.is_defined("permanent-cache-path"))
        {
            permanent_cache_path = f_opt.get_string("permanent-cache-path");
        }

        if(snap::mkdir_p(permanent_cache_path) != 0)
        {
            SNAP_LOG_WARNING("could not access the permanent cache path (")
                            (permanent_cache_path)
                            (")");
            return -1;
        }

        f_cache_permanent_filename = permanent_cache_path + "/" + canonicalized;
    }

    // the no-cache flag in a request is similar to a "must revalidate",
    // in our current implementation means we totally ignore our cache
    //
    if(f_client_ccs.get_no_cache())
    {
        return -1;
    }

    // does that file exist?
    //
    std::ifstream cp(f_cache_permanent_filename, std::ios_base::in | std::ios_base::binary);
    if(!cp.is_open())
    {
        // no cached file, all is fine
        //
        return -1;
    }

    // read the header
    //
    std::vector<std::string> lines;
    bool first(true);
    size_t offset(0);
    for(;;)
    {
        std::string line;
        for(;;)
        {
            char c;
            cp.get(c);
            if(cp.eof()
            || cp.fail())
            {
                // if we reach eof() before we can determine whether the
                // cached file is still valid, it's too late
                //
                delete_cache_file(f_cache_permanent_filename);
                return -1;
            }
            if(c == '\n')
            {
                break;
            }
            line += c;
        }
        if(line.empty())
        {
            // an empty line means end of header
            //
            break;
        }
        if(!first
        && (line[0] == ' ' || line[0] == '\t'))
        {
            // concatenate
            //
            size_t const max(line.length());
            size_t p(0);
            for(; p < max; ++p)
            {
                if(line[p] != ' '
                || line[p] != '\t')
                {
                    break;
                }
            }
            if(p < max)
            {
                size_t const last_line(lines.size() - 1);
                lines[last_line] += ' ';
                lines[last_line] += std::string(line.c_str() + p, max - p);
            }
        }
        else
        {
            if(first)
            {
                offset = line.length() + 1;
            }
            lines.push_back(line);
        }
        first = false;
    }

    // put the fields in the f_cache_fields map
    //
    for(auto l : lines)
    {
        std::string::size_type const pos(l.find(':'));
        if(pos > 0)
        {
            QString const name(QString::fromUtf8(l.substr(0, pos).c_str()).trimmed().toLower());
            QString const value(QString::fromUtf8(l.substr(pos + 1).c_str()).trimmed());
            f_cache_fields[name.toUtf8().data()] = value.toUtf8().data();
        }
    }

    // now search for the various fields that tell us whether we have
    // a valid cache for this request, we may have to return a 304 too
    //
    auto const snap_cgi_date(f_cache_fields.find("x-snap-cgi-date"));
    if(snap_cgi_date == f_cache_fields.end())
    {
        // this should not happen, we are managing our own cache and
        // handle this field specifically
        //
        SNAP_LOG_ERROR("missing X-Snap-CGI-Date field.");
        delete_cache_file(f_cache_permanent_filename);
        exit(1);
        return 0; // this is not correct so we can't return
    }

    time_t const date(std::stol(snap_cgi_date->second));
    if(date <= 0)
    {
        // this should not happen since we are managing the cache
        // and very specifically this date
        //
        SNAP_LOG_ERROR("invalid X-Snap-CGI-Date field (")
                      (snap_cgi_date->second)
                      (").");
        delete_cache_file(f_cache_permanent_filename);
        exit(1);
        return 0; // this is not correct so we can't return
    }

    // check for the Cache-Control to make sure the file is not out of date
    //
    auto const cache_control(f_cache_fields.find("cache-control"));
    if(cache_control == f_cache_fields.end())
    {
        // this should not happen, we don't save requests without a
        // Cache-Control field in our cache (i.e. because those are
        // viewed as private)
        //
        SNAP_LOG_ERROR("missing Cache-Control field.");
        delete_cache_file(f_cache_permanent_filename);
        exit(1);
        return 0; // this is not correct so we can't return
    }

    snap::cache_control_settings const ccs(QString::fromUtf8(cache_control->second.c_str()), false);

    int64_t max_age(ccs.get_s_maxage());
    if(-1 == max_age)
    {
        max_age = ccs.get_max_age();
    }

    // client may define a specific maximum age that will override the server
    // defined maximum age (now found in the headers); however, if the server
    // max_age is smaller we keep the server's max_age. The RFC says:
    //
    // \par
    // The "max-age" request directive indicates that the client is
    // unwilling to accept a response whose age is greater than the
    // specified number of seconds.
    //
    // a client's max_age=0 parameter means use the server defined
    // max-age which is the default
    //
    // note: the RFC may imply that if stale is also defined, then
    // max-age should be ignored
    //
    int64_t const client_max_age(f_client_ccs.get_max_age());
    if(client_max_age > 0 && client_max_age < max_age)
    {
        max_age = client_max_age;
    }

    // the client may request that the cache remains fresh (opposed to
    // becoming stale) for at least this many more seconds; in many
    // cases this won't be a problem (i.e. JS stay fresh for a long time)
    // but many of our pages time out immediately (no caching at all)
    // anyway
    //
    int64_t client_min_fresh(f_client_ccs.get_min_fresh());
    if(client_min_fresh < 0)
    {
        client_min_fresh = 0;
    }

    time_t const now(time(nullptr));
    if(now > date + max_age - client_min_fresh)
    {
        // if min-fresh=... was specified, we ignore stale=..., both
        // together doesn't make sense anyway and min-fresh is more
        // constraining
        //
        if(client_min_fresh > 0)
        {
            // this cached data may not even be stale yet
            // so for sure we want to keep it
            //
            //delete_cache_file(f_cache_permanent_filename);

            return -1;
        }

        // check client's stale parameter
        //
        // 1. stale is not defined, then it is IGNORE_VALUE and that means
        //    we see the cached file as invalid
        //
        // 2. stale is defined, set to an invalid value, then it gets set
        //    to IGNORE_VALUE and point (1) applies
        //
        // 3. stale is defined, set to zero (0), then whatever the age of
        //    the file, return the cached data
        //
        // 4. stale is defined, set to a non zero value, then add that to
        //    the date when the data became stale and see whether the
        //    data is really that much older and if so don't return the
        //    cache
        //
        // Note: that a stale larger then AGE_MAXIMUM is clamped to that
        // limit, which is 1 year. So you can get a cache file that
        // became stale for another year (a file that was given a maximum
        // age of 1 year + 1 year of stale can therefore be cahed for
        // a total of about 2 years, theoretically)
        //
        int64_t const max_stale(f_client_ccs.get_max_stale());
        if(max_stale == snap::cache_control_settings::IGNORE_VALUE
        || (max_stale > 0 && now > date + max_age + max_stale))
        {
            // this cached data has become stale, we keep it because some
            // requests may include a stale parameter (as shown in the
            // condition above)
            //
            // also, the max_age parameter may be "tweaked" by the client
            // which means that we can't rely on that parameter to know
            // that our data is stale
            //
            //delete_cache_file(f_cache_permanent_filename);

            return -1; // this is not correct so we can't return
        }
    }

    // the cache is up to date, the user may have a condition in his
    // header, though
    //
    // first check for the ETag
    //
    char const * const if_none_match(getenv("HTTP_IF_NONE_MATCH"));
    if(if_none_match != nullptr     // defined
    && *if_none_match != '\0')      // not an empty string
    {
        auto const etag(f_cache_fields.find("etag"));
        if(etag != f_cache_fields.end())
        {
            // there is an 'ETag' field, get the value and compare
            // against the user's
            //
            if(etag->second == if_none_match)
            {
                // this is the same, return 304
                //
                // As per RFC, the 304 can include the following fields:
                //
                //   Date              -- done by Apache, mandatory
                //   ETag              -- same as in 200 OK response
                //   Content-Location  -- same as in 200 OK response
                //   Expires           -- if changed since last request (?)
                //   Cache-Control     -- if changed since last request (?)
                //   Vary              -- if changed since last request (?)
                //
                // TODO: create a function because we do this twice so far...
                //
                QString err_name;
                snap::snap_child::define_http_name(snap::snap_child::http_code_t::HTTP_CODE_NOT_MODIFIED, err_name);
                std::cout << "Status: "
                          << static_cast<int>(snap::snap_child::http_code_t::HTTP_CODE_NOT_MODIFIED)
                          << " "
                          << err_name << std::endl;

                // So we don't do these at this time because it's really not
                // necessary although we could send the Cache-Control and ETag
                //for(auto l : lines)
                //{
                //    std::cout << l;
                //}

                // end of header
                //
                std::cout << std::endl;
                return 0;
            }
        }
    }

    // the ETag was not defined or not equal, try the last modification
    // date instead
    //
    char const * const if_modified_since(getenv("HTTP_IF_MODIFIED_SINCE"));
    if(if_modified_since != nullptr
    && *if_modified_since != '\0')
    {
        auto const last_modified_str(f_cache_fields.find("last-modified"));
        if(last_modified_str != f_cache_fields.end())
        {
            time_t const modified_since(snap::snap_child::string_to_date(QString::fromUtf8(if_modified_since)));
            time_t const last_modified(snap::snap_child::string_to_date(QString::fromUtf8(last_modified_str->second.c_str())));

            // TBD: should we use >= instead of == here?
            // (see in libsnapwebsites/src/snapwebsites/snap_child_cache_control.cpp too)
            //
            if(modified_since == last_modified
            && modified_since != -1)
            {
                // this is the same, return 304
                //
                // As per RFC, the 304 can include the following fields:
                //
                //   Date              -- done by Apache, mandatory
                //   ETag              -- same as in 200 OK response
                //   Content-Location  -- same as in 200 OK response
                //   Expires           -- if changed since last request (?)
                //   Cache-Control     -- if changed since last request (?)
                //   Vary              -- if changed since last request (?)
                //
                QString err_name;
                snap::snap_child::define_http_name(snap::snap_child::http_code_t::HTTP_CODE_NOT_MODIFIED, err_name);
                std::cout << "Status:"
                          << static_cast<int>(snap::snap_child::http_code_t::HTTP_CODE_NOT_MODIFIED)
                          << " "
                          << err_name << std::endl;

                std::cout << "Server:Snap! C++" << std::endl;

                // So we don't do these at this time because it's really not
                // necessary although we could send the Cache-Control and ETag
                //for(auto l : lines)
                //{
                //    std::cout << l;
                //}

                // end of header
                //
                std::cout << std::endl;
                return 0;
            }
        }
    }

    // rewind, but don't include the X-Snap-CGI-Date field which
    // is always the first (we save it that way in our cache for
    // ourselves)
    //
    cp.seekg(offset, std::ios::beg);
    //cp.seekg(0, std::ios::beg);

    // send it to Apache which will transmit to the client
    //
    char buf[4096];
    for(;;)
    {
        cp.read(buf, sizeof(buf));
        size_t const r(cp.gcount());
        if(r > 0)
        {
            if(fwrite(buf, r, 1, stdout) != 1)
            {
                // in this case we want to exit but we want to
                // keep the cached file
                //
                SNAP_LOG_FATAL("an I/O error occurred while sending the response to the client");
                exit(1);
                return 0; // this is not correct so we can't return
            }
        }
        if(cp.eof())
        {
            // it all worked!
            //
            // return 0 meaning that we sent a response and
            // can exit ASAP
            //
            return 0;
        }
        if(cp.fail())
        {
            SNAP_LOG_FATAL("an I/O error occurred while reading the response from cache file \"")
                          (f_cache_permanent_filename)
                          ("\".");
            delete_cache_file(f_cache_permanent_filename);
            exit(1);
            return 0; // this is not correct so we can't return
        }
    }
}


void snap_cgi::cache_data(char const * data, size_t size)
{
    switch(f_cache_state)
    {
    case cache_state_t::CACHE_STATE_FIELD_CACHE:
        // save to our file, it's not yet in the cache (because we're
        // still writing to it) but it's coming soon!
        //
        f_cache_file->write(data, size);
        if(f_cache_file->fail())
        {
            // the write failed, don't cache anything
            //
            f_cache_file.reset();
            f_cache_state = cache_state_t::CACHE_STATE_FIELD_NO_CACHE;
            snap::NOTUSED(delete_cache_file(f_cache_temporary_filename));
        }
        return;

    case cache_state_t::CACHE_STATE_FIELD_NO_CACHE:
        // no caching allowed, just ignore these calls
        //
        return;

    default:
        // in other cases, we are still reading the header so save that
        // data and move forward
        //
        f_cache.insert(f_cache.end(), data, data + size);
        break;

    }

    while(f_cache_pos < f_cache.size())
    {
        char const c(f_cache[f_cache_pos]);
        ++f_cache_pos;
        switch(f_cache_state)
        {
        case cache_state_t::CACHE_STATE_FIELD_CONTINUE:
            if(c == ' ' || c == '\t')
            {
                // Go to CACHE_STATE_FIELD_DATA_START next so we trim
                // extraneous spaces
                //
                f_cache_state = cache_state_t::CACHE_STATE_FIELD_DATA_START;

                // we're going to remove all the spaces and tabs so we must
                // add a space here
                //
                f_cache_field_data += ' ';
                break;
            }
            f_cache_state = cache_state_t::CACHE_STATE_FIELD_NAME;

            SNAP_LOG_DEBUG("got a new field: [")(f_cache_field_name)("] = \"")(f_cache_field_data)("\"");
            f_cache_fields[f_cache_field_name] = f_cache_field_data;
            f_cache_field_name.clear();
            f_cache_field_data.clear();
#if __cplusplus >= 201700
            [[fallthrough]];
#endif
        case cache_state_t::CACHE_STATE_FIELD_NAME:
            switch(c)
            {
            case '\n':
                if(!f_cache_field_name.empty())
                {
                    // a field name without a colon?
                    //
                    SNAP_LOG_WARNING("field name not terminated by a colon (:).");
                    f_cache_state = cache_state_t::CACHE_STATE_FIELD_NO_CACHE;
                    f_cache.clear();
                    return;
                }

                // we found the end of the header and no cache definitions
                // what do we do now?! we should cache this data, but for
                // now I'll just mark it as no-caching and we can adjust
                // that later
                //
                check_headers();
                return;

            case ':':
                // we found the field name/data separator, now read the data
                //
                f_cache_state = cache_state_t::CACHE_STATE_FIELD_DATA_START;
                break;

            case ' ':
            case '\t':
                // wait until we find the ':'
                //
                f_cache_state = cache_state_t::CACHE_STATE_FIELD_SEPARATOR;
                break;

            default:
                if(c >= 'A' && c <= 'Z')
                {
                    // force to lowercase
                    //
                    f_cache_field_name += c | 0x20;
                }
                else if((c < 'a' || c > 'z')
                     && (c < '0' || c > '9')
                     && c != '_'
                     && c != '-')
                {
                    // invalid character for a field name
                    //
                    SNAP_LOG_WARNING("field name include an unexpected character ('")(c)("').");
                    f_cache_state = cache_state_t::CACHE_STATE_FIELD_NO_CACHE;
                    f_cache.clear();
                    return;
                }
                else
                {
                    // other characters are kept as is
                    //
                    f_cache_field_name += c;
                }
                break;

            }
            break;

        case cache_state_t::CACHE_STATE_FIELD_SEPARATOR:
            switch(c)
            {
            case ' ':
            case '\t':
                // skip spaces and tabs after a field name
                //
                break;

            case ':':
                f_cache_state = cache_state_t::CACHE_STATE_FIELD_DATA_START;
                break;

            default:
                // we bumped in what looks like an invalid header field
                //
                SNAP_LOG_WARNING("invalid header field character found ('")(c)("'), expected spaces, tabs, or a colon.");
                f_cache_state = cache_state_t::CACHE_STATE_FIELD_NO_CACHE;
                f_cache.clear();
                return;

            }
            break;

        case cache_state_t::CACHE_STATE_FIELD_DATA_START:
            // trim leading spaces
            //
            if(c == ' ' || c == '\t')
            {
                break;
            }
            f_cache_state = cache_state_t::CACHE_STATE_FIELD_DATA;
#if __cplusplus >= 201700
            [[fallthrough]];
#endif
        case cache_state_t::CACHE_STATE_FIELD_DATA:
            if(c == '\n')
            {
                // found end of field?
                //
                f_cache_state = cache_state_t::CACHE_STATE_FIELD_CONTINUE;
                break;
            }
            f_cache_field_data += c;
            break;

        default:
            throw std::logic_error("invalid state for cache_data() header checking loop");

        }
    }
}


void snap_cgi::check_headers()
{
    // by default assume the worst
    //
    f_cache_state = cache_state_t::CACHE_STATE_FIELD_NO_CACHE;

    auto const status(f_cache_fields.find("status"));
    if(status != f_cache_fields.end())
    {
        std::string::size_type const pos(status->second.find(' '));
        if(pos != std::string::npos)
        {
            std::string const code(status->second.substr(0, pos));
            int const status_code(std::stoi(code));
            if(status_code != 200)
            {
                f_cache.clear();
                return;
            }
        }
    }

    snap::cache_control_settings ccs;
    auto const cache_control(f_cache_fields.find("cache-control"));
    if(cache_control != f_cache_fields.end())
    {
        ccs.set_must_revalidate(false); // the RFC default is `false` but we use `true` for Snap! -- which is "safer"
        ccs.set_cache_info(QString::fromUtf8(cache_control->second.c_str()), false);
//SNAP_LOG_WARNING
//    ("must-revalidate=")(ccs.get_must_revalidate() ? "true" : "false")
//    (" proxy-revalidate=")(ccs.get_proxy_revalidate() ? "true" : "false")
//    (" private=")(ccs.get_private() ? "true" : "false")
//    (" public=")(ccs.get_public() ? "true" : "false")
//    (" no-cache=")(ccs.get_no_cache() ? "true" : "false")
//    (" s-maxage=")(ccs.get_s_maxage())
//;
        if(
           !ccs.get_must_revalidate()       // no proxy caching
        && !ccs.get_proxy_revalidate()      // too complicated for now
        && !ccs.get_private()               // never cache private data
        &&  ccs.get_public()                // for now ignore if not specifically marked as public!
        && !ccs.get_no_cache()              // crystal clear
        &&  ccs.get_s_maxage() != 0         // no share cache if "s-maxage=0"
        )
        {
            f_cache_state = cache_state_t::CACHE_STATE_FIELD_CACHE;
        }
    }

    // TODO: handle other fields too?
    //
    //       Snap! creates the Cache-Control tag and the others
    //       (such as Expires and Pragma) define the same thing
    //       so we should not have to do anything with them,

    if(f_cache_state == cache_state_t::CACHE_STATE_FIELD_CACHE)
    {
        // caching allowed, save the data read so far to a .http file
        //
        std::string cache_path("/var/lib/snapwebsites/www/temporary");
        if(f_opt.is_defined("temporary-cache-path"))
        {
            cache_path = f_opt.get_string("temporary-cache-path");
        }

        // TODO: consider using open() with the O_TMPFILE flag which means
        //       that the file gets unlinked() automatically on exit
        //       the rename() needs to then change to a linkat() instead
        //
        f_cache_temporary_filename = cache_path + "/" + std::to_string(getpid()) + ".http";
        f_cache_file.reset(new std::ofstream(f_cache_temporary_filename, std::ios_base::out | std::ios_base::trunc | std::ios_base::binary));
        if(f_cache_file->is_open())
        {
            QString const date(QString("X-Snap-CGI-Date:%1\n").arg(time(nullptr)));
            QByteArray const date_bytes(date.toUtf8());
            f_cache_file->write(date_bytes.data(), date_bytes.size());

            // the cache must not include fields that are considered
            // private so we save the headers except those marked private
            // or in need of revalidation
            //
            snap::cache_control_settings::fields_t all_names;

            auto add_excluded_names = [](snap::cache_control_settings::fields_t & result,
                                         snap::cache_control_settings::fields_t const & names)
                    {
                        for(auto & n : names)
                        {
                            result.insert(boost::algorithm::to_lower_copy(n));
                        }
                    };
            add_excluded_names(all_names, ccs.get_private_field_names());
            add_excluded_names(all_names, ccs.get_revalidate_field_names());

            for(auto const & f : f_cache_fields)
            {
                if(std::find(all_names.begin(), all_names.end(), f.first) == all_names.end())
                {
                    *f_cache_file << f.first << ":" << f.second << std::endl;
                }
            }
            *f_cache_file << std::endl;

            // sanity check
            //
            if(f_cache_pos > f_cache.size())
            {
                throw std::logic_error("cache position (f_cache_pos) too large while saving file to cache");
            }

            // do not re-save the header, only the data
            //
            f_cache_file->write(f_cache.data() + f_cache_pos, f_cache.size() - f_cache_pos);

            if(f_cache_file->fail())
            {
                // the first write failed, don't cache anything
                //
                f_cache_file.reset();
            }
        }
        else
        {
            // could not open cache file
            //
            // TODO report to snapwatchdog
            //
            f_cache_file.reset();
        }
        if(f_cache_file == nullptr)
        {
            // something failed, no caching
            //
            f_cache_state = cache_state_t::CACHE_STATE_FIELD_NO_CACHE;
            snap::NOTUSED(delete_cache_file(f_cache_temporary_filename));
        }
    }

    // release the memory used so far, no need to waste it
    //
    f_cache.clear();
}


void snap_cgi::temporary_to_permanent_cache()
{
    // if we don't have a cache file, ignore
    //
    if(f_cache_file == nullptr)
    {
        return;
    }

    // the f_cache_temporary_filename is the name of the temporary cache file
    // we want to move it to the permanent location now
    //
    int r(delete_cache_file(f_cache_permanent_filename));
    if(r == -1 && errno == ENOENT)
    {
        // ignore this error
        //
        r = 0;
    }
    if(r == 0)
    {
        r = snap::mkdir_p(f_cache_permanent_filename, true);
        if(r == 0)
        {
            r = rename_cache_file(f_cache_temporary_filename, f_cache_permanent_filename);
        }
    }

    // on failure unlink the temporary file
    //
    // if renamed successfully no matter since the file was moved to
    // the new location, otherwise it could pile up for nothing in
    // our temporary cache folder
    //
    if(r != 0)
    {
        snap::NOTUSED(delete_cache_file(f_cache_temporary_filename));
    }
}


int snap_cgi::rename_cache_file(std::string const & from_filename, std::string const & to_filename)
{
    int const r(rename(from_filename.c_str(), to_filename.c_str()));
    if(r != 0)
    {
        int const e(errno);
        SNAP_LOG_ERROR("could not rename file \"")
                      (from_filename)
                      (" to file \"")
                      (to_filename)
                      ("\" (errno: ")
                      (e)
                      (" -- ")
                      (strerror(e))
                      (".)");
    }

    return r;
}


int snap_cgi::delete_cache_file(std::string const & filename)
{
    int const r(unlink(filename.c_str()));
    if(r != 0)
    {
        int const e(errno);
        SNAP_LOG_ERROR("could not delete cache file \"")
                      (filename)
                      ("\" (errno: ")
                      (e)
                      (" -- ")
                      (strerror(e))
                      (".)");
        // make sure to restore the error in this case
        //
        errno = e;
    }

    return r;
}



int main(int argc, char * argv[])
{
    // The Apache2 environment will pass parameters to us whenever the
    // end user enters a query string without an equal sign. For example:
    //
    //      http://www.example.com/cgi-bin/snapmanager.cgi?logout
    //
    // would add "logout" in argv[1]. That means hackers can pass any
    // parameter to us (since `-` is a legal character in such query
    // string parameters.) So here we clear the list and force the count
    // to exactly 1 (i.e. we keep the program name only.)
    //
    argc = 1;
    argv[1] = nullptr;

    try
    {
        snap_cgi cgi( argc, argv );
        try
        {
            if(!cgi.verify())
            {
                return 1;
            }
            return cgi.process();
        }
        catch(std::runtime_error const & e)
        {
#ifdef _DEBUG
            SNAP_LOG_DEBUG("runtime error ")(e.what());
#endif
            // this should never happen!
            cgi.error("503 Service Unavailable", nullptr, ("The Snap! C++ CGI script caught a runtime exception: " + std::string(e.what()) + ".").c_str());
            return 1;
        }
        catch(std::logic_error const & e)
        {
#ifdef _DEBUG
            SNAP_LOG_DEBUG("logic error ")(e.what());
#endif
            // this should never happen!
            cgi.error("503 Service Unavailable", nullptr, ("The Snap! C++ CGI script caught a logic exception: " + std::string(e.what()) + ".").c_str());
            return 1;
        }
        catch(...)
        {
#ifdef _DEBUG
            SNAP_LOG_DEBUG("unknown error!");
#endif
            // this should never happen!
            cgi.error("503 Service Unavailable", nullptr, "The Snap! C++ CGI script caught an unknown exception.");
            return 1;
        }
    }
    catch(std::exception const & e)
    {
#ifdef _DEBUG
        // the outter exception does not have access to the logger
        //SNAP_LOG_DEBUG("outer exception: ")(e.what());
#endif
        // we are in trouble, we cannot even answer
        std::cerr << "snap: exception: " << e.what() << std::endl;
        return 1;
    }
    snap::NOTREACHED();
}

// vim: ts=4 sw=4 et
