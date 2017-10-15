// Snap Websites Server -- snap websites CGI function
// Copyright (C) 2011-2017  Made to Order Software Corp.
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

#include "version.h"

#include <snapwebsites/addr.h>
#include <snapwebsites/log.h>
#include <snapwebsites/not_reached.h>
#include <snapwebsites/snapwebsites.h>
#include <snapwebsites/tcp_client_server.h>

#include <advgetopt/advgetopt.h>


namespace
{
    const std::vector<std::string> g_configuration_files
    {
        "@snapwebsites@", // project name
        "/etc/snapwebsites/snapcgi.conf"
    };

    const advgetopt::getopt::option g_snapcgi_options[] =
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
            advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE | advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            "snapserver",
            nullptr,
            "IP address on which the snapserver is running, it may include a port (i.e. 192.168.0.1:4004)",
            advgetopt::getopt::argument_mode_t::optional_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
            "log-config",
            "/etc/snapwebsites/logger/snapcgi.properties",
            "Full path of log configuration file",
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
            "use-ssl",
            nullptr,
            "Whether SSL should be used to connect to snapserver. Set to \"true\" or \"false\".",
            advgetopt::getopt::argument_mode_t::required_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            "version",
            nullptr,
            "Show the version of %p and exist.",
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
    advgetopt::getopt   f_opt;
    int                 f_port = -1;    // snap server port
    std::string         f_address;      // snap server address
};


snap_cgi::snap_cgi( int argc, char * argv[] )
    : f_opt(argc, argv, g_snapcgi_options, g_configuration_files, "SNAPCGI_OPTIONS")
    , f_port(4004)
    , f_address("127.0.0.1")
{
    if(f_opt.is_defined("version"))
    {
        std::cout << SNAPCGI_VERSION_STRING << std::endl;
        exit(0);
    }
    if(f_opt.is_defined("help"))
    {
        f_opt.usage(advgetopt::getopt::status_t::no_error, "Usage: %s -<arg> ...\n", argv[0]);
        exit(1);
    }

    // read log-config and setup the logger
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
        snap_addr::addr a(f_opt.get_string("snapserver"), f_address, f_port, "tcp");
        f_address = a.get_ipv4or6_string(false, false);
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
    SNAP_LOG_DEBUG("processing request_method=")(request_method);
#endif
    SNAP_LOG_DEBUG("f_address=")(f_address)(", f_port=")(f_port)(", secure=")(secure ? "true" : "false");

    {
        sigset_t set;
        sigemptyset(&set);
        sigaddset(&set, SIGPIPE);
        sigprocmask(SIG_BLOCK, &set, nullptr);
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
                SNAP_LOG_FATAL("an I/O error occurred while sending the response to the client");
                return 1;
            }
            wrote += r;

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
            SNAP_LOG_FATAL("an I/O error occurred while reading the response from the server");
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

    // TODO: handle potential read problems...
#ifdef _DEBUG
    SNAP_LOG_DEBUG("Closing connection...");
#endif
    return 0;
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
