/*
 * Text:
 *      snaplock/tests/test_multi_snaplocks.cpp
 *
 * Description:
 *      Test "any" number of snaplock/snapcommunicator combo on a single
 *      computer.
 *
 *      This test creates threads, each of which simulates the
 *      snapcommunicator, at least as much as snaplock requires.
 *
 *      The simulator still uses local network (127.0.0.1) connections
 *      using ports `9000` to `9000 + n - 1` with `n` being the number of
 *      simulator (WARNING: if you are running a DNS, 5353 is likely
 *      used so you should not create more than 352 instances.
 *
 * Documentation:
 *      What does the test do?
 *
 *      It creates `n` (command line parameter) instances of the
 *      snapcommunicator simulator. The simulator is a self contained
 *      class so it can safely be used with threads.
 *
 *      For each snapcommunicator instance, it sets up a configuration
 *      file and starts `snaplock -c <filename>`. That configuration will
 *      specify a service name and a server name on top of the usual
 *      parameters.
 *
 *      The test checks that the correct leaders get elected depending
 *      on the setup. If you set `n` to a pretty large value, the CLUSTERUP
 *      signal will not happen right away...
 *
 * License:
 *      Copyright (c) 2013-2019  Made to Order Software Corp.  All Rights Reserved
 * 
 *      https://snapwebsites.org/
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


// self
//
#include "test_multi_snaplocks.h"

#include "version.h"

// snapwebsites lib
//
#include <snapwebsites/mkdir_p.h>
#include <snapwebsites/qstring_stream.h>

// advgetopt lib
//
#include <advgetopt/exception.h>

// Qt lib
//
#include <QFile>

// C++ lib
//
#include <iostream>

// boost lib
//
#include <boost/preprocessor/stringize.hpp>

// C lib
//
#include <sys/wait.h>


namespace snap_test
{



/***************************************************************************
 *** COMMAND LINE OPTIONS **************************************************
 ***************************************************************************/

QString g_log_conf = QString("/etc/snapwebsites/logger/test_multi_snaplocks.properties");


advgetopt::option const g_options[] =
{
    {
        'c',
        advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::GETOPT_FLAG_REQUIRED | advgetopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "config",
        nullptr,
        "Path to configuration files.",
        nullptr
    },
    {
        'n',
        advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::GETOPT_FLAG_REQUIRED | advgetopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "count",
        "20",
        "Number of instances to play with, must be between 1 and 1000, default is 20.",
        nullptr
    },
    {
        '\0',
        advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::GETOPT_FLAG_REQUIRED | advgetopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "port",
        "9000",
        "define the starting port (default: 9000)",
        nullptr
    },
    {
        '\0',
        advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::GETOPT_FLAG_REQUIRED,
        "seed",
        nullptr,
        "define the seed to use for this run, otherwise a \"random\" one is assigned for you",
        nullptr
    },
    {
        '\0',
        advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_REQUIRED | advgetopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "snaplock",
        "snaplock",
        "path to the snaplock you want to run (should probably be a full path)",
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
advgetopt::options_environment const g_options_environment =
{
    .f_project_name = "snapwebsites",
    .f_options = g_options,
    .f_options_files_directory = nullptr,
    .f_environment_variable_name = "SNAPLOCK_TEST_OPTIONS",
    .f_configuration_files = nullptr,
    .f_configuration_filename = nullptr,
    .f_configuration_directories = nullptr,
    .f_environment_flags = advgetopt::GETOPT_ENVIRONMENT_FLAG_PROCESS_SYSTEM_PARAMETERS,
    .f_help_header = "Usage: %p [-<opt>]\n"
                     "where -<opt> is one or more of:",
    .f_help_footer = "%c",
    .f_version = SNAPLOCK_VERSION_STRING,
    .f_license = "GNU GPL v2",
    .f_copyright = "Copyright (c) 2013-"
                   BOOST_PP_STRINGIZE(UTC_BUILD_YEAR)
                   " by Made to Order Software Corporation -- All Rights Reserved",
    //.f_build_date = UTC_BUILD_DATE,
    //.f_build_time = UTC_BUILD_TIME
};
#pragma GCC diagnostic pop









/***************************************************************************
 *** SNAPCOMMUNICATOR MESSENGER ********************************************
 ***************************************************************************/


snapcommunicator_messenger::snapcommunicator_messenger(snapcommunicator_listener_pointer_t listener, tcp_client_server::bio_client::pointer_t client)
    : snap_tcp_server_client_message_connection(client)
    , f_listener(listener)
{
}


snapcommunicator_messenger::~snapcommunicator_messenger()
{
}


//void snapcommunicator_messenger::process_connection_failed(std::string const & error_message)
//{
//    snap_tcp_client_permanent_message_connection::process_connection_failed(error_message);
//}
//
//
//void snapcommunicator_messenger::process_connected()
//{
//    snap_tcp_client_permanent_message_connection::process_connected();
//}


void snapcommunicator_messenger::process_hup()
{
    snap_tcp_server_client_message_connection::process_hup();

    f_listener->messenger_hup();
}








/***************************************************************************
 *** SNAPCOMMUNICATOR LISTENER *********************************************
 ***************************************************************************/


/** \brief The listener initialization.
 *
 * The listener creates a new TCP server to listen for incoming
 * TCP connection.
 *
 * \warning
 * At this time the \p max_connections parameter is ignored.
 *
 * \param[in] addr  The address to listen on. Most often it is 0.0.0.0.
 * \param[in] port  The port to listen on.
 * \param[in] certificate  The filename of a PEM file with a certificate.
 * \param[in] private_key  The filename of a PEM file with a private key.
 * \param[in] max_connections  The maximum number of connections to keep
 *            waiting; if more arrive, refuse them until we are done with
 *            some existing connections.
 * \param[in] local  Whether this connection expects local services only.
 * \param[in] server_name  The name of the server running this instance.
 */
snapcommunicator_listener::snapcommunicator_listener(test_multi_snaplocks_pointer_t test, snapcommunicator_emulator_pointer_t ce, int port)
    : snap_tcp_server_connection(
                      "127.0.0.1"
                    , port
                    , std::string()
                    , std::string()
                    , tcp_client_server::bio_server::mode_t::MODE_PLAIN
                    , 20
                    , true)
    , f_test(test)
    , f_communicator_emulator(ce)
    , f_port(port)
{
}


snapcommunicator_listener::~snapcommunicator_listener()
{
    stop();
}


void snapcommunicator_listener::stop()
{
    snap::snap_communicator::instance()->remove_connection(f_messenger);
    f_messenger.reset();
}


snapcommunicator_listener::pointer_t snapcommunicator_listener::shared_from_this()
{
    return std::dynamic_pointer_cast<snapcommunicator_listener>(snap_tcp_server_connection::shared_from_this());
}


bool snapcommunicator_listener::send_message(snap::snap_communicator_message const & message, bool cache)
{
    if(f_messenger == nullptr)
    {
        throw test_exception_exit("the messenger of this snapcommunicator_listener (127.0.0.1:"
                  + std::to_string(f_port)
                  + ") is not yet in place.");
    }

    return f_messenger->send_message(message, cache);
}


bool snapcommunicator_listener::is_connected() const
{
    return f_messenger != nullptr;
}


void snapcommunicator_listener::messenger_hup()
{
    snap::snap_communicator::instance()->remove_connection(f_messenger);
    f_messenger.reset();
}


// snap::snap_communicator::snap_server_connection implementation
void snapcommunicator_listener::process_accept()
{
    if(is_connected())
    {
        throw test_exception_exit("received an accept() request on an already connected snapcommunicator_listener...");
    }

    // a new client just connected, create a new snapcommunicator_messenger
    // object and add it to the snap_communicator object.
    //
    tcp_client_server::bio_client::pointer_t const new_client(accept());
    if(new_client == nullptr)
    {
        // an error occurred (rare from accept())
        //
        int const e(errno);
        throw test_exception_exit("error: somehow accept() failed with errno: "
                  + std::to_string(e)
                  + " -- "
                  + strerror(e));
    }

    f_messenger.reset(new snapcommunicator_messenger(shared_from_this(), new_client));
    f_messenger->set_name("client connection");
    f_messenger->set_dispatcher(std::dynamic_pointer_cast<snap::dispatcher<snapcommunicator_emulator>>(f_communicator_emulator));

    if(!snap::snap_communicator::instance()->add_connection(f_messenger))
    {
        // this should never happen here since each new creates a
        // new pointer
        //
        throw test_exception_exit("error: new client connection could not be added to the snap_communicator list of connections");
    }

    f_test->received_new_connection();
}

















/***************************************************************************
 *** CTRL-C ****************************************************************
 ***************************************************************************/


/** \brief Initialize the Ctrl-C signal.
 *
 * This function initializes the snap_signal to listen on the SIGINT
 * Unix signal. It also saves the pointer \p s to the server so
 * it can be used to call the stop() function.
 *
 * \param[in] s  The server pointer.
 */
signal_ctrl_c::signal_ctrl_c(test_multi_snaplocks * s)
    : snap_signal(SIGINT)
    , f_server(s)
{
    unblock_signal_on_destruction();
    set_name("test_multi_snaplock Ctrl-C interrupt");
}


/** \brief Callback called each time the SIGINT signal occurs.
 *
 * This function gets called when SIGINT is received.
 *
 * The function calls the snaplock::stop() function.
 */
void signal_ctrl_c::process_signal()
{
    // check our children and remove zombies
    //
    f_server->stop();
}













/***************************************************************************
 *** CTRL-C ****************************************************************
 ***************************************************************************/


/** \brief Initialize the Ctrl-Break signal.
 *
 * This function initializes the snap_signal to listen on the SIGINT
 * Unix signal. It also saves the pointer \p s to the server so
 * it can be used to call the stop() function.
 *
 * \param[in] s  The server pointer.
 */
signal_terminate::signal_terminate(test_multi_snaplocks * s)
    : snap_signal(SIGTERM)
    , f_server(s)
{
    unblock_signal_on_destruction();
    set_name("test_multi_snaplock terminate");
}


/** \brief Callback called each time the SIGTERM signal occurs.
 *
 * This function gets called when SIGTERM is received.
 *
 * The function calls the snaplock::stop() function.
 */
void signal_terminate::process_signal()
{
    // check our children and remove zombies
    //
    f_server->stop();
}













/***************************************************************************
 *** SIGNAL CHILD DEATH ****************************************************
 ***************************************************************************/


/** \brief Initialize the child death signal.
 *
 * This function initializes the snap_signal to listen on the SIGCHLD
 * Unix signal. It also saves the pointer \p s to the server so
 * it can be used to call various functions in the server whenever
 * the signal occurs.
 *
 * \param[in] s  The server pointer.
 */
signal_child_death::signal_child_death(test_multi_snaplocks * s)
    : snap_signal(SIGCHLD)
    , f_server(s)
{
    unblock_signal_on_destruction();
    set_name("test_multi_snaplock zombie catcher");
}


/** \brief Callback called each time the SIGCHLD signal occurs.
 *
 * This function gets called each time a child dies.
 *
 * The function checks all the children and removes zombies.
 */
void signal_child_death::process_signal()
{
    // check our children and remove zombies
    //
    f_server->capture_zombie(get_child_pid());
}













/***************************************************************************
 *** SNAPCOMMUNICATOR EMULATOR *********************************************
 ***************************************************************************/


snap::dispatcher<snapcommunicator_emulator>::dispatcher_match::vector_t const
snapcommunicator_emulator::g_snapcommunicator_emulator_service_messages =
{
    {
        nullptr
      , &snapcommunicator_emulator::msg_callback
      , &snap::dispatcher<snapcommunicator_emulator>::dispatcher_match::callback_match
    },
    {
        "ACTIVATELOCK"
      , &snapcommunicator_emulator::msg_forward
    },
    {
        "ADDTICKET"
      , &snapcommunicator_emulator::msg_forward
    },
    {
        "CLUSTERSTATUS"
      , &snapcommunicator_emulator::msg_cluster_status
    },
    {
        "COMMANDS"
      , &snapcommunicator_emulator::msg_commands
    },
    {
        "DROPTICKET"
      , &snapcommunicator_emulator::msg_forward
    },
    {
        "GETMAXTICKET"
      , &snapcommunicator_emulator::msg_forward
    },
    {
        "LISTTICKETS"
      , &snapcommunicator_emulator::msg_forward
    },
    {
        "LOCK"
      , &snapcommunicator_emulator::msg_forward
    },
    {
        "LOCKACTIVATED"
      , &snapcommunicator_emulator::msg_forward
    },
    {
        "LOCKED"
      , &snapcommunicator_emulator::msg_locked
    },
    {
        "LOCKENTERED"
      , &snapcommunicator_emulator::msg_forward
    },
    {
        "LOCKENTERING"
      , &snapcommunicator_emulator::msg_forward
    },
    {
        "LOCKEXITING"
      , &snapcommunicator_emulator::msg_forward
    },
    {
        "LOCKFAILED"
      , &snapcommunicator_emulator::msg_lockfailed
    },
    {
        "LOCKLEADERS"
      , &snapcommunicator_emulator::msg_forward
    },
    {
        "LOCKREADY"
      , &snapcommunicator_emulator::msg_lockready
    },
    {
        "LOCKSTARTED"
      , &snapcommunicator_emulator::msg_forward
    },
    {
        "LOCKTICKETS"
      , &snapcommunicator_emulator::msg_forward
    },
    {
        "MAXTICKET"
      , &snapcommunicator_emulator::msg_forward
    },
    {
        "NOLOCK"
      , &snapcommunicator_emulator::msg_nolock
    },
    {
        "REGISTER"
      , &snapcommunicator_emulator::msg_register
    },
    {
        "TICKETADDED"
      , &snapcommunicator_emulator::msg_forward
    },
    {
        "TICKETREADY"
      , &snapcommunicator_emulator::msg_forward
    },
    {
        "TICKETLIST"
      , &snapcommunicator_emulator::msg_ticketlist
    },
    {
        "UNLOCK"
      , &snapcommunicator_emulator::msg_forward
    },
    {
        "UNLOCKED"
      , &snapcommunicator_emulator::msg_unlocked
    },
    {
        "UNREGISTER"
      , &snapcommunicator_emulator::msg_unregister
    }
};


snapcommunicator_emulator::snapcommunicator_emulator(test_multi_snaplocks_pointer_t test, int port)
    : snap_timer(-1)
    , dispatcher(this, g_snapcommunicator_emulator_service_messages)
    , f_test(test)
    , f_port(port)
{
    add_snap_communicator_commands();
    set_trace();
    set_timer();
}


snapcommunicator_emulator::~snapcommunicator_emulator()
{
    cleanup();
}


void snapcommunicator_emulator::cleanup()
{
    if(f_listener != nullptr)
    {
        f_listener->stop();
        snap::snap_communicator::instance()->remove_connection(f_listener);
        f_listener.reset();
    }
}


void snapcommunicator_emulator::set_timer()
{
    int64_t const duration(rand() % 200 + 30);
    set_timeout_date((time(nullptr) + duration) * 1000000LL);
}


snapcommunicator_emulator::pointer_t snapcommunicator_emulator::shared_from_this()
{
    return std::dynamic_pointer_cast<snapcommunicator_emulator>(snap_timer::shared_from_this());
}


void snapcommunicator_emulator::start()
{
    SNAP_LOG_INFO("***** start communicator (")(get_port())(") *****");

    f_listener.reset(new snapcommunicator_listener(f_test, shared_from_this(), f_port));
    if(!snap::snap_communicator::instance()->add_connection(f_listener))
    {
        throw test_exception_exit("error: could not add the messager connection to snap communicator.");
    }

    // generate a random object name for our LOCK tests
    // however, make sure many of the names are the exact same
    //
    f_object_name = QString("lock:%1").arg(rand() % 5 + 1);
}


int snapcommunicator_emulator::get_port() const
{
    return f_port;
}


bool snapcommunicator_emulator::is_connected() const
{
    if(f_listener == nullptr)
    {
        return false;
    }

    return f_listener->is_connected();
}


bool snapcommunicator_emulator::is_locked(QString const & object_name) const
{
    return f_object_name == object_name
        && f_locked;
}


bool snapcommunicator_emulator::send_message(snap::snap_communicator_message const & message, bool cache)
{
    if(f_listener == nullptr)
    {
        throw test_exception_exit("the listener of this snapcommunicator_emulator (127.0.0.1:"
                  + std::to_string(f_port)
                  + ") is not yet in place.");
    }

    return f_listener->send_message(message, cache);
}


void snapcommunicator_emulator::process_timeout()
{
    // can we even send a message to that one?
    //
    if(!is_connected())
    {
        // try again later
        //
        set_timer();
    }
    else if(f_locked)
    {
        // in this case we got a LOCK and want to send a clean UNLOCK to
        // release the lock (instead of letting it time out)
        //
        snap::snap_communicator_message unlock_message;
        unlock_message.set_command("UNLOCK");
        unlock_message.set_service(QString("snap%1_service").arg(f_port));
        unlock_message.set_sent_from_server(QString("snap%1").arg(f_port));
        unlock_message.set_sent_from_service(QString("backend%1_service").arg(f_port));
        unlock_message.add_parameter("object_name", f_object_name);
        unlock_message.add_parameter("pid", getpid());
        send_message(unlock_message, true);
    }
    else
    {
        // attempt a LOCK and see what happens
        //
        // the expected answer is either one of:
        //
        //    LOCKED
        //    LOCKFAILED
        //
        // if we get LOCKED, then we expect an UNLOCKED at some point (once it
        // times out)
        //

        int const obtention(rand() % 60 + 5);
        int const duration(rand() % 120 + 20);

std::cerr << "---- sending a LOCK #" << f_port << " message (" << obtention << ", " << duration << ", " << f_object_name << ")\n";
        snap::snap_communicator_message lock_message;
        lock_message.set_command("LOCK");
        lock_message.set_service(QString("snap%1_service").arg(f_port));
        lock_message.set_sent_from_server(QString("snap%1").arg(f_port));
        lock_message.set_sent_from_service(QString("backend%1_service").arg(f_port));
        lock_message.add_parameter("object_name", f_object_name);
        lock_message.add_parameter("pid", getpid());
        lock_message.add_parameter("timeout", time(nullptr) + obtention);
        lock_message.add_parameter("duration", duration);
        send_message(lock_message, true);
    }
}


bool snapcommunicator_emulator::need_to_forward_message(snap::snap_communicator_message & message, void (snapcommunicator_emulator::*func)(snap::snap_communicator_message & message))
{
    // in case of the LOCKFAILED it could be sent to another snaplock
    //
    QString const service_name(message.get_service());
    if(service_name == "snaplock")
    {
        msg_forward(message);
        return true;
    }

    // the sender may specify another emulator as the destination so we
    // have to verify before we check anything more
    //
    QString const server_name(message.get_server());
    if(server_name.startsWith("snap"))
    {
        bool ok(false);
        int const port(server_name.mid(4).toInt(&ok, 10));
        if(ok
        && port != f_port)
        {
            // find the correct destination and forward the message there
            //
            f_test->forward_message(message, port, func);
            return true;
        }
    }
    return false;
}


void snapcommunicator_emulator::msg_callback(snap::snap_communicator_message & message)
{
    message.set_sent_from_server(QString("snap%1").arg(f_port));
    message.set_sent_from_service(QString("snap%1_service").arg(f_port));
}


void snapcommunicator_emulator::msg_forward(snap::snap_communicator_message & message)
{
    QString const service(message.get_service());
    if(service == "*")
    {
        f_test->broadcast(message, f_port);
    }
    else
    {
        int port(0);

        // extract the port from the service name so we know to whom
        // to send this message
        //     "snap" + std::to_string(f_port) + "_service";
        //
        if(!service.startsWith("snap"))
        {
            throw test_exception_exit("service name does not start with \"snap\"? [" + service + "]");
        }
        int const minus(service.indexOf('_', 4));
        if(minus == -1)
        {
            if(service != "snaplock")
            {
                throw test_exception_exit("service name does not include an underscore? [" + service + "]");
            }

            // we may need to use the server instead
            //
            QString const server(message.get_server());
            if(!server.startsWith("snap"))
            {
                throw test_exception_exit("service is \"snapservice\" and server name does not start with \"snap\"? [" + service + "/" + server + "]");
            }
            bool ok(false);
            port = server.mid(4).toInt(&ok, 10);
            if(!ok
            || port < 1000
            || port > 65535)
            {
                throw test_exception_exit("server name does not include a valid port? [" + server + "]");
            }
        }
        else
        {
            if(service.mid(minus) != "_service")
            {
                throw test_exception_exit("service name does not end with the workd \"service\"? [" + service + "]");
            }
            bool ok(false);
            port = service.mid(4, minus - 4).toInt(&ok, 10);
            if(!ok
            || port < 1000
            || port > 65535)
            {
                throw test_exception_exit("service name does not include a valid port? [" + service + "]");
            }
        }
        f_test->send_message(message, port);
    }
}


void snapcommunicator_emulator::msg_cluster_status(snap::snap_communicator_message & message)
{
    int const count(f_test->get_count());
    int const quorum(count / 2 + 1);
    int const number_of_connections(f_test->get_number_of_connections());

    QString const cluster_status(number_of_connections >= quorum ? "CLUSTERUP" : "CLUSTERDOWN");

std::cerr << "cluster status now is: " << count << " vs " << number_of_connections << " vs " << quorum << " -> " << cluster_status << "\n";

    // always send the status so here we go
    //
    snap::snap_communicator_message cluster_status_msg;
    cluster_status_msg.set_command(cluster_status);
    cluster_status_msg.reply_to(message);
    cluster_status_msg.add_parameter("neighbors_count", count);
    send_message(cluster_status_msg);
}


void snapcommunicator_emulator::msg_commands(snap::snap_communicator_message & message)
{
    // we ignore this one, but print info to logs at least
    //
    SNAP_LOG_INFO("received COMMANDS \"")
                 (message.get_parameter("list"))
                 ("\".");
}


void snapcommunicator_emulator::msg_locked(snap::snap_communicator_message & message)
{
    if(need_to_forward_message(message, &snapcommunicator_emulator::msg_locked))
    {
        return;
    }

    // acknowledge that we got a lock
    // then next we should get an UNLOCKED when it times out
    //
    if(message.get_parameter("object_name") != f_object_name)
    {
        throw test_exception_exit("expected lock \"" + f_object_name + "\" but got \"" + message.get_parameter("object_name") + "\" instead.");
    }

    time_t const timeout_date(message.get_integer_parameter("timeout_date"));
    int64_t const diff(timeout_date - time(nullptr));
    int64_t const two_diff(diff * 2);

    int use_duration(rand() % two_diff);
    if(diff >= 15                       // this should always be true since we do +20 to the duration
    && use_duration < diff - 20)
    {
        // we want to send a clean UNLOCK instead of waiting for it
        // to timeout; this is our normal case usage so it makes sense
        // to test it a lot (i.e. roughly 50% of the time)
        //
        set_timeout_date((time(nullptr) + use_duration) * 1000000LL);
    }

std::cerr << "received LOCKED! for #" << f_port << " / " << f_object_name << " so we got a successful lock.\n";

    // check that no other emulator using the same object name
    // is currently locked because if so that's a HUGE bug
    // (i.e. two computers asking for the same LOCK and they
    // both got the lock simultaneously!)
    //
    f_test->verify_lock(f_object_name, f_port);

    f_locked = true;
}


void snapcommunicator_emulator::msg_lockfailed(snap::snap_communicator_message & message)
{
    // we don't break if we receive the message in the wrong emulator
    // however, we need to call set_timer() on the correct emulator
    //
    if(need_to_forward_message(message, &snapcommunicator_emulator::msg_lockfailed))
    {
        return;
    }

    SNAP_LOG_INFO("failed lock #")
                 (f_port)
                 (" for ")
                 (message.get_parameter("object_name"))
                 (" (error: ")
                 (message.get_parameter("error"))
                 (")");

    std::cerr << "failed lock #"
              << f_port
              << " ("
              << message.get_parameter("error")
              << ")"
              << std::endl;

    set_timer();
}


void snapcommunicator_emulator::msg_lockready(snap::snap_communicator_message & message)
{
    snap::NOTUSED(message);

    SNAP_LOG_INFO("told that locks of ")(f_port)(" are now ready.");

    // at the start the death timer is turned off because otherwise it could
    // happen while building the cluster and at this point we did not want
    // to test that part; so we set it up and running once the lock system
    // is (finally) ready
    //
    f_test->set_death_timer_status(true);

}


void snapcommunicator_emulator::msg_nolock(snap::snap_communicator_message & message)
{
    snap::NOTUSED(message);

    SNAP_LOG_INFO("told that locks of ")(f_port)(" are not yet available (or not available anymore.");

    // just in case, stop that when we get a NOLOCK status
    //
    f_test->set_death_timer_status(false);
}


void snapcommunicator_emulator::msg_register(snap::snap_communicator_message & message)
{
    snap::snap_communicator_message register_snaplock;
    register_snaplock.set_command("READY");
    register_snaplock.reply_to(message);
    send_message(register_snaplock);
}


void snapcommunicator_emulator::msg_ticketlist(snap::snap_communicator_message & message)
{
    // TBD -- we should remove this item from our lists
    // (right now we don't really "allow" UNREGISTER to happen, although
    // it does when we kill a snaplock process)
    //
    std::cerr << "got TICKETLIST"
              << std::endl
              << "result:"
              << std::endl
              << message.get_parameter("list")
              << std::endl;
}


void snapcommunicator_emulator::msg_unlocked(snap::snap_communicator_message & message)
{
    // unlocked need forwarding in our test
    //
    if(need_to_forward_message(message, &snapcommunicator_emulator::msg_unlocked))
    {
        return;
    }

    // TBD -- we should remove this item from our lists
    // (right now we don't really "allow" UNREGISTER to happen, although
    // it does when we kill a snaplock process)
    //
    if(message.get_parameter("object_name") != f_object_name)
    {
        throw test_exception_exit("msg_unlocked(): expected lock object \"" + f_object_name + "\" but got \"" + message.get_parameter("object_name") + "\" instead.");
    }

    if(!f_locked)
    {
        SNAP_LOG_ERROR("got an UNLOCK message for ")
                      (message.get_parameter("object_name"))
                      (" which wasn't locked (did not receive a LOCKED message for--or we died in between?)");
        std::cerr << "*** error: got an UNLOCK message for "
                  << message.get_parameter("object_name")
                  << " which wasn't locked (did not receive a LOCKED message for--or we died in between?)" << std::endl;
    }
    else
    {
        f_locked = false;

        // we must acknowledge if the UNLOCKED is a timed out UNLOCKED
        //
        if(message.has_parameter("error"))
        {
            snap::snap_communicator_message unlock_snaplock;
            unlock_snaplock.set_command("UNLOCK");
            unlock_snaplock.set_service(QString("snap%1_service").arg(f_port));
            unlock_snaplock.set_sent_from_server(QString("snap%1").arg(f_port));
            unlock_snaplock.set_sent_from_service(QString("backend%1_service").arg(f_port));
            unlock_snaplock.add_parameter("object_name", f_object_name);
            unlock_snaplock.add_parameter("pid", getpid());
            unlock_snaplock.reply_to(message);
            send_message(unlock_snaplock);
        }

std::cerr << "received UNLOCKED! for #" << f_port << " / " << f_object_name << " so it timed out as expected (TODO: send UNLOCK before the timeout).\n";
    }

    set_timer();
}


void snapcommunicator_emulator::msg_unregister(snap::snap_communicator_message & message)
{
    // TBD -- we should remove this item from our lists
    // (right now we don't really "allow" UNREGISTER to happen, although
    // it does when we kill a snaplock process)
    //
    std::cerr << "got UNREGISTER ("
              << f_port
              << "/"
              << message.get_parameter("service")
              << ")"
              << std::endl;
}


void snapcommunicator_emulator::mark_unlocked()
{
    f_locked = false;
}






/***************************************************************************
 *** SNAPLOCK EXECUTABLE ***************************************************
 ***************************************************************************/


snaplock_executable::snaplock_executable(test_multi_snaplocks_pointer_t test, int port, std::string const & snaplock_path, std::string const & config_path)
    : snap_timer(-1)
    , f_port(port)
    , f_snaplock_executable(snaplock_path)
    , f_config_path(config_path)
    , f_test(test)
{
}


snaplock_executable::~snaplock_executable()
{
    stop();
}


void snaplock_executable::start()
{
    SNAP_LOG_INFO("***** start snaplock (")(get_port())(") *****");

    if(f_child != static_cast<pid_t>(-1))
    {
        throw test_exception_exit("this snaplock executable is currently running.");
    }

    // TODO: look in a non-blocking way so we can attempt to stop the
    //       process cleanly?
    //
    f_child = fork();
    if(f_child < 0)
    {
        throw test_exception_exit("could not fork to start snaplock daemon.");
    }

    // WARNING: we want to generate the priority here otherwise rand() always
    //          return the same number within the child (i.e. it's always the
    //          same seed since we fork() and no other rand() occur in the
    //          parent in between...)
    //
    int random(rand());
#if 0
    int priority(random & 256 ? 15 : random % 15 + 1);  // use to have many 15 priorities (i.e. "off")
#else
    int priority(random % 15 + 1);                      // our regular 1 to 15 priority
#endif

    if(f_child == 0)
    {
        // in the child
        //

        // make sure to disconnect all snapcommunicator connections
        // because those are from snapcommunicator listener/messenger
        // and we do not want them in the child
        //
        // (this is a "sad" side effect of this test)
        //
        f_test->remove_communicators_and_locks();

        f_test->close_connections(true);

        // now run snaplock
        //
        // we use execvp() because we do not want to change pid_t
        // (i.e. so that way a kill() on this child PID will signal the
        // snaplock process)
        //
        snap::mkdir_p(f_config_path);

        snap::snap_config snapcommunicator_config(f_config_path + "/snapcommunicator.conf");
        snapcommunicator_config.save(false);
        snapcommunicator_config["local_listen"] = "127.0.0.1:" + std::to_string(f_port);
        snapcommunicator_config["listen"] = "10.10.10.10:" + std::to_string(f_port);
        snapcommunicator_config.save(false);

        snap::snap_config snaplock_config(f_config_path + "/snaplock.conf");
        snaplock_config.save(false);
        snaplock_config["server_name"] = "snap" + std::to_string(f_port);
        snaplock_config["service_name"] = "snap" + std::to_string(f_port) + "_service";
        snaplock_config["candidate_priority"] = priority == 15 ? std::string("off") : std::to_string(priority);
        snaplock_config["debug_lock_messages"] = "on";
        snaplock_config.save(false);

        std::vector<std::string> args;
        args.push_back(f_snaplock_executable);
        args.push_back("--debug");
        args.push_back("--config");
        args.push_back(f_config_path);
        int const args_max(args.size());

        std::vector<char const *> args_strings;
        args_strings.reserve(args_max + 1);
        for(int i(0); i < args_max; ++i)
        {
            args_strings.push_back(strdup(args[i].c_str()));
        }
        args_strings.push_back(nullptr); // NULL terminated

        execvp(
            f_snaplock_executable.c_str(),
            const_cast<char * const *>(&args_strings[0])
        );
        int const e(errno);

        // execvp() failed?!
        //
        throw test_exception_exit("error: execvp() failed to start snaplock ("
                  + f_snaplock_executable
                  + ") with errno: "
                  + std::to_string(e)
                  + ", "
                  + strerror(e));
    }
}


void snaplock_executable::stop(int sig)
{
    if(f_child != -1)
    {
        int const k(kill(f_child, sig));
        if(k != 0)
        {
            throw test_exception_exit("error: could not send SIGTERM to child (already dead?)");
        }
        SNAP_LOG_TRACE("kill() called with ")(f_child)(" and signal #")(sig);

        // we do not want to just wait here, the loop will get the SIGCHLD
        //wait_child();
    }
}


void snaplock_executable::wait_child()
{
    // nothing to wait on at the moment
    //
    if(f_child == -1)
    {
        return;
    }

    // TODO: if the SIGTERM doesn't stop snaplock we are going to be stuck here
    //
    int status(0);
    int const r(waitpid(f_child, &status, 0));
    if(r == -1)
    {
        throw test_exception_exit("error: waitpid() failed in snaplock_runner::run().");
    }
    if(WIFEXITED(status))
    {
        int const e(WEXITSTATUS(status));
        if(e != 0)
        {
            std::cerr << "warning: snaplock daemon exited with exit code: " << e << std::endl;
        }
    }
    else if(WIFSIGNALED(status))
    {
        std::cerr << "warning: snaplock daemon exited because of signal: " << WTERMSIG(status) << std::endl;
    }

    f_child = static_cast<pid_t>(-1);

    // this means we need to send a HANGUP to all the other processes
    //
    snap::snap_communicator_message hangup_msg;
    hangup_msg.set_command("HANGUP");
    hangup_msg.add_parameter("server_name", QString("snap%1").arg(f_port));
    f_test->broadcast(hangup_msg, f_port);

    // setup the timer so we can restart the snaplock soon
    // (between 5 and 120 seconds)
    //
    int64_t duration(rand() % 116 + 5);
    set_timeout_date((time(nullptr) + duration) * 1000000LL);
}


int snaplock_executable::get_port() const
{
    return f_port;
}


pid_t snaplock_executable::get_child_pid() const
{
    return f_child;
}


void snaplock_executable::process_timeout()
{
    // when the timer times out we had a hang up and we want to restart
    // the snaplock process
    //
    start();
}










/***************************************************************************
 *** COMMUNICATOR AND LOCK *************************************************
 ***************************************************************************/




communicator_and_lock::communicator_and_lock(test_multi_snaplocks_pointer_t test, int port, std::string const & snaplock_path, std::string const & config_path)
    : f_snaplock_executable(snaplock_path)
    , f_config_path(config_path)
    , f_communicator(std::make_shared<snapcommunicator_emulator>(test, port))
    , f_snaplock(std::make_shared<snaplock_executable>(test, port, snaplock_path, config_path))
{
    if(!snap::snap_communicator::instance()->add_connection(f_communicator))
    {
        // this should never happen here since each new creates a
        // new pointer
        //
        throw test_exception_exit("could not add the f_communicator timer to snap_communicator");
    }

    if(!snap::snap_communicator::instance()->add_connection(f_snaplock))
    {
        // this should never happen here since each new creates a
        // new pointer
        //
        throw test_exception_exit("could not add the f_snaplock timer to snap_communicator");
    }

}


communicator_and_lock::~communicator_and_lock()
{
    stop();
}


int communicator_and_lock::get_port() const
{
    return f_communicator->get_port();
}


int64_t communicator_and_lock::start_communicator()
{
    f_communicator->start();
    return 100000LL;
}


int64_t communicator_and_lock::start_snaplock()
{
    f_snaplock->start();
    return 100000LL;
}


int64_t communicator_and_lock::pause()
{
    // wait a little to avoid starting all at the same time
    //
    SNAP_LOG_INFO("***** PAUSE *****");
    return 1000000LL;
}


void communicator_and_lock::stop()
{
    if(f_communicator != nullptr)
    {
        f_communicator->cleanup();
        snap::snap_communicator::instance()->remove_connection(f_communicator);
        f_communicator.reset();
    }

    if(f_snaplock != nullptr)
    {
        f_snaplock->stop();
        snap::snap_communicator::instance()->remove_connection(f_snaplock);
        f_snaplock.reset();
    }
}


bool communicator_and_lock::is_communicator_connected() const
{
    if(f_communicator == nullptr)
    {
        return false;
    }

    return f_communicator->is_connected();
}


bool communicator_and_lock::send_message(snap::snap_communicator_message const & message)
{
    return f_communicator->send_message(message);
}


/** \brief Forget the communicator when in the child.
 *
 * When we fork() to create a snaplock, we must eliminate all the
 * communicator connections. This is the purpose of this function.
 */
void communicator_and_lock::remove_communicator()
{
    f_communicator->cleanup();
    snap::snap_communicator::instance()->remove_connection(f_communicator);
    f_communicator.reset();
}


void communicator_and_lock::remove_snaplock()
{
    snap::snap_communicator::instance()->remove_connection(f_snaplock);
    f_snaplock.reset();
}


void communicator_and_lock::kill_snaplock()
{
    if(f_snaplock != nullptr)
    {
        // select the signal to send
        //
        int const select(rand() % 12);
        int sig(SIGTERM);
        switch(select)
        {
        case 0:
            // a direct kill prevents the snaplock from sending a DISCONNECT
            // and/or give it a chance to cleanly get replaced if it is a
            // leader (although really at this time all signals are terminal
            // and don't give snaplock a chance to do anything, we have to
            // send a STOP to request a clean exit)
            //
            sig = SIGKILL;
            break;

        case 1:
            // we won't send a SIGSTOP, instead we send a STOP message unless
            // there is no messenger and then we send a SIGTERM anyway
            //
            sig = SIGSTOP;
            break;

        case 2:
        case 3:
        case 4:
            sig = SIGINT;
            break;

        case 5:
        case 6:
            sig = SIGQUIT;
            break;

        default:
            sig = SIGTERM;
            break;

        }
        if(sig == SIGSTOP)
        {
            if(f_communicator != nullptr
            && f_communicator->is_connected())
            {
                // do a soft STOP instead of a brutal kill()
                //
                SNAP_LOG_TRACE("sending a STOP message (instead of a signal) as backend #")(f_communicator->get_port());
                snap::snap_communicator_message stop_message;
                stop_message.set_command("STOP");
                stop_message.set_service(QString("snap%1_service").arg(f_communicator->get_port()));
                send_message(stop_message);
            }
            else
            {
                // revert to the default signal if we cannot send
                // the STOP message
                //
                sig = SIGTERM;
            }
        }
        if(sig != SIGSTOP)
        {
            f_snaplock->stop(sig);
            f_communicator->mark_unlocked();
        }
    }
}


snapcommunicator_emulator::pointer_t communicator_and_lock::get_communicator() const
{
    return f_communicator;
}


snaplock_executable::pointer_t communicator_and_lock::get_snaplock() const
{
    return f_snaplock;
}








/***************************************************************************
 *** TEST MULTI SNAPLOCKS **************************************************
 ***************************************************************************/




start_timer::start_timer(test_multi_snaplocks * test)
    : snap_timer(100000LL)  // 100 ms in microseconds
    , f_test(test)
{
}

void start_timer::process_timeout()
{
    f_test->start_next();
}








/***************************************************************************
 *** NEW CONNECTION TIMER **************************************************
 ***************************************************************************/




new_connection_timer::new_connection_timer(test_multi_snaplocks * test)
    : snap_timer(250000LL)  // 250 ms in microseconds
    , f_test(test)
{
    set_enable(false);
}

void new_connection_timer::process_timeout()
{
    f_test->check_cluster_status();
}








/***************************************************************************
 *** DEATH TIMER ***********************************************************
 ***************************************************************************/




death_timer::death_timer(test_multi_snaplocks * test)
    : snap_timer(5LL * 60LL * 1000000LL)  // 5 min. in microseconds
    , f_test(test)
{
    set_enable(false);
}

void death_timer::process_timeout()
{
    f_test->kill_a_snaplock();
}








/***************************************************************************
 *** TEST MULTI SNAPLOCKS **************************************************
 ***************************************************************************/





test_multi_snaplocks::test_multi_snaplocks(int argc, char ** argv)
    : f_opt(g_options_environment, argc, argv)
{
    f_count = f_opt.get_long("count", 0, 1, 1000);
    f_port = f_opt.get_long("port", 0, 1, 65535);
    f_snaplock_executable = f_opt.get_string("snaplock");

    // ensure configuration path is properly setup, we'll manually create
    // config files, one per snaplock instance
    //
    if(f_opt.is_defined("config"))
    {
        f_config_path = f_opt.get_string("config");
    }
    if(snap::mkdir_p(f_config_path) != 0)
    {
        throw test_exception_exit("error: could not create configuration directory \""
                 + f_config_path
                 + "\"; verify that you have enough permissions or change the path with --config <path>");
    }

    if(f_opt.is_defined("seed"))
    {
        srand(f_opt.get_long("seed"));
    }
    else
    {
        unsigned int const seed(static_cast<unsigned int>(time(nullptr)));
        srand(seed);
        std::cerr << "starting with seed: " << seed << ", use --seed to reuse the same seed again and again." << std::endl;
    }

    QFile log(g_log_conf);
    if(!log.exists())
    {
        std::cerr << "error: \""
                  << g_log_conf
                  << "\" does not exist, it is required for this test to start."
                  << std::endl;
        throw test_exception_exit("log property file missing");
    }

    snap::logging::configure_conffile(g_log_conf);

    SNAP_LOG_INFO("--------------------------- starting test_multi_snaplocks");

    // Stop on these signals, log them, then terminate.
    //
    signal( SIGSEGV, sighandler );
    signal( SIGBUS,  sighandler );
    signal( SIGFPE,  sighandler );
    signal( SIGILL,  sighandler );
    //signal( SIGTERM, sighandler );
    //signal( SIGINT,  sighandler );
    signal( SIGQUIT, sighandler );
    signal( SIGALRM, sighandler );
    signal( SIGABRT, sighandler );  // although we can't really return from this one, having the stack trace is useful

    // ignore a few
    //
    signal( SIGPIPE,  SIG_IGN );
    signal( SIGTSTP,  SIG_IGN );
    signal( SIGTTIN,  SIG_IGN );
    signal( SIGTTOU,  SIG_IGN );

}


test_multi_snaplocks::~test_multi_snaplocks()
{
    close_connections(false);
}


void test_multi_snaplocks::sighandler(int sig)
{
    std::string signame;
    bool output_stack_trace(true);
    switch(sig)
    {
        case SIGSEGV : signame = "SIGSEGV"; break;
        case SIGBUS  : signame = "SIGBUS";  break;
        case SIGFPE  : signame = "SIGFPE";  break;
        case SIGILL  : signame = "SIGILL";  break;
        case SIGTERM : signame = "SIGTERM"; output_stack_trace = false; break;
        case SIGINT  : signame = "SIGINT";  output_stack_trace = false; break;
        case SIGQUIT : signame = "SIGQUIT"; output_stack_trace = false; break;
        case SIGALRM : signame = "SIGALRM"; break;
        case SIGABRT : signame = "SIGABRT"; break;
        default      : signame = "UNKNOWN"; break;
    }

    SNAP_LOG_FATAL("POSIX signal caught: ")(signame);

    if(output_stack_trace)
    {
        snap::snap_exception_base::output_stack_trace();
    }

    // we can't safely return from one of these
    //
    ::exit(1);
}


void test_multi_snaplocks::run()
{
    for(int idx(0); idx < f_count; ++idx)
    {
        int const port(f_port + idx);
        communicator_and_lock::pointer_t t(std::make_shared<communicator_and_lock>(
                                  shared_from_this()
                                , port
                                , f_snaplock_executable
                                , f_config_path + "/" + std::to_string(port)));
        f_emulators.push_back(t);

        f_start.push_back(std::bind(&communicator_and_lock::start_communicator, t.get()));
        f_start.push_back(std::bind(&communicator_and_lock::start_snaplock, t.get()));

        // once in a while also add a small pause
        //
        if(rand() % 5 == 0)
        {
            f_start.push_back(std::bind(&communicator_and_lock::pause, t.get()));
        }
    }

    f_signal_ctrl_c = std::make_shared<signal_ctrl_c>(this);
    if(!snap::snap_communicator::instance()->add_connection(f_signal_ctrl_c))
    {
        throw test_exception_exit("could not add signal ctrl-c connection to snap communicator.");
    }

    f_signal_terminate = std::make_shared<signal_terminate>(this);
    if(!snap::snap_communicator::instance()->add_connection(f_signal_terminate))
    {
        throw test_exception_exit("could not add signal TERM connection to snap communicator.");
    }

    f_signal_child_death = std::make_shared<signal_child_death>(this);
    if(!snap::snap_communicator::instance()->add_connection(f_signal_child_death))
    {
        throw test_exception_exit("could not add signal child death connection to snap communicator.");
    }

    // randomize the start procedure
    //
    //std::random_shuffle(f_start.begin(), f_start.end()); -- this has problems with copy/move
    f_start_indexes.clear();
    for(size_t idx(0); idx < f_start.size(); ++idx)
    {
        f_start_indexes.push_back(idx);
    }
    std::random_shuffle(f_start_indexes.begin(), f_start_indexes.end());

    // get the start timer
    //
    f_start_timer = std::make_shared<start_timer>(this);
    if(!snap::snap_communicator::instance()->add_connection(f_start_timer))
    {
        throw test_exception_exit("could not add start timer to snap communicator.");
    }

    // get the new connection timer so we can check CLUSTERUP status a little
    // after it happens
    //
    f_new_connection_timer = std::make_shared<new_connection_timer>(this);
    if(!snap::snap_communicator::instance()->add_connection(f_new_connection_timer))
    {
        throw test_exception_exit("could not add new connection timer to snap communicator.");
    }

    // add a timer used to kill snaplock daemons once in a while to test
    // that such a loss does not break the locking mechanism; note that
    // the timer is quite slow (hence you need to run this test for a
    // while to make sure everything works well, like 24 hours...) which
    // means we do not try to get edge cases as when two leaders die
    // pretty much simultaneously, losing the cluster, etc.
    //
    f_death_timer = std::make_shared<death_timer>(this);
    if(!snap::snap_communicator::instance()->add_connection(f_death_timer))
    {
        throw test_exception_exit("could not add death timer to snap communicator.");
    }

    std::cerr << std::endl;
    snap::snap_communicator::instance()->run();
}


void test_multi_snaplocks::start_next()
{
    if(f_start_indexes.empty())
    {
std::cerr << "all started now!\n";
        snap::snap_communicator::instance()->remove_connection(f_start_timer);
        f_start_timer.reset();
        return;
    }

    size_t const idx(f_start_indexes[0]);
    f_start_indexes.erase(f_start_indexes.begin());

std::cerr << "start " << idx << " now.\033[K\r";
    int64_t const pause(f_start[idx]());
    f_start_timer->set_timeout_delay(pause);
}


void test_multi_snaplocks::stop()
{
    std::for_each(
              f_emulators.begin()
            , f_emulators.end()
            , [](auto & e)
            {
                e->stop();
            });
    f_emulators.clear();
    f_start.clear();

    close_connections(false);
}


void test_multi_snaplocks::capture_zombie(pid_t child)
{
    // search for that snaplock that died and call its wait_child() function
    //
    for(auto & e : f_emulators)
    {
        auto l(e->get_snaplock());
        if(l->get_child_pid() == child)
        {
            l->wait_child();
            return;
        }
    }

    std::cerr << "warning: could not find snaplock " << child << std::endl;
}


void test_multi_snaplocks::set_death_timer_status(bool status)
{
    if(f_death_timer != nullptr)
    {
        f_death_timer->set_enable(status);
    }
}


int test_multi_snaplocks::get_count() const
{
    return f_emulators.size();
}


int test_multi_snaplocks::get_number_of_connections() const
{
    int result(0);

    for(auto e : f_emulators)
    {
        if(e->is_communicator_connected())
        {
            ++result;
        }
    }

    return result;
}


bool test_multi_snaplocks::send_message(snap::snap_communicator_message const & message, int port)
{
    for(auto e : f_emulators)
    {
        if(e->get_port() == port)
        {
            if(!e->is_communicator_connected())
            {
                // this happens whenever a snaplock gets killed while we
                // are about to send it a message
                //
                // we should be able to know and avoid the send_message()
                // though, but right now it still happens...
                //
                SNAP_LOG_ERROR("the snapcommunicator for port ")
                          (std::to_string(port))
                          (" is not available.");
                return false;
            }
            return e->send_message(message);
        }
    }

    throw test_exception_exit("no snapcommunicator with port "
              + std::to_string(port)
              + " was found in test_multi_snaplocks::send_message().");
}


bool test_multi_snaplocks::broadcast(snap::snap_communicator_message const & message, int except_port)
{
    bool result(true);
    for(auto e : f_emulators)
    {
        if(e->get_port() != except_port
        && e->is_communicator_connected())
        {
            if(!e->send_message(message))
            {
                result = false;
            }
            //else
            //{
            //    SNAP_LOG_INFO("broadcast to ")(e->get_port());
            //}
        }
    }

    return result;
}


void test_multi_snaplocks::forward_message(snap::snap_communicator_message & message, int port, void (snapcommunicator_emulator::*func)(snap::snap_communicator_message & message))
{
    for(auto e : f_emulators)
    {
        if(e->get_port() == port)
        {
            if(e->is_communicator_connected())
            {
                (*e->get_communicator().*func)(message);
            }
            else
            {
                std::cerr << "----- error? trying to forward to a snapcommunicator which is not ready?" << std::endl;
            }
            return;
        }
    }

    throw test_exception_exit("no snapcommunicator with port "
              + std::to_string(port)
              + " was found in test_multi_snaplocks::forward_message().");
}


void test_multi_snaplocks::remove_communicators_and_locks()
{
    for(auto e : f_emulators)
    {
        e->remove_communicator();
        e->remove_snaplock();
    }
}


void test_multi_snaplocks::close_connections(bool force_close)
{
    // WARNING WARNING WARNING
    //
    // When we fork() to create a snaplock daemon, we call this function to
    // remove all the connections in the test_multi_snaplocks object.
    //
    // However, the child fork() never returns. This means the
    // snap_communicator::run() function keeps a copy of the array of
    // connections, including those below. In other words, none of the
    // connections get deleted. This is why we have to manually call
    // the close() function of the signals. Without that explicit
    // call, the signalfd() is still open and especially, the masked
    // signals are still masked in the child process (it is carried
    // through the fork() and execve() calls.)
    //
    snap::snap_communicator::instance()->remove_connection(f_signal_ctrl_c);
    if(force_close)
    {
        f_signal_ctrl_c->close();
    }
    f_signal_ctrl_c.reset();

    snap::snap_communicator::instance()->remove_connection(f_signal_terminate);
    if(force_close)
    {
        f_signal_terminate->close();
    }
    f_signal_terminate.reset();

    snap::snap_communicator::instance()->remove_connection(f_signal_child_death);
    f_signal_child_death->close();
    f_signal_child_death.reset();

    snap::snap_communicator::instance()->remove_connection(f_start_timer);
    f_start_timer.reset();

    snap::snap_communicator::instance()->remove_connection(f_new_connection_timer);
    f_new_connection_timer.reset();

    snap::snap_communicator::instance()->remove_connection(f_death_timer);
    f_death_timer.reset();
}


void test_multi_snaplocks::received_new_connection()
{
    f_new_connection_timer->set_enable(true);
}


void test_multi_snaplocks::check_cluster_status()
{
    f_new_connection_timer->set_enable(false);

    // we simulate the CLUSTERUP using the number of connections from
    // snaplock instead of other snapcommunicators... so it's a bit
    // dodge but it works as expected
    //
    int const count(get_count());
    int const quorum(count / 2 + 1);
    int const connections(get_number_of_connections());
    QString const new_status(connections >= quorum ? "CLUSTERUP" : "CLUSTERDOWN");
    if(f_cluster_status != new_status)
    {
        f_cluster_status = new_status;

        SNAP_LOG_INFO("+++ CLUSTER STATUS CHANGED TO ")(f_cluster_status)(" SENT AFTER START ENOUGH COMMUNICATORS +++");
std::cerr << "CLUSTER IS UP! status now is: " << count << " vs " << connections << " vs " << quorum << " -> " << f_cluster_status << "\n";

        snap::snap_communicator_message cluster_status_msg;
        cluster_status_msg.set_command(f_cluster_status);
        cluster_status_msg.add_parameter("neighbors_count", count);
        broadcast(cluster_status_msg, -1);
    }
}


void test_multi_snaplocks::verify_lock(QString const & object_name, int port)
{
    for(auto const & e : f_emulators)
    {
        if(e->get_communicator() != nullptr
        && e->get_communicator()->is_locked(object_name))
        {
            throw test_exception_exit(
                    QString("expected lock \"%1\" to be unique, but user %2 holds it now so %3 cannot get it too!")
                            .arg(object_name)
                            .arg(e->get_port())
                            .arg(port));
        }
    }
}


void test_multi_snaplocks::kill_a_snaplock()
{
    if(!f_emulators.empty())
    {
        int const idx(rand() % f_emulators.size());
        f_emulators[idx]->kill_snaplock();
    }
}






}
// snap_test namespace








int main(int argc, char *argv[])
{
    try
    {
        snap_test::test_multi_snaplocks::pointer_t test(new snap_test::test_multi_snaplocks(argc, argv));

        test->run();
    }
    catch( advgetopt::getopt_exit const & except )
    {
        return except.code();
    }
    catch(std::exception const & e)
    {
        std::cerr << "error: caught an exception: \"" << e.what() << "\"." << std::endl;
        exit(1);
    }

    exit(0);
}

// vim: ts=4 sw=4 et
