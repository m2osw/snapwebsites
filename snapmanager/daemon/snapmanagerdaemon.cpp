//
// File:        snapmanagerdaemon.cpp
// Object:      Allow for applying functions on any computer.
//
// Copyright:   Copyright (c) 2016-2017 Made to Order Software Corp.
//              All Rights Reserved.
//
// http://snapwebsites.org/
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

#include "snapmanagerdaemon.h"

#include <snapwebsites/addr.h>
#include <snapwebsites/log.h>
#include <snapwebsites/mkdir_p.h>
#include <snapwebsites/not_used.h>
#include <snapwebsites/tokenize_string.h>

#include <sstream>


namespace snap_manager
{


/** \brief Initialize the manager daemon.
 *
 * Initialize the various variable members that need a dynamic
 * initialization.
 */
manager_daemon::manager_daemon()
    : manager(true)
    , f_status_connection(new status_connection(this))
    , f_status_runner(this, f_status_connection)
    , f_status_thread("status", &f_status_runner)
    , f_bundle_loader()
    , f_bundle_thread("bundle_loader", &f_bundle_loader)
{
}


void manager_daemon::init(int argc, char * argv[])
{
    manager::init(argc, argv);

    f_status_connection->set_server_name(f_server_name);

    // local_listen=... from snapcommunicator.conf
    //
    tcp_client_server::get_addr_port(QString::fromUtf8(f_config("snapcommunicator", "local_listen").c_str()), f_communicator_address, f_communicator_port, "tcp");

    // TODO: make us snapwebsites by default and root only when required...
    //       (and use RAII to do the various switches)
    //
    if(setuid(0) != 0)
    {
        perror("manager_daemon:setuid(0)");
        throw std::runtime_error("fatal error: could not setup executable to run as user root.");
    }
    if(setgid(0) != 0)
    {
        perror("manager_daemon:setgid(0)");
        throw std::runtime_error("fatal error: could not setup executable to run as group root.");
    }

    // get the list of front end servers (i.e. list of computer names
    // accepting snapmanager.cgi requests)
    //
    if(f_config.has_parameter("snapmanager_frontend"))
    {
        f_status_runner.set_snapmanager_frontend(f_config["snapmanager_frontend"]);
    }

    // now try to load all the plugins
    //
    load_plugins();

    // handle the bundle loading now
    //
    if(f_config.has_parameter("bundle_uri"))
    {
        std::string const bundle_uri(f_config["bundle_uri"]);
        snap::tokenize_string(f_bundle_uri, bundle_uri, ",", true, " ");

        if(!f_bundle_uri.empty())
        {
            f_bundle_loader.set_bundle_uri(f_bundles_path, f_bundle_uri);
            if(!f_bundle_thread.start())
            {
                SNAP_LOG_ERROR("snapmanagerdaemon could not start the bundle loader thread.");
                // we do not prevent continuing...
            }
        }
    }
}


manager_daemon::~manager_daemon()
{
}


int manager_daemon::run()
{
    // Stop on these signals, log them, then terminate.
    //
    // Note: the handler uses the logger which the create_instance()
    //       initializes
    //
    signal( SIGSEGV, manager_daemon::sighandler );
    signal( SIGBUS,  manager_daemon::sighandler );
    signal( SIGFPE,  manager_daemon::sighandler );
    signal( SIGILL,  manager_daemon::sighandler );
    signal( SIGTERM, manager_daemon::sighandler );
    signal( SIGINT,  manager_daemon::sighandler );
    signal( SIGQUIT, manager_daemon::sighandler );

    // ignore console signals
    //
    signal( SIGTSTP,  SIG_IGN );
    signal( SIGTTIN,  SIG_IGN );
    signal( SIGTTOU,  SIG_IGN );

    SNAP_LOG_INFO("--------------------------------- snapmanagerdaemon v" SNAPMANAGERCGI_VERSION_STRING " started on ")(f_server_name);

    // initialize the communicator and its connections
    //
    f_communicator = snap::snap_communicator::instance();

    // capture Ctrl-C (SIGINT)
    //
    f_interrupt.reset(new manager_interrupt(this));
    f_communicator->add_connection(f_interrupt);

    // create a messenger to communicate with the Snap Communicator process
    // and snapmanager.cgi as required
    //
    f_messenger.reset(new manager_messenger(this, f_communicator_address.toUtf8().data(), f_communicator_port));
    f_communicator->add_connection(f_messenger);

    // also add the status connection created in the constructor
    //
    f_communicator->add_connection(f_status_connection);

    // Add the logging server through snapcommunicator:
    //
    snap::logging::set_log_messenger( f_messenger );

    // now run our listening loop
    //
    f_communicator->run();

    return f_force_restart ? 1 : 0;
}


/** \brief A static function to capture various signals.
 *
 * This function captures unwanted signals like SIGSEGV and SIGILL.
 *
 * The handler logs the information and then the service exists.
 * This is done mainly so we have a chance to debug problems even
 * when it crashes on a remote server.
 *
 * \warning
 * The signals are setup after the construction of the manager_daemon
 * object because that is where we initialize the logger.
 *
 * \param[in] sig  The signal that was just emitted by the OS.
 */
void manager_daemon::sighandler( int sig )
{
    QString signame;
    bool output_stack_trace(true);
    switch( sig )
    {
    case SIGSEGV:
        signame = "SIGSEGV";
        break;

    case SIGBUS:
        signame = "SIGBUS";
        break;

    case SIGFPE:
        signame = "SIGFPE";
        break;

    case SIGILL:
        signame = "SIGILL";
        break;

    case SIGTERM:
        signame = "SIGTERM";
        output_stack_trace = false;
        break;

    case SIGINT:
        signame = "SIGINT";
        output_stack_trace = false;
        break;

    case SIGQUIT:
        signame = "SIGQUIT";
        output_stack_trace = false;
        break;

    default:
        signame = "UNKNOWN";
        break;

    }

    if(output_stack_trace)
    {
        snap::snap_exception_base::output_stack_trace();
    }
    SNAP_LOG_FATAL("Fatal signal caught: ")(signame);

    // Exit with error status
    //
    ::exit( 1 );
    snap::NOTREACHED();
}


/** \brief Process a message received from Snap! Communicator.
 *
 * This function gets called whenever the Snap! Communicator sends
 * us a message. This includes the basic READY, HELP, and STOP commands.
 *
 * \param[in] message  The message we just received.
 */
void manager_daemon::process_message(snap::snap_communicator_message const & message)
{
    SNAP_LOG_TRACE("received messenger message [")(message.to_message())("] for ")(f_server_name);

    QString const command(message.get_command());

    switch(command[0].unicode())
    {
    case 'D':
        if(command == "DPKGUPDATE")
        {
            // at this time we ignore the "action" parameter and just
            // tell the backend to reset the dpkg status for all packages
            //
            // TBD: we may need/want to do this "a retardement" after a
            //      minute or two, so that way the system has some time
            //      to settle first?
            //
            reset_aptcheck();
            return;
        }
        break;

    case 'H':
        if(command == "HELP")
        {
            // Snap! Communicator is asking us about the commands that we support
            //
            snap::snap_communicator_message reply;
            reply.set_command("COMMANDS");

            snap::snap_string_list understood_commands;
            understood_commands << "DPKGUPDATE";
            understood_commands << "HELP";
            understood_commands << "LOG";
            understood_commands << "MANAGERINSTALL";
            understood_commands << "MANAGERRESEND";
            understood_commands << "MANAGERSTATUS";
            understood_commands << "MODIFYSETTINGS";
            understood_commands << "NEWREMOTECONNECTION";
            understood_commands << "QUITTING";
            understood_commands << "READY";
            understood_commands << "RELOADCONFIG";
            understood_commands << "SERVER_PUBLIC_IP";
            understood_commands << "STOP";
            understood_commands << "UNKNOWN";
            understood_commands << "UNREACHABLE";

            add_plugin_commands(understood_commands);

            // list of commands understood by service
            // (many are considered to be internal commands... users
            // should look at the LOCK and UNLOCK messages only)
            //
            reply.add_parameter("list", understood_commands.join(","));

            f_messenger->send_message(reply);

            // if we are a front end computer, we want to be kept informed
            // of the status of all the other computers in the cluster...
            // so ask all the other snapmanagerdaemon to broadcast their
            // status again
            //
            if(f_status_runner.is_snapmanager_frontend(f_server_name))
            {
                snap::snap_communicator_message resend;
                resend.set_service("*");
                resend.set_command("MANAGERRESEND");
                f_messenger->send_message(resend);
            }

            // we do this in the HELP instead of the READY to make sure
            // that the snap communicator receives replies only after
            // it receives our COMMANDS; otherwise it could break saying
            // that it does not know the command of a reply...
            //
            communication_ready();
            return;
        }
        break;

    case 'L':
        if(command == "LOG")
        {
            // logrotate just rotated the logs, we have to reconfigure
            //
            SNAP_LOG_INFO("Logging reconfiguration.");
            snap::logging::reconfigure();
            return;
        }
        break;

    case 'M':
        if(command == "MANAGERINSTALL")
        {
            // install a bundle
            //
            manager_install(message);
            return;
        }
        else if(command == "MANAGERRESEND")
        {
            bool const kick_now(message.has_parameter("kick")
                             && message.get_parameter("kick") == "now");
            f_status_runner.resend_status(kick_now);
            return;
        }
        else if(command == "MANAGERSTATUS")
        {
            // record the status of this and other managers
            //
            set_manager_status(message);
            return;
        }
        else if(command == "MODIFYSETTINGS")
        {
            // user just clicked "Save" from somewhere...
            //
            modify_settings(message);
            return;
        }
        break;

    case 'N':
        if(command == "NEWREMOTECONNECTION")
        {
            snap::snap_communicator_message resend;
            resend.set_server(message.get_parameter("server_name"));
            resend.set_service("snapmanagerdaemon");
            resend.set_command("MANAGERRESEND");
            f_messenger->send_message(resend);
            return;
        }
        break;

    case 'Q':
        if(command == "QUITTING")
        {
            // If we received the QUITTING command, then somehow we sent
            // a message to Snap! Communicator, which is already in the
            // process of quitting... we should get a STOP too, but we
            // can just quit ASAP too
            //
            stop(true);
            return;
        }
        break;

    case 'R':
        if(command == "READY")
        {
            // we now are connected to the snapcommunicator
            //

            // request a copy of our public IP address
            {
                snap::snap_communicator_message public_ip;
                public_ip.set_command("PUBLIC_IP");
                f_messenger->send_message(public_ip);
            }
            return;
        }
        else if(command == "RELOADCONFIG")
        {
            // at this time we do not know how to reload our configuration
            // file without just restarting 100% (especially think of the
            // problem of having connections to snapcommunicator and similar
            // systems... if the configuration changes their IP address,
            // what to do, really...)
            //
            // At this time this is a STOP with an error which in effects
            // asks systemd to restart us!
            //
            f_force_restart = true;
            stop(false);
            return;
        }
        break;

    case 'S':
        if(command == "SERVER_PUBLIC_IP")
        {
            // snapcommunicator replied
            //
            f_public_ip = message.get_parameter("public_ip");

            // start the status thread, used to gather this computer's status
            //
            if(!f_status_thread.start())
            {
                SNAP_LOG_ERROR("snapmanagerdaemon could not start its helper thread. Quitting immediately.");
                stop(false);
            }
            return;
        }
        else if(command == "STOP")
        {
            // Someone is asking us to leave (probably snapinit)
            //
            stop(false);
            return;
        }
        break;

    case 'U':
        if(command == "UNKNOWN")
        {
            // we sent a command that Snap! Communicator did not understand
            //
            SNAP_LOG_ERROR("we sent unknown command \"")(message.get_parameter("command"))("\" and probably did not get the expected result.");
            return;
        }
        else if(command == "UNREACHABLE")
        {
            unreachable_message(message);
            return;
        }
        break;

    }

    // if not a basic message, it could be a plugin defined message
    //
    bool processed(false);
    process_plugin_message(message, processed);
    if(processed)
    {
        return;
    }

    // unknown commands get reported and process goes on
    //
    SNAP_LOG_ERROR("unsupported command \"")(command)("\" was received on the connection with Snap! Communicator.");
    {
        snap::snap_communicator_message reply;
        reply.set_command("UNKNOWN");
        reply.add_parameter("command", command);
        f_messenger->send_message(reply);
    }

    return;
}




/** \brief Called whenever we receive the STOP command or equivalent.
 *
 * This function makes sure the manager_daemon exits as quickly as
 * possible.
 *
 * \li Marks the messenger as done.
 * \li UNREGISTER from snapcommunicator.
 *
 * \note
 * If the f_messenger is still in place, then just sending the
 * UNREGISTER is enough to quit normally. The socket of the
 * f_messenger will be closed by the snapcommunicator server
 * and we will get a HUP signal. However, we get the HUP only
 * because we first mark the messenger as done.
 *
 * \param[in] quitting  Set to true if we received a QUITTING message.
 */
void manager_daemon::stop(bool quitting)
{
    if(f_messenger != nullptr)
    {
        if(quitting || !f_messenger->is_connected())
        {
            // turn off that connection now, we cannot UNREGISTER since
            // we are not connected to snapcommunicator
            //
            f_communicator->remove_connection(f_messenger);
            f_messenger.reset();
        }
        else
        {
            f_messenger->mark_done();

            // unregister if we are still connected to the messenger
            // and Snap! Communicator is not already quitting
            //
            snap::snap_communicator_message cmd;
            cmd.set_command("UNREGISTER");
            cmd.add_parameter("service", "snapmanagerdaemon");
            f_messenger->send_message(cmd);
        }
    }

    if(f_status_connection != nullptr)
    {
        // WARNING: we cannot send a message to the status thread
        //          if it was not started
        //
        if(f_status_thread.is_running())
        {
            snap::snap_communicator_message cmd;
            cmd.set_command("STOP");
            f_status_connection->send_message(cmd);
        }

        // WARNING: currently, the send_message() of an inter-process
        //          connection immediately writes the message in the
        //          destination thread FIFO and immediately sends a
        //          signal; as a side effect we can immediatly forget
        //          about the status connection
        //
        f_communicator->remove_connection(f_status_connection);
        f_status_connection.reset();
    }

    f_communicator->remove_connection(f_interrupt);
}


/** \brief Manage this computer.
 *
 * This function processes a MANAGE command received by this daemon.
 *
 * This command is the one that allows us to fully manage a remote
 * computer from snapmanager.cgi.
 *
 * We decided that we would use ONE global message which supports
 * many functions rather than defining many messages and possibly
 * have problems later because of some clashes.
 *
 * \param[in] message  The message being worked on.
 */
void manager_daemon::manager_install(snap::snap_communicator_message const & message)
{
    snap::NOTUSED(message);
    //installer(message);
}




/** \brief Forword message to snapcommunicator.
 *
 * When we receive a message from our thread, and that message is not
 * directed to us (i.e. service name is the empty string or
 * snapmanagerdaemon) then the message needs to be sent to the
 * snapcommunicator.
 *
 * This function makes sure to send the message to the specified services
 * or even computers.
 *
 * At this time this is used to send the MANAGERSTATUS to all the
 * computers currently registered.
 *
 * The function can also be used by plugins that need to send messages
 * through the manager daemon connection to snapcommunicator.
 *
 * \param[in] message  The message to forward to snapcommunicator.
 */
void manager_daemon::forward_message(snap::snap_communicator_message const & message)
{
    // make sure the messenger is still alive
    //
    if(f_messenger)
    {
        f_messenger->send_message(message);
    }
}


/** \brief Get the list of frontend snapmanagerdaemons.
 *
 * This function returns a constant reference to the list of frontend
 * snapmanagerdaemon running on frontends (computers that an administrator
 * can access.)
 *
 * \return The list of snapmanager frontends server names.
 */
snap::snap_string_list const & manager_daemon::get_snapmanager_frontend() const
{
    return f_status_runner.get_snapmanager_frontend();
}


/** \brief Check whether the status runner thread is asking to stop ASAP.
 *
 * This function is expected to be called by plugins whenever their
 * retrieve_status() signal implementation function gets called. It
 * makes sure that the thread can quit quickly if asked to do so.
 *
 * This is important especially if some of your status gathering
 * functions are slow (i.e. run a command such as dpkg-query)
 *
 * \return true if the thread was asked to quit as soon as possible.
 */
bool manager_daemon::stop_now_prima() const
{
    return f_status_runner.stop_now_prima();
}


/** \brief A computer was unreachable, make sure to take note.
 *
 * The snapcommunicator will attempt to connect to remote computers
 * that are expected to run snapcommunicator, either with a direct
 * connection or to send it a GOSSIP message.
 *
 * If these connections fail, the snapcommunicator system sends an
 * UNREACHABLE message to all the local services currently
 * registered.
 *
 * Here we acknowledge the fact and make sure the mark that computer
 * as Down (it could still be marked as Up from previous runs.)
 *
 * \param[in] message  The message being worked on.
 */
void manager_daemon::unreachable_message(snap::snap_communicator_message const & message)
{
    // the parameter "who" must exist and define the IP address of the
    // computer that could not connect
    //
    snap_addr::addr who_addr(snap_addr::addr((message.get_parameter("who") + ":123").toUtf8().data(), "tcp"));

    // retrieve the list of servers (<data-path>/cluster-status/*.db file names)
    //
    std::vector<std::string> const servers(get_list_of_servers());

    for(auto const & s : servers)
    {
        server_status status_info(s.c_str());

        if(!status_info.read_header())
        {
            // the read_header() and sub-functions already emit errors
            // so we do not add any here
            //
            continue;
        }

        // TODO: the 'ip' and 'addr' parameters need to be canonicalized
        //       with snap::addr 
        //
        QString const ip(status_info.get_field("header", "ip"));
        if(ip.isEmpty())
        {
            continue;
        }
        snap_addr::addr server_addr(snap_addr::addr((ip + ":123").toUtf8().data(), "tcp"));

        // is that the one that is down?
        //
        if(who_addr != server_addr)
        {
            continue;
        }

        // server already marked as down?
        //
        snap_manager::status_t status(status_info.get_field_status("header", "status"));
        if(status.get_value() == "down")
        {
            continue;
        }

        // it is not yet marked Down, read the other fields before
        // updating the file.
        //
        if(!status_info.read_all())
        {
            // the read_all() function (and sub-functions) will generate
            // errors if such occur...
            //
            continue;
        }

        // okay! update the status now
        //
        status.set_value("down");
        status_info.set_field(status);

        // XXX: do we have to update the self::status field too?

        // write the result back to the file
        //
        snap::NOTUSED(status_info.write());
    }
}





}
// namespace snap_manager
// vim: ts=4 sw=4 et
