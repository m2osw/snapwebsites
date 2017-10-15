/*
 * Text:
 *      status.cpp
 *
 * Description:
 *      The implementation of the STATUS function.
 *
 * License:
 *      Copyright (c) 2016-2017 Made to Order Software Corp.
 *
 *      http://snapwebsites.org/
 *      contact@m2osw.com
 *
 *      Permission is hereby granted, free of charge, to any person obtaining a
 *      copy of this software and associated documentation files (the
 *      "Software"), to deal in the Software without restriction, including
 *      without limitation the rights to use, copy, modify, merge, publish,
 *      distribute, sublicense, and/or sell copies of the Software, and to
 *      permit persons to whom the Software is furnished to do so, subject to
 *      the following conditions:
 *
 *      The above copyright notice and this permission notice shall be included
 *      in all copies or substantial portions of the Software.
 *
 *      THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 *      OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *      MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *      IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 *      CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 *      TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 *      SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


// ourselves
//
#include "snapmanagerdaemon.h"

// our lib
//
#include "snapmanager/plugin_base.h"

// snapwebsites lib
//
#include <snapwebsites/chownnm.h>
#include <snapwebsites/log.h>

// C lib
//
#include <sys/file.h>
#include <sys/stat.h>

// last entry
//
#include <snapwebsites/poison.h>

namespace snap_manager
{





/** \brief Function called whenever the MANAGERSTATUS message is received.
 *
 * Whenever the status of a snapmanagerdaemon changes, it is sent to all
 * the other snapmanagerdaemon (and this daemon itself.)
 *
 * \param[in] message  The actual MANAGERSTATUS message.
 */
void manager_daemon::set_manager_status(snap::snap_communicator_message const & message)
{
    QString const server(message.get_sent_from_server());
    QString const status(message.get_parameter("status"));

    server_status s(f_cluster_status_path, server);

    // get this snapcommunicator status in our server_status object
    //
    if(!s.from_string(status))
    {
        return;
    }

    // count errors and warnings and save that to the header
    // note: we do not count the potential errors that we will be adding
    //       to the header (because those would certainly be counted twice)
    //
    {
        size_t const count(s.count_errors());
        if(count > 0)
        {
            snap_manager::status_t header_errors(snap_manager::status_t::state_t::STATUS_STATE_INFO,
                                                 "header",
                                                 "errors",
                                                 QString("%1").arg(count));
            s.set_field(header_errors);
        }
    }

    {
        size_t const count(s.count_warnings());
        if(count > 0)
        {
            snap_manager::status_t header_warnings(snap_manager::status_t::state_t::STATUS_STATE_INFO,
                                                   "header",
                                                   "warnings",
                                                   QString("%1").arg(count));
            s.set_field(header_warnings);
        }
    }

    // convert a few parameter to header parameters so they can be loaded
    // first without having to load the entire file (which can become
    // really big with time and additional packages to manage)
    //
    if(s.get_field_state("self", "status") != snap_manager::status_t::state_t::STATUS_STATE_UNDEFINED)
    {
        snap_manager::status_t header_status(s.get_field_status("self", "status"));
        header_status.set_plugin_name("header");
        s.set_field(header_status);
    }
    else
    {
        snap_manager::status_t const header_status(snap_manager::status_t::state_t::STATUS_STATE_ERROR, "header", "status", "unknown");
        s.set_field(header_status);
    }

    if(s.get_field_state("self", "ip") != snap_manager::status_t::state_t::STATUS_STATE_UNDEFINED)
    {
        snap_manager::status_t header_ip(s.get_field_status("self", "ip"));
        header_ip.set_plugin_name("header");
        s.set_field(header_ip);
    }
    else
    {
        // use a "valid" IP address, but not a correct IP address...
        // (because consumers won't expect an empty string here)
        //
        snap_manager::status_t const header_ip(snap_manager::status_t::state_t::STATUS_STATE_ERROR, "header", "ip", "127.0.0.1");
        s.set_field(header_ip);
    }

    if(!s.write())
    {
        return;
    }

    // in this case we may have created the file so we need to make sure
    // that the owner and group are correct
    //
    // Note: The mode should be correct from within the write(), although
    //       it will depend on the umask as well. So to make sure, we
    //       change it here to exactly what we expect.
    //
    QString const & filename(s.get_filename());
    chmod(filename.toUtf8().data(), S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH); // make sure we have -rw-rw-r--

    // TODO: move those hard coded user names to snapmanager.conf
    QString const owner("snapwebsites");
    QString const group("www-data");
    if(snap::chownnm(filename, owner, group) != 0)
    {
        // let admin know that this is not working
        SNAP_LOG_WARNING("could not change owner and group of \"")(filename)("\" to \"")(owner)(":")(group)("\".");
    }

    // keep a copy of our own information
    //
    //if(server == f_server_name)
    //{
    //    f_status = s;
    //}
}


void manager_daemon::send_ack( snap::snap_communicator_message const & message, bool const done )
{
    snap::snap_communicator_message acknowledge;
    acknowledge.reply_to(message);
    acknowledge.set_command("MANAGERACKNOWLEDGE");
    acknowledge.add_parameter("who", f_server_name);
    if( done )
    {
        acknowledge.add_parameter( "done", "true" );
    }
    else
    {
        acknowledge.add_parameter( "start", "true" );
    }
    f_messenger->send_message(acknowledge);
}


void manager_daemon::send_nak(snap::snap_communicator_message const & message)
{
    snap::snap_communicator_message acknowledge;
    acknowledge.reply_to(message);
    acknowledge.set_command("MANAGERACKNOWLEDGE");
    acknowledge.add_parameter("who", f_server_name);
    acknowledge.add_parameter("failed", "true");
    f_messenger->send_message(acknowledge);
}


void manager_daemon::modify_settings(snap::snap_communicator_message const & message)
{
    // sender wants at least one snapmanagerdaemon to acknowledge so we
    // have to send this reply
    //
    send_ack( message );

    // TODO: unfortunately, although it looks like we're sending that message
    //       right now, it's stuck until we're done because this process is
    //       currently blocking; we need to change that and maybe even use
    //       a separate process like the snapupgrader so that way we make
    //       sure installations that would want to restart snapmanagerdaemon
    //       do not kill us while we're still trying to install things...
    //       (for installations see the self.cpp plugin implementation, we
    //       probably want to do that there and not here.)
    //
    //       the following could be worked on by a thread as we mentioned
    //       in SNAP-395 -- snapmanagerdaemon already uses threads to run
    //       various other tasks (i.e the status thread and the bundle
    //       gathering thread...) so we could have one worker thread to
    //       which we send work to be done such as the modified settings.

    // now call the plugin change settings function
    //
    QString const button_name(message.get_parameter("button_name"));
    QString const field_name(message.get_parameter("field_name"));
    QString const new_value(message.get_parameter("new_value"));
    QString const plugin_name(message.get_parameter("plugin_name"));

    // if we have installation parameters, then there is no old value
    QString old_or_installation_value;
    //
    // SNAP-412: Check for button name to know what we are expected to use.
    //
    if(button_name == "install")
    {
        if(message.has_parameter("install_values"))
        {
            old_or_installation_value = message.get_parameter("install_values");
        }
    }
    else
    {
        if(message.has_parameter("old_value"))
        {
            old_or_installation_value = message.get_parameter("old_value");
        }
    }

    snap::plugins::plugin * p(snap::plugins::get_plugin(plugin_name));
    if(p == nullptr)
    {
        SNAP_LOG_WARNING("received message requiring to access plugin \"")(plugin_name)("\" which is not installed on this system. This is a normal warning when using the \"Save Everywhere\" button.");
        send_nak(message);
        return;
    }
    plugin_base * pb(dynamic_cast<plugin_base *>(p));
    if(pb == nullptr)
    {
        // this shoud never happens!
        SNAP_LOG_ERROR("plugin \"")(plugin_name)("\" is not a snapmanager base plugin.");
        send_nak(message);
        return;
    }
    std::set<QString> affected_services;
    if(pb->apply_setting(button_name, field_name, new_value, old_or_installation_value, affected_services))
    {
        handle_affected_services(affected_services);

        // TODO: when apply_setting() worked, we want to "PING" the status
        //       thread to re-read the info and save that in the
        //       <host>.db file ASAP
        //
        //...

        // force a resend because otherwise it may not notice the difference
        // and want to send the same status again, but we have to have the
        // [modified] removed ASAP...
        //
        f_status_runner.resend_status(true);

        // send a RELOADCONFIG to all the affected services
        // however, if one of the services is snapcommunicator, set a flag
        // and send a message to snapcommunicator last (otherwise we would
        // very likely lose many of the RELOADCONFIG messages)
        //
        bool snapcommunicator_was_affected(false);
        std::for_each(
                affected_services.begin(),
                affected_services.end(),
                [=, &snapcommunicator_was_affected](QString const & service_name)
                {
                    if(service_name == "snapcommunicator")
                    {
                        snapcommunicator_was_affected = true;
                    }
                    else
                    {
                        snap::snap_communicator_message reload_config;
                        reload_config.set_service(service_name);
                        reload_config.set_command("RELOADCONFIG");
                        f_messenger->send_message(reload_config);
                    }
                }
            );

        // Now we can send the message to the snapcommunicator service
        // itself (all messages are sent through that service)
        //
        if(snapcommunicator_was_affected)
        {
            snap::snap_communicator_message reload_config;
            //reload_config.set_service("snapcommunicator"); -- not necessary
            reload_config.set_command("RELOADCONFIG");
            f_messenger->send_message(reload_config);
        }
    }

    send_ack( message, true /*done*/ );
}



} // namespace snap_manager
// vim: ts=4 sw=4 et
