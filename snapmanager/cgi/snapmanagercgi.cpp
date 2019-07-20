//
// File:        snapmanager/cgi/snapmanagercgi.cpp
// Object:      Allow for managing a Snap! Cluster.
//
// Copyright:   Copyright (c) 2016-2019  Made to Order Software Corp.  All Rights Reserved
//              All Rights Reserved.
//
// https://snapwebsites.org/
// contact@m2osw.com
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//


// self
//
#include "snapmanagercgi.h"


// our lib
//
#include "snapmanager/plugin_base.h"
#include "snapmanager/server_status.h"


// snapwebsites lib
//
#include <snapwebsites/chownnm.h>
#include <snapwebsites/file_content.h>
#include <snapwebsites/glob_dir.h>
#include <snapwebsites/mkdir_p.h>
#include <snapwebsites/qdomhelpers.h>
#include <snapwebsites/snap_communicator.h>
#include <snapwebsites/snap_uri.h>


// snapdev lib
//
#include <snapdev/hexadecimal_string.h>
#include <snapdev/not_used.h>
#include <snapdev/tokenize_string.h>


// addr lib
#include <libaddr/addr_parser.h>


// Qt lib
//
#include <QFile>


// boost lib
//
#include <boost/algorithm/string/replace.hpp>
#include <boost/lexical_cast.hpp>


// C++ lib
//
#include <fstream>


// C lib
//
#include <fcntl.h>
#include <sys/file.h>


// OpenSLL lib
//
#include <openssl/rand.h>


// last include
//
#include <snapdev/poison.h>


namespace snap_manager
{


namespace
{






/** \brief Close a file descriptor.
 *
 * This function will close the file descriptor pointer by fd.
 *
 * \param[in] fd  Pointer to the file descriptor to close.
 */
void close_file(int * fd)
{
    close(*fd);
}


} // no name namespace


/** \brief Initialize the manager_cgi.
 *
 * The manager_cgi gets initialized with the argc and argv in case it
 * gets started from the command line. That way one can use --version
 * and --help, especially.
 */
manager_cgi::manager_cgi()
    : manager(false)
    , f_communicator_address("127.0.0.1")
    , f_communicator_port(4040)
{
}


manager_cgi::~manager_cgi()
{
}


manager_cgi::pointer_t manager_cgi::instance()
{
    return std::dynamic_pointer_cast<manager_cgi>(manager::instance());
}


std::string manager_cgi::server_type() const
{
    return "manager_cgi";
}


int manager_cgi::error(char const * code, char const * msg, char const * details)
{
    if(details == nullptr)
    {
        details = "No details.";
    }

    SNAP_LOG_FATAL("error(\"")(code)("\", \"")(msg)("\", \"")(details)("\")");

    std::string body("<h1>");
    body += code;
    body += "</h1><p>";
    body += (msg == nullptr ? "Sorry! We found an invalid server configuration or some other error occurred." : msg);
    body += "</p>";
    body += "<p><a href=\"/snapmanager\">Home</a></p>";

    std::cout   << "Status: " << code                       << std::endl
                << "Expires: Sun, 19 Nov 1978 05:00:00 GMT" << std::endl
                << "Connection: close"                      << std::endl
                << "Content-Type: text/html; charset=utf-8" << std::endl
                << "Content-Length: " << body.length()      << std::endl
                << f_cookie
                << "X-Powered-By: snapmanager.cgi"          << std::endl
                << std::endl
                << body
                ;

    return 1;
}


void manager_cgi::forbidden(std::string details, bool allow_redirect)
{
    if(details.empty())
    {
        details = "No details.";
    }

    // the administrator has the option to redirect a user instead of
    // outputing a 403...
    //
    if(allow_redirect
    && f_config.has_parameter("redirect_unwanted"))
    {
        std::string const uri(f_config["redirect_unwanted"]);
        if(!uri.empty())
        {
            // administrator wants to redirect unwanted users
            //
            SNAP_LOG_FATAL("Redirect user to \"")
                          (uri)
                          ("\" on error(\"403 Forbidden\", \"You are not allowed on this server.\", \"")
                          (details)
                          ("\")");

            std::cout   << "Status: 301"                            << std::endl
                        << "Location: " << uri                      << std::endl
                        << "Expires: Sun, 19 Nov 1978 05:00:00 GMT" << std::endl
                        << "Connection: close"                      << std::endl
                        << std::endl
                        ;

            // do not emit an error since we just sent a redirect
            return;
        }
    }

    error("403 Forbidden", "You are not allowed on this server.", details.c_str());
}



std::string manager_cgi::get_session_path(bool create)
{
    std::string path(get_www_cache_path().toUtf8().data());

    path += "/sessions/snapmanager";

    // user requested for the path to be created if necessary?
    //
    if(create)
    {
        if(snap::mkdir_p(path, false, 0700, "www-data", "www-data") != 0)
        {
            // could not create the directory, this is an error
            //
            std::stringstream ss;

            ss << "Could not ensure the availability of the session directory \""
               << path
               << "\".";

            error(
                  "500 Internal Server Error"
                , "An internal error occurred."
                , ss.str().c_str());

            // this function has to return a valid path or fail miserably
            // (i.e. an exit() in a random place in C++ may not work as
            // expected)
            //
            exit(1);
        }
    }

    return path;
}



/** \brief Verify that the request is acceptable.
 *
 * This function makes sure that the request corresponds to what we
 * generally expect.
 *
 * \return true if the request is accepted, false otherwise.
 */
bool manager_cgi::verify()
{
    if(!f_config.has_parameter("stylesheet"))
    {
        error("503 Service Unavailable",
              "The snapmanager.cgi service is not currently available.",
              "The stylesheet parameter is not defined.");
        return false;
    }

    // If not defined, keep the default of localhost:4040
    // TODO: make these "just in time" parameters, we nearly never need them
    if(f_config.has_parameter("snapcommunicator", "local_listen"))
    {
        addr::addr const a(addr::string_to_addr(f_config("snapcommunicator", "local_listen"), "127.0.0.1", 4040, "tcp"));
        f_communicator_address = a.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_ONLY);
        f_communicator_port = a.get_port();
    }

    {
        // if the SNAPMANAGER environment variable is not set, then we have
        // a problem and we want to emit an error
        //
        // (i.e. we are being accessed from the from domain)
        //
        char const * snapmanager(getenv("SNAPMANAGER"));
        if(snapmanager == nullptr
        || strcmp(snapmanager, "TRUE") != 0)
        {
            SNAP_LOG_FATAL("SNAPMANAGER variable is not set, check your Apache2 setup, you should have a `SetEnv SNAPMANAGER TRUE` line in your snapmanager-apache2.conf file.");
            std::string body("<html><head><title>Page Not Found</title></head><body><h1>Page Not Found</h1><p>Sorry. This page is not accessible from here.</p></body></html>");
            std::cout   << "Status: 404 Page Not Found"             << std::endl
                        << "Expires: Sat, 1 Jan 2000 00:00:00 GMT"  << std::endl
                        << "Connection: close"                      << std::endl
                        << "Content-Type: text/html; charset=utf-8" << std::endl
                        << "Content-Length: " << body.length()      << std::endl
                        << "X-Powered-By: snapmanager.cgi"          << std::endl
                        << std::endl
                        << body;
            return false;
        }
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
            std::string body("<html><head><title>Method Not Defined</title></head><body><h1>Method Not Defined</h1><p>Sorry. We only support GET and POST.</p></body></html>");
            std::cout   << "Status: 405 Method Not Defined"         << std::endl
                        << "Expires: Sat, 1 Jan 2000 00:00:00 GMT"  << std::endl
                        << "Allow: GET, POST"                       << std::endl
                        << "Connection: close"                      << std::endl
                        << "Content-Type: text/html; charset=utf-8" << std::endl
                        << "Content-Length: " << body.length()      << std::endl
                        << "X-Powered-By: snapmanager.cgi"          << std::endl
                        << std::endl
                        << body;
            return false;
        }
        if(strcmp(request_method, "GET") != 0
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
            std::string const body("<html><head><title>Method Not Allowed</title></head><body><h1>Method Not Allowed</h1><p>Sorry. We only support GET and POST.</p></body></html>");
            std::cout   << "Expires: Sat, 1 Jan 2000 00:00:00 GMT"  << std::endl
                        << "Allow: GET, POST"                       << std::endl
                        << "Connection: close"                      << std::endl
                        << "Content-Type: text/html; charset=utf-8" << std::endl
                        << "Content-Length: " << body.length()      << std::endl
                        << "X-Powered-By: snapamanager.cgi"         << std::endl
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

    // get the client IP address
    //
    char const * remote_addr(getenv("REMOTE_ADDR"));
    if(remote_addr == nullptr)
    {
        error(
            "400 Bad Request",
            nullptr,
            "The REMOTE_ADDR parameter is not available.");
        return false;
    }

    // verify that this is a client we allow to use snapmanager.cgi
    //
    if(!f_config.has_parameter("clients"))
    {
        forbidden("The clients=... parameter is undefined.");
        return false;
    }

    {
        addr::addr const remote_address(addr::string_to_addr(std::string(remote_addr) + ":80", std::string(), -1, "tcp"));
        std::string const client(f_config.has_parameter("clients") ? f_config["clients"] : std::string());

        snap::snap_string_list const client_list(QString::fromUtf8(client.c_str()).split(',', QString::SkipEmptyParts));
        bool found(false);
        for(auto const & c : client_list)
        {
            addr::addr const client_address(addr::string_to_addr((c.trimmed() + ":80").toUtf8().data(), std::string(), -1, "tcp"));
            if(client_address == remote_address)
            {
                found = true;
                break;
            }
        }
        if(!found)
        {
            forbidden("Your remote IP address, "
                     + remote_address.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_ALL)
                     + ", is unknown to this snapmanager.cgi instance.");
            return false;
        }
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

#if 0
        // we test that it is not null, but we accept access with plain IP
        // addresses (which may be used for the first few accesses)
        //
        if(tcp_client_server::is_ipv4(http_host))
        {
            SNAP_LOG_ERROR("The host cannot be an IPv4 address.");
            std::cout   << "Status: 444 No Response"        << std::endl
                        << "Connection: close"              << std::endl
                        << "X-Powered-By: snapmanager.cgi"  << std::endl
                        << std::endl
                        ;
            snap::server::block_ip(remote_addr, "week", "user accessed snapmanager.cgi with a bare IPv4 address");
            return false;
        }
        if(tcp_client_server::is_ipv6(http_host))
        {
            SNAP_LOG_ERROR("The host cannot be an IPv6 address.");
            std::cout   << "Status: 444 No Response"        << std::endl
                        << "Connection: close"              << std::endl
                        << "X-Powered-By: snapmanager.cgi"  << std::endl
                        << std::endl
                        ;
            snap::server::block_ip(remote_addr, "week", "user accessed snapmanager.cgi with a bare IPv6 address");
            return false;
        }
#endif
    }

    {
        // WARNING: do not use std::string because nullptr will crash
        //
        char const * request_uri(getenv(snap::get_name(snap::name_t::SNAP_NAME_CORE_REQUEST_URI)));
        if(request_uri == nullptr)
        {
            // this should NEVER happen because without a path after the method
            // we probably do not have our snapmanager.cgi run anyway...
            //
            error("400 Bad Request", "The path to the page you want to read must be specified.", nullptr);
            return false;
        }
#ifdef _DEBUG
        //SNAP_LOG_DEBUG("REQUEST_URI=")(request_uri);
#endif

        // if we do not receive this, somehow someone was able to access
        // snapmanager.cgi without specifying /cgi-bin/... which is not
        // correct
        //
        if(strncasecmp(request_uri, "/cgi-bin/", 9) == 0)
        {
            error(
                "404 Page Not Found",
                "We could not find the page you were looking for.",
                (std::string("The REQUEST_URI must start with \"/snapmanager\", it cannot include \"/cgi-bin/\" as in \"") + request_uri + "\".").c_str());
            snap::server::block_ip(remote_addr, QString(), "user tried to access snapmanager.cgi with \"/cgi-bin/...\" which is not allowed");
            return false;
        }

        // TODO: move to snapserver because this could be the name of a legal page...
        if(strcasestr(request_uri, "phpmyadmin") != nullptr)
        {
            // block myPhpAdmin accessors
            error("410 Gone", "MySQL left.", nullptr);
            snap::server::block_ip(remote_addr, "year", "user tried to access phpmyadmin through snapmanager.cgi");
            return false;
        }

        // once the index.html page is blocked, we would end up with a 404
        // instead we can just redirect the user to /snapmanager
        //
        if(strcmp(request_uri, "/") == 0)
        {
            SNAP_LOG_FATAL("Redirect user to \"/snapmanager\".");

            // We use 302 so it will be possible to see the index.html again
            // if we decide to set the status back to "new"
            //
            // We already tested and know that `http_host != nullptr`
            //
            char const * http_host(getenv("HTTP_HOST"));
            std::cout   << "Status: 302"                                       << std::endl
                        << "Location: https://" << http_host << "/snapmanager" << std::endl
                        << "Expires: Sun, 19 Nov 1978 05:00:00 GMT"            << std::endl
                        << "Connection: close"                                 << std::endl
                        << std::endl
                        ;
            return false;
        }

        // We do not allow any kind of proxy
        //
        // Note: Yes. This is not required at this point since we check that
        //       the path is "/snapmanager" and since it starts with "/"...
        //       However, we may change that later and we think it is
        //       preferable to keep things this way.
        //
        if(*request_uri != '/')
        {
            // avoid proxy accesses
            error(
                "404 Page Not Found",
                nullptr,
                "The REQUEST_URI cannot represent a proxy access.");
            snap::server::block_ip(remote_addr, "year", "user tried to access \"snapmanager.cgi\" with a proxy");
            return false;
        }

        // make sure the user is trying to access exactly "/snapmanager/?"
        // (with the '/' and '?' being optional)
        //
        // at this point we do not support any other paths
        //
        if(strcasecmp(request_uri, "/snapmanager") != 0
        && strcasecmp(request_uri, "/snapmanager/") != 0
        && strncasecmp(request_uri, "/snapmanager?", 13) != 0
        && strncasecmp(request_uri, "/snapmanager/?", 14) != 0)
        {
            error(
                "404 Page Not Found",
                "We could not find the page you were looking for.",
                (std::string("The REQUEST_URI must be \"/snapmanager\", not \"") + request_uri + "\".").c_str());
            snap::server::block_ip(remote_addr, QString(), "user tried to access \"/snapmanager\" through snapmanager.cgi");
            return false;
        }
    }

    {
        // WARNING: do not use std::string because nullptr will crash
        //
        char const * user_agent(getenv(snap::get_name(snap::name_t::SNAP_NAME_CORE_HTTP_USER_AGENT)));
        if(user_agent == nullptr)
        {
            // we request an agent specification
            //
            error("400 Bad Request", "The accessing agent must be specified.", nullptr);
            snap::server::block_ip(remote_addr, "month", "User-Agent header is missing");
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
        // snapmanager.cgi, which will not work right so better
        // err immediately
        //
        if(*user_agent == '\0'
        || (*user_agent == '-' && user_agent[1] == '\0')
        || strcasestr(user_agent, "ZmEu") != nullptr)
        {
            // note that we consider "-" as empty for this test
            error("400 Bad Request", nullptr, "The agent string cannot be empty.");
            snap::server::block_ip(remote_addr, "month", "this is ZmEu, we immediately block such requests");
            return false;
        }
    }

    // success
    return true;
}


/** \brief Process one hit to snapmanager.cgi.
 *
 * This is the function that generates the HTML or AJAX reply to
 * the client.
 *
 * \return 0 if the process worked as expected, 1 otherwise.
 */
int manager_cgi::process()
{
    // WARNING: do not use std::string because nullptr will crash
    char const * request_method_str( getenv("REQUEST_METHOD") );
    if(request_method_str == nullptr)
    {
        // the method was already checked in verify(), before this
        // call so it should always be defined here...
        //
        SNAP_LOG_FATAL("Method not defined in REQUEST_METHOD.");
        std::string body("<html><head><title>Method Not Defined</title></head><body><p>Sorry. We only support GET and POST.</p></body></html>");
        std::cout   << "Status: 405 Method Not Defined"         << std::endl
                    << "Expires: Sat, 1 Jan 2000 00:00:00 GMT"  << std::endl
                    << "Connection: close"                      << std::endl
                    << "Allow: GET, POST"                       << std::endl
                    << "Content-Type: text/html; charset=utf-8" << std::endl
                    << "Content-Length: " << body.length()      << std::endl
                    << "X-Powered-By: snapmanager.cgi"          << std::endl
                    << std::endl
                    << body;
        return 0;
    }
    std::string request_method(request_method_str);
#ifdef _DEBUG
    SNAP_LOG_DEBUG("processing request_method=")(request_method);
#endif

    if(request_method == "POST")
    {
        // a form posted?
        // convert the POST variables in a map
        //
        if(read_post_variables() != 0)
        {
            return 1;
        }
    }

    // retrieve the query string, that's all we use in this one (i.e.
    // at this point we ignore the path)
    //
    // TODO: add support to make sure the administrator uses HTTPS?
    //       (this can be done in Apache2)
    //
    char const * query_string(getenv("QUERY_STRING"));
    if(query_string != nullptr)
    {
        f_uri.set_query_string(QString::fromUtf8(query_string));
        SNAP_LOG_TRACE("QUERY_STRING=")(query_string);
    }

    // make sure the user is logged in
    //
    {
        int const r(is_logged_in(request_method));
        if(r != 0)
        {
            // return value is 2 if we are showing the logging screen
            // and 1 in all other cases (i.e. errors)
            //
            return r == 2 ? 0 : 1;
        }
    }

    if(request_method == "POST")
    {
        if(process_post() != 0)
        {
            SNAP_LOG_ERROR("POST discarded due to error!");
            // an error occurred, exit now
            return 0;
        }

        QString const host         = f_post_variables["hostname"].c_str();
        QString const plugin_name  = f_post_variables["plugin_name"].c_str();

        // Make sure host appears in the URI parameters.
        //
        if( !f_uri.has_query_option("host") )
        {
            f_uri.set_query_option("host", host);
        }

        QDomDocument doc;
        QDomElement output(doc.createElement("output"));
        //
        // Only generate the child div of the named plugin.
        //
        status_map_t status_map;
        get_status_map( host, status_map );
        generate_plugin_status( doc, output, plugin_name, status_map[plugin_name] );
        //
        // Add only this element to the "output" and send it back as a post.
        // Also, avoid the enclosed <output> section and send the div only.
        //
        doc.appendChild(output.firstChildElement());

        QString const new_div( doc.toString() );
//SNAP_LOG_TRACE("Handling POST: new_div=")(new_div);
        std::cout
                << "Expires: Sat, 1 Jan 2000 00:00:00 GMT"  << std::endl
                << "Connection: close"                      << std::endl
                << "Content-Type: text/html; charset=utf-8" << std::endl
                << "Content-Length: " << new_div.length()   << std::endl
                << f_cookie
                << "X-Powered-By: snapmanager.cgi"          << std::endl
                << std::endl
                << new_div;
    }
    else
    {
        QDomDocument doc;
        QDomElement root(doc.createElement("manager"));
        doc.appendChild(root);
        QDomElement output(doc.createElement("output"));
        root.appendChild(output);
        QDomElement snap_version(doc.createElement("snap-version"));
        root.appendChild(snap_version);
        QDomText snapversion_text(doc.createTextNode("Snap! Websites v" SNAPMANAGER_VERSION_STRING));
        snap_version.appendChild(snapversion_text);
        QDomElement menu(doc.createElement("menu"));
        root.appendChild(menu);

        // we need the plugins for the following test
        //
        load_plugins();

        generate_content(doc, root, output, menu, f_uri);

        // handle this warning after the generate_content() signal
        {
            // we force HTTPS by default, but someone could turn that feature
            // off...
            //
            char const * https(getenv("HTTPS"));
            if(https == nullptr
            || strcmp(https, "on") != 0)
            {
                QDomElement warning_div(doc.createElement("div"));
                warning_div.setAttribute("class", "access-warning");
                output.insertBefore(warning_div, QDomNode());

                // TODO: add a link to a help page on snapwebsites.org
                snap::snap_dom::insert_html_string_to_xml_doc(warning_div,
                        "<div class=\"access-title\">WARNING</div>"
                        "<p>You are accessing this website without SSL. All the data transfers will be unencrypted.</p>");
            }
        }

        snap::xslt x;
        x.set_xsl_from_file(f_config["stylesheet"]); // setup the .xsl file
        x.set_document(doc);

        QString const body("<!DOCTYPE html>" + x.evaluate_to_string());

        //std::string body("<html><head><title>Snap Manager</title></head><body><p>...TODO...</p></body></html>");
        std::cout   //<< "Status: 200 OK"                         << std::endl
                << "Expires: Sat, 1 Jan 2000 00:00:00 GMT"  << std::endl
                << "Connection: close"                      << std::endl
                << "Content-Type: text/html; charset=utf-8" << std::endl
                << "Content-Length: " << body.length()      << std::endl
                << f_cookie
                << "X-Powered-By: snapmanager.cgi"          << std::endl
                << std::endl
                << body;
    }

    return 0;
}


snap::snap_uri const & manager_cgi::get_uri() const
{
    return f_uri;
}


int manager_cgi::read_post_variables()
{
    char const * content_type(getenv("CONTENT_TYPE"));
    if(content_type == nullptr)
    {
        return error("500 Internal Server Error", "the CONTENT_TYPE variable was not defined along a POST.", nullptr);
    }
    bool const is_multipart(QString(content_type).startsWith("multipart/form-data"));
    int const break_char(is_multipart ? '\n' : '&');

    std::string name;
    std::string value;
    bool found_name(false);
    for(;;)
    {
        int const c(getchar());
        if(c == break_char || c == EOF)
        {
            if(!name.empty())
            {
                name = snap::snap_uri::urldecode(name.c_str(), true).toUtf8().data();
                value = snap::snap_uri::urldecode(value.c_str(), true).toUtf8().data();
                f_post_variables[name] = value;
#ifdef _DEBUG
                SNAP_LOG_DEBUG("got ")(name)(" = ")(f_post_variables[name]);
#endif
            }
            if(c == EOF)
            {
                // this was the last variable
                return 0;
            }
            name.clear();
            value.clear();
            found_name = false;
        }
        else if(c == '=')
        {
            found_name = true;
        }
        else if(found_name)
        {
            value += c;
        }
        else
        {
            name += c;
        }
    }
}


std::string manager_cgi::get_hit_filename()
{
    // name of the cache file used to count failed login attempts
    //
    // note: it will get deleted after 1 to 2 months if not
    //       accessed for that long--see snapbase.cron.monthly
    //       we also delete that file on a successful login
    //

    // the filename uses the remote IP address
    //
    char const * remote_addr(getenv("REMOTE_ADDR"));
    if(remote_addr == nullptr)
    {
        error(
            "400 Bad Request",
            nullptr,
            "The REMOTE_ADDR parameter is not available.");
        return std::string();
    }

    return QString("%1/snapmanagercgi/%2.hit")
                            .arg(get_www_cache_path())
                            .arg(remote_addr)
                                    .toUtf8().data();
}


int manager_cgi::is_ip_blocked()
{
    std::string const hit_filename(get_hit_filename());
    if(hit_filename.empty())
    {
        return 1;
    }

    // we cache the result, if -1 we are being called for the first time
    //
    if(f_login_attempts == -1)
    {
        f_login_attempts = 0;

        // read the value if file exists
        {
            std::ifstream hit_file;
            hit_file.open(hit_filename, std::ios_base::in | std::ios_base::binary);
            if(hit_file.is_open())
            {
                char buf[256];
                hit_file.getline(buf, sizeof(buf));
                if(hit_file.good())
                {
                    f_login_attempts = std::atoi(buf);
                }
            }
        }

        // get the maximum number of login attempts that the administrator
        // wants to allow for this instance
        //
        if(f_config.has_parameter("max_login_attempts"))
        {
            f_max_login_attempts = std::stoi(f_config["max_login_attempts"]);
            if(f_max_login_attempts < 2)
            {
                // this is tainted data, make sure it is at least 2
                //
                f_max_login_attempts = 2;
            }
            else if(f_max_login_attempts > 100)
            {
                // 100 is way too much already, we really need to block the
                // hackers sooner, but what can you do?!
                //
                f_max_login_attempts = 100;
            }
        }
    }

    // verify each time how many times the user failed entering his
    // credentials (it's important since it could be increased between
    // calls to this function)
    //
    int const result(f_login_attempts >= f_max_login_attempts ? 1 : 0);
    if(result != 0)
    {
        // TODO: block IP address in firewall?
        //       (it can be really annoying for the admin. but useful
        //       to really avoid additional hacker's mischiefs)

        // another failed attempt, do a `touch -ca $hit_filename`
        //
        struct timespec times[2];
        times[0].tv_sec = 0;
        times[0].tv_nsec = UTIME_NOW;
        times[1].tv_sec = 0;
        times[1].tv_nsec = UTIME_OMIT;
        utimensat(AT_FDCWD, hit_filename.c_str(), times, 0);

        std::stringstream ss;
        ss << "Your address ("
           << getenv("REMOTE_ADDR") // we know it won't return NULL because we called get_hit_filename() which verifies that
           << ") is currently blocked by its hit file ("
           << hit_filename
           << "). To restore access to your account, delete that file.";

        forbidden(ss.str(), false);
    }

    return result;
}


void manager_cgi::increase_hit_count()
{
    // one more user error, increase hit counter
    // and save it back to file
    //
    ++f_login_attempts;

    std::string const hit_filename(get_hit_filename());
    if(hit_filename.empty())
    {
        return;
    }

    // make sure the folder exists since we are dealing
    // with the cache (which gets its files/folders deleted
    // once in a while)
    //
    // here ownership is as expected
    //
    snap::mkdir_p(hit_filename, true);

    // TODO: we should probably create a new file, unlink the old one,
    //       then rename, to avoid the problem of an open/write error
    //       and losing the hit count (although the unlink+rename
    //       combo could fail bad too)
    //
    std::ofstream out;
    out.open(hit_filename, std::ios_base::out | std::ios_base::trunc | std::ios_base::binary);
    if(out.is_open())
    {
        out << f_login_attempts << std::endl;
    }
}


void manager_cgi::delete_hit_file()
{
    std::string const hit_filename(get_hit_filename());
    if(!hit_filename.empty())
    {
        unlink(hit_filename.c_str());
    }
}


int manager_cgi::is_logged_in(std::string & request_method)
{
    // session duration (TODO: make a parameter from the .conf)
    //
    int64_t const session_duration(3 * 24 * 60 * 60);

    auto login_form = [&](std::string const error_msg = "", bool logout = false)
        {
            // if the user attempted to log in too many times his IP gets blocked
            // for until they delete the hit file or 1 to 2 months when the
            // hit file gets deleted by the auto-cache removal script
            //
            int const hit_result(is_ip_blocked());
            if(hit_result != 0)
            {
                return hit_result;
            }

            snap::file_content login_page("/usr/share/snapwebsites/html/snapmanager/snapmanagercgi-login.html");
            if(!login_page.read_all())
            {
                return error(
                          "500 Internal Server Error"
                        , "An internal error occurred."
                        , "Could not load the login page from /usr/share/snapwebsites/html/snapmanager/snapmanagercgi-login.html");
            }
            std::string cookie;
            if(logout)
            {
                // delete the cookie on the client side when logging out
                //
                cookie += "Set-Cookie: snapmanager=void; Expires=Thu, 01-Jan-1970 00:00:01 GMT; Path=/; Secure; HttpOnly\n";
            }
            std::string login_html(login_page.get_content());
            boost::replace_all(login_html, "@error@", error_msg);
            std::cout   //<< "Status: 200 OK"                         << std::endl
                        << "Expires: Sat, 1 Jan 2000 00:00:00 GMT"   << std::endl
                        << "Connection: close"                       << std::endl
                        << "Content-Type: text/html; charset=utf-8"  << std::endl
                        << "Content-Length: " << login_html.length() << std::endl
                        << cookie
                        << "X-Powered-By: snapmanager.cgi"           << std::endl
                        << std::endl
                        << login_html;

            // it worked--return 2
            return 2;
        };

    auto read_user_info = [&](std::string const & user_name, std::map<std::string, std::string> & user_info)
        {
            user_info.clear();

            snap::file_content user_ref(get_session_path() + "/" + user_name + ".user");
            if(user_ref.read_all())
            {
                std::string const content(user_ref.get_content());

                std::vector<std::string> lines;
                snap::NOTUSED(snap::tokenize_string(lines, content, "\n", true, " "));

                for(auto line : lines)
                {
                    std::vector<std::string> name_value;
                    snap::NOTUSED(snap::tokenize_string(name_value, line, ":", false, " "));
                    if(name_value.size() != 2)
                    {
                        return error(
                                  "500 Internal Server Error"
                                , "User session reference is invalid."
                                , "A line was not exactly composed of a field name and value.");
                    }
                    user_info[name_value[0]] = name_value[1];
                }
            }
            return 0;
        };

    auto write_user_info = [&](std::string const & user_name, std::map<std::string, std::string> & user_info)
        {
            std::ofstream user_file;
            user_file.open(get_session_path(true) + "/" + user_name + ".user");
            if(!user_file.is_open())
            {
                return error(
                          "500 Internal Server Error"
                        , "Could not save user session information."
                        , "The system could not open the user session information file.");
            }
            for(auto f : user_info)
            {
                user_file << f.first << ": " << f.second << "\n";
            }
            return 0;
        };

    auto setup_cookie = [&](std::string const & session_id)
        {
            // we are logged in and session_id needs to be saved in the cookie
            //
            // TODO: add the domain, which should come from the .conf
            //
            // Note: session_id is a set of hexadecimal digits so it is safe to
            //       save it as is in the cookie
            //
            // Note: we add 5 min. to the duration so the age on the client side
            //       can be a bit off and we should not inadvertendly lose the
            //       connection
            //
            f_cookie = "Set-Cookie: snapmanager=" + session_id
                     + "; Max-Age=" + std::to_string(session_duration + 300)
                     + "; Path=/; Secure; HttpOnly\n";
        };

    // try to log the user in on a POST
    // verify whether the user is logged in on a GET
    // if not logged in, display the login form
    //
    std::string session_id;

    bool const logout(f_uri.has_query_option("logout"));
    if(request_method == "POST" && !logout)
    {
        // check whether this is a log in attempt or another POST
        //
        auto const & user_login_it(f_post_variables.find("user_login"));
        auto const & user_name_it(f_post_variables.find("user_name"));
        auto const & user_password_it(f_post_variables.find("user_password"));

        if(user_login_it    != f_post_variables.end()
        && user_name_it     != f_post_variables.end()
        && user_password_it != f_post_variables.end())
        {
            SNAP_LOG_TRACE("Received data from logging form, processing...");

            f_user_name = user_name_it->second;
            std::string const user_password(user_password_it->second);

            // check whether the user is already blocked
            //
            int const hit_result(is_ip_blocked());
            if(hit_result != 0)
            {
                return hit_result;
            }

            // check that the user exists and that the password is correct
            // for that user
            //
            // for that to work, we use the snappassword tool which can become
            // root on a --check command
            //
            // note that this is not 100% secure since the user password will
            // appear in the list of command line arguments
            //
            // Note: the snappassword.log file is created by the postinst
            //       and kept alive by logrotate as required
            //
            std::string const command("snappassword --check --username "
                                     + f_user_name
                                     + " --password "
                                     + user_password
                                     + " >>/var/log/snapwebsites/secure/snappassword.log 2>&1");
            int const r(system(command.c_str()));
            if(r != 0)
            {
                if(WIFEXITED(r)
                && WEXITSTATUS(r) == 2)
                {
                    // WARNING: The log statement uses the "secure" version
                    //          because the "user_name" variable could
                    //          include the user's password (it happens
                    //          that people space out and type their
                    //          password in the user_name field...)
                    //
                    SNAP_LOG_INFO(snap::logging::log_security_t::LOG_SECURITY_SECURE)
                                 ("Credential check failed. User \"")
                                 (f_user_name)
                                 ("\" will not be logged in.");

                    increase_hit_count();

                    // wait a little to slow the client a bit
                    //
                    sleep(10 * f_login_attempts);

                    // invalid credentials
                    //
                    // TODO: somehow we should show an error of some sort...
                    //
                    return login_form("Invalid credentials. Please try again.");
                }

                // we don't increase the hit counter in this case since
                // the user may have used the correct password
                //
                return error(
                          "500 Internal Server Error"
                        , "An internal error occurred."
                        , "Somehow the snappassword command failed.");
            }

            // the user knows his password, forget the hit counter for that IP
            //
            delete_hit_file();

            // user credentials were accepted, generate a session and a cookie
            //
            // loop until we get a unique session ID, it should be really rare
            // since the session_id is 16 bytes
            //
            // Note: we use open(2) so we can have the O_EXCL & O_CREAT flags
            //       used together to avoid any kind of race condition
            //
            std::string session_path;
            int session_fd(-1);
            int count(10); // do a max. of 10 attempts, it should never take more than 2 to succeed
            do
            {
                --count;
                if(count <= 0)
                {
                    return error(
                              "500 Internal Server Error"
                            , "An internal error occurred."
                            , "Failed creating a unique session filename.");
                }
                unsigned char buf[16];
                int const rs(RAND_bytes(buf, sizeof(buf)));
                if(rs != 1)
                {
                    return error(
                              "500 Internal Server Error"
                            , "Could not generate a session number."
                            , "Somehow RAND_bytes() failed.");
                }
                session_id = snap::bin_to_hex(std::string(reinterpret_cast<char *>(buf), sizeof(buf)));
                session_path = get_session_path(true) + "/" + session_id + ".session";

                session_fd = open(session_path.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0700);
            }
            while(session_fd == -1);
            std::unique_ptr<int, decltype(&close_file)> raii_session_fd(&session_fd, close_file);

            // check whether a user reference already exists, if so delete the
            // old session
            //
            std::map<std::string, std::string> user_info;
            if(read_user_info(f_user_name, user_info) != 0)
            {
                return 1;
            }

            if(user_info.find("Session") != user_info.end())
            {
                std::string const old_session_id(user_info["Session"]);
                unlink((get_session_path(false) + "/" + old_session_id + ".session").c_str());
            }

            // clear in case we make changes with various version it is safer
            //
            user_info.clear();
            user_info["Session"] = session_id;
            std::string const now(std::to_string(time(nullptr)));
            user_info["Date"] = now;
            user_info["Last-Access"] = now;

            if(write_user_info(f_user_name, user_info) != 0)
            {
                return 1;
            }

            // the session file is just the user name
            // we just add a newline for courtesy
            //
            std::string const line(f_user_name + "\n");
            if(::write(session_fd, line.c_str(), line.length()) != static_cast<ssize_t>(line.length()))
            {
                raii_session_fd.reset();
                unlink(session_path.c_str());

                return error(
                          "500 Internal Server Error"
                        , "Could not properly log you in."
                        , "The write to the session file failed.");
            }

            // "forget" about the POST, it is not compatible with the other
            // posts and we already used up the data here
            //
            request_method = "GET";

            SNAP_LOG_INFO("User \"")
                         (f_user_name)
                         ("\" is logged in.");

            setup_cookie(session_id);
            return 0;
        }
    }

    if(request_method == "GET" || request_method == "POST")
    {
        // the GET must have a cookie or we immediately display the login form
        //
        char const * http_cookies(getenv("HTTP_COOKIE"));
        if(http_cookies == nullptr)
        {
            // no cookies, the user is not logged in yet, present the login screen
            //
            // Note that we reach here even on a POST or HEAD...
            //
            return login_form();
        }

        // we have cookies, make sure one of them is our cookie
        // and if so, check whether the session is still valid
        //
        std::vector<std::string> cookies;
        snap::NOTUSED(snap::tokenize_string(cookies, http_cookies, ";", true, " "));

        // TBD: could we use the snap_uri() class to handle the raw cookie data?

        for(auto c : cookies)
        {
            std::vector<std::string> name_value;
            snap::NOTUSED(snap::tokenize_string(name_value, c, "=", true, " "));
            if(name_value.size() != 2)
            {
                continue;
            }

            std::string const cookie_name(snap::snap_uri::urldecode(QString::fromUtf8(name_value[0].c_str()), true).toUtf8().data());
            if(cookie_name != "snapmanager")
            {
#ifdef _DEBUG
                SNAP_LOG_TRACE("Found cookie \"")(cookie_name)("\", ignore.");
#endif
                continue;
            }

            SNAP_LOG_INFO("Found \"snapmanager\" cookie. Checking validity.");

            // we found out cookie, get the value (i.e. session ID)
            //
            std::string const attempt_session_id(snap::snap_uri::urldecode(QString::fromUtf8(name_value[1].c_str()), true).toUtf8().data());
            if(attempt_session_id.length() != 16 * 2)
            {
                // invalid cookie
                //
                SNAP_LOG_WARNING(snap::logging::log_security_t::LOG_SECURITY_SECURE)
                                ("Cookie value (")
                                (attempt_session_id)
                                (") is not exactly 32 characters as expected.");
                break;
            }

            // this is the correct length
            //
            try
            {
                // verify that it is hexadecimal, but we do not care about
                // the binary code here
                //
                snap::NOTUSED(snap::hex_to_bin(attempt_session_id));

                std::string const session_filename(get_session_path(false) + "/" + attempt_session_id + ".session");
                snap::file_content session_data(session_filename);
                if(!session_data.read_all())
                {
                    // invalid cookie
                    //
                    SNAP_LOG_WARNING(snap::logging::log_security_t::LOG_SECURITY_SECURE)
                                    ("No session corresponds to cookie \"")
                                    (attempt_session_id)
                                    ("\".");
                    break;
                }
                f_user_name = session_data.get_content();
                if(f_user_name.empty())
                {
                    // invalid cookie
                    //
                    SNAP_LOG_WARNING(snap::logging::log_security_t::LOG_SECURITY_SECURE)
                                    ("File of session \"")
                                    (attempt_session_id)
                                    ("\" is empty.");
                    break;
                }

                // lose the ending '\n' if present
                //
                if(f_user_name[f_user_name.length() - 1] == '\n')
                {
                    f_user_name.resize(f_user_name.length() - 1);
                }

                // with the user name we can read the user file
                // and make sure the session is still valid by
                // checking the date
                //
                std::map<std::string, std::string> user_info;
                if(read_user_info(f_user_name, user_info) != 0)
                {
                    // invalid cookie
                    //
                    SNAP_LOG_WARNING(snap::logging::log_security_t::LOG_SECURITY_SECURE)
                                    ("File of session \"")
                                    (attempt_session_id)
                                    ("\" references user \"")
                                    (f_user_name)
                                    ("\" who does not have a corresponding user file.");
                    return 1;
                }

                if(user_info.find("Session") == user_info.end()
                || user_info.find("Last-Access") == user_info.end())
                {
                    // invalid cookie
                    //
                    SNAP_LOG_WARNING(snap::logging::log_security_t::LOG_SECURITY_SECURE)
                                    ("User file of \"")
                                    (f_user_name)
                                    ("\" is missing some information (no Session or Last-Access field found.)");
                    break;
                }
                std::string const existing_session_id(user_info["Session"]);
                if(existing_session_id != attempt_session_id)
                {
                    // invalid cookie
                    //
                    SNAP_LOG_WARNING(snap::logging::log_security_t::LOG_SECURITY_SECURE)
                                    ("User file for \"")
                                    (f_user_name)
                                    ("\" has session \"")
                                    (existing_session_id)
                                    ("\" and the cookie we received has session \"")
                                    (attempt_session_id)
                                    ("\".");
                    break;
                }

                if(logout)
                {
                    // void the session
                    //
                    user_info["Last-Access"] = "0";
                    if(write_user_info(f_user_name, user_info) != 0)
                    {
                        SNAP_LOG_ERROR("Could not save the user \"")
                                      (f_user_name)
                                      ("\" new Last-Access information.");
                        return 1;
                    }

                    unlink(session_filename.c_str());
                    return login_form("Your were logged out.", true);
                }

                time_t const last_access(boost::lexical_cast<time_t>(user_info["Last-Access"]));
                time_t const now(time(nullptr));
                if(now < last_access + session_duration)
                {
                    // user is still logged in (i.e. the session did not yet
                    // time out)
                    //
                    session_id = attempt_session_id;

                    // extend the session
                    //
                    user_info["Last-Access"] = std::to_string(now);
                    if(write_user_info(f_user_name, user_info) != 0)
                    {
                        SNAP_LOG_ERROR("Could not save the user \"")
                                      (f_user_name)
                                      ("\" new Last-Access information.");
                        return 1;
                    }
                }
                else
                {
                    SNAP_LOG_WARNING(snap::logging::log_security_t::LOG_SECURITY_SECURE)
                                    ("The session of user \"")
                                    (f_user_name)
                                    ("\" has expired.");

                    // session timed out, get rid of it
                    //
                    unlink(session_filename.c_str());

                    // in this case we want to inform the user why he
                    // is not logged in
                    //
                    return login_form("Your session timed out.");
                }
            }
            catch(snap::string_exception_invalid_parameter const &)
            {
                // conversion failed, not too surprising from a
                // tainted variable, ignore; user is not logged in
                //
                SNAP_LOG_ERROR("A session parameter is not valid.");
            }
            catch(boost::bad_lexical_cast const &)
            {
                // this should not happen... we convert a number from a
                // file that end users have no access to
                //
                SNAP_LOG_ERROR("The Last-Access parameter of some user is not a valid decimal number.");
            }

            // we only check the first cookie named "snapmanager"
            // whether it failed or not
            //
            break;
        }

        // if no session was defined, then the user is not logged in
        // so we show him the login form
        //
        if(session_id.empty())
        {
            SNAP_LOG_ERROR("Cookie auto-login failed. Offer login form again.");

            // there is no specific error in this case, it should not
            // happen unless some sort of error occurs
            //
            return login_form();
        }

        setup_cookie(session_id);
        return 0;
    }

    SNAP_LOG_FATAL("Request method is \"")(request_method)("\", which we currently refuse.");
    std::string const body("<html><head><title>Method Not Allowed</title></head><body><h1>Method Not Allowed</h1><p>Sorry. We only support GET and POST.</p></body></html>");
    std::cout   << "Status: 405 Method Not Allowed"         << std::endl
                << "Expires: Sat, 1 Jan 2000 00:00:00 GMT"  << std::endl
                << "Allow: GET, POST"                       << std::endl
                << "Connection: close"                      << std::endl
                << "Content-Type: text/html; charset=utf-8" << std::endl
                << "Content-Length: " << body.length()      << std::endl
                << "X-Powered-By: snapamanager.cgi"         << std::endl
                << std::endl
                << body;
    return 1;
}


int manager_cgi::process_post()
{
    SNAP_LOG_TRACE("processing POST now!");

    // check that the plugin name is defined
    //
    auto const & plugin_name_it(f_post_variables.find("plugin_name"));
    if(plugin_name_it == f_post_variables.end())
    {
        return error("400 Bad Request", "The POST is expected to include a plugin_name variable.", nullptr);
    }
    QString const plugin_name(QString::fromUtf8(plugin_name_it->second.c_str()));

    // determine which button was clicked
    //
    std::vector<std::string> const button_names{
                "status",
                "save",
                "save_everywhere",
                "restore_default",
                "install",
                "uninstall",
                "reboot",
                "upgrade",
                "upgrade_everywhere",
                "refresh",
                "restart",
                "restart_everywhere"
            };
    auto const & button_it(std::find_first_of(
                f_post_variables.begin(), f_post_variables.end(),
                button_names.begin(), button_names.end(),
                [](auto const & a, auto const & b)
                {
                    // WARNING: the button is the variable name, the value
                    //          for a button is "" anyway
                    return a.first == b;
                }));
    if(button_it == f_post_variables.end())
    {
        return error("400 Bad Request", "The POST did not include a button as expected.", nullptr);
    }
    // WARNING: the button is the variable name, the value
    //          for a button is "" anyway
    QString const button_name(QString::fromUtf8(button_it->first.c_str()));

    // we need the plugins for the following test
    //
    load_plugins();

    // we should be able to find that plugin by name
    //
    snap::plugins::plugin * p(snap::plugins::get_plugin(plugin_name));
    if(p == nullptr)
    {
        return error("404 Plugin Not Found", ("Plugin \"" + plugin_name_it->second + "\" was not found. We cannot process this request.").c_str(), nullptr);
    }

    // check that the field name is defined
    //
    auto const & field_name_it(f_post_variables.find("field_name"));
    if(field_name_it == f_post_variables.end())
    {
        return error("400 Bad Request", "The POST is expected to include a field_name variable.", nullptr);
    }
    QString const field_name(field_name_it->second.c_str());

    // check that we have a host variable
    //
    auto const & host_it(f_post_variables.find("hostname"));
    if(host_it == f_post_variables.end())
    {
        return error("400 Bad Request", "The POST is expected to include a hostname variable.", nullptr);
    }
    QString const host(QString::fromUtf8(host_it->second.c_str()));

    // got the host variable, make sure we can load a file from it
    //
    server_status status_file(f_cluster_status_path, host);
    if(!status_file.read_all())
    {
        return error("404 Host Not Found", ("Host \"" + host_it->second + "\" is not known.").c_str(), nullptr);
    }

    // make sure that host is viewed as UP, otherwise we will not be
    // able to send it a message
    //
    if(status_file.get_field_state("header", "status") == snap_manager::status_t::state_t::STATUS_STATE_UNDEFINED)
    {
        return error("500 Internal Server Error"
                    , ("Host \""
                      + host_it->second
                      + "\" has not header::status field defined.").c_str()
                    , nullptr);
    }
    QString const host_status(status_file.get_field("header", "status"));
    if(host_status != "up")
    {
        return error("503 Service Unavailable"
                    , ("Host \""
                      + host_it->second
                      + "\" is "
                      + host_status.toUtf8().data()
                      + ".").c_str()
                    , nullptr);
    }

    // check that the field being updated exists on that host,
    // otherwise the plugin cannot do anything with it
    //
    // Note: "self::refresh" is a special case and no field actually
    //       exists in the status file for that one
    //
    //       "self::upgrade_required" disappears once the upgrade is
    //       done so we have to also manage it as a special case
    //
    if(plugin_name != "self"
    || (field_name != "refresh"
                && field_name != "upgrade_required"
                && field_name != "reboot_required"))
    {
        if(status_file.get_field_state(plugin_name, field_name) == snap_manager::status_t::state_t::STATUS_STATE_UNDEFINED)
        {
            return error("400 Bad Request"
                        , ("Host \""
                          + host_it->second
                          + "\" has no \""
                          + plugin_name_it->second
                          + "::"
                          + field_name_it->second
                          + "\" field defined.").c_str()
                        , nullptr);
        }
    }

    if( button_name == "status" )
    {
        // This is for checking, not for modifying, so do nothing else here.
        return 0;
    }

    // that very field should be defined in the POST variables
    //
    QString new_value;
    if(button_name == "save"
    || button_name == "save_everywhere")
    {
        auto const & new_value_it(f_post_variables.find(field_name_it->second));
        if(new_value_it == f_post_variables.end())
        {
            std::stringstream ss;
            ss << "Known variables from plugin \""
               << plugin_name
               << "\": ";
            bool add_comma(false);
            for(auto const & v : f_post_variables)
            {
                if(add_comma)
                {
                    ss << ", ";
                }
                else
                {
                    add_comma = true;
                }
                if(v.second.empty())
                {
                    ss << v.first << "=(empty)";
                }
                else
                {
                    ss << v.first << "=[" << v.second << "]";
                }
                if(v.second == field_name_it->second)
                {
                    ss << " (*)";
                }
            }
            return error("400 Bad Request"
                       , ("Variable \""
                         + field_name_it->second
                         + "\" was not found in this POST.").c_str()
                       , ss.str().c_str());
        }
        new_value = QString::fromUtf8(new_value_it->second.c_str());
    }
    // else -- install, the value is the field_name
    //      -- uninstall, the value is the field_name
    //      -- restore_default, the value is the default, whatever that might be
    //      -- reboot, the value is the button and server name

    // get the old value
    //
    QString const old_value(status_file.get_field(plugin_name, field_name));

    // although not 100% correct, we immediately update the field with
    // the new value but mark it as MODIFIED, since we do that before we
    // send the MODIFIYSETTINGS message, we at least know that another
    // update should happen and "fix" the status back to something else
    // than MODIFIED
    //
    if(plugin_name != "self"
    || field_name != "refresh")
    {
        snap_manager::status_t const modified(snap_manager::status_t::state_t::STATUS_STATE_MODIFIED, plugin_name, field_name, new_value);
        status_file.set_field(modified);
        status_file.write();
    }

    // retrieve installation variables which can be numerous
    //
    std::string install_variables;
    std::for_each(
            f_post_variables.begin(),
            f_post_variables.end(),
            [&install_variables](auto const & it)
            {
                if(it.first.compare(0, 22, "bundle_install_field::") == 0)
                {
                    if(!install_variables.empty())
                    {
                        install_variables += '\n';
                    }
                    install_variables += it.first.substr(22) + "=" + it.second;
                }
            });

    // we got all the elements, send a message because we may have to
    // save that data on multiple computers and also it needs to be
    // applied by snapmanagerdaemon and not us (i.e. snapmanagerdaemon
    // runs as root:root and thus it can modify settings and install
    // or remove software, whereas snapmanager.cgi runs as www-data...)
    //
    {
        // setup the message to send to other snapmanagerdaemons
        //
        snap::snap_communicator_message modify_settings;
        if(button_name == "save_everywhere"
        || button_name == "upgrade_everywhere"
        || button_name == "restart_everywhere")
        {
            // save everywhere means sending to all snapmanagerdaemons
            // anywhere in the cluster
            //
            // the upgrade_everywhere will first run an update then an
            // upgrade so it will upgrade any computer that's not 100%
            // up to date in one go (WARNING: this is not what we want
            // in the end but for now, that's really practical!)
            //
            modify_settings.set_service("*");
        }
        else
        {
            // our local snapmanagerdaemon only
            //
            modify_settings.set_server(host);
            modify_settings.set_service("snapmanagerdaemon");
        }
        modify_settings.set_command("MODIFYSETTINGS");
        modify_settings.add_parameter("plugin_name", plugin_name);
        modify_settings.add_parameter("field_name", field_name);
        modify_settings.add_parameter("old_value", old_value);
        modify_settings.add_parameter("new_value", new_value);
        modify_settings.add_parameter("button_name", button_name);
        if(!install_variables.empty())
        {
            modify_settings.add_parameter("install_values", QString::fromUtf8(install_variables.c_str()));
        }

SNAP_LOG_TRACE("msg.run()");
        // we need to quickly create a connection for that one...
        //
        messenger msg(f_communicator_address, f_communicator_port, modify_settings);
        msg.run();
SNAP_LOG_TRACE("msg.run() finished");
    }

    return 0;
}


/** \brief Generate the body of the page.
 *
 * This function checks the various query strings passed to the manager_cgi
 * and depending on those, generates a page.
 *
 * \param[in] doc  The document where we save the results.
 * \param[in] output  The output tag.
 * \param[in] menu  The menu tag.
 */
bool manager_cgi::generate_content_impl(QDomDocument doc, QDomElement root, QDomElement output, QDomElement menu, snap::snap_uri const & uri)
{
    snap::NOTUSED(output);
    snap::NOTUSED(uri);

    // is a host name specified?
    //
    if(f_uri.has_query_option("host"))
    {
        QString const host(f_uri.query_option("host"));

        // either way, if we are here, we can show two additional menus:
        //    host status
        //    installation bundles
        //
        QDomElement item(doc.createElement("item"));
        item.setAttribute("href", "?host=" + host);
        menu.appendChild(item);
        QDomText text(doc.createTextNode("Host Status"));
        item.appendChild(text);

        QDomElement status(doc.createElement("status"));
        menu.appendChild(status);
        status.appendChild(doc.createTextNode(QString("(Host: %1)").arg(host)));

        QDomElement title(doc.createElement("title"));
        root.appendChild(title);
        title.appendChild(doc.createTextNode(QString("Snap! Manager (%1)").arg(host)));
    }
    else
    {
        QDomElement status(doc.createElement("status"));
        menu.appendChild(status);
        status.appendChild(doc.createTextNode("(Select Host)"));

        QDomElement title(doc.createElement("title"));
        root.appendChild(title);
        title.appendChild(doc.createTextNode("Snap! Manager"));
    }

    return true;
}



void manager_cgi::generate_content_done(QDomDocument doc, QDomElement root, QDomElement output, QDomElement menu, snap::snap_uri const & uri)
{
    snap::NOTUSED(root);
    snap::NOTUSED(menu);

    // did one of the plugins generate the output?
    // if so then we have nothing to do here
    //
    // Note that we expect plugins to generate some output ONLY if they
    // understand the Query String function parameter. If that parameter
    // is not set then it should not do anything (although it could still
    // generate a menu entry.)
    //
    // QString const function(f_uri.query_option("function"));
    //
    if(!output.firstChild().isNull())
    {
        return;
    }

    // is a host name specified?
    // if so then the function / page has to be applied to that specific host
    //
    if(f_uri.has_query_option("host"))
    {
        // note: uri and f_uri are the same, we use uri to avoid the
        //       "not used" warning
        //
        QString const host(uri.query_option("host"));
        get_host_status(doc, output, host);
    }
    else
    {
        // no host specified, if there is a function it has to be applied
        // to all computers, otherwise show the list of computers and their
        // basic status
        //
        get_cluster_status(doc, output);
    }
}


QDomElement manager_cgi::create_table_header( QDomDocument& doc )
{
    // output/table
    QDomElement table(doc.createElement("table"));
    table.setAttribute("class", "server-status");

    // output/table/tr
    QDomElement tr(doc.createElement("tr"));
    table.appendChild(tr);

    // output/table/tr/th[1]
    QDomElement th(doc.createElement("th"));
    tr.appendChild(th);

    QDomText text(doc.createTextNode(QString("Name")));
    th.appendChild(text);

    // output/table/tr/th[3]
    th = doc.createElement("th");
    tr.appendChild(th);

    text = doc.createTextNode(QString("State"));
    th.appendChild(text);

    // output/table/tr/th[4]
    th = doc.createElement("th");
    tr.appendChild(th);

    text = doc.createTextNode("Value");
    th.appendChild(text);

    return table;
}


void manager_cgi::generate_self_refresh_plugin_entry( QDomDocument & doc, QDomElement & table )
{
    // add a "special" field so one can do a Refresh
    //
    snap::plugins::plugin * p(snap::plugins::get_plugin("self"));

    // output/table/tr
    QDomElement tr(doc.createElement("tr"));
    table.appendChild(tr);

    // output/table/tr/td[2]
    QDomElement td(doc.createElement("td"));
    tr.appendChild(td);

    QDomText text(doc.createTextNode("refresh"));
    td.appendChild(text);

    // output/table/tr/td[3]
    td = doc.createElement("td");
    tr.appendChild(td);

    text = doc.createTextNode("valid");
    td.appendChild(text);

    // output/table/tr/td[4]
    td = doc.createElement("td");
    tr.appendChild(td);

    plugin_base * pb(dynamic_cast<plugin_base *>(p));
    if(pb != nullptr)
    {
        // call that signal directly on that one plugin
        //
        snap_manager::status_t const refresh_status(
                    snap_manager::status_t::state_t::STATUS_STATE_INFO,
                    "self",
                    "refresh",
                    "");
        pb->display_value(td, refresh_status, f_uri);
    }
}


void manager_cgi::generate_plugin_entry( snap_manager::status_t status, QDomDocument & doc, QDomElement & table )
{
    QString const & plugin_name(status.get_plugin_name());
    QString const & field_name(status.get_field_name());

    snap::plugins::plugin * p(snap::plugins::get_plugin(plugin_name));

    // output/table/tr
    QDomElement tr(doc.createElement("tr"));
    table.appendChild(tr);

    snap::snap_string_list tr_classes;
    if(p == nullptr)
    {
        tr_classes << "missing-plugin";
    }

    snap_manager::status_t::state_t const state(status.get_state());
    switch(state)
    {
    case snap_manager::status_t::state_t::STATUS_STATE_MODIFIED:
        tr_classes << "modified";
        break;

    case snap_manager::status_t::state_t::STATUS_STATE_HIGHLIGHT:
        tr_classes << "highlight";
        break;

    case snap_manager::status_t::state_t::STATUS_STATE_WARNING:
        tr_classes << "warnings";
        break;

    case snap_manager::status_t::state_t::STATUS_STATE_ERROR:
    case snap_manager::status_t::state_t::STATUS_STATE_FATAL_ERROR:
        tr_classes << "errors";
        break;

    default:
        // do nothing otherwise
        break;

    }
    if(!tr_classes.isEmpty())
    {
        tr.setAttribute("class", tr_classes.join(" "));
    }
    tr.setAttribute("id", QString("%1::%2").arg(plugin_name).arg(field_name));

    // output/table/tr/td[2]
    QDomElement td(doc.createElement("td"));
    tr.appendChild(td);

    QDomText text(doc.createTextNode(field_name));
    td.appendChild(text);

    // output/table/tr/td[3]
    td = doc.createElement("td");
    tr.appendChild(td);

    QString field_state("???");
    switch(state)
    {
    case snap_manager::status_t::state_t::STATUS_STATE_UNDEFINED:
        field_state = "undefined";
        break;

    case snap_manager::status_t::state_t::STATUS_STATE_DEBUG:
        field_state = "debug";
        break;

    case snap_manager::status_t::state_t::STATUS_STATE_INFO:
        field_state = "valid";
        break;

    case snap_manager::status_t::state_t::STATUS_STATE_MODIFIED:
        field_state = "modified";
        break;

    case snap_manager::status_t::state_t::STATUS_STATE_HIGHLIGHT:
        field_state = "highlight";
        break;

    case snap_manager::status_t::state_t::STATUS_STATE_WARNING:
        field_state = "warning";
        break;

    case snap_manager::status_t::state_t::STATUS_STATE_ERROR:
        field_state = "error";
        break;

    case snap_manager::status_t::state_t::STATUS_STATE_FATAL_ERROR:
        field_state = "fatal error";
        break;

    }
    text = doc.createTextNode(field_state);
    td.appendChild(text);

    // output/table/tr/td[4]
    td = doc.createElement("td");
    tr.appendChild(td);

    bool managed(false);
    plugin_base * pb(dynamic_cast<plugin_base *>(p));
    if(pb != nullptr
    && state != snap_manager::status_t::state_t::STATUS_STATE_MODIFIED)
    {
        // call that signal directly on that one plugin
        //
        managed = pb->display_value(td, status, f_uri);
    }

    if(!managed)
    {
        text = doc.createTextNode(status.get_value());
        td.appendChild(text);
    }
}


void manager_cgi::generate_plugin_status
    ( QDomDocument& doc
    , QDomElement& output
    , QString const & plugin_name
    , status_list_t const & status_list
    , QString const & alerts
    )
{
    QDomElement div(doc.createElement("div"));
    div.setAttribute( "id", plugin_name );
    if( !alerts.isEmpty() )
    {
        div.setAttribute( "class", alerts );
    }
    output.appendChild(div);

    generate_plugin_status(doc, div, plugin_name, status_list);
}


void manager_cgi::generate_plugin_status
    ( QDomDocument& doc
    , QDomElement& output
    , QString const & plugin_name
    , status_list_t const & status_list
    )
{
    QDomElement table( create_table_header( doc ) );
    output.appendChild(table);

    if( plugin_name == "self" )
    {
        // add a "special" field so one can do a Refresh, at the top of the list
        //
        generate_self_refresh_plugin_entry( doc, table );
    }

    for( auto const &status : status_list )
    {
        generate_plugin_entry( status, doc, table );
    }
}


void manager_cgi::get_status_map( QString const & host, status_map_t & map )
{
    // create, open, read the file
    //
    server_status file(f_cluster_status_path, host);
    if(!file.read_all())
    {
        // TODO: add error info in output
        return;
    }

    // we need the plugins for the following (non-raw) loop
    //
    load_plugins();

    for(auto const & s : file.get_statuses())
    {
        QString const & plugin_name(s.second.get_plugin_name());
        if( plugin_name == "header" )
        {
            continue; // avoid the "header" plugins, since we cannot modify those statuses anyway
        }
        map[plugin_name].push_back( s.second );
    }
}


void add_state_class_name( QStringList& list, snap_manager::status_t::state_t const state )
{
    switch(state)
    {
    case snap_manager::status_t::state_t::STATUS_STATE_MODIFIED:
        list << "modified";
        break;

    case snap_manager::status_t::state_t::STATUS_STATE_HIGHLIGHT:
        list << "highlight";
        break;

    case snap_manager::status_t::state_t::STATUS_STATE_WARNING:
        list << "warnings";
        break;

    case snap_manager::status_t::state_t::STATUS_STATE_ERROR:
    case snap_manager::status_t::state_t::STATUS_STATE_FATAL_ERROR:
        list << "errors";
        break;

    default:
        // do nothing otherwise
        break;
    }
}


void manager_cgi::get_host_status(QDomDocument doc, QDomElement output, QString const & host)
{
    // Make a map of all of the status-to-plugins.
    // get_status_map() loads the plugins for us
    //
    std::map<QString, status_list_t> status_map;
    get_status_map( host, status_map );

    // Sort self first, then respect the order of the rest of the map
    // (self should always be there unless the 'host' file is missing)
    //
    std::vector<status_list_t> ordered_statuses;
    if(status_map.find("self") != status_map.end())
    {
        ordered_statuses.push_back(status_map["self"]);
    }
    for( auto const & s : status_map )
    {
        if( s.first == "self" )
        {
            continue;
        }
        ordered_statuses.push_back(s.second);
    }

    // add the dynamic title
    //
    QDomElement h1(doc.createElement("h1"));
    QDomText title(doc.createTextNode(QString("Snap! Manager (%1)").arg(host)));
    h1.appendChild(title);
    output.appendChild(h1);

    // add the <div>, the <ul> appears inside that <div>
    //
    QDomElement div(doc.createElement("div"));
    div.setAttribute("id", "tabs");
    output.appendChild(div);

    // Create <ul>...</ul> "menu" at the top. jQuery::tabs will turn this into
    // the tab button list.
    //
    // The 'self' plugin is always first.
    //
    std::map<QString, QString> alerts;
    QDomElement ul(doc.createElement("ul"));
    div.appendChild(ul);
    for(auto const & s : ordered_statuses)
    {
        QString const plugin_name(s[0].get_plugin_name());
        QDomElement li(doc.createElement("li"));
        ul.appendChild(li);
        //
        QDomElement a(doc.createElement("a"));
        a.setAttribute( "href", QString("#%1").arg(plugin_name) );
        QDomText text(doc.createTextNode(plugin_name));
        a.appendChild(text);
        li.appendChild(a);

        QStringList alert_classes;
        for( auto const & st : s )
        {
            add_state_class_name( alert_classes, st.get_state() );
        }
        //
        if( !alert_classes.isEmpty() )
        {
            alert_classes.removeDuplicates();
            QString const classes(alert_classes.join(" "));
            alerts[plugin_name] = classes;
            li.setAttribute( "class", classes );
        }
    }

    // Now put in the table entries
    //
    for(auto const & s : ordered_statuses)
    {
        QString const plugin_name(s[0].get_plugin_name());
        generate_plugin_status( doc, div, plugin_name, s, alerts[plugin_name] );
    }
}


void manager_cgi::get_cluster_status(QDomDocument doc, QDomElement output)
{
    snap::glob_dir the_glob;

    bool has_error(false);
    //
    QString err_msg;
    try
    {
        the_glob.set_path( QString("%1/*.db").arg(f_cluster_status_path).toUtf8().data(), GLOB_NOESCAPE, true );
    }
    catch( std::exception const & x )
    {
        err_msg = QString("An error [%1] occurred while reading status data. "
                          "Please check your snapmanagercgi.log file for more information.").arg(x.what());
        SNAP_LOG_ERROR("Exception caught! what=[")(x.what())("]");
    }
    catch( ... )
    {
        err_msg = "An unknown exception occurred while reading status data. "
                  "Please check your snapmanagercgi.log file for more information.";
        SNAP_LOG_ERROR("Caught unknown exception!");
        has_error = true;
    }
    //
    if( has_error )
    {
        QDomText text( doc.createTextNode(err_msg) );
        output.appendChild(text);
        return;
    }

    // output/table
    QDomElement table(doc.createElement("table"));
    output.appendChild(table);

    table.setAttribute("class", "cluster-status");

    // output/table/tr
    QDomElement tr(doc.createElement("tr"));
    table.appendChild(tr);

    // output/table/tr/th[1]
    QDomElement th(doc.createElement("th"));
    tr.appendChild(th);

    QDomText text(doc.createTextNode("Host"));
    th.appendChild(text);

    // output/table/tr/th[2]
    th = doc.createElement("th");
    tr.appendChild(th);

    text = doc.createTextNode("IP");
    th.appendChild(text);

    // output/table/tr/th[3]
    th = doc.createElement("th");
    tr.appendChild(th);

    text = doc.createTextNode("Status");
    th.appendChild(text);

    // output/table/tr/th[4]
    th = doc.createElement("th");
    tr.appendChild(th);

    text = doc.createTextNode("Err/War");
    th.appendChild(text);

    // output/table/tr/th[5]
    th = doc.createElement("th");
    tr.appendChild(th);

    text = doc.createTextNode("Last Updated");
    th.appendChild(text);

    auto handle_path = [&]( QString path )
    {
        server_status file(path);
        if(file.read_header())
        {
            // we got what looks like a valid status file
            //
            QString const status(file.get_field("header", "status"));
            if(!status.isEmpty())
            {
                // get number of errors
                //
                size_t error_count(0);
                if(file.get_field_state("header", "errors") != snap_manager::status_t::state_t::STATUS_STATE_UNDEFINED)
                {
                    QString const errors(file.get_field("header", "errors"));
                    error_count = errors.toLongLong();
                }

                // get number of warnings
                //
                size_t warning_count(0);
                if(file.get_field_state("header", "warnings") != snap_manager::status_t::state_t::STATUS_STATE_UNDEFINED)
                {
                    QString const warnings(file.get_field("header", "warnings"));
                    warning_count = warnings.toLongLong();
                }

                // output/table/tr
                tr = doc.createElement("tr");
                table.appendChild(tr);

                snap::snap_string_list row_class;
                if(error_count != 0)
                {
                    row_class << "errors";
                }
                if(warning_count != 0)
                {
                    row_class << "warnings";
                }
                if(status == "down" || status == "unknown")
                {
                    ++error_count;  // we consider this an error, so do +1 here
                    row_class << "down";
                }
                if(!row_class.isEmpty())
                {
                    tr.setAttribute("class", row_class.join(" "));
                }

                // output/table/tr/td[1]
                QDomElement td(doc.createElement("td"));
                tr.appendChild(td);

                // output/table/tr/td[1]/a
                QDomElement anchor(doc.createElement("a"));
                td.appendChild(anchor);

                int basename_pos(path.lastIndexOf('/'));
                // basename_pos will be -1 which is what you would
                // expect to get for the mid() call below!
                //if(basename_pos < 0)
                //{
                //    // this should not happen, although it is perfectly
                //    // possible that the administrator used "" as the
                //    // path where statuses should be saved.
                //    //
                //    basename_pos = 0;
                //}
                QString const host(path.mid(basename_pos + 1, path.length() - basename_pos - 1 - 3));

                anchor.setAttribute("href", QString("?host=%1").arg(host));

                // output/table/tr/td[1]/<text>
                text = doc.createTextNode(host);
                anchor.appendChild(text);

                // output/table/tr/td[2]
                td = doc.createElement("td");
                tr.appendChild(td);

                // output/table/tr/td[2]/<text>
                text = doc.createTextNode(file.get_field("header", "ip"));
                td.appendChild(text);

                // output/table/tr/td[3]
                td = doc.createElement("td");
                tr.appendChild(td);

                // output/table/tr/td[3]/<text>
                text = doc.createTextNode(status);
                td.appendChild(text);

                // output/table/tr/td[4]
                td = doc.createElement("td");
                tr.appendChild(td);

                // output/table/tr/td[4]/<text>
                text = doc.createTextNode(QString("%1/%2").arg(error_count).arg(warning_count));
                td.appendChild(text);

                // get the date when it was last modified
                //
                time_t last_modification(0);
                struct stat s;
                if(stat(path.toUtf8().data(), &s) == 0)
                {
                    last_modification = s.st_mtim.tv_sec;
                }
                struct tm * t(localtime(&last_modification));
                char last_mod[1024];
                strftime(last_mod, sizeof(last_mod), "%Y/%m/%d  %H:%M:%S", t);

                // output/table/tr/td[5]
                td = doc.createElement("td");
                tr.appendChild(td);

                // output/table/tr/td[4]/<text>
                text = doc.createTextNode(QString::fromUtf8(last_mod));
                td.appendChild(text);
            }

            if(file.has_error())
            {
                has_error = true;
            }
        }
        else
        {
            has_error = true;
        }
    };

    the_glob.enumerate_glob( handle_path );

    if( has_error )
    {
        // output/p/<text>
        QDomElement p(doc.createElement("p"));
        output.appendChild(p);

        p.setAttribute("class", "error");

        text = doc.createTextNode("Errors occurred while reading the status. Please check your snapmanagercgi.log file for details.");
        p.appendChild(text);
    }
}



}
// namespace snap_manager
// vim: ts=4 sw=4 et
