// Snap Websites Server -- snap websites CGI function tests
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

//
// This is in part because there isn't any easy way to run a command
// from a shell without inheriting all the variables from the parent
// shell. This test can create an exact environment table and start
// a process with a very specific environment.
//

//
// The following is a sample environment from Apache2.
//
// # arguments
// argv[0] = "/usr/clients/www/alexis.m2osw.com/cgi-bin/env_n_args.cgi"
//
// # environment
// UNIQUE_ID=TtISeX8AAAEAAHhHi7kAAAAB
// HTTP_HOST=alexis.m2osw.com
// HTTP_USER_AGENT=Mozilla/5.0 (X11; Linux i686 on x86_64; rv:8.0.1) Gecko/20111121 Firefox/8.0.1 SeaMonkey/2.5
// HTTP_ACCEPT=text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8
// HTTP_ACCEPT_LANGUAGE=en-us,en;q=0.8,fr-fr;q=0.5,fr;q=0.3
// HTTP_ACCEPT_ENCODING=gzip, deflate
// HTTP_ACCEPT_CHARSET=ISO-8859-1,utf-8;q=0.7,*;q=0.7
// HTTP_CONNECTION=keep-alive
// HTTP_COOKIE=SESS8b653582e586f876284c0be25de5ac73=d32eb1fccf3f3f3beb5bc2b9439dd160; DRUPAL_UID=1
// HTTP_CACHE_CONTROL=max-age=0
// PATH=/usr/local/bin:/usr/bin:/bin
// SERVER_SIGNATURE=
// SERVER_SOFTWARE=Apache
// SERVER_NAME=alexis.m2osw.com
// SERVER_ADDR=192.168.1.1
// SERVER_PORT=80
// REMOTE_HOST=adsl-64-166-38-38.dsl.scrm01.pacbell.net
// REMOTE_ADDR=64.166.38.38
// DOCUMENT_ROOT=/usr/clients/www/alexis.m2osw.com/public_html/
// SERVER_ADMIN=alexis@m2osw.com
// SCRIPT_FILENAME=/usr/clients/www/alexis.m2osw.com/cgi-bin/env_n_args.cgi
// REMOTE_PORT=37722
// GATEWAY_INTERFACE=CGI/1.1
// SERVER_PROTOCOL=HTTP/1.1
// REQUEST_METHOD=GET
// QUERY_STRING=testing=environment&lang=en
// REQUEST_URI=/cgi-bin/env_n_args.cgi?testing=environment&lang=en
// SCRIPT_NAME=/cgi-bin/env_n_args.cgi
//

#include "snapwebsites/not_reached.h"

#include <map>
#include <string>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

typedef std::map<std::string, std::string> environ_t;
environ_t env;

int verbose(0);




void request(const char *progname)
{
    const char *p(NULL), *q(NULL);
    for(const char *s(progname); *s != '\0'; ++s)
    {
        if(*s == '/')
        {
            p = q;
            q = s;
        }
    }
    if(p == NULL)
    {
        fprintf(stderr, "error: process was not started as expected (full path or relative including at least 2 directories.)\n");
        exit(1);
    }
    // get the path including the last '/'
    std::string path(progname, p - progname + 1);
    std::string snapcgi(path);
    snapcgi += "src/snap.cgi";

    // prepare argv
    char **argv = new char *[2];
    argv[0] = strdup(snapcgi.c_str());
    argv[1] = NULL;

    // prepare environ for execve() call
    char **exec_environ = new char *[env.size() + 1];
    int i = 0;
    for(environ_t::const_iterator it(env.begin()); it != env.end(); ++it, ++i)
    {
        exec_environ[i] = strdup((it->first + "=" + it->second).c_str());
//fprintf(stderr, "env = [%s]\n", exec_environ[i]);
    }
    exec_environ[i] = NULL;

    // run snap.cgi with the correct environment
    execve(argv[0], argv, exec_environ);

    // if we return, an error occured
    fprintf(stderr, "error: could not start the snap.cgi tool.\n");
    exit(1);
}


void parse_url(const char *url)
{
    // extract the protocol
    if(strncmp("http://", url, 7) == 0)
    {
        // do nothing...
        url += 7;
        env["SERVER_PORT"] = "80"; // default port, can be changed
    }
    else if(strncmp("https://", url, 8) == 0)
    {
        env["HTTPS"] = "on";
        env["SERVER_PORT"] = "443"; // default port, can be changed
        url += 8;
    }
    else
    {
        fprintf(stderr, "error: the only supported protocols are HTTP and HTTPS.\n");
        exit(1);
    }

    // extract the domain
    const char *slash = strchr(url, '/');
    if(slash == NULL)
    {
        slash = url + strlen(url);
    }
    int len(static_cast<int>(slash - url));
    if(len == 0)
    {
        fprintf(stderr, "error: could not determine domain name, got 3 or more / after the protocol or domain name is missing?\n");
        exit(1);
    }
    std::string domain(url, len);

    // we got the domain, it may include a username, password, & port
    std::string name;
    std::string pass;
    std::string port;
    const char *d = domain.c_str();
    const char *u = strchr(d, '@');
    if(u != NULL)
    {
        // got a user, change the domain and extract the user/password info
        const char *p = strchr(d, ':');
        if(p > u)
        {
            name.assign(d, u - d);
            // no password...
        }
        else
        {
            name.assign(d, p - d);
            pass.assign(p + 1, u - p - 1);
            if(name.empty() ^ pass.empty())
            {
                fprintf(stderr, "error: when a name/password definition includes a ':', then both must be indicated (not empty).\n");
                exit(1);
            }
        }
        // use intermediate string in case the copy from itself wouldn't work
        std::string new_domain(u + 1);
        domain = new_domain;
    }
    // domain may have changed, get the pointer again
    d = domain.c_str();
    const char *p = strchr(d, ':');
    if(p != NULL)
    {
        // got a port
        port.assign(p + 1);
        if(port.empty())
        {
            fprintf(stderr, "error: port cannot be empty.\n");
            exit(1);
        }
        // remove port from domain name
        std::string new_domain(d, p - d);
        domain = new_domain;

        for(++p; *p != '\0'; ++p)
        {
            if(*p < '0' || *p > '9')
            {
                fprintf(stderr, "error: port must be a positive decimal number.\n");
                exit(1);
            }
        }
        if(atoi(port.c_str()) == 0)
        {
            fprintf(stderr, "error: port cannot be zero.\n");
            exit(1);
        }
    }

    env["HTTP_HOST"] = domain;
    if(!port.empty())
    {
        env["SERVER_PORT"] = port;
    }
    // which vars take the name/password?

    if(*slash == '/')
    {
        ++slash;
    }
    url = slash;

    // verify that the anchor was not specified
    const char *hash = strchr(url, '#');
    if(hash != NULL)
    {
        fprintf(stderr, "error: a server cannot be sent the anchor data.\n");
        exit(1);
    }

    // the URI includes the query string...
    env["REQUEST_URI"] = url;

    // check for a query string
    const char *query = strchr(url, '?');
    if(query != NULL)
    {
        // note that query + 1 may be ""
        env["QUERY_STRING"] = query + 1;
    }
    else
    {
        // do not define the QUERY_STRING when not specified
    }
}

void usage()
{
    fprintf(stderr, "Usage: test_snap_cgi [-opt] URL\n");
    fprintf(stderr, "  where -opt is one of (each flag must appear separately):\n");
    fprintf(stderr, "    -a <agent info>      The agent information\n");
    fprintf(stderr, "    -e <name>=<value>    Add an environment variable\n");
    fprintf(stderr, "    -h                   Print out this help screen\n");
    exit(1);
}

int main(int argc, char *argv[])
{
    // define some defaults before parsing the command line options so the user
    // can change the defaults as required by his tests

    env["DOCUMENT_ROOT"] = "/var/www/"; // at some point will change to CMAKE_BINARY_DIR/something-such-as-www
    env["GATEWAY_INTERFACE"] = "SNAP/1.0";
    env["HTTP_ACCEPT"] = "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8";
    env["HTTP_ACCEPT_CHARSET"] = "ISO-8859-1,utf-8;q=0.7,*;q=0.7";
    env["HTTP_ACCEPT_ENCODING"] = "gzip, deflate";
    env["HTTP_ACCEPT_LANGUAGE"] = "en-us,en;q=0.8,fr-fr;q=0.5,fr;q=0.3";
    env["HTTP_CACHE_CONTROL"] = "max-age=0";
    env["HTTP_CONNECTION"] = "keep-alive";
    //env["HTTP_COOKIE"] = ""; no cookie by default
    //env["HTTP_HOST"] = ...; comes from the URL
    env["HTTP_USER_AGENT"] = "Mozilla/5.0 (X11; Linux i686 on x86_64; rv:8.0.1) Gecko/20111121 Firefox/8.0.1 SeaMonkey/2.5";
    env["PATH"] = "/usr/local/bin:/usr/bin:/bin";
    //env["QUERY_STRING"] = ...; set from URL defined on command line if present (may still be empty)
    env["REMOTE_ADDR"] = "127.0.0.1";
    env["REMOTE_HOST"] = "user.example.com";
    env["REMOTE_PORT"] = "32222";
    env["REQUEST_METHOD"] = "GET";
    //env["REQUEST_URI"] = ...; set from URL defined on command line
    env["SCRIPT_FILENAME"] = "/bin/snapserver";
    env["SCRIPT_NAME"] = "Snap Server";
    env["SERVER_ADDR"] = "127.0.0.1";
    env["SERVER_ADMIN"] = "admin@example.com";
    env["SERVER_NAME"] = "www.example.com";
    //env["SERVER_PORT"] = ...; http:// = 80, https:// = 443, or URL port
    env["SERVER_PROTOCOL"] = "HTTP/1.1";
    env["SERVER_SIGNATURE"] = "Apache 2.2";
    env["SERVER_SOFTWARE"] = "Apache";

    static const char id_chars[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::string uniq;
    srand(static_cast<unsigned int>(time(nullptr)));
    for(int i(0); i < 24; ++i)
    {
        uniq += id_chars[rand() % (sizeof(id_chars) - 1)];
    }
    env["UNIQUE_ID"] = uniq;

    // parse user options
    bool help(false);
    bool got_url(false);
    for(int i(1); i < argc; ++i)
    {
        if(argv[i][0] == '-')
        {
            switch(argv[i][1])
            {
            case 'a':
                ++i;
                if(i >= argc || argv[i][0] == '-')
                {
                    fprintf(stderr, "error: -a is expected to be followed by the name of the agent.\n");
                    help = true;
                }
                else
                {
                    env["HTTP_USER_AGENT"] = argv[i];
                }
                break;

            case 'e':
                // adding an environment variable
                ++i;
                if(i >= argc || argv[i][0] == '-')
                {
                    fprintf(stderr, "error: -e is expected to be followed by an environment variable name and value.\n");
                    help = true;
                }
                else
                {
                    const char *value = strchr(argv[i], '=');
                    const char *end;
                    if(value == NULL)
                    {
                        // put a default value (should we err instead?)
                        value = "1";
                        end = value + strlen(value);
                    }
                    else
                    {
                        end = value;
                        ++value;
                    }
                    std::string name(argv[i], end - value);
                    env[name] = value;
                }
                break;

            case 'h':
                usage();
                snap::NOTREACHED();

            case 'v':
                for(const char *q = argv[i] + 1; *q != '\0'; ++q)
                {
                    if(*q != 'v')
                    {
                        fprintf(stderr, "error: invalid usage of the -v option.\n");
                        help = true;
                    }
                    ++verbose;
                }
                break;

            default:
                fprintf(stderr, "error: unknown option '%c'.\n", argv[i][1]);
                help = true;
                break;

            }
        }
        else if(!got_url)
        {
            got_url = true;
            parse_url(argv[i]);
        }
        else
        {
            fprintf(stderr, "error: only one URL is accepted per call.\n");
            help = true;
        }
    }
    if(help)
    {
        usage();
        snap::NOTREACHED();
    }
    if(!got_url)
    {
        fprintf(stderr, "error: no URL specified, it is mandatory.\n");
        usage();
        snap::NOTREACHED();
    }

    if(verbose > 2)
    {
        // show the environment
        for(environ_t::const_iterator it(env.begin()); it != env.end(); ++it)
        {
            fprintf(stderr, "%s=%s\n", it->first.c_str(), it->second.c_str());
        }
    }
    if(verbose > 1)
    {
        fprintf(stderr, "HTTP request on \"%s\"\n", env["HTTP_HOST"].c_str());
    }

    request(argv[0]);
}

// vim: ts=4 sw=4 et
