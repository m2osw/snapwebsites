// Snap Websites Server -- snap websites initialize website implementation
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

#include "snapwebsites/snap_initialize_website.h"

#include "snapwebsites/log.h"
#include "snapwebsites/tcp_client_server.h"
#include "snapwebsites/snapwebsites.h"

#include <sstream>

#include "snapwebsites/poison.h"


namespace snap
{


snap_initialize_website::snap_initialize_website_runner::snap_initialize_website_runner(snap_initialize_website * parent,
                                                QString const & snap_host, int snap_port, bool secure,
                                                QString const & website_uri, int destination_port,
                                                QString const & query_string, QString const & protocol)
    : snap_runner("initialize_website")
    , f_parent(parent)
    //, f_mutex() -- auto-init
    //, f_done() -- auto-init
    , f_snap_host(snap_host)
    , f_snap_port(snap_port)
    , f_secure(secure)
    , f_website_uri(website_uri)
    , f_destination_port(destination_port)
    , f_query_string(query_string)
    , f_protocol(protocol.toUpper())
{
    if(f_protocol != "HTTP"
    && f_protocol != "HTTPS")
    {
        throw snap_initialize_website_exception_invalid_parameter("protocol must be \"HTTP\" or \"HTTPS\".");
    }
}


void snap_initialize_website::snap_initialize_website_runner::run()
{
    try
    {
        send_init_command();
    }
    catch(...)
    {
        // we are in thread, throwing is not too good
        // TODO: add something to tell about the problem?
        message("Snap! Manager received an unknown exception while initializing a website.");
    }
    done();
}


void snap_initialize_website::snap_initialize_website_runner::send_init_command()
{
    tcp_client_server::bio_client::pointer_t socket;
    try
    {
         tcp_client_server::bio_client::mode_t const mode(f_secure
                    ? tcp_client_server::bio_client::mode_t::MODE_SECURE
                    : tcp_client_server::bio_client::mode_t::MODE_PLAIN);
         socket.reset(new tcp_client_server::bio_client(f_snap_host.toUtf8().data(), f_snap_port, mode));
    }
    catch(tcp_client_server::tcp_client_server_runtime_error const&)
    {
        message("Snap! Manager was not able to connect to the Snap! Server (connection error).\n\nPlease verify that a Snap! server is running at the specified IP address.");
        return;
    }

    // send the #INIT command
#define INIT_COMMAND "#INIT=" SNAPWEBSITES_VERSION_STRING
    if(socket->write(INIT_COMMAND "\n", sizeof(INIT_COMMAND)) != sizeof(INIT_COMMAND))
    {
        message("Snap! Manager was not able to communicate with the Snap! Server (write \"" INIT_COMMAND "\" error).");
        return;
    }
#undef INIT_COMMAND

    // now send the environment
    std::stringstream ss;

    // HTTP_HOST
    ss << "HTTP_HOST=" << f_website_uri << std::endl;

    // HTTP_USER_AGENT
    ss << "HTTP_USER_AGENT=Snap/" << SNAPWEBSITES_VERSION_MAJOR
       << "." << SNAPWEBSITES_VERSION_MINOR
       << " (Linux) libsnapwebsites/" << SNAPWEBSITES_VERSION_MAJOR
       << "." << SNAPWEBSITES_VERSION_MINOR
       << "." << SNAPWEBSITES_VERSION_PATCH << std::endl;

    // HTTP_ACCEPT
    ss << "HTTP_ACCEPT=text/plain" << std::endl;

    // HTTP_ACCEPT_LANGUAGE
    ss << "HTTP_ACCEPT_LANGUAGE=en-us,en;q=0.8" << std::endl;

    // HTTP_ACCEPT_ENCODING
    //
    // TODO: add support for gzip compression
    //ss << "HTTP_ACCEPT_ENCODING=..." << std::endl;

    // HTTP_ACCEPT_CHARSET
    ss << "HTTP_ACCEPT_CHARSET=utf-8" << std::endl;

    // HTTP_CONNECTION
    ss << "HTTP_CONNECTION=close" << std::endl; // close once done

    // HTTP_CACHE_CONTROL
    ss << "HTTP_CACHE_CONTROL=max-age=0" << std::endl;

    // SERVER_SOFTWARE
    ss << "SERVER_SOFTWARE=Snap" << std::endl;

    // SERVER_NAME
    // TBD... requires a reverse DNS if f_snap_host is an IP
    //ss << "SERVER_NAME=" << f_snap_host << std::endl;

    // SERVER_ADDR
    // TODO: if f_snap_host is a name, convert to IP (use socket name?)
    ss << "SERVER_ADDR=" << f_snap_host << std::endl;

    // SERVER_PORT
    ss << "SERVER_PORT=" << f_destination_port << std::endl;

    // REMOTE_HOST
    char hostname[HOST_NAME_MAX + 1];
    hostname[HOST_NAME_MAX] = '\0';
    if(gethostname(hostname, HOST_NAME_MAX) == -1)
    {
        message("Snap! Manager could not determine your host name.");
        strncpy(hostname, "UNKNOWN", HOST_NAME_MAX);
    }
    ss << "REMOTE_HOST=" << hostname << std::endl;

    // REMOTE_ADDR
    ss << "REMOTE_ADDR=" << socket->get_client_addr() << std::endl;

    // REMOTE_PORT
    ss << "REMOTE_PORT=" << socket->get_client_port() << std::endl;

    // GATEWAY_INTERFACE
    ss << "GATEWAY_INTERFACE=libsnapwebsites/" << SNAPWEBSITES_VERSION_MAJOR
       << "." << SNAPWEBSITES_VERSION_MINOR
       << "." << SNAPWEBSITES_VERSION_PATCH << std::endl;

    // SERVER_PROTOCOL
    ss << "SERVER_PROTOCOL=HTTP/1.1" << std::endl;

    // REQUEST_METHOD
    ss << get_name(name_t::SNAP_NAME_CORE_REQUEST_METHOD) << "=GET" << std::endl;

    // QUERY_STRING
    ss << "QUERY_STRING=initialize_website=1";
    if(!f_query_string.isEmpty())
    {
        ss << "&" << f_query_string;
    }
    ss << std::endl;

    // REQUEST_URI
    ss << snap::get_name(name_t::SNAP_NAME_CORE_REQUEST_URI) << "=/" << std::endl;

    // SCRIPT_NAME
    //ss << "SCRIPT_NAME=..." << std::endl;

    // HTTPS
    //
    if(f_protocol == "HTTPS")
    {
        ss << "HTTPS=on" << std::endl;
    }

    // send the environment in one block
    std::string const env(ss.str());
std::cerr << "---ENV---\n" << env << "---ENV---\n";
    if(socket->write(env.c_str(), env.size()) != static_cast<int>(env.size()))
    {
        message("Snap! Manager was not able to communicate with the Snap! Server (write error while sending environment).");
        return;
    }

    // send the #END
    if(socket->write("#END\n", 5) != 5)
    {
        message("Snap! Manager was not able to communicate with the Snap! Server (write \"#END\" error).");
        return;
    }

    // read the results of the #START command
    bool started(false);
    for(;;)
    {
        std::string buf;
        int const r(socket->read_line(buf));
        if(r <= 0)
        {
            // note that r == 0 is not an error but it should not happen
            // (i.e. I/O is blocking so we should not return too soon.)
            if(started)
            {
                message("Snap! Manager never received the #END signal.");
            }
            else
            {
                message("Snap! Manager was not able to communicate with the Snap! Server (read error).");
            }
            return;
        }
        if(!started)
        {
            if(buf != "#START")
            {
                message("Snap! Manager was able to communicate with the Snap! Server but got unexpected protocol data.");
                return;
            }
            started = true;
        }
        else if(buf == "#END")
        {
            // got the #END mark, we are done
            break;
        }
        else
        {
            QString const line(QString::fromUtf8(buf.c_str(), buf.length()));
            message("Status: " + line);
        }
    }
}


void snap_initialize_website::snap_initialize_website_runner::message(QString const& msg)
{
    snap_thread::snap_lock lock(f_mutex);
    f_message_queue.push_back(msg);
}


QString snap_initialize_website::snap_initialize_website_runner::next_message()
{
    // TODO: It looks like we created our own queue here when the thread
    // implementation offers one that most certainly works just fine -- switch!
    snap_thread::snap_lock lock(f_mutex);
    if(f_message_queue.empty())
    {
        return QString();
    }
    QString const msg(f_message_queue.front());
    f_message_queue.pop_front();
    return msg;
}


bool snap_initialize_website::snap_initialize_website_runner::is_done() const
{
    snap_thread::snap_lock lock(f_mutex);
    return f_done;
}


void snap_initialize_website::snap_initialize_website_runner::done()
{
    // mark the process as done
    snap_thread::snap_lock lock(f_mutex);
    f_done = true;
}




snap_initialize_website::snap_initialize_website(QString const & snap_host, int snap_port, bool secure,
                                                 QString const & website_uri, int destination_port,
                                                 QString const & query_string, QString const & protocol)
    : f_website_runner(new snap_initialize_website_runner(this, snap_host, snap_port, secure, website_uri, destination_port, query_string, protocol))
    , f_process_thread(new snap_thread("Initialize Website Thread", f_website_runner.get()))
{
}


bool snap_initialize_website::start_process()
{
    if(!f_process_thread->start())
    {
        SNAP_LOG_FATAL("cannot start thread for website initialization!");
        return false;
    }

    return true;
}


QString snap_initialize_website::get_status()
{
    return f_website_runner->next_message();
}


bool snap_initialize_website::is_done() const
{
    return f_website_runner->is_done();
}



}
// namespace snap

// vim: ts=4 sw=4 et
